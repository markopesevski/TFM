
 PARAMETER VERSION = 2.2.0


BEGIN OS
 PARAMETER OS_NAME = standalone
 PARAMETER OS_VER = 3.11.a
 PARAMETER PROC_INSTANCE = microblaze_0
 PARAMETER STDIN = usb_uart
 PARAMETER STDOUT = usb_uart
 PARAMETER MICROBLAZE_EXCEPTIONS = true
 PARAMETER PREDECODE_FPU_EXCEPTIONS = true
END


BEGIN PROCESSOR
 PARAMETER DRIVER_NAME = cpu
 PARAMETER DRIVER_VER = 1.15.a
 PARAMETER HW_INSTANCE = microblaze_0
END


BEGIN DRIVER
 PARAMETER DRIVER_NAME = tmrctr
 PARAMETER DRIVER_VER = 2.05.a
 PARAMETER HW_INSTANCE = axi_timer_0
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 2.01.a
 PARAMETER HW_INSTANCE = debug_module
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = dip_switches_4bits
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = emaclite
 PARAMETER DRIVER_VER = 3.04.a
 PARAMETER HW_INSTANCE = ethernet_mac
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = gpio
 PARAMETER DRIVER_VER = 3.01.a
 PARAMETER HW_INSTANCE = leds_4bits
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = s6_ddrx
 PARAMETER DRIVER_VER = 1.00.a
 PARAMETER HW_INSTANCE = mcb3_lpddr
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.03.a
 PARAMETER HW_INSTANCE = microblaze_0_d_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = bram
 PARAMETER DRIVER_VER = 3.03.a
 PARAMETER HW_INSTANCE = microblaze_0_i_bram_ctrl
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = intc
 PARAMETER DRIVER_VER = 2.06.a
 PARAMETER HW_INSTANCE = microblaze_0_intc
END

BEGIN DRIVER
 PARAMETER DRIVER_NAME = uartlite
 PARAMETER DRIVER_VER = 2.01.a
 PARAMETER HW_INSTANCE = usb_uart
END


BEGIN LIBRARY
 PARAMETER LIBRARY_NAME = lwip140
 PARAMETER LIBRARY_VER = 1.06.a
 PARAMETER PROC_INSTANCE = microblaze_0
 PARAMETER N_TX_DESCRIPTORS = 256
 PARAMETER N_RX_DESCRIPTORS = 256
 PARAMETER PHY_LINK_SPEED = CONFIG_LINKSPEED100
 PARAMETER MEMP_N_PBUF = 1024
 PARAMETER MEMP_N_TCP_SEG = 1024
 PARAMETER PBUF_POOL_SIZE = 1024
 PARAMETER TCP_WND = 4096
 PARAMETER TCP_MSS = 1450
 PARAMETER LWIP_DHCP = true
END

