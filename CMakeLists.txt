cmake_minimum_required(VERSION 3.14) # Recommended for modern GTest integration
project(HFT_Core CXX)

# CRITICAL: Must be at least 17 for 'if constexpr' and 'is_trivially_copyable_v'
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Add include directory so both main and tests can find RingBuffer.hpp
include_directories(include)

# Add a toggle so you can choose which one to run
option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_ASAN "Enable AddressSanitizer" OFF)

if(USE_TSAN)
    # Using a list (semicolons) tells CMake these are separate flags
    set(TSAN_FLAGS "-fsanitize=thread" "-g")
    add_compile_options(${TSAN_FLAGS})
    add_link_options(${TSAN_FLAGS})
elseif(USE_ASAN)
    set(ASAN_FLAGS "-fsanitize=address" "-fno-omit-frame-pointer" "-g")
    add_compile_options(${ASAN_FLAGS})
    add_link_options(${ASAN_FLAGS})
endif()
# Main executable (if you have one)
add_executable(hft_main main.cpp)

if(NOT MSVC)
    target_compile_options(hft_main PRIVATE -Wall -Wextra -pedantic -pthread)
endif()

# Tell CMake to look into the test directory
enable_testing()
add_subdirectory(test)

