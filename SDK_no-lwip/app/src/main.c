#include "stdio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xemaclite.h"
#include "xintc.h"
#include "mb_interface.h"
#include "xil_exception.h"

#define MY_MAC_ADDRESS			{0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
#define MY_MAC_ADDRESS_INV		{0x02, 0x01, 0x00, 0x35, 0x0a, 0x00}
#define DESTINATION_MAC_ADDRESS	{0x34, 0x97, 0xF6, 0xDB, 0x8E, 0x1F}
#define BROADCAST_MAC_ADDRESS	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MY_IP_ADDRESS			{192, 168, 1, 200}
#define MY_IP_ADDRESS_INV			{200, 1, 168, 192}
#define DESTINATION_IP_ADDRESS	{192, 168, 1, 130}
#define NET_MASK				{255, 255, 255, 0}
#define GW_ADDRESS				{192, 168, 1, 1}
#define DEFAULT_PORT			5001
#define LEDS_ADDR XPAR_GPIO_1_BASEADDR
/* this number is not magic, reading the datasheet: PHYAD are 5 bits set on PCB to be 0b11111 = 0x1F  */
	#define PHY_ADDRESS 0x1F
#define LEDS_REGISTER 0x18
#define BMCR_REGISTER 0x00
#define MAC_ADDRESS_LENGTH 6
#define IP_ADDRESS_LENGTH 4
#define ETHERTYPE_LENGTH 2
#define ETHERNET_FCS_LENGTH 4

typedef enum
{
	IPv4 = 0x0800,
	IPv4_big_endian = 0x0008, /* really it is 0x0800, but for speed going to compare directly with 0x0008 because this processor is little endian */
	ARP = 0x0806,
	ARP_big_endian = 0x0608, /* really it is 0x0806, but for speed going to compare directly with 0x0608 because this processor is little endian */
	WOL = 0x0842
} ethertype_t;

typedef enum
{
	RESERVED = 0b100,
	DONT_FRAGMENT = 0b010,
	MORE_FRAGMENTS = 0b001
} flags_t;

typedef enum
{
	BROADCAST = 0xFF,
	MINE = 0x01,
	UNKNOWN = 0x00
} eth_frame_mac_t;

typedef enum
{
	ICMP = 0x01,
	UDP = 0x11
} ipv4_protocol_t;

typedef enum
{
	ECHO_REPLY = 0x00, /* type field is enumerated here, code field is required 0 for both */
	ECHO_REQUEST = 0x08
} icmp_type_t;

typedef enum
{
	ARP_REQUEST_WINDOWS = 0x0001,
	ARP_REPLY_WINDOWS = 0x0002,
	ARP_REQUEST_LINUX = 0x0100,
	ARP_REPLY_LINUX = 0x0200
} arp_opcodes_t;

//#define READ_PHY_REGISTERS
//#define PLAY_WITH_PHY_LEDS
//#define CHECK_LEDS
//#define CHECK_LOOPBACK_IS_DISABLED
//#define GENERATE_ICMP_PING_REQUEST_PROGRAMATICALLY
//#define SEND_ICMP_PING_GENERATED_PROGRAMATICALLY_TO_PC
//#define SEND_ICMP_PING_REQUEST_COPYING
//#define BROADCAST_ARP_REPLY
//#define DEBUGGING

