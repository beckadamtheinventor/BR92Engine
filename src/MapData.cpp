
#include "MapData.hpp"
#include "AssetPath.hpp"
#include "raylib.h"
#include "raymath.h"
#include "raymath.h"
#include <thread>

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

static inline bool _readInt(RBuffer<char>& data, int& x) {
    char c;
    if (!data.read(c)) {
        return false;
    }
    x = c;
    if (!data.read(c)) {
        return false;
    }
    x |= c << 8;
    if (!data.read(c)) {
        return false;
    }
    x |= c << 16;
    if (!data.read(c)) {
        return false;
    }
    x |= c << 24;
    return true;
}

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
    const char* v = AssetPath::shader("vert");
    char* vcopy = new char[strlen(v)+1];
    memcpy(vcopy, v, strlen(v)+1);
    mainShader = LoadShader(vcopy, AssetPath::shader("frag"));
    delete [] vcopy;
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
bool MapData::LoadMap(RBuffer<char>& data) {
    int magic;
    if (!_readInt(data, magic)) {
        return false;
    }
    if (magic == LARGE_MAP_MAGIC_NUMBER) {

    } else if (magic == SECTIONED_MAP_MAGIC_NUMBER) {
        do {
            if (!MapData::LoadMapSection(data)) {
                return false;
            }
        } while (!data.eof());
    }
    return true;
}
#pragma endregion

#pragma region LoadMapSection()
bool MapData::LoadMapSection(RBuffer<char>& data) {
    TileArray map;
    int x, y, z;
    if (!_readInt(data, x)) {
        return false;
    }
    if (!_readInt(data, y)) {
        return false;
    }
    if (!_readInt(data, z)) {
        return false;
    }
    char sizeX, sizeZ;
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
            char c;
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
            if (tile != nullptr && tile->light > 0) {
                lightList.append({x+xx, y, z+zz, tile->light, tile->tintr, tile->tintg, tile->tintb});
            }
        }
    }
    maps.append(map);
    positions.append({x, y, z});
    lightmaps.append(new LightMap(sizeX, sizeZ));
    meshes.append(new IntMesh());
    return true;
}
#pragma endregion

#pragma region BuildLighting()
void MapData::BuildLighting() {
    TraceLog(LOG_INFO, "Building lighting for %llu lights...", lightList.length());
    DynamicArray<int> heights;
    for (size_t i=0; i<lightList.length(); i++) {
        bool shouldAdd = true;
        int y = lightList[i].y;
        for (size_t j=0; j<heights.length(); j++) {
            if (heights[j] == y) {
                shouldAdd = false;
            }
        }
        if (shouldAdd) {
            heights.append(y);
        }
    }
    for (size_t i=0; i<heights.length(); i++) {
        int y = heights[i];
        TileArray* map = &maps[i];
        for (size_t j=0; j<lightList.length(); j++) {
            if (lightList[j].y == y) {
                PlacedLight* light = &lightList[j];
                int xx = light->x;
                int zz = light->z;
                addLight(xx, y, zz, light->v, light->r, light->g, light->b);
                for (int o=-LIGHT_RANGE; o<=LIGHT_RANGE; o++) {
                    bool px_col = false, pz_col = false, mx_col = false, mz_col = false;
                    addLight(xx+o, y, zz, light->v, light->r, light->g, light->b);
                    addLight(xx, y, zz+o, light->v, light->r, light->g, light->b);
                    for (int di=1; di<=LIGHT_RANGE; di++) {
                        int lightValue = inverseSquareRoot((di*di*0.5f + o*o*0.5f) * 0.075f) * light->v;
                        if (lightValue > 0) {
                            MapTile* tile;
                            unsigned short tid;
                            if (!pz_col) {
                                tile = tileRegistry->of(tid = map->getOrDefault({xx+o, zz+di}));
                                if (tile != nullptr && tile->blocksLight) {
                                    pz_col = true;
                                }
                                addLight(xx+o, y, zz+di, lightValue, light->r, light->g, light->b);
                            }
                            if (!mz_col) {
                                tile = tileRegistry->of(tid = map->getOrDefault({xx+o, zz-di}));
                                if (tile != nullptr && tile->blocksLight) {
                                    mz_col = true;
                                }
                                addLight(xx+o, y, zz-di, lightValue, light->r, light->g, light->b);
                            }
                            if (!px_col) {
                                tile = tileRegistry->of(tid = map->getOrDefault({xx+di, zz+o}));
                                if (tile != nullptr && tile->blocksLight) {
                                    px_col = true;
                                }
                                addLight(xx+di, y, zz+o, lightValue, light->r, light->g, light->b);
                            }
                            if (!mx_col) {
                                tile = tileRegistry->of(tid = map->getOrDefault({xx-di, zz+o}));
                                if (tile != nullptr && tile->blocksLight) {
                                    mx_col = true;
                                }
                                addLight(xx-di, y, zz+o, lightValue, light->r, light->g, light->b);
                            }
                        }
                    }
                }
            }
        }
    }
}
#pragma endregion

