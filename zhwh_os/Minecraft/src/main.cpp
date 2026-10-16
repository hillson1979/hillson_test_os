#include "graph/graph.hpp"
#include "minecraft/world.hpp"

#include <iostream>

int main() {
  minecraft::World world;
  std::cout << "Hillson Minecraft-style voxel engine\n"
            << "starter chunk solid blocks: " << world.starterChunk().solidBlockCount() << "\n";

  graph::Application app({1280, 720, "Hillson Minecraft", true});
  if (!app.initialize()) {
    std::cerr << "OpenGL window unavailable. Install GLFW to run the graphical client.\n";
    return 0;
  }
  while (!app.shouldClose()) {
    app.pollEvents();
    app.beginFrame(0.42f, 0.68f, 0.92f);
    app.endFrame();
  }
  return 0;
}
