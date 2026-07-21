################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Components/Hardware/btns.c \
../Components/Hardware/buzzer.c \
../Components/Hardware/led.c 

OBJS += \
./Components/Hardware/btns.o \
./Components/Hardware/buzzer.o \
./Components/Hardware/led.o 

C_DEPS += \
./Components/Hardware/btns.d \
./Components/Hardware/buzzer.d \
./Components/Hardware/led.d 


# Each subdirectory must supply rules for building sources it contributes
Components/Hardware/%.o Components/Hardware/%.su Components/Hardware/%.cyclo: ../Components/Hardware/%.c Components/Hardware/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../FATFS/Target -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/System" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Hardware" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Sensors" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/UI" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Drivers/Middlewares/OLED" -I../FATFS/App -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Components-2f-Hardware

clean-Components-2f-Hardware:
	-$(RM) ./Components/Hardware/btns.cyclo ./Components/Hardware/btns.d ./Components/Hardware/btns.o ./Components/Hardware/btns.su ./Components/Hardware/buzzer.cyclo ./Components/Hardware/buzzer.d ./Components/Hardware/buzzer.o ./Components/Hardware/buzzer.su ./Components/Hardware/led.cyclo ./Components/Hardware/led.d ./Components/Hardware/led.o ./Components/Hardware/led.su

.PHONY: clean-Components-2f-Hardware

