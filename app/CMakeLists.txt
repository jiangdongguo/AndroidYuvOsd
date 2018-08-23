# 设置编译native library需要最小的cmake版本
cmake_minimum_required(VERSION 3.4.1)

# 设置so动态库最后的输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI})

# 将指定的源文件编译为YuvOsd.so动态库
add_library(YuvOsd
            SHARED
            src/main/cpp/YuvOsd.cpp
            src/main/cpp/yuv/yuv.c)

find_library(log-lib log )
target_link_libraries(YuvOsd ${log-lib} )
