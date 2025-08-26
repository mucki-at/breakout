//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "common.h"
#include "vkutils.h"
#include "buffermanager.h"
#include "swapchainmanager.h"
#include "pipeline.h"
#include "dynamicresource.h"
#include "game.h"
#include "texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static inline constexpr size_t DefaultWidth  = 800;
static inline constexpr size_t DefaultHeight = 600;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key >= 0)
    {
        Game* game=static_cast<Game*>(glfwGetWindowUserPointer(window));
        if (game) game->setKey(key, action != GLFW_RELEASE);
    }
}

void resize_callback(GLFWwindow* window, int width, int height)
{
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
}


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription( 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos) ),
            vk::VertexInputAttributeDescription( 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) ),
            vk::VertexInputAttributeDescription( 2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) )
        };
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

using MyPipeline = Pipeline<Vertex, UniformBufferObject, 2>;


//!@brief
//!
//!@param argc
//!@param argv
//!@return int
int main(int argc, char* argv[])
try {

    // Step 1: initialize graphics
    // Step 1.1: initialize glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    auto window = glfwCreateWindow(DefaultWidth, DefaultHeight, "Break Out Volcano !!", nullptr, nullptr);

    // Step 1.2: initialize Vulkan
    vk::raii::Context context; //!< overall context for Vulkan RAII wrapper. This handles loader and dispatcher logic

    //! Some information about our application
    auto appInfo = vk::ApplicationInfo {
        .pApplicationName = "Break Out Volcano",
        .applicationVersion = vk::makeVersion(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    vk::InstanceCreateFlags instanceCreateFlags={};
    vector<const char*> requiredExtensions;
    requiredExtensions.assign(glfwExtensions, glfwExtensions+glfwExtensionCount);

#ifdef __APPLE__
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);       // required for MAC OS
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

    requiredExtensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName); // required for MAC OS as dependency for portability subset later on
#endif

    //! Create a vulkan instance (global object to interact with vulkan api)
    auto instance = vk::raii::Instance(
        context,
        vk::InstanceCreateInfo {
            .flags=instanceCreateFlags,
            .pApplicationInfo = &appInfo
        }.setPEnabledExtensionNames(requiredExtensions)
    );

    //! Create a vulkan surface to display stuff on screen
    VkSurfaceKHR surfaceHandle;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surfaceHandle) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    vk::raii::SurfaceKHR surface(instance, surfaceHandle); // make RAII type from SDL created

    //! Find the proper physical device
    vector<const char*> deviceExtensions = {
#ifdef __APPLE__
        vk::KHRPortabilitySubsetExtensionName,
#endif
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRSynchronization2ExtensionName,
        vk::KHRCreateRenderpass2ExtensionName
    };

    auto [physicalDevice, graphicsQueueIndex, presentQueueIndex] = findAppropriateDeviceAndQueueFamily(
        instance,
        {
            .apiVersion = vk::ApiVersion13,
            .deviceExtensions = deviceExtensions,
            .features = { .samplerAnisotropy = true },
            .surface = &surface
        }
    );

    //! Create a logical device
    float prio = 1.0f;
    auto queueInfo = vk::DeviceQueueCreateInfo
    {
        .queueFamilyIndex = graphicsQueueIndex
    }.setQueuePriorities(prio);
    // TODO: why do we not need a second queueInfo for presentQueue if the index is different???

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain =
    {
        {.features = {.samplerAnisotropy = true} },   // vk::PhysicalDeviceFeatures2 
        {.shaderDrawParameters = true },  // Enable shader draw parameters
        {
            .dynamicRendering = true,      // Enable dynamic rendering from Vulkan 1.3
            .synchronization2 = true        // must enable this to use sync primitives for dynamic rendering
        },
        {.extendedDynamicState = true }   // Enable extended dynamic state from the extension
    }
    ;
    auto createInfo = vk::DeviceCreateInfo
    {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
    };
    createInfo.setQueueCreateInfos(queueInfo);
    createInfo.setPEnabledExtensionNames(deviceExtensions);

    auto device = vk::raii::Device(physicalDevice, createInfo);

    //! Create graphics and present queues
    auto graphicsQueue = vk::raii::Queue(device, graphicsQueueIndex, 0);
    auto presentQueue = vk::raii::Queue(device, presentQueueIndex, 0);

    auto allocator = vma::createAllocatorUnique({
        .physicalDevice=physicalDevice,
        .device=device,
        .instance=instance
    });
    

    auto bufferManager = BufferManager(*allocator, device, graphicsQueue, graphicsQueueIndex);

    //! create a swap chain
    auto swapper = SwapChainManager(
        device,
        graphicsQueue,
        presentQueue,
        graphicsQueueIndex,
        MyPipeline::MaxFramesInFlight
    );
    swapper.reset(physicalDevice, surface);

   // Step 2: initialize Game
    auto breakout = Game(DefaultWidth, DefaultHeight);
    breakout.init();

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetWindowUserPointer(window, &breakout);

    // Step 3: Run game loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    //! Load our resources
    Vertex vertices[]= {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };
    uint16_t indices[] = {
        0, 1, 2, 2, 3, 0
    };

    auto buffer=bufferManager.createBuffer(
        sizeof(vertices)+sizeof(indices),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        false
    );
    byte* ptr=static_cast<byte*>(bufferManager.getStage(0, sizeof(vertices)+sizeof(indices)));
    memcpy(ptr, vertices, sizeof(vertices));
    memcpy(ptr+sizeof(vertices), indices, sizeof(indices));
    bufferManager.upload(buffer, vk::BufferCopy{.size=sizeof(vertices)+sizeof(indices)});

    auto texture=createImageFromFile("textures/texture.jpg", bufferManager);
    auto sampler=createSampler(physicalDevice, device);

        //! create a pipeline
    auto pipeline = MyPipeline(device, swapper, bufferManager, texture, sampler);

 
    while (!glfwWindowShouldClose(window))
    {
        // Step 3.1: wait until we are ready for next frame (present has finished)
        if (swapper.waitForNextFrame())
        {
            // we need a reset
            swapper.reset(physicalDevice, surface);
            continue;
        }

        // Step 3.2: process input and update game state
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwPollEvents();

        breakout.processInput(deltaTime);
        breakout.update(deltaTime);
        
        // dummy
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), currentFrame * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapper.getExtent().width) / static_cast<float>(swapper.getExtent().height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        const auto& perFrame=pipeline.getNextFrame();
        memcpy(perFrame.uniforms, &ubo, sizeof(ubo));

        // Step 3.3: render frame
        auto& commandBuffer = swapper.beginFrame();
        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.25f, 1.0f);
        auto attachmentInfo = vk::RenderingAttachmentInfo{
            .imageView = swapper.getCurrentView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        auto renderingInfo = vk::RenderingInfo{
            .renderArea = { .offset = { 0, 0 }, .extent = swapper.getExtent() },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };
        commandBuffer.beginRendering(renderingInfo);
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapper.getExtent().width), static_cast<float>(swapper.getExtent().height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapper.getExtent()));

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getPipeline());
        commandBuffer.bindVertexBuffers(0, {buffer}, {0});
        commandBuffer.bindIndexBuffer(buffer, sizeof(vertices), vk::IndexType::eUint16);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getLayout(), 0, perFrame.descriptors, nullptr);

        //breakout.render();
        commandBuffer.drawIndexed(6, 1, 0, 0, 0);
        
        commandBuffer.endRendering();

        if (swapper.endFrame())
        {
            // we need a reset
            swapper.reset(physicalDevice, surface);
        }
    }

    swapper.cleanup();
 
    // Step 4: cleanup glfw (vulkan uses RAII and doesn't need cleanup code)
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
catch (vk::SystemError& e)
{
    cerr << "Vulkan error: " << e.what() << std::endl;
    return 1;
}
catch (runtime_error& e)
{
    cerr << "runtime error: " << e.what() << std::endl;
    return 2;
}
catch (...)
{
    cerr << "unknown error" << std::endl;
    return -1;
}
