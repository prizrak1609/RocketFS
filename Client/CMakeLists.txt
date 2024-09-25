cmake_minimum_required(VERSION 3.16)

project(Client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

set(PROJECT_SOURCES
        main.cpp

        server.h
        server.cpp

        filesystemdatasource.h
        filesystemdatasource.cpp

        filesystem.h
        filesystem.cpp
)

qt_add_executable(Client MANUAL_FINALIZATION ${PROJECT_SOURCES})
# Define target properties for Android with Qt 6 as:
set_property(TARGET Client APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)

add_dependencies(Client WebSocketConnectionLib CommandsLib)

target_link_directories(Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} "C:/Program Files (x86)/WinFsp/lib")
target_link_libraries(Client
    PRIVATE

    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2

    CommandsLib
    WebSocketConnectionLib

    winfsp-x64
)

qt_add_qml_module(Client
    URI qml.module
    VERSION 1.0
    QML_FILES
        "MainWindow.qml"
        "NavigationView.qml"
        "Connect.qml"
        "TextInputField.qml"
        "BorderColor.qml"
        "FileSystem.qml"
)

target_include_directories(Client PRIVATE "${CommandsLib_INCLUDES}" "${WebSocketConnectionLib_INCLUDES}" "C:/Program Files (x86)/WinFsp/inc")

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

qt_generate_deploy_qml_app_script(
    TARGET Client
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

qt_finalize_executable(Client)
