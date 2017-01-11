 /*
	SHIP.v
 
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
	

module SHIP(

	//////////// CLOCK //////////
	CLOCK_50,
	CLOCK2_50,
	CLOCK3_50,

	//////////// LED //////////
	LEDG,
	LEDR,

	//////////// KEY //////////
	KEY,

	//////////// SW //////////
	SW,

	//////////// SEG7 //////////
	HEX0,
	HEX1,
	HEX2,
	HEX3,
	HEX4,
	HEX5,
	HEX6,
	HEX7,

	//////////// LCD //////////
	LCD_BLON,
	LCD_DATA,
	LCD_EN,
	LCD_ON,
	LCD_RS,
	LCD_RW,

	//////////// RS232 //////////
	UART_CTS,
	UART_RTS,
	UART_RXD,
	UART_TXD,
/*
	//////////// VGA //////////
	VGA_B,
	VGA_BLANK_N,
	VGA_CLK,
	VGA_G,
	VGA_HS,
	VGA_R,
	VGA_SYNC_N,
	VGA_VS,
*/
	//////////// Ethernet 0 //////////
	ENET0_GTX_CLK,
	ENET0_INT_N,
	ENET0_LINK100,
	ENET0_MDC,
	ENET0_MDIO,
	ENET0_RST_N,
	ENET0_RX_CLK,
	ENET0_RX_COL,
	ENET0_RX_CRS,
	ENET0_RX_DATA,
	ENET0_RX_DV,
	ENET0_RX_ER,
	ENET0_TX_CLK,
	ENET0_TX_DATA,
	ENET0_TX_EN,
	ENET0_TX_ER,
	ENETCLK_25,
/*
	//////////// Ethernet 1 //////////
	ENET1_GTX_CLK,
	ENET1_INT_N,
	ENET1_LINK100,
	ENET1_MDC,
	ENET1_MDIO,
	ENET1_RST_N,
	ENET1_RX_CLK,
	ENET1_RX_COL,
	ENET1_RX_CRS,
	ENET1_RX_DATA,
	ENET1_RX_DV,
	ENET1_RX_ER,
	ENET1_TX_CLK,
	ENET1_TX_DATA,
	ENET1_TX_EN,
	ENET1_TX_ER,
*/
	//////////// SDRAM //////////
	DRAM_ADDR,
	DRAM_BA,
	DRAM_CAS_N,
	DRAM_CKE,
	DRAM_CLK,
	DRAM_CS_N,
	DRAM_DQ,
	DRAM_DQM,
	DRAM_RAS_N,
	DRAM_WE_N,

	//////////// SRAM //////////
	SRAM_ADDR,
	SRAM_CE_N,
	SRAM_DQ,
	SRAM_LB_N,
	SRAM_OE_N,
	SRAM_UB_N,
	SRAM_WE_N 
);

//=======================================================
//  PARAMETER declarations
//=======================================================


//=======================================================
//  PORT declarations
//=======================================================

//////////// CLOCK //////////
input		          		CLOCK_50;
input		          		CLOCK2_50;
input		          		CLOCK3_50;

//////////// LED //////////
output		     [8:0]		LEDG;
output		    [17:0]		LEDR;

//////////// KEY //////////
input		     [3:0]		KEY;

//////////// SW //////////
input		    [17:0]		SW;

//////////// SEG7 //////////
output		     [6:0]		HEX0;
output		     [6:0]		HEX1;
output		     [6:0]		HEX2;
output		     [6:0]		HEX3;
output		     [6:0]		HEX4;
output		     [6:0]		HEX5;
output		     [6:0]		HEX6;
output		     [6:0]		HEX7;

//////////// LCD //////////
output		          		LCD_BLON;
inout		     [7:0]		LCD_DATA;
output		          		LCD_EN;
output		          		LCD_ON;
output		          		LCD_RS;
output		          		LCD_RW;

//////////// RS232 //////////
output		          		UART_CTS;
input		          		UART_RTS;
input		          		UART_RXD;
output		          		UART_TXD;
/*
//////////// VGA //////////
output		     [7:0]		VGA_B;
output		          		VGA_BLANK_N;
output		          		VGA_CLK;
output		     [7:0]		VGA_G;
output		          		VGA_HS;
output		     [7:0]		VGA_R;
output		          		VGA_SYNC_N;
output		          		VGA_VS;
*/
//////////// Ethernet 0 //////////
output		          		ENET0_GTX_CLK;		// GMII Transmit Clock
input		          		ENET0_INT_N; 			// Interrupt open drain output
input		          		ENET0_LINK100; 		// Parallel LED output of 100BASE-TX link
output		          		ENET0_MDC; 			// Management data clock reference
inout		          		ENET0_MDIO;				// Management Data
output		          		ENET0_RST_N;		// Hardware reset Signal
input		          		ENET0_RX_CLK;			// GMII/MII Receive clock
input		          		ENET0_RX_COL;			// GMII/MII Collision
input		          		ENET0_RX_CRS;			// GMII/MII Carrier sense
input		     [3:0]		ENET0_RX_DATA;			// GMII/MII Receive data
input		          		ENET0_RX_DV;			// GMII/MII Receive data valid
input		          		ENET0_RX_ER;			// GMII/MII Receive error
input		          		ENET0_TX_CLK;			// MII Transmit Clock
output		     [3:0]		ENET0_TX_DATA;		// MII Transmit Data
output		          		ENET0_TX_EN;		// GMII/MII Transmit enable
output		          		ENET0_TX_ER;		// GMII/MII Transmit error

