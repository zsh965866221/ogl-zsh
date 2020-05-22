#include "middle/application.h"
#include "middle/buffer.h"
#include "middle/camera_simple.h"
#include "middle/compute.h"
#include "middle/program.h"
#include "middle/shader.h"
#include "middle/texture.h"
#include "middle/utils.h"

#include <iostream>

namespace zsh = zexz;

class SimpleApplication: public zsh::middle::Application {
private:
  // Struct
  struct UI {
    UI(): 
      Swivel(0.0f),
      Tilt(0.0f),
      Distance(0.0f),
      Specular(false) {}
      float Swivel;
      float Tilt;
      float Distance;
      bool Specular;
  };
  struct Data {
    Data():
      program(nullptr),
      texture(0),
      image_width(0),
      image_height(0),
      image_channels(0),
      VAO(0),
      VBO(0),
      IBO(0) {
    }
    std::unique_ptr<zexz::middle::Program> program;
    GLuint texture;
    int image_width;
    int image_height;
    int image_channels;
    GLuint VAO;
    GLuint VBO;
    GLuint IBO;
  };

  struct CameraStruct {
    CameraStruct():
      distance(0.0f),
      mousePosition(glm::vec2(0.0f, 0.0f)),
      midPressed(false),
      mouseMidStart(glm::vec2(0.0f, 0.0f)),
      mouseMidOffset(glm::vec2(0.0f, 0.0f)),
      mouseMidOffsetTmp(glm::vec2(0.0f, 0.0f)) {
    }

    float distance;
    glm::vec2 mousePosition;
    bool midPressed;
    glm::vec2 mouseMidStart;
    glm::vec2 mouseMidOffset;
    glm::vec2 mouseMidOffsetTmp;
  };

public:
  SimpleApplication(): Application(
    "Simple",
    1230,
    900
  ) {}

public:
  bool onInit() {
    // args
    std::string path_image = resource_dir + "/images/f.jpg";
    std::string path_vertex = resource_dir + "/shaders/Compute/template.vsh";
    std::string path_fragment = resource_dir + "/shaders/Compute/template.fsh";
    // texture
    data.texture = zexz::middle::load_texture(
      path_image,
      &(data.image_width), 
      &(data.image_height),
      &(data.image_channels));
    // shader
    zexz::middle::Shader shader_vertex(zexz::middle::ReadText(path_vertex), zexz::middle::ShaderType_Vertex);
    shader_vertex.complie();

    zexz::middle::Shader shader_fragment(zexz::middle::ReadText(path_fragment), zexz::middle::ShaderType_Fragment);
    shader_fragment.complie();
    // program
    data.program.reset(new zexz::middle::Program());
    data.program->attach(shader_vertex);
    data.program->attach(shader_fragment);
    data.program->link();
    // VAO VBO IBO
    float vertices[] = {
      // positions                 // texture coords
       1.0f,  1.0f, 0.0f, 1.0f,    1.0f, 1.0f, // top right
       1.0f, -1.0f, 0.0f, 1.0f,    1.0f, 0.0f, // bottom right
      -1.0f, -1.0f, 0.0f, 1.0f,    0.0f, 0.0f, // bottom left
      -1.0f,  1.0f, 0.0f, 1.0f,    0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {
      0, 1, 3,
      1, 2, 3
    };
    glGenVertexArrays(1, &(data.VAO));
    glGenBuffers(1, &(data.VBO));
    glGenBuffers(1, &(data.IBO));

    glBindVertexArray(data.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, data.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // camera
    camera.reset(new zexz::middle::SimpleCamera(
      glm::vec3(0.0f, 0.0f, -2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f),
      window_width, 
      window_height
    ));

    // compute shader
    int work_group_count[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &(work_group_count[0]));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &(work_group_count[1]));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &(work_group_count[2]));
    std::cout << work_group_count[0] << "," << work_group_count[1] << "," << work_group_count[2] << std::endl;
    
    int work_group_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_size[2]);
    std::cout << work_group_size[0] << "," << work_group_size[1] << "," << work_group_size[2] << std::endl;

