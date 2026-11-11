# The tests run with HOME in the build tree, so that they never read or
# write the user's ~/.rcssmonitor.conf (Options reads and writes it).
set(TEST_HOME ${CMAKE_CURRENT_BINARY_DIR}/home)
file(MAKE_DIRECTORY ${TEST_HOME})

# rcssmonitor_add_test(<name> <libraries>...)
# builds test_<name>.cpp into test_<name> and registers it as <name>.
function(rcssmonitor_add_test name)
  add_executable(test_${name}
    test_${name}.cpp
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
    )

  target_link_libraries(test_${name}
    PRIVATE
    ${ARGN}
    Qt5::Test
    )

  add_test(NAME ${name} COMMAND test_${name})
  set_tests_properties(${name}
    PROPERTIES
    ENVIRONMENT "HOME=${TEST_HOME};QT_QPA_PLATFORM=offscreen"
    )
endfunction()

rcssmonitor_add_test(rcg_parser rcssrcg Qt5::Core)
rcssmonitor_add_test(rcg_util rcssrcg Qt5::Core)
rcssmonitor_add_test(disp_holder rcssmonitor_core)
rcssmonitor_add_test(overlay_math rcssmonitor_core)
rcssmonitor_add_test(play_mode_position rcssmonitor_core)
rcssmonitor_add_test(config_dialog rcssmonitor_core)
rcssmonitor_add_test(player_select_keys rcssmonitor_core)
rcssmonitor_add_test(play_mode_actions rcssmonitor_core)
rcssmonitor_add_test(player_type_sort Qt5::Core Qt5::Gui)
rcssmonitor_add_test(game_log_file rcssmonitor_core)
rcssmonitor_add_test(line_parser rcssrcg Qt5::Core)
rcssmonitor_add_test(rcg_types rcssrcg Qt5::Core)

# A benchmark of loading game logs (test/bench_load.cpp). CTest does not run it.
add_executable(bench_load
  bench_load.cpp
  )

target_link_libraries(bench_load
  PRIVATE
  rcssmonitor_core
  )
