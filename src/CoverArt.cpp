//
// Created by Sebastian Sandstig on 2025-01-08.
//

#include <CoverArt.h>

#include <cstring>
#include <iostream>
#include <map>
#include <queue>

#include <communication/content_download.h>


CoverArt::CoverArt(const size_t image_size, const std::shared_ptr<Palette> &palette) : m_image_size(image_size),
    m_cover_art_pixels(image_size),
    m_palette(palette)
{
    for (size_t i = 0; i < m_image_size; i += 4) {
        m_cover_art_pixels[i + 0] = 255;
        m_cover_art_pixels[i + 1] = 255;
        m_cover_art_pixels[i + 2] = 255;
        m_cover_art_pixels[i + 3] = 255;
    }
    m_palette->generate_target(m_cover_art_pixels, true);
}
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
void CoverArt::acquire_default(std::vector<uint8_t>& dst_pixels, const glm::vec3 color) {
    std::lock_guard guard(m_busy_mtx);
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
    m_palette->generate_target(m_cover_art_pixels, true);
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

    m_palette->generate_target(m_cover_art_pixels, true);
}
