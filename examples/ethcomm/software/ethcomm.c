 /*
	ethcomm.c
 
   This file is part of The FPGA Ethernet Communications Interface.
   This is a final project from Cornell ECE5760. See:
   <http://people.ece.cornell.edu/land/courses/ece5760/FinalProjects/>
   
   Authors:
		-Michael Spanier (mis47@cornell.edu)
		-Alex Gorenstein (ayg6@cornell.edu)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser Public License for more details.

    You should have received a copy of the GNU Lesser Public License
    along with this program.  If not, see 
	<http://www.gnu.org/licenses/lgpl.html>.
	*/
	
#include "altera_avalon_pio_regs.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
//IOWR_ALTERA_AVALON_PIO_DATA(MAX_ITER_BASE, max_iter);
//result = IORD_ALTERA_AVALON_PIO_DATA(MANLING_RESULT_BASE);

	int ctrl_sel = 0;
	int mem_sel =1;
	
	unsigned int uniqueid;
	
	unsigned char my_MAC1=0x00;
	unsigned char my_MAC2=0xFF;
	unsigned char my_MAC3=0xEE;
	unsigned char my_MAC4=0xF0;
	unsigned char my_MAC5=0xDA;
	unsigned char my_MAC6=0x42;
	
	unsigned int my_IP = 0xC0A8012A; //192.168.1.42
	
	unsigned int gate_IP = 0x0;//ip of dhcp gateway server
	

unsigned char mem_byte_read(int addr){
	unsigned int word = IORD(WISH_BRIDGE_BASE + (addr), 0);
	int byteaddr = addr&0x3;
	int shno = (3-byteaddr)*8;
	word = word>>shno;
	unsigned char byteresult = (word&0xFF);
	return byteresult;
}

int mem_word_read(int addr){
	int word = IORD(WISH_BRIDGE_BASE + (addr), 0);
	return word;
}

void mem_word_write(int addr, unsigned int data){
	IOWR(WISH_BRIDGE_BASE + (addr), 0,data);
}

void mem_byte_write(int addr, unsigned char wrbyte){
	unsigned int data = mem_word_read(addr);
	unsigned int byteaddr = addr&0x3;
	unsigned int mask = 0xFF;
	unsigned int shno = ((3-byteaddr)*8);
	mask = mask<<shno;
	data = data & (~mask); //clears the byte area
	unsigned int shbyte = wrbyte<<shno;
	//sets empyt byte area with new byte:
	data = data | shbyte;
	mem_word_write(addr,data);
	
}

void mem_copy(int rx_addr,int tx_addr,int len){
	//len number of bytes to copy
	unsigned char copybyte;
	unsigned int i;
	for (i=0;i<len;i++){
		copybyte = mem_byte_read(rx_addr + i);
		mem_byte_write(tx_addr + i,copybyte);
	}
}


int interpret_packet(int addr){
	
	IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, mem_sel);
	// 1 means ping
	// 2 means arp
	// 3 means udp
	// 0 means unknown.
	//4 is TCP
	
	
	//ARP: bytes 12 (0xC) and 13 (0xD) are 0x0806
	unsigned char b1 = mem_byte_read(addr+12);
	unsigned char b2 = mem_byte_read(addr+13);
	
	
	if ((b1 == 0x08) && (b2 == 0x06)){
		return 2;//ARP
	}
	
	
	if ((b1 == 0x08) && (b2 == 0x00)){//IP protocol
		
		b1 = mem_byte_read(addr+23);
		
		if (b1==0x01){//ICMP
			return 1;//aka ping
		}
		
		if (b1==0x11){//UDP
			return 3;
		}
		
		if (b1==0x06){//TCP
			return 4;
		}
		
	}
	
	
	
	return 0;
	
}


int request_packet(int addr){
	
	IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
	
	//clear rx interrupt:
	IOWR(WISH_BRIDGE_BASE+0x04, 0, 0x4);
	
	

	IOWR(WISH_BRIDGE_BASE+0x604, 0, addr);
	IOWR(WISH_BRIDGE_BASE+0x600, 0, 0xE000);//enable interrupt

	//printf("Waiting for Packet....\n");
	
	//wait for rx:
	
	int result;
	do{
		result = IORD(WISH_BRIDGE_BASE+0x04, 0);
	}while ((result&0x4)==0);	

	result = IORD(WISH_BRIDGE_BASE+0x600, 0);

	//printf("Packet Recieved. Length: %X bytes\n",(result>>16));
	
	return (result>>16); //packet length
}

