################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Prueba\ Memoria.c 

OBJS += \
./src/Prueba\ Memoria.o 

C_DEPS += \
./src/Prueba\ Memoria.d 


# Each subdirectory must supply rules for building sources it contributes
src/Prueba\ Memoria.o: ../src/Prueba\ Memoria.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/Prueba Memoria.d" -MT"src/Prueba\ Memoria.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


