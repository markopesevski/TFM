 /*
	memory_wb_slave.v
 
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
	

/*
Wishbone Memory Module:
Manages SRAM memory inteface from two Wishbone slave ports
*/
module memory_wb_slave(

//////////// SRAM //////////
output		    [19:0]		SRAM_ADDR,
output		          		SRAM_CE_N,
inout		    [15:0]		SRAM_DQ,
output		          		SRAM_LB_N,
output		          		SRAM_OE_N,
output		          		SRAM_UB_N,
output		          		SRAM_WE_N,

// WISHBONE common
input           wb_clk_i,     // WISHBONE clock
input           wb_rst_i,     // WISHBONE reset

// WISHBONE slave 1
input   [31:0]  wb_adr_i1,     // WISHBONE address input
input    [3:0]  wb_sel_i1,     // WISHBONE byte select input
input           wb_we_i1,     // WISHBONE write enable input
input           wb_cyc_i1,     // WISHBONE cycle input
input           wb_stb_i1,     // WISHBONE strobe input
output          wb_ack_o1,     // WISHBONE acknowledge output
input   [31:0]  wb_dat_i1,     // WISHBONE data input
output  [31:0]  wb_dat_o1,     // WISHBONE data output
output          wb_err_o1,     // WISHBONE error output

// WISHBONE slave 2
input   [31:0]  wb_adr_i2,     // WISHBONE address input
input    [3:0]  wb_sel_i2,     // WISHBONE byte select input
input           wb_we_i2,     // WISHBONE write enable input
input           wb_cyc_i2,     // WISHBONE cycle input
input           wb_stb_i2,     // WISHBONE strobe input
output          wb_ack_o2,     // WISHBONE acknowledge output
input   [31:0]  wb_dat_i2,     // WISHBONE data input
output  [31:0]  wb_dat_o2,     // WISHBONE data output
output          wb_err_o2     // WISHBONE error output

);
wire clk = wb_clk_i;
wire reset = wb_rst_i;

assign SRAM_UB_N = 0;					// hi byte select enabled
assign SRAM_LB_N = 0;					// lo byte select enabled
assign SRAM_CE_N = 0;					// chip is enabled
assign SRAM_OE_N = 0;
assign SRAM_WE_N = ~we;
assign SRAM_DQ = (we)? data_reg : 16'hzzzz;
assign SRAM_ADDR = addr_reg;

assign wb_err_o1 =0;
assign wb_err_o2 =0;


reg switch_ack;
assign wb_ack_o1 = (portactive==1)?switch_ack : 0;
assign wb_ack_o2 = (portactive==2)?switch_ack : 0;




reg [19:0] addr_reg; //memory address register for SRAM
reg [15:0] data_reg; //memory data register  for SRAM
reg we;
reg [3:0] state;
reg [15:0] loresult;

wire port1action = (wb_cyc_i1 & wb_stb_i1);
wire port2action = (wb_cyc_i2 & wb_stb_i2);

wire [2:0] portactive = port1action? 1 : port2action? 2 : 0;

wire [31:0] in_addr = (portactive==1)?wb_adr_i1 : (portactive==2)? wb_adr_i2: 0;
wire [31:0] in_data = (portactive==1)?wb_dat_i1 : (portactive==2)? wb_dat_i2: 0;
reg [31:0] out_data;
assign wb_dat_o1 = (portactive==1)?out_data : 0;
assign wb_dat_o2 = (portactive==2)?out_data : 0;

wire dowrite = (portactive==1)?wb_we_i1 : (portactive==2)? wb_we_i2: 0;

wire [3:0] byte_sel = (portactive==1)?wb_sel_i1 : (portactive==2)? wb_sel_i2: 0;
//eth interface needs byte select for reading?

//dual interface - give priority to slave bus 1.


//need to emulate a 32 bit word-addressed memory.

//do reads and writes in 16-bit chunks.

//maximum address is 20 bits!!!

//store/read first 16 bits to {address[20:2],0}, second 16 to {address[20:2],1}




