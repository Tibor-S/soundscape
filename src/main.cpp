#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <AudioRecord.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <chrono>
#include "HelloTriangleApplication.h"
#include <cstdlib>
#include <Visual.h>
#include <portaudio.h>

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
    explicit Application(Visual* vis) : InterFrame(), m_vis(vis) {
        m_bone_displacement.resize(m_bar_count);
        m_image_count = vis->get_image_count();
        m_audio_record = new AudioRecord(32768);

        m_last_frame = std::chrono::steady_clock::now();


        const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        m_bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        m_scale_factor = 1 / (m_bar_diameter / 2.0f);

        m_audio_record->start_recognition();
    }
    ~Application() override {
        delete m_audio_record;
    }

    void start_audio_recording() const {
        if (!m_audio_record->start_recording()) {
            throw std::runtime_error("Failed to start audio recording");
        }
    }

    void load_bars() const {
        // const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        // const float bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        // const float bar_scale_factor = bar_diameter / 2.0f;

        std::map<const char*, SpriteKind> sprite_load = {};
        for (size_t i = 0; i < m_bar_count; i++) {
            sprite_load[BAR_NAMES[i]] = BAR_SPRITE;
        }
        sprite_load["back_drop"] = BACK_DROP_SPRITE;
        m_vis->load_sprites(sprite_load);
        auto initial_bone_buffer = BoneBuffer {
            .bone = {glm::mat4(1.0f), glm::mat4(1.0f)}
        };

        const float low = (- m_bars_width + m_bar_diameter) / 2.0f;
        for (int i = 0; i < m_bar_count; i++) {
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);
            SpriteModel data = {
                .model_matrix = scale(
                    translate(glm::mat4(1.0f),
                              glm::vec3(low + static_cast<float>(i) * (m_bar_diameter + m_bar_margin), 0.0f, 0.0f)),
                    glm::vec3(1 / m_scale_factor, 1 / m_scale_factor, 1 / m_scale_factor)),
            };
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &data, sizeof(SpriteModel));
                sp->set_buffer(j, 2, &initial_bone_buffer, sizeof(BoneBuffer));
            }
        }

        const auto sp = m_vis->get_sprite("back_drop");
        auto model = SpriteModel {
            .model_matrix = scale(translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f)), glm::vec3(30.0f, 20.0f, 20.0f))
        };
        auto bone = BoneBuffer {
            .bone = {glm::mat4(1.0f), glm::mat4(1.0f)}
        };
        for (size_t j = 0; j < m_image_count; j++) {
            sp->set_buffer(j, 1, &model, sizeof(SpriteModel));
            sp->set_buffer(j, 2, &bone, sizeof(BoneBuffer));
        }
    }

    void inter_frame() override {
        m_frame += 1;
        //
        // if (m_frame % 1000 == 0) {
        //     using namespace std;
        //     cout << "Trying to recognize" << endl;
        //     m_audio_record->start_recognition();
        //     // return;
        // }

        const size_t first_frequency = 24;
        const size_t last_frequency = 4187;
        const size_t frequency_count = last_frequency - first_frequency;
        const auto current_frequencies = m_audio_record->current_frequencies();
        const size_t bin_count = m_bar_count;
        size_t frequencies_per_bin = frequency_count / bin_count;
        std::vector<float> amps(m_bar_count);



        float max_amp = 4.0f;
        for (size_t bin = 0; bin < bin_count; bin++) {
            float avg_amp = 0.0f;
            for (size_t i = 0; i < frequencies_per_bin; i++)
                avg_amp += current_frequencies[bin * frequencies_per_bin + i + first_frequency];
            amps[bin] = avg_amp / static_cast<float>(frequencies_per_bin);
            if (bin == 0) {
                amps[bin] /= 2.0f;
            }
            if (amps[bin] > max_amp) {
                amps[bin] = max_amp;
            }
        }

        for (size_t i = 0; i < m_bar_count; i++) {
            const auto amp = amps[i] * m_scale_factor;
            // using namespace std;
            // cout << "i: " << i << " amp: " << amp << endl;
            auto bone_buffer = BoneBuffer {};
            bone_buffer.bone[0] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, amp));
            bone_buffer.bone[1] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -amp));
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);

            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 2, &bone_buffer, sizeof(BoneBuffer));
            }
        }

        // auto camera = m_vis->get_camera();
        // auto data = camera->get_data();
        // data.view = glm::lookAt(glm::vec3(20 * glm::cos(2 * M_PI * m_frame / 1000), 20 * glm::sin(2 * M_PI * m_frame / 1000), 20.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        //                            glm::vec3(0.0f, 0.0f, 1.0f));
        // camera->set_data(data);

        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_frame).
               count() < 1000 / m_frame_rate) {
        }
        m_last_frame = std::chrono::steady_clock::now();
    }

private:
    Visual* m_vis;
    size_t m_image_count;
    float m_scale_factor;
    float m_bar_diameter;
    std::vector<float> m_bone_displacement;
    size_t m_frame = 0;
    size_t m_bar_count = 16;
    float m_bars_width = 20;
    float m_bar_margin = 0.2;

    AudioRecord* m_audio_record;

    size_t m_frame_rate = 60;
    std::chrono::time_point<std::chrono::steady_clock> m_last_frame;

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
    app.start_audio_recording();

    try {
        vis->run(&app);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
