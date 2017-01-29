#include "stdio.h"
#include "xparameters.h"
#include "netif/xadapter.h"
#include "xil_printf.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xtmrctr_l.h"
#include "xemaclite.h"

void handler_temporitzador(void *p);
void imprimeix_direccio_ip(char *msg, struct ip_addr *ip);
void imprimeix_configuracio_ip(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw);
void arrenca_app(void);
err_t callback_paquet_rebut(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t callback_connexio_acceptada(void *arg, struct tcp_pcb *newpcb, err_t err);
void inicialitza_interrupcions(void);
void inicialitza_temporitzador(void);
void inicialitza_lwip(void);

/* estructura de configuracio del periferic Intc (Interrupt Controller) */
static XIntc intc;

/* 0 significa nomes el rapid, 1 significa tots dos temporitzadors */
static int both_timers = 0;
volatile int temporitzador_tcp_rapid = 0;
volatile int temporitzador_tcp_lent = 0;

/* adreça MAC de la placa */
static unsigned char direccio_mac[] = {0x00, 0x0a, 0x35, 0x00, 0x01, 0x02};

/* estructura per la interficie de xarxa */
static struct netif interficie_xarxa;

/* estructures per les adreces IP que es faran servir */
static struct ip_addr direccio_ip, mascara_xarxa, gateway;

int main(void)
{
	/* neteja la consola UART i imprimeix missatge */
	xil_printf("%c[2J",27);
	xil_printf("----- TFM - Marko Peshevski - versio LwIP -----\r\n");

	inicialitza_temporitzador();
	inicialitza_interrupcions();
	inicialitza_lwip();

	xil_printf("Arrenca aplicacio que respon a eco... ");
	arrenca_app();
	xil_printf("Aplicacio en marxa\r\n");

	/* activa les interrupcions a nivell de processador */
	Xil_ExceptionEnable();
	while (1)
	{
		/* rep dades de la interficie de xarxa (driver ethernet MAC) */
		xemacif_input(&interficie_xarxa);

		/* consulta temporitzadors i executa tasques periodiques */
		if (temporitzador_tcp_rapid)
		{
			tcp_fasttmr();
			temporitzador_tcp_rapid = 0;
		}
		if (temporitzador_tcp_lent)
		{
			tcp_slowtmr();
			temporitzador_tcp_lent = 0;
		}
	}
	return 0;
}

void handler_temporitzador(void *p)
{
	/* neteja el bit d'interrupcio i carrega el valor a comptar al registre del temporitzador */
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, XTC_CSR_INT_OCCURED_MASK |
														   XTC_CSR_LOAD_MASK);

	/* inicia el temporitzador i la interrupcio i compta descendent */
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK |
														   XTC_CSR_ENABLE_INT_MASK |
														   XTC_CSR_DOWN_COUNT_MASK);

	/* neteja el bit d'interrupcio del registre */
	XIntc_AckIntr(XPAR_INTC_0_BASEADDR, (1 << XPAR_INTC_0_TMRCTR_0_VEC_ID));

	/* actualitza valors dels temporitzadors rapid i lent */
	temporitzador_tcp_rapid = 1;
	both_timers = !both_timers;
	if (both_timers)
	{
		temporitzador_tcp_lent = 1;
	}
}

