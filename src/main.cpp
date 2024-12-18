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
void LoadTexture(GLuint* texture, const char* filename, bool hasAlpha = false, bool singleChannel = false);
void PickupAction();
void HoldItems();
void PlaceItem(int slot);
void CheckAnswer();
void Collision();

struct Item
{
    RenderObject* object;
    bool pickedUp;
    int slot = 5;
    int key;
    int pedestal = 5;
};

Item items[4];
std::vector<Light> lights;
std::vector<RenderObject> objects;
RenderObject* pedestals[4];

Vector3 camPos{ -8.0f, 3.0f, -8.0f };

float camPitch = -20;
float camYaw = 45;

float itemYawRange = 90;

float pitchChangeFactor = 0;
float customRoll = 0;
float customPitch = 0;

bool passThroughMiddle = false;
bool puzzleSolved = false;
static int invSlot = 0;

Vector3 camX;
Vector3 camY;
Vector3 camZ;
Matrix camRotation;
Vector3 frontView = V3_FORWARD;

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

    // Walls from https://polyhaven.com/a/dry_riverbed_rock
    GLuint texStone = GL_NONE;
    LoadTexture(&texStone, "./assets/textures/stone.jpg");
    GLuint texStoneNormal = GL_NONE;
    LoadTexture(&texStoneNormal, "./assets/textures/stoneNormal.jpg");
    GLuint texStoneHeight = GL_NONE;
    LoadTexture(&texStoneHeight, "./assets/textures/stoneHeightInv.jpg");

    // Floor from https://polyhaven.com/a/recycled_brick_floor
    GLuint floorTex = GL_NONE;
    LoadTexture(&floorTex, "./assets/textures/floor.jpg");
    GLuint floorNormal = GL_NONE;
    LoadTexture(&floorNormal, "./assets/textures/floorNormal.jpg");

    // https://polyhaven.com/a/horse_statue_01
    GLuint horseTex = GL_NONE;
    LoadTexture(&horseTex, "./assets/textures/horse.jpg");
    GLuint horseNormal = GL_NONE;
    LoadTexture(&horseNormal, "./assets/textures/horseNormal.jpg");

    // https://www.fab.com/listings/6fa7eb1d-c62a-4e59-b651-dec4610eaa9b
    GLuint elephantTex = GL_NONE;
    LoadTexture(&elephantTex, "./assets/textures/elephant.jpg");
    GLuint elephantNormal = GL_NONE;
    LoadTexture(&elephantNormal, "./assets/textures/elephantNormal.jpg");

    // https://www.turbosquid.com/3d-models/wall-sconce-1698414
    GLuint sconceTex = GL_NONE;
    LoadTexture(&sconceTex, "./assets/textures/sconce.jpg");
    GLuint sconceNormal = GL_NONE;
    LoadTexture(&sconceNormal, "./assets/textures/sconceNormal.jpg");

    // https://www.fab.com/listings/58f4fe0d-6e74-41a8-a05f-d2f054667631
    GLuint dragonTex = GL_NONE;
    LoadTexture(&dragonTex, "./assets/textures/dragon.jpg");
    GLuint dragonNormal = GL_NONE;
    LoadTexture(&dragonNormal, "./assets/textures/dragonNormal.jpg");

    // https://www.fab.com/listings/2dec82b6-55c7-40d0-9c8a-fd294f9a887b
    GLuint fishTex = GL_NONE;
    LoadTexture(&fishTex, "./assets/textures/fish.jpg");
    GLuint fishNormal = GL_NONE;
    LoadTexture(&fishNormal, "./assets/textures/fishNormal.jpg");

    // https://www.fab.com/listings/4edfaf98-bbc9-4a7d-9d8a-b45f590a39be
    GLuint pillarTex = GL_NONE;
    LoadTexture(&pillarTex, "./assets/textures/pillar.jpg");
    GLuint pillarNormal = GL_NONE;
    LoadTexture(&pillarNormal, "./assets/textures/pillarNormal.jpg");

    // https://www.turbosquid.com/3d-models/ornate-stone-pedestal-01-2275938
    GLuint pedestalTex = GL_NONE;
    LoadTexture(&pedestalTex, "./assets/textures/pedestal2.jpg");
    GLuint pedestalNormal = GL_NONE;
    LoadTexture(&pedestalNormal, "./assets/textures/pedestalNormal2.jpg");

    // Literally straight from Guilty Gear Strive source files
    GLuint potTex = GL_NONE;
    LoadTexture(&potTex, "./assets/textures/POT_base.png", true);

    float fov = 75.0f * DEG2RAD;
    float left = -1.0f;
    float right = 1.0f;
    float top = 1.0f;
    float bottom = -1.0f;
    float near = 0.001f;
    float far = 50.0f;
    float panScale = 0.25f;

    // Whether we render the imgui demo widgets
    bool imguiDemo = false;
    bool camToggle = true;
    bool debugToggle = false;
    bool freeCam = false;
    bool showLights = false;
    int normalToggle = false;

    // ENABLE BACKFACE CULLING
    glEnable(GL_CULL_FACE);

    Mesh sphereMesh, insidePlaneMesh, outsidePlaneMesh, horseMesh, elephantMesh, sconceMesh, dragonMesh, fishMesh, pillarMesh, pedestalMesh, potMesh;
    CreateMesh(&sphereMesh, SPHERE);
    CreateMesh(&insidePlaneMesh, PLANE);
    CreateMesh(&outsidePlaneMesh, PLANE, {6.0f, 1.0f});
    CreateMesh(&horseMesh, "./assets/meshes/horse.obj");
    CreateMesh(&elephantMesh, "./assets/meshes/elephant.obj");
    CreateMesh(&sconceMesh, "./assets/meshes/sconce.obj");
    CreateMesh(&dragonMesh, "./assets/meshes/dragon.obj");
    CreateMesh(&fishMesh, "./assets/meshes/fish.obj");
    CreateMesh(&pillarMesh, "./assets/meshes/pillar.obj");
    CreateMesh(&pedestalMesh, "./assets/meshes/pedestal.obj");
    CreateMesh(&potMesh, "./assets/meshes/potemkin.obj");

    objects.resize(35); // positions from bottom corner of obj, will center if have time (doubt, heh) 
    objects[0] = RenderObject{ {-15.0f, 0.0f, 15.0f},  {30.0f, 30.0f, 1.0f}, {-90.0f,0.0f,0.0f},  &insidePlaneMesh, floorTex, floorNormal, NULL, false};
    objects[1] = RenderObject{ {-15.0f, 5.0f, -15.0f}, {30.0f, 30.0f, 0.0f}, {90.0f, 0.0f, 0.0f}, &insidePlaneMesh, floorTex, floorNormal, NULL, false};
    
    // Outside Walls
    objects[2] = RenderObject{ {-15.0f, 0.0f, -15.0f}, {30.0f, 5.0f, 1.0f}, V3_ZERO,              &outsidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[3] = RenderObject{ {15.0f, 0.0f, 15.0f},   {30.0f, 5.0f, 1.0f}, {0.0f, 180.0f, 0.0f}, &outsidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[4] = RenderObject{ {-15.0f, 0.0f, 15.0f},  {30.0f, 5.0f, 1.0f}, {0.0f, 90.0f, 0.0f},  &outsidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[5] = RenderObject{ {15.0f, 0.0f, -15.0f},  {30.0f, 5.0f, 1.0f}, {0.0f, -90.0f, 0.0f}, &outsidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    
    // Inside Walls
    objects[6] = RenderObject{ {-5.0f, 0.0f, 5.0f},  {10.0f, 5.0f, 1.0f}, V3_ZERO,              &insidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[7] = RenderObject{ {5.0f, 0.0f, -5.0f},  {10.0f, 5.0f, 1.0f}, {0.0f, 180.0f, 0.0f}, &insidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[8] = RenderObject{ {5.0f, 0.0f, 5.0f},   {10.0f, 5.0f, 1.0f}, {0.0f, 90.0f, 0.0f},  &insidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    objects[9] = RenderObject{ {-5.0f, 0.0f, -5.0f}, {10.0f, 5.0f, 1.0f}, {0.0f, -90.0f, 0.0f}, &insidePlaneMesh, texStone, texStoneNormal, texStoneHeight, false};
    
    // Items
    objects[10] = RenderObject{ {14.0f, 0.0f, 0.0f},   V3_ONE * 5.0f, {0.0f, -90.0f, 0.0f}, &horseMesh,    horseTex,    horseNormal,    false };
    objects[11] = RenderObject{ {0.0f, 0.0f, 14.0f}, V3_ONE * 5.0f, {0.0f, 180.0f, 0.0f}, &elephantMesh, elephantTex, elephantNormal, false};
    objects[12] = RenderObject{ {-14.0f, 0.0f, 0.0f}, V3_ONE * 5.0f, {0.0f, 90.0f, 0.0f}, &dragonMesh, dragonTex, dragonNormal, false};
    objects[13] = RenderObject{ {0.0f, 0.0f, -14.0f}, V3_ONE * 5.0f, V3_ZERO, &fishMesh, fishTex, fishNormal, false };
    
    // ANSWER = .. hey, no cheating, go away! (fdhe)
    items[0].object = &objects[10];
    items[0].key = 2;
    items[1].object = &objects[11];
    items[1].key = 3;
    items[2].object = &objects[12];
    items[2].key = 1;
    items[3].object = &objects[13];
    items[3].key = 0;

    // Torches
    objects[14] = RenderObject{ {-5.5f, 3.25f, 0.0f}, V3_ONE * 5.0f, {0.0f, 180.0f, 0.0f}, &sconceMesh, sconceTex, sconceNormal, true};
    objects[15] = RenderObject{ {5.5f, 3.25f, 0.0f},  V3_ONE * 5.0f, {0.0f, 0.0f, 0.0f},   &sconceMesh, sconceTex, sconceNormal, true};
    objects[16] = RenderObject{ {0.0f, 3.25f, 5.5f},  V3_ONE * 5.0f, {0.0f, -90.0f, 0.0f}, &sconceMesh, sconceTex, sconceNormal, true};
    objects[17] = RenderObject{ {0.0f, 3.25f, -5.5f}, V3_ONE * 5.0f, {0.0f, 90.0f, 0.0f},  &sconceMesh, sconceTex, sconceNormal, true};
    objects[30] = RenderObject{ {-13.85f, 3.25f, -13.85f}, V3_ONE * 5.0f, {0.0f, -45.0f, 0.0f},  &sconceMesh, sconceTex, sconceNormal, true};

    // Pillars

    objects[18] = RenderObject{ {5.0f, -0.1f, 5.0f}, V3_ONE * 5.0f,     {0.0f, 0.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false};
    objects[19] = RenderObject{ {-5.0f, -0.1f, -5.0f}, V3_ONE * 5.0f,   {0.0f, 180.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false };
    objects[20] = RenderObject{ {5.0f, -0.1f, -5.0f}, V3_ONE * 5.0f,    {0.0f, -90.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false };
    objects[21] = RenderObject{ {-5.0f, -0.1f, 5.0f}, V3_ONE * 5.0f,    {0.0f, 90.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false };
    objects[22] = RenderObject{ {15.0f, -0.1f, 15.0f}, V3_ONE * 5.0f,   {0.0f, 0.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false };
    objects[23] = RenderObject{ {-15.0f, -0.1f, -15.0f}, V3_ONE * 5.0f, {0.0f, 180.0f, 0.0f}, &pillarMesh, pillarTex, pillarNormal, false };
    objects[24] = RenderObject{ {15.0f, -0.1f, -15.0f}, V3_ONE * 5.0f,  {0.0f, -90.0f, 0.0f},  &pillarMesh, pillarTex, pillarNormal, false };
    objects[25] = RenderObject{ {-15.0f, -0.1f, 15.0f}, V3_ONE * 5.0f,  {0.0f, 90.0f, 0.0f},   &pillarMesh, pillarTex, pillarNormal, false };

    // Item pedestals

    objects[26] = RenderObject{ {-14.0f, 0.0f, -8.33f}, V3_ONE * 3.0f,  {0.0f, 90.0f, 0.0f},   &pedestalMesh, pedestalTex, pedestalNormal, false };
    objects[27] = RenderObject{ {-14.0f, 0.0f, -11.66f}, V3_ONE * 3.0f,  {0.0f, 90.0f, 0.0f},   &pedestalMesh, pedestalTex, pedestalNormal, false };
    objects[28] = RenderObject{ {-11.66f, 0.0f, -14.0f}, V3_ONE * 3.0f,  {0.0f, 0.0f, 0.0f},   &pedestalMesh, pedestalTex, pedestalNormal, false };
    objects[29] = RenderObject{ {-8.33f, 0.0f, -14.0f}, V3_ONE * 3.0f,  {0.0f, 0.0f, 0.0f},   &pedestalMesh, pedestalTex, pedestalNormal, false };

    pedestals[0] = &objects[26];
    pedestals[1] = &objects[27];
    pedestals[2] = &objects[28];
    pedestals[3] = &objects[29];

    // Reward
    objects[31] = RenderObject{ {0.0f, -3.2f, 0.0f}, V3_ONE * 1.2f,  {0.0f, 0.0f, 0.0f},   &potMesh, potTex, NULL, false };
    

    lights.resize(24);
    lights[0] = Light{ {0.0f, 4.9f, 0.0f},{0.0f, 0.0f, 0.0f} , SPOT_LIGHT, {0.0f, -1.0f, 0.0f},  30, 7 };

    lights[3] = Light{ camPos, V3_ONE, SPOT_LIGHT, frontView, 35, 20 };
    lights[3].intensity = 0.6f;

    objects[14].lightIndex = 4;
    objects[15].lightIndex = objects[14].lightIndex + 1 * RenderObject::lightAmount;
    objects[16].lightIndex = objects[14].lightIndex + 2 * RenderObject::lightAmount;
    objects[17].lightIndex = objects[14].lightIndex + 3 * RenderObject::lightAmount;
    objects[30].lightIndex = objects[14].lightIndex + 4 * RenderObject::lightAmount;
    
    for (int i = 0; i < RenderObject::lightAmount; i++)
    {
        lights[objects[14].lightIndex + i] = objects[14].lights[i];
        lights[objects[15].lightIndex + i] = objects[15].lights[i];
        lights[objects[16].lightIndex + i] = objects[16].lights[i];
        lights[objects[17].lightIndex + i] = objects[17].lights[i];
        lights[objects[30].lightIndex + i] = objects[30].lights[i];
    }


    float ambientFactor = 0.25f;
    float diffuseFactor = 1.0f;
    float heightScale = 0.005f;


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
                // Looping mouse
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

            if (IsKeyPressed(GLFW_KEY_APOSTROPHE))
                debugToggle = !debugToggle;

            if (debugToggle)
            {
                if (IsKeyPressed(GLFW_KEY_L))
                    showLights = !showLights;

                if (IsKeyPressed(GLFW_KEY_F))
                    freeCam = !freeCam;

                if (IsKeyPressed(GLFW_KEY_T))
                    puzzleSolved = true;
            }
                if (IsKeyPressed(GLFW_KEY_C))
                    camToggle = !camToggle;

            if (IsKeyPressed(GLFW_KEY_P))
                PickupAction();

            if (IsKeyPressed(GLFW_KEY_1))
                PlaceItem(0);

            if (IsKeyPressed(GLFW_KEY_2))
                PlaceItem(1);
            
            if (IsKeyPressed(GLFW_KEY_3))
                PlaceItem(2);
            
            if (IsKeyPressed(GLFW_KEY_4))
                PlaceItem(3);

            if (IsKeyPressed(GLFW_KEY_N))
                normalToggle = !normalToggle; // I know what I'm doing intellisense, go away >:(

            if (!camToggle)
            {
                mouseDelta = V2_ZERO;
            }

            camPitch -= mouseDelta.y * panScale;
            if (camPitch > 89)
            {
                camPitch = 89;
            }
            else if (camPitch < -89)
            {
                camPitch = -89;
            }

            camYaw -= mouseDelta.x * panScale;
            camRotation = ToMatrix(FromEuler(camPitch * DEG2RAD, camYaw * DEG2RAD, 0.0f));

            camX = { camRotation.m0, camRotation.m1, camRotation.m2 };
            camY = { camRotation.m4, camRotation.m5, camRotation.m6 };
            camZ = { camRotation.m8, camRotation.m9, camRotation.m10 };
            
            frontView = Multiply(V3_FORWARD, camRotation);

            // Speeds cam when shifted
            float camTranslateValue = 0.225;
            if (IsKeyDown(GLFW_KEY_LEFT_SHIFT))
            {
                camTranslateValue = 0.4;
            }
            if (freeCam)
            {
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
            else
            {
                if (IsKeyDown(GLFW_KEY_W))
                {
                    // forwards
                    camPos -= Normalize(Vector3{ frontView.x, 0.0f,frontView.z }) * camTranslateValue;
                }
                if (IsKeyDown(GLFW_KEY_S))
                {
                    // back
                    camPos += Normalize(Vector3{ frontView.x, 0.0f,frontView.z }) * camTranslateValue;
                }
                if (IsKeyDown(GLFW_KEY_A))
                {
                    // left    Very normal
                    camPos -= Normalize(Multiply(Normalize(Vector3{ frontView.x, 0.0f, frontView.z }), RotateY(90 * DEG2RAD))) * camTranslateValue;
                }
                if (IsKeyDown(GLFW_KEY_D))
                {
                    // right
                    camPos += Normalize(Multiply(Normalize(Vector3{ frontView.x, 0.0f, frontView.z }), RotateY(90 * DEG2RAD))) * camTranslateValue;
                }

                Collision();
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
        // --------------------------------  VV MY STUFF VV  ----------------------------------------
        // ------------------------------------------------------------------------------------------

        /// Victory
        if (puzzleSolved)
        {
            static int ticks;

            const float step = 1.0f / 120.0f;
            objects[31].rotationVec.y += 4;
            lights[0].color = { 1.0f, 1.0f, 1.0f };
            if (ticks < 120)
            {
                // Pedestal rumble
                float rngRange = 0.02f;
                pedestals[0]->position = Vector3{ Random(-14.0f - rngRange, -14.0f + rngRange), 0.0f, Random(-8.33 - rngRange, -8.33 + rngRange) };
                pedestals[1]->position = Vector3{ Random(-14.0f - rngRange, -14.0f + rngRange), 0.0f, Random(-11.66 - rngRange, -11.66 + rngRange) };
                pedestals[2]->position = Vector3{ Random(-11.66f - rngRange, -11.66f + rngRange), 0.0f, Random(-14.0f - rngRange, -14.0f + rngRange) };
                pedestals[3]->position = Vector3{ Random(-8.33 - rngRange, -8.33 + rngRange), 0.0f, Random(-14.0f - rngRange, -14.0f + rngRange) };

                for (int i = 0; i < 4; i++)
                {
                    items[i].object->position = pedestals[items[i].key]->position + Vector3{ 0.0f, 2.05f, 0.0f };
                    items[i].object->rotationVec = pedestals[items[i].key]->rotationVec;
                    items[i].object->scale = V3_ONE * 5.0f;
                }

                ticks++;
            }
            else if (pedestals[0]->position.y > -3.6f)
            {
                // Pedestal retreat
                pedestals[0]->position.y -= 3.6f * step;
                pedestals[1]->position.y -= 3.6f * step;
                pedestals[2]->position.y -= 3.6f * step;
                pedestals[3]->position.y -= 3.6f * step;

                items[0].object->position.y -= 3.6f * step;
                items[1].object->position.y -= 3.6f * step;
                items[2].object->position.y -= 3.6f * step;
                items[3].object->position.y -= 3.6f * step;
            }
            else if (objects[14].position.x > -14.5f && ticks < 180)
            {
                // Torches
                objects[14].position.x -= 9.0f * step / 3 * 2;
                objects[15].position.x += 9.0f * step / 3 * 2;
                objects[16].position.z += 9.0f * step / 3 * 2;
                objects[17].position.z -= 9.0f * step / 3 * 2;

                objects[14].rotationVec.y += 180 * step / 3 * 2;
                objects[15].rotationVec.y += 180 * step / 3 * 2;
                objects[16].rotationVec.y += 180 * step / 3 * 2;
                objects[17].rotationVec.y += 180 * step / 3 * 2;

                objects[30].rotationVec.y += 180 * step / 3 * 2;
                objects[30].position.x += 8.04f * step / 3 * 2;
                objects[30].position.z += 8.04f * step / 3 * 2;

            }
            else if (objects[6].position.y > -5.0f)
            {
                // Wall retreat
                objects[6].position.y -= 5.0f * step;
                objects[7].position.y -= 5.0f * step;
                objects[8].position.y -= 5.0f * step;
                objects[9].position.y -= 5.0f * step;
            }
            else if (objects[31].position.y < 1.0f)
            { 
                // Ta-daaa  (biggest .obj....)
                passThroughMiddle = true;
                objects[31].position.y += 5.0f * step;
            }
            else
                ticks++;
        }

        /// Sending to GPU

        shaderProgram = shaderPhong;
        glUseProgram(shaderProgram);
        
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
        glUniform1f(glGetUniformLocation(shaderProgram, "u_heightScale"), heightScale);
        glUniform1i(u_normalToggle, normalToggle);

        glUseProgram(shaderPhong);
        mvp = world * view * proj;

        HoldItems();

        // Lights

        lights[3].position = camPos;
        lights[3].direction = frontView * -1.0f;

        for (int i = 0; i < lights.size(); i++)
        {
            lights[i].Render(shaderPhong, i);

            world = Scale(V3_ONE * lights[i].radius) * Translate(lights[i].position);
            mvp = world * view * proj;

            if (showLights)
                lights[i].DrawLight(shaderUniformColor, &mvp, sphereMesh);
        }

        // Objects

        for (int i = 0; i < objects.size(); i++)
        {
            if (objects[i].texture != NULL)
            {
                world = Scale(objects[i].scale) * (// Rotation, because it doensn't like 180
                    Rotate(V3_RIGHT, objects[i].rotationVec.x * DEG2RAD) *
                    Rotate(V3_UP, objects[i].rotationVec.y * DEG2RAD) *
                    Rotate(V3_FORWARD, objects[i].rotationVec.z * DEG2RAD)
                    ) * Translate(objects[i].position);
                mvp = world * view * proj;
                objects[i].Render(shaderPhong, &mvp, &world);
                objects[i].Emit();

                if (objects[i].shouldEmit)
                {
                    for (int k = 0; k < objects[i].lightAmount; k++)
                    {
                        lights[k + objects[i].lightIndex] = objects[i].lights[k];
                    }
                }
            }
        }



        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------
        // ------------------------------------------------------------------------------------------

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (imguiDemo)
            ImGui::ShowDemoWindow();
        else if (debugToggle)
        {
            ImGui::SliderFloat3("Camera Position", &camPos.x, -15.0f, 15.0f);
            ImGui::SliderFloat3("Front View?", &frontView.x, -1.0f, 1.0f);
            ImGui::SliderFloat3("Cam Y-axis", &camY.x, -1.0f, 1.0f);
            ImGui::NewLine();

            ImGui::SliderFloat("Cam FOV", &lights[3].FOV, 0.0f, 180.0f);
            ImGui::SliderFloat("Cam FOV Blend", &lights[3].FOVbloom, 0.0f, 180.0f - lights[3].FOV);      
            ImGui::NewLine();

            ImGui::SliderFloat3("Fish Position",  &objects[13].position.x, -15.0f, 15.0f);
            ImGui::SliderFloat3("Fish Rotation", &objects[13].rotationVec.x, -180.0f, 180.0f);
            ImGui::SliderFloat3("Fish Scale",     &objects[13].scale.x, -10.0f, 10.0f);
            ImGui::NewLine();

            ImGui::SliderFloat("Yaw Change Range", &itemYawRange, -180.0f, 180.0f);
            ImGui::SliderFloat("Pitch Change Factor", &pitchChangeFactor, -3.0f, 3.0f);
            ImGui::SliderFloat("Custom Roll", &customRoll, -180.0f, 180.0f);
            ImGui::SliderFloat("Custom Pitch", &customPitch, -180.0f, 180.0f);
            ImGui::SliderFloat("Ambient", &ambientFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Diffuse", &diffuseFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("HeightScale", &heightScale, -0.25f, 0.25f);
            ImGui::Checkbox("Normal Toggle", (bool*)&normalToggle); ImGui::SameLine();
            ImGui::Checkbox("Show Lights", &showLights);ImGui::SameLine();
            ImGui::Checkbox("Free Cam", &freeCam);

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

void LoadTexture(GLuint *texture, const char* filename, bool hasAlpha, bool singleChannel)
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
    else if(singleChannel)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texWidth, texHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    pixels = nullptr;
}

void FindLowestSlot()
{
    invSlot = 0;
    for (int i = -1; i < 4; i++)
    {
        if ((i == -1 ? items[3].slot : items[i].slot) == invSlot)
        {
            invSlot++;
            i = -1;
        }
    }
}

void PickupAction()
{
    for (int i = 0; i < 4; i++)
    {
        if(items[i].object != nullptr )
        {
            if (!puzzleSolved && Distance(Vector2{ items[i].object->position.x, items[i].object->position.z }, Vector2{ camPos.x, camPos.z }) < 2.0f && items[i].pickedUp == false)
            {
                items[i].pickedUp = true;
                items[i].slot = invSlot;
                items[i].pedestal = 5;
                FindLowestSlot();
                break;
            }
        }
    }
}

void PlaceItem(int slot)
{
    for (int i = 0; i < 4; i++)
    {
        // Distance, ignoring vertical component
        if (Distance(Vector2{ pedestals[i]->position.x, pedestals[i]->position.z }, Vector2{ camPos.x, camPos.z }) < 2.0f)
        {
            for (int k = 0; k < 4; k++)
            {
                // Pedestal full
                if (items[k].pedestal == i)
                    return;
            }
            for (int k = 0; k < 4; k++)
            {
                if(items[k].pickedUp == true && items[k].slot == slot)
                {
                    items[k].pickedUp = false;
                    items[k].slot = 5;
                    items[k].object->position = pedestals[i]->position + Vector3{0.0f, 2.05f, 0.0f};
                    items[k].object->rotationVec = pedestals[i]->rotationVec;
                    items[k].object->scale = V3_ONE * 5.0f;
                    items[k].pedestal = i;
                    continue;
                }
            }
        }
    }

    FindLowestSlot();
    CheckAnswer();
}

void HoldItems()
{
    for (int i = 0; i < 4; i++)
    {
        if (items[i].pickedUp == true)
        {
            // TO DO: Make items face cameraPos at all times. 
            // best way I can think of doing it is modifying the forward vector (WHICH IS KINDA OFF, USE SOMETHING ELSE THAT'S THE REAL VALUE)
            // Modify it by rotating it to desired locations, and placing items along those new vectors, a set distance
            // Get camera vector, rotate it in the respective camX & camY axis by item unique pitch/yaw amounts
            // Normalize it, and multiply by desired length  THIS WILL BE THE POSITION
            // 
            // To get updated rotation: 
            // calc eulers with the up direction in mind, from the camZ to the position vector
            // this SHOULD only give roll & yaw components, which are the new rotation values
            // See Unity & phys project for details on quaternions
            //
            // /\/\/\ Kinda done (good enough) /\/\/\


            float distScaleModifier = 0.005;
            float yawRange = 70;
            float pitchOffset = -30;
            float yawOffset = (yawRange * items[i].slot / 3.0f - yawRange / 2.0f) * -1.0f;

            // Get local Z by manipulating camX with desired offsets
            Vector3 objLocalZ = Multiply(frontView, Rotate(camX, pitchOffset * DEG2RAD) * Rotate(camY, yawOffset * DEG2RAD));

            // Get local X with cross of Z & camY as they're on the 'same' plane
            Vector3 objLocalX = Cross(objLocalZ, camY) * -1.0f;
            Vector3 objLocalY = Cross(objLocalZ, objLocalX) * 1.0f;

            Matrix objectRotationMatrix = { objLocalX.x, objLocalX.y, objLocalX.z, 0.0f,
                                            objLocalY.x, objLocalY.y, objLocalY.z, 0.0f,
                                            objLocalZ.x, objLocalZ.y, objLocalZ.z, 0.0f,
                                            0.0f,        0.0f,        0.0f,        1.0f };

            // Some quaternion sorcery.. it works good enough, but it feels like a crime
            Quaternion objQuat = FromMatrix(objectRotationMatrix);
            objQuat = { objQuat.y,
                        objQuat.z,
                        objQuat.x,
                        objQuat.w };
            Vector3 objEuler = ToEuler(objQuat) * RAD2DEG;

            items[i].object->scale = V3_ONE * distScaleModifier;
            items[i].object->rotationVec = Vector3{ -objEuler.z, -objEuler.x, 0.0f };
            items[i].object->position = camPos - objLocalZ * distScaleModifier;
        }
    }
}

void CheckAnswer()
{
    for (int i = 0; i < 4; i++)
    {
        if (items[i].key != items[i].pedestal)
        {
            return;
        }
    }
    puzzleSolved = true;
}

void Collision()
{
    if (camPos.x > 14.5f)
        camPos.x = 14.5f;
    if (camPos.x < -14.5f)
        camPos.x = -14.5f;
    if (camPos.z > 14.5f)
        camPos.z = 14.5f;
    if (camPos.z < -14.5f)
        camPos.z = -14.5f;

    if (camPos.x > -5.5f && camPos.x < 5.5f &&
        camPos.z > -5.5f && camPos.z < 5.5f && !passThroughMiddle)
    {
        if (abs(camPos.x) > abs(camPos.z))
            camPos.x = camPos.x > 0 ? 5.5f : -5.5f;
        else
            camPos.z = camPos.z > 0 ? 5.5f : -5.5f;
    }
}