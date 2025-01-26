//
// Created by Sebastian Sandstig on 2025-01-08.
//

#include <iostream>
#include <Palette.h>

#include <map>
#include <queue>
#include <__ranges/elements_view.h>

ColorCube::ColorCube(const std::vector<uint8_t> &pixels, const uint8_t relevant_bits, const bool alpha_channel)
{
    const size_t offset = alpha_channel ? 4 : 3;

    // Put in bins
    for (size_t i = 0; i < pixels.size(); i += offset) {
        const auto red = pixels[i+0];
        const auto green = pixels[i+1];
        const auto blue = pixels[i+2];

        const uint8_t red_bin = red >> (8 - relevant_bits);
        const uint8_t green_bin = green >> (8 - relevant_bits);
        const uint8_t blue_bin = blue >> (8 - relevant_bits);

        std::array voxel = {red_bin, green_bin, blue_bin};

        glm::vec3 color {};
        color.r = static_cast<float>(red) / 255.0f;
        color.g = static_cast<float>(green) / 255.0f;
        color.b = static_cast<float>(blue) / 255.0f;

        if (contains(voxel)) {

            at(voxel).append_color(color);
        } else {
            insert_or_assign(voxel, ColorGroup(color));
        }
    }
}

BgColorCube::BgColorCube(const std::vector<uint8_t> &pixels, uint8_t relevant_bits, size_t width, bool alpha_channel)
{
    const size_t offset = alpha_channel ? 4 : 3;
    const size_t height = pixels.size() / (width * offset);
    std::map<std::array<uint8_t, 3>, float> importance {};

    // Put in bins
    const float mid_x = static_cast<float>(width) / 2;
    const float mid_y = static_cast<float>(height) / 2;
    const float max_dist = mid_x + mid_y;
    float highest_fac = 0.0f;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            const float fac = (abs(mid_x - static_cast<float>(x)) + abs(mid_y - static_cast<float>(y))) / max_dist;
            const size_t i = offset * (y * width + x);
            const auto red = pixels[i+0];
            const auto green = pixels[i+1];
            const auto blue = pixels[i+2];

            const uint8_t red_bin = red >> (8 - relevant_bits);
            const uint8_t green_bin = green >> (8 - relevant_bits);
            const uint8_t blue_bin = blue >> (8 - relevant_bits);

            std::array voxel = {red_bin, green_bin, blue_bin};

            glm::vec3 color {};
            color.r = static_cast<float>(red) / 256.0f;
            color.g = static_cast<float>(green) / 256.0f;
            color.b = static_cast<float>(blue) / 256.0f;

            if (contains(voxel)) {
                importance[voxel] *= static_cast<float>(at(voxel).count());
                importance[voxel] += fac;
                at(voxel).append_color(color);
                importance[voxel] /= static_cast<float>(at(voxel).count());
            } else {
                insert_or_assign(voxel, ColorGroup(color));
                importance[voxel] = fac;
            }

            if (importance[voxel] >= highest_fac) {
                highest_fac = importance[voxel];
                m_bg_most_important = voxel;
            }
        }
    }
}


void StandardPalette::generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) {
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
    std::array<uint8_t, 3> c_bin = {0,0,0};
    size_t c_bin_count = 0;
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
    }

    m_target.dark = avg_color[d_bin];
    m_target.light = avg_color[l_bin];
    m_target.comp = avg_color[c_bin];
    m_target.main = avg_color[m_bin];
}


