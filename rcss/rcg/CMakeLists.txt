
add_library(rcssrcg SHARED
	handler.cpp
	parser.cpp
	parser_v1.cpp
	parser_v2.cpp
	parser_v3.cpp
	parser_v4.cpp
	parser_json.cpp
	util.cpp
	types.cpp
	team_graphic.cpp
	xpm_tile.cpp
  )

target_include_directories(rcssrcg
  PUBLIC
#  ${Boost_INCLUDE_DIRS}
  PRIVATE
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
  )

target_compile_options(rcssrcg
  PRIVATE
  -W -Wall
  )


set_target_properties(rcssrcg PROPERTIES
  VERSION 17.0.0
  SOVERSION 17
  )

install(TARGETS rcssrcg LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT Libraries
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  )

install(FILES
  handler.h
  parser.h
  parser_v1.h
  parser_v2.h
  parser_v3.h
  parser_v4.h
  types.h
  util.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rcss/rcg
  )
