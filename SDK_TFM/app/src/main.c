#include "stdio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xemaclite.h"
#include "xintc.h"

#define SOURCE_MAC_ADDRESS		{0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
#define SOURCE_MAC_ADDRESS_INV	{0x02, 0x01, 0x00, 0x35, 0x0a, 0x00}
#define DESTINATION_MAC_ADDRESS	{0x34, 0x97, 0xF6, 0xDB, 0x8E, 0x1F}
#define BROADCAST_MAC_ADDRESS	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define SOURCE_IP_ADDRESS		{192, 168, 1, 200}
#define DESTINATION_IP_ADDRESS	{192, 168, 1, 130}
#define NET_MASK				{255, 255, 255, 0}
#define GW_ADDRESS				{192, 168, 1, 1}
#define DEFAULT_PORT			5001
#define LEDS_ADDR XPAR_GPIO_1_BASEADDR
/* this number is not magic, reading the datasheet: PHYAD are 5 bits set on PCB to be 0b11111 = 0x1F  */
	#define PHY_ADDRESS 0x1F
#define LEDS_REGISTER 0x18
#define BMCR_REGISTER 0x00

typedef enum
{
	IPv4 = 0x0800,
	ARP = 0x0806,
	WOL = 0x0842
} ethertype_t;

typedef enum
{
	RESERVED = 0b100,
	DONT_FRAGMENT = 0b010,
	MORE_FRAGMENTS = 0b001
} flags_t;

//#define READ_PHY_REGISTERS
//#define PLAY_WITH_PHY_LEDS
//#define CHECK_LEDS
//#define CHECK_LOOPBACK_IS_DISABLED
//#define GENERATE_ICMP_PING_REQUEST_PROGRAMATICALLY
//#define SEND_ICMP_PING_GENERATED_PROGRAMATICALLY_TO_PC
//#define SEND_ICMP_PING_REQUEST_COPYING
#define RECEIVE_PACKET
#define BROADCAST_ARP_REPLY

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

#ifdef RECEIVE_PACKET

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

#endif

/* the mac address of the board. this should be unique per board */
//u8 source_mac_address[6] = SOURCE_MAC_ADDRESS;
u8 source_mac_address[6] = SOURCE_MAC_ADDRESS_INV;
u8 destination_mac_address[6] = DESTINATION_MAC_ADDRESS;
u8 broadcast_mac_address[6] = BROADCAST_MAC_ADDRESS;
u8 source_ip_address[4] = SOURCE_IP_ADDRESS;
u8 destination_ip_address[4] = DESTINATION_IP_ADDRESS;
u8 * board_leds = (u8*) LEDS_ADDR;
XEmacLite emaclite_inst;
XIntc intc_inst;
XIntc_Config * config_intc;
u8 buffer[2048] = {'\0'};
u16 packetlen = 0;
u16 data = 0;

typedef struct ethernet_frame_t
{
	u16 total_length;
	u8 destination_mac[6];
	u8 source_mac[6];
	u8 ether_type[2];
	u8 * payload;
	u8 frame_checksum[4]; /* remember to send frame_checksum and payload in inverse order */
} ethernet_frame_t;

typedef struct ip_frame_t
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

typedef struct icmp_frame_t
{
	u16 total_length;
	u8 type_of_message;
	u8 code;
	u16 header_checksum;
	u32 header_data;
	u8 * payload_data;
} icmp_frame_t;

void print_mac_address(u8 * addr);
void recv_callback(void * callbackReference);
void sent_callback(void);
u16 calculate_header_checksum_ip(ip_frame_t packet);
u32 convert_ip_address(u8 * ip_array);
u16 calculate_header_checksum_icmp(icmp_frame_t packet, u32 data_length);

#ifdef RECEIVE_PACKET
	u8 recv_frame[2048] = {'\0'};
	u16 recv_response = 0;
	u16 recv_length = 0;
#endif


// XPAR_INTC_0_EMACLITE_0_VEC_ID