    int work_group_invocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_group_invocations);
    std::cout << work_group_invocations << std::endl;

    std::string path_compute = resource_dir + "/shaders/Compute/compute.csh";
    zexz::middle::Shader shader_compute(
      zexz::middle::ReadText(path_compute),
      zexz::middle::ShaderType_Compute
    );
    shader_compute.complie();

    zexz::middle::ComputeProgram program_compute;
    program_compute.attach(shader_compute);
    program_compute.link();

    // buffer
    zexz::middle::GPUBuffer<float> inBuffer(zexz::middle::GPUBufferType_SHADER_STORAGE, 4);
    inBuffer.map([&] (const auto* self, auto* data) {
      data[0] = 1.0f;
      data[3] = 4.0f;
    });
    zexz::middle::GPUBuffer<float> outBuffer(zexz::middle::GPUBufferType_SHADER_STORAGE, 4);

    program_compute.use();
    program_compute.bindBuffer(inBuffer, 0);
    program_compute.bindBuffer(outBuffer, 1);
    program_compute.compute(1, 1, 1);
    program_compute.unuse();

    outBuffer.map([&](const auto* self, auto* data) {
      std::cout << data[0] << ", "
                << data[1] << ", "
                << data[2] << ", "
                << data[3] << std::endl;
    });
    return true;
  }

  bool onGUI() {
    ImGui::Begin("Basic 3D");
    ImGui::SliderFloat("Swivel", &(ui.Swivel), -180.0, 180.0);
    ImGui::SliderFloat("Tilt", &(ui.Tilt), -180.0, 180.0);
    ImGui::DragFloat(
      "Distance to Image", 
      &(ui.Distance)
    );
    ImGui::Checkbox("Specular Highlight", &(ui.Specular));
    ImGui::End();

    // help
    ImGui::Begin("Help");
    ImGui::Text("1. Mouse Scroll for Scale.");
    ImGui::Text("2. Mouse Middle Button for Move.");
    ImGui::End();

    return true;
  }

  bool onDraw() {
    CHECK_NOTNULL(data.program);

    { // draw main
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, window_width, window_height);
      glEnable(GL_DEPTH_TEST);
      glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      data.program->use();

      // MVP
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::scale(model, glm::vec3(1.0, (float)(data.image_height)/ (float)(data.image_width), 1.0));
      model = glm::rotate(model, animation.timer.time * 1.0f, glm::vec3(0.0, 0.0, 1.0));

      data.program->setMat4("uMVPMatrix", camera->projection * camera->view * model);

      // texture matrix
      glm::mat4 textureMat = glm::mat4(1.0f);
      textureMat = glm::translate(textureMat, glm::vec3(0.5f, 0.5f, 0.0f));
      textureMat = glm::rotate(textureMat, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
      textureMat = glm::rotate(textureMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      textureMat = glm::translate(textureMat, glm::vec3(-0.5f, -0.5f, 0.0f));
      data.program->setMat4("uTexuvMat1", textureMat);

      data.program->setTexture("uBitmap1", data.texture, 0);

      data.program->setFloat("uTextureWidth1", (float)data.image_width);
      data.program->setFloat("uTextureHeight1", (float)data.image_height);

      // uniform
      data.program->setFloat("uTilt", ui.Tilt);
      data.program->setFloat("uSwivel", ui.Swivel);
      data.program->setFloat("uDistance", ui.Distance);
      data.program->setBool("uSpecular", ui.Specular);

      glBindVertexArray(data.VAO);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      data.program->unuse();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    return true;
  }

  bool onDestroy() {
    glDeleteVertexArrays(1, &(data.VAO));
    glDeleteBuffers(1, &(data.VBO));
    glDeleteBuffers(1, &(data.IBO));

    return true;
  }

  bool onEvent(const SDL_Event* event) {
    camera->onEvent(event);

    return true;
  }

private:
  UI ui;
  Data data;
  std::unique_ptr<zsh::middle::SimpleCamera> camera;
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = true;

  SimpleApplication app;
  app.init();
  app.run();
  return 0;
}
