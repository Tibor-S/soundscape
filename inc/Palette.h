//
// Created by Sebastian Sandstig on 2025-01-08.
//

#ifndef PALETTE_H
#define PALETTE_H

#include <iostream>
#include <mutex>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <Globals.h>
#include <map>
#include <queue>
#include <glm/mat4x4.hpp>
#include <__ranges/elements_view.h>
// #include <glm/ext/quaternion_common.hpp>

class ColorGroupCount {
public:
    ColorGroupCount() {
        m_count = 0;
    }

    explicit ColorGroupCount(const size_t c) {
        m_count = c;
    }

    [[nodiscard]] size_t count() const { return m_count; }

    bool operator >(const ColorGroupCount& rhs) const {
        return m_count > rhs.m_count;
    }
    bool operator <(const ColorGroupCount& rhs) const {
        return m_count < rhs.m_count;
    }
    bool operator ==(const ColorGroupCount& rhs) const {
        return m_count == rhs.m_count;
    }

protected:
    void increment() {
        m_count++;
    }
    void increment(const size_t c) {
        m_count += c;
    }
    void decrement() {
        m_count--;
    }
    void decrement(const size_t c) {
        m_count -= c;
    }
    void scale(const float f) {
        m_count = static_cast<size_t>(static_cast<float>(m_count) * f);
    }
private:
    size_t m_count;

};

class ColorGroupLight {
public:
    ColorGroupLight() = default;
    explicit ColorGroupLight(const float l) {
        set_light(l);
    }
    ColorGroupLight(const float r, const float g, const float b) {
        set_light(r, g, b);
    }

    [[nodiscard]] float c_max() const {
        return m_c_max;
    }
    [[nodiscard]] float c_min() const {
        return m_c_min;
    }
    [[nodiscard]] float light() const {
        return m_light;
    }

    bool operator >(const ColorGroupLight& rhs) const {
        return light() > rhs.light();
    }
    bool operator <(const ColorGroupLight& rhs) const {
        return light() < rhs.light();
    }

protected:
    void set_light(const float l) {
        m_light = l;
        m_c_max = l;
        m_c_min = l;
    }
    void set_light(const float l, const float delta) {
        m_light = l;
        m_c_max = delta / 2 + m_light;
        m_c_min = m_c_max - delta;
    }
    void set_light(const float r, const float g, const float b) {
        m_c_max = std::max({r, g, b});
        m_c_min = std::min({r, g, b});
        m_light = (m_c_max + m_c_min) / 2;
    }
private:
    float m_c_max = 0;
    float m_c_min = 0;
    float m_light = 0;
};

class ColorGroupSaturation : public ColorGroupLight {
public:
    ColorGroupSaturation() = default;
    explicit ColorGroupSaturation(const float s) {
        set_saturation(s);
    }
    ColorGroupSaturation(const float s, const float l) {
        set_saturation(s, l);
    }
    explicit ColorGroupSaturation(const float r, const float g, const float b) : ColorGroupLight(r, g, b) {
        m_delta = c_max() - c_min();
        const auto divisor = (1 - abs(2 * light() - 1));
        m_saturation = divisor == 0 ? 0 : m_delta / divisor;
    };

    [[nodiscard]] float delta() const {
        return m_delta;
    }
    [[nodiscard]] float saturation() const {
        return m_saturation;

    }

    bool operator >(const ColorGroupSaturation& rhs) const {
        return saturation() > rhs.saturation();
    }
    bool operator <(const ColorGroupSaturation& rhs) const {
        return saturation() < rhs.saturation();
    }

protected:
    void set_saturation(const float s) {
        m_saturation = s;
        set_light(1);
    }
    void set_saturation(const float s, const float l) {
        m_saturation = s;
        m_delta = m_saturation * (1 - abs(2 * l - 1));
        set_light(l, m_delta);
    }
    void set_saturation(const float r, const float g, const float b) {
        set_light(r, g, b);
        const auto divisor = (1 - abs(2 * light() - 1));
        m_saturation = divisor == 0 ? 0 : m_delta / divisor;
    }

private:
    float m_delta = 0;;
    float m_saturation = 0;
};

class ColorGroupHue : public ColorGroupSaturation {
public:
    // enum RADIANS_RANGE {
    //     UNSIGNED ,
    //     SIGNED
    // };

    ColorGroupHue() = default;
    explicit
    ColorGroupHue(const float r, const float g, const float b) :
                  ColorGroupSaturation(r, g, b)
    {
        m_hue = pick_hue(r, g, b, c_max(), delta());
    }

