include_directories(AFTER ${GD_INCLUDE_DIRS} ${FLEX_INCLUDE_DIRS} "${CMAKE_CURRENT_BINARY_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(TIMELINE2GIF_SRC_FILES
  effects.c
  t2g.c
  timeline2gif.c
  write-gif.c
)

BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
FLEX_TARGET(lexer lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
add_executable(timeline2gif ${TIMELINE2GIF_SRC_FILES} ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS})
target_link_libraries(timeline2gif ${GD_LIBRARIES} ${CMAKE_DL_LIBS})

install(TARGETS timeline2gif DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT Application)
