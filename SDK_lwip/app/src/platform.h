#ifndef __PLATFORM_H_
#define __PLATFORM_H_

	#include "xintc.h"
	#include "xil_exception.h"
	#include "xtmrctr_l.h"
	#include "lwip/tcp.h"

	#define PLATFORM_EMAC_BASEADDR XPAR_EMACLITE_0_BASEADDR
	/* timer configuration */
	#define PLATFORM_TIMER_BASEADDR XPAR_TMRCTR_0_BASEADDR
	#define PLATFORM_TIMER_INTERRUPT_INTR XPAR_INTC_0_TMRCTR_0_VEC_ID
	#define PLATFORM_TIMER_INTERRUPT_MASK (1 << PLATFORM_TIMER_INTERRUPT_INTR)

	int init_platform();
	void cleanup_platform();
	void platform_enable_interrupts();
	void timer_callback();
	void xadapter_timer_handler(void *p);
	void platform_setup_timer();
	void platform_enable_interrupts();
	void platform_setup_interrupts();
	void enable_caches();
	void disable_caches();
	int init_platform();
	void cleanup_platform();

	void print_ip(char *msg, struct ip_addr *ip);
	void print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw);
	void start_applications();
	err_t rxperf_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	err_t rxperf_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);

#endif //__PLATFORM_H_
