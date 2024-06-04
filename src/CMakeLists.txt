cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)


project(Ambient_Noise_Tracker)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(tracker
	main.c
	functs.c
	gps.c
	microphone.c
	liquid_crystal_i2c.c
)

target_include_directories(tracker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(tracker 
	pico_stdlib
	pico_flash
	hardware_timer
	pico_cyw43_arch_none 
	hardware_gpio 
	hardware_pwm 
	hardware_irq 
	hardware_sync
	hardware_adc
	hardware_uart
	hardware_dma
	hardware_sleep
	hardware_clocks
	hardware_pwm
	hardware_xosc
	hardware_rosc
	hardware_i2c
	)

pico_enable_stdio_uart(tracker 0)
pico_enable_stdio_usb(tracker 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(tracker)

# Memory usage
SET(GCC_EXE_LINKER_FLAGS    "-Wl,--print-memory-usage")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_EXE_LINKER_FLAGS}")
