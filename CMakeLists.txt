cmake_minimum_required(VERSION 3.25)
project(BENNU LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CMakeDependentOption)

########################################
# Vulkan

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    message(STATUS "Using module to find Vulkan")
    find_package(Vulkan)
endif ()

if (WIN32)
    if (NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
        if (Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        endif ()
    endif ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif (LINUX)
    if (NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
        if (Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        endif ()
    endif ()
    find_package(Threads REQUIRED)
    if (USE_D2D_WSI)
        MESSAGE("Using direct to display extension...")
        add_definitions(-D_DIRECT2DISPLAY)
    elseif (USE_DIRECTFB_WSI)
        find_package(DirectFB REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_DIRECTFB_EXT")
        include_directories(${DIRECTFB_INCLUDE_DIR})
    elseif (USE_WAYLAND_WSI)
        find_program(PKG_CONFIG pkg-config)
        if (NOT PKG_CONFIG)
            message(FATAL_ERROR "pkg-config binary not found")
        endif ()
        find_package(Wayland REQUIRED)
        if (NOT WAYLAND_FOUND)
            message(FATAL_ERROR "Wayland development package not found")
        endif ()
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        if (NOT WAYLAND_PROTOCOLS_FOUND)
            message(FATAL_ERROR "Wayland protocols package not found")
        endif ()
        find_program(WAYLAND_SCANNER wayland-scanner)
        if (NOT WAYLAND_SCANNER)
            message(FATAL_ERROR "wayland-scanner binary not found")
        endif ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
        include_directories(${WAYLAND_INCLUDE_DIR})
        pkg_get_variable(protocol_dir wayland-protocols pkgdatadir)
        execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h
                COMMAND ${WAYLAND_SCANNER} private-code ${protocol_dir}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.c)
        include_directories(${CMAKE_BINARY_DIR})
    elseif (USE_HEADLESS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_HEADLESS_EXT")
    else (USE_D2D_WSI)
        find_package(XCB REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
    endif (USE_D2D_WSI)
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK -DVK_EXAMPLE_XCODE_GENERATED")
endif ()

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else ()
    message(STATUS ${Vulkan_LIBRARY})
endif ()

########################################

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX")

########################################
# thirdparty libraries

set(BUILD_SHARED_LIBS OFF)
cmake_dependent_option(USE_HYBRID_GRAPHICS "Compile GLFW for systems with hybrid graphics" OFF
        "WIN32" OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/external)

########################################

set(BENNU_CORE_HEADERS
        src/core/engine.h
        src/core/inputmanager.h
        src/core/math/aabb.h
        )

set(BENNU_CORE_SOURCE
        src/core/engine.cpp
        src/core/inputmanager.cpp
        src/core/math/aabb.cpp
        )

set(BENNU_GRAPHICS_HEADERS
        src/graphics/renderingdevice.h
        src/graphics/vulkancontext.h
        src/graphics/swapchain.h
        src/graphics/buffer.h
        src/graphics/texture.h
        src/graphics/rendertarget.h
        src/graphics/utilities.h
        src/graphics/clusterbuilder.h
        )

set(BENNU_GRAPHICS_SOURCE
        src/graphics/renderingdevice.cpp
        src/graphics/vulkancontext.cpp
        src/graphics/swapchain.cpp
        src/graphics/buffer.cpp
        src/graphics/texture.cpp
        src/graphics/rendertarget.cpp
        src/graphics/utilities.cpp
        src/graphics/clusterbuilder.cpp
        )

set(BENNU_SCENE_HEADERS
        src/scene/scene.h
        src/scene/model.h
        src/scene/camera.h
        src/scene/light.h
        src/scene/material.h
        )

set(BENNU_SCENE_SOURCE
        src/scene/scene.cpp
        src/scene/model.cpp
        src/scene/camera.cpp
        src/scene/light.cpp
        src/scene/material.cpp
        )

add_library(bennu_lib STATIC
        ${BENNU_CORE_HEADERS}
        ${BENNU_CORE_SOURCE}
        ${BENNU_GRAPHICS_HEADERS}
        ${BENNU_GRAPHICS_SOURCE}
        ${BENNU_SCENE_HEADERS}
        ${BENNU_SCENE_SOURCE}
        )

target_include_directories(bennu_lib PUBLIC
        src
        src/external
        ${ASSIMP_INCLUDE}
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE}
        ${GLM_INCLUDE}
        ${STB_INCLUDE}
        )

target_compile_options(bennu_lib PUBLIC ${BENNU_CXX_FLAGS})

if (WIN32)
    # Avoid a name clash when building on Visual Studio
    set_target_properties(bennu_lib PROPERTIES OUTPUT_NAME libbennu)
endif ()

set(BENNU_LIBS
        bennu_lib
        assimp
        glfw
        Vulkan::Vulkan
        )

add_executable(bennu_exe src/main.cpp)
add_executable(bennu::bennu_exe ALIAS bennu_exe)

target_compile_options(bennu_exe PRIVATE ${BENNU_CXX_FLAGS})
target_include_directories(bennu_exe PRIVATE src src/external)
target_link_libraries(bennu_exe ${BENNU_LIBS})
set_target_properties(bennu_exe PROPERTIES OUTPUT_NAME bennu)

# Installation

install(TARGETS
        bennu_exe
        DESTINATION
        bin
        )

install(TARGETS
        bennu_lib
        DESTINATION
        lib
        )
