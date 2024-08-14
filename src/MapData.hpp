#pragma once

#include "DynamicArray.hpp"
#include "Array2D.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "Buffer.hpp"
#include "Vec3.hpp"

#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"

#pragma region Defines
#define WALL_MAP_MAGIC_NUMBER   (*(uint32_t*)"WALL")
#define TILE_MAP_MAGIC_NUMBER   (*(uint32_t*)"TILE")

#define LIGHT_RANGE 6
#define PLAYER_HEIGHT 0.4f
#define LEVEL_HEIGHT 1.0f
#define PLAYER_WIDTH 0.125f

#pragma endregion

#pragma region IntMesh
class IntMesh {
    public:
    unsigned int vertexCount = 0;
    unsigned int triangleCount = 0;
    unsigned int vbo[2] = {0, 0};
    unsigned int vao = 0;
    unsigned int* verts = nullptr;
    unsigned short* indices = nullptr;
};
#pragma endregion

#pragma region PlacedLight
class PlacedLight {
    public:
    int x, y, z;
    unsigned char v, r, g, b;
};
#pragma endregion

#pragma region MapTile
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
#pragma endregion

#pragma region MapTileRegistry
class MapTileRegistry : public Registry<MapTile> {};
#pragma endregion

#pragma region TileArray
class TileArray : public Array2D<unsigned short> {
    public:
    unsigned short getOrDefault(ArrayIndex i, unsigned short d=0) {
        if (i.x >= 0 && i.y >= 0 && i.x < w && i.y < h) {
            return get(i);
        }
        return d;
    }
};
#pragma endregion

#pragma region LightMap
struct LightMapEntry {
    unsigned char r, g, b, n;
    float v;
};
class LightMap : public Array2D<LightMapEntry> {
    public:
    LightMap() {}
    LightMap(int width, int height) {
        w = width;
        h = height;
        l = w * h;
        values = new LightMapEntry[l];
        for (size_t i=0; i<l; i++) {
            values[i] = {255, 255, 255, 0, 0};
        }
    }
    LightMapEntry tryGet(int ix, int iy) {
        if (ix >= 0 && iy >= 0 && ix < w && iy < h) {
            return values[iy * w + ix];
        }
        return {0};
    }

    LightMapEntry getInterpolated(int x, int z) {
        LightMapEntry v00 = tryGet((int)floorf(x + 0.5f), (int)floorf(z + 0.5f));
        LightMapEntry v01 = tryGet((int)floorf(x + 0.5f), (int)floorf(z - 0.5f));
        LightMapEntry v10 = tryGet((int)floorf(x - 0.5f), (int)floorf(z + 0.5f));
        LightMapEntry v11 = tryGet((int)floorf(x - 0.5f), (int)floorf(z - 0.5f));
        unsigned int r = v00.r + v01.r + v10.r + v11.r;
        unsigned int g = v00.g + v01.g + v10.g + v11.g;
        unsigned int b = v00.b + v01.b + v10.b + v11.b;
        // unsigned char n = (v00.n + v01.n + v10.n + v11.n) / 4;
        float v = v00.v + v01.v + v10.v + v11.v;
        unsigned char rc = r / 4;
        unsigned char gc = g / 4;
        unsigned char bc = b / 4;
        return {rc, gc, bc, 1, v};
    }
};
#pragma endregion

#pragma region HitInfo
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
#pragma endregion

#pragma region MapData
class MapData {
    DynamicArray<Vec3I> positions;
    DynamicArray<TileArray> maps;
    DynamicArray<IntMesh*> intMeshes;
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
    bool LoadMapChunk(RBuffer<char>& data);
    bool LoadMapWalls(RBuffer<char>& data);
    bool LoadMapTiles(RBuffer<char>& data);
    Vector3 RayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps=100);
    void GenerateMesh();
    void BuildLighting();
    void UploadMap(size_t mapno);
    void UploadMap();
    void SetTileRegistry(MapTileRegistry* reg);
    void SetTextureRegistry(TextureRegistry* reg);
    unsigned short get(Vector3 pos);
    unsigned short get(int x, int y, int z);
    void setLight(Vector3 pos, float v, unsigned char r, unsigned char g, unsigned char b);
    void setLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b);
    LightMapEntry* getLight(Vector3 pos);
    LightMapEntry* getLight(int x, int y, int z);
    void addLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b);
    bool ShouldRenderMap(Vector3 pos, size_t mapno);
    void Draw(Vector3 camerapos, Matrix* mat=nullptr);
    void SetFog(float fogMin, float fogMax, float* fogColor);
    Vector3 MoveTo(Vector3 position, Vector3 move, bool noclip=false);
    Vector3 ApplyGravity(Vector3 position, float amount);
};
#pragma endregion