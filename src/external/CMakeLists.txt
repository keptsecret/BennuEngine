cmake_minimum_required(VERSION 3.25)

###########################################################################
# assimp

set (ASSIMP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include ${CMAKE_CURRENT_BINARY_DIR}/assimp/include PARENT_SCOPE)

set (BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set (ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set (ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)

add_subdirectory (assimp)

set_property (TARGET assimp PROPERTY FOLDER "external")

###########################################################################
# glfw

set (GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)

set (GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# in the case of hybrid graphics on Windows
if (USE_HYBRID_GRAPHICS)
    message (STATUS "Setting GLFW to compile for hybrid graphics")
    set (GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
endif ()

add_subdirectory (glfw)
set_property (TARGET glfw PROPERTY FOLDER "external")

###########################################################################
# glm

set (GLM_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)

###########################################################################
# stb

set (STB_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/stb PARENT_SCOPE)