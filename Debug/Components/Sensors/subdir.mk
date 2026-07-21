################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Components/Sensors/dust.c \
../Components/Sensors/sd.c \
../Components/Sensors/sht30.c 

OBJS += \
./Components/Sensors/dust.o \
./Components/Sensors/sd.o \
./Components/Sensors/sht30.o 

C_DEPS += \
./Components/Sensors/dust.d \
./Components/Sensors/sd.d \
./Components/Sensors/sht30.d 


# Each subdirectory must supply rules for building sources it contributes
Components/Sensors/%.o Components/Sensors/%.su Components/Sensors/%.cyclo: ../Components/Sensors/%.c Components/Sensors/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../FATFS/Target -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/System" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Hardware" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Sensors" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/UI" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Drivers/Middlewares/OLED" -I../FATFS/App -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Components-2f-Sensors

clean-Components-2f-Sensors:
	-$(RM) ./Components/Sensors/dust.cyclo ./Components/Sensors/dust.d ./Components/Sensors/dust.o ./Components/Sensors/dust.su ./Components/Sensors/sd.cyclo ./Components/Sensors/sd.d ./Components/Sensors/sd.o ./Components/Sensors/sd.su ./Components/Sensors/sht30.cyclo ./Components/Sensors/sht30.d ./Components/Sensors/sht30.o ./Components/Sensors/sht30.su

.PHONY: clean-Components-2f-Sensors

