cmake_minimum_required (VERSION 3.30)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_VERBOSE_MAKEFILE YES)


project (BreakOutVolcano VERSION 0.1.0 LANGUAGES CXX)


find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (VulkanMemoryAllocator CONFIG REQUIRED)
find_package (VulkanMemoryAllocator-Hpp CONFIG REQUIRED)

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

function (add_slang_shaders)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "TARGET" "SOURCES")

    if (NOT arg_TARGET)
        set(arg_TARGET shaders)
    endif()
    set (arg_SHADERS_DIR ${CMAKE_CURRENT_LIST_DIR}/shaders)
    set (arg_ENTRY_POINTS -entry vertMain -entry fragMain)
  
    add_custom_command (
        OUTPUT ${arg_SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${arg_SHADERS_DIR}
    )
    add_custom_command (
            OUTPUT  ${arg_SHADERS_DIR}/slang.spv
            COMMAND ${SLANGC_EXECUTABLE} ${arg_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${arg_SHADERS_DIR}/slang.spv
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            DEPENDS ${arg_SHADERS_DIR} ${arg_SOURCES}
            COMMENT "Compiling Slang Shaders"
            VERBATIM
    )
    add_custom_target (${arg_TARGET} DEPENDS ${arg_SHADERS_DIR}/slang.spv)
endfunction()

add_executable (breakout
    main.cpp
    game.cpp
    vkutils.cpp
    buffermanager.cpp
    swapchainmanager.cpp
    stb.cpp
    vma.cpp
    texture.cpp
)

target_link_libraries (breakout Vulkan::Vulkan glfw GPUOpen::VulkanMemoryAllocator VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)

#target_include_directories (breakout PRIVATE ${STB_INCLUDEDIR})

add_slang_shaders(SOURCES shader.slang)
add_dependencies(breakout shaders)
