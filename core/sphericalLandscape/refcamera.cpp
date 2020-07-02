#include "refcamera.h"

#include <glad/glad.h>

#include "cmake_source_dir.h"

Shader refCamera::shader;
bool refCamera::drawFrustrum = false;

static unsigned int cubeVAO=0, cubeVBO=0;

void refCamera::render_frustrum() const
{

    glm::vec2 s(tanf(0.5f*Zoom) * Aspect,
                tanf(0.5f*Zoom));


    float vertices[] = {
        // x-dir
        -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
        -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right

        -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
        -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        // y dir
        -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right

        1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
        1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
        1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
        1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
        // z dir
        1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right

        -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,  // bottom-left
        -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right

        // Coordinate rays
        0,0,0,0,0,0,0,0,
        1,0,0,0,0,0,0,0,

        0,0,0,0,0,0,0,0,
        0,1,0,0,0,0,0,0,

        0,0,0,0,0,0,0,0,
        0,0,1,0,0,0,0,0
    };

    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_LINES, 0, sizeof(vertices)/sizeof(float));
    glBindVertexArray(0);
}
void refCamera::draw_frustrum() const
{
    shader.use();

    // translate, then rotate locally
    glm::mat4 model = glm::translate(glm::mat4(1), Position) * glm::mat4(glm::quat(rotation));

    // Set a readable cliping range
    //Near = 0.1f; Far = 10.0f;

    // Draw frustrum
    shader.use();
    shader.setMat4("projection_view", reference.GetFrustumMatrix());
    shader.setMat4("model", model );
    render_frustrum();
}

#include "imgui.h"

void refCamera::draw_frustrum_gui()
{
    // Draw frustrum
    ImGui::Separator();
    ImGui::Text("refCamera::panel");
    ImGui::Checkbox("Draw camera frustrum",&drawFrustrum);
    if (drawFrustrum) {this->draw_frustrum();}
    if(ImGui::Button("update camera vectors")) {this->updateCameraVectors();}
}

void forwarder(void* context)
{
    static_cast<refCamera*>(context)->draw_frustrum_gui();
}

void refCamera::gui_interface()
{
    // pass functor into base class Camera
    dynamic_cast<Camera*>(this)->gui_interface(&forwarder, this);
}
