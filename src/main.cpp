#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
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
    explicit Application(Visual* vis) : InterFrame(), m_vis(vis), m_amplitude(m_bar_count) {
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

    void load_scene() const {
        // const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        // const float bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        // const float bar_scale_factor = bar_diameter / 2.0f;

        std::map<const char*, SpriteKind> sprite_load = {};
        for (size_t i = 0; i < m_bar_count; i++) {
            sprite_load[BAR_NAMES[i]] = BAR_SPRITE;
        }
        sprite_load["back_drop"] = BACK_DROP_SPRITE;
        sprite_load["cover_art"] = COVER_ART_SPRITE;
        m_vis->load_sprites(sprite_load);
        auto initial_bone_buffer = BoneBuffer {
            .bone = {
                translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f))
            }
        };

        for (int i = 0; i < m_bar_count; i++) {
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);
            SpriteModel data = {
                .model_matrix = translate(glm::mat4(1.0f),
                                          glm::vec3(1.1f - static_cast<float>(m_bar_count - i - 1) * 0.25f, 0.0f,
                                                    0.0f)),
            };
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &data, sizeof(SpriteModel));
                sp->set_buffer(j, 2, &initial_bone_buffer, sizeof(BoneBuffer));
            }
        }


        {
            CornerColors corner_colors = {};
            corner_colors.color[0] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            corner_colors.color[1] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            corner_colors.color[2] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            corner_colors.color[3] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            corner_colors.color[4] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            // std::vector<float> data = {
            //     1, 0, 0, 1,
            //     0, 0, 1, 1,
            //     1, 0, 0, 1,
            //     0, 0, 1, 1,
            //     1, 1, 1, 1,
            // };
            const auto sp = m_vis->get_sprite("back_drop");
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &corner_colors, sizeof(CornerColors));
            }
        }
        // const auto sp = m_vis->get_sprite("back_drop");
        // auto model = SpriteModel {
        //     .model_matrix = scale(translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f)), glm::vec3(30.0f, 20.0f, 20.0f))
        // };
        // auto bone = BoneBuffer {
        //     .bone = {glm::mat4(1.0f), glm::mat4(1.0f)}
        // };
        // for (size_t j = 0; j < m_image_count; j++) {
        //     sp->set_buffer(j, 1, &model, sizeof(SpriteModel));
        //     sp->set_buffer(j, 2, &bone, sizeof(BoneBuffer));
        // }
    }

    void inter_frame() override {
        // static uint8_t red_channel = 0;
        m_frame += 1;
        //
        if (m_frame % 100 == 0) {
            auto image_url = m_audio_record->cover_art_url();
            if (image_url) {
                auto res_data = Communication::load_cover_art(image_url.value());
                int w, h, c = 0;
                uint8_t *pixel_data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(res_data.c_str()),
                                                            static_cast<int>(res_data.size()), &w, &h, &c,
                                                            STBI_rgb_alpha);

                auto cover_art = m_vis->get_sprite("cover_art");
                for (size_t j = 0; j < m_image_count; j++) {
                    cover_art->set_image(j, 1, pixel_data, w * h * STBI_rgb_alpha);
                }
                stbi_image_free(pixel_data);
                // stbi_load_from_file(m_cover_art_file, x, 0, );
            }
        //     red_channel += 100;
        // //     using namespace std;
        // //     cout << "Trying to recognize" << endl;
        // //     m_audio_record->start_recognition();
        // //     // return;
        }

        const size_t first_frequency = 30;
        const size_t last_frequency = 4000;
        const size_t frequency_count = last_frequency - first_frequency;
        const auto current_frequencies = m_audio_record->current_frequencies();
        const size_t bin_count = m_bar_count;
        size_t frequencies_per_bin = frequency_count / bin_count;
        std::vector<float> amps(m_bar_count);



        for (size_t bin = 0; bin < bin_count; bin++) {
            float avg_amp = 0.0f;
            float max_amp = 0.0f;
            for (size_t i = 0; i < frequencies_per_bin; i++) {
                float camp = current_frequencies[bin * frequencies_per_bin + i + first_frequency];
                if (camp > max_amp) {
                    max_amp = camp;
                }
                avg_amp += camp;
            }
            amps[bin] = avg_amp / static_cast<float>(frequencies_per_bin);
            if (bin == 0) amps[bin] /= 2.0f;
            amps[bin] = 1.0f - glm::exp(-0.5*amps[bin]);
            // amps[bin] /= 5.0f;
            // amps[bin] = max_amp;
            // if (bin == 0) {
            //     amps[bin] /= 4.0f;
            // }
        }

        for (size_t i = 0; i < m_bar_count; i++) {
            // const auto amp = amps[i] / max_amp;
            m_amplitude[i] += 0.3f * (amps[i] - m_amplitude[i]);
            // m_amplitude[i] = amps[i];
            // using namespace std;
            // cout << "i: " << i << " amp: " << amp << endl;
            auto bone_buffer = BoneBuffer {};
            bone_buffer.bone[0] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, m_amplitude[i]));
            bone_buffer.bone[1] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -m_amplitude[i]));
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);

            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 2, &bone_buffer, sizeof(BoneBuffer));
            }
        }

        // auto camera = m_vis->get_camera();
        // auto data = camera->get_data();
        // data.view = glm::lookAt(glm::vec3(5 * glm::cos(2 * M_PI * m_frame / 1000), 5 * glm::sin(2 * M_PI * m_frame / 1000), 5.0f * glm::cos(M_PI + 2 * M_PI * m_frame / 1000)), glm::vec3(0.0f, 0.0f, 0.0f),
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
    float m_bars_width = 6;
    float m_bar_margin = 0.1 / 8;
    std::vector<float> m_amplitude;

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

    // const Model model(Model::BAR);

    // std::map<const char*, SpriteKind> sprite_load = {};
    // sprite_load["main"] = BAR_SPRITE;
    // vis->load_sprites(sprite_load);
    // delete vis;
    // return 0;

    auto camera = vis->get_camera();
    auto camera_data = camera->get_data();
    camera_data.view = glm::lookAt(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, 0.0f, 1.0f));
    // camera_data.proj = glm::ortho(0, 400, 0, 400);
    camera->set_data(camera_data);

    Application app(vis);

    app.load_scene();
    app.start_audio_recording();

    try {
        vis->run(&app);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
