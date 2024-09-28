
#include "MapData.hpp"
#include "AssetPath.hpp"
#include "ShaderLoader.hpp"
#include "TileRegistry.hpp"
#include "raylib.h"
#include "raymath.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <thread>

MapData* GlobalMapData=nullptr;

#pragma region Const Data
static const constexpr char cubeverts[] = {
    // +y
    1, 1, 1,
    0, 1, 1,
    0, 1, 0,
    1, 1, 0,
    // -y
    1, 0, 1,
    1, 0, 0,
    0, 0, 0,
    0, 0, 1,
    // +x
    1, 0, 0,
    1, 1, 0,
    1, 1, 1,
    1, 0, 1,
    // -x
    0, 0, 1,
    0, 1, 1,
    0, 1, 0,
    0, 0, 0,
    // +z
    1, 0, 1,
    1, 1, 1,
    0, 1, 1,
    0, 0, 1,
    // -z
    0, 0, 0,
    0, 1, 0,
    1, 1, 0,
    1, 0, 0,
};
static const constexpr char vertexnumbers[] = {
    3, 1, 0, 2,
};
static const constexpr char directions[] {
    0, 1, 0,
    0, -1, 0,
    1, 0, 0,
    -1, 0, 0,
    0, 0, 1,
    0, 0, -1,
};
static const constexpr char triangleindices[6] = {
    0, 1, 2, 0, 2, 3,
};
#pragma endregion

#pragma region Helper Functions
#define OpenGLDebug(s) if (GLenum e = glGetError()) printf("%s: OpenGL Error: %u\n", s, e)

float inverseSquareRoot(float v) {
	int32_t i;
	float x2, y;
	const float threehalfs = 1.5f;
	y = v;
	x2 = y * 0.5f;
	i = *(int32_t*) &y; // evil floating point bit level hacking
	i = 0x5f3759df - ( i >> 1 ); // what the fuck? 
	y = *(float*) &i;
	return y * (threehalfs - (x2*y*y)); // 1st iteration (Newton's method)
}
#pragma endregion

#pragma region InitMesher()
void MapData::InitMesher(unsigned int depthTextureId) {
    mainShader = ShaderLoader::load(AssetPath::shader("main"));
    spriteShader = ShaderLoader::load(AssetPath::shader("sprite"));
    glBindAttribLocation(mainShader.id, 0, "vertexInfo1");
    glBindAttribLocation(mainShader.id, 1, "vertexInfo2");
    this->depthTextureId = depthTextureId;
}
#pragma endregion

#pragma region BuildAtlas()
void MapData::BuildAtlas() {
    atlas = textureRegistry->build();
}
#pragma endregion

#pragma region LoadMap()
bool MapData::LoadMap(RBuffer& data) {
    do {
        if (!LoadMapChunk(data)) {
            return false;
        }
    } while (!data.eof());
    return true;
}
#pragma endregion

#pragma region LoadMapChunk()
bool MapData::LoadMapChunk(RBuffer& data) {
    unsigned int magic;
    if (!data.readV<unsigned int>(&magic)) {
        return false;
    }
    if (magic == WALL_MAP_MAGIC_NUMBER) {
        if (!LoadMapWalls(data)) {
            return false;
        }
    } else if (magic == TILE_MAP_MAGIC_NUMBER) {
        if (!LoadMapTiles(data)) {
            return false;
        }
    } else if (magic == LIGHT_MAP_MAGIC_NUMBER) {
        hasLoadedLightmaps = true;
        if (!LoadLightMap(data)) {
            return false;
        }
    } else if (magic == FOG_MAGIC_NUMBER) {
        unsigned char c;
        for (char i=0; i<4; i++) {
            if (!data.read(c)) {
                return false;
            }
            fogColor[i] = c / 255.0f;
        }
        if (!data.readV<float>(&fogMin)) {
            return false;
        }
        if (!data.readV<float>(&fogMax)) {
            return false;
        }
    } else if (magic == LIGHT_MULTIPLIER_MAGIC_NUMBER) {
        if (!data.readV<float>(&lightLevel)) {
            return false;
        }
    }
    return true;
}
#pragma endregion

