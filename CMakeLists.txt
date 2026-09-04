cmake_minimum_required(VERSION 3.15)
project(ShaderTester)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# CROSS-PLATFORM COMPILER FIXES
# ==============================================================================
if(MSVC)
    # Windows/MSVC only: Force static runtime to prevent linker mismatch errors
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
#    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
# Enable stuff below if memory leak debugging is desired
# ==============================================================================
# MEMORY LEAK DETECTION (Address Sanitizer)
# Note: On Windows, you must have the "C++ AddressSanitizer" component 
# installed via the Visual Studio Installer for this to work.
# ==============================================================================
#set(ENABLE_ASAN OFF CACHE BOOL "Enable Address Sanitizer for memory debugging")
#
#if(ENABLE_ASAN)
#    if(MSVC)
#        add_compile_options(/fsanitize=address)
#    else()
#        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
#        add_link_options(-fsanitize=address)
#    endif()
#endif()

# ==============================================================================
# Ubuntu 24.04 requirements
# Vulkan SDK is required for jolt physics
# wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
# sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
# sudo apt update
# sudo apt install vulkan-sdk
# note: vulkan sdk install is own version of glslc, if it's installed remove it first
# ==============================================================================
if(UNIX AND NOT APPLE)
    # Linux only: Ensure we compile with position-independent code
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
if(WIN32)
    # Prevents Windows.h from defining min/max macros
    add_definitions(-DNOMINMAX)
    # Strips out rarely used Windows APIs (helps with Raylib collisions)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()
# ==============================================================================
# DEPENDENCIES
# ==============================================================================

# --- 1. RAYLIB ---
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
add_subdirectory(raylib)

# --- 2. JOLT PHYSICS ---
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
#set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
#set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
#set(TARGET_TEST_FRAMEWORK OFF CACHE BOOL "" FORCE)
#set(GENERATE_SHADERS OFF CACHE BOOL "" FORCE)
add_subdirectory(JoltPhysics/Build)

# --- 3. IMGUI & RLIMGUI ---
# Grab all the core ImGui source files
file(GLOB IMGUI_SOURCES "imgui/*.cpp")
# We also need to compile the rlImGui bridge file
set(RLIMGUI_SOURCE "rlImGui/rlImGui.cpp")

# Custom stuff being added:
file(GLOB_RECURSE PROJECT_CORE 
        "ProjectCore/*.cpp"
        "ProjectCore/*.h"
)

# ==============================================================================
# MAIN EXECUTABLE
# ==============================================================================

# Compile our main file alongside the ImGui UI files
add_executable(${PROJECT_NAME} main.cpp ${IMGUI_SOURCES} ${RLIMGUI_SOURCE} ${PROJECT_CORE})

# Tell the compiler where to find all the #include headers
target_include_directories(${PROJECT_NAME} PRIVATE  
    imgui 
    rlImGui 
    ProjectCore
)

# Link the compiled engines to our executable
target_link_libraries(${PROJECT_NAME} PRIVATE
        raylib
        Jolt
)
