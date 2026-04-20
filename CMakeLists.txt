cmake_minimum_required(VERSION 3.16)
project(phonedrop C)

set(CMAKE_C_STANDARD 11)

include_directories(
        src
        dependencies/nuklear
        dependencies/qrcodegen
)

set(PHONEDROP_SOURCES
        src/main.c
        src/server.c
        src/net_utils.c
        src/qr_utils.c
        src/gui.c
        dependencies/qrcodegen/qrcodegen.c
)

if(WIN32)
    set(WIN_RESOURCES assets/resources.rc)
    list(APPEND PHONEDROP_SOURCES ${WIN_RESOURCES})
endif()

add_executable(phonedrop ${PHONEDROP_SOURCES})

if(WIN32)
    target_link_libraries(phonedrop ws2_32 gdi32 user32 shell32 iphlpapi comdlg32 msimg32)
    set_target_properties(phonedrop PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    find_package(X11 REQUIRED)
    target_link_libraries(phonedrop ${X11_LIBRARIES} pthread m)
    target_include_directories(phonedrop PRIVATE ${X11_INCLUDE_DIR})
endif()

add_custom_command(TARGET phonedrop POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:phonedrop>/web)

if(UNIX)
    install(TARGETS phonedrop DESTINATION bin)
    install(DIRECTORY web DESTINATION share/phonedrop)
    install(FILES assets/phonedrop.desktop DESTINATION share/applications)
    install(FILES assets/phonedrop.svg DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES assets/phonedrop.png DESTINATION share/icons/hicolor/256x256/apps)
elseif(WIN32)
    install(TARGETS phonedrop DESTINATION .)
    install(DIRECTORY web DESTINATION .)
endif()

set(CPACK_PACKAGE_NAME "Phonedrop")
set(CPACK_PACKAGE_VENDOR "Willman's Toolbox")
set(CPACK_PACKAGE_VERSION "1.0.0")

if(UNIX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Willman <ticuette@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_DEPEND "libx11-6, zenity")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/postinst"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/postrm")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
endif()

include(CPack)