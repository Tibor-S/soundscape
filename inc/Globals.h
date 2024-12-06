//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef GLOBALS_H
#define GLOBALS_H
#include <vulkan/vulkan.hpp>
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "/Users/sebastian/CLionProjects/soundscape/models/viking_room.obj";
const std::string TEXTURE_PATH = "/Users/sebastian/CLionProjects/soundscape/textures/viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
#endif //GLOBALS_H