#ifdef SEND_ICMP_PING_REQUEST_COPYING
	#define DESTINATION_MAC_OFFSET					0
	#define SOURCE_MAC_OFFSET						6
	#define ETHERTYPE_OFFSET						12
	#define IP_VERSION_HEADER_LENGTH_OFFSET			14
	#define DIFFERENTIATED_SERVICES_OFFSET			15
	#define TOTAL_LENGTH_OFFSET						16
	#define IDENTIFICATION_OFFSET					18
	#define FLAGS_FRAGMENTATION_OFFSET				20
	#define TTL_OFFSET								22
	#define PROTOCOL_OFFSET							23
	#define IP_HEADER_CHECKSUM_OFFSET				24
	#define SOURCE_IP_OFFSET						26
	#define DESTINATION_IP_OFFSET					30
	#define ICMP_TYPE_OFFSET						34
	#define ICMP_CODE_OFFSET						35
	#define ICMP_CHECKSUM_OFFSET					36
	#define ICMP_IDENTIFIER_OFFSET					38
	#define ICMP_SEQUENCE_OFFSET					40
	#define ICMP_TIMESTAMP_OFFSET					42
	#define	ICMP_PAYLOAD_OFFSET						50
	u8 ping_frame[] =
	{
		0x34,0x97,0xF6,0xDB,0x8E,0x1F, /* destination MAC */
		0x00,0x0a,0x35,0x00,0x01,0x02, /* source MAC */
		0x08,0x00, /* ethertype */
		0x45, /* IP version and header length */
		0x00, /* Differentiated Services Field */
		0x00,0x1C, /* total length */
		0x00,0x01, /* identification */
		0x40,0x00, /* flags (don't fragment), and fragment offset (0x00) */
		0xFF, /* TTL */
		0x01, /* protocol (ICMP) */
		0xf7,0x44, /* header checksum */
		0xC0,0xA8,0x01,0xC8, /* source IP */
		0xC0,0xA8,0x01,0x82, /* destination IP */
		0x08,0x00, /* ICMP type and code (echo request) */
		0xF7,0xFD, /* checksum */
		0x00,0x01, /* identifier */
		0x00,0x00/*,*/ /* sequence number */
		//0x10,0x14,0x79,0x58,0x00,0x00,0x00,0x00, /* information (timestamp) */
		//0x87,0x68,0x04,0x00,0x00,0x00,0x00,0x00,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37 /* payload data */
	};
#endif

u8 arp_response[] =
{
	0xff,0xff,0xff,0xff,0xff,0xff, /* ethernet frame destination mac */
	0x00,0x0a,0x35,0x00,0x01,0x02, /* ethernet frame source mac */
	0x08,0x06, /* ethertype (ARP) */
	0x00,0x01, /* ARP hardware type */
	0x08,0x00, /* ARP protocol type */
	0x06, /* ARP hardware address size in bytes (6 bytes for ethernet mac address) */
	0x04, /* ARP length of protocol addresses in bytes, IPv4 is 4 bytes */
	0x00,0x02, /* ARP operation code, 1 for request, 2 for reply */
	0x00,0x0a,0x35,0x00,0x01,0x02, /* ARP sender hardware address */
	0xc0,0xa8,0x01,0xc8, /* ARP sender protocol address */
	0xff,0xff,0xff,0xff,0xff,0xff, /* ARP target hardware address */
	0xc0,0xa8,0x01,0x01, /* ARP target protocol address */
};

/* the mac address of the board. this should be unique per board */
u8 my_mac_address[MAC_ADDRESS_LENGTH] = MY_MAC_ADDRESS;
u8 destination_mac_address[MAC_ADDRESS_LENGTH] = DESTINATION_MAC_ADDRESS;
u8 broadcast_mac_address[MAC_ADDRESS_LENGTH] = BROADCAST_MAC_ADDRESS;
u8 my_ip_address[IP_ADDRESS_LENGTH] = MY_IP_ADDRESS;
u8 my_ip_address_inv[IP_ADDRESS_LENGTH] = MY_IP_ADDRESS_INV;
u8 destination_ip_address[IP_ADDRESS_LENGTH] = DESTINATION_IP_ADDRESS;
u8 * board_leds = (u8*) LEDS_ADDR;
XEmacLite emaclite_inst;
XIntc intc_inst;
XIntc_Config * config_intc;
u8 buffer[2048] = {'\0'};
u16 packetlen = 0;
u16 data = 0;
int response = XST_SUCCESS;

typedef struct __attribute__((__packed__))
{
	u8 destination_mac[MAC_ADDRESS_LENGTH];
	u8 source_mac[MAC_ADDRESS_LENGTH];
	u16 ethertype;
	u8 * payload;
} ethernet_frame_t;

typedef struct __attribute__((__packed__))
{
	u8 version_header_length;
	u8 type_of_service;
	u16 total_length;
	u16 identification;
	u16 flags_offset;
	u8 ttl;
	u8 protocol;
	u16 header_checksum;
	u32 source_ip;
	u32 destination_ip;
	u8 * payload_data;
} ip_frame_t;

typedef struct __attribute__((__packed__))
{
	u8 type_of_message;
	u8 code;
	u16 header_checksum;
	u32 header_data;
	u8 * payload_data;
} icmp_frame_t;

typedef struct __attribute__((__packed__))
{
	u16 hardware_type;
	u16 protocol_type;
	u8 hardware_size;
	u8 protocol_size;
	u16 operation_code;
	u8 sender_mac[MAC_ADDRESS_LENGTH];
	u32 sender_ip;
	u8 target_mac[MAC_ADDRESS_LENGTH];
	u32 target_ip;
} arp_frame_t;

