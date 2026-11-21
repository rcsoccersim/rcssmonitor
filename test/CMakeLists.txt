# The tests run with HOME in the build tree, so that they never read or
# write the user's ~/.rcssmonitor.conf (Options reads and writes it). Each
# test removes the settings that an earlier run left there before it starts
# (fresh_settings.cpp), when HOME is RCSSMONITOR_TEST_HOME. The XDG data
# folders are there too, so that they never read the user's figure packs
# (FigurePacks).
set(TEST_HOME ${CMAKE_CURRENT_BINARY_DIR}/home)
file(MAKE_DIRECTORY ${TEST_HOME})

# rcssmonitor_add_test(<name> <libraries>...)
# builds test_<name>.cpp into test_<name> and registers it as <name>.
function(rcssmonitor_add_test name)
  add_executable(test_${name}
    test_${name}.cpp
    fresh_settings.cpp
    )

  target_include_directories(test_${name}
    PRIVATE
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
    )

  target_compile_definitions(test_${name}
    PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    TEST_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    )

  target_link_libraries(test_${name}
    PRIVATE
    ${ARGN}
    Qt${QT_VERSION_MAJOR}::Test
    )

  add_test(NAME ${name} COMMAND test_${name})
  # a test that Qt Test skips (QSKIP, e.g. without OpenGL) is reported as
  # skipped, not as passed
  set_tests_properties(${name}
    PROPERTIES
    ENVIRONMENT "HOME=${TEST_HOME};RCSSMONITOR_TEST_HOME=${TEST_HOME};XDG_DATA_HOME=${TEST_HOME}/share;XDG_DATA_DIRS=${TEST_HOME}/share-system;QT_QPA_PLATFORM=offscreen"
    SKIP_REGULAR_EXPRESSION "\nSKIP   : "
    )
endfunction()

rcssmonitor_add_test(rcg_parser rcssrcg Qt${QT_VERSION_MAJOR}::Core)
rcssmonitor_add_test(rcg_util rcssrcg Qt${QT_VERSION_MAJOR}::Core)
rcssmonitor_add_test(disp_holder rcssmonitor_core)
rcssmonitor_add_test(log_player rcssmonitor_core)
rcssmonitor_add_test(overlay_math rcssmonitor_core)
rcssmonitor_add_test(player_style rcssmonitor_core)
rcssmonitor_add_test(player_selection rcssmonitor_core)
rcssmonitor_add_test(play_mode_position rcssmonitor_core)
rcssmonitor_add_test(config_dialog rcssmonitor_core)
rcssmonitor_add_test(player_select_keys rcssmonitor_core)
rcssmonitor_add_test(referee_menu rcssmonitor_core)
rcssmonitor_add_test(referee_commands rcssmonitor_core)
rcssmonitor_add_test(tool_icon rcssmonitor_core)
rcssmonitor_add_test(toolbar rcssmonitor_core)
rcssmonitor_add_test(tooltip_marker rcssmonitor_core)
rcssmonitor_add_test(player_type_sort Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)
rcssmonitor_add_test(game_log_file rcssmonitor_core)
rcssmonitor_add_test(line_parser rcssrcg Qt${QT_VERSION_MAJOR}::Core)
rcssmonitor_add_test(rcg_types rcssrcg Qt${QT_VERSION_MAJOR}::Core)
rcssmonitor_add_test(message_splitter rcssmonitor_core)
if(HAVE_QT_WEBSOCKETS)
  rcssmonitor_add_test(monitor_transport rcssmonitor_core Qt${QT_VERSION_MAJOR}::WebSockets)
else()
  rcssmonitor_add_test(monitor_transport rcssmonitor_core)
endif()

# only src/view3d/gl calls OpenGL (docs/plan/2026-11-11_3d-view.md §5.3)
rcssmonitor_add_test(gl_isolation Qt${QT_VERSION_MAJOR}::Core)

if(HAVE_3D_VIEW)
  rcssmonitor_add_test(cameras_3d rcssmonitor_core)
  rcssmonitor_add_test(popup_closer rcssmonitor_core)
  rcssmonitor_add_test(figure_actions rcssmonitor_core)
  rcssmonitor_add_test(figure_looks rcssmonitor_core)
  rcssmonitor_add_test(figure_pack rcssmonitor_core)
  rcssmonitor_add_test(figure_uniform rcssmonitor_core)
  rcssmonitor_add_test(figures_3d rcssmonitor_core)
  rcssmonitor_add_test(frame_builder_3d rcssmonitor_core)
  rcssmonitor_add_test(gait_table rcssmonitor_core)
  rcssmonitor_add_test(grass_3d rcssmonitor_core)
  rcssmonitor_add_test(gltf_model rcssmonitor_core)
  rcssmonitor_add_test(fbx_model rcssmonitor_core)
  rcssmonitor_add_test(bvh_model rcssmonitor_core)
  rcssmonitor_add_test(ground_texture rcssmonitor_core)
  rcssmonitor_add_test(guides_3d rcssmonitor_core)
  rcssmonitor_add_test(humanoid rcssmonitor_core)
  rcssmonitor_add_test(interpolate rcssmonitor_core)
  rcssmonitor_add_test(label_atlas rcssmonitor_core)
  rcssmonitor_add_test(meshes_3d rcssmonitor_core)
  rcssmonitor_add_test(overlay_builder rcssmonitor_core)
  rcssmonitor_add_test(picking rcssmonitor_core)
  rcssmonitor_add_test(player_camera rcssmonitor_core)
  rcssmonitor_add_test(scene_3d rcssmonitor_core)
  rcssmonitor_add_test(scene_builder rcssmonitor_core)
  rcssmonitor_add_test(sight rcssmonitor_core)
  rcssmonitor_add_test(scenery_3d rcssmonitor_core)
  rcssmonitor_add_test(stadium_3d rcssmonitor_core)
  rcssmonitor_add_test(stands_3d rcssmonitor_core)
  rcssmonitor_add_test(ball_3d rcssmonitor_core)
  rcssmonitor_add_test(ball_aids_3d rcssmonitor_core)
  rcssmonitor_add_test(ball_flight_3d rcssmonitor_core)
  rcssmonitor_add_test(fps_meter rcssmonitor_core)
  rcssmonitor_add_test(sun_3d rcssmonitor_core)
  rcssmonitor_add_test(view_switch rcssmonitor_core)
endif()

# A benchmark of loading game logs (test/bench_load.cpp). CTest does not run it.
add_executable(bench_load
  bench_load.cpp
  )

target_link_libraries(bench_load
  PRIVATE
  rcssmonitor_core
  )
