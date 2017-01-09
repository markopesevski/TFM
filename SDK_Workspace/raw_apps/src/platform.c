#include "platform.h"

volatile int TcpFastTmrFlag = 0;
volatile int TcpSlowTmrFlag = 0;
volatile int fastestTmrFlag = 0;
volatile int fastestTmrCount = 0;

#if LWIP_DHCP==1
	volatile int dhcp_timoutcntr = 24;
#endif //LWIP_DHCP

volatile int TxPerfConnMonCntr = 0;

void timer_callback()
{
	static int odd = 1;
	#if LWIP_DHCP==1
		static int dhcp_timer = 0;
	#endif //LWIP_DHCP

	fastestTmrFlag = 1;

	if(fastestTmrCount >= 3)
	{
		fastestTmrCount = 0;
		TcpFastTmrFlag = 1;

		odd = !odd;
		if (odd)
		{
			#if LWIP_DHCP==1
				dhcp_timer++;
				dhcp_timoutcntr--;
			#endif //LWIP_DHCP

			TcpSlowTmrFlag = 1;

			#if LWIP_DHCP==1
				dhcp_fine_tmr();
				if (dhcp_timer >= 120)
				{
					dhcp_coarse_tmr();
					dhcp_timer = 0;
				}
			#endif //LWIP_DHCP
		}
	}

	fastestTmrCount++;
}

void xadapter_timer_handler(void *p)
{
	timer_callback();

	/* Load timer, clear interrupt bit */
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_INT_OCCURED_MASK | XTC_CSR_LOAD_MASK);
	XTmrCtr_SetControlStatusReg(PLATFORM_TIMER_BASEADDR, 0, XTC_CSR_ENABLE_TMR_MASK | XTC_CSR_ENABLE_INT_MASK | XTC_CSR_AUTO_RELOAD_MASK | XTC_CSR_DOWN_COUNT_MASK);

	/* Clear interrupt bit */
	XIntc_AckIntr(XPAR_INTC_0_BASEADDR, PLATFORM_TIMER_INTERRUPT_MASK);
}

//#define TIMER_TLR (XPAR_TMRCTR_0_CLOCK_FREQ_HZ / 4)
#define TIMER_TLR (XPAR_TMRCTR_0_CLOCK_FREQ_HZ / 12)
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
	//microblaze_enable_exceptions();
	//microblaze_enable_interrupts();
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

void print_headers()
{
    xil_printf("\r\n");
    xil_printf("%20s %6s %s\r\n", "Server", "Port", "Connect With..");
    xil_printf("%20s %6s %s\r\n", "--------------------", "------", "--------------------");

    if (INCLUDE_RXPERF_SERVER)
        print_rxperf_app_header();

    if (INCLUDE_TXPERF_CLIENT)
        print_txperf_app_header();

    if (INCLUDE_TXUPERF_CLIENT)
    	print_utxperf_app_header();

    if (INCLUDE_RXUPERF_CLIENT)
    	print_urxperf_app_header();

    xil_printf("\r\n");
}

void start_applications()
{
    if (INCLUDE_RXPERF_SERVER)
        start_rxperf_application();

    if (INCLUDE_TXPERF_CLIENT)
        start_txperf_application();

    if (INCLUDE_TXUPERF_CLIENT)
    	start_utxperf_application();

    if (INCLUDE_RXUPERF_CLIENT)
    	start_urxperf_application();
}

void transfer_data()
{
    if (INCLUDE_RXPERF_SERVER)
        transfer_rxperf_data();

    if (INCLUDE_TXPERF_CLIENT)
        transfer_txperf_data();

    if (INCLUDE_TXUPERF_CLIENT)
    	transfer_utxperf_data();

    if (INCLUDE_RXUPERF_CLIENT)
    	transfer_urxperf_data();
}