always @ (posedge clk)
begin
	if (reset)
	begin
		state<=1;
		we<=0;
		switch_ack<=0;
	end
	
	if (state==1)
	begin
		if ((portactive==1) | (portactive==2))
		begin
			we<=0;
			//port 1 lower 16 bits
			addr_reg<={in_addr[20:2],1'b0};
			data_reg<=in_data[15:0];
			state<=2;//do next bits
		end
		//otherwise, no operation; do nothing
	end
	else if (state==2)
	begin
		if (dowrite)
		begin
			we<=1;
		end
		else
		begin
			loresult<=SRAM_DQ;
			//do a read
			we<=0;
		end
		state<=3;//do one cycle ack
	end
	else if (state==3)
	begin
		we<=we;
		addr_reg<={in_addr[20:2],1'b1};
		data_reg<=in_data[31:16];
		state<=4;//do one cycle ack
	end
	else if (state==4)
	begin
	
		if (~dowrite)
		begin
			out_data<={SRAM_DQ,loresult};
		end
	
		we<=0;
		
		switch_ack<=1;
		state<=5;
	end
		else if (state==5)//reset ack
	begin
			//ack was set for one cycle, now set it to zero
		//reset both ack's:
		switch_ack<=0;
		we<=0;
		state<=1;
	end else
	begin
	state<=1;
	end

end





/*
reg [31:0] datalatch1, datalatch2;
always @ (posedge clk)
begin
datalatch1 <= wb_dat_i1;
datalatch2 <= wb_dat_i2;

end
*/
/*
always @ (posedge clk)
begin
	if (reset)
	begin
		state<=1;
		we<=0;
		ack1<=0;
		ack2<=0;
	end
	
	if (state==1)
	begin
		if (wb_cyc_i1 & wb_stb_i1)
		begin
			//port 1 lower 16 bits
			addr_reg<={wb_adr_i1[20:2],1'b0};
			if (wb_we_i1)
			begin
				we<=1;
				data_reg<=wb_dat_i1[15:0];
			end
			else
			begin
				//do a read
				we<=0;
			end
			state<=2;//do next bits
		end
		else if (wb_cyc_i2 & wb_stb_i2)
		begin
			//port 2 lower 16 bits
			addr_reg<={wb_adr_i2[20:2],1'b0};
			if (wb_we_i2)
			begin
				we<=1;
				data_reg<=wb_dat_i2[15:0];
			end
			else
			begin
				//do a read
				we<=0;
			end
			state<=3;// do next bits
		end
		//otherwise, no operation; do nothing
	end
	else if (state==2)//port 1 upper 16 bits
	begin
	addr_reg<={wb_adr_i1[20:2],1'b1};
		if (wb_we_i1)
		begin
			we<=1;
			data_reg<=wb_dat_i1[31:16];
		end
		else
		begin
			loresult<=SRAM_DQ;
			//do a read
			we<=0;
		end
		state<=5;//do one cycle ack
	end
	else if (state==3)//port 2 upper 16 bits
	begin
	addr_reg<={wb_adr_i2[20:2],1'b1};
		if (wb_we_i2)
		begin
			we<=1;
			data_reg<=wb_dat_i2[31:16];
		end
		else
		begin
			loresult<=SRAM_DQ;
			//do a read
			we<=0;
		end
		state<=6;//do one cycle ack
	end
	else if (state==4)
	begin
		//ack was set for one cycle, now set it to zero
		//reset both ack's:
		ack1<=0;
		ack2<=0;
		state<=1;
		we<=0;
	end
		else if (state==5)//port 1 ack and result read
	begin
		we<=0;
		wb_dat_o1<={SRAM_DQ,loresult};
		ack1<=1;
		state<=4;
	end
		else if (state==6)//port 2 ack and result read
	begin
		we<=0;
		wb_dat_o2<={SRAM_DQ,loresult};
		ack2<=1;
		state<=4;
	end

end

*/

endmodule