int main(void)
{
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

	xil_printf("Enabling Intc peripheral...");
	XIntc_Enable(&intc_inst, XPAR_INTC_0_EMACLITE_0_VEC_ID);
	if(intc_inst.IsReady == 0)
	{
		xil_printf("Intc peripheral not ready, returning!\r\n");
	}
	xil_printf("OK!\r\n");

	xil_printf("Initializing system interrupts... ");
	if (XIntc_Start(&intc_inst, XIN_REAL_MODE) != XST_SUCCESS)
	{
		xil_printf("Could not initialize interrupts, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	xil_printf("Connecting interrupt source for Emaclite... ");
	if (XIntc_Connect(&intc_inst, XPAR_INTC_0_EMACLITE_0_VEC_ID, (XInterruptHandler) XEmacLite_InterruptHandler, &intc_inst) != XST_SUCCESS)
	{
		xil_printf("Something failed!\r\n");
	}
	xil_printf("OK!\r\n");

	if (!intc_inst.IsReady)
	{
		xil_printf("Intc peripheral not ready, returning!\r\n");
	}
	if (!intc_inst.IsStarted)
	{
		xil_printf("Intc peripheral not started, returning!\r\n");
	}
	if (&XEmacLite_InterruptHandler != &(intc_inst.CfgPtr->HandlerTable[XPAR_INTC_0_EMACLITE_0_VEC_ID].Handler))
	{
		xil_printf("Interrupt handler for Emaclite not assigned properly!\r\n");
	}


	/* initialize emaclite driver and PHY */
	xil_printf("Initializing Emaclite... ");
	if (XEmacLite_Initialize(&emaclite_inst, XPAR_ETHERNET_MAC_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("Could not initialize Emaclite, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	/* emaclite self-test */
	xil_printf("Performing Emaclite self-test... ");
	if(XEmacLite_SelfTest(&emaclite_inst) != XST_SUCCESS)
	{
		xil_printf("Self-test incorrect, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	/* set MAC address */
	xil_printf("Setting MAC address to: '");
	print_mac_address(source_mac_address);
	xil_printf("'... ");
	XEmacLite_SetMacAddress(&emaclite_inst, (u8 *) source_mac_address);
	xil_printf("OK!\r\n");

	/* enable interrupts */
	xil_printf("Enabling interrupts... ");
	if (XEmacLite_EnableInterrupts(&emaclite_inst) == XST_NO_CALLBACK)
	{
		xil_printf("Assigning callback functions... ");
		XEmacLite_SetRecvHandler(&emaclite_inst, &emaclite_inst, (XEmacLite_Handler) recv_callback);
		XEmacLite_SetSendHandler(&emaclite_inst, &emaclite_inst, (XEmacLite_Handler) sent_callback);
		if (XEmacLite_EnableInterrupts(&emaclite_inst) != XST_SUCCESS)
		{
			xil_printf("Error enabling interrupts, returning!\r\n");
			return 0;
		}
	}
	else
	{
		xil_printf("Callbacks already assigned? Trying to enable interrupts... \r\n");
		if (XEmacLite_EnableInterrupts(&emaclite_inst) != XST_SUCCESS)
		{
			xil_printf("Error enabling interrupts, returning!\r\n");
			return 0;
		}
		return 0;
	}
	xil_printf("OK!\r\n");
	xil_printf("Following addresses should be equal: %x == %x ?\r\n", &(emaclite_inst.RecvHandler), &(recv_callback));

	#ifdef RECEIVE_PACKET
		recv_response = XEmacLite_Recv(&emaclite_inst, &recv_frame[0]);
	#endif

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
		ip_frame.source_ip = convert_ip_address(source_ip_address);
		ip_frame.destination_ip = convert_ip_address(destination_ip_address);
		ip_frame.payload_data = (u8 *) &icmp_frame;
		ip_frame.header_checksum = calculate_header_checksum_ip(ip_frame);
		memcpy(&ethernet_frame.destination_mac[0], &destination_mac_address[0], 6);
		memcpy(&ethernet_frame.source_mac[0], &source_mac_address[0], 6);
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

	while (1)
	{
		/* if any data to process */
	}

	/* deinitialize driver and PHY */
	/* never reached */
	return 0;
}

void print_mac_address(u8 * addr)
{
	xil_printf("%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

void recv_callback(void * callbackReference)
{
	//xil_printf("Packet received!\r\n");
	*board_leds = 0b1010;
	#ifdef RECEIVE_PACKET
		recv_response = XEmacLite_Recv(&emaclite_inst, &recv_frame[0]);
	#endif
}

void sent_callback(void)
{
	//xil_printf("Packet sent!\r\n");
	*board_leds = 0b0101;
}

u16 calculate_header_checksum_ip(ip_frame_t packet)
{
	u32 sum = 0;
	u16 ret_value = 0;

	sum =	packet.version_header_length +
			packet.type_of_service +
			packet.total_length +
			packet.identification +
			packet.flags_offset +
			packet.ttl +
			packet.protocol +
			packet.source_ip +
			packet.destination_ip;

	ret_value = ~((sum && 0xFF00) + (sum && 0x00FF));
	return ret_value;
}

u32 convert_ip_address(u8 * ip_array)
{
	return (ip_array[3] << 24) | (ip_array[2] << 16) | (ip_array[1] << 8) | (ip_array[0]);
}

u16 calculate_header_checksum_icmp(icmp_frame_t packet, u32 data_length)
{
	u32 sum = 0;
	u32 i = 0;
	u16 ret_value = 0;

	sum =	packet.type_of_message +
			packet.code +
			packet.header_data;

	for (i = 0; i < data_length; i++)
	{
		sum += packet.payload_data[i];
	}

	ret_value = ~((sum && 0xFF00) + (sum && 0x00FF));
	return ret_value;
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
