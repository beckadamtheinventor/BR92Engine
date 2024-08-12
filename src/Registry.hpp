#pragma once

#include "DynamicArray.hpp"
#include "Dictionary.hpp"

template <class T>
class Registry {
    protected:
    DynamicArray<T*> tiles;
    Dictionary<unsigned short> dict;
    unsigned short nextid() {
        return tiles.length();
    }
    public:
    bool has(unsigned short id) {
        return id < nextid();
    }
    bool has(const char* id) {
        return dict.has(id);
    }
    T* of(unsigned short id) {
        if (id < tiles.length()) {
            return tiles[id];
        }
        return nullptr;
    }
    T* of(const char* id) {
        if (dict.has(id)) {
            return tiles[dict[id]];
        }
        return nullptr;
    }
    T* add(const char* id) {
        T* tile = new T();
        tile->id = nextid();
        tiles.append(tile);
        dict[id] = tile->id;
        return tile;
    }
};