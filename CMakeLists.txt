# blu_fx build script.
#
# Useful command line flags:
#   * -DCMAKE_BUILD_TYPE=XXX, where XXX is one of Debug, Release,
#     RelWithDebInfo and MinSizeRel. Default is Release.
#     (NOTE: not supported; this comment was copied ... :-) )

cmake_minimum_required(VERSION 3.9)
project(blu_fx VERSION 2.7.5 DESCRIPTION "Blu-FX X-Plane plugin")

# By default, use optimized release configuration.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 14)

# Set include directories used by our code and dependencies.
include_directories("${CMAKE_SOURCE_DIR}/SDK/CHeaders/XPLM")
include_directories("${CMAKE_SOURCE_DIR}/SDK/CHeaders/Widgets")
include_directories("${CMAKE_SOURCE_DIR}/GLee5_4")
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

# Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)
# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)
# Define GLEXT prototypes option:
add_definitions(-DGL_GLEXT_PROTOTYPES)

# Force-enable exception support. This is most likely redundant, although for C
# code the default is the opposite. Since we are mixing C++ and C libraries,
# safer to set it on?
add_compile_options(-fexceptions)
# On UNIX systems this makes symbols non-exported by default. On Windows this
# option is simply ignored, since symbol visibility works differently there.
add_compile_options(-fvisibility=hidden)

if (WIN32)
    add_compile_options(-fpermissive)
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

# X-Plane plugin
list(APPEND BLUFX_SRCS blu_fx.cpp GLee5_4/GLee.c)

add_library(blu_fx SHARED ${BLUFX_SRCS})

target_compile_features(blu_fx PUBLIC cxx_std_11)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING
        "Minimum macOS version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64"
        CACHE STRING "Build architectures for mac OS X" FORCE)
    # X-Plane supports OS X 10.10+, so this should ensure blu_fx can run on
    # all supported versions.
    target_compile_options(blu_fx PUBLIC
                           -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    target_link_libraries(blu_fx
                          -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif ()

# Link OpenGL and related libraries.
if (UNIX)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
find_library(GLUT_LIBRARY NAMES glut GLUT glut64)  # apt install freeglut3-dev
target_link_libraries(blu_fx ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
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
    target_link_libraries(blu_fx ${CORE_FOUNDATION_LIBRARY})
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

# Install target based on platform
if (WIN32)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/win_x64")
elseif (APPLE)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/mac_x64")
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp12/Resources/plugins/blu_fx/mac_x64")
elseif (UNIX)
    install(TARGETS blu_fx DESTINATION
            "/Users/brat/xp11/Resources/plugins/blu_fx/lin_x64")
endif ()