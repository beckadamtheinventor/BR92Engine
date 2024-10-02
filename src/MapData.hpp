#pragma once

#include "DynamicArray.hpp"
#include "Array2D.hpp"
#include "TileRegistry.hpp"
#include "TextureRegistry.hpp"
#include "Buffer.hpp"
#include "Vec3.hpp"

#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"

#pragma region Defines
#define TILE_MAP_MAGIC_NUMBER_STR "TILE"
#define LIGHT_MAP_MAGIC_NUMBER_STR "LMAP"
#define WALL_MAP_MAGIC_NUMBER_STR "WALL"
#define FOG_MAGIC_NUMBER_STR "FOGC"
#define LIGHT_MULTIPLIER_MAGIC_NUMBER_STR "LMUL"
#define ENTITY_MAGIC_NUMBER_STR "ENTT"
#define TILE_MAP_MAGIC_NUMBER   (*(uint32_t*)TILE_MAP_MAGIC_NUMBER_STR)
#define LIGHT_MAP_MAGIC_NUMBER  (*(uint32_t*)LIGHT_MAP_MAGIC_NUMBER_STR)
#define WALL_MAP_MAGIC_NUMBER   (*(uint32_t*)WALL_MAP_MAGIC_NUMBER_STR)
#define FOG_MAGIC_NUMBER        (*(uint32_t*)FOG_MAGIC_NUMBER_STR)
#define LIGHT_MULTIPLIER_MAGIC_NUMBER (*(uint32_t*)LIGHT_MULTIPLIER_MAGIC_NUMBER_STR)
#define ENTITY_MAGIC_NUMBER (*(uint32_t*)ENTITY_MAGIC_NUMBER_STR)

#define LIGHT_RANGE 6
#define PLAYER_HEIGHT 0.4f
#define PLAYER_JUMP 0.15f
#define LEVEL_HEIGHT 1.0f
#define PLAYER_WIDTH 0.125f
#define GRAVITY -9.81
#define MAX_FALL_SPEED 10.0

#pragma endregion

#pragma region MapIntMesh
class MapIntMesh {
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
// struct LightMapEntry {
//     unsigned char r, g, b, n;
//     float v;
// };
class LightMap {
    Texture2D _tex = {0};
    Image _img = {0};
    public:
    LightMap() {}
    LightMap(int width, int height) {
        _img = GenImageColor(width, height, WHITE);
    }
    void upload() {
        _tex = LoadTextureFromImage(_img);
    }
    size_t width() {
        return _tex.width;
    }
    size_t height() {
        return _tex.height;
    }
    Color* get(int x, int y) {
        Color* colors = (Color*)_img.data;
        if (y >= 0 && y < height()) {
            if (x >= 0 && x < width()) {
                return &colors[x + y*width()];
            }
        }
        return nullptr;
    }
    unsigned int getId() {
        return _tex.id;
    }
};
#pragma endregion

#pragma region HitInfo
struct HitInfo {
    float distance;
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
    DynamicArray<MapIntMesh*> MapIntMeshes;
    DynamicArray<LightMap*> lightmaps;
    DynamicArray<PlacedLight> lightList;
    MapTileRegistry* tileRegistry = nullptr;
    TextureRegistry* textureRegistry = nullptr;
    unsigned int depthTextureId;
    bool hasLoadedLightmaps = false;
    public:
    DynamicArray<Vector3> spawnableSpaces;
    Shader mainShader, spriteShader;
    Texture2D atlas = {0};
    float fogMin, fogMax, fogColor[4], lightLevel, renderDistance;
    void BuildAtlas();
    void InitMesher(unsigned int depthTextureId);
    bool LoadMap(RBuffer& data);
    bool LoadMapChunk(RBuffer& data);
    bool LoadMapWalls(RBuffer& data);
    bool LoadMapTiles(RBuffer& data);
    bool LoadLightMap(RBuffer& data);
    bool HasLoadedLightmaps();
    void SaveMap(const char* fname);
    void SaveMap(std::ostream& fd);
    void SaveMapTile(std::ostream& fd, TileArray* arr, Vec3I position);
    void SaveLightMap(std::ostream& fd, size_t i, LightMap* map=nullptr);
    Vector3 RayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps=100);
    void SetLevelMesh(size_t i, unsigned int vertCount, unsigned int* verts, unsigned int triangleCount, unsigned short* indices);
    void GenerateMesh(size_t i);
    void GenerateMesh();
    void BuildLighting();
    void UploadMap(size_t mapno);
    void UploadMap();
    void ClearMap();
    void SetTileRegistry(MapTileRegistry* reg);
    void SetTextureRegistry(TextureRegistry* reg);
    unsigned short get(Vector3 pos);
    unsigned short get(int x, int y, int z);
    void setLight(Vector3 p1, Vector3 p2, float v, unsigned char r, unsigned char g, unsigned char b);
    void setLight(Vector3 pos, float v, unsigned char r, unsigned char g, unsigned char b);
    void setLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b);
    Color* getLight(Vector3 pos);
    size_t findLight(Vector3 pos);
    size_t findLight(int x, int y, int z);
    LightMap* getLightMap(Vector3 pos);
    Color* getLight(int x, int y, int z);
    void addLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b);
    bool ShouldRenderMap(Vector3 pos, size_t mapno);
    void Draw(Vector3 camerapos, Matrix* mat=nullptr, float renderwidth=1920);
    void SetFog(float fogMin, float fogMax, float* fogColor);
    Vector3 MoveTo(Vector3 position, Vector3 move, bool noclip=false);
    Vector3 ApplyGravity(Vector3 position, float& momentum, float dt);
};

extern MapData* GlobalMapData;

#pragma endregion