add_library(opengl STATIC
  src/gl_context.c
)
target_include_directories(opengl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(opengl PUBLIC graph)
target_compile_features(opengl PUBLIC c_std_99)
