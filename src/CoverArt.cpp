//
// Created by Sebastian Sandstig on 2025-01-08.
//

#include <CoverArt.h>

#include <cstring>
#include <iostream>
#include <map>
#include <queue>

#include <communication/content_download.h>

CoverArt::~CoverArt() {
    if (m_reset_future_opt.has_value()) m_reset_future_opt.value().get();
    if (m_load_future_opt.has_value()) m_load_future_opt.value().get();
};

bool CoverArt::try_reset(const glm::vec3 color) {
    using namespace std::chrono_literals;
    if (m_reset_future_opt.has_value()) {
        if (m_reset_future_opt.value().wait_for(0ms) != std::future_status::ready) return false;
    }
    m_reset_future_opt = std::async(std::launch::async, &CoverArt::reset_aux, this, color);
    return true;
}
bool CoverArt::try_load(const std::string &url) {
    using namespace std::chrono_literals;
    if (m_load_future_opt.has_value()) {
        if (m_load_future_opt.value().wait_for(0ms) != std::future_status::ready) return false;
    }
    m_load_future_opt = std::async(std::launch::async, &CoverArt::load_aux, this, url);
    return true;
}
// Make sure dst_pixels is allocated with at least the size of the image
bool CoverArt::try_approach(std::vector<uint8_t> &dst_pixels, const float factor)
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
void CoverArt::set_default(std::vector<uint8_t>& dst_pixels, const glm::vec3 color) const {
    dst_pixels.resize(m_image_size);
    for (size_t i = 0; i < m_image_size; i += 4) {
        dst_pixels[i + 0] = static_cast<uint8_t>(color.r * 255);
        dst_pixels[i + 1] = static_cast<uint8_t>(color.g * 255);
        dst_pixels[i + 2] = static_cast<uint8_t>(color.b * 255);
        dst_pixels[i + 3] = 255;
    }
}


void CoverArt::reset_aux(const glm::vec3 color) {
    std::lock_guard guard(m_busy_mtx);

    for (int i = 0; i < m_image_size; i += 4) {
        m_cover_art_pixels[i + 0] = static_cast<uint8_t>(color.r * 255);
        m_cover_art_pixels[i + 1] = static_cast<uint8_t>(color.g * 255);
        m_cover_art_pixels[i + 2] = static_cast<uint8_t>(color.b * 255);
        m_cover_art_pixels[i + 3] = 255;
    }
    m_palette->try_reset_palette();
}
void CoverArt::load_aux(const std::string &url) {
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
void CoverArt::set_palette(const std::vector<uint8_t> &pixels, ColorPalette &palette, const float threshold,
                        const bool alpha_channel)
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
