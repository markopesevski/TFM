int main(void) {
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
	while (1) {
		/* rep dades de la interficie de xarxa (driver ethernet MAC) */
		xemacif_input(&interficie_xarxa);
		/* consulta temporitzadors i executa tasques periodiques */
		if (temporitzador_tcp_rapid) {
			tcp_fasttmr();
			temporitzador_tcp_rapid = 0;
		}
		if (temporitzador_tcp_lent) {
			tcp_slowtmr();
			temporitzador_tcp_lent = 0;
		}
	}
	return 0;
}
