
add_executable(rcssmonitor
  rcsslogplayer/gzfstream.cpp
  rcsslogplayer/parser.cpp
  rcsslogplayer/types.cpp
  rcsslogplayer/util.cpp
  angle_deg.cpp
  ball_painter.cpp
  circle_2d.cpp
  config_dialog.cpp
  disp_holder.cpp
  draw_info_painter.cpp
  field_canvas.cpp
  field_painter.cpp
  line_2d.cpp
  log_player.cpp
  log_player_slider.cpp
  main_window.cpp
  monitor_client.cpp
  options.cpp
  player_painter.cpp
  player_type_dialog.cpp
  score_board_painter.cpp
  team_graphic.cpp
  team_graphic_painter.cpp
  vector_2d.cpp
  main.cpp
  )

target_include_directories(rcssmonitor
  PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
#  ${Boost_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  PRIVATE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src
  )

target_link_libraries(rcssmonitor
  Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network
#  Boost::program_options Boost::system
  ZLIB::ZLIB
  )

target_compile_definitions(rcssmonitor
  PUBLIC
  HAVE_CONFIG_H
)

install(TARGETS rcssmonitor
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
