# blu_fx build script.
#
# Useful command line flags:
#   * -DCMAKE_BUILD_TYPE=XXX, where XXX is one of Debug, Release,
#     RelWithDebInfo and MinSizeRel. Default is Release.
#     (NOTE: not supported; this comment was copied ... :-) )

cmake_minimum_required(VERSION 3.9)

# Must be set BEFORE project() so CMake initializes the macOS toolchain for Universal Binary
# (x86_64 + arm64) and deployment target on the very first configure pass (ignored on non-macOS).
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum macOS version" FORCE)
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)

project(blu_fx VERSION 2.1.0 DESCRIPTION "Blu-FX X-Plane plugin")

# By default, use optimized release configuration.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------------------
# Self-Healing Submodule Initialization (ImgWindow & ImGee)
# ------------------------------------------------------------------------------
find_package(Git QUIET)
if(GIT_FOUND)
    # Resolve ImgWindow path (supports both ./ImgWindow and ./src/ImgWindow layouts)
    set(_IMGWINDOW_CHECK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImgWindow")
    if(NOT EXISTS "${_IMGWINDOW_CHECK_DIR}")
        set(_IMGWINDOW_CHECK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ImgWindow")
    endif()

    # Resolve enclosing git repository root (handles both top-level and src/ CMakeLists.txt)
    set(_GIT_REPO_ROOT "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        set(_GIT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
        get_filename_component(_GIT_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    endif()

    # Case 1: ImgWindow itself is not yet initialized (fresh clone without --recursive)
    if(_GIT_REPO_ROOT AND NOT EXISTS "${_IMGWINDOW_CHECK_DIR}/ImgWindow.h")
        message(STATUS "[ImgWindow] Submodule missing -- initializing recursively...")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive
            WORKING_DIRECTORY "${_GIT_REPO_ROOT}"
            RESULT_VARIABLE _GIT_SUBMOD_RESULT
        )
        if(NOT _GIT_SUBMOD_RESULT EQUAL "0")
            message(WARNING "[ImgWindow] Automatic submodule update failed. Please run 'git submodule update --init --recursive' manually.")
        else()
            message(STATUS "[ImgWindow] Submodules successfully initialized!")
        endif()
    # Case 2: ImgWindow is present, but nested contrib/ImGee submodule is uninitialized.
    # Runs inside ImgWindow itself so we initialize ImGee without reverting a local ImgWindow checkout!
    elseif(EXISTS "${_IMGWINDOW_CHECK_DIR}/.git"
           AND EXISTS "${_IMGWINDOW_CHECK_DIR}/.gitmodules"
           AND NOT EXISTS "${_IMGWINDOW_CHECK_DIR}/contrib/ImGee/imgee.h")
        message(STATUS "[ImgWindow] contrib/ImGee submodule missing -- initializing recursively...")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" submodule update --init --recursive
            WORKING_DIRECTORY "${_IMGWINDOW_CHECK_DIR}"
            RESULT_VARIABLE _GIT_SUBMOD_RESULT
        )
        if(NOT _GIT_SUBMOD_RESULT EQUAL "0")
            message(WARNING "[ImgWindow] Automatic contrib/ImGee submodule update failed. Please run 'git -C \"${_IMGWINDOW_CHECK_DIR}\" submodule update --init --recursive' manually.")
        else()
            message(STATUS "[ImgWindow] contrib/ImGee submodule successfully initialized!")
        endif()
    endif()
endif()

# Set include directories used by our code and dependencies.
include_directories("${CMAKE_SOURCE_DIR}/SDK/CHeaders/XPLM")
include_directories("${CMAKE_SOURCE_DIR}/SDK/CHeaders/Widgets")
include_directories("${CMAKE_SOURCE_DIR}/GLee5_4")
include_directories("${CMAKE_SOURCE_DIR}/ImGui")
include_directories("${CMAKE_SOURCE_DIR}/ImGui/misc/cpp")
include_directories("${CMAKE_SOURCE_DIR}/ImgWindow")
include_directories("${CMAKE_SOURCE_DIR}/.")

if (WIN32)
#TODO: find and install glut for Windows here or else update this:
#    include_directories("${CMAKE_SOURCE_DIR}/GLUT_for_Windows")
endif (WIN32)

# Specify library search locations.
if (WIN32)
    #list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/include64")
#    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/GLUT_for_Windows")
#    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/SDK/Libraries/Win")
elseif (APPLE)
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_SOURCE_DIR}/SDK/Libraries/Mac")
    #list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_SOURCE_DIR}/mac_libs")
elseif (UNIX)
    # any other dependencies here
endif ()

# Enable all X-Plane SDK APIs up to 3xx (and DO NOT define XPLM440 so we support XP11.10+ cleanly).
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM303=1)
# Enable ImgWindow v2 dynamic Panel Graphics support (XP12.4.4+ with automatic OpenGL fallback):
add_definitions(-DIMGWINDOW_USE_PANEL_GRAPHICS)
# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)
# Define GLEXT prototypes option:
add_definitions(-DGL_GLEXT_PROTOTYPES)
if (APPLE)
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif ()

# Force-enable exception support. This is most likely redundant, although for C
# code the default is the opposite. Since we are mixing C++ and C libraries,
# safer to set it on?
add_compile_options(-fexceptions)
# On UNIX systems this makes symbols non-exported by default. On Windows this
# option is simply ignored, since symbol visibility works differently there.
add_compile_options(-fvisibility=hidden)

