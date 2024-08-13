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

#pragma region Defines
// "SECT"
#define SECTIONED_MAP_MAGIC_NUMBER   0x54434553
// "LARG"
#define LARGE_MAP_MAGIC_NUMBER       0x4752414C

#define LIGHT_RANGE 8

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
    Vector3 RayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps=100);
    void GenerateMesh();
    void BuildLighting();
    void UploadMap(size_t mapno);
    void UploadMap();
    void SetTileRegistry(MapTileRegistry* reg);
    void SetTextureRegistry(TextureRegistry* reg);
    unsigned short get(Vector3 pos);
    unsigned short get(int x, int y, int z);
    void setLight(Vector3 pos, unsigned char v, unsigned char r, unsigned char g, unsigned char b);
    void setLight(int x, int y, int z, unsigned char v, unsigned char r, unsigned char g, unsigned char b);
    Color* getLight(Vector3 pos);
    Color* getLight(int x, int y, int z);
    void addLight(int x, int y, int z, unsigned char v, unsigned char r, unsigned char g, unsigned char b);
    bool ShouldRenderMap(Vector3 pos, size_t mapno);
    void Draw(Vector3 camerapos);
    void SetFog(float fogMin, float fogMax, float* fogColor);
};
#pragma endregion