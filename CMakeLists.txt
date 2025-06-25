cmake_minimum_required(VERSION 3.16)
project(tystnad VERSION 0.1.0 LANGUAGES CXX)

option(TYSTNAD_BUILD_APPIMAGE "Bundle the app and optionally build an AppImage (Linux only)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(MACOS_ICON "data/tystnad.icns")

set(DATA_FILES
        data/logo.svg
        data/logo-off.svg
        data/logo-on.svg
)

include_directories(data-headers)
include_directories(include)

function(generate_cpp_headers OUT_VAR)
    set(headers "")
    set(SCRIPT "${CMAKE_SOURCE_DIR}/py/bin_to_header.py")

    foreach(input_file IN LISTS ARGN)
        get_filename_component(filename ${input_file} NAME)
        set(output_file "${CMAKE_SOURCE_DIR}/data-headers/${filename}.hpp")

        file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data-headers")

        add_custom_command(
                OUTPUT ${output_file}
                COMMAND python3 ${SCRIPT} ${CMAKE_SOURCE_DIR}/${input_file} ${output_file}
                DEPENDS ${input_file} ${SCRIPT}
                COMMENT "Generating C++ header for ${input_file} -> ${output_file}"
                VERBATIM
        )

        list(APPEND headers ${output_file})
    endforeach()

    set(${OUT_VAR} "${headers}" PARENT_SCOPE)
endfunction()

generate_cpp_headers(HEADER_FILES ${DATA_FILES})

add_custom_target(data-headers DEPENDS ${HEADER_FILES})

find_package(Qt6 REQUIRED COMPONENTS
        Core Widgets Svg SvgWidgets DBus
)

qt_add_executable(tystnad
        src/main.cpp
        include/config_dialog.hpp
        src/config_dialog.cpp
        include/audio_manager.hpp
        src/launch_agent.cpp
        include/launch_agent.hpp
        src/wav.cpp
        include/svg.hpp
        include/setting.hpp
        ${MACOS_ICON}
)

if (APPLE)
    add_compile_definitions(MACOS)
endif()
if (UNIX AND NOT APPLE)
    add_compile_definitions(LINUX)
endif()
add_compile_definitions(TYSTNAD_VERSION="${CMAKE_PROJECT_VERSION}")

add_dependencies(tystnad data-headers)

target_link_libraries(tystnad PRIVATE
        Qt6::Core Qt6::Widgets Qt6::Svg Qt6::SvgWidgets Qt6::DBus
)

if (APPLE)
    target_link_libraries(tystnad PRIVATE
            "-framework AudioToolbox"
    )
endif()
if (UNIX AND NOT APPLE)
    target_link_libraries(tystnad PRIVATE
        asound
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(TYSTNAD_DEBUG)
endif()

if (APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_source_files_properties(${MACOS_ICON} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )

    set_target_properties(tystnad PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_ICON_FILE "tystnad.icns"
    )

    configure_file(${CMAKE_SOURCE_DIR}/cmake/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist @ONLY)

    set_target_properties(tystnad PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
    )

    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")

    add_custom_command(TARGET tystnad POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}" $<TARGET_BUNDLE_DIR:tystnad>
            COMMENT "Running macdeployqt to bundle Qt frameworks and plugins"
    )

    add_custom_command(TARGET tystnad POST_BUILD
            COMMAND codesign --force --deep --sign - $<TARGET_BUNDLE_DIR:tystnad>
            COMMENT "Code signing the app bundle"
    )

    add_custom_command(TARGET tystnad POST_BUILD
            COMMAND xattr -rd com.apple.quarantine $<TARGET_BUNDLE_DIR:tystnad>
            COMMENT "Removing quarantine attribute from the app bundle"
    )
endif()

if (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/cmake/tystnad.desktop
            DESTINATION share/applications
    )

    install(FILES ${CMAKE_SOURCE_DIR}/data/logo.svg
            DESTINATION share/icons/hicolor/scalable/apps
            RENAME tystnad.svg
    )
endif()

if (UNIX AND NOT APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)

    if (TYSTNAD_BUILD_APPIMAGE AND LINUXDEPLOYQT_EXECUTABLE)
        message(STATUS "AppImage build enabled; will run linuxdeployqt")

        set(APPDIR "${CMAKE_BINARY_DIR}/AppDir")
        set(APP_BIN_DIR "${APPDIR}/usr/bin")
        set(APP_SHARE_DIR "${APPDIR}/usr/share")
        set(APP_DESKTOP_FILE "${APP_SHARE_DIR}/applications/tystnad.desktop")

        install(TARGETS tystnad DESTINATION ${APP_BIN_DIR})
        install(FILES ${CMAKE_SOURCE_DIR}/cmake/tystnad.desktop DESTINATION ${APP_SHARE_DIR}/applications)
        install(FILES ${CMAKE_SOURCE_DIR}/data/logo.svg DESTINATION ${APP_SHARE_DIR}/icons/hicolor/scalable/apps RENAME tystnad.svg)

        install(CODE "
            execute_process(COMMAND ${CMAKE_COMMAND} --install \"${CMAKE_BINARY_DIR}\" --prefix \"${APPDIR}\")
        ")

        add_custom_command(TARGET tystnad POST_BUILD
                COMMAND ${CMAKE_COMMAND} --install \"${CMAKE_BINARY_DIR}\" --prefix \"${APPDIR}\"
                COMMAND ${LINUXDEPLOYQT_EXECUTABLE} ${APP_DESKTOP_FILE} -bundle-non-qt-libs -appimage
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Bundling Qt dependencies and building AppImage using linuxdeployqt"
        )
    elseif (TYSTNAD_BUILD_APPIMAGE)
        message(WARNING "TYSTNAD_BUILD_APPIMAGE is ON but linuxdeployqt was not found")
    endif()
endif()
