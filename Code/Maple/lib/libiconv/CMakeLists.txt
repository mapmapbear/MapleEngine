cmake_minimum_required(VERSION 3.10)

project(iconv)


file(GLOB SRC
	src/localcharset.c
	src/iconv.c
)

add_library(iconv STATIC ${SRC})

target_include_directories(iconv PUBLIC 
	include
	src
)