    [[nodiscard]] float hue() const {
        return m_hue;
    }
    [[nodiscard]] float hue_signed() const {
        return m_hue <= M_PI ? m_hue : -2 * static_cast<float>(M_PI) + m_hue;
    }

    bool operator >(const ColorGroupHue& rhs) const {
        return hue() > rhs.hue();
    }
    bool operator <(const ColorGroupHue& rhs) const {
        return hue() < rhs.hue();
    }
protected:
    void to_rgb(float* r, float* g, float* b) const {
        const float c = (1 - abs(2 * light() -1)) * saturation();
        const float x = c * (1 - abs(glm::mod(hue(), static_cast<float>(M_PI) / 3) - 1));
        switch (static_cast<size_t>(3.0f * hue() / M_PI) % 6) {
            case 0: *r = c; *g = x; *b = 0; break;
            case 1: *r = x; *g = c; *b = 0; break;
            case 2: *r = 0; *g = c; *b = x; break;
            case 3: *r = 0; *g = x; *b = c; break;
            case 4: *r = x; *g = 0; *b = c; break;
            case 5: *r = c; *g = 0; *b = x; break;
            default: break;
        }
        const float m = light() - c / 2;
        *r += m; *g += m; *b += m;
    }
    void set_hue(const float r, const float g, const float b) {
        set_saturation(r, g, b);
        m_hue = pick_hue(r, g, b, c_max(), delta());
    }
    void set_hue(const float radians) {
        m_hue = glm::mod(radians, 2 * static_cast<float>(M_PI));
    }
    void rotate_right(const float radians) {
        set_hue(m_hue - radians);
    }

private:
    float m_hue = 0; // radians

    static size_t closest_channel(const float r, const float g, const float b, const float c_max) {
        glm::vec3 channels(r,g,b);
        channels -= c_max;
        channels = abs(channels);

        return channels.r < channels.g
                    ? channels.r < channels.b
                        ? 0
                        : 2
                    : channels.g < channels.b
                        ? 1
                        : 2;
    }
    static float pick_hue(const float r, const float g, const float b, const float c_max, const float delta) {
        float hue = 0;
        if (delta == 0) {
            return hue;
        }
        switch (closest_channel(r, g, b, c_max)) {
            case 0: hue = glm::mod((g - b) / delta, 6.0f); break;
            case 1: hue = (b - r) / delta + 2; break;
            case 2: hue = (r - g) / delta + 4; break;
            default: hue = 0; // Should never happen...
        }
        hue *= static_cast<float>(M_PI) / 3;
        return hue;
    }
};

class ColorGroup : public ColorGroupCount, public ColorGroupHue {
public:
    ColorGroup() = default;
    explicit ColorGroup(const glm::vec3 color) : ColorGroupCount(1), ColorGroupHue(color.r, color.g, color.b) {
        m_color = color;
        std::cout << "org_col: " << m_color.x << " " << m_color.y << " " << m_color.z << std::endl;
    }

    [[nodiscard]] glm::vec3 get_color() const { return m_color; }
    void rotate_hue_right(const float hue) {
        rotate_right(hue);
        to_rgb(&m_color.r, &m_color.g, &m_color.b);
    }
    void append_color(const glm::vec3 color) {
        m_color = (static_cast<float>(count()) * m_color + color) / static_cast<float>(count() + 1);
        increment();
        set_hue(m_color.r, m_color.g, m_color.b);
    }
    void remove_color(const glm::vec3 color) {
        m_color = (static_cast<float>(count()) * m_color - color) / static_cast<float>(count() - 1);
        decrement();
        set_hue(m_color.r, m_color.g, m_color.b);
    }
private:
    glm::vec3 m_color = glm::vec3(1.0f);
};

class ColorCube : protected std::map<std::array<uint8_t, 3>, ColorGroup> {
public:
    typedef std::array<uint8_t, 3> key_t;

    ColorCube() = default;
    ColorCube(const std::vector<uint8_t> &pixels, uint8_t relevant_bits, bool alpha_channel);
    ~ColorCube() = default;

    ColorGroup& voxel_ref(const key_t position) { return at(position); }
    template<class T>
    std::optional<ColorGroup> get_voxel_by_highest_param() {
        std::optional<ColorGroup> highest_grp = std::nullopt;
        for (auto &[_, grp] : *this) {
            if (!highest_grp.has_value()) {
                highest_grp = grp;
                continue;
            }
            if (static_cast<T>(grp) > static_cast<T>(highest_grp.value())) {
                highest_grp = grp;
            }
        }
        return highest_grp;
    }
    template<class T>
    std::optional<ColorGroup> remove_voxel_by_highest_param() {
        std::optional<ColorGroup> highest_grp = std::nullopt;
        key_t highest_pos = {UINT8_MAX, UINT8_MAX, UINT8_MAX};
        for (auto &[pos, grp] : *this) {
            if (!highest_grp.has_value()) {
                highest_grp = grp;
                highest_pos = pos;
                continue;
            }
            if (static_cast<T>(grp) > static_cast<T>(highest_grp.value())) {
                highest_grp = grp;
                highest_pos = pos;
            }
        }
        remove_voxel(highest_pos);
        return highest_grp;
    }
    void remove_voxel(const std::array<uint8_t, 3> position) { erase(position); }

