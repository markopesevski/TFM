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

			/* checks if packet is ARP */
			if(eth_frame_p->ethertype == ARP_big_endian)
			{
				arp_frame_t * arp_frame_p = (arp_frame_t *) &eth_frame_p->payload;
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

					arp_frame_p->target_ip = arp_frame_p->sender_ip;
					arp_frame_p->sender_ip = my_ip_address_32;
					memcpy(arp_frame_p->target_mac, arp_frame_p->sender_mac, MAC_ADDRESS_LENGTH);
					memcpy(arp_frame_p->sender_mac, my_mac_address, MAC_ADDRESS_LENGTH);

					XEmacLite_Send(&emaclite_inst, buffer, sizeof(arp_frame_t) + sizeof(ethernet_frame_t));
				}
			}
			else if(eth_frame_p->ethertype == IPv4_big_endian)
			{
				ip_frame_t * ip_frame_p = (ip_frame_t *) &eth_frame_p->payload;
				if(ip_frame_p->protocol == ICMP)
				{
					icmp_frame_t * icmp_frame_p = (icmp_frame_t *) &ip_frame_p->payload_data;
					if(icmp_frame_p->type_of_message == ECHO_REQUEST)
					{
						icmp_frame_p->type_of_message = ECHO_REPLY;
						memcpy(&ip_frame_p->destination_ip, &ip_frame_p->source_ip, IP_ADDRESS_LENGTH);
						memcpy(&ip_frame_p->source_ip, &my_ip_address, IP_ADDRESS_LENGTH);
						icmp_frame_p->header_checksum = 0x0000;
						calculate_checksum_from_to(icmp_frame_p, &icmp_frame_p->type_of_message, &buffer[sys.packet_received_length-ETHERNET_FCS_LENGTH]);
						XEmacLite_Send(&emaclite_inst, buffer, sys.packet_received_length - ETHERNET_FCS_LENGTH);
					}
				}
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
