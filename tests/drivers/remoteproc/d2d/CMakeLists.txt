# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(d2d_firmware)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/d2d_fw.elf
                  COMMAND ${CMAKE_C_COMPILER} -ffreestanding
                       -nostdlib -Wl,--section-start=.text=0x04202000
                       ${CMAKE_CURRENT_SOURCE_DIR}/src/d2d_firmware.S
                       -o ${CMAKE_BINARY_DIR}/d2d_fw.elf
                  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/d2d_firmware.S
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/d2d_fw.bin
                  COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/d2d_fw.elf
                       ${CMAKE_BINARY_DIR}/d2d_fw.bin
                  DEPENDS ${CMAKE_BINARY_DIR}/d2d_fw.elf
)

add_custom_target(d2d_fw_bin ALL
                 DEPENDS ${CMAKE_BINARY_DIR}/d2d_fw.bin
)

set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
zephyr_include_directories(${gen_inc_dir})

generate_inc_file_for_target(
       app ${CMAKE_BINARY_DIR}/d2d_fw.bin ${gen_inc_dir}/d2d_fw_bin.inc
)

add_dependencies(app d2d_fw_bin)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
