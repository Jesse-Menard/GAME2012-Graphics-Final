#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Mesh.h"

// TODO -- Texture.h & Texture.cpp during lab, show result next lexture
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr float SCREEN_ASPECT = SCREEN_WIDTH / (float)SCREEN_HEIGHT;

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void error_callback(int error, const char* description);

GLuint CreateShader(GLint type, const char* path);
GLuint CreateProgram(GLuint vs, GLuint fs);

std::array<int, GLFW_KEY_LAST> gKeysCurr{}, gKeysPrev{};
bool IsKeyDown(int key);
bool IsKeyUp(int key);
bool IsKeyPressed(int key);

void Print(Matrix m);

Vector3 CalcFacingVector3(const Vector3 target, const Vector3 source);

enum Projection : int
{
    ORTHO,  // Orthographic, 2D
    PERSP   // Perspective,  3D
};

struct Pixel
{
    stbi_uc r = 255;
    stbi_uc g = 255;
    stbi_uc b = 255;
    stbi_uc a = 255;
};

int main(void)
{
    glfwSetErrorCallback(error_callback);
    assert(glfwInit() == GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef NDEBUG
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "POTEMKIN NEEDS BUSTING", NULL, NULL);
    glfwMakeContextCurrent(window);
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));
    glfwSetKeyCallback(window, key_callback);

