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
#include <thread>

// "SECT"
#define SECTIONED_MAP_MAGIC_NUMBER   0x54434553
// "LARG"
#define LARGE_MAP_MAGIC_NUMBER       0x4752414C

struct IntMesh {
    unsigned int vertexCount;
    unsigned int triangleCount;
    unsigned int vbo[2];
    unsigned int vao;
    unsigned int* verts;
    unsigned short* indices;
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
        };
    };
    unsigned short floor, ceiling, wall;
};

class MapTileRegistry : public Registry<MapTile> {};

class TileArray : public Array2D<unsigned short> {};

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
    Texture2D atlas = {0};
    MapTileRegistry* tileRegistry = nullptr;
    TextureRegistry* textureRegistry = nullptr;
    Shader mainShader;
    unsigned int depthTextureId;
    public:
    float fogMin, fogMax, fogColor[4], lightLevel;
    void BuildAtlas();
    void InitMesher(unsigned int depthTextureId);
    bool LoadMap(RBuffer<char>& data);
    bool LoadMapSection(RBuffer<char>& data);
    Vector3 rayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps=100);
    void GenerateMesh();
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
    bool ShouldRenderMap(Vector3 pos, size_t mapno) {
        TileArray& map = maps[mapno];
        float d = Vector3Distance(pos, {positions[mapno].x, positions[mapno].y, positions[mapno].z});
        float maxd = 100;
        return d < maxd;
    }
    void Draw(Vector3 camerapos);
    void SetFog(float fogMin, float fogMax, float* fogColor);
};