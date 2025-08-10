#pragma once
#include <cstdint>
#include <vector>
#include <random>

struct SimConfig {
    int width = 320;
    int height = 180;
};

enum class Element : uint16_t {
    Empty = 0,
    Sand  = 1,
    Water = 2,
    Stone = 3
};

class Simulator {
public:
    explicit Simulator(const SimConfig& cfg);
    void reset();
    void tick();
    void paint(int cx, int cy, int radius, Element e, bool allowOverwrite=false);
    const std::vector<uint32_t>& frame() const { return framebuffer_; }
    int width() const { return cfg_.width; }
    int height() const { return cfg_.height; }

    // hotkey helpers
    void setSelected(Element e) { selected_ = e; }
    Element selected() const { return selected_; }

private:
    SimConfig cfg_;
    std::vector<uint16_t> grid_;     // element ids (current)
    std::vector<uint16_t> gridNext_; // next buffer
    std::vector<uint32_t> framebuffer_;

    std::mt19937 rng_;
    Element selected_ = Element::Sand;

    inline int idx(int x, int y) const { return y * cfg_.width + x; }

    bool inBounds(int x, int y) const {
        return (unsigned)x < (unsigned)cfg_.width && (unsigned)y < (unsigned)cfg_.height;
    }

    void render();
    void swapGrids();

    // movement rules
    void updateCell(int x, int y, std::uniform_int_distribution<int>& coin);
    void setNext(int x, int y, Element e);
    Element get(int x, int y) const;
    Element getNext(int x, int y) const;
};
