int main(void) {
	/* neteja la consola UART i imprimeix missatge */
	xil_printf("%c[2J",27);
	xil_printf("----- TFM - Marko Peshevski - versio NO-LwIP -----\r\n");
	inicialitza_interrupcions();
	inicialitza_emaclite();
	/* activa les interrupcions a nivell de processador */
	Xil_ExceptionEnable();
	while (1) {
		if (sys.paquet_rebut) {
			sys.paquet_rebut = FALSE; /* neteja el flag */
			/* inverteix les direccions MAC, respon d'on ha vingut el paquet */
			memcpy(trama_ethernet->mac_desti, trama_ethernet->mac_origen, LLARGARIA_MAC);
			memcpy(trama_ethernet->mac_origen, direccio_mac, LLARGARIA_MAC);
			/* mira si el paquet es ARP. necessita girar els bytes */
			if(INVERTEIX_BYTES_16(trama_ethernet->ethertype) == ARP) {
				/* codi a la seguent diapositiva */
			}
			/* mira si es un paquet IPv4. necessita girar els bytes */
			else if(INVERTEIX_BYTES_16(trama_ethernet->ethertype) == IPv4) {
				/* codi a la seguent diapositiva */
			}
		}
		if(sys.paquet_enviat){
			sys.paquet_enviat = FALSE; /* neteja el flag */
		}
	}
	return 0;
}
