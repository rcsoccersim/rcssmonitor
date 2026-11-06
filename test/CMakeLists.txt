
add_executable(test_message_splitter
  test_message_splitter.cpp
  ${PROJECT_SOURCE_DIR}/src/message_splitter.cpp
  )

target_include_directories(test_message_splitter
  PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src
  )

target_link_libraries(test_message_splitter
  PRIVATE
  Qt5::Core Qt5::Test
  )

add_test(NAME message_splitter COMMAND test_message_splitter)
