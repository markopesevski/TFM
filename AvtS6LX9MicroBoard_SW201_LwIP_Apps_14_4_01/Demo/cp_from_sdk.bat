set SDK_PATH=..\SDK_Workspace

copy %SDK_PATH%\xps_hw_platform\download.bit .\bootloop.bit

copy /Y %SDK_PATH%\raw_apps\Debug\raw_apps.elf .

