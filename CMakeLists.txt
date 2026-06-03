cmake_minimum_required(VERSION 3.16)
project(NodeRelay VERSION 0.19.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Svg)
find_package(Qt6Keychain QUIET)
if(NOT Qt6Keychain_FOUND)
    include(FetchContent)
    FetchContent_Declare(qtkeychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG        0.14.3
        GIT_SHALLOW    TRUE
    )
    set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
    set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "" FORCE)
    set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(qtkeychain)
    # qtkeychain 0.14.x creates qt6keychain in the build tree, not the
    # namespaced alias that only appears in installed CMake config files
    if(NOT TARGET Qt6Keychain::Qt6Keychain)
        foreach(_t qt6keychain qtkeychain)
            if(TARGET ${_t})
                add_library(Qt6Keychain::Qt6Keychain ALIAS ${_t})
                break()
            endif()
        endforeach()
    endif()
    # The source tree puts keychain.h at its root; installed packages put it
    # under qt6keychain/. Generate a forwarding header to match the installed
    # layout so #include <qt6keychain/keychain.h> resolves from either path.
    FetchContent_GetProperties(qtkeychain SOURCE_DIR _qtkc_src BINARY_DIR _qtkc_bin)
    set(_qtkc_compat "${CMAKE_BINARY_DIR}/qt6keychain-compat")
    file(MAKE_DIRECTORY "${_qtkc_compat}/qt6keychain")
    file(WRITE "${_qtkc_compat}/qt6keychain/keychain.h"
        "#pragma once\n#include \"${_qtkc_src}/keychain.h\"\n")
    # _qtkc_bin holds qkeychain_export.h (generated at configure time by CMake)
    set(QTKEYCHAIN_COMPAT_INCLUDE "${_qtkc_compat}" "${_qtkc_bin}")
endif()
find_package(tomlplusplus QUIET)
if(NOT tomlplusplus_FOUND)
    include(FetchContent)
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.4.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()
qt_standard_project_setup()

qt_add_executable(NodeRelay WIN32
    resources/resources.qrc
    src/main.cpp
    src/config/config.cpp
    src/config/config.h
    src/config/keychainhelper.cpp
    src/config/keychainhelper.h
    src/model/sessionmodel.cpp
    src/model/sessionmodel.h
    src/model/serversession.h
    src/model/channel.h
    src/model/message.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/channelpane.cpp
    src/ui/channelpane.h
    src/ui/trayicon.cpp
    src/ui/trayicon.h
    src/ui/aboutdialog.cpp
    src/ui/aboutdialog.h
    src/ui/docsdialog.cpp
    src/ui/docsdialog.h
    src/ui/fontdialog.cpp
    src/ui/fontdialog.h
    src/ui/serverdialog.cpp
    src/ui/serverdialog.h
    src/ui/manageserversdialog.cpp
    src/ui/manageserversdialog.h
    src/ui/preferencesdialog.cpp
    src/ui/preferencesdialog.h
    src/ui/appicons.h
    src/ui/emojidata.h
    src/ui/emojipicker.h
    src/ui/emojipicker.cpp
    src/ui/linkpreview.cpp
    src/ui/linkpreview.h
    src/ui/signalbars.cpp
    src/ui/signalbars.h
    src/ui/themeloader.cpp
    src/ui/themeloader.h
    src/irc/ircclient.cpp
    src/irc/ircclient.h
    src/irc/ircparser.cpp
    src/irc/ircparser.h
    src/irc/stsstore.cpp
    src/irc/stsstore.h
    src/irc/dccsend.cpp
    src/irc/dccsend.h
    src/irc/dccreceive.cpp
    src/irc/dccreceive.h
)

configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
target_include_directories(NodeRelay PRIVATE src ${CMAKE_CURRENT_BINARY_DIR} ${QTKEYCHAIN_COMPAT_INCLUDE})

target_link_libraries(NodeRelay PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6Keychain::Qt6Keychain
    tomlplusplus::tomlplusplus
)

# Prevent .relr.dyn sections that linuxdeploy's bundled strip cannot handle
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(NodeRelay PRIVATE -Wl,-z,nopack-relative-relocs)
endif()

# Copy themes next to binary for development
add_custom_command(TARGET NodeRelay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/themes $<TARGET_FILE_DIR:NodeRelay>/themes
)

if(APPLE)
    set_target_properties(NodeRelay PROPERTIES MACOSX_BUNDLE TRUE)
endif()

install(TARGETS NodeRelay DESTINATION bin)
install(DIRECTORY themes/ DESTINATION share/noderelay/themes)
install(FILES packaging/NodeRelay.desktop DESTINATION share/applications)
install(FILES resources/icons/icon-tower.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME noderelay.svg)