void SaturatedPalette::generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) {
    auto guard = lock();
    const size_t offset = alpha_channel ? 4 : 3;

    ColorCube color_cube(pixels, m_relevant_bits, alpha_channel);

    // Get most saturated
    std::priority_queue<ColorGroup, std::vector<ColorGroup>, std::less<ColorGroupSaturation>> most_saturated {};
    color_cube.clone_values(most_saturated);

    // Default in case there are few bins
    m_target.main = most_saturated.top().get_color();
    m_target.comp = most_saturated.top().get_color();
    m_target.dark = most_saturated.top().get_color();
    m_target.main = most_saturated.top().get_color();

    // Sort by lightness
    auto darkest = ColorGroupLight(1);
    auto lightest = ColorGroupLight(0);
    auto main_saturation = ColorGroupSaturation(0);
    auto comp_saturation = ColorGroupSaturation(0);
    constexpr size_t count = sizeof(Palette4x) / sizeof(Palette4x::main);
    for (size_t i = 0; i < count && !most_saturated.empty(); i++, most_saturated.pop()) {
        const auto color_group = most_saturated.top();
        const auto by_light = static_cast<ColorGroupLight>(color_group);
        const auto by_saturation = static_cast<ColorGroupSaturation>(color_group);
        if (by_light < darkest) {
            darkest = by_light;
            m_target.dark = color_group.get_color();
        }
        if (by_light > lightest) {
            lightest = by_light;
            m_target.light = color_group.get_color();
        }
        if (by_saturation > main_saturation) {
            comp_saturation = main_saturation;
            m_target.comp = m_target.main;
            main_saturation = by_saturation;
            m_target.main = color_group.get_color();
        } else if (by_saturation > comp_saturation) {
            comp_saturation = by_saturation;
            m_target.comp = color_group.get_color();
        }
    }
}

void AnalogousPalette::generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) {
    auto guard = lock();
    const size_t offset = alpha_channel ? 4 : 3;

    BgColorCube color_cube(pixels, m_relevant_bits, m_image_width, alpha_channel);

    auto pivot = color_cube.bg_voxel();
    m_target.main = pivot.get_color();
    color_cube.remove_bg_voxel();
    std::cout << "fin_col: " << pivot.get_color().x << " " << pivot.get_color().y << " " << pivot.get_color().z << std::endl;


    auto comp_opt = color_cube.remove_voxel_by_highest_param<ColorGroupSaturation>();
    if(!comp_opt.has_value()) {
        ColorGroup left(pivot);
        ColorGroup right(pivot);
        left.rotate_hue_right(-OPTIMAL_HUE_DIF);
        right.rotate_hue_right(OPTIMAL_HUE_DIF);
        m_target.comp = left.get_color();
        m_target.comp_mirror = right.get_color();

        return;
    }

    auto comp = comp_opt.value();
    std::cout << "piv_hue: " << pivot.hue() << std::endl;
    std::cout << "cmp_hue: " << comp.hue() << std::endl;
    comp.rotate_hue_right(pivot.hue()); // Hue relative to pivot
    std::cout << "rot_hue: " << comp.hue_signed() << std::endl;

    auto next_comp_opt = color_cube.remove_voxel_by_highest_param<ColorGroupSaturation>();
    while (glm::abs(comp.hue_signed()) < OPTIMAL_HUE_DIF && next_comp_opt.has_value()) {
        auto next_comp = next_comp_opt.value();
        std::cout << "rot_hue: " << next_comp.hue_signed() << std::endl;
        next_comp.rotate_hue_right(pivot.hue()); // Hue relative to pivot
        if (glm::abs(next_comp.hue_signed()) >= OPTIMAL_HUE_DIF) {
            comp = next_comp;
            break;
        }

        const float curr_dist = glm::distance(glm::abs(comp.hue_signed()), OPTIMAL_HUE_DIF);
        const float next_dist = glm::distance(glm::abs(next_comp.hue_signed()), OPTIMAL_HUE_DIF);
        if (next_dist < curr_dist) {
            comp = next_comp;
        }

        next_comp_opt = color_cube.remove_voxel_by_highest_param<ColorGroupSaturation>();
    }

    auto hue_dif = comp.hue_signed();
    std::cout << "fin_hue: " << comp.hue_signed() << std::endl;
    comp.rotate_hue_right(-pivot.hue()); // Rewind from previous alter
    auto comp2 = ColorGroup(comp);
    comp2.rotate_hue_right(hue_dif);

    m_target.comp = comp.get_color();
    m_target.comp_mirror = comp2.get_color();

}