void imprimeix_direccio_ip(char *msg, struct ip_addr *ip)
{
	xil_printf(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
}

void imprimeix_configuracio_ip(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{
	imprimeix_direccio_ip("Direccio IP: ", ip);
	imprimeix_direccio_ip("Mascara    : ", mask);
	imprimeix_direccio_ip("Gateway    : ", gw);
}

void arrenca_app(void)
{
    struct tcp_pcb *pcb;
    err_t err;

    /* crea i reserva memoria per una estructura protocol control block (PCB) nova */
    pcb = tcp_new();
    if (!pcb)
    {
    	xil_printf("Error creant PCB, falta memoria\r\n");
    	return;
    }

    /* lliga aquest PCB al port 80 (per respondre a peticions HTTP, per exemple) */
    err = tcp_bind(pcb, IP_ADDR_ANY, 80);
    if (err != ERR_OK)
    {
    	xil_printf("No he pogut lligar al port 80. err = %d\r\n", err);
    	return;
    }

    /* diu a la llibreria que quan cridi a les funcions callback d'aquest servei no retorni arguments */
    tcp_arg(pcb, NULL);

    /* comença a escoltar per si hi han connexions */
    pcb = tcp_listen(pcb);
    if (!pcb)
    {
    	xil_printf("M'he quedat sense memoria quan volia començar a escoltar\r\n");
    	return;
    }

    /* especifica la funcio callback per connexions vinents */
    tcp_accept(pcb, callback_connexio_acceptada);
}

err_t callback_paquet_rebut(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    /* tanca la connexio si el transmissor ha enviat el paquet FIN de TCP  */
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    /* avisa a la llibreria que el paquet ha estat rebut */
    tcp_recved(tpcb, p->tot_len);

    /* no s'ha de fer res mes amb el paquet, aixi que allibera l'espai de memoria que feia servir aquest */
    pbuf_free(p);

    return ERR_OK;
}

err_t callback_connexio_acceptada(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	/* quan hi ha una connexio per acceptar l'accepta i lliga el callback de dades rebudes a aquest paquet */
    tcp_recv(newpcb, callback_paquet_rebut);

    return ERR_OK;
}

/* Intc stands for Interrupt Controller. the flow for an interrupt is:
 * peripheral (hardware IP core) generates interrupt -> core exception handler
 * interrupts processor. so both must be enabled separately */
void inicialitza_interrupcions(void)
{
	/* inicialitza el periferic Intc de MicroBlaze */
	XIntc_Initialize(&intc, XPAR_INTC_0_DEVICE_ID);

	/* arrenca el periferic en mode real (no simulacio) */
	XIntc_Start(&intc, XIN_REAL_MODE);

	/* especifica la funcio que gestiona les interrupcions del temporitzador. la del ethernet MAC es fa des de lwip */
	XIntc_RegisterHandler(XPAR_INTC_0_BASEADDR, XPAR_INTC_0_TMRCTR_0_VEC_ID, (XInterruptHandler)handler_temporitzador, &intc);

	/* habilita la interrupcio del temporitzador */
	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, (1 << XPAR_INTC_0_TMRCTR_0_VEC_ID));

	/* habilita la interrupcio del ethernet MAC */
	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);

	/* activa les interrupcions	del timer i ethernet MAC */
	XIntc_Enable(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID);
	XIntc_Enable(&intc, XPAR_INTC_0_EMACLITE_0_VEC_ID);
}

void inicialitza_temporitzador(void)
{
	/* especifica quin nombre de cicles ha de comptar el temporitzador abans d'interrompre */
	/* amb un rellotge de 100 Mhz -> 0.01us per cada periode per tant, per 250ms -> 25000000 periodes */
	XTmrCtr_SetLoadReg(XPAR_TMRCTR_0_BASEADDR, 0, 25000000);

	/* neteja el bit d'interrupcio i carrega el valor a comptar al registre del temporitzador */
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, XTC_CSR_INT_OCCURED_MASK |
														   XTC_CSR_LOAD_MASK);

	/* inicia el temporitzador i la interrupcio i compta descendent */
	XTmrCtr_SetControlStatusReg(XPAR_TMRCTR_0_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK |
														   XTC_CSR_ENABLE_INT_MASK |
														   XTC_CSR_DOWN_COUNT_MASK);
}

void inicialitza_lwip(void)
{
	/* inicialitzacio interna de la lwip */
	lwip_init();

	/* especifica les adreces IP a utilitzar manualment, no fa servir DHCP */
	IP4_ADDR(&direccio_ip,  192, 168,   1, 200);
	IP4_ADDR(&mascara_xarxa, 255, 255, 255,  0);
	IP4_ADDR(&gateway,      192, 168,   1,  1);
	imprimeix_configuracio_ip(&direccio_ip, &mascara_xarxa, &gateway);

	/* afegeix la interficie de xarxa a la llista de les disponibles de la llibreria */
	xemac_add(&interficie_xarxa, &direccio_ip, &mascara_xarxa, &gateway, direccio_mac, XPAR_EMACLITE_0_BASEADDR);

	/* configura la interficie de xarxa per defecte i la posa en marxa */
	netif_set_default(&interficie_xarxa);
	netif_set_up(&interficie_xarxa);
}
