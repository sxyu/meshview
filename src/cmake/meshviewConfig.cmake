include(CMakeFindDependencyMacro)

find_dependency(Threads)
find_dependency(glfw3)
find_dependency(Eigen3)
set(OpenGL_GL_PREFERENCE GLVND)
find_dependency(OpenGL)

include("${CMAKE_CURRENT_LIST_DIR}/meshviewTargets.cmake")