void send_packet(int rx_addr, int addr,int length){
	
	
	
				
		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
		//wait for tx buffer to be clear:
		int status;
		do{
			status = IORD(WISH_BRIDGE_BASE+0x400, 0);
		}while ((status&0x8000)==1);	

		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, mem_sel);

		//copy data to tx buffer
		mem_copy(rx_addr,addr,length);


		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
	

	
	//clear tx interrupt:
	IOWR(WISH_BRIDGE_BASE+0x04, 0, 0x1);
	
	IOWR(WISH_BRIDGE_BASE+0x404, 0, addr);
	IOWR(WISH_BRIDGE_BASE+0x400, 0, ((length<<16)|(0xF000)));//enable interrupt
	
	/*
	//wait for tx success:
	int result;
	do{
		result = IORD(WISH_BRIDGE_BASE+0x04, 0);
	}while ((result&0x1)==0);	
	*/
	
}

void configure_eth(){
	IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
	
	//send in MAC address:
	unsigned int MAC0 = (my_MAC3<<0x18) | (my_MAC4<<0x10) | (my_MAC5<<0x8) | my_MAC6;
	unsigned int MAC1 = (my_MAC1<<0x8) | my_MAC2;
	IOWR(WISH_BRIDGE_BASE+0x40, 0, MAC0);
	IOWR(WISH_BRIDGE_BASE+0x44, 0, MAC1);
	
	
	//sending and receiving enabled:
	IOWR(WISH_BRIDGE_BASE+0x0, 0, 0xA003);
}

int buildpacket(int rx_addr,int swap){
	//only for IP type packets (not ARP)
	//if swap is 1, swap destination and source IP address
	
	//swap MAC address
	//extract source MAC
	//MAC is bytes 6 to 11
	unsigned char sMAC1= mem_byte_read(rx_addr+6);
	unsigned char sMAC2 = mem_byte_read(rx_addr+7);
	unsigned char sMAC3 = mem_byte_read(rx_addr+8);
	unsigned char sMAC4= mem_byte_read(rx_addr+9);
	unsigned char sMAC5 = mem_byte_read(rx_addr+10);
	unsigned char sMAC6 = mem_byte_read(rx_addr+11);

	
	
					//write to destination header:
				mem_byte_write(rx_addr+0,sMAC1);
				mem_byte_write(rx_addr+1,sMAC2);
				mem_byte_write(rx_addr+2,sMAC3);
				mem_byte_write(rx_addr+3,sMAC4);
				mem_byte_write(rx_addr+4,sMAC5);
				mem_byte_write(rx_addr+5,sMAC6);
	
	
					//write to source header:
				mem_byte_write(rx_addr+6,my_MAC1);
				mem_byte_write(rx_addr+7,my_MAC2);
				mem_byte_write(rx_addr+8,my_MAC3);
				mem_byte_write(rx_addr+9,my_MAC4);
				mem_byte_write(rx_addr+10,my_MAC5);
				mem_byte_write(rx_addr+11,my_MAC6);	
				
				
					//swap IP address
					if (swap){
					//swap sender/destination ip in ip header
					
			unsigned int sIP1= mem_byte_read(rx_addr+26);
			unsigned int sIP2 = mem_byte_read(rx_addr+27);
			unsigned int sIP3 = mem_byte_read(rx_addr+28);
			unsigned int sIP4 = mem_byte_read(rx_addr+29);
			
			//extract destination IP
			unsigned int dIP1= mem_byte_read(rx_addr+30);
			unsigned int dIP2 = mem_byte_read(rx_addr+31);
			unsigned int dIP3 = mem_byte_read(rx_addr+32);
			unsigned int dIP4 = mem_byte_read(rx_addr+33);
			
				
			//write sender
			mem_byte_write(rx_addr+26,dIP1);
			mem_byte_write(rx_addr+27,dIP2);
			mem_byte_write(rx_addr+28,dIP3);
			mem_byte_write(rx_addr+29,dIP4);
			//write destination
			mem_byte_write(rx_addr+30,sIP1);
			mem_byte_write(rx_addr+31,sIP2);
			mem_byte_write(rx_addr+32,sIP3);
			mem_byte_write(rx_addr+33,sIP4);
		}
				
				
				
				
				
							//increment and place in ip ID number:
			uniqueid++;
			unsigned int upperid = uniqueid >> 8;
			unsigned int lowerid = uniqueid & 0xff;
			mem_byte_write(rx_addr+18,upperid);
			mem_byte_write(rx_addr+19,lowerid);
				
				
				
				
			//get number of bytes in IP header: (lower nibble)
			unsigned int iphsize = (mem_byte_read(rx_addr+14)&0x0F)*4;
			//ip header starts at 14, so udp header starts at 14+iphsize
			
			unsigned int datastart = 14+iphsize;
			
			

				
	
							
			
			return datastart;
				
}