#pragma region LoadMapWalls()
bool MapData::LoadMapWalls(RBuffer& data) {
    int x, y, z;
    if (!data.readV<int>(&x)) {
        return false;
    }
    if (!data.readV<int>(&y)) {
        return false;
    }
    if (!data.readV<int>(&z)) {
        return false;
    }
    return true;
}
#pragma endregion

#pragma region LoadMapTile()
bool MapData::LoadMapTiles(RBuffer& data) {
    TileArray map;
    int x, y, z;
    if (!data.readV<int>(&x)) {
        return false;
    }
    if (!data.readV<int>(&y)) {
        return false;
    }
    if (!data.readV<int>(&z)) {
        return false;
    }
    unsigned char sizeX, sizeZ;
    if (!data.read(sizeX)) {
        return false;
    }
    if (!data.read(sizeZ)) {
        return false;
    }
    if (sizeX==0 || sizeZ==0) {
        return false;
    }
    map.resize(sizeX, sizeZ);
    for (unsigned char zz=0; zz<sizeZ; zz++) {
        for (unsigned char xx=0; xx<sizeX; xx++) {
            unsigned short tid;
            unsigned char c;
            if (!data.read(c)) {
                return false;
            }
            tid = c;
            if (!data.read(c)) {
                return false;
            }
            tid |= c << 8;
            map[{xx, zz}] = tid;
            MapTile* tile = tileRegistry->of(tid);
            if (tile != nullptr) {
                if (tile->light > 0) {
                    lightList.append({x+xx, y, z+zz, tile->light, tile->tintr, tile->tintg, tile->tintb});
                }
                if (tile->isSpawnable) {
                    spawnableSpaces.append({(float)x+xx, (float)y, (float)z+zz});
                }
            }
        }
    }
    maps.append(map);
    positions.append({x, y, z});
    lightmaps.append(new LightMap(sizeX, sizeZ));
    MapIntMeshes.append(new MapIntMesh());
    TraceLog(LOG_INFO, "Loaded map #%llu at %d,%d,%d size %d,%d", maps.length(), x, y, z, sizeX, sizeZ);
    return true;
}
#pragma endregion

#pragma region LoadLightMap()
bool MapData::HasLoadedLightmaps() {
    return hasLoadedLightmaps;
}
bool MapData::LoadLightMap(RBuffer& data) {
    int i;
    if (!data.readV<int>(&i)) {
        return false;
    }
    LightMap* map = nullptr;
    char sizeX, sizeZ;
    if (i < lightmaps.length()) {
        map = lightmaps[i];
        sizeX = map->width();
        sizeZ = map->height();
    } else {
        if (i < maps.length()) {
            sizeX = maps[i].width();
            sizeZ = maps[i].height();
        } else {
            return false;
        }
        map = new LightMap(sizeX, sizeZ);
        lightmaps[i] = map;
    }
    if (sizeX==0 || sizeZ==0) {
        return false;
    }
    for (int z=0; z<sizeZ; z++) {
        for (int x=0; x<sizeX; x++) {
            Color l;
            if (!data.read(l.r)) {
                return false;
            }
            if (!data.read(l.g)) {
                return false;
            }
            if (!data.read(l.b)) {
                return false;
            }
            l.a = 255;
            *map->get(x, z) = l;
       }
    }
    return true;
}

#pragma endregion

#pragma region SaveMap()
void MapData::SaveMap(const char* fname) {
    std::ofstream fd(fname, std::ios::binary|std::ios::out);
    SaveMap(fd);
    fd.close();
}

void MapData::SaveMap(std::ostream& fd) {
    for (size_t i=0; i<maps.length(); i++) {
        SaveMapTile(fd, &maps[i], positions[i]);
    }
    for (size_t i=0; i<maps.length(); i++) {
        SaveLightMap(fd, i);
    }
}
#pragma endregion

#pragma region SaveMapTile()
void MapData::SaveMapTile(std::ostream& fd, TileArray* map, Vec3I position) {
    fd.write(TILE_MAP_MAGIC_NUMBER_STR, 4);
    fd.write((char*)&position.x, 4);
    fd.write((char*)&position.y, 4);
    fd.write((char*)&position.z, 4);
    fd.put(map->width());
    fd.put(map->height());
    for (int y=0; y<map->height(); y++) {
        for (int x=0; x<map->width(); x++) {
            unsigned short c = map->get({x, y});
            fd.write((char*)&c, 2);
        }
    }
}
#pragma endregion

