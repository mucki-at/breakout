//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "game.h"
#include <iostream>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpragma-pack"
#include <SDL.h>
#include <SDL_vulkan.h>
#pragma clang diagnostic pop
#include <vulkan/vulkan.hpp>

static inline constexpr size_t DefaultWidth  = 800;
static inline constexpr size_t DefaultHeight = 600;

struct SDLChecker
{
    SDLChecker& operator=(int value)
    {
        if (value!=0)
        {
            std::cerr << "SDL check failed: " << SDL_GetError() << std::endl;
            exit(-1);
        }
        return *this;
    }
};

struct VKChecker
{
    VKChecker& operator=(VkResult result)
    {
        if (result!=VK_SUCCESS)
        {
            std::cerr << "VK check failed: " << static_cast<int>(result) << std::endl;
            exit(-1);
        }
        return *this;
    }
};

#define SDL_CHECKED SDLChecker() = 
#define VK_CHECKED VKChecker() = 

//!@brief
//!
//!@param argc
//!@param argv
//!@return int
int main(int argc, char* argv[])
{
    auto breakout = Game(DefaultWidth, DefaultHeight);

    // Step 1: initialize Vulkan
    SDL_CHECKED SDL_Init(SDL_INIT_VIDEO);
    SDL_CHECKED SDL_Vulkan_LoadLibrary(nullptr);

    SDL_Window* window = SDL_CreateWindow(
        "Break Out Volcano!!",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        DefaultWidth,
        DefaultHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);


    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> requiredExtensions;
    unsigned int extensionCount=0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);

    requiredExtensions.resize(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, requiredExtensions.data());
    
    requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    
    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = requiredExtensions.size();
    instInfo.ppEnabledExtensionNames = requiredExtensions.data();
    instInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VkInstance vkInst=0;
    VK_CHECKED vkCreateInstance(&instInfo, nullptr, &vkInst);

    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(vkInst, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vkInst, &physicalDeviceCount, physicalDevices.data());
    VkPhysicalDevice physicalDevice = physicalDevices[0];

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window, vkInst, &surface);

    uint32_t graphicsQueueIndex = UINT32_MAX;
    uint32_t presentQueueIndex  = UINT32_MAX;
    VkBool32 support;
    uint32_t i = 0;
    for (VkQueueFamilyProperties queueFamily: queueFamilies)
    {
        if (graphicsQueueIndex == UINT32_MAX && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsQueueIndex = i;
        if (presentQueueIndex == UINT32_MAX)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &support);
            if (support) presentQueueIndex = i;
        }
        ++i;
    }

    float                   queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo     = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
        nullptr,                                    // pNext
        0,                                          // flags
        graphicsQueueIndex,                         // graphicsQueueIndex
        1,                                          // queueCount
        &queuePriority,                             // pQueuePriorities
    };

    VkPhysicalDeviceFeatures deviceFeatures         = {};
    const char*              deviceExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo       createInfo             = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
        nullptr,                              // pNext
        0,                                    // flags
        1,                                    // queueCreateInfoCount
        &queueInfo,                           // pQueueCreateInfos
        0,                                    // enabledLayerCount
        nullptr,                              // ppEnabledLayerNames
        1,                                    // enabledExtensionCount
        deviceExtensionNames,                 // ppEnabledExtensionNames
        &deviceFeatures,                      // pEnabledFeatures
    };
    VkDevice device;
    vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);

    VkQueue graphicsQueue;
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);

    VkQueue presentQueue;
    vkGetDeviceQueue(device, presentQueueIndex, 0, &presentQueue);

    SDL_Log("Initialized with errors: %s", SDL_GetError());

    // Step 2: initialize Game
    breakout.init();

    // Step 3: Run game loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    bool running = true;
    while (running)
    {
        // Step 3.1: update time and get input
        float currentFrame = SDL_GetTicks64();
        deltaTime          = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        SDL_Event windowEvent;
        while (SDL_PollEvent(&windowEvent))
            if (windowEvent.type == SDL_QUIT)
            {
                running = false;
                break;
            }


        // Step 3.2: process input and update game state
        breakout.processInput(deltaTime);
        breakout.update(deltaTime);

        // Step 3.3: render frame
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);
        breakout.render();

        // glfwSwapBuffers(window);
    }

    // delete all resources as loaded using the resource manager
    // ---------------------------------------------------------
    // ResourceManager::Clear();

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vkInst, nullptr);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();

    SDL_Log("Cleaned up with errors: %s", SDL_GetError());

    return 0;
}

#if 0
#    include "game.h"
#    include "resource_manager.h"

#    include <iostream>

// GLFW function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// The Width of the screen
const unsigned int SCREEN_WIDTH = 800;
// The height of the screen
const unsigned int SCREEN_HEIGHT = 600;

Game Breakout(SCREEN_WIDTH, SCREEN_HEIGHT);

//!@brief 
//!
//!@param argc 
//!@param argv 
//!@return int 
int main(int argc, char *argv[])
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#    ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#    endif
    glfwWindowHint(GLFW_RESIZABLE, false);

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // OpenGL configuration
    // --------------------
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // initialize game
    // ---------------
    Breakout.Init();

    // deltaTime variables
    // -------------------
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        // calculate delta time
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwPollEvents();

        // manage user input
        // -----------------
        Breakout.ProcessInput(deltaTime);

        // update game state
        // -----------------
        Breakout.Update(deltaTime);

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        Breakout.Render();

        glfwSwapBuffers(window);
    }

    // delete all resources as loaded using the resource manager
    // ---------------------------------------------------------
    ResourceManager::Clear();

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // when a user presses the escape key, we set the WindowShouldClose property to true, closing the application
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            Breakout.Keys[key] = true;
        else if (action == GLFW_RELEASE)
            Breakout.Keys[key] = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

#endif
