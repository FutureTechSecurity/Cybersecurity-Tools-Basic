cmake_minimum_required(VERSION 3.16)
project(SecureAudioStego VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenSSL REQUIRED)

add_executable(SecureAudioStego main.cpp)
target_link_libraries(SecureAudioStego PRIVATE Qt6::Widgets OpenSSL::Crypto)
