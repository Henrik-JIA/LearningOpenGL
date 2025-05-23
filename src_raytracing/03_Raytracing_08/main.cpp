#include <iostream>
#include <memory>

#include <glad/glad.h>
//
#include <GLFW/glfw3.h>
//
#include <glm/glm.hpp>
//
#include <tool/Gui.h>
//
#include "constant.h"
#include "rectangle.h"
#include "renderer.h"
#include "shader.h"

std::unique_ptr<Renderer> renderer;

void handleInput(GLFWwindow* window, const ImGuiIO& io) {
  // Close Application
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  // Clear Render
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    renderer->clear();
  }

  // Camera Movement
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS &&
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
    renderer->moveCamera(glm::vec3(0, 0, -io.MouseDelta.y));
  } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS &&
             glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) ==
                 GLFW_PRESS) {
    renderer->moveCamera(glm::vec3(io.MouseDelta.x, io.MouseDelta.y, 0));
  }
  // Camera Orbit
  else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
    const float orbitSpeed = 0.01f;
    renderer->orbitCamera(orbitSpeed * io.MouseDelta.y,
                          orbitSpeed * io.MouseDelta.x);
  }
}

int main() {
  // init glfw
  if (!glfwInit()) {
    std::cerr << "failed to initialize GLFW" << std::endl;
  }

  // set glfw error callback
  glfwSetErrorCallback([]([[maybe_unused]] int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
    std::exit(EXIT_FAILURE);
  });

  // setup window and context
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
  GLFWwindow* window =
      glfwCreateWindow(1280, 720, "GLSL CornellBox", nullptr, nullptr);
  if (!window) {
    std::cerr << "failed to create window" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);

  // disable v-sync
  glfwSwapInterval(0);

  // initialize glad
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "failed to initialize glad" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  // setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // setup renderer
  renderer = std::make_unique<Renderer>(512, 512);

  // main app loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Renderer");
    {
      static int resolution[2] = {static_cast<int>(renderer->getWidth()),
                                  static_cast<int>(renderer->getHeight())};
      if (ImGui::InputInt2("Resolution", resolution)) {
        renderer->resize(resolution[0], resolution[1]);
      }

      static RenderMode mode = renderer->getRenderMode();
      if (ImGui::Combo("Layer", reinterpret_cast<int*>(&mode),
                       "Render\0Normal\0Depth\0Albedo\0UV\0\0")) {
        renderer->setRenderMode(mode);
      }

      static Integrator integrator = renderer->getIntegrator();
      if (ImGui::Combo("Integrator", reinterpret_cast<int*>(&integrator),
                       "PT\0PTNEE\0\0")) {
        renderer->setIntegrator(integrator);
      }

      static SceneType scene_type = renderer->getSceneType();
      if (ImGui::Combo("Scene", reinterpret_cast<int*>(&scene_type),
                       "Original\0Sphere\0Indirect\0")) {
        renderer->setSceneType(scene_type);
      }

      ImGui::Text("Samples: %d", renderer->getSamples());

      glm::vec3 camPos = renderer->getCameraPosition();
      ImGui::Text("Camera Position: (%.3f, %.3f, %.3f)", camPos.x, camPos.y,
                  camPos.z);

      static float fov = renderer->getCameraFOV() / PI * 180.0f;
      if (ImGui::InputFloat("FOV", &fov)) {
        renderer->setFOV(fov / 180.0f * PI);
      }

      ImGui::Text("FPS: %.1f", io.Framerate);

      ImGui::Separator();

      ImGui::Text("Camera Rotate: [MMB Drag]");
      ImGui::Text("Camera Move: [LShift] + [MMB Drag]");
      ImGui::Text("Camera Zoom: [LCtrl] + [MMB Drag]");
    }
    ImGui::End();

    // Handle Input
    handleInput(window, io);

    // Rendering
    glClear(GL_COLOR_BUFFER_BIT);

    renderer->render();

    // ImGui Rendering
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // exit
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  renderer->destroy();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}