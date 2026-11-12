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