#pragma region SaveLightMap()
void MapData::SaveLightMap(std::ostream& fd, size_t i, LightMap* map) {
    if (map == nullptr) {
        map = lightmaps[i];
    }
    unsigned int il = i;
    fd.write(LIGHT_MAP_MAGIC_NUMBER_STR, 4);
    fd.write((char*)&il, 4);
    for (int z=0; z<map->height(); z++) {
        for (int x=0; x<map->width(); x++) {
            Color l = *map->get(x, z);
            fd.put(l.r);
            fd.put(l.g);
            fd.put(l.b);
        }
    }
}
#pragma endregion

#pragma region DDACast()
float DDACast(MapTileRegistry* reg, MapData* map, int y, int x1, int z1, int x2, int z2, bool (*issolid)(MapTileRegistry* reg, unsigned short tid)) {
    int sx = x1 < x2 ? x1 : x2;
    int sz = z1 < z2 ? z1 : z2;
    int ex = x1 > x2 ? x1 : x2;
    int ez = z1 > z2 ? z1 : z2;
    Vector2 rdelta = Vector2Normalize({(float)ex-sx, (float)ez-sz});
    float ddx = rdelta.x == 0 ? 1e30 : 1.0f / rdelta.x;
    float ddz = rdelta.y == 0 ? 1e30 : 1.0f / rdelta.y;
    int stepX, stepZ;
    float sdx, sdz;
    if (rdelta.x < 0) {
        stepX = -1;
        sdx = 0.0f;
    } else {
        stepX = 1;
        sdx = 1.0f;
    }
    if (rdelta.y < 0) {
        stepZ = -1;
        sdz = 0.0f;
    } else {
        stepZ = 1;
        sdz = 1.0f;
    }
    unsigned short tid;
    bool zside = false;
    do {
        if (sdx < sdz) {
            sdx += ddx;
            sx += stepX;
            zside = false;
        } else {
            sdz += ddz;
            sz += stepZ;
            zside = true;
        }
        tid = map->get({(float)sx, (float)y, (float)sz});
        if (issolid(reg, tid)) {
            return 0;
        }
    } while (sx < ex && sz < ez);
    if (zside) {
        return sdz - ddz;
    } else {
        return sdx - ddx;
    }
}
#pragma endregion

#pragma region BuildLighting()
bool isSolidToLight(MapTileRegistry* reg, unsigned short tid) {
    MapTile* tile = reg->of(tid);
    return tile->blocksLight;
}

void MapData::BuildLighting() {
    TraceLog(LOG_INFO, "Building lighting for %llu lights...", lightList.length());
    for (size_t i=0; i<maps.length(); i++) {
        TileArray* map = &maps[i];
        int y = positions[i].y;
        int xx = positions[i].x;
        int zz = positions[i].z;
        for (size_t j=0; j<lightList.length(); j++) {
            PlacedLight* light = &lightList[j];
            TraceLog(LOG_INFO, "Building light %llu", j);
            // float dy = powf(y - light->y, 2);
            for (int z=0; z<map->height(); z++) {
                for (int x=0; x<map->width(); x++) {
                    float dist = DDACast(tileRegistry, this, y, xx+x, zz+z, light->x, light->z, isSolidToLight);
                    if (dist > 0) {
                        float d = inverseSquareRoot(dist*dist);
                        addLight(xx+x, y, zz+z, light->v*d, light->r, light->g, light->b);
                    }
                }
            }
        }
    }
    TraceLog(LOG_INFO, "Done building lights!");
}
#pragma endregion

#pragma region RayCast()
Vector3 MapData::RayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps) {
    Vector3 p = pos;
    hit.flags = 0;
    for (size_t i=0; i<max_steps; i++) {
        MapTile* tile = tileRegistry->of(get(p.x, p.y, p.z));
        if (tile != nullptr && tile->isSolid) {
            hit.hitWall = true;
            hit.distance = Vector3Distance(pos, p);
            break;
        }
        p = Vector3Add(p, dir);
    }
    return p;
}
#pragma endregion

