#include "sim.hpp"
#include <algorithm>
#include <cmath>

static inline uint32_t packRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255){
    return (uint32_t(a)<<24) | (uint32_t(r)<<16) | (uint32_t(g)<<8) | uint32_t(b);
}

static inline uint32_t colorFor(Element e){
    switch(e){
        case Element::Sand:  return packRGBA(200,170,100);
        case Element::Water: return packRGBA( 60,100,220);
        case Element::Stone: return packRGBA(120,120,130);
        default:             return packRGBA(  8,  8, 12);
    }
}

Simulator::Simulator(const SimConfig& cfg): cfg_(cfg),
    grid_(cfg.width * cfg.height, (uint16_t)Element::Empty),
    gridNext_(cfg.width * cfg.height, (uint16_t)Element::Empty),
    framebuffer_(cfg.width * cfg.height, 0),
    rng_(std::random_device{}())
{
    render();
}

void Simulator::reset(){
    std::fill(grid_.begin(), grid_.end(), (uint16_t)Element::Empty);
    std::fill(gridNext_.begin(), gridNext_.end(), (uint16_t)Element::Empty);
    render();
}

Element Simulator::get(int x, int y) const {
    return (Element)grid_[idx(x,y)];
}

Element Simulator::getNext(int x, int y) const {
    return (Element)gridNext_[idx(x,y)];
}

void Simulator::setNext(int x, int y, Element e){
    gridNext_[idx(x,y)] = (uint16_t)e;
}

void Simulator::swapGrids(){
    grid_.swap(gridNext_);
}

void Simulator::tick(){
    // randomize update direction slightly to reduce bias
    std::uniform_int_distribution<int> coin(0,1);
    std::fill(gridNext_.begin(), gridNext_.end(), (uint16_t)Element::Empty);

    // bottom-up for gravity effects
    for(int y = cfg_.height-1; y >= 0; --y){
        int xStart = coin(rng_) ? 0 : 1; // checkerboardish
        for(int x = xStart; x < cfg_.width; x += 2){
            updateCell(x, y, coin);
        }
        xStart = 1 - xStart;
        for(int x = xStart; x < cfg_.width; x += 2){
            updateCell(x, y, coin);
        }
    }

    swapGrids();
    render();
}

// Looser target checks to allow cascading falls within the same tick.
// We mainly require gridNext to be empty; grid_ may still show an occupant
// that already moved earlier in this tick.
void Simulator::updateCell(int x, int y, std::uniform_int_distribution<int>& coin){
    Element e = get(x,y);
    if(e == Element::Empty){
        return;
    }

    auto canWriteNext = [&](int tx, int ty){
        return inBounds(tx,ty) && getNext(tx,ty) == Element::Empty;
    };

    auto try_set = [&](int tx, int ty, Element val){
        if(canWriteNext(tx,ty)){
            setNext(tx,ty,val);
            return true;
        }
        return false;
    };

    auto isWaterNow = [&](int tx, int ty){
        return inBounds(tx,ty) && get(tx,ty) == Element::Water && getNext(tx,ty)==Element::Empty;
    };

    if(e == Element::Sand){
        int belowY = y+1;
        if(try_set(x, belowY, Element::Sand)) return;
        if(isWaterNow(x, belowY)){
            setNext(x, belowY, Element::Sand);
            setNext(x, y, Element::Water);
            return;
        }
        int dir = coin(rng_) ? 1 : -1;
        if(try_set(x+dir, belowY, Element::Sand)) return;
        if(isWaterNow(x+dir, belowY)){
            setNext(x+dir, belowY, Element::Sand);
            setNext(x, y, Element::Water);
            return;
        }
        try_set(x, y, Element::Sand);
        return;
    }

    if(e == Element::Water){
        int belowY = y+1;
        if(try_set(x, belowY, Element::Water)) return;
        int dir = coin(rng_) ? 1 : -1;
        if(try_set(x+dir, belowY, Element::Water)) return;
        if(try_set(x-dir, belowY, Element::Water)) return;
        if(try_set(x+dir, y, Element::Water)) return;
        if(try_set(x-dir, y, Element::Water)) return;
        try_set(x, y, Element::Water);
        return;
    }

    if(e == Element::Stone){
        try_set(x, y, Element::Stone);
        return;
    }
}

void Simulator::paint(int cx, int cy, int radius, Element e, bool allowOverwrite){
    int r2 = radius * radius;
    for(int y = cy - radius; y <= cy + radius; ++y){
        for(int x = cx - radius; x <= cx + radius; ++x){
            int dx = x - cx, dy = y - cy;
            if(dx*dx + dy*dy <= r2 && inBounds(x,y)){
                int i = idx(x,y);
                if(e == Element::Empty){
                    grid_[i] = (uint16_t)Element::Empty;
                    gridNext_[i] = (uint16_t)Element::Empty; // keep buffers in sync
                }else{
                    // Only write into Empty unless explicit overwrite is allowed
                    if(allowOverwrite || grid_[i] == (uint16_t)Element::Empty){
                        grid_[i] = (uint16_t)e;
                        gridNext_[i] = (uint16_t)e; // sync next to avoid one-tick artifacts
                    }
                }
            }
        }
    }
    render();
}

void Simulator::render(){
    // map elements to pixels
    for(int y=0; y<cfg_.height; ++y){
        for(int x=0; x<cfg_.width; ++x){
            Element e = (Element)grid_[idx(x,y)];
            uint32_t c;
            switch(e){
                case Element::Sand:  c = packRGBA(200,170,100); break;
                case Element::Water: c = packRGBA( 60,100,220); break;
                case Element::Stone: c = packRGBA(120,120,130); break;
                default:             c = packRGBA(  8,  8, 12); break;
            }
            framebuffer_[idx(x,y)] = c;
        }
    }
}
