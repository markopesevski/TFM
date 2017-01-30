#include "stdio.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xemaclite.h"
#include "xintc.h"
#include "xil_exception.h"

#define LLARGARIA_MAC 6
#define LLARGARIA_FCS 4
#define IPv4 0x0800
#define ARP 0x0806
#define ICMP 0x01
#define ECHO_REPLY 0x00
#define ECHO_REQUEST 0x08
#define ARP_REQUEST_WINDOWS 0x0001
#define ARP_REPLY_WINDOWS 0x0002
#define ARP_REQUEST_LINUX 0x0100
#define ARP_REPLY_LINUX 0x0200
#define INVERTEIX_BYTES_16(x) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))

typedef struct __attribute__((__packed__))
{
	u8 mac_desti[LLARGARIA_MAC];
	u8 mac_origen[LLARGARIA_MAC];
	u16 ethertype;
	u8 * dades;
} trama_ethernet_t;

typedef struct __attribute__((__packed__))
{
	u8 versio_llargaria_header;
	u8 tipus_de_servei;
	u16 llargaria_total;
	u16 identificacio;
	u16 flags_fragments;
	u8 temps_de_vida;
	u8 protocol;
	u16 suma_verificacio;
	u32 ip_origen;
	u32 ip_desti;
	u8 * dades;
} paquet_ip_t;

typedef struct __attribute__((__packed__))
{
	u16 tipus_de_medi;
	u16 identificacio;
	u8 llargaria_direccio_fisica;
	u8 llargaria_direccio_logica;
	u16 operacio;
	u8 mac_origen[LLARGARIA_MAC];
	u32 ip_origen;
	u8 mac_desti[LLARGARIA_MAC];
	u32 ip_desti;
} paquet_arp_t;

typedef struct __attribute__((__packed__))
{
	u8 tipus_de_missatge;
	u8 codi;
	u16 suma_verificacio;
	u32 dades_header;
	u8 * dades;
} paquet_icmp_t;

typedef struct
{
	volatile Xboolean paquet_rebut;
	volatile Xboolean paquet_enviat;
	volatile u16 llargaria_paquet_rebut;
} variables_sistema;

void imprimeix_direccio_mac(u8 * addr);
void callback_rebut(XEmacLite * callbackReference);
void callback_enviat(XEmacLite * callbackReference);
void inicialitza_interrupcions(void);
void inicialitza_emaclite(void);

/* the mac address of the board. this should be unique per board */
u8 direccio_mac[LLARGARIA_MAC] = {0x00, 0x0a, 0x35, 0x00, 0x01, 0x02};
u32 direccio_ip = (192)|(168<<8)|(1<<16)|(200<<24);
XEmacLite emaclite;
XIntc intc;
static u8 buffer[2048] = {'\0'};
static volatile variables_sistema sys;
static trama_ethernet_t * trama_ethernet = (trama_ethernet_t *) &buffer[0];

int main(void)
{
	/* neteja la consola UART i imprimeix missatge */
	xil_printf("%c[2J",27);
	xil_printf("----- TFM - Marko Peshevski - versio NO-LwIP -----\r\n");

	inicialitza_interrupcions();
	inicialitza_emaclite();

	/* activa les interrupcions a nivell de processador */
	Xil_ExceptionEnable();

	while (1)
	{
		if (sys.paquet_rebut)
		{
			/* neteja el flag */
			sys.paquet_rebut = FALSE;

			/* inverteix les direccions MAC, respon d'on ha vingut el paquet */
			memcpy(trama_ethernet->mac_desti, trama_ethernet->mac_origen, LLARGARIA_MAC);
			memcpy(trama_ethernet->mac_origen, direccio_mac, LLARGARIA_MAC);

			/* mira si el paquet es ARP. necessita girar els bytes */
			if(INVERTEIX_BYTES_16(trama_ethernet->ethertype) == ARP)
			{
				/* simplement es un cast que interpreta les dades de memoria per facilitar */
				paquet_arp_t * paquet_arp = (paquet_arp_t *) &trama_ethernet->dades;

				/* mira si el paquet anava dirigit per la nostra IP */
				if(paquet_arp->ip_desti == direccio_ip)
				{
					if(paquet_arp->operacio == ARP_REQUEST_LINUX) /* ARP request, it is a 1 in the high byte */
					{
						paquet_arp->operacio = ARP_REPLY_LINUX; /* ARP reply, it is a 2 in the high byte */
					}
					else if(paquet_arp->operacio == ARP_REQUEST_WINDOWS) /* ARP request, it is a 1 in the low byte */
					{
						paquet_arp->operacio = ARP_REPLY_WINDOWS; /* ARP request, it is a 2 in the low byte */
					}

					/* inverteix adreces IP i MAC del paquet */
					paquet_arp->ip_desti = paquet_arp->ip_origen;
					paquet_arp->ip_origen = direccio_ip;
					memcpy(paquet_arp->mac_desti, paquet_arp->mac_origen, LLARGARIA_MAC);
					memcpy(paquet_arp->mac_origen, direccio_mac, LLARGARIA_MAC);

					/* envia la resposta */
					XEmacLite_Send(&emaclite, buffer, sys.llargaria_paquet_rebut - LLARGARIA_FCS);
				}
			}
			/* mira si es un paquet IPv4. necessita girar els bytes */
			else if(INVERTEIX_BYTES_16(trama_ethernet->ethertype) == IPv4)
			{
				/* simplement es un cast que interpreta les dades de memoria per facilitar */
				paquet_ip_t * paquet_ip = (paquet_ip_t *) &trama_ethernet->dades;

				/* mira si es un paquet de tipus ICMP */
				if(paquet_ip->protocol == ICMP)
				{
					/* simplement es un cast que interpreta les dades de memoria per facilitar */
					paquet_icmp_t * paquet_icmp = (paquet_icmp_t *) &paquet_ip->dades;

					/* mira si es una peticio d'eco */
					if(paquet_icmp->tipus_de_missatge == ECHO_REQUEST)
					{
						/* canvia la suma de verificacio */
						paquet_icmp->suma_verificacio += ECHO_REQUEST; /* -ECHO_REPLY, pero no cal perque es 0 */

						/* modifica el tipus de missatge */
						paquet_icmp->tipus_de_missatge = ECHO_REPLY;

						/* inverteix les adreces del paquet IPv4 */
						paquet_ip->ip_desti = paquet_ip->ip_origen;
						paquet_ip->ip_origen = direccio_ip;

						/* envia la resposta */
						XEmacLite_Send(&emaclite, buffer, sys.llargaria_paquet_rebut - LLARGARIA_FCS);
					}
				}
			}
		}
		if(sys.paquet_enviat)
		{
			/* neteja el flag */
			sys.paquet_enviat = FALSE;
		}
	}
	return 0;
}


