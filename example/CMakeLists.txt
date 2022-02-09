cmake_minimum_required(VERSION 3.16)

# current project
project( httpclient
         VERSION      1.0
         DESCRIPTION  "http client with c"
         LANGUAGES    C
)

# select complier
set(CMAKE_C_COMPILER /usr/bin/gcc)

# set header file directory
include_directories(../src)

# add extern source file directory
add_subdirectory(../src src.out)

# set target file
add_executable(Demo get_request.c)
#add_executable(Demo post_request.c)

# add library
target_link_libraries(Demo httpc)

message (">>> project     : ${PROJECT_NAME}")
message (">>> version     : ${PROJECT_VERSION}")
message (">>> description : ${CMAKE_PROJECT_DESCRIPTION}") 
