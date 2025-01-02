cmake_minimum_required(VERSION 3.10)
project(index)

set(CMAKE_CXX_STANDARD 17)

# Determine the platform
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(PLATFORM_WEB "PLATFORM_WEB")
else()
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")
endif()

# Include directories
include_directories(
    include
    include/raylib
)

# Add Emscripten-specific include paths
if(EMSCRIPTEN)
    include_directories(${EMSCRIPTEN_ROOT_PATH}/upstream/emscripten/system/include/emscripten)
    include_directories(${EMSDK}/upstream/emscripten/system/include/emscripten)
    include_directories($ENV{EMSDK}/upstream/emscripten/cache/sysroot/include)
endif()

# Source files
file(GLOB SOURCES
    "src/*.cpp"
)

# Set build output directories
set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# Define the executable
add_executable(index ${SOURCES})

# Handle local definitions
if(USE_LOCAL)
    target_compile_definitions(index PRIVATE LOCAL)
endif()

# Platform-specific configuration
if(EMSCRIPTEN)
    # Link to appropriate Raylib library for web
    set(RAYLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylibweb.a")
    
    # Emscripten linker flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
        -s USE_GLFW=3 \
        -s ASSERTIONS=1 \
        -s WASM=1 \
        -s ASYNCIFY \
        -s FORCE_FILESYSTEM=1 \
        -s INITIAL_MEMORY=268435456 \
        -s STACK_SIZE=5242880 \
        --preload-file ../assets")
    
    target_compile_definitions(index PRIVATE ${PLATFORM_WEB})
else()
    # Native build configuration
    if(APPLE)
        set(RAYLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylibmac.a")
    elseif(WIN32)
        set(RAYLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a")
    else()
        set(RAYLIB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/libraylib.a")
    endif()
endif()

# Link Raylib and additional libraries
target_link_libraries(index ${RAYLIB_LIBRARY})

if(EMSCRIPTEN)
    # Add Emscripten-specific libraries
    target_link_libraries(index -lwebsocket.js)
else()
    # Native platform libraries
    if(WIN32)
        target_link_libraries(index -lwinmm -lgdi32 -lopengl32)
    elseif(APPLE)
        target_link_libraries(index "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
    else()
        target_link_libraries(index -lGL -lGLU -lm -lpthread)
    endif()
endif()
