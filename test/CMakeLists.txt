add_executable(test_rcg_parser
  test_rcg_parser.cpp
  )

target_include_directories(test_rcg_parser
  PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  )

target_compile_definitions(test_rcg_parser
  PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
  )

target_link_libraries(test_rcg_parser
  PRIVATE
  rcssrcg
  Qt5::Core Qt5::Test
  )

add_test(NAME rcg_parser COMMAND test_rcg_parser)
