################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/lib/CapaFS.c \
../src/lib/CapaMemoria.c \
../src/lib/accionesDeKernel.c \
../src/lib/pcb.c \
../src/lib/stack.c 

OBJS += \
./src/lib/CapaFS.o \
./src/lib/CapaMemoria.o \
./src/lib/accionesDeKernel.o \
./src/lib/pcb.o \
./src/lib/stack.o 

C_DEPS += \
./src/lib/CapaFS.d \
./src/lib/CapaMemoria.d \
./src/lib/accionesDeKernel.d \
./src/lib/pcb.d \
./src/lib/stack.d 


# Each subdirectory must supply rules for building sources it contributes
src/lib/%.o: ../src/lib/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


