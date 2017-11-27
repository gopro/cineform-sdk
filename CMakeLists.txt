cmake_minimum_required (VERSION 3.5.1)
project (CineFormSDK C CXX)


# Build settings
option(BUILD_STATIC_LIBS "Build static library" ON)
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_TOOLS "Build tools" ON)
set(BUILD_TOOLS_STATIC OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
add_definitions(-D_ALLOCATOR=1 -DWARPSTUFF=1)

if (WIN32)
    include(ucm.cmake)
    ucm_set_runtime(STATIC)

    set(COMPILER_FLAGS "")
    set(COMPILER_FLAGS_W_OMP "/openmp")
endif (WIN32)

if (UNIX)
    set(COMPILER_FLAGS -O3)
    set(COMPILER_FLAGS_W_OMP -fopenmp -O3)
    set(ADDITIONAL_LIBS "-lpthread -luuid -lm -lgomp")
endif (UNIX)

if (APPLE)
    set(COMPILER_FLAGS -fvisibility=hidden -O3)
    set(COMPILER_FLAGS_W_OMP -O3)
    set(ADDITIONAL_LIBS "-lpthread -lm")
endif (APPLE)


# Source files
include_directories("Common" "Tables" "Codec" "ConvertLib" "WarpLib" "Example")
file(GLOB PUBLIC_HEADERS "Common/CFHD*.h")
file(GLOB CODEC_SOURCES "Codec/*.c" "Codec/*.h" "Codec/*.cpp" "WarpLib/*.c" "WarpLib/*.h" "Common/Settings.cpp")
file(GLOB ENCODER_SOURCES "EncoderSDK/*.cpp" "Common/*.h" "WarpLib/*.c" "WarpLib/*.h")
file(GLOB DECODER_SOURCES "DecoderSDK/*.cpp" "Common/*.h" "WarpLib/*.c" "WarpLib/*.h" "ConvertLib/*.cpp" "ConvertLib/*.h")

file(GLOB EXAMPLE_SOURCE "Example/*.cpp" "Example/*.h")
file(GLOB WAVELETDEMO_SOURCE "Example/WaveletDemo/*.c" "Example/WaveletDemo/*.h")


# Build CFHDCodec library (static and shared rules)
if (BUILD_STATIC_LIBS)
    add_library(CFHDCodecStatic STATIC ${CODEC_SOURCES} ${ENCODER_SOURCES} ${DECODER_SOURCES})
    target_compile_options(CFHDCodecStatic PUBLIC ${COMPILER_FLAGS})
    set_target_properties(CFHDCodecStatic PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if (UNIX)
        set_target_properties(CFHDCodecStatic PROPERTIES OUTPUT_NAME CFHDCodec)
    endif (UNIX)
    target_link_libraries(CFHDCodecStatic)
endif (BUILD_STATIC_LIBS)

if (BUILD_SHARED_LIBS)
    add_library(CFHDCodecShared SHARED ${CODEC_SOURCES} ${ENCODER_SOURCES} ${DECODER_SOURCES})
    set_target_properties(CFHDCodecShared PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(CFHDCodecShared PROPERTIES OUTPUT_NAME CFHDCodec)
    target_compile_options(CFHDCodecShared PUBLIC ${COMPILER_FLAGS})
    target_compile_definitions(CFHDCodecShared PUBLIC -DDYNAMICLIB=1)
    target_link_libraries(CFHDCodecShared)
endif (BUILD_SHARED_LIBS)

# Build tools
if (BUILD_TOOLS)
    # TestCFHD
    add_executable(TestCFHD ${EXAMPLE_SOURCE})
    target_compile_options(TestCFHD PRIVATE ${COMPILER_FLAGS_W_OMP})

    if (BUILD_TOOLS_STATIC)
        target_link_libraries(TestCFHD CFHDCodecStatic ${ADDITIONAL_LIBS})
    else (BUILD_TOOLS_STATIC)
        target_link_libraries(TestCFHD CFHDCodecShared ${ADDITIONAL_LIBS})
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
if (BUILD_STATIC_LIBS)
    install(TARGETS CFHDCodecStatic DESTINATION lib/)
endif (BUILD_STATIC_LIBS)
if (BUILD_SHARED_LIBS)
    install(TARGETS CFHDCodecShared DESTINATION lib/)
endif (BUILD_SHARED_LIBS)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libcineformsdk.pc DESTINATION lib/pkgconfig/)
install(FILES ${PUBLIC_HEADERS} DESTINATION include/cineformsdk/)
