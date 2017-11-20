cmake_minimum_required (VERSION 3.5.1)
project (CineFormSDK)


# Build settings
set(BUILD_CODECSDK ON)
set(BUILD_TOOLS ON)
set(BUILD_TOOLS_STATIC OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
add_definitions(-D_ALLOCATOR=1 -DWARPSTUFF=1)

if (WIN32)
	include(ucm.cmake)
	ucm_set_runtime(STATIC)

	set(COMPILER_FLAGS "")
	set(COMPILER_FLAGS_W_OMP "/openmp" )
	set(ADDITIONAL_LIBS "")
endif (WIN32)

if (UNIX)
	set(COMPILER_FLAGS -fPIC -O3)
	set(COMPILER_FLAGS_W_OMP -fopenmp -O3)
	set(ADDITIONAL_LIBS "-luuid -lpthread -lgomp")
	set(TOY_LIBS "-lm")
endif (UNIX)

if (APPLE)
	set(COMPILER_FLAGS -fvisibility=hidden -O3)
	set(COMPILER_FLAGS_W_OMP -O3)
	set(ADDITIONAL_LIBS "-lpthread")
endif (APPLE)


# Source files
include_directories("Common" "Tables" "Codec" "ConvertLib" "WarpLib" "Example")
file(GLOB PUBLIC_HEADERS "Common/CFHDAllocator.h Common/CFHDDecoder.h Common/CFHDEncoder.h  Common/CFHDError.h Common/CFHDMetadata.h Common/CFHDMetadataTags.h Common/CFHDSampleHeader.h Common/CFHDTypes.h")
file(GLOB CODEC_SOURCES "Codec/*.c" "Codec/*.cpp" "WarpLib/*.c" "Common/Settings.cpp")
file(GLOB ENCODER_ALL_SOURCES "Codec/*.c" "Codec/*.cpp" "Common/Settings.cpp" "EncoderSDK/*.cpp")
file(GLOB DECODER_ALL_SOURCES "Codec/*.c" "Codec/*.cpp" "Common/Settings.cpp"  "ConvertLib/*.cpp" "WarpLib/*.c" "DecoderSDK/*.cpp")
file(GLOB ENCODER_SOURCES "EncoderSDK/*.cpp")
file(GLOB DECODER_SOURCES "DecoderSDK/*.cpp" "WarpLib/*.c" "ConvertLib/*.cpp")
file(GLOB EXAMPLE_SOURCE "Example/*.cpp")
file(GLOB WAVELETDEMO_SOURCE "Example/WaveletDemo/*.c")


# Build CFHDEncoder and CFHDDecoder libraries
add_library(CFHDEncoderStatic ${ENCODER_ALL_SOURCES})
add_library(CFHDDecoderStatic ${DECODER_ALL_SOURCES})
add_library(CFHDEncoder SHARED ${ENCODER_SOURCES})
add_library(CFHDDecoder SHARED ${DECODER_SOURCES})

set_target_properties(CFHDEncoder PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(CFHDDecoder PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(CFHDEncoder PUBLIC ${COMPILER_FLAGS})
target_compile_options(CFHDDecoder PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(CFHDEncoder PUBLIC -DDYNAMICLIB=1)
target_compile_definitions(CFHDDecoder PUBLIC -DDYNAMICLIB=1)

target_link_libraries(CFHDEncoderStatic)
target_link_libraries(CFHDDecoderStatic)


# Build CodecSDK library
if (BUILD_CODECSDK)
	add_library(CodecSDK ${CODEC_SOURCES})
	set_target_properties(CodecSDK PROPERTIES POSITION_INDEPENDENT_CODE ON)
	target_compile_options(CodecSDK PUBLIC ${COMPILER_FLAGS})
	target_link_libraries (CFHDEncoder CodecSDK)
	target_link_libraries (CFHDDecoder CodecSDK)
endif (BUILD_CODECSDK)


# Build tools
if (BUILD_TOOLS)
	# TestCFHD
	add_executable(TestCFHD ${EXAMPLE_SOURCE})
	target_compile_options(TestCFHD PRIVATE ${COMPILER_FLAGS_W_OMP})

	if (BUILD_TOOLS_STATIC)
		target_link_libraries(TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
		target_link_libraries(TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
	else (BUILD_TOOLS_STATIC)
		target_link_libraries(TestCFHD CFHDEncoder CFHDDecoder ${ADDITIONAL_LIBS})
		target_link_libraries(TestCFHD CFHDEncoder CFHDDecoder ${ADDITIONAL_LIBS})
	endif (BUILD_TOOLS_STATIC)

	# WaveletDemo
	add_executable(WaveletDemo ${WAVELETDEMO_SOURCE})
	target_link_libraries(WaveletDemo ${TOY_LIBS})
endif (BUILD_TOOLS)


# Pkgconfig integration
set(PROJECT_VERSION "10.0.2")
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH  "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/cineformsdk CACHE PATH "Installation prefix for header files" FORCE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libcineformsdk.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libcineformsdk.pc)


# System wide installation
if (UNIX)
	install(TARGETS CFHDDecoder DESTINATION lib/)
	install(TARGETS CFHDEncoder DESTINATION lib/)
	install(FILES libcineformsdk.pc DESTINATION lib/pkgconfig/)
	install(FILES ${PUBLIC_HEADERS} DESTINATION include/cineformsdk/)
endif (UNIX)