input		          		ENETCLK_25; // Internal Clock (SHARED) 25MHZ


/*
//////////// Ethernet 1 //////////
output		          		ENET1_GTX_CLK;
input		          		ENET1_INT_N;
input		          		ENET1_LINK100;
output		          		ENET1_MDC;
inout		          		ENET1_MDIO;
output		          		ENET1_RST_N;
input		          		ENET1_RX_CLK;
input		          		ENET1_RX_COL;
input		          		ENET1_RX_CRS;
input		     [3:0]		ENET1_RX_DATA;
input		          		ENET1_RX_DV;
input		          		ENET1_RX_ER;
input		          		ENET1_TX_CLK;
output		     [3:0]		ENET1_TX_DATA;
output		          		ENET1_TX_EN;
output		          		ENET1_TX_ER;
*/
//////////// SDRAM //////////
output		    [12:0]		DRAM_ADDR;
output		     [1:0]		DRAM_BA;
output		          		DRAM_CAS_N;
output		          		DRAM_CKE;
output		          		DRAM_CLK;
output		          		DRAM_CS_N;
inout		    [31:0]		DRAM_DQ;
output		     [3:0]		DRAM_DQM;
output		          		DRAM_RAS_N;
output		          		DRAM_WE_N;

//////////// SRAM //////////
output		    [19:0]		SRAM_ADDR;
output		          		SRAM_CE_N;
inout		    [15:0]		SRAM_DQ;
output		          		SRAM_LB_N;
output		          		SRAM_OE_N;
output		          		SRAM_UB_N;
output		          		SRAM_WE_N;


//=======================================================
//  REG/WIRE declarations
//=======================================================







//=======NIOS

packettester niostester(
	 // 1) global signals:
	  .clk_0(CLOCK_50),
	  .reset_n(reset),
	.clocks_0_SDRAM_CLK_out(DRAM_CLK),
	
	  
			// the_sdram_0
	  .zs_addr_from_the_sdram_0(DRAM_ADDR),
	  .zs_ba_from_the_sdram_0(DRAM_BA),
	  .zs_cas_n_from_the_sdram_0(DRAM_CAS_N),
	  .zs_cke_from_the_sdram_0(DRAM_CKE),
	  .zs_cs_n_from_the_sdram_0(DRAM_CS_N),
	  .zs_dq_to_and_from_the_sdram_0(DRAM_DQ),
	  .zs_dqm_from_the_sdram_0(DRAM_DQM),
	  .zs_ras_n_from_the_sdram_0(DRAM_RAS_N),
	  .zs_we_n_from_the_sdram_0(DRAM_WE_N),
	  
	  	   // the_wish_sel
	  .out_port_from_the_wish_sel(wishsel),
	  
	  	   // ethernet interrupt port
	  .in_port_to_the_eth_irq(eth_irq),
	  
			// the_wish_bridge
	  .acknowledge_to_the_wish_bridge(br_ack),
	  .address_from_the_wish_bridge(br_addr),
	  .bus_enable_from_the_wish_bridge(br_en),
	  .byte_enable_from_the_wish_bridge(br_sel),
	  .irq_to_the_wish_bridge(br_irq),
	  .read_data_to_the_wish_bridge(br_rdata),
	  .rw_from_the_wish_bridge(br_rw),
	  .write_data_from_the_wish_bridge(br_wdata)

 
 );
 
 // wishbone to memory
wire [31:0] ms_addr,ms_dat_o,ms_dat_i;
wire [3:0] ms_sel;
wire ms_we,ms_cyc,ms_stb,ms_ack,ms_err;


 wire wishsel;
 
 //from nios
 wire [31:0] br_rdata,br_wdata;
 wire br_ack, br_en, br_irq, br_rw;
 wire [14:0] br_addr;
 wire [3:0] br_sel;
 
// nios to ctrl port

wire ctrl_cyc,ctrl_stb,ctrl_ack,ctrl_err;
wire [31:0] ctrl_dat_r;

// nios to memory

wire mem_cyc,mem_stb,mem_ack,mem_err;
wire [31:0] mem_dat_r;

assign mem_stb = mem_cyc;
assign ctrl_stb = ctrl_cyc;