#pragma region RayCast()
Vector3 MapData::RayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps) {
    Vector3 p = pos;
    hit.flags = 0;
    for (size_t i=0; i<max_steps; i++) {
        if (tileRegistry->of(get(p.x, p.y, p.z))->isSolid) {
            hit.hitWall = true;
        }
        p = Vector3Add(p, dir);
    }
    return p;
}
#pragma endregion

#pragma region _GenerateMesh()
static void _GenerateMesh(TileArray* map, LightMap* lmap, MapTileRegistry* tileRegistry, DynamicArray<unsigned int,0>* verts, DynamicArray<unsigned short,0>* indices) {
    unsigned int mi = 0;
    for (int z=0; z<map->height(); z++) {
        for (int x=0; x<map->width(); x++) {
            MapTile* tile = tileRegistry->of(map->get({x, z}));
            if (tile == nullptr) {
                continue;
            }
            LightMapEntry l = lmap->get({x, z});
            unsigned short tid;
            for (char fi=0; fi<6; fi++) {
                if (fi == 0) {
                    tid = tile->ceiling;
                } else if (fi == 1) {
                    tid = tile->floor;
                } else {
                    MapTile* tile2;
                    tid = tile->wall;
                    if (fi==3 && x > 0) {
                        tile2 = tileRegistry->of(map->get({x-1, z}));
                        if (tile2->wall > 0) {
                            continue;
                        }
                        l = lmap->get({x-1, z});
                    } else if (fi==2 && x < map->width()-1) {
                        tile2 = tileRegistry->of(map->get({x+1, z}));
                        if (tile2->wall > 0) {
                            continue;
                        }
                        l = lmap->get({x+1, z});
                    } else if (fi==5 && z > 0) {
                        tile2 = tileRegistry->of(map->get({x, z-1}));
                        if (tile2->wall > 0) {
                            continue;
                        }
                        l = lmap->get({x, z-1});
                    } else if (fi==4 && z < map->height()-1) {
                        tile2 = tileRegistry->of(map->get({x, z+1}));
                        if (tile2->wall > 0) {
                            continue;
                        }
                        l = lmap->get({x, z+1});
                    }
                }
                if (tid > 0) {
                    unsigned char lv = l.v / (1 + l.n);
                    char fo = fi*3*4;
                    for (char j=0; j<4; j++) {
                        // TraceLog(LOG_INFO, "vertex %d, %d, %d [%u]",
                            // V[fo + j*3 + 0] + x, V[fo + j*3 + 1], V[fo + j*3 + 2] + z, tid);
                        verts->append(
                            (vertexnumbers[j] << 30) | // Vertex number
                            (cubeverts[fo + j*3 + 1]<<29) | // Y position
                            (lv << 16) | // todo: light levels
                            ((cubeverts[fo + j*3 + 0] + x)<<8) | // X position
                            ((cubeverts[fo + j*3 + 2] + z)<<0) // Z position
                        );
                        verts->append(tid | ((l.r&0xF0)<<16) | ((l.g&0xF0)<<12) | ((l.b&0xF0)<<8)); // Tile ID and tint
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
void MapData::GenerateMesh() {
    std::thread threads[maps.length()];
    DynamicArray<unsigned int, 0>* vertarrays[maps.length()];
    DynamicArray<unsigned short, 0>* indexarrays[maps.length()];
    for (size_t i=0; i<maps.length(); i++) {
        vertarrays[i] = new DynamicArray<unsigned int, 0>();
        indexarrays[i] = new DynamicArray<unsigned short, 0>();
        vertarrays[i]->resize(maps[i].size()*6*4*2);
        indexarrays[i]->resize(maps[i].size()*36);
        threads[i] = std::thread(_GenerateMesh, &maps[i], lightmaps[i], tileRegistry, vertarrays[i], indexarrays[i]);
    }
    for (size_t i=0; i<maps.length(); i++) {
        if (threads[i].joinable()) {
            threads[i].join();
            IntMesh* mesh = meshes[i];
            if (mesh->verts != nullptr) {
                delete mesh->verts;
            }
            if (mesh->indices != nullptr) {
                delete mesh->indices;
            }
            mesh->vertexCount = vertarrays[i]->length() / 2;
            mesh->verts = vertarrays[i]->collapse();
            mesh->triangleCount = indexarrays[i]->length() / 3;
            mesh->indices = indexarrays[i]->collapse();
            TraceLog(LOG_INFO, "Generated level mesh #%llu with %u verts and %u triangles.",
                i, mesh->vertexCount, mesh->triangleCount);
            delete vertarrays[i];
            delete indexarrays[i];
        }
    }
}
#pragma endregion

#pragma region UploadMap()
void MapData::UploadMap(size_t mapno) {
    IntMesh* mesh = meshes[mapno];
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
    glBufferData(GL_ARRAY_BUFFER, mesh->vertexCount*2*sizeof(uint32_t), mesh->verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->triangleCount*3*sizeof(unsigned short), mesh->indices, GL_STATIC_DRAW);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*2, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*2, (void*)sizeof(uint32_t));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void MapData::UploadMap() {
    for (size_t i=0; i<meshes.length(); i++) {
        UploadMap(i);
    }
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
    return get(pos.x, pos.y, pos.z);
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
void MapData::setLight(Vector3 pos, unsigned char v, unsigned char r, unsigned char g, unsigned char b) {
    LightMapEntry* l = getLight(pos);
    if (l != nullptr) {
        *l = {r, g, b, v};
    }
}
void MapData::setLight(int x, int y, int z, unsigned char v, unsigned char r, unsigned char g, unsigned char b) {
    LightMapEntry* l = getLight(x, y, z);
    if (l != nullptr) {
        *l = {r, g, b, v};
    }
}
#pragma endregion

#pragma region getLight()
LightMapEntry* MapData::getLight(Vector3 pos) {
    return getLight(pos.x, pos.y, pos.z);
}
LightMapEntry* MapData::getLight(int x, int y, int z) {
    for (size_t i=0; i<lightmaps.length(); i++) {
        Vec3I p = positions[i];
        if (x >= p.x && y == p.y && z >= p.z) {
            LightMap* map = lightmaps[i];
            if (x-p.x < map->width() && z-p.z < map->height()) {
                return &map->get({x-(int)p.x, z-(int)p.z});
            }
        }
    }
    return nullptr;
}
#pragma endregion

#pragma region addLight()
void MapData::addLight(int x, int y, int z, unsigned int v, unsigned char r, unsigned char g, unsigned char b) {
    LightMapEntry* c = getLight(x, y, z);
    if (c != nullptr) {
        unsigned char n = c->n + 1;
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
        // TraceLog(LOG_INFO, "Added light to %d,%d,%d. New Light: %u/%u 0x%02X%02X%02X", x, y, z, c->v, n+1, r, g, b);
        *c = {r, g, b, n, v+c->v};
    }
}
#pragma endregion

#pragma region ShouldRenderMap
bool MapData::ShouldRenderMap(Vector3 pos, size_t mapno) {
    float d = Vector3Distance(pos, {(float)positions[mapno].x, (float)positions[mapno].y, (float)positions[mapno].z});
    return d < renderDistance;
}
#pragma endregion

#pragma region Draw()
void MapData::Draw(Vector3 camerapos) {
    unsigned int loc;
    glUseProgram(mainShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.id);
    loc = GetShaderLocation(mainShader, "texture0");
    glUniform1i(loc, 0);
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
    // loc = GetShaderLocation(mainShader, "cameraPosition");
    // glUniform3f(loc, camerapos.x, camerapos.y, camerapos.z);
    Matrix matView = rlGetMatrixModelview();
    Matrix matProjection = rlGetMatrixProjection();
    Matrix matModel = rlGetMatrixTransform();
    Matrix matModelView = MatrixMultiply(matModel, matView);
    Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);
    rlSetUniformMatrix(mainShader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);
    loc = GetShaderLocation(mainShader, "FogMin");
    glUniform1f(loc, fogMin);
    loc = GetShaderLocation(mainShader, "FogMax");
    glUniform1f(loc, fogMax);
    loc = GetShaderLocation(mainShader, "FogColor");
    glUniform4f(loc, fogColor[0], fogColor[1], fogColor[2], fogColor[3]);
    loc = GetShaderLocation(mainShader, "LightLevel");
    glUniform1f(loc, lightLevel);
    loc = GetShaderLocation(mainShader, "drawPosition");
    for (size_t i=0; i<meshes.length(); i++) {
        if (ShouldRenderMap(camerapos, i)) {
            glUniform3f(loc, positions[i].x, positions[i].y, positions[i].z);
            glBindVertexArray(meshes[i]->vao);
            glDrawElements(GL_TRIANGLES, meshes[i]->triangleCount*3, GL_UNSIGNED_SHORT, 0);
            glBindVertexArray(0);
        }
    }
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