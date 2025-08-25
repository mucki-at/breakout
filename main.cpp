//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "common.h"
#include "vkutils.h"
#include "game.h"

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


//!@brief
//!
//!@param argc
//!@param argv
//!@return int
int main(int argc, char* argv[])
try
{
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
        {},                               // vk::PhysicalDeviceFeatures2 (empty for now)
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

    //! create a swap chain
    auto swapper = SwapChainManager(
        device,
        graphicsQueue,
        presentQueue,
        graphicsQueueIndex,
        2
    );
    swapper.reset(physicalDevice, surface);

    //! create a pipeline
    auto shaderModule=loadShaderModule(device, "shaders/slang.spv");
    vk::PipelineShaderStageCreateInfo shaderStages[]=
    {
        vk::PipelineShaderStageCreateInfo
        {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain"
        },
        vk::PipelineShaderStageCreateInfo
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain"
        }
    };

    auto dynamicStates={vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    auto dynamicState = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates);
    
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{};
    auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo
    {
        .topology = vk::PrimitiveTopology::eTriangleList
    };
    auto viewport = vk::Viewport
    {
        .x=0.0f,
        .y=0.0f,
        .width=static_cast<float>(swapper.getExtent().width),
        .height=static_cast<float>(swapper.getExtent().height),
        .minDepth=0.0f,
        .maxDepth=1.0f
    };
    auto scissorRect=vk::Rect2D{ vk::Offset2D{ 0, 0 }, swapper.getExtent() };
    auto viewportState = vk::PipelineViewportStateCreateInfo{}
        .setViewports(viewport)
        .setScissors(scissorRect);
    auto rasterizer = vk::PipelineRasterizationStateCreateInfo
    {
        .cullMode = {}, //vk::CullModeFlagBits::eBack,
        .lineWidth = 1.0f
    };
    auto multisampling=vk::PipelineMultisampleStateCreateInfo{};
    auto colorBlendAttachment=vk::PipelineColorBlendAttachmentState
    {
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    };
    auto colorBlending=vk::PipelineColorBlendStateCreateInfo{}.setAttachments(colorBlendAttachment);
    
    auto pipelineLayoutInfo =vk::PipelineLayoutCreateInfo{};
    auto pipelineLayout = vk::raii::PipelineLayout( device, pipelineLayoutInfo );

    auto pipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo
    {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapper.getFormat()
    };

    auto pipelineInfo = vk::GraphicsPipelineCreateInfo
    {
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout
    };

    auto graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);

    // Step 2: initialize Game
    auto breakout = Game(DefaultWidth, DefaultHeight);
    breakout.init();

    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowUserPointer(window, &breakout);

    // Step 3: Run game loop
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

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

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapper.getExtent().width), static_cast<float>(swapper.getExtent().height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapper.getExtent()));

        //breakout.render();
        commandBuffer.draw(3, 1, 0, 0);
        
        commandBuffer.endRendering();

        if (swapper.endFrame())
        {
            // we need a reset
            swapper.reset(physicalDevice, surface);
        }
    }

    swapper.cleanup();

    // delete all resources as loaded using the resource manager
    // ---------------------------------------------------------
    // ResourceManager::Clear();

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