    [[nodiscard]] bool empty() const { return map::empty(); };

    template<class T>
    void clone_values(std::priority_queue<ColorGroup, std::vector<ColorGroup>, T>& container) {
        for (auto &[_, grp] : *this) {
            container.push(grp);
        }
    }
};

class BgColorCube : public ColorCube {
public:
    BgColorCube(const std::vector<uint8_t> &pixels, uint8_t relevant_bits, size_t width, bool alpha_channel);
    ~BgColorCube() = default;

    ColorGroup& bg_voxel() { return voxel_ref(m_bg_most_important); }
    void remove_bg_voxel() { remove_voxel(m_bg_most_important); }
private:
    std::array<uint8_t, 3> m_bg_most_important = {0, 0, 0};
};


struct PaletteAnalogous {
    glm::vec3 main;
    glm::vec3 comp;
    glm::vec3 comp_mirror;

    static PaletteAnalogous default_palette() {
        return {glm::vec3(1.0f), glm::vec3(1.0f)};
    };
    void lerp(const PaletteAnalogous& other, const float fac) {
        comp = glm::mix(comp, other.comp, fac);
        comp_mirror = glm::mix(comp_mirror, other.comp_mirror, fac);
        main = glm::mix(main, other.main, fac);
    }
};

struct Palette4x {
    glm::vec3 dark;
    glm::vec3 light;
    glm::vec3 comp;
    glm::vec3 main;

    static Palette4x default_palette() {
        return {glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
    };
    void lerp(const Palette4x& other, const float fac) {
        dark = glm::mix(dark, other.dark, fac);
        light = glm::mix(light, other.light, fac);
        comp = glm::mix(comp, other.comp, fac);
        main = glm::mix(main, other.main, fac);
    }
};

class Palette {
public:
    Palette() = default;
    virtual ~Palette() = default;

    virtual void generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) {
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

template<typename T>
class LerpApproachPalette : public Palette {
public:
    LerpApproachPalette() {
        m_palette = T::default_palette();
        m_target = T::default_palette();
    };
    explicit LerpApproachPalette(const Palette4x& base) {
        try_set_palette(base);
    }
    bool try_approach_target(float factor) final {
        if (!try_lock()) return false;
        m_palette.lerp(m_target, factor);
        unlock();
        return true;
    }
    bool try_set_palette(const T& palette) {
        if (!try_lock()) return false;
        m_palette = palette;
        m_target = palette;
        unlock();
        return true;
    }
    [[nodiscard]] std::optional<T> try_get_palette() {
        if (!try_lock()) return {};
        auto palette = m_palette;
        unlock();
        return palette;
    }

    bool try_reset_palette() override {
        if (!try_lock()) return false;
        m_palette = T::default_palette();
        m_target = T::default_palette();
        unlock();
        return true;
    }

protected:
    T m_palette {};
    T m_target {};
};

class StandardPalette final : public LerpApproachPalette<Palette4x> {
public:
    explicit
    StandardPalette(const uint8_t relevant_bits) : m_relevant_bits(relevant_bits) {}
    ~StandardPalette() override = default;

    void generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) override;
private:
    uint8_t m_relevant_bits = 1;
};

class SaturatedPalette final : public LerpApproachPalette<Palette4x> {
public:
    explicit
    SaturatedPalette(const uint8_t relevant_bits) : m_relevant_bits(relevant_bits) {}
    ~SaturatedPalette() override = default;

    void generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) override;
private:
    uint8_t m_relevant_bits = 1;

};

class AnalogousPalette final : public LerpApproachPalette<PaletteAnalogous> {
public:
    explicit
    AnalogousPalette(const size_t image_width, const uint8_t relevant_bits) : m_image_width(image_width),
                                                                              m_relevant_bits(relevant_bits) {}
    ~AnalogousPalette() override = default;

    void generate_target(const std::vector<uint8_t> &pixels, bool alpha_channel) override;
private:
    const float OPTIMAL_HUE_DIF = M_PI / 6;

    size_t m_image_width = 0;
    uint8_t m_relevant_bits = 1;

};

#endif //PALETTE_H
