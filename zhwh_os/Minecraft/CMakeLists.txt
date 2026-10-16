add_executable(minecraft
  src/main.cpp
  src/world.cpp
)
target_include_directories(minecraft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(minecraft PRIVATE opengl graph)
