cmake_minimum_required(VERSION 3.0)
project(ds4_sbc C)

link_libraries("asound" "sbc")
set(audio_src ds4-sbc.c ds4-report.c)
set(js_src ds4-js.c)
add_executable(${PROJECT_NAME} ${audio_src})
