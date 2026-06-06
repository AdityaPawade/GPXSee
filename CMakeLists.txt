cmake_minimum_required(VERSION 3.21)

project(GPXSee VERSION 16.8 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# GPXSee — CMake build (replaces the historical qmake gpxsee.pro).
# Dependencies are supplied by the vcpkg manifest (vcpkg.json) through the
# vcpkg toolchain file: Qt 5.15.14 + OpenSSL 3.x, all already present in the
# local vcpkg.  Nothing is copied by hand; vcpkg + windeployqt deploy the DLLs.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt modules — mirrors "QT += ..." in gpxsee.pro (Qt5 path; no openglwidgets,
# which is Qt6-only in the .pro).  Positioning comes from qt5-location.
find_package(Qt5 5.15 REQUIRED COMPONENTS
    Core Gui Widgets Network Sql Concurrent PrintSupport
    Positioning Svg SerialPort Multimedia MultimediaWidgets
)

# zlib — GPXSee calls it directly (common/util.cpp). qmake used Qt's bundled
# QtZlib header on Windows; with vcpkg we use the standalone zlib that is
# already installed as a qt5-base dependency.
find_package(ZLIB REQUIRED)

# Sources — mirrors "$$files(src/*.cpp, true)" recursive globbing in gpxsee.pro
file(GLOB_RECURSE GPXSEE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE GPXSEE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

# Resources (platform-dependent theme, as in gpxsee.pro)
set(GPXSEE_RESOURCES gpxsee.qrc)
if(WIN32)
    list(APPEND GPXSEE_RESOURCES theme-color.qrc)
else()
    list(APPEND GPXSEE_RESOURCES theme-grayscale.qrc)
endif()

add_executable(GPXSee WIN32
    ${GPXSEE_SOURCES}
    ${GPXSEE_HEADERS}
    ${GPXSEE_RESOURCES}
)

target_include_directories(GPXSee PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}   # QZipReader lives in gui-private (gpxsee.pro)
)

target_compile_definitions(GPXSee PRIVATE APP_VERSION="${PROJECT_VERSION}")
if(WIN32)
    target_compile_definitions(GPXSee PRIVATE _USE_MATH_DEFINES NOGDI)
endif()
if(MSVC)
    target_compile_options(GPXSee PRIVATE /MP /utf-8)
endif()

target_link_libraries(GPXSee PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Sql Qt5::Concurrent
    Qt5::PrintSupport Qt5::Positioning Qt5::Svg Qt5::SerialPort
    Qt5::Multimedia Qt5::MultimediaWidgets
    ZLIB::ZLIB
)

# ---------------------------------------------------------------------------
# Deployment.  The vcpkg toolchain's applocal step copies the runtime DLLs
# (Qt5*.dll, libssl-3-x64.dll, libcrypto-3-x64.dll, ...) and Qt plugins next
# to the executable automatically at build time — no manual DLL copying.
# install(TARGETS) repeats it for a clean deploy/ folder.
# ---------------------------------------------------------------------------
# Ship the telemetry-overlay config next to the executable so VizConfig
# finds it via QCoreApplication::applicationDirPath() (build tree + deploy).
add_custom_command(TARGET GPXSee POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/viz.cfg"
            "$<TARGET_FILE_DIR:GPXSee>/viz.cfg"
    COMMENT "Deploying viz.cfg next to GPXSee.exe"
)

install(TARGETS GPXSee RUNTIME DESTINATION ".")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/viz.cfg" DESTINATION ".")

# Ship a default online basemap so a fresh install has a map out of the box
# (used when no user maps dir exists; Qt scans the application directory too).
add_custom_command(TARGET GPXSee POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GPXSee>/maps"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/maps/OpenStreetMap.xml"
            "$<TARGET_FILE_DIR:GPXSee>/maps/OpenStreetMap.xml"
    COMMENT "Deploying default online map next to GPXSee.exe"
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/maps" DESTINATION ".")