#ifdef NDEBUG
#else
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugOutput, nullptr);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Vertex shaders:
    GLuint vs = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/default.vert");
    GLuint vsSkybox = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/skybox.vert");
    GLuint vsPoints = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/points.vert");
    GLuint vsLines = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/lines.vert");
    GLuint vsVertexPositionColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/vertex_color.vert");
    GLuint vsColorBufferColor = CreateShader(GL_VERTEX_SHADER, "./assets/shaders/buffer_color.vert");
    
    // Fragment shaders:
    GLuint fsSkybox = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/skybox.frag");
    GLuint fsLines = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/lines.frag");
    GLuint fsUniformColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsVertexColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/vertex_color.frag");
    GLuint fsTcoords = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/tcoord_color.frag");
    GLuint fsNormals = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/normal_color.frag");
    GLuint fsTexture = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/texture_color.frag");
    GLuint fsTextureMix = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/texture_color_mix.frag");
    GLuint fsPhong = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/phong.frag");
    
    // Shader programs:
    GLuint shaderUniformColor = CreateProgram(vs, fsUniformColor);
    GLuint shaderVertexPositionColor = CreateProgram(vsVertexPositionColor, fsVertexColor);
    GLuint shaderVertexBufferColor = CreateProgram(vsColorBufferColor, fsVertexColor);
    GLuint shaderPoints = CreateProgram(vsPoints, fsVertexColor);
    GLuint shaderLines = CreateProgram(vsLines, fsLines);
    GLuint shaderTcoords = CreateProgram(vs, fsTcoords);
    GLuint shaderNormals = CreateProgram(vs, fsNormals);
    GLuint shaderTexture = CreateProgram(vs, fsTexture);
    GLuint shaderTextureMix = CreateProgram(vs, fsTextureMix);
    GLuint shaderSkybox = CreateProgram(vsSkybox, fsSkybox);
    GLuint shaderPhong = CreateProgram(vs, fsPhong);

    // See Diffuse 2.png for context
    //Vector2 N = Rotate(Vector2{ 0.0f, 1.0f }, 30.0f * DEG2RAD);
    //Vector2 L = Normalize(Vector2{ 6.0f, 5.0f });
    //float t = Dot(N, L);

    // Our obj file defines tcoords as 0 = bottom, 1 = top, but OpenGL defines as 0 = top 1 = bottom.
    // Flipping our image vertically is the best way to solve this as it ensures a "one-stop" solution (rather than an in-shader solution).
    stbi_set_flip_vertically_on_load(true);

    // Step 1: Load image from disk to CPU
    int texPotWidth = 0;
    int texPotHeight = 0;
    int texPotBaseChannels = 0;
    stbi_uc* pixelsPotBase = stbi_load("./assets/textures/POT_base.png", &texPotWidth, &texPotHeight, &texPotBaseChannels, 0);
    
    // Step 2: Upload image from CPU to GPU
    GLuint texPotBase = GL_NONE;
    glGenTextures(1, &texPotBase);
    glBindTexture(GL_TEXTURE_2D, texPotBase);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texPotWidth, texPotHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsPotBase);
    stbi_image_free(pixelsPotBase);
    //pixelsPotBase = nullptr;

    // Step 1: Load image from disk to CPU
    int texWallWidth = 0;
    int texWallHeight = 0;
    int texWallBaseChannels = 0;
    stbi_uc* pixelsWallBase = stbi_load("./assets/textures/WallDiffuse.jpg", &texWallWidth, &texWallHeight, &texWallBaseChannels, 0);

    // Step 2: Upload image from CPU to GPU
    GLuint texWallBase = GL_NONE;
    glGenTextures(1, &texWallBase);
    glBindTexture(GL_TEXTURE_2D, texWallBase);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWallWidth, texWallHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelsWallBase);
    stbi_image_free(pixelsWallBase);
    pixelsWallBase = nullptr;

    // Step 1: Load image from disk to CPU
    int texWallNormalWidth = 0;
    int texWallNormalHeight = 0;
    int texWallNormalBaseChannels = 0;
    stbi_uc* pixelsWallNormal = stbi_load("./assets/textures/WallNormal.jpg", &texWallNormalWidth, &texWallNormalHeight, &texWallNormalBaseChannels, 0);
    
    // Step 2: Upload image from CPU to GPU
    GLuint texWallNormal = GL_NONE;
    glGenTextures(1, &texWallNormal);
    glBindTexture(GL_TEXTURE_2D, texWallNormal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWallNormalWidth, texWallNormalHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelsWallNormal);
    stbi_image_free(pixelsWallNormal);
    pixelsWallNormal = nullptr;

    int object = 0;
    printf("Object %i\n", object + 1);

    Projection projection = PERSP;
    Vector3 camPos{ 0.0f, 2.0f, 3.0f };
    float fov = 75.0f * DEG2RAD;
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;
    float near = 0.001f; // 1.0 for testing purposes. Usually 0.1f or 0.01f
    float far = 15.0f;
    float camPitch = 0;
    float camYaw = 0;
    float panScale = 0.25f;
    Vector3 frontView = V3_FORWARD;
    float planeRotation = 0;

    // Whether we render the imgui demo widgets
    bool imguiDemo = false;
    bool camToggle = true;

    Mesh potMesh, cubeMesh, sphereMesh, planeMesh;
    CreateMesh(&potMesh, "assets/meshes/potemkin.obj");
    CreateMesh(&cubeMesh, CUBE);
    CreateMesh(&sphereMesh, SPHERE);
    CreateMesh(&planeMesh, PLANE);

    Vector3 planeColor = { 0.3, 0.3, 0.3 };

    Vector3 directionLightPosition = { 10.0f, 10.0f, 10.0f };
    Vector3 directionLightColor = { 1.0f, 1.0f, 1.0f };
    Vector3 directionLightDirection = CalcFacingVector3(Vector3{ 0 }, directionLightPosition);
    int allowDirectionLight = false;

    Vector3 pointLightPosition = { 3.0f, 3.0f, 2.0f };
    Vector3 pointLightColor = { 1.0f, 1.0f, 1.0f };
    int allowPointLight = true;

    Vector3 spotLightPosition = { 0.0f, 3.5f, 0.0f };
    Vector3 spotLightTarget = { 0.0f, 0.0f, 0.0f };
    Vector3 spotLightColor = { 1.0f, 1.0f, 1.0f };
    Vector3 spotLightDirection = CalcFacingVector3(spotLightTarget, spotLightPosition);
    float spotLightFOV = 18;
    float spotLightFOVBlend = 6;
    int allowSpotLight = false;
    int normalToggle = false;

    float lightRadius = 2.5f;
    float lightAngle = 90.0f * DEG2RAD;

    float ambientFactor = 0.25f;
    float diffuseFactor = 1.0f;
    float specularPower = 64.0f;
    float attenuationScale = 0.0f;

    // Render looks weird cause this isn't enabled, but its causing unexpected problems which I'll fix soon!
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    float timePrev = glfwGetTime();
    float timeCurr = glfwGetTime();
    float dt = 0.0f;

    double pmx = 0.0, pmy = 0.0, mx = 0.0, my = 0.0;

    // Initialize mouse position
    glfwGetCursorPos(window, &mx, &my);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        float time = glfwGetTime();
        timePrev = time;

        pmx = mx; pmy = my;
        glfwGetCursorPos(window, &mx, &my);
        if (camToggle)
        {
            // Looping cam
            if (mx > 1280)
            {
                glfwSetCursorPos(window, 0.0, my);

                mx = 0.0;
                pmx = 0.0;
            }
            else if (mx < 0)
            {
                glfwSetCursorPos(window, 1280.0, my);

                mx = 1280.0;
                pmx = 1280.0;
            }
            if (my > 720)
            {
                glfwSetCursorPos(window, mx, 0.0);

                my = 0.0;
                pmy = 0.0;
            }
            else if (my < 0)
            {
                glfwSetCursorPos(window, mx, 720.0);

                my = 720.0;
                pmy = 720.0;
            }
        }
        Vector2 mouseDelta = { mx - pmx, my - pmy };

        // Change object when space is pressed
        if (IsKeyPressed(GLFW_KEY_TAB))
        {
            ++object %= 5;
            printf("Object %i\n", object + 1);
        }

        if (IsKeyPressed(GLFW_KEY_I))
            imguiDemo = !imguiDemo;
        
        if (IsKeyPressed(GLFW_KEY_C))
            camToggle = !camToggle;

        if (!camToggle)
        {
            mouseDelta = V2_ZERO;
        }

        camPitch -= mouseDelta.y * panScale;
        if (camPitch > 88)
        {
            camPitch = 88;
        }
        else if (camPitch < -88)
        {
            camPitch = -88;
        }

        camYaw -= mouseDelta.x * panScale;
        Matrix camRotation = ToMatrix(FromEuler(camPitch * DEG2RAD, camYaw * DEG2RAD, 0.0f));

        Vector3 camX = { camRotation.m0, camRotation.m1, camRotation.m2 };
        Vector3 camY = { camRotation.m4, camRotation.m5, camRotation.m6 };
        Vector3 camZ = { camRotation.m8, camRotation.m9, camRotation.m10 };

        frontView = Multiply(V3_FORWARD, camRotation);
        
        // Slows cam when shifted
        float camTranslateValue = 0.01;
        if (IsKeyDown(GLFW_KEY_LEFT_SHIFT))
        {
            camTranslateValue = 0.002;
        }

        if (IsKeyDown(GLFW_KEY_W))
        {
            // forwards
            camPos -= camZ * camTranslateValue;
        }
        if (IsKeyDown(GLFW_KEY_S))
        {
            // back
            camPos += camZ * camTranslateValue;
        }
        if (IsKeyDown(GLFW_KEY_A))
        {
            // left
            camPos -= camX * camTranslateValue;
        }
        if (IsKeyDown(GLFW_KEY_D))
        {
            // right
            camPos += camX * camTranslateValue;
        }
        if (IsKeyDown(GLFW_KEY_E))
        {
            // up
            camPos += camY * camTranslateValue;
        }
        if (IsKeyDown(GLFW_KEY_Q))
        {
            // down
            camPos -= camY * camTranslateValue;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Matrix rotationX = RotateX(100.0f * time * DEG2RAD);
        Matrix rotationY = RotateY(100.0f * time * DEG2RAD);

        Matrix normal = MatrixIdentity();
        Matrix world = MatrixIdentity();
        Matrix view = LookAt(camPos, camPos - frontView, V3_UP);
        Matrix proj = projection == ORTHO ? Ortho(left, right, bottom, top, near, far) : Perspective(fov, SCREEN_ASPECT, near, far);
        Matrix mvp = MatrixIdentity();

        GLuint u_color = -2;
        GLint u_normal = -2;
        GLint u_world = -2;
        GLint u_mvp = -2;
        GLint u_tex = -2;
        GLint u_normalMap = -2;
        GLint u_textureSlots[2]{ -2, -2 };
        GLuint shaderProgram = GL_NONE;
        GLuint texture = texPotBase;
        GLint u_t = GL_NONE;

        GLint u_cameraPosition = -2;
        GLint u_lightPosition = -2;
        GLint u_directionLightColor = -2;
        GLint u_lightDirection = -2;
        GLint u_lightColor = -2;
        GLint u_pointLightPosition = -2;
        GLint u_pointLightColor = -2;
        GLint u_spotLightPosition = -2;
        GLint u_spotLightDirection = -2;
        GLint u_spotLightColor = -2;
        GLint u_lightRadius = -2;
        
        GLint u_ambientFactor = -2;
        GLint u_diffuseFactor = -2;
        GLint u_SpecularPower = -2;
        GLint u_attenuationScale = -2;

        GLint u_allowDirectionLight = -2;
        GLint u_allowPointLight = -2;
        GLint u_allowSpotLight = -2;
        GLint u_fov = -2;
        GLint u_fovBlend = -2;

        GLint u_normalToggle = -2;

        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------

        pointLightPosition = Multiply(pointLightPosition, RotateZ(sin(dt * 4)));
        world = Translate(-0.5, -0.5, 0) * RotateY(DEG2RAD * planeRotation) * Scale(10, 10, 10);//RotateX(DEG2RAD * 90) * Translate(-0.5f, 0.0f, -0.5f) * Scale(10, 1, 10);

        shaderProgram = shaderPhong;
        glUseProgram(shaderProgram);
        mvp = world * view * proj;
        
        normal = Transpose(Invert(world));

        
        u_normal = glGetUniformLocation(shaderProgram, "u_normal");
        u_world = glGetUniformLocation(shaderProgram, "u_world");
        u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
        
        u_cameraPosition = glGetUniformLocation(shaderProgram, "u_cameraPosition");
        u_directionLightColor = glGetUniformLocation(shaderProgram, "u_directionLightColor");
        u_lightDirection = glGetUniformLocation(shaderProgram, "u_lightDirection");
        u_pointLightPosition = glGetUniformLocation(shaderProgram, "u_pointLightPosition");
        u_pointLightColor = glGetUniformLocation(shaderProgram, "u_pointLightColor");
        u_spotLightPosition = glGetUniformLocation(shaderProgram, "u_spotLightPosition");
        u_spotLightColor = glGetUniformLocation(shaderProgram, "u_spotLightColor");
        u_spotLightDirection = glGetUniformLocation(shaderProgram, "u_spotLightDirection");
        u_lightRadius = glGetUniformLocation(shaderProgram, "u_lightRadius");
        
        u_ambientFactor = glGetUniformLocation(shaderProgram, "u_ambientFactor");
        u_diffuseFactor = glGetUniformLocation(shaderProgram, "u_diffuseFactor");
        u_SpecularPower = glGetUniformLocation(shaderProgram, "u_specularPower");
        u_attenuationScale = glGetUniformLocation(shaderProgram, "u_attenuationScale");
        u_normalToggle = glGetUniformLocation(shaderProgram, "u_normalToggle");
        
        u_allowDirectionLight = glGetUniformLocation(shaderProgram, "u_allowDirectionLight");
        u_allowPointLight = glGetUniformLocation(shaderProgram, "u_allowPointLight");
        u_allowSpotLight = glGetUniformLocation(shaderProgram, "u_allowSpotLight");
        u_fov = glGetUniformLocation(shaderProgram, "u_fov");
        u_fovBlend = glGetUniformLocation(shaderProgram, "u_fovBlend");

        u_tex = glGetUniformLocation(shaderProgram, "u_tex");
        
        glUniformMatrix3fv(u_normal, 1, GL_FALSE, ToFloat9(normal).v);
        glUniformMatrix4fv(u_world, 1, GL_FALSE, ToFloat16(world).v);
        glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
        
        glUniform3fv(u_cameraPosition, 1, &camPos.x);
        glUniform3fv(u_lightDirection, 1, &directionLightDirection.x);
        glUniform3fv(u_directionLightColor, 1, &directionLightColor.x);
        glUniform3fv(u_pointLightPosition, 1, &pointLightPosition.x);
        glUniform3fv(u_pointLightColor, 1, &pointLightColor.x);
        glUniform3fv(u_spotLightPosition, 1, &spotLightPosition.x);
        glUniform3fv(u_spotLightDirection, 1, &spotLightDirection.x);
        glUniform3fv(u_spotLightColor, 1, &spotLightColor.x);
        glUniform1f(u_lightRadius, lightRadius);
        
        glUniform1f(u_ambientFactor, ambientFactor);
        glUniform1f(u_diffuseFactor, diffuseFactor);
        glUniform1f(u_SpecularPower, specularPower);
        glUniform1f(u_attenuationScale, attenuationScale);
        glUniform1i(u_normalToggle, normalToggle);

        glUniform1i(u_allowDirectionLight, allowDirectionLight);
        glUniform1i(u_allowPointLight, allowPointLight);
        glUniform1i(u_allowSpotLight, allowSpotLight);

        //  Vector3 displacement = Normalize(camPos - spotLightPosition); //(position.x - u_spotLightPosition.x, position.y - u_spotLightPosition.y, position.z - u_spotLightPosition.z);
        //  testDOT = Dot(displacement, spotLightDirection);
        //  testAngle = RAD2DEG * acos(testDOT);
        //  std::cout << testAngle << std::endl;

        glUniform1f(u_fov, spotLightFOV);
        glUniform1f(u_fovBlend, spotLightFOVBlend);

        u_normalMap = glGetUniformLocation(shaderProgram, "u_normalMap");
        u_tex = glGetUniformLocation(shaderProgram, "u_tex");

        glUniform1i(u_normalMap, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texWallNormal);

        glUniform1i(u_tex, 1);
        glActiveTexture(GL_TEXTURE0 + 1); // this caught me up for so long.. lol
        glBindTexture(GL_TEXTURE_2D, texWallBase);

        DrawMesh(planeMesh);

        //  glUniform1i(u_tex, 0);
        //  glActiveTexture(GL_TEXTURE0);
        //  glBindTexture(GL_TEXTURE_2D, texPotBase);

        //  DrawMesh(potMesh);
        
        //shaderProgram = shaderPhong;
        //glUseProgram(shaderProgram);


        if (allowDirectionLight)
        {
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            world = Translate(directionLightPosition);
            mvp = world * view * proj;
            //u_world = glGetUniformLocation(shaderProgram, "u_world");
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            u_color = glGetUniformLocation(shaderProgram, "u_color");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glUniform3fv(u_color, 1, &directionLightColor.x);
            DrawMesh(sphereMesh);
        }

        if (allowPointLight)
        {
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            world = Scale(V3_ONE * lightRadius) * Translate(pointLightPosition);
            mvp = world * view * proj;
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            u_color = glGetUniformLocation(shaderProgram, "u_color");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glUniform3fv(u_color, 1, &pointLightColor.x);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            DrawMesh(sphereMesh);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (allowSpotLight)
        {
            shaderProgram = shaderUniformColor;
            glUseProgram(shaderProgram);
            world = Scale(V3_ONE / 2) * Translate(spotLightPosition);
            mvp = world * view * proj;
            u_mvp = glGetUniformLocation(shaderProgram, "u_mvp");
            u_color = glGetUniformLocation(shaderProgram, "u_color");
            glUniformMatrix4fv(u_mvp, 1, GL_FALSE, ToFloat16(mvp).v);
            glUniform3fv(u_color, 1, &spotLightColor.x);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            DrawMesh(sphereMesh);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }         


        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (imguiDemo)
            ImGui::ShowDemoWindow();
        else
        {
            ImGui::SliderFloat3("Camera Position", &camPos.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Directional Light Position", &directionLightPosition.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Directional Light Color", &directionLightColor.x, 0.0f, 1.0f);
            ImGui::NewLine();
            ImGui::SliderFloat3("Point Light Position", &pointLightPosition.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Point Light Color", &pointLightColor.x, 0.0f, 1.0f);
            ImGui::NewLine();
            ImGui::SliderFloat3("Spot Light Position", &spotLightPosition.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Spot Light Color", &spotLightColor.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Spot Light FOV", &spotLightFOV, 0.0f, 180.0f);
            ImGui::SliderFloat("Spot Light FOV Blend", &spotLightFOVBlend, 0.0f, 180.0f - spotLightFOV);
            ImGui::NewLine();
            ImGui::SliderFloat("Light Radius", &lightRadius, 0.25f, 5.0f);
            ImGui::SliderAngle("Light Angle", &lightAngle);
            ImGui::Checkbox("Direction Light", (bool*)&allowDirectionLight); ImGui::SameLine();
            ImGui::Checkbox("Point Light", (bool*)&allowPointLight); ImGui::SameLine();
            ImGui::Checkbox("Spot Light", (bool*) & allowSpotLight);
            ImGui::NewLine();

            ImGui::SliderFloat("Plane Rotation", &planeRotation, 0.0f, 360.0f);
            ImGui::NewLine();

            ImGui::SliderFloat("Ambient", &ambientFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Diffuse", &diffuseFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular", &specularPower, 8.0f, 256.0f);
            ImGui::SliderFloat("Attenuation", &attenuationScale, 0.0f, 1.0f);
            ImGui::Checkbox("Normal Toggle", (bool*)&normalToggle);

            ImGui::RadioButton("Orthographic", (int*)&projection, 0); ImGui::SameLine();
            ImGui::RadioButton("Perspective", (int*)&projection, 1);

            ImGui::SliderFloat("Near", &near, 0.0f, 10.0f);
            ImGui::SliderFloat("Far", &far,0.0f, 100.0f);
            if (projection == ORTHO)
            {
                ImGui::SliderFloat("Left", &left, -1.0f, -10.0f);
                ImGui::SliderFloat("Right", &right, 1.0f, 10.0f);
                ImGui::SliderFloat("Top", &top, 1.0f, 10.0f);
                ImGui::SliderFloat("Bottom", &bottom, -1.0f, -10.0f);
            }
            else if (projection == PERSP)
            {
                ImGui::SliderAngle("FoV", &fov, 10.0f, 90.0f);
            }
        }
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        timeCurr = glfwGetTime();
        dt = timeCurr - timePrev;

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll and process events */
        memcpy(gKeysPrev.data(), gKeysCurr.data(), GLFW_KEY_LAST * sizeof(int));
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Disable repeat events so keys are either up or down
    if (action == GLFW_REPEAT) return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    gKeysCurr[key] = action;

    // Key logging
    //const char* name = glfwGetKeyName(key, scancode);
    //if (action == GLFW_PRESS)
    //    printf("%s\n", name);
}

void error_callback(int error, const char* description)
{
    printf("GLFW Error %d: %s\n", error, description);
}

// Compile a shader
GLuint CreateShader(GLint type, const char* path)
{
    GLuint shader = GL_NONE;
    try
    {
        // Load text file
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path);

        // Interpret the file as a giant string
        std::stringstream stream;
        stream << file.rdbuf();
        file.close();

        // Verify shader type matches shader file extension
        const char* ext = strrchr(path, '.');
        switch (type)
        {
        case GL_VERTEX_SHADER:
            assert(strcmp(ext, ".vert") == 0);
            break;

        case GL_FRAGMENT_SHADER:
            assert(strcmp(ext, ".frag") == 0);
            break;
        default:
            assert(false, "Invalid shader type");
            break;
        }

        // Compile text as a shader
        std::string str = stream.str();
        const char* src = str.c_str();
        shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);

        // Check for compilation errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Shader failed to compile: \n" << infoLog << std::endl;
        }
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "Shader (" << path << ") not found: " << e.what() << std::endl;
        assert(false);
    }

    return shader;
}

// Combine two compiled shaders into a program that can run on the GPU
GLuint CreateProgram(GLuint vs, GLuint fs)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        program = GL_NONE;
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    return program;
}

// Graphics debug callback
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

bool IsKeyDown(int key)
{
    return gKeysCurr[key] == GLFW_PRESS;
}

bool IsKeyUp(int key)
{
    return gKeysCurr[key] == GLFW_RELEASE;
}

bool IsKeyPressed(int key)
{
    return gKeysPrev[key] == GLFW_PRESS && gKeysCurr[key] == GLFW_RELEASE;
}

void Print(Matrix m)
{
    printf("%f %f %f %f\n", m.m0, m.m4, m.m8, m.m12);
    printf("%f %f %f %f\n", m.m1, m.m5, m.m9, m.m13);
    printf("%f %f %f %f\n", m.m2, m.m6, m.m10, m.m14);
    printf("%f %f %f %f\n\n", m.m3, m.m7, m.m11, m.m15);
}

Vector3 CalcFacingVector3(const Vector3 target, const Vector3 source)
{
    return Normalize(Subtract(target, source));
}