typedef struct
{
	volatile Xboolean packet_received;
	u16 packet_received_length;
	volatile Xboolean packet_sent;
} system_t;

system_t sys;
ethernet_frame_t * eth_frame_p;
void print_mac_address(u8 * addr);
void print_ip_address(u8 * addr);
void recv_callback(XEmacLite * callbackReference);
void sent_callback(XEmacLite * callbackReference);
u16 calculate_header_checksum_ip(ip_frame_t * packet);
u32 convert_ip_address(u8 * ip_array);
u16 calculate_header_checksum_icmp(icmp_frame_t * packet, u32 data_length);
eth_frame_mac_t check_eth_frame_destination_mac(ethernet_frame_t * frame_p);
ethertype_t check_eth_frame_ethertype(ethernet_frame_t * frame_p);
void calculate_checksum_from_to(icmp_frame_t * frame, u8 * address_from, u8 * address_to);

int main(void)
{
	u8 i = 0;
	u32 my_ip_address_32 = (my_ip_address[3]<<24)|(my_ip_address[2]<<16)|(my_ip_address[1]<<8)|(my_ip_address[0]);
	#ifdef READ_PHY_REGISTERS
		u8 i = 0;
	#endif

	#ifdef PLAY_WITH_PHY_LEDS
		u16 leds_phy_reg = 0;
	#endif

	#ifdef SEND_ICMP_PING_GENERATED_PROGRAMATICALLY_TO_PC
		ip_frame_t ip_frame;
		icmp_frame_t icmp_frame;
		ethernet_frame_t ethernet_frame;
	#endif

	#ifdef GENERATE_ICMP_PING_REQUEST_PROGRAMATICALLY
		#ifndef SEND_ICMP_PING_GENERATED_PROGRAMATICALLY_TO_PC
		ip_frame_t ip_frame;
		icmp_frame_t icmp_frame;
		ethernet_frame_t ethernet_frame;
		#endif
		u16 i = 0;
	#endif

	/* turn off board leds */
	*board_leds = 0b0000;

	/* clears output */
	xil_printf("%c[2J",27);
	xil_printf("----- TFM - Marko Peshevski -----\r\n");

	/* initialize and start system interrupts */
	xil_printf("Initializing Intc hardware peripheral... ");
	if (XIntc_Initialize(&intc_inst, XPAR_INTC_0_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("Error initializing Intc hardware peripheral, returning!\r\n");
	}
	xil_printf("OK!\r\n");

	xil_printf("Starting Intc hardware peripheral... ");
	if (XIntc_Start(&intc_inst, XIN_REAL_MODE) != XST_SUCCESS)
	{
		xil_printf("Could not start Intc hardware peripheral, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	/* register interrupt handler in MB_InterruptVectorTable */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XIntc_InterruptHandler, &intc_inst);

	/* enable specific interrupt in interrupt controller */
	XIntc_EnableIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);

	/* enable interrupt source in system hardware */
	XIntc_Enable(&intc_inst, XPAR_INTC_0_EMACLITE_0_VEC_ID);

	/* connecting interrupt source to ethernet MAC */
	xil_printf("Connecting interrupt source for Emaclite... ");
	if (XIntc_Connect(&intc_inst, XPAR_INTC_0_EMACLITE_0_VEC_ID, (XInterruptHandler) XEmacLite_InterruptHandler, (void *) &intc_inst) != XST_SUCCESS)
	{
		xil_printf("Something failed!\r\n");
	}
	xil_printf("OK!\r\n");

	/* initialize emaclite driver and PHY */
	xil_printf("Initializing Emaclite... ");
	if (XEmacLite_Initialize(&emaclite_inst, XPAR_ETHERNET_MAC_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("Could not initialize Emaclite, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	XIntc_RegisterHandler(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_INTC_0_EMACLITE_0_VEC_ID, (XInterruptHandler)XEmacLite_InterruptHandler, &emaclite_inst);

	/* set MAC address */
	xil_printf("Setting MAC address to: '");
	print_mac_address(my_mac_address);
	xil_printf("'... ");
	XEmacLite_SetMacAddress(&emaclite_inst, (u8 *) my_mac_address);
	xil_printf("OK!\r\n");

	/* flush any frames already received */
	XEmacLite_FlushReceive(&emaclite_inst);

	/* enable interrupts */
	xil_printf("Assigning callback functions... ");
	XEmacLite_SetRecvHandler(&emaclite_inst, &emaclite_inst, (XEmacLite_Handler) recv_callback);
	XEmacLite_SetSendHandler(&emaclite_inst, &emaclite_inst, (XEmacLite_Handler) sent_callback);
	xil_printf("Enabling Emaclite interrupts... ");
	if (XEmacLite_EnableInterrupts(&emaclite_inst) != XST_SUCCESS)
	{
		xil_printf("Error enabling interrupts, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	/* finally enable exceptions on core level */
	Xil_ExceptionEnable();

	#ifdef READ_PHY_REGISTERS
		xil_printf("Reading PHY registers:\r\n");
		for(i = 0; i < 32; i++)
		{
				if (XEmacLite_PhyRead(&emaclite_inst, PHY_ADDRESS, i, &data) == XST_SUCCESS)
				{
					xil_printf("\tRegister address 0x%02x from PHY 0x%02x: 0x%04x\r\n", i, PHY_ADDRESS, data);
				}
				else
				{
					xil_printf("\tXST_DEVICE_BUSY\r\n");
					return 0;
				}
		}
	#endif

	#ifdef PLAY_WITH_PHY_LEDS
		while(1)
		{
			xil_printf("Playing with LEDs...\r\n");
			if (XEmacLite_PhyRead(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, &leds_phy_reg) == XST_SUCCESS)
			{
				xil_printf("\tRegister address 0x%02x from PHY 0x%02x: 0x%04x\r\n", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
				return 0;
			}

			leds_phy_reg |= 0b110000;
			leds_phy_reg &= ~0b110;
			xil_printf("\tWriting register 0x%02x on PHY 0x%02x: 0x%04x... ", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
			if (XEmacLite_PhyWrite(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, leds_phy_reg) == XST_SUCCESS)
			{
				xil_printf("OK!\r\n");
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
				return 0;
			}

			leds_phy_reg |= 0b10;
			xil_printf("\tWriting register 0x%02x on PHY 0x%02x: 0x%04x... ", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
			if (XEmacLite_PhyWrite(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, leds_phy_reg) == XST_SUCCESS)
			{
				xil_printf("OK!\r\n");
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
				return 0;
			}

			leds_phy_reg |= 0b100;
			leds_phy_reg &= ~0b010;
			xil_printf("\tWriting register 0x%02x on PHY 0x%02x: 0x%04x... ", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
			if (XEmacLite_PhyWrite(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, leds_phy_reg) == XST_SUCCESS)
			{
				xil_printf("OK!\r\n");
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
				return 0;
			}

			leds_phy_reg |= 0b010;
			xil_printf("\tWriting register 0x%02x on PHY 0x%02x: 0x%04x... ", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
			if (XEmacLite_PhyWrite(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, leds_phy_reg) == XST_SUCCESS)
			{
				xil_printf("OK!\r\n");
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
				return 0;
			}
		}
	#endif

	#ifdef CHECK_LEDS
		xil_printf("Turning off PHY LEDs... ");
		if (XEmacLite_PhyWrite(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, 0b110110) == XST_SUCCESS)
		{
			xil_printf("OK!\r\n");
		}
		else
		{
			xil_printf("\tXST_DEVICE_BUSY\r\n");
			return 0;
		}
		xil_printf("Checking for changes... \r\n");
		if (XEmacLite_PhyRead(&emaclite_inst, PHY_ADDRESS, LEDS_REGISTER, &leds_phy_reg) == XST_SUCCESS)
		{
			xil_printf("\tRegister address 0x%02x from PHY 0x%02x: 0x%04x\r\n", LEDS_REGISTER, PHY_ADDRESS, leds_phy_reg);
		}
		else
		{
			xil_printf("\tXST_DEVICE_BUSY\r\n");
			return 0;
		}
	#endif

	#ifdef CHECK_LOOPBACK_IS_DISABLED
		xil_printf("Checking loopback from BMCR register... ");
		if (XEmacLite_PhyRead(&emaclite_inst, PHY_ADDRESS, BMCR_REGISTER, &data) == XST_SUCCESS)
		{
			if((data & 0x4000) >> 15)
			{
				xil_printf("Loopback is enabled, returning!\r\n");
				return 0;
			}
			else
			{
				xil_printf("Loopback is disabled, OK!\r\n");
			}
		}
		else
		{
			xil_printf("\tXST_DEVICE_BUSY\r\n");
			return 0;
		}
	#endif

	#ifdef GENERATE_ICMP_PING_REQUEST_PROGRAMATICALLY
		icmp_frame.total_length =	sizeof(icmp_frame.type_of_message) +
									sizeof(icmp_frame.code) +
									sizeof(icmp_frame.header_checksum) +
									sizeof(icmp_frame.header_data) +
									0 * sizeof(icmp_frame.payload_data); /* 0 stands for payload data length */

		icmp_frame.type_of_message = 0x08; /* echo request */
		icmp_frame.code = 0x00;
		icmp_frame.header_data = (0x00 << 16) | (0x00 << 16); /* identifier [31:16] | sequence number [15:0] */
		icmp_frame.header_checksum = calculate_header_checksum_icmp(icmp_frame, 0);

		ip_frame.total_length = 	sizeof(ip_frame.version_header_length) +
									sizeof(ip_frame.type_of_service) +
									sizeof(ip_frame.total_length) +
									sizeof(ip_frame.identification) +
									sizeof(ip_frame.flags_offset) +
									sizeof(ip_frame.ttl) +
									sizeof(ip_frame.protocol) +
									sizeof(ip_frame.header_checksum) +
									sizeof(ip_frame.source_ip) +
									sizeof(ip_frame.destination_ip) +
									icmp_frame.total_length * sizeof(ip_frame.payload_data); /* icmp packet is payload, so stands for payload data length */
		ip_frame.version_header_length = (0b0100 << 4) | (0b0101); /* IP version 4 and internet header length 5 32-bit words = 20 bytes */
		ip_frame.type_of_service = 0x00; /* Differentiated Services Code Point (RFC2474) and Explicit Congestion Notification (RFC3168) */
		ip_frame.identification = 0x0000;
		ip_frame.flags_offset = (0b010 << 13) | 0b0000000000000; /* [15] reserved 0, [14] Don't Fragment, [13] More Fragments, [12:0] fragment offset */
		ip_frame.ttl = 0xFF;
		ip_frame.protocol = 0x01; /* 0x01 means ICMP (RFC790) */
		ip_frame.source_ip = convert_ip_address(my_ip_address);
		ip_frame.destination_ip = convert_ip_address(destination_ip_address);
		ip_frame.payload_data = (u8 *) &icmp_frame;
		ip_frame.header_checksum = calculate_header_checksum_ip(ip_frame);
		memcpy(&ethernet_frame.destination_mac[0], &destination_mac_address[0], 6);
		memcpy(&ethernet_frame.source_mac[0], &my_mac_address[0], 6);
		ethernet_frame.ether_type[0] = 0x08;
		ethernet_frame.ether_type[1] = 0x00;
		ethernet_frame.payload = (u8 *) &ip_frame;
		ethernet_frame.total_length = ip_frame.total_length + 14;

		//for(i = 0; i < ip_frame.total_length; i=i+2)
		//for(i = 0; i < ethernet_frame.total_length; i++)
		for(i = 0; i < ethernet_frame.total_length; i=i+2)
		{
			//xil_printf("0x%02x/0x%02x: 0x%04x\r\n", i, ip_frame.total_length, (ethernet_frame[i] << 8)|ethernet_frame[i+1]);
			xil_printf("0x%04x\r\n", (((u8)*((&ethernet_frame.total_length)+i))<<8) | ((u8)*((&ethernet_frame.total_length)+i+1)));
		}
	#endif

	#ifdef SEND_ICMP_PING_GENERATED_PROGRAMATICALLY_TO_PC
		xil_printf("Sending ICMP echo request to 192.168.1.130... ");
		if (XEmacLite_Send(&emaclite_inst, (u8 *) &ethernet_frame.destination_mac[0], ip_frame.total_length) == XST_SUCCESS)
		{
			xil_printf("OK!\r\n");
		}
		else
		{
			xil_printf("\tXST_FAILURE\r\n");
			return 0;
		}
	#endif

	#ifdef SEND_ICMP_PING_REQUEST_COPYING
		xil_printf("Sending ICMP echo request to 192.168.1.130... ");
		if (XEmacLite_Send(&emaclite_inst, (u8 *) &ping_frame[0], 44) == XST_SUCCESS)
		{
			xil_printf("OK!\r\n");
		}
		else
		{
			xil_printf("\tXST_FAILURE\r\n");
			return 0;
		}
	#endif

	#ifdef BROADCAST_ARP_REPLY
		xil_printf("Sending ARP reply broadcast... ");
		if (XEmacLite_Send(&emaclite_inst, (u8 *) &arp_response[0], 42) == XST_SUCCESS)
		{
			xil_printf("OK!\r\n");
		}
		else
		{
			xil_printf("\tXST_FAILURE\r\n");
			return 0;
		}
	#endif

	eth_frame_p = (ethernet_frame_t *) &buffer[0];
	while (1)
	{
		/* if any data to process */
		if (sys.packet_received)
		{
			/* clears flag */
			sys.packet_received = FALSE;

			memcpy(eth_frame_p->destination_mac, eth_frame_p->source_mac, MAC_ADDRESS_LENGTH);
			memcpy(eth_frame_p->source_mac, my_mac_address, MAC_ADDRESS_LENGTH);
			/*
			for (i = 0; i < 6; i++)
			{
				eth_frame_p->destination_mac[i] = eth_frame_p->source_mac[i];
				eth_frame_p->source_mac[i] = my_mac_address[i];
			}
			*/

			/* checks if packet is ARP */
			//if (check_eth_frame_ethertype(eth_frame_p) == ARP)
			//if((eth_frame_p->ethertype[0]<<8|eth_frame_p->ethertype[1]) == ARP)
			if(eth_frame_p->ethertype == ARP_big_endian)
			{
				arp_frame_t * arp_frame_p = (arp_frame_t *) &eth_frame_p->payload;
				//if(!memcmp(arp_frame_p->target_ip, my_ip_address, IP_ADDRESS_LENGTH))
				if(arp_frame_p->target_ip == my_ip_address_32)
				{
					if(arp_frame_p->operation_code == ARP_REQUEST_LINUX) /* ARP request, it is a 1 in the high byte */
					{
						arp_frame_p->operation_code = ARP_REPLY_LINUX; /* ARP reply, it is a 2 in the high byte */
					}
					else if(arp_frame_p->operation_code == ARP_REQUEST_WINDOWS) /* ARP request, it is a 1 in the low byte */
					{
						arp_frame_p->operation_code = ARP_REPLY_WINDOWS; /* ARP request, it is a 2 in the low byte */
					}

					//memcpy(arp_frame_p->target_ip, arp_frame_p->sender_ip, IP_ADDRESS_LENGTH);
					//memcpy(arp_frame_p->target_mac, arp_frame_p->sender_mac, MAC_ADDRESS_LENGTH);
					//memcpy(arp_frame_p->sender_ip, my_ip_address, IP_ADDRESS_LENGTH);
					//memcpy(arp_frame_p->sender_mac, my_mac_address, MAC_ADDRESS_LENGTH);
					//memcpy(eth_frame_p->destination_mac, eth_frame_p->source_mac, MAC_ADDRESS_LENGTH);
					//memcpy(eth_frame_p->source_mac, my_mac_address, MAC_ADDRESS_LENGTH);
					arp_frame_p->target_ip = arp_frame_p->sender_ip;
					arp_frame_p->sender_ip = my_ip_address_32;

					/*
					for (i = 0; i < 6; i++)
					{
						arp_frame_p->target_mac[i] = arp_frame_p->sender_mac[i];
						arp_frame_p->sender_mac[i] = my_mac_address[i];
					}
					*/
					memcpy(arp_frame_p->target_mac, arp_frame_p->sender_mac, MAC_ADDRESS_LENGTH);
					memcpy(arp_frame_p->sender_mac, my_mac_address, MAC_ADDRESS_LENGTH);
					#ifdef DEBUGGING
						xil_printf("Responding to ARP... ");
						//if(XEmacLite_Send(&emaclite_inst, buffer, sizeof(arp_frame_t) + 14) != XST_SUCCESS)
						if(XEmacLite_Send(&emaclite_inst, buffer, sizeof(arp_frame_t) + sizeof(ethernet_frame_t)) != XST_SUCCESS)
						{
							xil_printf("Error!\r\n");
						}
						xil_printf("OK!\r\n");
					#else
						//XEmacLite_Send(&emaclite_inst, buffer, sizeof(arp_frame_t) + 14);
						XEmacLite_Send(&emaclite_inst, buffer, sizeof(arp_frame_t) + sizeof(ethernet_frame_t));
					#endif
				}
			}
			//else if (check_eth_frame_ethertype(eth_frame_p) == IPv4)
			//else if((eth_frame_p->ethertype[0]<<8|eth_frame_p->ethertype[1]) == IPv4)
			else if(eth_frame_p->ethertype == IPv4_big_endian)
			{
				ip_frame_t * ip_frame_p = (ip_frame_t *) &eth_frame_p->payload;
				//if((ipv4_protocol_t) ip_frame_p->protocol == ICMP)
				if(ip_frame_p->protocol == ICMP)
				{
					icmp_frame_t * icmp_frame_p = (icmp_frame_t *) &ip_frame_p->payload_data;
					//if((icmp_type_t) icmp_frame_p->type_of_message == ECHO_REQUEST && icmp_frame_p->code == 0x00)
					//if((icmp_frame_p->type_of_message == ECHO_REQUEST) && icmp_frame_p->code == 0x00)
					if(icmp_frame_p->type_of_message == ECHO_REQUEST)
					{
						#ifdef DEBUGGING
							u16 i = 0;
						#endif
						icmp_frame_p->type_of_message = ECHO_REPLY;
						//memcpy(&ip_frame_p->destination_ip, &ip_frame_p->source_ip, IP_ADDRESS_LENGTH);
						//memcpy(&ip_frame_p->source_ip, &my_ip_address, IP_ADDRESS_LENGTH);
						//memcpy(eth_frame_p->destination_mac, eth_frame_p->source_mac, MAC_ADDRESS_LENGTH);
						//memcpy(eth_frame_p->source_mac, my_mac_address, MAC_ADDRESS_LENGTH);
						memcpy(&ip_frame_p->destination_ip, &ip_frame_p->source_ip, IP_ADDRESS_LENGTH);
						memcpy(&ip_frame_p->source_ip, &my_ip_address, IP_ADDRESS_LENGTH);
						icmp_frame_p->header_checksum = 0x0000;
						#ifdef DEBUGGING
							xil_printf("Packet length: %d\r\n", sys.packet_received_length);
							for (i = 0; i< sys.packet_received_length; i++)
							{
								xil_printf("0x%02x\r\n", buffer[i]);
							}
						#endif
						calculate_checksum_from_to(icmp_frame_p, &icmp_frame_p->type_of_message, &buffer[sys.packet_received_length-ETHERNET_FCS_LENGTH]);
						#ifdef DEBUGGING
							xil_printf("Responding to ping... ");
							if(XEmacLite_Send(&emaclite_inst, buffer, sys.packet_received_length - ETHERNET_FCS_LENGTH) != XST_SUCCESS)
							{
								xil_printf("Error!\r\n");
							}
							xil_printf("OK!\r\n");
						#else
							//XEmacLite_Send(&emaclite_inst, buffer, sizeof(ip_frame_t) + sizeof(icmp_frame_t) + 14);
							XEmacLite_Send(&emaclite_inst, buffer, sys.packet_received_length - ETHERNET_FCS_LENGTH);
						#endif
					}
				}
			}

			/* checks if broadcast or packet destined for us */
			if (check_eth_frame_destination_mac(eth_frame_p) == BROADCAST)
			{

			}
			else if (check_eth_frame_destination_mac(eth_frame_p) == MINE)
			{

			}
		}
		if(sys.packet_sent)
		{
			/* clears flag */
			sys.packet_sent = FALSE;
		}
	}

	/* deinitialize driver and PHY */
	/* never reached */
	return 0;
}

eth_frame_mac_t check_eth_frame_destination_mac(ethernet_frame_t * frame_p)
{
	if (!memcmp(frame_p->destination_mac, my_mac_address, MAC_ADDRESS_LENGTH))
	{
		return MINE;
	}
	else if (!memcmp(frame_p->destination_mac, broadcast_mac_address, MAC_ADDRESS_LENGTH))
	{
		return BROADCAST;
	}
	else
	{
		return UNKNOWN;
	}
}

//ethertype_t check_eth_frame_ethertype(ethernet_frame_t * frame_p)
//{
//	return (ethertype_t) (frame_p->ethertype[0]<<8|frame_p->ethertype[1]);
//}

void print_mac_address(u8 * addr)
{
	xil_printf("%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

void print_ip_address(u8 * addr)
{
	xil_printf("%03d.%03d.%03d.%03d", addr[0], addr[1], addr[2], addr[3]);
}

void recv_callback(XEmacLite * callbackReference)
{
	//if (callbackReference->EmacLiteConfig.BaseAddress == XPAR_EMACLITE_0_BASEADDR)
	//{
		XIntc_AckIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);
		sys.packet_received_length = XEmacLite_Recv(&emaclite_inst, &buffer[0]);
		sys.packet_received = TRUE;
		#ifdef DEBUGGING
			xil_printf("Packet received: len = %d\r\n", sys.packet_received_length);
		#endif
	//}
}

void sent_callback(XEmacLite * callbackReference)
{
	//if (callbackReference->EmacLiteConfig.BaseAddress == XPAR_EMACLITE_0_BASEADDR)
	//{
		XIntc_AckIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);
		sys.packet_sent = TRUE;
	//}
}

u16 calculate_header_checksum_ip(ip_frame_t * packet)
{
	u32 sum = 0;
	u16 ret_value = 0;

	sum =	packet->version_header_length +
			packet->type_of_service +
			packet->total_length +
			packet->identification +
			packet->flags_offset +
			packet->ttl +
			packet->protocol +
			packet->source_ip +
			packet->destination_ip;

	ret_value = ~((sum && 0xFF00) + (sum && 0x00FF));
	return ret_value;
}

u32 convert_ip_address(u8 * ip_array)
{
	return (ip_array[3] << 24) | (ip_array[2] << 16) | (ip_array[1] << 8) | (ip_array[0]);
}

u16 calculate_header_checksum_icmp(icmp_frame_t * packet, u32 data_length)
{
	u32 sum = 0;
	u32 i = 0;
	u16 ret_value = 0;

	sum =	packet->type_of_message +
			packet->code +
			packet->header_data;

	for (i = 0; i < data_length; i++)
	{
		sum += packet->payload_data[i];
	}

	ret_value = ~((sum && 0xFF00) + (sum && 0x00FF));
	return ret_value;
}

void calculate_checksum_from_to(icmp_frame_t * frame, u8 * address_from, u8 * address_to)
{
	u8 * i = 0;
	u32 sum = 0;
	u16 ret_value = 0;
	u16 aux_swap = 0;

	for (i = address_from; i < address_to; i=i+2)
	{
		sum += (*i<<8)|(*(i+1));
	}

	ret_value = ~(((sum & 0xFF0000)>>16) + (sum & 0x00FFFF));
	aux_swap = ((ret_value & 0xFF00) >> 8) | ((ret_value & 0x00FF) << 8);
	frame->header_checksum = aux_swap;
	return;
}

#ifdef SEND_ICMP_PING_REQUEST_COPYING
	void set_eth_frame_destination_mac(u8 * frame, u8 * mac)
	{
		memcpy((&frame + DESTINATION_MAC_OFFSET), mac, 6 * sizeof(u8));
	}

	void set_eth_frame_source_mac(u8 * frame, u8 * mac)
	{
		memcpy((&frame + SOURCE_MAC_OFFSET), mac, 6 * sizeof(u8));
	}

	void set_eth_frame_ethertype(u8 * frame, ethertype_t type)
	{
		memcpy((&frame + ETHERTYPE_OFFSET), (u16 *) &type, 1 * sizeof(u16));
	}

	int set_ip_frame_version_header_length(u8 * frame, u8 ip_version, u8 header_length)
	{
		if (ip_version != 4 && ip_version != 6)
		{
			return -1;
		}
		else if (header_length > 15)
		{
			return -1;
		}

		memcpy((&frame + IP_VERSION_HEADER_LENGTH_OFFSET), (u8 *) (((ip_version & 0x0F) << 4) | (header_length & 0x0F)), 1 * sizeof(u8));
		return 0;
	}

	void set_ip_frame_total_length(u8 * frame)
	{
		//TODO
	}

	void set_ip_frame_identification(u8 * frame)
	{
		/*
			The IPv4 ID field is thus meaningful only for non-atomic datagrams --
			either those datagrams that have already been fragmented or those for
			which fragmentation remains permitted.
	   */
	}

	int set_ip_frame_flags_fragmentation(u8 * frame, flags_t flags, u16 fragment_offset)
	{
		if (flags >= 0b100)
		{
			return -1;
		}
		else if (fragment_offset > 8192)
		{
			return -1;
		}

		return 0;
	}
#endif