void checksum(int rx_addr, int datastart){
	
				
			unsigned int shi;
			unsigned int slo;
			unsigned int sum;
				
	//calculate IPv4 checksum: (of header)
			//set checksum bits to zero
			mem_byte_write(rx_addr+24,0x00);
			mem_byte_write(rx_addr+25,0x00);
			
			sum =0;
			int i;
			for (i=14; i<(datastart-1); i=i+2){
				shi = mem_byte_read(rx_addr+i);
				slo = mem_byte_read(rx_addr+i+1);
				sum = sum + ((shi<<8) | slo);
			}
			
			//add overflow bits (upper 16) to lower 16 bits:
			sum = (sum>>16) + (sum & 0xFFFF);
			sum = ~sum;
			//printf("Final checksum:%X\n",sum);
			//write checksum to bytes icmpstart+2 and icmpstart+3
			mem_byte_write(rx_addr+24, sum>>8 );//hi byte
			mem_byte_write(rx_addr+25, sum & 0xff );//lo byte
	
}

int verifyIP(rx_addr){
	//IPv4, verify destination IP address is a match;
	
		//check packet destination!!!
	
	//extract destination IP  (38 to 41)
	unsigned int dIP1= mem_byte_read(rx_addr+30);
	unsigned int dIP2 = mem_byte_read(rx_addr+31);
	unsigned int dIP3 = mem_byte_read(rx_addr+32);
	unsigned int dIP4 = mem_byte_read(rx_addr+33);
	unsigned int dest_IP = (dIP1<<0x18) | (dIP2<<0x10) | (dIP3<<0x8) | dIP4;
			
	if ((dest_IP==my_IP) ||(dest_IP==0xffffffff)){
		
		return 1;
	}else {
		return 0;
	}
	
}


int memorytest(){
	IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, mem_sel);
	int success = 1;
	int k;
	int l;
	unsigned int result;
	unsigned int dat;
	for (l=0; l<256; l++){
		for (k=0; k<256; k++){
			mem_byte_write(l, k);
			result = mem_byte_read(l);
			if (k!=result){
				printf("Test failed at byte %X: wrote:%X, got:%X\n",l,k,result);
				success = 0;
				break;
			}
		}
	}
	return success;
}


/*
int mii_op(int addr, int data, int we){
	int result=0;
	IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
	int phys_address = 0x0;
	IOWR(WISH_BRIDGE_BASE+0x30, 0, ((addr<<8) | (phys_address)));
	if (we){
		IOWR(WISH_BRIDGE_BASE+0x34, 0, data);
		IOWR(WISH_BRIDGE_BASE+0x2c, 0, 0x2);
		
	}else {
		IOWR(WISH_BRIDGE_BASE+0x2c, 0, 0x4);
		result = IORD(WISH_BRIDGE_BASE+0x038, 0);
	}
	
	return result;
}
*/
/*
long get_mac_address(){
	
	//mac_0 is 0x3, mac_1 is 0x4
	
	
	long mac=0;
	
	return mac;
}
*/

/* function dump_line
(from: http://www.drpaulcarter.com/cs/common-c-errors.php#3.3 )
*  This function reads and dumps any remaining characters on the current input
*  line of a file.
*  Parameter:
*     fp - pointer to a FILE to read characters from
*  Precondition:
*     fp points to a open file
*  Postcondition:
*     the file referenced by fp is positioned at the end of the next line
*     or the end of the file.
*/
void dump_line( FILE * fp )
{
  int ch;

  while( (ch = fgetc(fp)) != EOF && ch != '\n' )
    /* null body */;
}
	