if (WIN32)
    add_compile_options(-fpermissive)
    add_definitions(-D__gl_glext_h_=1)
endif ()

# Enable stricter warnings and then disable some we are not interested in.
#TEMPORARILY COMMENTED OUT; see below:
#add_compile_options(-Wall -Wpedantic -Wshadow -Wfloat-equal -Wextra)
#add_compile_options(-Wno-unused)
#
# Instead, brat sez just get rid of all those stupid warnings for now:
add_compile_options(-w)

# Always use position-independent code and highest optimization level (FPS!).
add_compile_options(-O3 -fPIC)

# Turn on Debug mode
# add_compile_options(-g)

# X-Plane plugin sources
list(APPEND BLUFX_SRCS
    blu_fx.cpp
    settings-img-window.cpp
    doc-viewer-img-window.cpp
    GLee5_4/GLee.c
    ImGui/imgui.cpp
    ImGui/imgui_demo.cpp
    ImGui/imgui_draw.cpp
    ImGui/imgui_tables.cpp
    ImGui/imgui_widgets.cpp
    ImGui/misc/cpp/imgui_stdlib.cpp
    ImgWindow/ImgFontAtlas.cpp
    ImgWindow/ImgWindow.cpp
)

add_library(blu_fx SHARED ${BLUFX_SRCS})

target_compile_features(blu_fx PUBLIC cxx_std_17)

if (APPLE)
    set_target_properties(blu_fx PROPERTIES OSX_ARCHITECTURES "x86_64;arm64")
    # X-Plane supports OS X 10.10+, so this should ensure blu_fx can run on
    # all supported versions.
    target_compile_options(blu_fx PUBLIC
                           -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    target_link_libraries(blu_fx
                          -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif ()

# Link OpenGL and related libraries.
if (UNIX)
find_package(OpenGL REQUIRED)
target_link_libraries(blu_fx ${OPENGL_LIBRARIES})
elseif (WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(blu_fx ${OPENGL_LIBRARIES})
endif ()

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32)
    target_link_libraries(blu_fx ${CMAKE_SOURCE_DIR}/SDK/Libraries/Win/XPLM_64.lib)
    target_link_libraries(blu_fx ${CMAKE_SOURCE_DIR}/SDK/Libraries/Win/XPWidgets_64.lib)
    target_link_libraries(blu_fx winmm.lib)
elseif (APPLE)
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib)
    find_library(XPWIDGETS_LIBRARY NAMES XPWidgets XPWidgets_64.lib)
    target_link_libraries(blu_fx ${XPLM_LIBRARY} ${XPWIDGETS_LIBRARY})
endif ()

#TODO: I think these are not needed (I copied them from another makelists):
# Link library for dynamic loading of shared objects on UNIX systems.
#if (UNIX)
#    find_library(DL_LIBRARY dl)
#    target_link_libraries(blu_fx ${DL_LIBRARY})
#endif ()

# Link OS X core system libraries.
if (APPLE)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(APPLICATION_SERVICES_LIBRARY ApplicationServices)
    target_link_libraries(blu_fx ${CORE_FOUNDATION_LIBRARY} ${APPLICATION_SERVICES_LIBRARY})
endif ()

if (WIN32)
    # Unlike OS X and Linux we build standard libraries statically since X-Plane does not provide them.
    target_link_libraries(blu_fx -static-libgcc -static-libstdc++)
    # MSYS2 for some reason always links against winpthread, so we have to work around it by statically linking the
    # entire plugin. This allows building the plugin nativaly on Windows.
    target_link_libraries(blu_fx -static)
elseif (APPLE)
    # Restrict set of symbols exported from the plugin:
    target_link_libraries(blu_fx "-exported_symbols_list ${CMAKE_SOURCE_DIR}/blu_fx.sym_mac")
elseif (UNIX)
    # Specify additional runtime search laths for dynamically-linked libraries.
#TODO: maybe remove this; currently it's commented out:
#   target_link_libraries(blu_fx -Wl,-rpath=./Resources/plugins/blu_fx/64)
    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(blu_fx -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/blu_fx.sym)
endif ()


# set_target_properties(blu_fx PROPERTIES PREFIX "")
# if (WIN32)
#     set_target_properties(blu_fx PROPERTIES OUTPUT_NAME "win")
# elseif (APPLE)
#     set_target_properties(blu_fx PROPERTIES OUTPUT_NAME "mac")
# elseif (UNIX)
#     set_target_properties(blu_fx PROPERTIES OUTPUT_NAME "lin")
# endif ()
# set_target_properties(blu_fx PROPERTIES SUFFIX ".xpl")

set_target_properties(blu_fx
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "blu_fx"
    SUFFIX ".xpl"
)

if (APPLE)
    set_target_properties(blu_fx PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (WIN32)
    set_target_properties(blu_fx PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
    set_target_properties(blu_fx PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
elseif (UNIX)
    set_target_properties(blu_fx PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif ()

# Install target based on platform
if (WIN32)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/win_x64")
elseif (APPLE)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/mac_x64")
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp12/Resources/plugins/blu_fx/mac_x64")
    install(DIRECTORY docs DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx")
    install(DIRECTORY docs DESTINATION
            "/Users/brat/xp12/Resources/plugins/blu_fx")
    install(DIRECTORY docs/images DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx")
    install(DIRECTORY docs/images DESTINATION
            "/Users/brat/xp12/Resources/plugins/blu_fx")
elseif (UNIX)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/lin_x64")
endif ()