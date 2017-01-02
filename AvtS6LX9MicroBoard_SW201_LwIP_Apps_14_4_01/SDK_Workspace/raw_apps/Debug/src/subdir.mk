################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dispatch.c \
../src/main.c \
../src/platform.c \
../src/prot_malloc.c \
../src/rxperf.c \
../src/txperf.c \
../src/urxperf.c \
../src/utxperf.c 

LD_SRCS += \
../src/lscript.ld 

OBJS += \
./src/dispatch.o \
./src/main.o \
./src/platform.o \
./src/prot_malloc.o \
./src/rxperf.o \
./src/txperf.o \
./src/urxperf.o \
./src/utxperf.o 

C_DEPS += \
./src/dispatch.d \
./src/main.d \
./src/platform.d \
./src/prot_malloc.d \
./src/rxperf.d \
./src/txperf.d \
./src/urxperf.d \
./src/utxperf.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -c -fmessage-length=0 -I../../standalone_bsp_0/microblaze_0/include -mlittle-endian -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mcpu=v8.40.b -mno-xl-soft-mul -Wl,--no-relax -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


