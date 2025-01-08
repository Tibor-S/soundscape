//
// Created by Sebastian Sandstig on 2025-01-08.
//

#include <Palette.h>

#include <map>

void StandardPalette::generate_target(const std::vector<uint8_t> &pixels, size_t pixel_size, bool alpha_channel) {
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

bool StandardPalette::try_approach_target(const float factor) {
    if (!try_lock()) return false;
    m_palette.dark = m_target.dark * factor + m_palette.dark * (1 - factor);
    m_palette.light = m_target.light * factor + m_palette.light * (1 - factor);
    m_palette.third = m_target.third * factor + m_palette.third * (1 - factor);
    m_palette.second = m_target.second * factor + m_palette.second * (1 - factor);
    m_palette.main = m_target.main * factor + m_palette.main * (1 - factor);
    unlock();
    return true;
}

[[nodiscard]] std::optional<ColorPalette> StandardPalette::try_get_palette() {
    if (!try_lock()) return {};
    auto palette = m_palette;
    unlock();
    return palette;
}

bool StandardPalette::try_set_palette(const ColorPalette& palette) {
    if (!try_lock()) return false;
    m_palette = palette;
    m_target = palette;
    unlock();
    return true;
}

bool StandardPalette::try_reset_palette() {
    if (!try_lock()) return false;
    m_palette = DEFAULT;
    m_target = DEFAULT;
    unlock();
    return true;
}