cmake_minimum_required(VERSION 3.16)
project(fork_httpd)

set(CMAKE_CXX_STANDARD 11)

add_executable(fork_httpd fork_httpd.cpp)