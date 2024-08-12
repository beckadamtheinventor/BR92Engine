
#include "MapData.hpp"
#include "AssetPath.hpp"
#include "raylib.h"
#include <functional>
#include <ios>
#include <thread>

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

void MapData::BuildAtlas() {
    atlas = textureRegistry->build();
}

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
        }
    }
    maps.append(map);
    positions.append({(float)x, (float)y, (float)z});
    meshes.append(new IntMesh{0});
    return true;
}

Vector3 MapData::rayCast(Vector3 pos, Vector3 dir, HitInfo& hit, size_t max_steps) {
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

static const constexpr char cubeverts[] = {
    // +y
    0, 1, 1,
    0, 1, 0,
    1, 1, 0,
    1, 1, 1,
    // -y
    1, 0, 0,
    0, 0, 0,
    0, 0, 1,
    1, 0, 1,
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

static void _GenerateMesh(IntMesh* mesh, TileArray* map, MapTileRegistry* tileRegistry) {
    DynamicArray<unsigned int>* verts = new DynamicArray<unsigned int>();
    DynamicArray<unsigned short>* indices = new DynamicArray<unsigned short>();
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
                    tid = tile->wall;
                }
                if (tid > 0) {
                    char fo = fi*3*4;
                    for (char j=0; j<4; j++) {
                        // TraceLog(LOG_INFO, "vertex %d, %d, %d [%u]",
                            // V[fo + j*3 + 0] + x, V[fo + j*3 + 1], V[fo + j*3 + 2] + z, tid);
                        verts->append(
                            (vertexnumbers[j] << 30) | // Vertex number
                            (cubeverts[fo + j*3 + 1]<<29) | // Y position
                            (0x7f << 16) | // todo: light levels
                            ((cubeverts[fo + j*3 + 0] + x)<<8) | // X position
                            ((cubeverts[fo + j*3 + 2] + z)<<0) // Z position
                        );
                        verts->append(tid | (0xfff<<12)); // Tile ID and tint
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
    if (mesh->verts != nullptr) {
        delete[] mesh->verts;
    }
    if (mesh->indices != nullptr) {
        delete[] mesh->indices;
    }
    mesh->vertexCount = verts->length() / 2;
    mesh->verts = verts->collapse();
    mesh->triangleCount = indices->length() / 3;
    mesh->indices = indices->collapse();
    delete verts;
    delete indices;
    TraceLog(LOG_INFO, "Generated level mesh with %u verts and %u triangles.", mesh->vertexCount, mesh->triangleCount);
}

void MapData::GenerateMesh() {
    std::thread threads[maps.length()];
    for (size_t i=0; i<maps.length(); i++) {
        threads[i] = std::thread(_GenerateMesh, meshes[i], &maps[i], tileRegistry);
        // _GenerateMesh(meshes[i], maps[i], tileRegistry);
    }
    for (size_t i=0; i<maps.length(); i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}

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

void MapData::Draw(Vector3 camerapos) {
    unsigned int loc;
    glUseProgram(mainShader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.id);
    loc = GetShaderLocation(mainShader, "texture0");
    glUniform1i(loc, 0);
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

void MapData::SetFog(float fogMin, float fogMax, float* fogColor) {
    this->fogMin = fogMin;
    this->fogMax = fogMax;
    this->fogColor[0] = fogColor[0];
    this->fogColor[1] = fogColor[1];
    this->fogColor[2] = fogColor[2];
    this->fogColor[3] = fogColor[3];
}
