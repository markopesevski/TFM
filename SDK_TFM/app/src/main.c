#include "stdio.h"
#include "xparameters.h"
//#include "platform.h"
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

int main(void)
{
	*leds = 0b0000;
	/* clears output */
	xil_printf("%c[2J",27);
	xil_printf("----- SDAV - Marko Peshevski -----\r\n");

	/* initialize emaclite driver and PHY */

	while (1)
	{
		/* if any data to process */
	}

	/* never reached */
	/* deinitialize driver and PHY */
	return 0;
}
