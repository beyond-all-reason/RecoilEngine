cmake_minimum_required(VERSION 3.5)
project(Glad)

if    (UNIX AND NOT MINGW AND NOT APPLE)
	add_library(glad glad.c glad_glx.c)
else  ()
	add_library(glad glad.c)
endif ()

target_include_directories(glad PUBLIC /)