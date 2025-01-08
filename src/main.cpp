#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <queue>

#include <AudioRecord.h>
#include <Visual.h>
#include <communication/content_download.h>

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

#define PALETTE_SIZE 5
struct ColorPalette {
    glm::vec3 dark;
    glm::vec3 light;
    glm::vec3 third;
    glm::vec3 second;
    glm::vec3 main;
};

struct ColorGroup {
    glm::vec3 avg_color;
    size_t count;

};

struct ColorGroupByCount : ColorGroup {
    explicit ColorGroupByCount(const ColorGroup& group) : ColorGroup(group) {};
    bool operator >(const ColorGroup& rhs) const {
        return count > rhs.count;
    }
    bool operator <(const ColorGroup& rhs) const {
        return count < rhs.count;
    }
};

struct ColorGroupByLight : ColorGroup {
    explicit ColorGroupByLight(const ColorGroup& group) : ColorGroup(group) {};
    [[nodiscard]] float c_max() const {
        return std::max({avg_color.r, avg_color.g, avg_color.b});
    }
    [[nodiscard]] float c_min() const {
        return std::min({avg_color.r, avg_color.g, avg_color.b});
    }
    [[nodiscard]] float light() const {
        return (c_max() + c_min()) / 2;
    }
    bool operator >(const ColorGroupByLight& rhs) const {
        return light() > rhs.light();
    }
    bool operator <(const ColorGroupByLight& rhs) const {
        return light() < rhs.light();
    }
};

struct ColorGroupBySaturation : ColorGroupByLight {
    explicit ColorGroupBySaturation(const ColorGroup& group) : ColorGroupByLight(group) {};
    [[nodiscard]] float delta() const {
        return c_max() - c_min();
    }
    [[nodiscard]] float saturation() const {
        return delta() / (1 - abs(2 * light() - 1));

    }
    bool operator >(const ColorGroupBySaturation& rhs) const {
        return saturation() > rhs.saturation();
    }
    bool operator <(const ColorGroupBySaturation& rhs) const {
        return saturation() < rhs.saturation();
    }
};

class Palette {
public:
    Palette() = default;
    virtual ~Palette() = default;

    virtual void generate_target(const std::vector<uint8_t> &pixels, size_t pixel_size, bool alpha_channel) {
        auto guard = lock();
    }
    virtual bool try_approach_target(float factor) {
        if (!try_lock()) return false;
        unlock();
        return true;
    }
    virtual bool try_reset_palette() {
        if (!try_lock()) return false;
        unlock();
        return true;
    }
protected:
    std::lock_guard<std::mutex> lock() {return std::lock_guard(m_mtx); }
    bool try_lock() {return m_mtx.try_lock(); }
    void unlock() {return m_mtx.unlock(); }
private:
    std::mutex m_mtx;
};


class StandardPalette : public Palette {
public:
    static constexpr ColorPalette DEFAULT = {glm::vec3(1.0f),glm::vec3(1.0f),glm::vec3(1.0f),glm::vec3(1.0f),glm::vec3(1.0f)};

    explicit StandardPalette(const uint8_t relevant_bits) : m_relevant_bits(relevant_bits) {}
    ~StandardPalette() override = default;

