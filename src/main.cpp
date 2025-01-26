#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <queue>

#include <AudioRecord.h>
#include <CoverArt.h>
#include <Visual.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
    explicit Application(Visual *vis) : InterFrame(), m_vis(vis), m_amplitude(m_bar_count)

    {
        m_bone_displacement.resize(m_bar_count);
        m_image_count = vis->get_image_count();
        m_audio_record = new AudioRecord(32768);

        m_last_frame = std::chrono::steady_clock::now();


        const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        m_bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        m_scale_factor = 1 / (m_bar_diameter / 2.0f);


        m_audio_record->start_recognition();
        m_palette = std::make_shared<AnalogousPalette>(400, 3);
        m_cover_art = new CoverArt(400 * 400 * STBI_rgb_alpha, m_palette);
        m_cover_art->acquire_default(m_cover_art_pixels, glm::vec3(1.0f));
    }
    ~Application() override {
        delete m_audio_record;
        delete m_cover_art;
    }

    void start_audio_recording() const {
        if (!m_audio_record->start_recording()) {
            throw std::runtime_error("Failed to start audio recording");
        }
    }

    void load_scene() const {
        std::map<const char*, SpriteKind> sprite_load = {};
        for (size_t i = 0; i < m_bar_count; i++) {
            sprite_load[BAR_NAMES[i]] = BAR_SPRITE;
        }
        sprite_load["back_drop"] = BACK_DROP_SPRITE;
        sprite_load["cover_art"] = COVER_ART_SPRITE;
        m_vis->load_sprites(sprite_load);
        const auto initial_bone_buffer = BoneBuffer {
            .bone = {
                translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f))
            }
        };
        const glm::vec3 color(0.1f);

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
                sp->set_buffer(j, 3, &color, sizeof(glm::vec3));
            }
        }


        {
            CornerColors corner_colors = {};
            corner_colors.color[0] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            corner_colors.color[1] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            corner_colors.color[2] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            corner_colors.color[3] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
            corner_colors.color[4] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            const auto sp = m_vis->get_sprite("back_drop");
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &corner_colors, sizeof(CornerColors));
            }
        }
    }

    void inter_frame() override {
        // static uint8_t red_channel = 0;
        m_frame += 1;
        auto image_url_opt = m_audio_record->cover_art_url();
        if (image_url_opt != m_last_cover_art) {
            m_last_cover_art = image_url_opt;
            using namespace std;
            cout << "new url" << endl;
            if (image_url_opt.has_value()) {
                m_cover_art->try_load(image_url_opt.value());
            } else {
                m_cover_art->try_reset(glm::vec3(1.0f));
            }
        }
        m_cover_art->try_approach(m_cover_art_pixels);
        const auto cover_art = m_vis->get_sprite("cover_art");
        for (size_t j = 0; j < m_image_count; j++) {
            cover_art->set_image(j, 1, m_cover_art_pixels.data(), m_cover_art->image_size());
        }

        auto palette_opt = m_palette->try_get_palette();
        if (palette_opt.has_value()) {
            auto palette = palette_opt.value();
            CornerColors corner_colors = {};
            corner_colors.color = {
                glm::vec4(palette.main, 1.0),
                glm::vec4(palette.main, 1.0),
                glm::vec4(palette.main, 1.0),
                glm::vec4(palette.main, 1.0),
            };
            corner_colors.fac.x = 0.33f;
            corner_colors.fac.y = 0.67f;
            const auto back_drop = m_vis->get_sprite("back_drop");
            for (size_t j = 0; j < m_image_count; j++) {
                back_drop->set_buffer(j, 1, &corner_colors, sizeof(CornerColors));
            }

            for (size_t i = 0; i < m_bar_count; i++) {
                const auto sp = m_vis->get_sprite(BAR_NAMES[i]);
                for (size_t j = 0; j < m_image_count; j++) {
                    sp->set_buffer(j, 3, &palette.comp, sizeof(glm::vec3));
                }
            }

        }

        const size_t first_frequency = 30;
        const size_t last_frequency = 10000;
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
            m_amplitude[i] += 0.3f * (amps[i] - m_amplitude[i]);
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
        // data.view = glm::lookAt(
        //     glm::vec3(5 * glm::cos(2 * M_PI * m_frame / 1000), 5 * glm::sin(2 * M_PI * m_frame / 1000),
        //               5.0f * glm::cos(M_PI + 2 * M_PI * m_frame / 1000)), glm::vec3(0.0f, 0.0f, 0.0f),
        //                            glm::vec3(0.0f, 0.0f, 1.0f));
        // camera->set_data(data);


        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_last_frame).count() < 1000 / m_frame_rate) {}
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
    std::optional<std::string> m_last_cover_art = std::nullopt;

    CoverArt* m_cover_art;
    std::vector<uint8_t> m_cover_art_pixels {};
    std::shared_ptr<AnalogousPalette> m_palette;


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
