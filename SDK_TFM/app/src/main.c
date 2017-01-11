#include "stdio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xemaclite.h"

#define SOURCE_MAC_ADDRESS		{0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
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

//#define READ_PHY_REGISTERS
//#define PLAY_WITH_PHY_LEDS
//#define CHECK_LEDS
//#define CHECK_LOOPBACK_IS_DISABLED
#define GENERATE_ICMP_PING_REQUEST
#define SEND_ICMP_PING_TO_PC

/* the mac address of the board. this should be unique per board */
u8 source_mac_address[6] = SOURCE_MAC_ADDRESS;
u8 destination_mac_address[6] = DESTINATION_MAC_ADDRESS;
u8 broadcast_mac_address[6] = BROADCAST_MAC_ADDRESS;
u8 source_ip_address[4] = SOURCE_IP_ADDRESS;
u8 destination_ip_address[4] = DESTINATION_IP_ADDRESS;
u8 * board_leds = (u8*) LEDS_ADDR;
XEmacLite emaclite_inst;
u8 buffer[2048] = {'\0'};
u16 packetlen = 0;
u16 data = 0;

typedef struct ethernet_frame_t
{
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
void recv_callback(void);
void sent_callback(void);
u16 calculate_header_checksum_ip(ip_frame_t packet);
u32 convert_ip_address(u8 * ip_array);
u16 calculate_header_checksum_icmp(icmp_frame_t packet, u32 data_length);

int main(void)
{
	#ifdef READ_PHY_REGISTERS
		u8 i = 0;
	#endif
	#ifdef PLAY_WITH_PHY_LEDS
		u16 leds_phy_reg = 0;
	#endif
	#ifdef SEND_ICMP_PING_TO_PC
		ip_frame_t ip_frame;
		icmp_frame_t icmp_frame;
		ethernet_frame_t ethernet_frame;
	#endif
	#ifdef GENERATE_ICMP_PING_REQUEST
		#ifndef SEND_ICMP_PING_TO_PC
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

	/* initialize emaclite driver and PHY */
	xil_printf("Initializing Emaclite... ");
	if (XEmacLite_Initialize(&emaclite_inst, XPAR_ETHERNET_MAC_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("Could not initialize Emaclite, returning!\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

	/* emaclite self-test */
	xil_printf("Performing self-test... ");
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
		XEmacLite_SetRecvHandler(&emaclite_inst, NULL, (XEmacLite_Handler) recv_callback);
		XEmacLite_SetSendHandler(&emaclite_inst, NULL, (XEmacLite_Handler) sent_callback);
	}
	else
	{
		xil_printf("Callbacks already assigned?\r\n");
		return 0;
	}
	xil_printf("OK!\r\n");

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

	#ifdef GENERATE_ICMP_PING_REQUEST
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
		ethernet_frame.ether_type[0] = 0x08;
		ethernet_frame.ether_type[1] = 0x00;
		ethernet_frame.payload = (u8 *) &ip_frame;

		//for(i = 0; i < ip_frame.total_length; i=i+2)
		for(i = 0; i < ip_frame.total_length; i++)
		{
			//xil_printf("0x%02x/0x%02x: 0x%04x\r\n", i, ip_frame.total_length, (ethernet_frame[i] << 8)|ethernet_frame[i+1]);
			xil_printf("0x%02x\r\n", *(&ethernet_frame+i));
		}
	#endif
	#ifdef SEND_ICMP_PING_TO_PC
		xil_printf("Sending ICMP echo request to 192.168.1.130... ");
		if (XEmacLite_Send(&emaclite_inst, (u8*) &ethernet_frame, ip_frame.total_length) == XST_SUCCESS)
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

void recv_callback(void)
{
	xil_printf("Packet received!\r\n");
}

void sent_callback(void)
{
	xil_printf("Packet sent!\r\n");
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