    void generate_target(const std::vector<uint8_t> &pixels, size_t pixel_size, bool alpha_channel) override {
        auto guard = lock();
        std::map<std::array<uint8_t, 3>, size_t> bin_count {};
        std::map<std::array<uint8_t, 3>, glm::vec3> avg_color {};
        const uint16_t manhattan_max = (1 << m_relevant_bits) + (1 << m_relevant_bits) + (1 << m_relevant_bits) - 3;
        const uint16_t dl_manhattan_max = 1 << (m_relevant_bits - 1);
        const size_t offset = alpha_channel ? 4 : 3;

        std::array<uint8_t, 3> d_bin = {0,0,0};
        size_t d_bin_count = 0;
        std::array<uint8_t, 3> l_bin = {0,0,0};
        size_t l_bin_count = 0;
        std::array<uint8_t, 3> t_bin = {0,0,0};
        size_t t_bin_count = 0;
        std::array<uint8_t, 3> s_bin = {0,0,0};
        size_t s_bin_count = 0;
        std::array<uint8_t, 3> m_bin = {0,0,0};
        size_t m_bin_count = 0;
        for (size_t i = 0; i < pixels.size(); i += offset) {
            const auto red = pixels[i+0]; // 0xFF
            const auto green = pixels[i+1];
            const auto blue = pixels[i+2];

            const uint8_t red_bin = red >> (8 - m_relevant_bits);
            const uint8_t green_bin = green >> (8 - m_relevant_bits);
            const uint8_t blue_bin = blue >> (8 - m_relevant_bits);

            const uint16_t d_manhattan = static_cast<uint16_t>(red_bin) +
                                            static_cast<uint16_t>(green_bin) +
                                            static_cast<uint16_t>(blue_bin);
            const uint16_t l_manhattan = manhattan_max - d_manhattan;
            const auto is_dark = d_manhattan <= dl_manhattan_max;
            const auto is_light = l_manhattan <= dl_manhattan_max;

            std::array bin = {red_bin, green_bin, blue_bin};

            glm::vec3 color {};
            color.r = static_cast<float>(red) / 255.0f;
            color.g = static_cast<float>(green) / 255.0f;
            color.b = static_cast<float>(blue) / 255.0f;

            size_t current_bin_count = 0;
            if (!bin_count.contains(bin)) {
                bin_count[bin] = 1;
                avg_color[bin] = color;
                current_bin_count = 1;
            } else {
                auto old_color = avg_color[bin];
                const auto new_count = bin_count[bin] + 1;
                avg_color[bin] = (old_color * static_cast<float>(bin_count[bin]) + color)
                                / static_cast<float>(new_count);
                bin_count[bin] = new_count;
                current_bin_count = new_count;
            }

            if (is_dark && current_bin_count > d_bin_count) {
                d_bin_count = current_bin_count;
                d_bin = bin;
            } else if (is_light && current_bin_count > l_bin_count) {
                l_bin_count = current_bin_count;
                l_bin = bin;
            } else if (current_bin_count > m_bin_count) {
                if (m_bin != bin) {
                    t_bin_count = s_bin_count;
                    t_bin = s_bin;
                    s_bin_count = m_bin_count;
                    s_bin = m_bin;
                    m_bin_count = current_bin_count;
                    m_bin = bin;
                } else {
                    m_bin_count = current_bin_count;
                }
            } else if (current_bin_count > s_bin_count) {
                if (s_bin != bin) {
                    t_bin_count = s_bin_count;
                    t_bin = s_bin;
                    s_bin_count = current_bin_count;
                    s_bin = bin;
                } else {
                    s_bin_count = current_bin_count;
                }
            } else if (current_bin_count > t_bin_count) {
                t_bin_count = current_bin_count;
                t_bin = bin;
            }
        }

        // Make sure m and d have colors
        if (d_bin_count == 0) d_bin = m_bin; // At least one of these must have a color
        if (d_bin_count == 0) d_bin = l_bin; // At least one of these must have a color
        if (m_bin_count == 0) m_bin = d_bin; // At least one of these must have a color

        if (s_bin_count == 0) s_bin = m_bin;
        if (t_bin_count == 0) t_bin = s_bin;
        if (l_bin_count == 0) l_bin = t_bin;

        m_target.dark = avg_color[d_bin];
        m_target.light = avg_color[l_bin];
        m_target.third = avg_color[t_bin];
        m_target.second = avg_color[s_bin];
        m_target.main = avg_color[m_bin];
    }
    bool try_approach_target(const float factor) override {
        if (!try_lock()) return false;
        m_palette.dark = m_target.dark * factor + m_palette.dark * (1 - factor);
        m_palette.light = m_target.light * factor + m_palette.light * (1 - factor);
        m_palette.third = m_target.third * factor + m_palette.third * (1 - factor);
        m_palette.second = m_target.second * factor + m_palette.second * (1 - factor);
        m_palette.main = m_target.main * factor + m_palette.main * (1 - factor);
        unlock();
        return true;
    }
    [[nodiscard]] std::optional<ColorPalette> try_get_palette() {
        if (!try_lock()) return {};
        auto palette = m_palette;
        unlock();
        return palette;
    }
    bool try_set_palette(const ColorPalette& palette) {
        if (!try_lock()) return false;
        m_palette = palette;
        m_target = palette;
        unlock();
        return true;
    }
    bool try_reset_palette() override {
        if (!try_lock()) return false;
        m_palette = DEFAULT;
        m_target = DEFAULT;
        unlock();
        return true;
    }
private:
    uint8_t m_relevant_bits = 1;
    ColorPalette m_palette {};
    ColorPalette m_target {};

};

