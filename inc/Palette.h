//
// Created by Sebastian Sandstig on 2025-01-08.
//

#ifndef PALETTE_H
#define PALETTE_H

#include <mutex>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>

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
    static constexpr ColorPalette DEFAULT = {
        glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)
    };

    explicit StandardPalette(const uint8_t relevant_bits) : m_relevant_bits(relevant_bits) {}
    ~StandardPalette() override = default;


    void generate_target(const std::vector<uint8_t> &pixels, size_t pixel_size, bool alpha_channel) override;
    bool try_approach_target(float factor) override;
    [[nodiscard]] std::optional<ColorPalette> try_get_palette();
    bool try_set_palette(const ColorPalette& palette);
    bool try_reset_palette() override;
private:
    uint8_t m_relevant_bits = 1;
    ColorPalette m_palette {};
    ColorPalette m_target {};
};

#endif //PALETTE_H
