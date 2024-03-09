#include "engine/core/Window.h"

Window::Window() {}

// TODO: Remove all the resources we used
Window::~Window()
{
}

Window *Window::m_window = nullptr;

void Window::setupWindowHints() const
{
    /* Asking for core profile. Should be after glfwInit and before creating a windo. Otherwise, it won't work.*/
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    std::cout << "Running on Mac" << std::endl;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void Window::setupImgui() const
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_glfw_window, true);

    const char *glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void Window::showImguiDemo() const
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    static float f = 0.0f;
    static int counter = 0;

    bool show_demo_window = true;
    bool show_another_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    // 3. Show another simple window.
    if (true)
    {
        ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}

void Window::setupCallBack() const
{
    glfwSetCursorPosCallback(m_glfw_window, MouseListener::cursorPositionCallback);
    glfwSetMouseButtonCallback(m_glfw_window, MouseListener::mouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, MouseListener::scrollCallback);
    glfwSetKeyCallback(m_glfw_window, KeyListener::keyCallback);
}

void Window::initializeWindow()
{
    /* Init GLFW */
    if (!glfwInit())
        exit(EXIT_FAILURE);

    setupWindowHints();

    m_glfw_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);

    glfwMakeContextCurrent(m_glfw_window);
    glfwSwapInterval(1);
    if (!m_glfw_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    setupCallBack();

    /* Initialize Glew. Must be done after glfw is initialized!*/
    GLenum res = glewInit();
    if (res != GLEW_OK)
    {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void Window::mainLoop()
{
    std::cout << "Drawing our scene" << std::endl;
    ImVec4 clear_color = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);

    setupImgui();
    glfwSwapInterval(1);
    // start scene manager by loading the first scene
    m_sceneManager->start();

    while (!glfwWindowShouldClose(m_glfw_window))
    {
        glfwPollEvents();
        showImguiDemo();
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

        if (KeyListener::isKeyPressed(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(m_glfw_window, true);
            std::cout << "Escape" << std::endl;
        }
        else if (KeyListener::isKeyPressed(GLFW_KEY_RIGHT))
        {
            m_sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(-500.0f * Time::deltaTime(), 0.0f, 0.0f));
        }
        else if (KeyListener::isKeyPressed(GLFW_KEY_LEFT))
        {
            m_sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(500.0f * Time::deltaTime(), 0.0f, 0.0f));
        }
        else if (KeyListener::isKeyPressed(GLFW_KEY_DOWN))
        {
            m_sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, 500.0f * Time::deltaTime(), 0.0f));
        }
        else if (KeyListener::isKeyPressed(GLFW_KEY_UP))
        {
            m_sceneManager->getActiveScene()->getCamera()->update(Time::deltaTime(), glm::vec3(0.0f, -500.0f * Time::deltaTime(), 0.0f));
        }
        m_sceneManager->update(Time::deltaTime(), m_glfw_window);

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // TODO: why have a static Time class if we are going to pass deltaTime to the update function?
        Time::update();
        glfwSwapBuffers(m_glfw_window);
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

Window *Window::getWindow()
{
    if (m_window == nullptr)
    {
        m_window = new Window();
    }

    return m_window;
};

void Window::addSceneManager(std::shared_ptr<SceneManager> sceneManager)
{
    m_sceneManager = sceneManager;
}

GLFWwindow *Window::getGlfwWindow()
{
    if (m_glfw_window == nullptr)
    {
        std::cout << "GLFW window is null" << std::endl;
        return nullptr;
    }
    return m_glfw_window;
}
void Window::run()
{
    mainLoop();
}