class CoverArt {
public:
    CoverArt(const size_t image_size, const std::shared_ptr<Palette> &palette) : m_cover_art_pixels(image_size),
                                                                                 m_palette(palette),
                                                                                 m_image_size(image_size) {
    }
    // explicit CoverArt(const size_t image_size) : m_image_size(image_size), m_cover_art_pixels(m_image_size) {}
    ~CoverArt() {
        if (m_reset_future_opt.has_value()) m_reset_future_opt.value().get();
        if (m_load_future_opt.has_value()) m_load_future_opt.value().get();
    };

    [[nodiscard]] size_t image_size() const { return m_image_size; }

    bool try_reset(const glm::vec3 color) {
        using namespace std::chrono_literals;
        if (m_reset_future_opt.has_value()) {
            if (m_reset_future_opt.value().wait_for(0ms) != std::future_status::ready) return false;
        }
        m_reset_future_opt = std::async(std::launch::async, &CoverArt::reset_aux, this, color);
        return true;
    }
    bool try_load(const std::string &url) {
        using namespace std::chrono_literals;
        if (m_load_future_opt.has_value()) {
            if (m_load_future_opt.value().wait_for(0ms) != std::future_status::ready) return false;
        }
        m_load_future_opt = std::async(std::launch::async, &CoverArt::load_aux, this, url);
        return true;
    }
    // Make sure dst_pixels is allocated with at least the size of the image
    bool try_approach(std::vector<uint8_t> &dst_pixels, const float factor = 0.1f)
    {
        if (!m_busy_mtx.try_lock()) return false;

        for (size_t i = 0; i < m_image_size; i++) {
            if (-1 <= (static_cast<int16_t>(dst_pixels[i]) - static_cast<int16_t>(m_cover_art_pixels[i])) &&
                (static_cast<int16_t>(dst_pixels[i]) - static_cast<int16_t>(m_cover_art_pixels[i])) <= 1)
            {
                dst_pixels[i] = m_cover_art_pixels[i];
                continue;
            }
            if (dst_pixels[i] != m_cover_art_pixels[i]) {
                while (false) {}
            }
            const auto c_val = static_cast<float>(dst_pixels[i]);
            const auto t_val = static_cast<float>(m_cover_art_pixels[i]);
            auto n_val = static_cast<uint8_t>((1 - factor) * c_val + factor * t_val);
            if (n_val == dst_pixels[i]) n_val = m_cover_art_pixels[i];
            dst_pixels[i] = n_val;
        }


        m_busy_mtx.unlock();

        return m_palette->try_approach_target(factor);
    }
    void set_default(std::vector<uint8_t>& dst_pixels, const glm::vec3 color) {
        dst_pixels.resize(m_image_size);
        for (size_t i = 0; i < m_image_size; i += 4) {
            dst_pixels[i + 0] = static_cast<uint8_t>(color.r * 255);
            dst_pixels[i + 1] = static_cast<uint8_t>(color.g * 255);
            dst_pixels[i + 2] = static_cast<uint8_t>(color.b * 255);
            dst_pixels[i + 3] = 255;
        }
    }

private:
    std::mutex m_busy_mtx;
    size_t m_image_size = 400 * 400 * STBI_rgb_alpha;
    std::vector<uint8_t> m_cover_art_pixels {};
    std::shared_ptr<Palette> m_palette;

