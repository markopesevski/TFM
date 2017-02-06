/* simplement es un cast que interpreta les dades de memoria per facilitar */
paquet_ip_t * paquet_ip = (paquet_ip_t *) &trama_ethernet->dades;
/* mira si es un paquet de tipus ICMP */
if(paquet_ip->protocol == ICMP) {
	/* simplement es un cast que interpreta les dades de memoria per facilitar */
	paquet_icmp_t * paquet_icmp = (paquet_icmp_t *) &paquet_ip->dades;
	/* mira si es una peticio d'eco */
	if(paquet_icmp->tipus_de_missatge == ECHO_REQUEST) {
		/* canvia la suma de verificacio */
		paquet_icmp->suma_verificacio += ECHO_REQUEST; /* -ECHO_REPLY */
		/* modifica el tipus de missatge */
		paquet_icmp->tipus_de_missatge = ECHO_REPLY;
		/* inverteix les adreces del paquet IPv4 */
		paquet_ip->ip_desti = paquet_ip->ip_origen;
		paquet_ip->ip_origen = direccio_ip;
		/* envia la resposta */
		XEmacLite_Send(&emaclite, buffer, sys.llargaria_paquet_rebut - LLARGARIA_FCS);
	}
}
