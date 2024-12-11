#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "HelloTriangleApplication.h"
#include <cstdlib>
#include <Visual.h>

void printEnv(const char* var) {
    const char* value = std::getenv(var);
    if (value) {
        std::cout << var << ": " << value << std::endl;
    } else {
        std::cout << var << " is not set!" << std::endl;
    }
}

class Application : public InterFrame {
public:
    explicit Application(Visual* vis) : InterFrame(), m_vis(vis) {}
    ~Application() override = default;

    void inter_frame() override {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        auto sp = m_vis->get_sprite("main");
        SpriteModel sp_mod =  {
            .model_matrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };
        BoneBuffer bone_mod = {};
        bone_mod.bone[0] = glm::translate(glm::mat4(1.0f), (glm::sin(time) + 1) * glm::vec3(0.0f, 0.0f, 1.0f) / 2.0f);
        bone_mod.bone[1] = glm::translate(glm::mat4(1.0f), (glm::sin(time) + 1) * glm::vec3(0.0f, 0.0f, -1.0f) / 2.0f);
        sp->set_buffer(m_vis->current_frame(), 1, &sp_mod, sizeof(SpriteModel));
        sp->set_buffer(m_vis->current_frame(), 2, &bone_mod, sizeof(BoneBuffer));
        // auto sp2 = m_vis->get_sprite("second");
        // SpriteModel sp_mod2 =  {
        //     .model_matrix = glm::translate(glm::mat4(1.0f), glm::sin(time) * glm::vec3(1.0f, 0.0f, 0.0f)),
        // };
        // sp2->set_buffer(m_vis->current_frame(), 1, &sp_mod2, sizeof(SpriteModel));
    }

    void audio_bar();
private:
    Visual* m_vis;
};

int main() {
    printEnv("VULKAN_SDK");
    printEnv("VK_ICD_FILENAMES");
    printEnv("VK_LAYER_PATH");
    printEnv("DYLD_LIBRARY_PATH");
    // auto app = new HelloTriangleApplication();
    auto vis = new Visual();

    std::map<const char*, SpriteKind> sprite_load = {};
    sprite_load["main"] = BAR_SPRITE;
    sprite_load["second"] = VIKING_ROOM;
    vis->load_sprites(sprite_load);
    auto sp = vis->get_sprite("main");
    SpriteModel data = {
        .model_matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    };
    BoneBuffer data_bones {
        .bone = {
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f))
        }
    };
    sp->set_buffer(0, 1, &data, sizeof(SpriteModel));
    sp->set_buffer(1, 1, &data, sizeof(SpriteModel));
    sp->set_buffer(0, 2, &data_bones, sizeof(BoneBuffer));
    sp->set_buffer(1, 2, &data_bones, sizeof(BoneBuffer));

    auto sp2 = vis->get_sprite("second");
    sp2->set_buffer(0, 1, &data, sizeof(SpriteModel));
    sp2->set_buffer(1, 1, &data, sizeof(SpriteModel));

    Application app(vis);

    try {
        vis->run(&app);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