#pragma region _GenerateMesh()
static void _GenerateMesh(TileArray* map, LightMap* lmap, MapTileRegistry* tileRegistry, DynamicArray<unsigned int>* verts, DynamicArray<unsigned short>* indices) {
    unsigned int mi = 0;
    for (int z=0; z<map->height(); z++) {
        for (int x=0; x<map->width(); x++) {
            MapTile* tile = tileRegistry->of(map->get({x, z}));
            if (tile == nullptr) {
                continue;
            }
            unsigned short tid;
            for (char fi=0; fi<6; fi++) {
                if (fi == 0) {
                    tid = tile->ceiling;
                } else if (fi == 1) {
                    tid = tile->floor;
                } else {
                    MapTile* tile2;
                    unsigned short tid2;
                    tid = tile->wall;
                    if (fi == 2) { // +X
                        tid2 = map->getOrDefault({x+1, z});
                    } else if (fi == 3) { // -X
                        tid2 = map->getOrDefault({x-1, z});
                    } else if (fi == 4) { // +Z
                        tid2 = map->getOrDefault({x, z+1});
                    } else if (fi == 5) { // -Z
                        tid2 = map->getOrDefault({x, z-1});
                    }
                    tile2 = tileRegistry->of(tid2);
                    if (tile2 != nullptr && tile2->isSolid) {
                        continue;
                    }
                }
                if (tid > 0) {
                    char fo = fi*3*4;
                    for (char j=0; j<4; j++) {
                        // TraceLog(LOG_INFO, "vertex %d, %d, %d [%u]",
                        //     cubeverts[fo + j*3 + 0] + x, cubeverts[fo + j*3 + 1], cubeverts[fo + j*3 + 2] + z, tid);
                        verts->append(
                            (vertexnumbers[j] << 30) | // Vertex number
                            (cubeverts[fo + j*3 + 1]<<29) | // Y position
                            ((cubeverts[fo + j*3 + 0] + x)<<20) | // X position
                            ((cubeverts[fo + j*3 + 2] + z)<<12) | // Z position
                            tid & 0xfff // texture ID
                        );
                    }
                    for (char j=0; j<6; j++) {
                        // TraceLog(LOG_INFO, "index %d", mi+I[j]);
                        indices->append(mi+triangleindices[j]);
                    }
                    mi += 4;
                }
            }
        }
    }
}
#pragma endregion

#pragma region GenerateMesh()
void MapData::SetLevelMesh(size_t i, unsigned int vertCount, unsigned int* verts, unsigned int triangleCount, unsigned short* indices) {
    MapIntMesh* mesh = MapIntMeshes[i];
    if (mesh->verts != nullptr) {
        delete mesh->verts;
    }
    if (mesh->indices != nullptr) {
        delete mesh->indices;
    }
    mesh->vertexCount = vertCount;
    mesh->verts = verts;
    mesh->triangleCount = triangleCount;
    mesh->indices = indices;
    TraceLog(LOG_INFO, "Generated level mesh #%llu with %u verts and %u triangles.",
        i+1, mesh->vertexCount, mesh->triangleCount);
}

void MapData::GenerateMesh(size_t i) {
    DynamicArray<unsigned int>* vertarray = new DynamicArray<unsigned int>(maps[i].size()*6*4*2);
    DynamicArray<unsigned short>* indexarray = new DynamicArray<unsigned short>(maps[i].size()*36);
    _GenerateMesh(&maps[i], lightmaps[i], tileRegistry, vertarray, indexarray);
    SetLevelMesh(i,
        vertarray->length()/2, vertarray->collapse(),
        indexarray->length()/3, indexarray->collapse()
    );
    delete vertarray;
    delete indexarray;
}

