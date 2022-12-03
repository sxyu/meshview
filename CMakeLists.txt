cmake_minimum_required( VERSION 3.3 )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)
if (POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif (POLICY CMP0069)
if (POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif (POLICY CMP0072)

project( meshview )
set(MESHVIEW_VERSION_MAJOR 0)
set(MESHVIEW_VERSION_MINOR 0)
set(MESHVIEW_VERSION_PATCH 1)
set(MESHVIEW_VERSION ${MESHVIEW_VERSION_MAJOR}.${MESHVIEW_VERSION_MINOR}.${MESHVIEW_VERSION_PATCH})

option( MESHVIEW_USE_SYSTEM_EIGEN
    "Use system Eigen. If false, please set EIGEN_INCLUDE_DIR manually" ON )
option( MESHVIEW_USE_SYSTEM_GLFW
    "Use system glfw rather than the included glfw submodule if available" OFF )
option( MESHVIEW_BUILD_IMGUI "Build with Dear ImGui integrated GUI" ON )
option( MESHVIEW_BUILD_EXAMPLE "Build the example program" ON )
option( MESHVIEW_BUILD_INSTALL "Build the install target" ON )
option( MESHVIEW_BUILD_PYTHON "Build Python bindings" OFF )
option( MESHVIEW_USE_FFAST_MATH "Use -ffast-math" OFF )

set( CMAKE_CXX_STACK_SIZE "10000000" )
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON)
set( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

set( INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" )
set( SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )
set( VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty" )
set( STB_DIR "${VENDOR_DIR}/stb" )
set( IMGUI_DIR "${VENDOR_DIR}/imgui_mod" )
set( GLFW_DIR "${VENDOR_DIR}/glfw" )
set( GLEW_DIR "${VENDOR_DIR}/glew" )

set( PROJ_LIB_NAME "meshview" )

if ( CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations -O3 -funroll-loops -g" )
    if( MESHVIEW_USE_FFAST_MATH )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math" )
    endif()
elseif( MSVC )
    if( MESHVIEW_USE_FFAST_MATH )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast" )
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /GLT /Ox")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /GLT")
    # Silence deprecation warning coming from Eigen; silence secure warning
    add_definitions(-D_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING -D_CRT_SECURE_NO_WARNINGS)
endif ( )

# Git submodule auto update
# https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Running git submodule update --init --recursive")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
        message(STATUS "Submodule update done")
    endif()
endif()

if(NOT EXISTS "${GLFW_DIR}/CMakeLists.txt")
   message(FATAL_ERROR "A submodule as not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if ( MESHVIEW_BUILD_IMGUI )
    message(STATUS "Building with Dear ImGui integrated GUI")
    set (_MESHVIEW_IMGUI_ "")
    file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/meshview/imgui/*.cpp)
    set(IMGUI_HEADERS
            ${IMGUI_DIR}/meshview/imgui/imconfig.h
            ${IMGUI_DIR}/meshview/imgui/imgui.h
            ${IMGUI_DIR}/meshview/imgui/imgui_impl_glfw.h
            ${IMGUI_DIR}/meshview/imgui/imgui_impl_opengl3.h
            ${IMGUI_DIR}/meshview/imgui/imgui_stdlib.h
            ${IMGUI_DIR}/meshview/imgui/imstb_rectpack.h
            ${IMGUI_DIR}/meshview/imgui/imstb_textedit.h
            ${IMGUI_DIR}/meshview/imgui/imstb_truetype.h
        )
else()
    message(STATUS "Dear ImGui integrated GUI disabled")
    set (_MESHVIEW_IMGUI_ "//")
    set( IMGUI_SOURCES )
    set (IMGUI_HEADERS )
endif()

add_definitions(-DGLEW_STATIC)

file(GLOB MESHVIEW_SOURCES ${SRC_DIR}/*.cpp)
file(GLOB MESHVIEW_PUBLIC_HEADERS ${INCLUDE_DIR}/meshview/*.hpp)
file(GLOB MESHVIEW_PRIVATE_HEADERS ${INCLUDE_DIR}/meshview/internal/*.hpp)

# create meshview.hpp
set ( COMMON_HPP_PATH "${PROJECT_BINARY_DIR}/include/meshview/common.hpp" )
configure_file( common.hpp.in "${COMMON_HPP_PATH}" )
set ( MESHVIEW_PUBLIC_HEADERS ${MESHVIEW_PUBLIC_HEADERS} "${COMMON_HPP_PATH}")

set ( MESHVIEW_HEADERS ${MESHVIEW_PUBLIC_HEADERS} ${MESHVIEW_PRIVATE_HEADERS} )

set( MESHVIEW_VENDOR_SOURCES
    ${STB_DIR}/stb_image.cpp
    ${GLEW_DIR}/src/glew.c
)

source_group( "Header Files" FILES ${MESHVIEW_HEADERS} )
source_group( "Source Files" FILES ${MESHVIEW_SOURCES} )

set( DEPENDENCIES )

if ( MESHVIEW_USE_SYSTEM_EIGEN )
    find_package(Eigen3 3.3 REQUIRED NO_MODULE)
    if (TARGET Eigen3::Eigen)
        message(STATUS "Using system Eigen")
        set (DEPENDENCIES ${DEPENDENCIES} Eigen3::Eigen)
    else()
        message(STATUS "System Eigen > 3.3 not found, using vendor Eigen")
        include_directories(${EIGEN_DIR})
    endif()
else()
    # Set EIGEN_INCLUDE_DIR manually
    include_directories(${EIGEN_INCLUDE_DIR})
endif()

include_directories(
    ${STB_DIR}
    ${GLEW_DIR}/include
)

find_package(OpenGL REQUIRED)
set( DEPENDENCIES ${DEPENDENCIES} OpenGL::GL )

set ( WILL_USE_SYSTEM_GLFW ${MESHVIEW_USE_SYSTEM_GLFW} )

find_package(PkgConfig)
if ( NOT PkgConfig_FOUND )
    set ( WILL_USE_SYSTEM_GLFW OFF )
else()
    pkg_check_modules(GLFW glfw3)
    if ( NOT GLFW_FOUND )
        set ( WILL_USE_SYSTEM_GLFW OFF )
    endif ()
endif ()

if ( NOT WILL_USE_SYSTEM_GLFW )
    message ( STATUS "Using included glfw3 (in 3rdparty/)" )
    # Build GLFW
    SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW example" FORCE)
    SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW tests" FORCE)
    SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW docs" FORCE)
    SET(GLFW_INSTALL ${MESHVIEW_BUILD_INSTALL} CACHE BOOL "GLFW install" FORCE)
    add_subdirectory(${GLFW_DIR})
    set( DEPENDENCIES ${DEPENDENCIES} glfw )
    include_directories( "${GLFW_DIR}/include" )
else()
    message ( STATUS "Using system glfw3" )
    set( DEPENDENCIES ${DEPENDENCIES} glfw3 )
endif()

find_package(Threads REQUIRED)

# Optional pybind11
find_package( pybind11 )
if (NOT pybind11_FOUND AND ${MESHVIEW_BUILD_PYTHON})
    message( WARNING "pybind11 not found, Python binding is disabled. Install from https://github.com/pybind/pybind11" )
endif()

add_library( ${PROJ_LIB_NAME} STATIC ${MESHVIEW_SOURCES}
    ${IMGUI_SOURCES} ${MESHVIEW_VENDOR_SOURCES} )

target_include_directories(
    ${PROJ_LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if (MESHVIEW_BUILD_IMGUI)
    target_include_directories(
        ${PROJ_LIB_NAME} PUBLIC
        $<BUILD_INTERFACE:${IMGUI_DIR}>
    )
endif()
target_link_libraries( ${PROJ_LIB_NAME} PUBLIC
    ${DEPENDENCIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
)

if (MESHVIEW_BUILD_INSTALL)
    if ( NOT WILL_USE_SYSTEM_GLFW )
        # Ugly hack
        export(TARGETS glfw
               NAMESPACE
               FILE "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/glfwConfig.cmake")
    endif()
    set_target_properties( ${PROJ_LIB_NAME} PROPERTIES
        PUBLIC_HEADER "${MESHVIEW_PUBLIC_HEADERS}"
    )

    include(GNUInstallDirs)
    install(TARGETS ${PROJ_LIB_NAME}
            EXPORT ${PROJ_LIB_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/meshview
    )
    export(TARGETS ${PROJ_LIB_NAME}
           NAMESPACE meshview::
           FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJ_LIB_NAME}Config.cmake")
    install(EXPORT
                ${PROJ_LIB_NAME}Targets
                DESTINATION "${CMAKE_INSTALL_DATADIR}/meshview/cmake"
                NAMESPACE meshview::
            )
    install(FILES "${SRC_DIR}/cmake/meshviewConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/meshview/cmake")
    if (MESHVIEW_BUILD_IMGUI)
        install(FILES ${IMGUI_HEADERS}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/meshview/imgui")
    endif()
endif(MESHVIEW_BUILD_INSTALL)

if (MESHVIEW_BUILD_EXAMPLE)
    add_executable( example example.cpp )
    target_link_libraries( example ${PROJ_LIB_NAME} )
    set_target_properties( example PROPERTIES OUTPUT_NAME "meshview-example" )
endif()

if (${pybind11_FOUND} AND ${MESHVIEW_BUILD_PYTHON})
    message(STATUS "Building Python bindings")
    pybind11_add_module(pymeshview SHARED ${MESHVIEW_SOURCES} ${IMGUI_SOURCES} ${MESHVIEW_VENDOR_SOURCES} pybind.cpp)
    set_target_properties(pymeshview PROPERTIES OUTPUT_NAME meshview)
    target_include_directories(
        pymeshview PUBLIC
        $<BUILD_INTERFACE:${INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    if (MESHVIEW_BUILD_IMGUI)
        target_include_directories(
            pymeshview PUBLIC
            $<BUILD_INTERFACE:${IMGUI_DIR}>
        )
    endif()
    target_link_libraries(pymeshview PRIVATE ${DEPENDENCIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
endif ()

if(WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
    if (MSVC AND MESHVIEW_BUILD_EXAMPLE)
        set_property(TARGET example APPEND PROPERTY LINK_FLAGS "/DEBUG /LTCG" )
    endif ( MSVC AND MESHVIEW_BUILD_EXAMPLE )
endif()
