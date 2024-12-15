#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Mesh.h"
#include "RenderObject.h"
#include "Light.h"

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
void LoadTexture(GLuint* texture, const char* filename, bool hasAlpha = false);

std::vector<Light> lights;
std::vector<RenderObject> objects;

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
    
    // Fragment shaders:
    GLuint fsUniformColor = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/uniform_color.frag");
    GLuint fsPhong = CreateShader(GL_FRAGMENT_SHADER, "./assets/shaders/phong.frag");
    
    // Shader programs:
    GLuint shaderUniformColor = CreateProgram(vs, fsUniformColor);
    GLuint shaderPhong = CreateProgram(vs, fsPhong);

    // Our obj file defines tcoords as 0 = bottom, 1 = top, but OpenGL defines as 0 = top 1 = bottom.
    // Flipping our image vertically is the best way to solve this as it ensures a "one-stop" solution (rather than an in-shader solution).
    stbi_set_flip_vertically_on_load(true);

    // Wall textures from https://polyhaven.com/a/brick_wall_001
    GLuint texWallBase = GL_NONE;
    LoadTexture(&texWallBase, "./assets/textures/WallDiffuse.jpg");
    GLuint texWallNormal = GL_NONE;
    LoadTexture(&texWallNormal, "./assets/textures/WallNormal.jpg");
    GLuint texWallSpec = GL_NONE;
    LoadTexture(&texWallSpec, "./assets/textures/WallSpec.jpg");

    GLuint texStone = GL_NONE;
    LoadTexture(&texStone, "./assets/textures/Stone.jpg");
    GLuint texStoneNormal = GL_NONE;
    LoadTexture(&texStoneNormal, "./assets/textures/StoneNormal.jpg");

    // Floor from https://polyhaven.com/a/recycled_brick_floor
    GLuint floorTex = GL_NONE;
    LoadTexture(&floorTex, "./assets/textures/Floor.jpg");
    GLuint floorNormal = GL_NONE;
    LoadTexture(&floorNormal, "./assets/textures/FloorNormal.jpg");

    GLuint horseTex = GL_NONE;
    LoadTexture(&horseTex, "./assets/textures/horse.png");
    GLuint horseNormal = GL_NONE;
    LoadTexture(&horseNormal, "./assets/textures/horseNormal.jpg");

    GLuint elephantTex = GL_NONE;
    LoadTexture(&elephantTex, "./assets/textures/elephant.jpg");
    GLuint elephantNormal = GL_NONE;
    LoadTexture(&elephantNormal, "./assets/textures/elephantNormal.jpg");

    GLuint sconceTex = GL_NONE;
    LoadTexture(&sconceTex, "./assets/textures/sconce.png");
    GLuint sconceNormal = GL_NONE;
    LoadTexture(&sconceNormal, "./assets/textures/sconceNormal.png");

    Vector3 camPos{ 0.0f, 2.0f, 3.0f };
    float fov = 75.0f * DEG2RAD;
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;
    float near = 0.001f;
    float far = 50.0f;
    float camPitch = 0;
    float camYaw = 0;
    float panScale = 0.25f;
    Vector3 frontView = V3_FORWARD;
    float planeRotationX = 0;
    float planeRotationY = 0;
    float planeRotationZ = 0;

    // Whether we render the imgui demo widgets
    bool imguiDemo = false;
    bool camToggle = true;
    int normalToggle = false;

    // ENABLE BACKFACE CULLING
    glEnable(GL_CULL_FACE);

    Mesh sphereMesh;// , horse;
    CreateMesh(&sphereMesh, SPHERE);
    //CreateMesh(&horse, "./assets/meshes/horse.obj");

    objects.resize(20); // positions from bottom corner of obj, will center if have time (doubt, heh) 
    objects[0] = RenderObject{ {-15.0f, 0.0f, 15.0f}, {30.0f, 30.0f, 1.0f}, V2_ONE, V3_UP, PLANE, floorTex, floorNormal };
    objects[1] = RenderObject{ {-15.0f, 5.0f, -15.0f}, {30.0f, 30.0f, 0.0f}, V2_ONE, {0.0f, -1.0f, 0.0f}, PLANE, floorTex, floorNormal};
    
    // Outside Walls
    objects[2] = RenderObject{ {-15.0f, 0.0f, -15.0f}, {30.0f, 5.0f, 1.0f}, {1.0f, 0.166667}, V3_FORWARD, PLANE, texStone, texStoneNormal };
    objects[3] = RenderObject{ {15.0f, 0.0f, 15.0f}, {30.0f, 5.0f, 1.0f}, {1.0f, 0.166667}, V3_FORWARD * -1.0f, PLANE, texStone, texStoneNormal };
    objects[4] = RenderObject{ {-15.0f, 0.0f, 15.0f}, {30.0f, 5.0f, 1.0f}, {1.0f, 0.166667}, V3_RIGHT, PLANE, texStone, texStoneNormal };
    objects[5] = RenderObject{ {15.0f, 0.0f, -15.0f}, {30.0f, 5.0f, 1.0f}, {1.0f, 0.166667f}, V3_RIGHT * -1.0f, PLANE, texStone, texStoneNormal };
    
    // Inside Walls
    objects[6] = RenderObject{ {-5.0f, 0.0f, 5.0f}, {10.0f, 5.0f, 1.0f}, {1.0f, 0.5}, V3_FORWARD, PLANE, texStone, texStoneNormal };
    objects[7] = RenderObject{ {5.0f, 0.0f, -5.0f}, {10.0f, 5.0f, 1.0f}, {1.0f, 0.5}, V3_FORWARD * -1.0f, PLANE, texStone, texStoneNormal };
    objects[8] = RenderObject{ {5.0f, 0.0f, 5.0f}, {10.0f, 5.0f, 1.0f}, {1.0f, 0.5}, V3_RIGHT, PLANE, texStone, texStoneNormal };
    objects[9] = RenderObject{ {-5.0f, 0.0f, -5.0f}, {10.0f, 5.0f, 1.0f}, {1.0f, 0.5}, V3_RIGHT * -1.0f, PLANE, texStone, texStoneNormal };
    
    // Items
    objects[10] = RenderObject{ {1.0f, 1.0f, 1.0f}, V3_ONE * 5.0f, V2_ONE, V3_FORWARD,"./assets/meshes/horse.obj", horseTex, horseNormal };
    objects[11] = RenderObject{ {-1.0f, 1.0f, -1.0f}, V3_ONE * 5.0f, V2_ONE, V3_FORWARD,"./assets/meshes/elephant.obj", elephantTex, elephantNormal };
    objects[12] = RenderObject{ {-1.0f, 1.0f, 1.0f}, V3_ONE * 5.0f, V2_ONE, V3_FORWARD, "./assets/meshes/sconce.obj", sconceTex, sconceNormal, true};


    lights.resize(20);
    lights[0] = Light{ objects[0].position, V3_ONE, POINT_LIGHT};
    lights[0].radius = 1.0f;
    lights[0].intensity = 2.0f;

    lights[1] = Light{ {0.0, 2.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, POINT_LIGHT };
    lights[1].intensity = 2.0f;
    lights[1].specularScale = 2.0f;
    lights[1].radius = 5.0f;

    lights[2] = Light();
    lights[2].color = { 1.0f, 0.0f, 0.0f };

    lights[3] = objects[13].light;

    lights[4] = Light{ camPos, V3_ONE, SPOT_LIGHT, frontView, 20, 10 };

    float ambientFactor = 0.25f;
    float diffuseFactor = 1.0f;


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

        // Input
        {
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

            if (IsKeyPressed(GLFW_KEY_I))
                imguiDemo = !imguiDemo;

            if (IsKeyPressed(GLFW_KEY_C))
                camToggle = !camToggle;

            if (IsKeyPressed(GLFW_KEY_N))
                normalToggle = !normalToggle; // I know what I'm doing intellisense, go away >:(

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
            float camTranslateValue = 0.10;
            if (IsKeyDown(GLFW_KEY_LEFT_SHIFT))
            {
                camTranslateValue = 0.20;
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
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Matrix normal = MatrixIdentity();
        Matrix world = MatrixIdentity();
        Matrix view = LookAt(camPos, camPos - frontView, V3_UP);
        Matrix proj = Perspective(fov, SCREEN_ASPECT, near, far);
        Matrix mvp = MatrixIdentity();

        GLuint u_color = -2;
        GLint u_normal = -2;
        GLint u_world = -2;
        GLint u_mvp = -2;
        GLint u_tex = -2;
        GLint u_normalMap = -2;
        GLuint shaderProgram = GL_NONE;

        GLint u_cameraPosition = -2;
        
        GLint u_ambientFactor = -2;
        GLint u_diffuseFactor = -2;
        GLint u_normalToggle = -2;

        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------


        //world = Translate(-0.5, -0.5, 0) * RotateX(DEG2RAD * planeRotationX) *  RotateY(DEG2RAD * planeRotationY) *  RotateZ(DEG2RAD * planeRotationY) * Scale(10, 10, 10);//RotateX(DEG2RAD * 90) * Translate(-0.5f, 0.0f, -0.5f) * Scale(10, 1, 10);

        shaderProgram = shaderPhong;
        glUseProgram(shaderProgram);
        //mvp = world * view * proj;
        
        normal = Transpose(Invert(world));

        u_normal = glGetUniformLocation(shaderProgram, "u_normal");

        u_cameraPosition = glGetUniformLocation(shaderProgram, "u_cameraPosition");
        
        u_ambientFactor = glGetUniformLocation(shaderProgram, "u_ambientFactor");
        u_diffuseFactor = glGetUniformLocation(shaderProgram, "u_diffuseFactor");
        u_normalToggle = glGetUniformLocation(shaderProgram, "u_normalToggle");
        
        glUniformMatrix3fv(u_normal, 1, GL_FALSE, ToFloat9(normal).v);

        glUniform3fv(u_cameraPosition, 1, &camPos.x);     
        
        glUniform1f(u_ambientFactor, ambientFactor);
        glUniform1f(u_diffuseFactor, diffuseFactor);
        glUniform1i(u_normalToggle, normalToggle);


        // Objects

        glUseProgram(shaderPhong);
        mvp = world * view * proj;

        objects[10].facing = Rotate(objects[10].facing, V3_UP, DEG2RAD * 0.1);

        for (int i = 0; i < objects.size(); i++)
        {
            if (objects[i].texture != NULL)
            {
                world = Scale(objects[i].scale) * // Rotation, because it doensn't like 180
                    (objects[i].facing == V3_FORWARD * -1.0f ? RotateY(180 * DEG2RAD) : ToMatrix(FromTo(V3_FORWARD, Normalize(objects[i].facing)))) * 
                    Translate(objects[i].position);
                mvp = world * view * proj;
                objects[i].Render(shaderPhong, &mvp, &world);
                objects[i].Emit();
            }
        }

        // Lights

        //  lights[0].position = Multiply(lights[0].position, RotateZ(sin(dt * 15)));
        //  lights[0].direction = CalcFacingVector3(Vector3{ 0 }, lights[0].position);
        //lights[0].position = objects[0].position;
        lights[2].position = objects[0].GetCenteredPosition();

        lights[3] = objects[12].light;

        for (int i = 0; i < lights.size(); i++)
        {
            lights[i].Render(shaderPhong, i);
            
            world = Scale(V3_ONE * lights[i].radius) * Translate(lights[i].position);
            mvp = world * view * proj;

            lights[i].DrawLight(shaderUniformColor, &mvp, sphereMesh);
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

            ImGui::SliderFloat3("Light Position", &lights[0].position.x, -15.0f, 15.0f);
            ImGui::SliderFloat3("Light Direction", &lights[0].direction.x, -1.0f, 1.0f);
            ImGui::SliderFloat3("Light Color", &lights[0].color.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Light Radius", &lights[0].radius, 0.0f, 15.0f);
            ImGui::SliderFloat("Light Intensity", &lights[0].intensity, 0.0f, 20.0f);
            ImGui::SliderFloat("Light FOV", &lights[0].FOV, 0.0f, 180.0f);
            ImGui::SliderFloat("Light FOV Blend", &lights[0].FOVbloom, 0.0f, 180.0f - lights[0].FOV);      
            ImGui::SliderInt("Light Type", &lights[0].type, 0, 2);
            ImGui::NewLine();

            ImGui::SliderFloat3("Wall Position", &objects[0].position.x, -10.0f, 10.0f);
            ImGui::SliderFloat3("Wall Direction", &objects[0].facing.x, -1.0f, 1.0f);
            ImGui::SliderFloat3("Wall Scale", &objects[0].scale.x, -10.0f, 10.0f);
            ImGui::NewLine();

            ImGui::SliderFloat("Plane Rotation X", &planeRotationX, 0.0f, 360.0f);
            ImGui::SliderFloat("Plane Rotation Y", &planeRotationY, 0.0f, 360.0f);
            ImGui::SliderFloat("Plane Rotation Z", &planeRotationZ, 0.0f, 360.0f);
            ImGui::NewLine();

            ImGui::SliderFloat("Ambient", &ambientFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Diffuse", &diffuseFactor, 0.0f, 1.0f);
            ImGui::Checkbox("Normal Toggle", (bool*)&normalToggle);

            ImGui::SliderFloat("Near", &near, 0.0f, 10.0f);
            ImGui::SliderFloat("Far", &far,0.0f, 100.0f);
            ImGui::SliderAngle("FoV", &fov, 10.0f, 90.0f);
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

void LoadTexture(GLuint *texture, const char* filename, bool hasAlpha)
{
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels;

    pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, 0);

    *texture = GL_NONE;
    glGenTextures(1, &*texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if(hasAlpha)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    pixels = nullptr;
}