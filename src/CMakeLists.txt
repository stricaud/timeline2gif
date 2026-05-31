find_package(PkgConfig REQUIRED)
pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)
pkg_check_modules(WEBPMUX    REQUIRED libwebpmux)
pkg_check_modules(WEBP       REQUIRED libwebp)
pkg_check_modules(RSVG       REQUIRED librsvg-2.0)

include_directories(
    ${GD_INCLUDE_DIRS}
    ${FLEX_INCLUDE_DIRS}
    ${PANGOCAIRO_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${RSVG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(TIMELINE2GIF_SRC_FILES
    easing.c
    render.c
    output.c
    t2g.c
    timeline2gif.c
)

BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
FLEX_TARGET(lexer   lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

add_executable(timeline2gif
    ${TIMELINE2GIF_SRC_FILES}
    ${BISON_parser_OUTPUTS}
    ${FLEX_lexer_OUTPUTS}
)

target_link_libraries(timeline2gif
    ${GD_LIBRARIES}
    ${PANGOCAIRO_LDFLAGS}
    ${WEBPMUX_LDFLAGS}
    ${WEBP_LDFLAGS}
    ${RSVG_LDFLAGS}
    z        # zlib for APNG chunk CRC + compression
    m        # math for easing
    ${CMAKE_DL_LIBS}
)

target_compile_options(timeline2gif PRIVATE
    ${PANGOCAIRO_CFLAGS_OTHER}
    ${RSVG_CFLAGS_OTHER}
)

install(TARGETS timeline2gif
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    COMPONENT Application)
