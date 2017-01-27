#include "stdio.h"
#include "xparameters.h"
#include "netif/xadapter.h"
#include "platform.h"
#include "xil_printf.h"
#include "lwipopts.h"
#include "lwip/dhcp.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"

#define ETHERNET_MAC_ADDRESS	{0x00, 0x0a, 0x35, 0x00, 0x01, 0x02}
#define LEDS_ADDR XPAR_GPIO_1_BASEADDR

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
extern volatile int fastestTmrFlag;

/* the mac address of the board. this should be unique per board */
unsigned char ethernet_mac_address[] = ETHERNET_MAC_ADDRESS;

struct netif server_netif, *netif = &server_netif;
struct ip_addr ipaddr, netmask, gw; /* gw = gateway */
unsigned char * leds = (unsigned char*) LEDS_ADDR;

int main(void)
{
	/* clears output */
	xil_printf("%c[2J",27);
	xil_printf("\r\n");
	xil_printf("----- TFM - Marko Peshevski -----\r\n");

	//netif = &server_netif;

	/* inits platform, timers and such */
	if (init_platform() < 0)
	{
		xil_printf("ERROR initializing platform.\r\n");
		return -1;
	}

	/* inits lwip library */
	lwip_init();

	/* initliaze IP addresses to be used */
	IP4_ADDR(&ipaddr,  192, 168,   1, 200);
	IP4_ADDR(&netmask, 255, 255, 255,  0);
	IP4_ADDR(&gw,      192, 168,   1,  1);
	print_ip_settings(&ipaddr, &netmask, &gw);

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(netif, &ipaddr, &netmask, &gw, ethernet_mac_address, PLATFORM_EMAC_BASEADDR))
	{
		xil_printf("Error adding net interface\r\n");
		return -1;
	}

	/* set the default network interface (ethernet) */
	netif_set_default(netif);

	/* specify that the network if is up */
	netif_set_up(netif);

	/* now enable interrupts */
	platform_enable_interrupts();

	/* start the application */
	xil_printf("Starting echo responsive app... ");
	start_applications();
	xil_printf("Echo app started\r\n");

	while (1)
	{
		if (TcpFastTmrFlag)
		{
			tcp_fasttmr();
			TcpFastTmrFlag = 0;
		}
		if (TcpSlowTmrFlag)
		{
			tcp_slowtmr();
			TcpSlowTmrFlag = 0;
		}

		xemacif_input(netif);
	}

	/* never reached */
	cleanup_platform();

	return 0;
}
