set SDK_EXPORT=..\xps\SDK\SDK_Export\hw
set ISE_VER=14.3

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
data2mem -bm %SDK_EXPORT%\system_bd.bmm -bt %SDK_EXPORT%\system.bit -bd %XILINX_EDK%\sw\lib\microblaze\mb_bootloop_le.elf tag microblaze_0 -o b bootloop.bit 