    // Threads
    std::optional<std::future<void>> m_reset_future_opt {};
    std::optional<std::future<void>> m_load_future_opt {};
    // std::thread* m_reset_thread = nullptr;
    // std::thread* m_load_thread = nullptr;

    void reset_aux(const glm::vec3 color) {
        std::lock_guard guard(m_busy_mtx);

        for (int i = 0; i < m_image_size; i += 4) {
            m_cover_art_pixels[i + 0] = static_cast<uint8_t>(color.r * 255);
            m_cover_art_pixels[i + 1] = static_cast<uint8_t>(color.g * 255);
            m_cover_art_pixels[i + 2] = static_cast<uint8_t>(color.b * 255);
            m_cover_art_pixels[i + 3] = 255;
        }
        m_palette->try_reset_palette();
    }
    void load_aux(const std::string &url) {
        std::lock_guard guard(m_busy_mtx);

        const auto res_data = Communication::load_cover_art(url);
        int w, h, c = 0;
        uint8_t *pixel_data = stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(res_data.c_str()),
                                                    static_cast<int>(res_data.size()), &w, &h, &c,
                                                    STBI_rgb_alpha);
        // NOTE: EVEN IF PIXEL DATA IS RGBA `c == STBI_rgb_alpha` IS NOT NECESSARILY TRUE?
        // TODO: CHECK ACTUAL CHANNEL COUNT AND ADJUST FOR LOOP
        const auto len = std::min(static_cast<int>(m_image_size), w * h * STBI_rgb_alpha);
        memcpy(m_cover_art_pixels.data(), pixel_data, len);

        stbi_image_free(pixel_data);

