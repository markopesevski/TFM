#ifndef __PLATFORM_H_
#define __PLATFORM_H_

	#include "arch/cc.h"
	#include "lwipopts.h"
	#include "xenv_standalone.h"
	#include "xparameters.h"
	#include "xintc.h"
	#include "xil_exception.h"
	#include "mb_interface.h"
	#include "xtmrctr_l.h"
	#include "lwip/tcp.h"
	#include "lwip/inet.h"
	#include "lwip/ip_addr.h"
	#include "xil_printf.h"

	#define INCLUDE_RXPERF_SERVER  0
	#define INCLUDE_TXPERF_CLIENT  0
	#define INCLUDE_TXUPERF_CLIENT 0
	#define INCLUDE_RXUPERF_CLIENT 0

	#define VDD_ADC 3.3

	#define PLATFORM_EMAC_BASEADDR XPAR_EMACLITE_0_BASEADDR
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

	void print_rxperf_app_header();
	void print_txperf_app_header();
	void print_utxperf_app_header();
	void print_urxperf_app_header();
	void start_rxperf_application();
	int start_txperf_application();
	void start_utxperf_application();
	void start_urxperf_application();
	int transfer_rxperf_data();
	int transfer_txperf_data();
	void transfer_utxperf_data();
	void transfer_urxperf_data();
	void print_ip(char *msg, struct ip_addr *ip);
	void print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw);
	void print_headers();
	void start_applications();
	void transfer_data();

	#if LWIP_DHCP==1
		void dhcp_fine_tmr();
		void dhcp_coarse_tmr();
	#endif //LWIP_DHCP

#endif //__PLATFORM_H_
