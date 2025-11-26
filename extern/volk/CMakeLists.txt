cmake_minimum_required(VERSION 3.15)
project(volk VERSION 313 LANGUAGES C)

# Volk library
add_library(volk STATIC volk.h volk.c)
add_library(volk::volk ALIAS volk)
target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(volk PUBLIC ".")

# Vulkan
# find_package(Vulkan REQUIRED)
target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_compile_definitions(volk PRIVATE VK_NO_PROTOTYPES)
# if(TARGET Vulkan::Vulkan)
  # Note: We don't use target_link_libraries for Vulkan::Vulkan to avoid a static dependency on libvulkan1
  # target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
# elseif(DEFINED ENV{VULKAN_SDK})
  #target_include_directories(volk PUBLIC "$ENV{VULKAN_SDK}/include")
# endif()
if(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
    message(STATUS "Using VULKAN_SDK: $ENV{VULKAN_SDK}")
    target_include_directories(volk PUBLIC "$ENV{VULKAN_SDK}/include")
elseif(DEFINED ENV{VK_SDK_PATH} AND NOT "$ENV{VK_SDK_PATH}" STREQUAL "")
    message(STATUS "Using VK_SDK_PATH: $ENV{VK_SDK_PATH}")
    target_include_directories(volk PUBLIC "$ENV{VK_SDK_PATH}/include")
else()
    message(FATAL_ERROR "No Vulkan SDK found! Set VULKAN_SDK or VK_SDK_PATH.")
endif()
	