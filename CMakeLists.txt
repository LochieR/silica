cmake_minimum_required(VERSION 4.1.0)

project(silica VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(vendor/nvrhi)
add_subdirectory(vendor/glfw)

file(GLOB_RECURSE HEADERS "silica/src/*.h")
file(GLOB_RECURSE CPPSOURCES "silica/src/*.cpp")

find_package(Vulkan REQUIRED)

add_executable(silica)
target_sources(silica PRIVATE ${HEADERS} ${CPPSOURCES})

target_link_libraries(silica PRIVATE Vulkan::Vulkan nvrhi nvrhi_vk nvrhi_d3d11 nvrhi_d3d12 glfw)
target_include_directories(silica PRIVATE
    silica/src
    vendor/nvrhi/include
    vendor/nvrhi/thirdparty/Vulkan-Headers/include
    vendor/nvrhi/thirdparty/DirectX-Headers/include
    vendor/glfw/include
)

target_compile_definitions(silica PRIVATE
    $<$<PLATFORM_ID:Windows>:SIL_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:SIL_PLATFORM_MAC>
    $<$<PLATFORM_ID:Linux>:SIL_PLATFORM_LINUX>

    $<$<CONFIG:Debug>:SIL_DEBUG>
    $<$<CONFIG:Release>:SIL_RELEASE>

    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)