void MapData::GenerateMesh() {
    std::thread threads[maps.length()];
    DynamicArray<unsigned int>* vertarrays[maps.length()];
    DynamicArray<unsigned short>* indexarrays[maps.length()];
    for (size_t i=0; i<maps.length(); i++) {
        vertarrays[i] = new DynamicArray<unsigned int>();
        indexarrays[i] = new DynamicArray<unsigned short>();
        vertarrays[i]->resize(maps[i].size()*6*4*2);
        indexarrays[i]->resize(maps[i].size()*36);
        threads[i] = std::thread(_GenerateMesh, &maps[i], lightmaps[i], tileRegistry, vertarrays[i], indexarrays[i]);
    }
    for (size_t i=0; i<maps.length(); i++) {
        if (threads[i].joinable()) {
            threads[i].join();
            SetLevelMesh(i,
                vertarrays[i]->length(), vertarrays[i]->collapse(),
                indexarrays[i]->length()/3, indexarrays[i]->collapse()
            );
            delete vertarrays[i];
            delete indexarrays[i];
        }
    }
}
#pragma endregion

#pragma region UploadMap()
void MapData::UploadMap(size_t mapno) {
    MapIntMesh* mesh = MapIntMeshes[mapno];
    if (mesh->vao == 0) {
        glGenVertexArrays(1, &mesh->vao);
    }
    if (mesh->vbo[0] == 0) {
        glGenBuffers(1, &mesh->vbo[0]);
    }
    if (mesh->vbo[1] == 0) {
        glGenBuffers(1, &mesh->vbo[1]);
    }
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertexCount*sizeof(uint32_t), mesh->verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->triangleCount*3*sizeof(unsigned short), mesh->indices, GL_STATIC_DRAW);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*1, (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void MapData::UploadMap() {
    for (size_t i=0; i<MapIntMeshes.length(); i++) {
        UploadMap(i);
    }
}
#pragma endregion

#pragma region ClearMap
void MapData::ClearMap() {
    for (size_t i=0; i<MapIntMeshes.length(); i++) {
        MapIntMesh* mesh = MapIntMeshes[i];
        delete mesh->indices;
        delete mesh->verts;
        mesh->vertexCount = 0;
        mesh->indices = 0;
        glDeleteVertexArrays(1, &mesh->vao);
        glDeleteBuffers(2, mesh->vbo);
        delete mesh;
    }
    for (size_t i=0; i<lightmaps.length(); i++) {
        delete lightmaps[i];
    }
    for (size_t i=0; i<maps.length(); i++) {
        maps[i].resize(0, 0);
    }
    MapIntMeshes.clear();
    lightmaps.clear();
    maps.clear();
    positions.clear();
    lightList.clear();
    hasLoadedLightmaps = false;
}
#pragma endregion

#pragma region SetTileRegistry()
void MapData::SetTileRegistry(MapTileRegistry* reg) {
    tileRegistry = reg;
}
#pragma endregion

#pragma region SetTextureRegistry()
void MapData::SetTextureRegistry(TextureRegistry* reg) {
    textureRegistry = reg;
}
#pragma endregion

#pragma region get()
unsigned short MapData::get(Vector3 pos) {
    return get(floorf(pos.x), floorf(pos.y), floorf(pos.z));
}
unsigned short MapData::get(int x, int y, int z) {
    for (size_t i=0; i<maps.length(); i++) {
        Vec3I p = positions[i];
        if (x >= p.x && y == p.y && z >= p.z) {
            TileArray& map = maps[i];
            if (x-p.x < map.width() && z-p.z < map.height()) {
                return map[{x-p.x, z-p.z}];
            }
        }
    }
    return 0;
}
#pragma endregion

#pragma region setLight()
void MapData::setLight(Vector3 p1, Vector3 p2, float v, unsigned char r, unsigned char g, unsigned char b) {
    int sx = std::min(floorf(p1.x), floorf(p2.x));
    int sy = std::min(floorf(p1.y), floorf(p2.y));
    int sz = std::min(floorf(p1.z), floorf(p2.z));
    int ex = std::max(floorf(p1.x), floorf(p2.x));
    int ey = std::max(floorf(p1.y), floorf(p2.y));
    int ez = std::max(floorf(p1.z), floorf(p2.z));
    Color l = {r, g, b, 255};
    for (int y=sy; y<=ey; y++) {
        for (int z=sz; z<=ez; z++) {
            for (int x=sx; x<=ex; x++) {
                Color* ptr = getLight(x, y, z);
                if (ptr != nullptr) {
                    *ptr = l;
                }
            }
        }
    }
    // regen and reupload the whole map for convenience.
    GenerateMesh();
    UploadMap();
}
void MapData::setLight(Vector3 pos, float v, unsigned char r, unsigned char g, unsigned char b) {
    setLight(floorf(pos.x), floorf(pos.y), floorf(pos.z), v, r, g, b);
}
void MapData::setLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b) {
    Color* l = getLight(x, y, z);
    if (l != nullptr) {
        // set the lightmap value
        *l = {r, g, b, 255};
        // regen and reupload the associated map mesh
        // size_t i = findLight(x, y, z);
        // if (i != -1) {
        //     GenerateMesh(i);
        //     UploadMap(i);
        //     return;
        // }
    }
    TraceLog(LOG_WARNING, "Failed to set light at %d,%d,%d", x, y, z);
}
#pragma endregion

