cmake_minimum_required (VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "OFF" FORCE)

project (BreakOutVolcano VERSION 0.1.0 LANGUAGES CXX)

find_package (glfw3 CONFIG REQUIRED)
find_package (glm CONFIG REQUIRED)
find_package (Freetype REQUIRED)
find_package (Vulkan REQUIRED)
find_package (VulkanMemoryAllocator CONFIG REQUIRED)
find_package (VulkanMemoryAllocator-Hpp CONFIG REQUIRED)
find_package (SDL3 CONFIG REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
find_program(MAGICK_EXECUTABLE magick REQUIRED)

function (add_texture arg_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "TARGET;IMAGE" "")

    if (NOT arg_TARGET)
        set(arg_TARGET textures)
    endif()
    set (arg_TEXTURES_DIR ${CMAKE_CURRENT_LIST_DIR}/textures)
    set (arg_OUTPUT ${arg_TEXTURES_DIR}/${arg_NAME}.png)
  
    add_custom_command (
        OUTPUT ${arg_TEXTURES_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${arg_TEXTURES_DIR}
    )
    add_custom_command (
            OUTPUT  ${arg_OUTPUT}
            COMMAND ${MAGICK_EXECUTABLE} ${arg_IMAGE} ${arg_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${arg_SHADERS_DIR} ${arg_IMAGE}
            COMMENT "Compiling Textures"
            VERBATIM
    )

    add_custom_target (${arg_NAME} DEPENDS ${arg_OUTPUT})

    if(NOT TARGET ${arg_TARGET})
        add_custom_target (${arg_TARGET})
    endif()
    add_dependencies(${arg_TARGET} ${arg_NAME})
endfunction()


function (add_slang_shader arg_NAME)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "TARGET" "SOURCES")

    if (NOT arg_TARGET)
        set(arg_TARGET shaders)
    endif()
    set (arg_SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
    set (arg_ENTRY_POINTS -entry vertMain -entry fragMain)
    set (arg_OUTPUT ${arg_SHADERS_DIR}/${arg_NAME}.spv)
  
    add_custom_command (
        OUTPUT ${arg_SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${arg_SHADERS_DIR}
    )
    add_custom_command (
            OUTPUT  ${arg_OUTPUT}
            COMMAND ${SLANGC_EXECUTABLE} ${arg_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${arg_OUTPUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${arg_SHADERS_DIR} ${arg_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )

    add_custom_target (${arg_NAME} DEPENDS ${arg_OUTPUT})

    if(NOT TARGET ${arg_TARGET})
        add_custom_target (${arg_TARGET})
    endif()
    add_dependencies(${arg_TARGET} ${arg_NAME})
endfunction()

add_executable (breakout
    main.cpp
    game.cpp
    level.cpp
    vkutils.cpp
    buffermanager.cpp
    rendertarget.cpp
    imagerendertarget.cpp
    swapchain.cpp
    postprocess.cpp
    vulkan.cpp
    spritemanager.cpp
    particlesystem.cpp
    pipelinebuilder.cpp
    stb.cpp
    vma.cpp
    texture.cpp
    audiomanager.cpp
    font.cpp
)

target_link_libraries (breakout
    Vulkan::Vulkan
    glfw
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    SDL3::SDL3
    Freetype::Freetype
)

#target_include_directories (breakout PRIVATE ${STB_INCLUDEDIR})

add_slang_shader(slang SOURCES shader.slang)
add_slang_shader(sprites SOURCES sprites.slang)
add_slang_shader(particles SOURCES particles.slang)
add_slang_shader(postprocess SOURCES postprocess.slang)
add_slang_shader(text SOURCES text.slang)
add_dependencies(breakout shaders)

add_texture(ball IMAGE resource/awesomeface.png)
add_texture(background IMAGE resource/background.jpg)
add_texture(block IMAGE resource/block.png)
add_texture(fragment IMAGE resource/fragment.png)
add_texture(paddle IMAGE resource/paddle.png)
add_texture(powerup_chaos IMAGE resource/powerup_chaos.png)
add_texture(powerup_confuse IMAGE resource/powerup_chaos.png)
add_texture(powerup_increase IMAGE resource/powerup_chaos.png)
add_texture(powerup_passthrough IMAGE resource/powerup_chaos.png)
add_texture(powerup_speed IMAGE resource/powerup_chaos.png)
add_texture(powerup_sticky IMAGE resource/powerup_chaos.png)
add_texture(solid IMAGE resource/solid.png)
add_texture(trail IMAGE resource/circle.png)
add_dependencies(breakout textures)

file(COPY_FILE resource/exan3.ttf textures/font.ttf ONLY_IF_DIFFERENT)