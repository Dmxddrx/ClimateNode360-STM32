################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Components/UI/oled.c \
../Components/UI/oledGUI.c 

OBJS += \
./Components/UI/oled.o \
./Components/UI/oledGUI.o 

C_DEPS += \
./Components/UI/oled.d \
./Components/UI/oledGUI.d 


# Each subdirectory must supply rules for building sources it contributes
Components/UI/%.o Components/UI/%.su Components/UI/%.cyclo: ../Components/UI/%.c Components/UI/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../FATFS/Target -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/System" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Hardware" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Sensors" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/UI" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Drivers/Middlewares/OLED" -I../FATFS/App -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Components-2f-UI

clean-Components-2f-UI:
	-$(RM) ./Components/UI/oled.cyclo ./Components/UI/oled.d ./Components/UI/oled.o ./Components/UI/oled.su ./Components/UI/oledGUI.cyclo ./Components/UI/oledGUI.d ./Components/UI/oledGUI.o ./Components/UI/oledGUI.su

.PHONY: clean-Components-2f-UI

