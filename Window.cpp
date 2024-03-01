#include "Window.h"


Window::Window(/* args */) {}


// TODO: Remove all the resources we used
Window::~Window() {
    delete mp_scene_manager;
}


Window* Window::mp_window = nullptr;


void Window::setupWindowHints() const {
    /* Asking for core profile. Should be after glfwInit and before creating a windo. Otherwise, it won't work.*/
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    std::cout << "Running on Mac" << std::endl;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
}


void Window::setupImgui() const {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(mp_glfw_window, true);

    const char* glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);
}


void Window::showImguiDemo() const {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    static float f = 0.0f;
    static int counter = 0;

    bool show_demo_window = true;
    bool show_another_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    // 3. Show another simple window.
    if (true)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}


void Window::setupCallBack() const {
    glfwSetCursorPosCallback(mp_glfw_window, MouseListener::cursorPositionCallback);
    glfwSetMouseButtonCallback(mp_glfw_window, MouseListener::mouseButtonCallback);
    glfwSetScrollCallback(mp_glfw_window, MouseListener::scrollCallback);
    glfwSetKeyCallback(mp_glfw_window, KeyListener::keyCallback);
}


void Window::initializeWindow() {
    /* Init GLFW */
    if( !glfwInit() ) exit( EXIT_FAILURE );

    setupWindowHints();

    mp_glfw_window = glfwCreateWindow( m_width, m_height, mp_title, NULL, NULL );

    // initialize scene manager
    mp_scene_manager = new SceneManager(mp_glfw_window);
    
    glfwMakeContextCurrent(mp_glfw_window);
    glfwSwapInterval(1);
    if (!mp_glfw_window)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    setupCallBack();

    /* Initialize Glew. Must be done after glfw is initialized!*/
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return ;
    }
}


void Window::mainLoop() const {
    std::cout << "Drawing our scene" << std::endl;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    setupImgui();
    glfwSwapInterval(1);
    while (!glfwWindowShouldClose(mp_glfw_window))
    {
        glfwPollEvents();
        showImguiDemo();

        mp_scene_manager->loadScene("triangle_scene");

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        Time::update();
        // std::cout << (Time::deltaTime()) << " seconds" << std::endl;
        glfwSwapBuffers(mp_glfw_window);
    }

    glfwTerminate();
    exit( EXIT_SUCCESS );
}


Window* Window::getWindow() {
    if (mp_window == nullptr) {
        mp_window = new Window();
    }

    return mp_window;
};


void Window::run() {
    initializeWindow();
    mainLoop();
}
