#include "stdio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xemaclite.h"

#define ETHERNET_MAC_ADDRESS	{0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
#define IP_ADDRESS				{192, 168, 1, 200}
#define NET_MASK				{255, 255, 255, 0}
#define GW_ADDRESS				{192, 168, 1, 1}
#define DEFAULT_PORT			5001
#define LEDS_ADDR XPAR_GPIO_1_BASEADDR

/* the mac address of the board. this should be unique per board */
unsigned char mac_ethernet_address[] = ETHERNET_MAC_ADDRESS;
unsigned char ip_address[] = IP_ADDRESS;
unsigned char * leds = (unsigned char*) LEDS_ADDR;
XEmacLite emaclite_inst;
u8 buffer[2048] = {'\0'};
u16 packetlen = 0;
u16 data = 0;

void print_mac_address(u8 * addr);
void recv_callback(void);
void sent_callback(void);

int main(void)
{
	u8 i = 0;
	*leds = 0b0000;
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

	/* set MAC address */
	xil_printf("Setting MAC address to: '");
	print_mac_address(mac_ethernet_address);
	xil_printf("'... ");
	XEmacLite_SetMacAddress(&emaclite_inst, (u8 *) mac_ethernet_address);
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

	xil_printf("Reading PHY registers:\r\n");
	for(i = 0; i < 32; i++)
	{
			if (XEmacLite_PhyRead(&emaclite_inst, 0x1F, i, &data) == XST_SUCCESS)
			{
				xil_printf("\tRegister address 0x%02x from PHY 0x%02x: 0x%04x\r\n", i, 0x1F, data);
			}
			else
			{
				xil_printf("\tXST_DEVICE_BUSY\r\n");
			}
	}

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
