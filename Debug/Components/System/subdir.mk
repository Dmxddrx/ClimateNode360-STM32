################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Components/System/fatfs_sd.c \
../Components/System/general.c \
../Components/System/wifi.c 

OBJS += \
./Components/System/fatfs_sd.o \
./Components/System/general.o \
./Components/System/wifi.o 

C_DEPS += \
./Components/System/fatfs_sd.d \
./Components/System/general.d \
./Components/System/wifi.d 


# Each subdirectory must supply rules for building sources it contributes
Components/System/%.o Components/System/%.su Components/System/%.cyclo: ../Components/System/%.c Components/System/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../FATFS/Target -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/System" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Hardware" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/Sensors" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Components/UI" -I"H:/Programing_Files_IoT/STM32CubeIDE/workspace_2.1.1/STM32F103C8T6/ClimateNode360/Drivers/Middlewares/OLED" -I../FATFS/App -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Components-2f-System

clean-Components-2f-System:
	-$(RM) ./Components/System/fatfs_sd.cyclo ./Components/System/fatfs_sd.d ./Components/System/fatfs_sd.o ./Components/System/fatfs_sd.su ./Components/System/general.cyclo ./Components/System/general.d ./Components/System/general.o ./Components/System/general.su ./Components/System/wifi.cyclo ./Components/System/wifi.d ./Components/System/wifi.o ./Components/System/wifi.su

.PHONY: clean-Components-2f-System

