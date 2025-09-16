cmake_minimum_required(VERSION 3.16)
project(qwhisper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Network)

# Find optional packages
find_package(Threads REQUIRED)

# Add whisper.cpp as external project (we'll download it)
include(FetchContent)

# Configure whisper to build as static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "whisper: build tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "whisper: build examples" FORCE)

# Enable CUDA support if available
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(GGML_CUDA ON CACHE BOOL "ggml: enable CUDA" FORCE)
    message(STATUS "CUDA support enabled for Whisper")
else()
    message(STATUS "CUDA not found, GPU acceleration will not be available")
endif()

FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(whisper)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/audio/audiocapture.cpp
    src/audio/audioprocessor.cpp
    src/audio/audiofilter.cpp
    src/whisper/whisperprocessor.cpp
    src/whisper/whispermodels.cpp
    src/whisper/devicemanager.cpp
    src/whisper/modeldownloader.cpp
    src/config/configmanager.cpp
    src/output/outputmanager.cpp
    src/output/fileoutput.cpp
    src/output/windowtyper.cpp
    src/ui/configwidget.cpp
    src/ui/transcriptwidget.cpp
    src/ui/audiomonitor.cpp
    src/ui/settingsdialog.cpp
)

# Header files
set(HEADERS
    src/mainwindow.h
    src/audio/audiocapture.h
    src/audio/audioprocessor.h
    src/audio/audiofilter.h
    src/whisper/whisperprocessor.h
    src/whisper/whispermodels.h
    src/whisper/devicemanager.h
    src/whisper/modeldownloader.h
    src/config/configmanager.h
    src/output/outputmanager.h
    src/output/fileoutput.h
    src/output/windowtyper.h
    src/ui/configwidget.h
    src/ui/transcriptwidget.h
    src/ui/audiomonitor.h
    src/ui/settingsdialog.h
)

# Resources
set(RESOURCES
    resources/qwhisper.qrc
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
    whisper
    ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${whisper_SOURCE_DIR}
    ${whisper_SOURCE_DIR}/include
    ${whisper_SOURCE_DIR}/ggml/include
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux specific
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED libpulse)
    target_link_libraries(${PROJECT_NAME} ${PULSE_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${PULSE_INCLUDE_DIRS})
    
    # X11 libraries for window typing functionality
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIR})
        
        # Check for XTest extension
        if(X11_XTest_FOUND)
            target_link_libraries(${PROJECT_NAME} ${X11_XTest_LIB})
            message(STATUS "X11 XTest extension found - window typing will be available")
        else()
            message(WARNING "X11 XTest extension not found - window typing may not work")
        endif()
    else()
        message(WARNING "X11 not found - window typing will use fallback methods")
    endif()
endif()

# Set RPATH for Linux to find whisper libraries
if(UNIX AND NOT APPLE)
    # Get the whisper library directories
    get_target_property(WHISPER_LIB_DIR whisper BINARY_DIR)
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/_deps/whisper-build/src:${CMAKE_BINARY_DIR}/_deps/whisper-build/ggml/src"
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install whisper libraries if they are shared
if(BUILD_SHARED_LIBS)
    if(UNIX AND NOT APPLE)
        # Install whisper and ggml shared libraries
        install(FILES 
            ${CMAKE_BINARY_DIR}/_deps/whisper-build/src/libwhisper.so
            ${CMAKE_BINARY_DIR}/_deps/whisper-build/ggml/src/libggml.so
            DESTINATION lib
            OPTIONAL
        )
    endif()
endif()

# Create a launcher script for development
if(UNIX AND NOT APPLE)
    file(WRITE ${CMAKE_BINARY_DIR}/qwhisper-launcher.sh
"#!/bin/bash
# Auto-generated launcher script
SCRIPT_DIR=\"\$( cd \"\$( dirname \"\${BASH_SOURCE[0]}\" )\" && pwd )\"
export LD_LIBRARY_PATH=\"\${SCRIPT_DIR}/_deps/whisper-build/src:\${SCRIPT_DIR}/_deps/whisper-build/ggml/src:\$LD_LIBRARY_PATH\"
exec \"\${SCRIPT_DIR}/qwhisper\" \"\$@\"
")
    
    file(CHMOD ${CMAKE_BINARY_DIR}/qwhisper-launcher.sh
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)
endif()
