# CMakeList.txt : CMake project for OpenLightShadow, include source and define
# project specific logic here.
#

set(CMAKE_CXX_STANDARD 14)

file(GLOB f_dir
    "../../common/*.h"
    "../../common/*.cpp"
)

# glew
include_directories(../libs/GLEW/include)
link_directories(../libs/GLEW/lib/Release/x64)

# glfw
include_directories(../libs/GLFW/include)     
link_directories(../libs/GLFW/lib-static-ucrt )

#glm
include_directories(../libs/GLM)     
link_directories(../libs/GLM/build/glm/Debug)

include_directories(../libs/stb)

include_directories(../libs/tinoy_obj_loader)
add_subdirectory(imgui)

include_directories(${PROJECT_SOURCE_DIR})
add_executable (OpenLightShadow "Application.cpp" "Application.h" ${f_dir} "OLS.cpp" "OLS.h")

target_link_libraries(OpenLightShadow  ${OPENGL_gl_LIBRARY} imgui glm_static glfw3dll glew32s opengl32)

