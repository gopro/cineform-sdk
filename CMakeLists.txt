# CMakeLists.txt
cmake_minimum_required (VERSION 3.8.2)
project (CineFormSDK)

set(LINK_STATIC OFF)

if (WIN32)
	include(ucm.cmake)
	ucm_set_runtime(STATIC)
	
	SET(COMPILER_FLAGS "")
	SET(COMPILER_FLAGS_W_OMP "/openmp" )
	SET(ADDITIONAL_LIBS "")
endif (WIN32)
 
if (UNIX)
	SET(COMPILER_FLAGS "-fPIC" )
	SET(COMPILER_FLAGS_W_OMP "-fopenmp")
	SET(ADDITIONAL_LIBS "-luuid -lpthread -lgomp")
	SET(TOY_LIBS "-lm")
endif (UNIX)

if (APPLE)
	SET(COMPILER_FLAGS -fvisibility=hidden -O3)
	SET(COMPILER_FLAGS_W_OMP -O3)
	SET(ADDITIONAL_LIBS "-lpthread")
endif (APPLE)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

include_directories("Common" "Tables" "Codec" "ConvertLib" "WarpLib" "Example")
file(GLOB CODEC_SOURCES "Codec/*.c" "Codec/*.cpp" "WarpLib/*.c" "Common/Settings.cpp")
file(GLOB ENCODER_ALL_SOURCES "Codec/*.c" "Codec/*.cpp" "Common/Settings.cpp" "EncoderSDK/*.cpp")
file(GLOB DECODER_ALL_SOURCES "Codec/*.c" "Codec/*.cpp" "Common/Settings.cpp"  "ConvertLib/*.cpp" "WarpLib/*.c" "DecoderSDK/*.cpp")
file(GLOB ENCODER_SOURCES "EncoderSDK/*.cpp")
file(GLOB DECODER_SOURCES "DecoderSDK/*.cpp" "WarpLib/*.c" "ConvertLib/*.cpp" )
file(GLOB EXAMPLE_SOURCE "Example/*.cpp" )
file(GLOB WAVELETDEMO_SOURCE "Example/WaveletDemo/*.c" )

add_definitions(-D_ALLOCATOR=1 -DWARPSTUFF=1)

add_library(CodecSDK ${CODEC_SOURCES})
add_library(CFHDEncoderStatic ${ENCODER_ALL_SOURCES})
add_library(CFHDDecoderStatic ${DECODER_ALL_SOURCES})
add_library(CFHDEncoder SHARED ${ENCODER_SOURCES})
add_library(CFHDDecoder SHARED ${DECODER_SOURCES})

set_target_properties(CodecSDK PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(CodecSDK PUBLIC ${COMPILER_FLAGS})
target_compile_options(CFHDEncoder PUBLIC ${COMPILER_FLAGS})
target_compile_options(CFHDDecoder PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(CFHDEncoder PUBLIC -DDYNAMICLIB=1)
target_compile_definitions(CFHDDecoder PUBLIC -DDYNAMICLIB=1)

target_link_libraries (CFHDEncoder CodecSDK)
target_link_libraries (CFHDDecoder CodecSDK)
target_link_libraries (CFHDEncoderStatic)
target_link_libraries (CFHDDecoderStatic)

add_executable (TestCFHD ${EXAMPLE_SOURCE})
target_compile_options(TestCFHD PRIVATE ${COMPILER_FLAGS_W_OMP})

if (LINK_STATIC)
target_link_libraries (TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
target_link_libraries (TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
else (LINK_STATIC)
target_link_libraries (TestCFHD CFHDEncoder CFHDDecoder ${ADDITIONAL_LIBS})
target_link_libraries (TestCFHD CFHDEncoder CFHDDecoder ${ADDITIONAL_LIBS})
endif (LINK_STATIC)

add_executable (WaveletDemo ${WAVELETDEMO_SOURCE})
target_link_libraries (WaveletDemo ${TOY_LIBS})
