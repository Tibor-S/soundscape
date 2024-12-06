cmake_minimum_required(VERSION 3.29)
project(soundscape)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan SDK environment variables
set(VULKAN_SDK /Users/sebastian/VulkanSDK/1.3.296.0/macOS)
set(ENV{VK_ICD_FILENAMES} ${VULKAN_SDK}/etc/vulkan/icd.d/MoltenVK_icd.json)
set(ENV{VK_LAYER_PATH} ${VULKAN_SDK}/etc/vulkan/explicit_layer.d)
set(ENV{VK_LOADER_DEBUG} all)

# Find dependencies
find_package(glfw3 3.4 REQUIRED)
#find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
#find_package(Vulkan REQUIRED)

# Include directories (if needed for custom libraries)
include_directories(${VULKAN_SDK}/include)
include_directories(./include)
include_directories(./inc)

set(SOURCE_FILES
        src/HelloTriangleApplication.cpp
        src/Device.cpp
        src/Image.cpp
        src/SamplerImage.cpp
        src/TextureImage.cpp
        src/SwapChain.cpp
        src/RenderPass.cpp
        src/Framebuffer.cpp
        src/RenderTarget.cpp
        src/PipelineLayout.cpp
        src/Pipeline.cpp
        src/Buffer.cpp
        src/UniformBuffer.cpp
        src/StagingBuffer.cpp
        src/VertexBuffer.cpp

        src/Sprite.cpp

        src/DescriptorManager.cpp
#        src/CommandManager.cpp

        src/Queue.cpp
        src/Memory.cpp
        src/Command.cpp
        src/Format.cpp
)

set(HEADER_FILES
        inc/HelloTriangleApplication.h
        inc/Device.h
        inc/Image.h
        inc/SamplerImage.h
        inc/TextureImage.h
        inc/SwapChain.h
        inc/RenderPass.h
        inc/Framebuffer.h
        inc/RenderTarget.h
        inc/PipelineLayout.h
        inc/Pipeline.h
        inc/Buffer.h
        inc/UniformBuffer.h
        inc/StagingBuffer.h
        inc/VertexBuffer.h

        inc/Sprite.h

        inc/DescriptorManager.h
#        inc/CommandManager.h

        inc/Queue.h
        inc/Memory.h
        inc/SwapChainSupport.h
        inc/Globals.h
        inc/Command.h
        inc/Format.h
        inc/Vertex.h

)

# Add executable
add_executable(soundscape src/main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

# Link libraries
target_link_libraries(soundscape PRIVATE
#        glm
        glfw
        Vulkan::Vulkan
)

# Include directories (specific to this project)
target_include_directories(soundscape PRIVATE
        ${VULKAN_SDK}/include
        ./include
)