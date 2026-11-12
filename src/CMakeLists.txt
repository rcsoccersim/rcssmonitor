
# The application except main(), as a library that the unit tests link too.
add_library(rcssmonitor_core STATIC
  angle_deg.cpp
  ball_painter.cpp
  circle_2d.cpp
  config_dialog.cpp
  disp_holder.cpp
  draw_info_painter.cpp
  field_canvas.cpp
  field_painter.cpp
  game_log_file.cpp
  line_2d.cpp
  log_player.cpp
  log_player_slider.cpp
  main_window.cpp
  monitor_client.cpp
  options.cpp
  overlay_math.cpp
  play_mode_position.cpp
  player_painter.cpp
  player_style.cpp
  player_type_dialog.cpp
  rcg_handler.cpp
  score_board_painter.cpp
  team_graphic.cpp
  team_graphic_painter.cpp
  tool_icon.cpp
  vector_2d.cpp
  )

target_include_directories(rcssmonitor_core
  PUBLIC
#  ${Boost_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src
  )

target_link_libraries(rcssmonitor_core
  PUBLIC
  rcssrcg
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Network
#  Boost::program_options Boost::system
  ZLIB::ZLIB
  )

if(HAVE_3D_VIEW)
  target_sources(rcssmonitor_core
    PRIVATE
    view3d/field_view_3d.cpp
    view3d/ground_texture.cpp
    view3d/interpolate.cpp
    view3d/scene_builder.cpp
    view3d/scene_3d.cpp
    )
  target_include_directories(rcssmonitor_core PUBLIC ${PROJECT_SOURCE_DIR}/src/view3d)
  target_link_libraries(rcssmonitor_core PUBLIC ${QT_3D_VIEW_LIBRARIES})
endif()

if(HAVE_LIBDEFLATE)
  target_include_directories(rcssmonitor_core PRIVATE ${LIBDEFLATE_INCLUDE_DIR})
  target_link_libraries(rcssmonitor_core PRIVATE ${LIBDEFLATE_LIBRARY})
endif()

target_compile_definitions(rcssmonitor_core
  PUBLIC
  HAVE_CONFIG_H
  )

target_compile_options(rcssmonitor_core
  PRIVATE
  -W -Wall
  )

add_executable(rcssmonitor
  main.cpp
  )

target_link_libraries(rcssmonitor
  PRIVATE
  rcssmonitor_core
  )

target_compile_options(rcssmonitor
  PRIVATE
  -W -Wall
  )

set_target_properties(rcssmonitor
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
  )

install(TARGETS rcssmonitor
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