int main()
{
	unsigned int writemode;
	unsigned int addr;
	unsigned int data=0;
	unsigned int length;
	unsigned int i;
	unsigned int wishsel;
	unsigned int result;
	unsigned int state;
	
	
	uniqueid = 0x1111;

	
  printf("Hello from Nios II!\n");
	
	
	
	printf("Choose Operation:\n0-Debug, 1-Automatic Mode:");
	scanf ("%d",&state);
	if (state==1){
		unsigned int rx_addr=0x0;//store packet at SRAM memory address 0
		unsigned int tx_addr=0x100;//where to place transmit packet
			
		configure_eth();
		
		
int notdone;

	
		do{
			notdone=0;
		request_packet(rx_addr);
		
		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, mem_sel);

		int packettype = interpret_packet(rx_addr);
			
			
		
		if ((packettype==1) && verifyIP(rx_addr)){
			
		//TO REPLY:
			unsigned int icmpstart = buildpacket(rx_addr,1);
			
			
			
			//change type to zero
			mem_byte_write(rx_addr+icmpstart,0x00);
			
			
			//unsigned int datlen_0 = mem_byte_read(rx_addr+16);
			unsigned int datlen_1 = mem_byte_read(rx_addr+17);
			//unsigned long datlen= datlen_0<<32 | datlen_1;
			
			unsigned int len = (14 + datlen_1);//length in bytes AFTER byte # 13
			//printf("length:%X\n",len);
			
	
			//break ICMP section into 16-bit words
			//add them together
			//then invert (NOT).
			//we can update the ICMP checksum using the request checksum
			unsigned int shi;
			unsigned int slo;
			unsigned int sum;

			shi = mem_byte_read(rx_addr+icmpstart+2);
			slo = mem_byte_read(rx_addr+icmpstart+3);
			sum = shi<<8 | slo;
			//first, uninvert
			sum = ~sum;
			
			//subtract 0x0800
			sum = sum - 0x0800;
			
			//re-invert
			sum = ~sum;
			
			//printf("Final checksum:%X\n",sum);
			//write checksum to bytes icmpstart+2 and icmpstart+3
			mem_byte_write(rx_addr+icmpstart+2, sum>>8 );//hi byte
			mem_byte_write(rx_addr+icmpstart+3, sum & 0xff );//lo byte
			
			checksum(rx_addr,icmpstart);

			
			send_packet(rx_addr,tx_addr,len);
				
			printf("Ping (ICMP) response sent\n");
				
			notdone=1;
			
		} else if (packettype==2){
			//extract source IP: bytes 28 to 31
			unsigned int sIP1= mem_byte_read(rx_addr+28);
			unsigned int sIP2 = mem_byte_read(rx_addr+29);
			unsigned int sIP3 = mem_byte_read(rx_addr+30);
			unsigned int sIP4 = mem_byte_read(rx_addr+31);
			//int src_IP = (sIP1<<0x18) | (sIP2<<0x10) | (sIP3<<0x8) | sIP4;
			
			//extract destination IP  (38 to 41)
			unsigned int dIP1= mem_byte_read(rx_addr+38);
			unsigned int dIP2 = mem_byte_read(rx_addr+39);
			unsigned int dIP3 = mem_byte_read(rx_addr+40);
			unsigned int dIP4 = mem_byte_read(rx_addr+41);
			unsigned int dest_IP = (dIP1<<0x18) | (dIP2<<0x10) | (dIP3<<0x8) | dIP4;
			
			//operation byte (@21): 1 means request; 2 means reply
			
			unsigned char opcode = mem_byte_read(rx_addr+21);
			
			
			//cross-check IP and verify it's a request
			
			if ((dest_IP==my_IP) &&(opcode==1)){
				
			//extract source MAC
			//MAC is bytes 22 to 27
			unsigned char sMAC1= mem_byte_read(rx_addr+22);
			unsigned char sMAC2 = mem_byte_read(rx_addr+23);
			unsigned char sMAC3 = mem_byte_read(rx_addr+24);
			//int src_MAC_0 = (sMACb1<<0x10) | (sMACb2<<0x8) |sMACb3;
			
			unsigned char sMAC4= mem_byte_read(rx_addr+25);
			unsigned char sMAC5 = mem_byte_read(rx_addr+26);
			unsigned char sMAC6 = mem_byte_read(rx_addr+27);
			//int src_MAC_1 = (sMACb4<<0x10) | (sMACb5<<0x8) | sMACb6;
				
				
				
			//edit packet:
			//swap destination with source MAC address:
				
				//write to destination header:
				mem_byte_write(rx_addr+0,sMAC1);
				mem_byte_write(rx_addr+1,sMAC2);
				mem_byte_write(rx_addr+2,sMAC3);
				mem_byte_write(rx_addr+3,sMAC4);
				mem_byte_write(rx_addr+4,sMAC5);
				mem_byte_write(rx_addr+5,sMAC6);
				//write to arp destination (32 to 37)
				mem_byte_write(rx_addr+32,sMAC1);
				mem_byte_write(rx_addr+33,sMAC2);
				mem_byte_write(rx_addr+34,sMAC3);
				mem_byte_write(rx_addr+35,sMAC4);
				mem_byte_write(rx_addr+36,sMAC5);
				mem_byte_write(rx_addr+37,sMAC6);
				
				//write to source header:
				mem_byte_write(rx_addr+6,my_MAC1);
				mem_byte_write(rx_addr+7,my_MAC2);
				mem_byte_write(rx_addr+8,my_MAC3);
				mem_byte_write(rx_addr+9,my_MAC4);
				mem_byte_write(rx_addr+10,my_MAC5);
				mem_byte_write(rx_addr+11,my_MAC6);				
				//write to arp source:
				mem_byte_write(rx_addr+22,my_MAC1);
				mem_byte_write(rx_addr+23,my_MAC2);
				mem_byte_write(rx_addr+24,my_MAC3);
				mem_byte_write(rx_addr+25,my_MAC4);
				mem_byte_write(rx_addr+26,my_MAC5);
				mem_byte_write(rx_addr+27,my_MAC6);	
			
			//swap sender/destination IP
			//write sender
			mem_byte_write(rx_addr+28,dIP1);
			mem_byte_write(rx_addr+29,dIP2);
			mem_byte_write(rx_addr+30,dIP3);
			mem_byte_write(rx_addr+31,dIP4);
			//write destination
			mem_byte_write(rx_addr+38,sIP1);
			mem_byte_write(rx_addr+39,sIP2);
			mem_byte_write(rx_addr+40,sIP3);
			mem_byte_write(rx_addr+41,sIP4);
			
			//change request to reply
			mem_byte_write(rx_addr+21,0x02);//2 means reply
			
			
			int len=0x2A;//length in bytes (42): ARP's are 42 bytes
			
				
			send_packet(rx_addr,tx_addr,len);
			
			printf("ARP response sent.\n");	
			printf("..to MAC:%2X%2X%2X%2X%2X%2X\n",sMAC1,sMAC2,sMAC3,sMAC4,sMAC5,sMAC6);
			printf("..from (my) MAC:%2X%2X%2X%2X%2X%2X\n",my_MAC1,my_MAC2,my_MAC3,my_MAC4,my_MAC5,my_MAC6);	
			
			notdone=1;
			}else{
				//packet intended for someone else
				//printf("Picked up someone else's packet (wrong IP)\n");
				//printf("Destination IP:%X\n",dest_IP);
				notdone=1;
			}
			
			


			
		} else if ((packettype==3) && verifyIP(rx_addr)){
			
			//printf("UDP Recieved\n");
			
			unsigned int udpstart = buildpacket(rx_addr,1);
			unsigned int dstport0 = mem_byte_read(rx_addr+udpstart+2);
			unsigned int dstport1 = mem_byte_read(rx_addr+udpstart+3);
			unsigned int srcport0 = mem_byte_read(rx_addr+udpstart);
			unsigned int srcport1 = mem_byte_read(rx_addr+udpstart+1);
			
			
			//length is total length of UDP frame
			unsigned int udp_dat_len0 = mem_byte_read(rx_addr+udpstart+4);
			unsigned int udp_dat_len1 = mem_byte_read(rx_addr+udpstart+5);
			unsigned int udp_dat_len = (udp_dat_len0<<8) | udp_dat_len1;
			
			//get and display UDP data:
			//data (string) is from udpstart+8 to udpstart+udp_dat_len-1
			unsigned int datlen = udp_dat_len-8;
			
			char instr[datlen+1];
			for (i=0; i<datlen;i++){
				unsigned int databyte = mem_byte_read(rx_addr+udpstart+8+i);
				instr[i] = (unsigned char) databyte;
			}
			instr[datlen] = '\0';//add null terminator
			printf(">>%s\n",instr);
			
			//char sendstr[50]="auto-reply\0";
char sendstr[50];
			//scanf("Response:>>%s\n",sendstr);
			printf(">>");
			dump_line(stdin);
			fgets(sendstr, 49, stdin);
			printf("\nSending:%49s",sendstr);
			
			//add data padding: need at least 8 bytes!
			unsigned int sendlen = strlen(sendstr);
			//insert string into data:
			for (i=0; i<sendlen;i++){
				mem_byte_write(rx_addr+udpstart+8+i,sendstr[i]);
			}
			sendlen = sendlen+8;
			sendlen = sendlen-5;
			//update udp length:
			mem_byte_write(rx_addr+udpstart+4,sendlen>>8);
			mem_byte_write(rx_addr+udpstart+5,sendlen&0xff);
			
			
			//build the rest of the packet:
					
					//swap destination and source ports:
				mem_byte_write(rx_addr+udpstart,dstport0);
				mem_byte_write(rx_addr+udpstart+1,dstport1);
				mem_byte_write(rx_addr+udpstart+2,srcport0);
				mem_byte_write(rx_addr+udpstart+3,srcport1);	
					
				
			
			
			
			//unsigned int datlen_0 = mem_byte_read(rx_addr+16);
			//unsigned int datlen_1 = mem_byte_read(rx_addr+17);
			//unsigned long datlen= datlen_0<<32 | datlen_1;
			
			//unsigned int len = (14 + datlen_1);//length in bytes AFTER byte # 13
			
			
			unsigned int totallen = 20 + sendlen;
			unsigned int iplen = totallen +13;
			//mem_byte_write(rx_addr+udpstart+16,totallen>>32);
			mem_byte_write(rx_addr+udpstart+17,totallen&0xffffffff);
			
			
			
			
							//send packet:
				checksum(rx_addr,udpstart);
				send_packet(rx_addr,tx_addr,iplen);
		
			notdone=1;
			
			
		} else if ((packettype==4) && verifyIP(rx_addr)){
				//TCP packet
				printf("TCP Recieved\n");
				
				
			
			
			
			
			notdone=1;
				
		} else {
			//printf("Unknown Packet Recieved\n");
			notdone=1;
		}
		
		
		
		
		
	}while(notdone);
		
		//printf("Turning off Eth RX/TX:\n");
		//IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
		//IOWR(WISH_BRIDGE_BASE+0x0, 0, 0xA000);
		
	}
	
	printf("\nDebug Mode:\n");
	while(1){
		
	
		
	printf("Select location:\n0-Ethmac, 1-Memory:");
	scanf ("%d",&wishsel);
			
	printf("Select Mode:\n0-Read, 1-Write, 2-ByteRead, 3-ByteWrite,4-Memtest:");
	scanf ("%d",&writemode);
			
	printf("Enter Address (32-bit Hex):");
	scanf ("%X",&addr);
	
	if ((writemode == 1)||(writemode==3)){ //write
		printf("Enter Write Data (32-bit Hex):");
		scanf ("%X",&data);
	}
	

		
	if (wishsel==0){
		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, ctrl_sel);
		
		if (writemode==1){
			IOWR(WISH_BRIDGE_BASE+addr, 0, data);
		}else{
			result = IORD(WISH_BRIDGE_BASE+addr, 0);
			printf("|%.8X|\n",result);
			
		}
		

	} else {
		IOWR_ALTERA_AVALON_PIO_DATA(WISH_SEL_BASE, mem_sel);
		
		if (writemode==0){//read word
			result = mem_word_read(addr);
			printf("|%.8X|\n",result);
		}else if(writemode==1){//write word
			mem_word_write(addr,data);
		}else if(writemode==2){//read byte
			unsigned char resultbyte;
			printf("Number of consecutive bytes to read (32-bit Hex):");
			scanf ("%X",&length);
			if (length>0){
				for (i=0; i<length; i++){
					resultbyte = mem_byte_read(addr+i);
					 if (i%8==0){
						 printf("  ");
					 }
					 if (i%16==0){
						 printf("\n");
					 }
					printf(" %.2X",resultbyte);
				}
				printf("\n");
			}else{
				result = IORD(WISH_BRIDGE_BASE+addr, 0);
				printf("(length zero)|%.8X|\n",result);
			}
		}else if(writemode==3){//write byte
			mem_byte_write(addr,data);
		}else if(writemode==4){
			if (memorytest()){
				printf("MemTest Pass\n");
				
			}
		}
		
	}
	
	
}	// end while(1)
	
	
  return 0;
}
