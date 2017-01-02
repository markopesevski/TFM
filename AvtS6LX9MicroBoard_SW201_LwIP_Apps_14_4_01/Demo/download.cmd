setMode -bscan                      
setCable -port usb21 -baud 12000000 
identify												    
assignfile -p 1 -file bootloop.bit	
program -p 1											  
quit													      
