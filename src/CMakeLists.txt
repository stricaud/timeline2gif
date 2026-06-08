find_package(PkgConfig REQUIRED)
pkg_check_modules(PANGOCAIRO REQUIRED pangocairo)
pkg_check_modules(WEBPMUX    REQUIRED libwebpmux)
pkg_check_modules(WEBP       REQUIRED libwebp)
pkg_check_modules(RSVG       REQUIRED librsvg-2.0)

set(T2G_INCLUDE_DIRS
    ${GD_INCLUDE_DIRS}
    ${FLEX_INCLUDE_DIRS}
    ${PANGOCAIRO_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${RSVG_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(T2G_LINK_LIBS
    ${GD_LIBRARIES}
    ${PANGOCAIRO_LDFLAGS}
    ${WEBPMUX_LDFLAGS}
    ${WEBP_LDFLAGS}
    ${RSVG_LDFLAGS}
    z        # zlib for APNG chunk CRC + compression
    m        # math for easing
    ${CMAKE_DL_LIBS}
)

set(T2G_COMPILE_OPTS
    ${PANGOCAIRO_CFLAGS_OTHER}
    ${RSVG_CFLAGS_OTHER}
)

BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
FLEX_TARGET(lexer   lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

# ── Core static library (used by both CLI and UI) ───────────────────
add_library(t2g STATIC
    easing.c
    render.c
    output.c
    t2g.c
    timeline2gif.c   # contains t2g_generate(), no main()
    ${BISON_parser_OUTPUTS}
    ${FLEX_lexer_OUTPUTS}
)

target_include_directories(t2g PUBLIC ${T2G_INCLUDE_DIRS})
target_link_libraries(t2g PUBLIC ${T2G_LINK_LIBS})
target_compile_options(t2g PRIVATE ${T2G_COMPILE_OPTS})

# ── CLI executable (thin main() wrapper around libt2g) ───────────────
add_executable(timeline2gif cli.c)
target_link_libraries(timeline2gif PRIVATE t2g)

install(TARGETS timeline2gif
    DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    COMPONENT Application)
