add_library(opengl STATIC
  src/gl_context.c
  ../user/graph_user.c
)
target_include_directories(opengl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../user
)
target_compile_features(opengl PUBLIC c_std_99)
