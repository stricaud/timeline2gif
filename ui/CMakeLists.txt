find_package(wxWidgets REQUIRED COMPONENTS core base adv stc)
include(${wxWidgets_USE_FILE})

# wxWidgets 3.x requires at least C++11; use 17 for broad compatibility.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(t2g-ui WIN32 MACOSX_BUNDLE
    main.cpp
    MainFrame.cpp
    PreviewPanel.cpp
    AddEventDialog.cpp
    ThemeDialog.cpp
    PlaybackFrame.cpp
    TigEditor.cpp
)

target_include_directories(t2g-ui PRIVATE
    ${wxWidgets_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src          # for timeline2gif.h
    ${CMAKE_BINARY_DIR}/src          # for parser.h (generated)
)
target_link_libraries(t2g-ui PRIVATE t2g ${wxWidgets_LIBRARIES})

# On macOS, embed the bundle name so it shows nicely in the dock
set_target_properties(t2g-ui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME    "Timeline Studio"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.timeline2gif.timeline-studio"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION       "1"
    OUTPUT_NAME "Timeline")

# App icon — icns is generated from ui/icon.svg before cmake runs in CI,
# or can be pre-generated locally: see README for instructions.
if(APPLE)
    set(_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.icns")
    if(EXISTS "${_ICNS}")
        set_source_files_properties("${_ICNS}"
            PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(t2g-ui PRIVATE "${_ICNS}")
        set_target_properties(t2g-ui PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
    endif()
endif()

install(TARGETS t2g-ui
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
