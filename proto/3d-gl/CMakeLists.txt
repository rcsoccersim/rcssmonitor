# Prototypes of the 3D view (docs/PLAN_3D-VIEW.md, Phase 0, D1), built with
# -DBUILD_PROTO_3D=ON. Not part of the application.

if(QT_VERSION_MAJOR EQUAL 6)
  find_package(Qt6 REQUIRED COMPONENTS OpenGL OpenGLWidgets)
  set(PROTO_GL_LIBS Qt6::OpenGL Qt6::OpenGLWidgets)
else()
  # QOpenGLWidget is in Qt5 Widgets, and QOpenGLFunctions_3_3_Core in Gui
  set(PROTO_GL_LIBS)
endif()

# the log and the model, for both prototypes
add_library(proto3d_common STATIC
  proto_log.cpp
  gltf_rig.cpp
  )
target_include_directories(proto3d_common
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/third_party/cgltf
  )
target_link_libraries(proto3d_common PUBLIC rcssmonitor_core)
target_compile_definitions(proto3d_common
  PUBLIC
  PROTO_MODEL_PATH="${PROJECT_SOURCE_DIR}/resources/models/Mannequin.glb"
  )

# OpenGL 3.3 core in a QOpenGLWidget (the recommended way of D1)
add_executable(proto_gl
  main_gl.cpp
  scene_gl.cpp
  view_gl.cpp
  )
target_link_libraries(proto_gl PRIVATE proto3d_common ${PROTO_GL_LIBS})
set_target_properties(proto_gl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Qt Quick 3D (the other way of D1), with Qt6 only
if(QT_VERSION_MAJOR EQUAL 6)
  find_package(Qt6 QUIET COMPONENTS Qml Quick Quick3D QuickWidgets)
  find_package(Python3 QUIET COMPONENTS Interpreter)
  find_program(BALSAM_EXECUTABLE balsam
    HINTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}" "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}" /usr/lib/qt6/bin)
  if(Qt6Quick3D_FOUND AND Qt6QuickWidgets_FOUND AND Python3_FOUND AND BALSAM_EXECUTABLE)
    set(QUICK3D_DIR ${CMAKE_CURRENT_BINARY_DIR}/quick3d)
    add_custom_command(
      OUTPUT ${QUICK3D_DIR}/Mannequin.qml
      COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/make_quick3d_model.py
              ${BALSAM_EXECUTABLE} ${PROJECT_SOURCE_DIR}/resources/models/Mannequin.glb ${QUICK3D_DIR}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/make_quick3d_model.py ${PROJECT_SOURCE_DIR}/resources/models/Mannequin.glb
      COMMENT "Converting the model for Qt Quick 3D"
      )
    add_custom_command(
      OUTPUT ${QUICK3D_DIR}/Scene.qml
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Scene.qml ${QUICK3D_DIR}/Scene.qml
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Scene.qml
      )
    add_executable(proto_quick3d
      main_quick3d.cpp
      ${QUICK3D_DIR}/Mannequin.qml
      ${QUICK3D_DIR}/Scene.qml
      )
    target_link_libraries(proto_quick3d PRIVATE proto3d_common Qt6::Qml Qt6::Quick Qt6::Quick3D Qt6::QuickWidgets)
    target_compile_definitions(proto_quick3d PRIVATE PROTO_QUICK3D_DIR="${QUICK3D_DIR}")
    set_target_properties(proto_quick3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  else()
    message(STATUS "Qt Quick 3D, balsam or Python 3 not found: proto_quick3d is not built")
  endif()
endif()
