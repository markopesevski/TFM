#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/tcp.h"

static unsigned rxperf_port = 5001;	/* iperf default port */
static unsigned rxperf_server_running = 0;

int transfer_rxperf_data()
{
    return 0;
}

static err_t rxperf_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    /* close socket if the peer has sent the FIN packet  */
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

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

int start_rxperf_application()
{
    struct tcp_pcb *pcb;
    err_t err;

    /* create new TCP PCB structure */
    pcb = tcp_new();
    if (!pcb)
    {
    	xil_printf("rxperf: Error creating PCB. Out of Memory\r\n");
    	return -1;
    }

    /* bind to iperf @port */
    err = tcp_bind(pcb, IP_ADDR_ANY, rxperf_port);
    if (err != ERR_OK)
    {
    	xil_printf("rxperf: Unable to bind to port %d: err = %d\r\n", rxperf_port, err);
    	return -2;
    }

    /* we do not need any arguments to callback functions :) */
    tcp_arg(pcb, NULL);

    /* listen for connections */
    pcb = tcp_listen(pcb);
    if (!pcb)
    {
    	xil_printf("rxperf: Out of memory while tcp_listen\r\n");
    	return -3;
    }

    /* specify callback to use for incoming connections */
    tcp_accept(pcb, rxperf_accept_callback);

    rxperf_server_running = 1;

    return 0;
}

void print_rxperf_app_header()
{
    xil_printf("%20s %6d %s\r\n", "rxperf server",
                        rxperf_port,
                        "$ iperf3 -c <Board IP> -i 5 -t 100 -p 5001");
}
