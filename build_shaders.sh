VULKAN_SDK=/Users/sebastian/VulkanSDK/1.3.296.0/macOS
${VULKAN_SDK}/bin/glslc shaders/shader.vert -o shaders/vert.spv
${VULKAN_SDK}/bin/glslc shaders/shader.frag -o shaders/frag.spv
${VULKAN_SDK}/bin/glslc shaders/bar.vert -o shaders/bar_vert.spv
${VULKAN_SDK}/bin/glslc shaders/bar.frag -o shaders/bar_frag.spv
${VULKAN_SDK}/bin/glslc shaders/backdrop.vert -o shaders/backdrop_vert.spv
${VULKAN_SDK}/bin/glslc shaders/backdrop.frag -o shaders/backdrop_frag.spv