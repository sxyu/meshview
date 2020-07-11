cmake_minimum_required( VERSION 3.11 )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)
if (POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif (POLICY CMP0072)


project( meshview )
option( MESHVIEW_USE_SYSTEM_EIGEN "Use system Eigen rather than the included Eigen submodule if available" ON )
option( MESHVIEW_BUILD_IMGUI "Build with Dear ImGui integrated GUI" ON )
option( MESHVIEW_BUILD_EXAMPLE "Build the example program" ON )

set( CMAKE_CXX_STACK_SIZE "10000000" )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON)
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules" )

set( INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include" )
set( VENDOR_DIR "${PROJECT_SOURCE_DIR}/3rdparty" )
set( STB_DIR "${VENDOR_DIR}/stb" )
set( IMGUI_DIR "${VENDOR_DIR}/imgui_mod" )
set( GLFW_DIR "${VENDOR_DIR}/glfw" )
set( GLEW_DIR "${VENDOR_DIR}/glew" )

set( PROJ_LIB_NAME "meshview" )

if ( CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations -O3 -funroll-loops -g" )
    if( ${SMPLX_USE_FFAST_MATH} )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math" )
    endif()
elseif( MSVC )
    if( ${SMPLX_USE_FFAST_MATH} )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast" )
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /GLT /Ox")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /GLT")
    if (SMPLX_USE_CUDA)
        set( CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} -Xcompiler=\"/MT\"" )
    endif()
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
        message(STATUS "Running update --init --recursive")
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

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HEADERS ${INCLUDE_DIR}/meshview/*.cpp)

set( MESHVIEW_VENDOR_SOURCES
    ${STB_DIR}/stb_image.cpp
    ${GLEW_DIR}/src/glew.c
)

source_group( "Header Files" FILES ${HEADERS} ${MESHVIEW_HEADERS} )
source_group( "Source Files" FILES ${SOURCES} ${MESHVIEW_SOURCES} )

set( DEPENDENCIES )

if ( MESHVIEW_USE_SYSTEM_EIGEN )
    find_package(Eigen3 3.3 QUIET NO_MODULE)
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

if ( MESHVIEW_BUILD_IMGUI )
    message(STATUS "Building with Dear ImGui integrated GUI")
    add_definitions( -DMESHVIEW_IMGUI )
    file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp)
else()
    set( IMGUI_SOURCES )
endif()


include_directories(
    ${INCLUDE_DIR}
    ${STB_DIR}
    ${IMGUI_DIR}
    ${GLEW_DIR}/include
    ${GLFW_DIR}/include
    ${PROJECT_BINARY_DIR}/include
)

# Build GLFW
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW example" FORCE)
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW tests" FORCE)
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW docs" FORCE)
SET(GLFW_INSTALL OFF CACHE BOOL "GLFW install" FORCE)
add_subdirectory(${GLFW_DIR})

find_package(OpenGL REQUIRED)
set( DEPENDENCIES ${DEPENDENCIES} OpenGL::GL glfw)

find_package(Threads REQUIRED)

add_library( ${PROJ_LIB_NAME} STATIC ${SOURCES} ${IMGUI_SOURCES} ${MESHVIEW_VENDOR_SOURCES} )
target_link_libraries( ${PROJ_LIB_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    ${DEPENDENCIES}
)
install(TARGETS ${MESHVIEW_NAME} DESTINATION lib)

if (MESHVIEW_BUILD_EXAMPLE)
    add_executable( example example.cpp )
    target_link_libraries( example ${PROJ_LIB_NAME} )
    set_target_properties( example PROPERTIES OUTPUT_NAME "meshview" )
    install(TARGETS example DESTINATION bin)
endif()

if(WIN32)
    add_definitions(-DNOMINMAX)
    if (MSVC)
        set_property(TARGET example APPEND PROPERTY LINK_FLAGS "/DEBUG /LTCG" )
    endif ( MSVC )
endif()
