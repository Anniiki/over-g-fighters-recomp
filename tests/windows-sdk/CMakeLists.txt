cmake_minimum_required(VERSION 3.25)
project(overg_sdk_check LANGUAGES CXX)

find_package(rexglue 0.10.1 EXACT REQUIRED CONFIG)
add_executable(sdk_check main.cpp ../../src/language.cpp)
target_compile_features(sdk_check PRIVATE cxx_std_23)
target_link_libraries(sdk_check PRIVATE rex::runtime)
rexglue_apply_target_settings(sdk_check)