#pragma region getLight()
Color* MapData::getLight(Vector3 pos) {
    return getLight(pos.x, pos.y, pos.z);
}
Color* MapData::getLight(int x, int y, int z) {
    for (size_t i=0; i<lightmaps.length(); i++) {
        Vec3I p = positions[i];
        if (x >= p.x && y == p.y && z >= p.z) {
            LightMap* map = lightmaps[i];
            if (x-p.x < map->width() && z-p.z < map->height()) {
                return map->get(x-p.x, z-p.z);
            }
        }
    }
    return nullptr;
}
LightMap* MapData::getLightMap(Vector3 pos) {
    size_t i = MapData::findLight(pos);
    if (i == -1) {
        return nullptr;
    }
    return lightmaps[i];
}
#pragma endregion

#pragma region findLight()
size_t MapData::findLight(Vector3 pos) {
    return findLight(floorf(pos.x), floorf(pos.y), floorf(pos.z));
}

size_t MapData::findLight(int x, int y, int z) {
    for (size_t i=0; i<lightmaps.length(); i++) {
        Vec3I p = positions[i];
        if (x >= p.x && y == p.y && z >= p.z) {
            LightMap* map = lightmaps[i];
            if (x-p.x < map->width() && z-p.z < map->height()) {
                return i;
            }
        }
    }
    return -1;
}
#pragma endregion

#pragma region addLight()
void MapData::addLight(int x, int y, int z, float v, unsigned char r, unsigned char g, unsigned char b) {
    Color* c = getLight(x, y, z);
    if (c != nullptr) {
        unsigned short rs = r;
        unsigned short gs = g;
        unsigned short bs = b;
        rs = (rs * c->r) >> 8;
        gs = (gs * c->g) >> 8;
        bs = (bs * c->b) >> 8;
        if (rs > 255) rs = 255;
        if (gs > 255) gs = 255;
        if (bs > 255) bs = 255;
        r = rs; g = gs; b = bs;
        *c = {r, g, b, 255};
    }
}
#pragma endregion

#pragma region ShouldRenderMap
bool MapData::ShouldRenderMap(Vector3 pos, size_t mapno) {
    float x = (float)positions[mapno].x + maps[mapno].width() * 0.5f;
    float z = (float)positions[mapno].z + maps[mapno].height() * 0.5f;
    float d = Vector3Distance(pos, {x, (float)positions[mapno].y, z});
    if (maps[mapno].width() >= maps[mapno].height()) {
        d -= maps[mapno].width();
    } else {
        d -= maps[mapno].height();
    }
    return d < renderDistance;
}
#pragma endregion


