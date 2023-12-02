cmake_minimum_required(VERSION 3.5)

project(Client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebSockets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui

        filesystem/filesystem.h
        filesystem/filesystem.cpp
        filesystem/filesystem_operations.cpp
        filesystem/filesystemimpl.h

        connection/connection.h
        connection/connection.cpp
        connection/connection_pool.h
        connection/connection_pool.cpp

        command/ICommand.h
        command/getattrcmd.h
        command/getattrcmd.cpp
        command/readdircmd.h
        command/readdircmd.cpp
        command/rmdircmd.h
        command/rmdircmd.cpp
        command/mkdircmd.h
        command/mkdircmd.cpp
        command/renamecmd.h
        command/renamecmd.cpp
        command/createfilecmd.h
        command/createfilecmd.cpp
        command/rmfilecmd.h
        command/rmfilecmd.cpp
        command/openfilecmd.h
        command/openfilecmd.cpp
        command/readfilecmd.h
        command/readfilecmd.cpp
        command/writefilecmd.h
        command/writefilecmd.cpp
        command/closefilecmd.h
        command/closefilecmd.cpp
        command/statfscmd.h
        command/statfscmd.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)

qt_add_executable(Client
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Client APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Client SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Client
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Client PRIVATE  Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::WebSockets "C:\\Program Files (x86)\\WinFsp\\lib\\winfsp-x64.lib")

target_include_directories(Client PRIVATE "C:\\Program Files (x86)\\WinFsp\\inc" "filesystem" "connection" "command")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Client)
endif()
set_target_properties(Client PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Client)
endif()