void imprimeix_direccio_mac(u8 * addr)
{
	xil_printf("%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

void callback_rebut(XEmacLite * callbackReference)
{
	/* neteja la interrupcio del sistema */
	XIntc_AckIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);

	/* llegeix la llargaria de les dades rebudes */
	sys.llargaria_paquet_rebut = XEmacLite_Recv(&emaclite, &buffer[0]);

	/* posa un flag per notificar al bucle principal */
	sys.paquet_rebut = TRUE;
}

void callback_enviat(XEmacLite * callbackReference)
{
	/* neteja la interrupcio del sistema */
	XIntc_AckIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);

	/* posa un flag per notificar al bucle principal */
	sys.paquet_enviat = TRUE;
}

void inicialitza_interrupcions(void)
{
	/* inicialitza el periferic Intc de MicroBlaze */
	xil_printf("Inicialitzant periferic Intc... ");
	if (XIntc_Initialize(&intc, XPAR_INTC_0_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("No s'ha pogut completar!\r\n");
		return;
	}
	xil_printf("OK!\r\n");

	/* arrenca el periferic en mode real (no simulacio) */
	xil_printf("Arrencant periferic Intc... ");
	if (XIntc_Start(&intc, XIN_REAL_MODE) != XST_SUCCESS)
	{
		xil_printf("No s'ha pogut completar!\r\n");
		return;
	}
	xil_printf("OK!\r\n");

	/* especifica la funcio que gestiona les interrupcions del Emaclite */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XIntc_InterruptHandler, &intc);

	/* habilita les interrupcions del Emaclite */
	XIntc_EnableIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);

	/* activa les interrupcions del Emaclite */
	XIntc_Enable(&intc, XPAR_INTC_0_EMACLITE_0_VEC_ID);

	/* connecta el senyal d'interrupcio del Emaclite amb el periferic Intc */
	xil_printf("Connectant senyal d'interrupcio del Emaclite... ");
	if (XIntc_Connect(&intc, XPAR_INTC_0_EMACLITE_0_VEC_ID, (XInterruptHandler) XEmacLite_InterruptHandler, (void *) &intc) != XST_SUCCESS)
	{
		xil_printf("No s'ha pogut completar!\r\n");
	}
	xil_printf("OK!\r\n");
}

void inicialitza_emaclite(void)
{
	/* inicialitza el Emaclite i el xip PHY de la placa */
	xil_printf("Inicialitzant Emaclite i PHY... ");
	if (XEmacLite_Initialize(&emaclite, XPAR_ETHERNET_MAC_DEVICE_ID) != XST_SUCCESS)
	{
		xil_printf("No s'ha pogut completar!\r\n");
		return;
	}
	xil_printf("OK!\r\n");

	XIntc_RegisterHandler(XPAR_MICROBLAZE_0_INTC_BASEADDR, XPAR_INTC_0_EMACLITE_0_VEC_ID, (XInterruptHandler)XEmacLite_InterruptHandler, &emaclite);

	/* configura la direccio MAC */
	xil_printf("Configurant la següent direccio MAC: '");
	imprimeix_direccio_mac(direccio_mac);
	xil_printf("'... ");
	XEmacLite_SetMacAddress(&emaclite, (u8 *) direccio_mac);
	xil_printf("OK!\r\n");

	/* neteja els buffers de recepcio */
	XEmacLite_FlushReceive(&emaclite);

	/* assigna funcions callback i habilita interrupcions */
	xil_printf("Assignant funcions de callback per les interrupcions... ");
	XEmacLite_SetRecvHandler(&emaclite, &emaclite, (XEmacLite_Handler) callback_rebut);
	XEmacLite_SetSendHandler(&emaclite, &emaclite, (XEmacLite_Handler) callback_enviat);
	xil_printf("Habilitant interrupcions d'Emaclite... ");
	if (XEmacLite_EnableInterrupts(&emaclite) != XST_SUCCESS)
	{
		xil_printf("No s'ha pogut completar!\r\n");
		return;
	}
	xil_printf("OK!\r\n");
}
