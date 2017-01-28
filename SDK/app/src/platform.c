#include "platform.h"

volatile int TcpFastTmrFlag = 0;
volatile int TcpSlowTmrFlag = 0;

static unsigned rxperf_port = 5001;	/* iperf default port */
static unsigned rxperf_server_running = 0;

void timer_callback()
{
	static int odd = 1;

	TcpFastTmrFlag = 1;

	odd = !odd;
	if (odd)
	{
		TcpSlowTmrFlag = 1;
	}
}

void xadapter_timer_handler(void *p)
{
	/* Load timer, clear interrupt bit */
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_INT_OCCURED_MASK | XTC_CSR_LOAD_MASK);
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK | XTC_CSR_ENABLE_INT_MASK | XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_DOWN_COUNT_MASK);

	/* Clear interrupt bit */
	XIntc_AckIntr(XPAR_INTC_0_BASEADDR, PLATFORM_TIMER_INTERRUPT_MASK);

	/* Call callback funcion */
	timer_callback();
}

#define TIMER_TLR (XPAR_TMRCTR_0_CLOCK_FREQ_HZ / 4)
//#define TIMER_TLR (XPAR_TMRCTR_0_CLOCK_FREQ_HZ / 12)
void platform_setup_timer()
{
	/* set the number of cycles the timer counts before interrupting */
	/* 100 Mhz clock => .01us for 1 clk tick. For 100ms, 10000000 clk ticks need to elapse  */
	XTmrCtr_SetLoadReg(PLATFORM_TIMER_BASEADDR, 0, TIMER_TLR);

	/* reset the timers, and clear interrupts */
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_INT_OCCURED_MASK | XTC_CSR_LOAD_MASK );

	/* start the timers */
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK | XTC_CSR_ENABLE_INT_MASK | XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_DOWN_COUNT_MASK);

	/* Register Timer handler */
	XIntc_RegisterHandler(XPAR_INTC_0_BASEADDR, PLATFORM_TIMER_INTERRUPT_INTR, (XInterruptHandler)xadapter_timer_handler, 0);
	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, PLATFORM_TIMER_INTERRUPT_MASK);
}

void platform_enable_interrupts()
{
	Xil_ExceptionEnable();
}

static XIntc intc;

void platform_setup_interrupts()
{
	XIntc *intcp;
	intcp = &intc;

	XIntc_Initialize(intcp, XPAR_INTC_0_DEVICE_ID);
	XIntc_Start(intcp, XIN_REAL_MODE);

	platform_setup_timer();
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XIntc_InterruptHandler, intcp);
	XIntc_EnableIntr(XPAR_MICROBLAZE_0_INTC_BASEADDR, PLATFORM_TIMER_INTERRUPT_MASK | XPAR_ETHERNET_MAC_IP2INTC_IRPT_MASK);
	XIntc_Enable(intcp, PLATFORM_TIMER_INTERRUPT_INTR);
	XIntc_Enable(intcp, XPAR_INTC_0_EMACLITE_0_VEC_ID);
}

void enable_caches()
{
	microblaze_invalidate_icache();
	microblaze_enable_icache();
	microblaze_invalidate_dcache();
	microblaze_enable_dcache();
}

void disable_caches()
{
	microblaze_invalidate_dcache();
	microblaze_disable_dcache();
	microblaze_invalidate_icache();
	microblaze_disable_icache();
}

int init_platform()
{
	enable_caches();
	platform_setup_interrupts();

	return 0;
}

void cleanup_platform()
{
	disable_caches();
}

void print_ip(char *msg, struct ip_addr *ip)
{
	print(msg);
	xil_printf("%d.%d.%d.%d\r\n", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{
	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}

void start_applications()
{
    struct tcp_pcb *pcb;
    err_t err;

    /* create new TCP PCB structure */
    pcb = tcp_new();
    if (!pcb)
    {
    	xil_printf("rxperf: Error creating PCB. Out of Memory\r\n");
    	return;
    }

    /* bind to iperf @port */
    err = tcp_bind(pcb, IP_ADDR_ANY, rxperf_port);
    if (err != ERR_OK)
    {
    	xil_printf("rxperf: Unable to bind to port %d: err = %d\r\n", rxperf_port, err);
    	return;
    }

    /* we do not need any arguments to callback functions :) */
    tcp_arg(pcb, NULL);

    /* listen for connections */
    pcb = tcp_listen(pcb);
    if (!pcb)
    {
    	xil_printf("rxperf: Out of memory while tcp_listen\r\n");
    	return;
    }

    /* specify callback to use for incoming connections */
    tcp_accept(pcb, rxperf_accept_callback);

    rxperf_server_running = 1;

    return;
}

err_t rxperf_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    /* close socket if the peer has sent the FIN packet  */
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    xil_printf("receive callback\r\n");

    /* all we do is say we've received the packet */
    /* we don't actually make use of it */
    tcp_recved(tpcb, p->tot_len);

    pbuf_free(p);
    return ERR_OK;
}

err_t rxperf_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    xil_printf("rxperf: Connection Accepted\r\n");
    tcp_recv(newpcb, rxperf_recv_callback);

    return ERR_OK;
}
