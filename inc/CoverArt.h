//
// Created by Sebastian Sandstig on 2025-01-08.
//

#ifndef COVERART_H
#define COVERART_H

#include <future>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>

#include <stb_image.h>

#include <Palette.h>

class CoverArt {
public:
    CoverArt(const size_t image_size, const std::shared_ptr<Palette> &palette);
    ~CoverArt();

    [[nodiscard]] size_t image_size() const { return m_image_size; }

    bool try_reset(glm::vec3 color);
    bool try_load(const std::string &url);
    // Make sure dst_pixels is allocated with at least the size of the image
    bool try_approach(std::vector<uint8_t> &dst_pixels, float factor = 0.1f);

    void acquire_default(std::vector<uint8_t>& dst_pixels, glm::vec3 color);
private:
    std::mutex m_busy_mtx;
    size_t m_image_size = 400 * 400 * STBI_rgb_alpha;
    std::vector<uint8_t> m_cover_art_pixels {};
    std::shared_ptr<Palette> m_palette;

    // Threads
    std::optional<std::future<void>> m_reset_future_opt {};
    std::optional<std::future<void>> m_load_future_opt {};

    void reset_aux(glm::vec3 color);
    void load_aux(const std::string &url);
};

#endif //COVERART_H
