cmake_minimum_required (VERSION 3.5.1)
project (CineFormSDK)


# Build settings
set(BUILD_TOOLS ON)
set(BUILD_TOOLS_STATIC OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
add_definitions(-D_ALLOCATOR=1 -DWARPSTUFF=1)

if (WIN32)
	include(ucm.cmake)
	ucm_set_runtime(STATIC)

	set(COMPILER_FLAGS "")
	set(COMPILER_FLAGS_W_OMP "/openmp")
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
file(GLOB PUBLIC_HEADERS "Common/CFHDAllocator.h Common/CFHDDecoder.h Common/CFHDEncoder.h Common/CFHDError.h Common/CFHDMetadata.h Common/CFHDMetadataTags.h Common/CFHDSampleHeader.h Common/CFHDTypes.h")
file(GLOB CODEC_SOURCES "Codec/*.c" "Codec/*.h" "Codec/*.cpp" "WarpLib/*.c" "WarpLib/*.h" "Common/Settings.cpp")
file(GLOB ENCODER_ALL_SOURCES ${CODEC_SOURCES} "EncoderSDK/*.cpp")
file(GLOB DECODER_ALL_SOURCES ${CODEC_SOURCES} "DecoderSDK/*.cpp" "ConvertLib/*.cpp")
file(GLOB ENCODER_SOURCES "EncoderSDK/*.cpp" "Common/*.h" "WarpLib/*.c" "WarpLib/*.h")
file(GLOB DECODER_SOURCES "DecoderSDK/*.cpp" "Common/*.h" "WarpLib/*.c" "WarpLib/*.h" "ConvertLib/*.cpp" "ConvertLib/*.h")
file(GLOB EXAMPLE_SOURCE "Example/*.cpp" "Example/*.h")
file(GLOB WAVELETDEMO_SOURCE "Example/WaveletDemo/*.c" "Example/WaveletDemo/*.h")


# Build CodecSDK library
add_library(CodecSDK STATIC ${CODEC_SOURCES})
set_target_properties(CodecSDK PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(CodecSDK PUBLIC ${COMPILER_FLAGS})

# Build CFHDEncoder and CFHDDecoder libraries (both static and dynamic)
add_library(CFHDEncoderStatic STATIC ${ENCODER_ALL_SOURCES})
add_library(CFHDDecoderStatic STATIC ${DECODER_ALL_SOURCES})
add_library(CFHDEncoderDynamic SHARED ${ENCODER_SOURCES})
add_library(CFHDDecoderDynamic SHARED ${DECODER_SOURCES})

set_target_properties(CFHDEncoderDynamic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(CFHDDecoderDynamic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(CFHDEncoderDynamic PROPERTIES OUTPUT_NAME CFHDEncoder)
set_target_properties(CFHDDecoderDynamic PROPERTIES OUTPUT_NAME CFHDDecoder)
set_target_properties(CFHDEncoderStatic PROPERTIES OUTPUT_NAME CFHDEncoder)
set_target_properties(CFHDDecoderStatic PROPERTIES OUTPUT_NAME CFHDDecoder)

target_compile_options(CFHDEncoderDynamic PUBLIC ${COMPILER_FLAGS})
target_compile_options(CFHDDecoderDynamic PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(CFHDEncoderDynamic PUBLIC -DDYNAMICLIB=1)
target_compile_definitions(CFHDDecoderDynamic PUBLIC -DDYNAMICLIB=1)

target_link_libraries(CFHDEncoderStatic)
target_link_libraries(CFHDDecoderStatic)
target_link_libraries(CFHDEncoderDynamic CodecSDK)
target_link_libraries(CFHDDecoderDynamic CodecSDK)


# Build tools
if (BUILD_TOOLS)
	# TestCFHD
	add_executable(TestCFHD ${EXAMPLE_SOURCE})
	target_compile_options(TestCFHD PRIVATE ${COMPILER_FLAGS_W_OMP})

	if (BUILD_TOOLS_STATIC)
		target_link_libraries(TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
		target_link_libraries(TestCFHD CFHDEncoderStatic CFHDDecoderStatic ${ADDITIONAL_LIBS})
	else (BUILD_TOOLS_STATIC)
		target_link_libraries(TestCFHD CFHDEncoderDynamic CFHDDecoderDynamic ${ADDITIONAL_LIBS})
		target_link_libraries(TestCFHD CFHDEncoderDynamic CFHDDecoderDynamic ${ADDITIONAL_LIBS})
	endif (BUILD_TOOLS_STATIC)

	# WaveletDemo
	add_executable(WaveletDemo ${WAVELETDEMO_SOURCE})
	target_link_libraries(WaveletDemo ${TOY_LIBS})
endif (BUILD_TOOLS)


# pkg-config integration
set(PROJECT_VERSION "10.0.2")
set(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)
set(LIB_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX} CACHE PATH "Installation prefix for object code libraries" FORCE)
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include/cineformsdk CACHE PATH "Installation prefix for header files" FORCE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libcineformsdk.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libcineformsdk.pc)


# System wide installation
if (UNIX)
	install(TARGETS CFHDEncoderStatic DESTINATION lib/)
	install(TARGETS CFHDDecoderStatic DESTINATION lib/)
	install(TARGETS CFHDEncoderDynamic DESTINATION lib/)
	install(TARGETS CFHDDecoderDynamic DESTINATION lib/)
	install(FILES libcineformsdk.pc DESTINATION lib/pkgconfig/)
	install(FILES ${PUBLIC_HEADERS} DESTINATION include/cineformsdk/)
endif (UNIX)