#pragma region Draw()
void MapData::Draw(Vector3 camerapos, Matrix* mat, float renderwidth) {
    unsigned int loc;
    glUseProgram(mainShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.id);
    loc = GetShaderLocation(mainShader, "texture0");
    glUniform1i(loc, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // loc = GetShaderLocation(mainShader, "cameraPosition");
    // glUniform3f(loc, camerapos.x, camerapos.y, camerapos.z);
    if (mat==nullptr) {
        Matrix matView = rlGetMatrixModelview();
        Matrix matProjection = rlGetMatrixProjection();
        Matrix matModel = rlGetMatrixTransform();
        Matrix matModelView = MatrixMultiply(matModel, matView);
        Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);
        rlSetUniformMatrix(mainShader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);
    } else {
        rlSetUniformMatrix(mainShader.locs[SHADER_LOC_MATRIX_MVP], *mat);
    }
    loc = GetShaderLocation(mainShader, "renderwidth");
    glUniform1f(loc, renderwidth);
    loc = GetShaderLocation(mainShader, "FogMin");
    glUniform1f(loc, fogMin);
    loc = GetShaderLocation(mainShader, "FogMax");
    glUniform1f(loc, fogMax);
    loc = GetShaderLocation(mainShader, "FogColor");
    glUniform4f(loc, fogColor[0], fogColor[1], fogColor[2], fogColor[3]);
    loc = GetShaderLocation(mainShader, "LightLevel");
    glUniform1f(loc, lightLevel);
    loc = GetShaderLocation(mainShader, "drawPosition");
    unsigned int lmaploc = GetShaderLocation(mainShader, "texture1");
    for (size_t i=0; i<MapIntMeshes.length(); i++) {
        if (ShouldRenderMap(camerapos, i)) {
            MapIntMesh* imesh = MapIntMeshes[i];
            Vec3I pos = positions[i];
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, lightmaps[i]->getId());
            glUniform1i(lmaploc, 1);
            glUniform3f(loc, pos.x, pos.y, pos.z);
            glBindVertexArray(imesh->vao);
            glDrawElements(GL_TRIANGLES, imesh->triangleCount*3, GL_UNSIGNED_SHORT, 0);
        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
#pragma endregion

#pragma region SetFog()
void MapData::SetFog(float fogMin, float fogMax, float* fogColor) {
    this->fogMin = fogMin;
    this->fogMax = fogMax;
    this->fogColor[0] = fogColor[0];
    this->fogColor[1] = fogColor[1];
    this->fogColor[2] = fogColor[2];
    this->fogColor[3] = fogColor[3];
}
#pragma endregion

#pragma region Movement
Vector3 MapData::MoveTo(Vector3 position, Vector3 move, bool noclip) {
    bool collided = false;
    unsigned short tid1 = get({position.x + move.x + (move.x>0?PLAYER_WIDTH:-PLAYER_WIDTH), position.y, position.z});
    MapTile* tile1 = tileRegistry->of(tid1);
    unsigned short tid2 = get({position.x, position.y, position.z + move.z + (move.z>0?PLAYER_WIDTH:-PLAYER_WIDTH)});
    MapTile* tile2 = tileRegistry->of(tid2);
    if (noclip) {
        position = Vector3Add(position, move);
    } else {
        if ((tile1 == nullptr || !tile1->isSolid)) {
            position.x += move.x;
        } else {
            if (move.x > 0) {
                position.x = ceilf(position.x) - PLAYER_WIDTH;
            } else {
                position.x = floorf(position.x) + PLAYER_WIDTH;
            }
        }
        if ((tile2 == nullptr || !tile2->isSolid)) {
            position.z += move.z;
        } else {
            if (move.z > 0) {
                position.z = ceilf(position.z) - PLAYER_WIDTH;
            } else {
                position.z = floorf(position.z) + PLAYER_WIDTH;
            }
        }
    }
    return position;
}

Vector3 MapData::ApplyGravity(Vector3 position, float& momentum, float dt) {
    if (momentum >= 0) {
        position.y += momentum*dt + PLAYER_WIDTH;
        unsigned short tid = get(position);
        MapTile* tile = tileRegistry->of(tid);
        if (!(tile == nullptr || !tile->solidCeiling)) {
            position.y = ceilf(position.y);
        }
        position.y -= PLAYER_WIDTH;
    } else {
        position.y += momentum*dt - PLAYER_HEIGHT;
        unsigned short tid = get(position);
        MapTile* tile = tileRegistry->of(tid);
        if (tile == nullptr || !tile->solidFloor) {
            momentum += GRAVITY * dt;
        } else {
            position.y = floorf(position.y);
            momentum = 0;
        }
        position.y += PLAYER_HEIGHT;
    }
    return position;
}

#pragma endregion
