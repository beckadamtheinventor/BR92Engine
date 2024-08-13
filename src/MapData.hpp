#pragma once

#include "DynamicArray.hpp"
#include "Array2D.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "Buffer.hpp"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "raymath.h"
#include "external/glad.h"

// "SECT"
#define SECTIONED_MAP_MAGIC_NUMBER   0x54434553
// "LARG"
#define LARGE_MAP_MAGIC_NUMBER       0x4752414C

#define LIGHT_RANGE 8

class IntMesh {
    public:
    unsigned int vertexCount = 0;
    unsigned int triangleCount = 0;
    unsigned int vbo[2] = {0, 0};
    unsigned int vao = 0;
    unsigned int* verts = nullptr;
    unsigned short* indices = nullptr;
};

class PlacedLight {
    public:
    int x, y, z;
    unsigned char v, r, g, b;
};

class MapTile {
    public:
    unsigned short id;
    union {
        unsigned short flags;
        struct {
            bool isSolid : 1;
            bool isWall : 1;
            bool isSpawnable : 1;
            bool blocksLight : 1;
        };
    };
    unsigned char light, tintr, tintg, tintb;
    unsigned short floor, ceiling, wall;
};

class MapTileRegistry : public Registry<MapTile> {};

class TileArray : public Array2D<unsigned short> {
    public:
    unsigned short getOrDefault(ArrayIndex i, unsigned short d=0) {
        if (i.x >= 0 && i.y >= 0 && i.x < w && i.y < h) {
            return get(i);
        }
        return d;
    }
};

class LightMap : public Array2D<Color> {
    public:
    LightMap() {}
    LightMap(int width, int height) {
        w = width;
        h = height;
        l = w * h;
        values = new Color[l];
        for (size_t i=0; i<l; i++) {
            values[i] = {255, 255, 255, 255};
        }
    }
};

struct HitInfo {
    unsigned short tileid;
    union {
        unsigned char flags;
        struct {
            bool hitFloor : 1;
            bool hitCeiling : 1;
            bool hitWall : 1;
        };
    };
};

class MapData {
    DynamicArray<Vector3> positions;
    DynamicArray<TileArray> maps;
    DynamicArray<IntMesh*> meshes;
    DynamicArray<LightMap*> lightmaps;
    DynamicArray<PlacedLight> lightList;
    Texture2D atlas = {0};
    MapTileRegistry* tileRegistry = nullptr;
    TextureRegistry* textureRegistry = nullptr;
    Shader mainShader;
    unsigned int depthTextureId;
    public:
    float fogMin, fogMax, fogColor[4], lightLevel, renderDistance;
    void BuildAtlas();
    void InitMesher(unsigned int depthTextureId);
    bool LoadMap(RBuffer<char>& data);
    bool LoadMapSection(RBuffer<char>& data);
    Vector3 rayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps=100);
    void GenerateMesh();
    void BuildLighting();
    void UploadMap(size_t mapno);
    void UploadMap() {
        for (size_t i=0; i<meshes.length(); i++) {
            UploadMap(i);
        }
    }
    void TileRegistry(MapTileRegistry* reg) {
        tileRegistry = reg;
    }
    void TextureRegistry(TextureRegistry* reg) {
        textureRegistry = reg;
    }
    unsigned short get(Vector3 pos) {
        return get(pos.x, pos.y, pos.z);
    }
    unsigned short get(int x, int y, int z) {
        for (size_t i=0; i<maps.length(); i++) {
            Vector3 p = positions[i];
            if (x >= p.x && y == p.y && z >= p.z) {
                TileArray& map = maps[i];
                if (x-p.x < map.width() && z-p.z < map.height()) {
                    return map[{x-(int)p.x, z-(int)p.z}];
                }
            }
        }
        return 0;
    }
    void setLight(Vector3 pos, unsigned char v, unsigned char r, unsigned char g, unsigned char b) {
        Color* l = getLight(pos);
        if (l != nullptr) {
            *l = {r, g, b, v};
        }
    }
    void setLight(int x, int y, int z, unsigned char v, unsigned char r, unsigned char g, unsigned char b) {
        Color* l = getLight(x, y, z);
        if (l != nullptr) {
            *l = {r, g, b, v};
        }
    }
    Color* getLight(Vector3 pos) {
        return getLight(pos.x, pos.y, pos.z);
    }
    Color* getLight(int x, int y, int z) {
        for (size_t i=0; i<lightmaps.length(); i++) {
            Vector3 p = positions[i];
            if (x >= p.x && y == p.y && z >= p.z) {
                LightMap* map = lightmaps[i];
                if (x-p.x < map->width() && z-p.z < map->height()) {
                    return &map->get({x-(int)p.x, z-(int)p.z});
                }
            }
        }
        return nullptr;
    }
    void addLight(int x, int y, int z, unsigned char v, unsigned char r, unsigned char g, unsigned char b) {
        Color* c = getLight(x, y, z);
        if (c != nullptr) {
            unsigned short rs = r;
            unsigned short gs = g;
            unsigned short bs = b;
            unsigned short vs = v;
            r = (rs * c->r) >> 8;
            g = (gs * c->g) >> 8;
            b = (bs * c->b) >> 8;
            v = (vs * c->a) >> 8;
            setLight(x, y, z, v, r, g, b);
        }
    }
    bool ShouldRenderMap(Vector3 pos, size_t mapno) {
        TileArray& map = maps[mapno];
        float d = Vector3Distance(pos, {positions[mapno].x, positions[mapno].y, positions[mapno].z});
        return d < renderDistance;
    }
    void Draw(Vector3 camerapos);
    void SetFog(float fogMin, float fogMax, float* fogColor);
};