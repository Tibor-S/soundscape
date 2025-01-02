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
find_library(CURL_LIBRARY NAMES curl)
find_package(glfw3 3.4 REQUIRED)
#find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
#find_package(Vulkan REQUIRED)
#find_package(portaudio REQUIRED)

set(FFTW3_PATH /opt/homebrew/Cellar/fftw/3.3.10_1)
set(FFTW3_INCLUDE_DIR ${FFTW3_PATH}/include)
set(FFTW3_LIBRARY ${FFTW3_PATH}/lib/libfftw3.a)

# Include directories (if needed for custom libraries)
include_directories(${VULKAN_SDK}/include)
include_directories(./include)
include_directories(./inc)



set(SOURCE_FILES
        src/Visual.cpp
        src/communication/shazam.cpp

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

        src/Queue.cpp
        src/Memory.cpp
        src/Command.cpp
        src/Format.cpp
        src/AudioRecord.cpp
        src/communication/content_download.cpp
        src/Model.cpp
)

set(HEADER_FILES
        inc/Visual.h

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
        inc/Model.h
        inc/Texture.h
        inc/Descriptor.h
        inc/AudioRecord.h
        inc/communication/content_download.h
)

# Add executable
add_executable(soundscape src/main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES}
)

add_library(portaudio STATIC IMPORTED)
set_target_properties(portaudio PROPERTIES IMPORTED_LOCATION /Users/sebastian/CLionProjects/soundscape/lib/portaudio/libportaudio.a)

add_library(vibra STATIC IMPORTED)
set_target_properties(vibra PROPERTIES IMPORTED_LOCATION /Users/sebastian/CLionProjects/soundscape/lib/vibra/libvibra.a)

set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)


# Link libraries
target_link_libraries(soundscape PRIVATE
#        glm
        glfw
        Vulkan::Vulkan
        portaudio
        vibra
        curl
        ${FFTW3_LIBRARY}
)

if(APPLE)
    target_link_libraries(soundscape PRIVATE
#            "-framework Foundation"
#            "-framework IOKit"
            "-framework CoreServices"
            "-framework CoreFoundation"
            "-framework AudioUnit"
            "-framework AudioToolbox"
            "-framework CoreAudio"
    )
endif()


# Include directories (specific to this project)
target_include_directories(soundscape PRIVATE
        ${VULKAN_SDK}/include
        ./include
        ${FFTW3_INCLUDE_DIR}
)