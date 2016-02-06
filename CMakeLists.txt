cmake_minimum_required(VERSION 3.2)
project(os_hw)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

add_executable(tsh tsh/tsh.c tsh/errmsg.c tsh/job.c tsh/sigutil.c)