assign mem_cyc = br_en&wishsel;
assign ctrl_cyc = br_en&(~wishsel);

assign br_rdata = wishsel?mem_dat_r:ctrl_dat_r;
assign br_ack = wishsel?mem_ack:ctrl_ack;

wire [31:0] com_dat_w = br_wdata;
wire [31:0] com_addr = {7'b0,br_addr};
wire [3:0] com_sel = br_sel;
wire com_we = ~br_rw;

 


ethmac ethwish1(
  // WISHBONE common
  .wb_clk_i(CLOCK_50),
  .wb_rst_i(~reset),


  // WISHBONE slave
  .wb_adr_i(com_addr[11:2]),
  .wb_sel_i(com_sel),
  .wb_we_i(com_we),
  .wb_cyc_i(ctrl_cyc),
  .wb_stb_i(ctrl_stb),
  .wb_ack_o(ctrl_ack),
  .wb_err_o(ctrl_err),
  
  .wb_dat_i(com_dat_w),
  .wb_dat_o(ctrl_dat_r),

  // WISHBONE master
  .m_wb_adr_o(ms_addr),
  .m_wb_sel_o(ms_sel),
  .m_wb_we_o(ms_we),
  .m_wb_dat_o(ms_dat_o),
  .m_wb_dat_i(ms_dat_i),
  .m_wb_cyc_o(ms_cyc),
  .m_wb_stb_o(ms_stb),
  .m_wb_ack_i(ms_ack),
  .m_wb_err_i(ms_err),



  //TX
  .mtx_clk_pad_i(ENET0_TX_CLK),
  .mtxd_pad_o(ENET0_TX_DATA),
  .mtxen_pad_o(ENET0_TX_EN),
  .mtxerr_pad_o(ENET0_TX_ER),

  //RX
  .mrx_clk_pad_i(ENET0_RX_CLK),
  .mrxd_pad_i(ENET0_RX_DATA),
  .mrxdv_pad_i(ENET0_RX_DV),
  .mrxerr_pad_i(ENET0_RX_ER),
  .mcoll_pad_i(ENET0_RX_COL),
  .mcrs_pad_i(ENET0_RX_CRS),
  
  // MIIM
  .mdc_pad_o(ENET0_MDC),
  .md_pad_i(md_in),
  .md_pad_o(md_out),
  .md_padoe_o(md_we),

  .int_o(eth_irq)

);
//convert tristate pin to two separate lines:
wire md_we, md_out,md_in;
assign ENET0_MDIO = md_we? md_out:1'bz;
assign md_in = ENET0_MDIO;

assign ENET0_RST_N = reset;

wire eth_irq;

//don't forget reset-delay on ethernet chip




// reset
wire reset ;
assign reset = KEY[0] ;
//assign LEDG[7] = KEY[2] ;
//==================================


//memory has two wishbone interfaces, NIOS takes #1, which has priority.



memory_wb_slave sharedmem (

//////////// SRAM //////////
.SRAM_ADDR(SRAM_ADDR),
.SRAM_CE_N(SRAM_CE_N),
.SRAM_DQ(SRAM_DQ),
.SRAM_LB_N(SRAM_LB_N),
.SRAM_OE_N(SRAM_OE_N),
.SRAM_UB_N(SRAM_UB_N),
.SRAM_WE_N(SRAM_WE_N),


// WISHBONE common
.wb_clk_i(CLOCK_50),     // WISHBONE clock
.wb_rst_i(~reset),     // WISHBONE reset

// WISHBONE slave 1 (nios)
.wb_adr_i1(com_addr),     // WISHBONE address input
.wb_sel_i1(com_sel),     // WISHBONE byte select input
.wb_we_i1(com_we),     // WISHBONE write enable input
.wb_cyc_i1(mem_cyc),     // WISHBONE cycle input
.wb_stb_i1(mem_stb),     // WISHBONE strobe input
.wb_ack_o1(mem_ack),     // WISHBONE acknowledge output
.wb_dat_i1(com_dat_w),     // WISHBONE data input
.wb_dat_o1(mem_dat_r),     // WISHBONE data output
.wb_err_o1(mem_err),     // WISHBONE error output

// WISHBONE slave 2 (eth interface)
.wb_adr_i2(ms_addr),     // WISHBONE address input
.wb_sel_i2(ms_sel),     // WISHBONE byte select input
.wb_we_i2(ms_we),     // WISHBONE write enable input
.wb_cyc_i2(ms_cyc),     // WISHBONE cycle input
.wb_stb_i2(ms_stb),     // WISHBONE strobe input
.wb_ack_o2(ms_ack),     // WISHBONE acknowledge output
.wb_dat_i2(ms_dat_o),     // WISHBONE data input
.wb_dat_o2(ms_dat_i),     // WISHBONE data output
.wb_err_o2(ms_err)     // WISHBONE error output

);



endmodule //top module

////////// end of file //////////////////////////