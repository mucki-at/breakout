cmake_minimum_required (VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "OFF" FORCE)

project (BreakOutVolcano VERSION 0.1.0 LANGUAGES CXX)

find_package (glfw3 CONFIG REQUIRED)
find_package (glm CONFIG REQUIRED)
find_package (Vulkan REQUIRED)
find_package (VulkanMemoryAllocator CONFIG REQUIRED)
find_package (VulkanMemoryAllocator-Hpp CONFIG REQUIRED)
find_package (SDL3 CONFIG REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

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
)

target_link_libraries (breakout
    Vulkan::Vulkan
    glfw
    GPUOpen::VulkanMemoryAllocator
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    SDL3::SDL3
)

#target_include_directories (breakout PRIVATE ${STB_INCLUDEDIR})

add_slang_shader(slang SOURCES shader.slang)
add_slang_shader(sprites SOURCES sprites.slang)
add_slang_shader(particles SOURCES particles.slang)
add_slang_shader(postprocess SOURCES postprocess.slang)
add_dependencies(breakout shaders)
