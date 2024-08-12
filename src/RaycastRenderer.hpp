#pragma once

#include "DynamicArray.hpp"
#include "Array2D.hpp"
#include "MapData.hpp"
#include "raylib.h"

class Renderer {
    struct HitInfo {
        float dist;
        unsigned char side;
        unsigned char height;
        unsigned short tex;
    };
    DynamicArray<HitInfo> columns;
    Array2D<Color> pixels;
    MapData* map;
    MapTileRegistry* tiles;
    public:
    Renderer() : Renderer(1, 1) {}
    Renderer(size_t width, size_t height) {
        map = nullptr;
        resize(width, height);
    }
    void resize(size_t width, size_t height) {
        columns.resize(width);
        pixels.resize(width, height);
    }
    void setMap(MapData* data) {
        map = data;
    }
    void setTiles(MapTileRegistry* reg) {
        tiles = reg;
    }
    void renderColumns(Vector3 pos, Vector2 dir, Vector2 plane) {
        for (size_t x=0; x<columns.length(); x++) {
            int mapx = (int)pos.x;
            int mapy = (int)pos.y;
            float cameraX = 2 * x / (float)columns.length() - 1; // x coordinate in camera space
            float rdx = dir.x + plane.x * cameraX;
            float rdy = dir.y + plane.y * cameraX;
            float ddx = (rdx == 0) ? 1e30 : fabs(1 / rdx);
            float ddy = (rdy == 0) ? 1e30 : fabs(1 / rdy);
            char stepX, stepY;
            float sdx, sdy;
            if (rdx < 0) {
                stepX = -1;
                sdx = (pos.x - mapx) * ddx;
            } else {
                stepX = 1;
                sdx = (mapx + 1.0f - pos.x) * ddx;
            }
            if (rdy < 0) {
                stepY = -1;
                sdy = (pos.y - mapy) * ddy;
            } else {
                stepY = 1;
                sdy = (mapy + 1.0f - pos.y) * ddy;
            }
            bool yside = false;
            unsigned short tileid = 0;
            do {
                if (sdx < sdy) {
                    sdx += ddx;
                    mapx += stepX;
                    yside = false;
                } else {
                    sdy += ddy;
                    mapy += stepY;
                    yside = true;
                }
                tileid = map->get(mapx, mapy);
            } while (!tiles->of(tileid)->isWall);
        }
    }
};