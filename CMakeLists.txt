cmake_minimum_required(VERSION 3.0)
project(ds4_hid C)

#aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} src)
add_executable(${PROJECT_NAME} ds4-js.c joystick.c ${src})