set ISE_VER=14.4

REM If the XILINX env var is not set, or if it isn't set to the correct ISE_VER,
REM then go set it correctly before doing the rest of the batch tasks
if NOT "%XILINX%" == "C:\Xilinx\%ISE_VER%\ISE_DS\ISE" GOTO CHECK64
@ECHO The correct XILINX environment variable was detected.
GOTO DO_BATCH

:CHECK64
if NOT "%PROCESSOR_ARCHITECTURE%" == "AMD64" GOTO SET32
@ECHO The XILINX environment variable was not detected and this is a 64-bit PC.
@ECHO Setting 64-bit environment variables now...
call C:\Xilinx\%ISE_VER%\ISE_DS\settings64.bat
GOTO DO_BATCH

:SET32
@ECHO The XILINX environment variable was not detected and this is a 32-bit PC.
@ECHO Setting 32-bit environment variables now...
call C:\Xilinx\%ISE_VER%\ISE_DS\settings32.bat

:DO_BATCH
ECHO OFF
CLS
ECHO.
ECHO ...............................................................
ECHO PRESS 1, 2 OR 3 to select your JTAG cable option, or 4 to EXIT.
ECHO ...............................................................
ECHO.
ECHO 1 - On-board JTAG (Type-A USB, P1)
ECHO 2 - Digilent HS1 (back side, J6)
ECHO 3 - Xilinx Platform Cable USB/USBII (back side, J6)
ECHO 4 - EXIT
ECHO.
CHOICE /C:1234 /N /M "Type 1, 2, 3, or 4 to choose: "
::SET /P M=Type 1, 2, 3, or 4 then press ENTER:
IF ERRORLEVEL 1 SET M=1
IF ERRORLEVEL 2 SET M=2
IF ERRORLEVEL 3 SET M=3
IF ERRORLEVEL 4 SET M=4
IF %M%==1 GOTO ONBOARD
IF %M%==2 GOTO HS1
IF %M%==3 GOTO XPC
IF %M%==4 GOTO EOF

:ONBOARD
@echo setMode -bscan                                                          >  download.cmd
@echo setCable -target "digilent_plugin DEVICE=SN:000000000000 FREQUENCY=-1"  >> download.cmd
@echo identify							                                                  >> download.cmd
@echo assignfile -p 1 -file bootloop.bit                                      >> download.cmd
@echo program -p 1                                                            >> download.cmd
@echo quit								                                                    >> download.cmd
GOTO CONNECT

:HS1
@echo setMode -bscan                                                               >  download.cmd
@echo setCable -target "digilent_plugin DEVICE=SN:210205306152 FREQUENCY=30000000" >> download.cmd
@echo identify												                                             >> download.cmd
@echo assignfile -p 1 -file bootloop.bit					                                 >> download.cmd
@echo program -p 1											                                           >> download.cmd
@echo quit													                                               >> download.cmd
GOTO CONNECT

:XPC
@echo setMode -bscan                      >  download.cmd
@echo setCable -port usb21 -baud 12000000 >> download.cmd
@echo identify												    >> download.cmd
@echo assignfile -p 1 -file bootloop.bit	>> download.cmd
@echo program -p 1											  >> download.cmd
@echo quit													      >> download.cmd
GOTO CONNECT

:CONNECT
impact -batch download.cmd
@echo Waiting 3 seconds to allow PHY negotiation to complete
sleep 3
echo Loading lwIP Raw Mode Applications
@echo connect mb mdm                       >  xmd.ini
@echo rst 																 >> xmd.ini
@echo dow -data image.mfs 0xA5000000			 >> xmd.ini
@echo dow raw_apps.elf   									 >> xmd.ini
@echo run                                  >> xmd.ini
@echo exit																 >> xmd.ini
xmd

:EOF
