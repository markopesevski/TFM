/* simplement es un cast que interpreta les dades de memoria per facilitar */
paquet_arp_t * paquet_arp = (paquet_arp_t *) &trama_ethernet->dades;
/* mira si el paquet anava dirigit per la nostra IP */
if(paquet_arp->ip_desti == direccio_ip) {
	if(paquet_arp->operacio == ARP_REQUEST_LINUX) {
		paquet_arp->operacio = ARP_REPLY_LINUX;
	}
	else if(paquet_arp->operacio == ARP_REQUEST_WINDOWS) {
		paquet_arp->operacio = ARP_REPLY_WINDOWS;
	}
	/* inverteix adreces IP i MAC del paquet */
	paquet_arp->ip_desti = paquet_arp->ip_origen;
	paquet_arp->ip_origen = direccio_ip;
	memcpy(paquet_arp->mac_desti, paquet_arp->mac_origen, LLARGARIA_MAC);
	memcpy(paquet_arp->mac_origen, direccio_mac, LLARGARIA_MAC);
	/* envia la resposta */
	XEmacLite_Send(&emaclite, buffer, sys.llargaria_paquet_rebut - LLARGARIA_FCS);
}
