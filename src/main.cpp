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

constexpr size_t MAX_BAR_COUNT = 16;
const char* BAR_NAMES[MAX_BAR_COUNT] = {
    "bar0",
    "bar1",
    "bar2",
    "bar3",
    "bar4",
    "bar5",
    "bar6",
    "bar7",
    "bar8",
    "bar9",
    "bar10",
    "bar11",
    "bar12",
    "bar13",
    "bar14",
    "bar15",
};

class Application : public InterFrame {
public:
    explicit Application(Visual* vis) : InterFrame(), m_vis(vis) {
        m_bone_displacement.resize(m_bar_count);
        m_image_count = vis->get_image_count();

    }
    ~Application() override = default;

    void load_bars() const {
        const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        const float bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        const float bar_scale_factor = bar_diameter / 2.0f;

        std::map<const char*, SpriteKind> sprite_load = {};
        for (size_t i = 0; i < m_bar_count; i++) {
            sprite_load[BAR_NAMES[i]] = BAR_SPRITE;
        }
        m_vis->load_sprites(sprite_load);
        auto initial_bone_buffer = BoneBuffer {
            .bone = {glm::mat4(1.0f), glm::mat4(1.0f)}
        };

        const float low = (- m_bars_width + bar_diameter) / 2.0f;
        for (int i = 0; i < m_bar_count; i++) {
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);
            SpriteModel data = {
                .model_matrix = scale(
                    translate(glm::mat4(1.0f),
                              glm::vec3(low + static_cast<float>(i) * (bar_diameter + m_bar_margin), 0.0f, 0.0f)),
                    glm::vec3(bar_scale_factor, bar_scale_factor, 1.0f)),
            };
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &data, sizeof(SpriteModel));
                sp->set_buffer(j, 2, &initial_bone_buffer, sizeof(BoneBuffer));
            }
        }
    }

    void inter_frame() override {
        m_frame += 1;
        const auto t = static_cast<float>(m_frame) / 100.0f;
        for (size_t i = 0; i < m_bar_count; i++) {
            const auto ang = glm::radians(360 * (static_cast<float>(i) / static_cast<float>(m_bar_count) + t));
            const auto displacement = (glm::sin(ang) + 1);
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);

            auto bone_buffer = BoneBuffer {};
            bone_buffer.bone[0] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, displacement));
            bone_buffer.bone[1] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -displacement));

            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 2, &bone_buffer, sizeof(BoneBuffer));
            }
        }
    }

    void audio_bar();
private:
    Visual* m_vis;
    size_t m_image_count;
    std::vector<float> m_bone_displacement;
    size_t m_frame = 0;
    size_t m_bar_count = 16;
    float m_bars_width = 16;
    float m_bar_margin = 0.5;
};

int main() {
    printEnv("VULKAN_SDK");
    printEnv("VK_ICD_FILENAMES");
    printEnv("VK_LAYER_PATH");
    printEnv("DYLD_LIBRARY_PATH");
    auto vis = new Visual(1);

    auto camera = vis->get_camera();
    auto camera_data = camera->get_data();
    camera_data.view = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, 0.0f, 1.0f));
    camera->set_data(camera_data);

    Application app(vis);

    app.load_bars();

    try {
        vis->run(&app);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
