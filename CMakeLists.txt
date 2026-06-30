cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(safeguard_node)

target_sources(app PRIVATE
  src/main.c
  src/bus/zbus_channels.c
  src/logic/proc.c
  src/actuators/led.c
  src/actuators/buzzer.c
  src/sensor/adxl345.c
  src/shell/shell_cmd.c
  src/settings/settings_nvs.c
)

target_include_directories(app PRIVATE src)
