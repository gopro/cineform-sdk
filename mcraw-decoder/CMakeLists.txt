cmake_minimum_required(VERSION 3.1...3.29)

project(
	ExampleDecoder
	VERSION 1.0
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O3 -march=armv8-a -mcpu=apple-m1 -funroll-loops -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=armv8-a -mcpu=apple-m1 -funroll-loops -flto")

include_directories(motioncam_decoder lib/include thirdparty)

# Add CineForm SDK include paths
include_directories(../Common ../Codec ../ConvertLib ../WarpLib ../Tables ../third_party/simde)

# Link against parent directory's CineForm library
link_directories(../build)

add_library(motioncam_decoder lib/Decoder.cpp lib/RawData.cpp lib/RawData_Legacy.cpp)
set_property(TARGET motioncam_decoder PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(example example.cpp)

# Link against CineForm codec library and motioncam decoder
target_link_libraries(example PRIVATE motioncam_decoder CFHDCodec)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Werror -O3)
endif()