        m_palette->generate_target(m_cover_art_pixels, m_image_size, true);
    }

    static void set_palette(const std::vector<uint8_t> &pixels, ColorPalette &palette, const float threshold = 0.15,
                            const bool alpha_channel = true)
    {
        std::vector<ColorGroup> color_groups {};
        const size_t offset = alpha_channel ? 4 : 3;

        // Set groups
        for (size_t i = 0; i < pixels.size(); i += offset) {
            glm::vec3 pixel {};
            pixel.r = static_cast<float>(pixels[i+0]) / 255.0f;
            pixel.g = static_cast<float>(pixels[i+1]) / 255.0f;
            pixel.b = static_cast<float>(pixels[i+2]) / 255.0f;

            // Check if pixel fits into group
            int group_index = -1;
            float min_distance = threshold;
            for (int j = 0; j < color_groups.size(); j++) {
                float distance = glm::distance(pixel, color_groups[j].avg_color);
                if (distance < min_distance) {
                    min_distance = distance;
                    group_index = j;
                }
            }

            // New group
            if (group_index == -1) {
                using namespace std;
                cout << "\tNEW COLOR: " << pixel.r << "\t" << pixel.g << "\t" << pixel.b << "\t" << endl;
                color_groups.push_back(ColorGroup {
                    .avg_color = pixel,
                    .count = 1,
                });
                continue;
            }

            // Adjust group
            auto color_group = color_groups[group_index];
            size_t new_count = color_group.count + 1;
            color_groups[group_index].avg_color =
                    (static_cast<float>(color_group.count) * color_group.avg_color + pixel)
                    / static_cast<float>(new_count);
            color_groups[group_index].count = new_count;
        }

        // Pick most occurring groups
        std::priority_queue<ColorGroupByCount, std::vector<ColorGroupByCount>, std::greater<>> palette_by_cnt {};
        for (auto group : color_groups) {
            palette_by_cnt.emplace(group);
            if (palette_by_cnt.size() > PALETTE_SIZE) palette_by_cnt.pop();
        }

        // Sort by saturation
        std::priority_queue<ColorGroupBySaturation, std::vector<ColorGroupBySaturation>, std::greater<>> palette_by_sat {};
        for (; !palette_by_cnt.empty(); palette_by_cnt.pop())
        {
            palette_by_sat.emplace(palette_by_cnt.top());
        }

        // Set default in case palette_by_sat has size zero (just in case) or 1
        palette.dark = palette_by_sat.top().avg_color;
        palette.light = palette_by_sat.top().avg_color;
        palette.third = palette_by_sat.top().avg_color;
        palette.second = palette_by_sat.top().avg_color;
        palette.main = palette_by_sat.top().avg_color;


        std::vector<glm::vec3*> light_dst {};
        std::vector<glm::vec3*> main_dst {};

        if (palette_by_sat.size() == 1) {
            using namespace std;
            cout << "\tgroups: 1" << endl;
            main_dst.push_back(&palette.dark);
            main_dst.push_back(&palette.light);
            main_dst.push_back(&palette.third);
            main_dst.push_back(&palette.second);
            main_dst.push_back(&palette.main);
        } else if (palette_by_sat.size() == 2) {
            using namespace std;
            cout << "\tgroups: 2" << endl;
            light_dst.push_back(&palette.dark);
            main_dst.push_back(&palette.light);
            main_dst.push_back(&palette.third);
            main_dst.push_back(&palette.second);
            main_dst.push_back(&palette.main);
        } else if (palette_by_sat.size() >= 3) {
            using namespace std;
            cout << "\tgroups: " << palette_by_sat.size() << endl;
            light_dst.push_back(&palette.dark);
            light_dst.push_back(&palette.light);
            main_dst.push_back(&palette.third);
            main_dst.push_back(&palette.second);
            main_dst.push_back(&palette.main);
        }

        // Sort least saturated by light
        std::priority_queue<ColorGroupByLight, std::vector<ColorGroupByLight>, std::greater<>> low_palette_by_lgt {};
        for (size_t i = 0; i < light_dst.size() && !palette_by_sat.empty(); i++)
        {
            low_palette_by_lgt.emplace(palette_by_sat.top());
            if (palette_by_sat.size() > 1) palette_by_sat.pop();
        }

        // Pick dark and light
        for (size_t i = 0; i < light_dst.size() && !low_palette_by_lgt.empty(); i++) {
            *(light_dst[i]) = low_palette_by_lgt.top().avg_color;
            if (low_palette_by_lgt.size() > 1) low_palette_by_lgt.pop();
        }

        // Sort rest by count
        std::priority_queue<ColorGroupByCount, std::vector<ColorGroupByCount>, std::greater<>> main_palette_by_cnt {};
        for (size_t i = 0; i < main_dst.size() && !palette_by_sat.empty(); i++)
        {
            main_palette_by_cnt.emplace(palette_by_sat.top());
            if (palette_by_sat.size() > 1) palette_by_sat.pop();
        }

        // Pick main, second and third
        for (size_t i = 0; i < main_dst.size() && !main_palette_by_cnt.empty(); i++) {
            *(main_dst[i]) = main_palette_by_cnt.top().avg_color;
            if (main_palette_by_cnt.size() > 1) main_palette_by_cnt.pop();
        }
    }
};

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
        m_palette = std::make_shared<StandardPalette>(4);
        m_cover_art = new CoverArt(400 * 400 * STBI_rgb_alpha, m_palette);
        m_cover_art->set_default(m_cover_art_pixels, glm::vec3(1.0f));
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
            CornerColors corner_colors = {
                glm::vec4(palette.second, 1.0),
                glm::vec4(palette.second, 1.0),
                glm::vec4(palette.second, 1.0),
                glm::vec4(palette.second, 1.0),
                glm::vec4(palette.dark, 1.0),
            };
            const auto back_drop = m_vis->get_sprite("back_drop");
            for (size_t j = 0; j < m_image_count; j++) {
                back_drop->set_buffer(j, 1, &corner_colors, sizeof(CornerColors));
            }

            for (size_t i = 0; i < m_bar_count; i++) {
                const auto sp = m_vis->get_sprite(BAR_NAMES[i]);

                for (size_t j = 0; j < m_image_count; j++) {
                    sp->set_buffer(j, 3, &palette.main, sizeof(glm::vec3));
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
    std::shared_ptr<StandardPalette> m_palette;


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
