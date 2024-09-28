#pragma once

#include "EntityRegistry.hpp"
#include "MapData.hpp"
#include "Registries.hpp"
#include "ScriptEngine/ScriptBytecode.hpp"
#include "external/glad.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <ios>

#define OpenGLDebug(s) if (GLenum e = glGetError()) printf("%s: OpenGL Error: %u\n", s, e)

class Entity {
    public:
    Vector3 pos;
    float rot, scale;
    unsigned short tno;
    unsigned short type;
    float timer;

    Entity(unsigned short t, unsigned short ty=0, Vector3 p={0,0,0}, float r=0.0, float s=1.0) {
        tno = t;
        pos = p;
        rot = r;
        scale = s;
        type = ty;
        timer = 0;
    }

    void Rotate(float r, bool set=false) {
        if (set) {
            rot = r;
        } else {
            rot += r;
        }
    }
    void Move(Vector3 dist, bool set=false) {
        if (set) {
            pos = dist;
        } else {
            pos = Vector3Add(pos, dist);
        }
    }
};

class EntityRenderer : public DynamicArray<Entity*> {
    unsigned int vao, vbo;
    static const constexpr char cubeverts[12] = {
        1, 0, 0,
        1, 1, 0,
        1, 1, 1,
        1, 0, 1,
    };
    static const constexpr char vertexnumbers[4] = {
        3, 1, 0, 2,
    };
    static const constexpr char triangleindices[6] = {
        0, 1, 2, 0, 2, 3,
    };
    static const constexpr float vertexdata[3*6] = {
        0, -0.25, 3,
        0.5, -0.25, 1,
        0.5, 0.25, 0,
        0, -0.25, 3,
        0.5, 0.25, 0,
        0, 0.25, 2,
    };
    public:
    void init() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 6*3*sizeof(float), vertexdata, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float)*3, nullptr);
        glEnableVertexAttribArray(0);
    }
    Entity* add(unsigned short tno, unsigned short type, Vector3 pos, float rot=0) {
        Entity* ent = new Entity(tno, type, pos, rot);
        append(ent);
        return ent;
    }
    Entity* add(unsigned short tno, Vector3 pos, float rot=0) {
        Entity* ent = new Entity(tno, 0, pos, rot);
        append(ent);
        return ent;
    }

    void Update(MapData* map, Vector3 camera, float dt) {
        for (size_t i=0; i<length(); i++) {
            Entity* ent = get(i);
            if (ent == nullptr || ent->type == 0) {
                continue;
            }
            EntityType* ent_type = GlobalEntityRegistry->of(ent->type);
            if (ent_type == nullptr) {
                continue;
            }
            if (ent_type->facesplayer) {
                Vector3 dir = Vector3Subtract(ent->pos, camera);
                float rot = atan2f(dir.z, dir.x);
                ent->Rotate(rot, true);
            }
            if (ent_type->script == 0) {
                continue;
            }
            Script* script = GlobalScriptRegistry->of(ent_type->script);
            if (script == nullptr) {
                continue;
            }
            long long rval = 0;
            long long argv[2] = {(signed)i, ent->tno};
            int res = script->code.run(2, argv, &rval);
            if (res != ScriptBytecode::Result::Success) {
                char name[64];
                char* data;
                size_t len = script->code.dump(&data);
                snprintf(name, sizeof(name), "script%u_dump.bin", script->id);
                std::ofstream fd(name, std::ios_base::out | std::ios_base::binary);
                if (fd.is_open()) {
                    fd.write(data, len);
                    fd.close();
                }
                ent_type->script = 0;
            }
        }
    }

    void Draw(MapData* map, Vector3 camera, float renderwidth) {
        Shader shader = map->spriteShader;
        glUseProgram(shader.id);
        unsigned int loc = GetShaderLocation(shader, "renderwidth");
        glUniform1f(loc, renderwidth);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, map->atlas.id);
        loc = GetShaderLocation(shader, "texture0");
        glUniform1i(loc, 0);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        Matrix matView = rlGetMatrixModelview();
        Matrix matProjection = rlGetMatrixProjection();
        Matrix matModel = rlGetMatrixTransform();
        Matrix matModelView = MatrixMultiply(matModel, matView);
        Matrix matModelViewProjection = MatrixMultiply(matModelView, matProjection);
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);
        loc = GetShaderLocation(shader, "renderwidth");
        glUniform1f(loc, renderwidth);
        loc = GetShaderLocation(shader, "FogMin");
        glUniform1f(loc, map->fogMin);
        loc = GetShaderLocation(shader, "FogMax");
        glUniform1f(loc, map->fogMax);
        loc = GetShaderLocation(shader, "FogColor");
        glUniform4f(loc, map->fogColor[0], map->fogColor[1], map->fogColor[2], map->fogColor[3]);
        loc = GetShaderLocation(shader, "LightLevel");
        glUniform1f(loc, map->lightLevel);
        unsigned int tnoloc = GetShaderLocation(shader, "tno");
        unsigned int t1loc = GetShaderLocation(shader, "texture1");
        unsigned int modelloc = GetShaderLocation(shader, "model");
        unsigned int scaleloc = GetShaderLocation(shader, "scale");
        for (size_t i=0; i<length(); i++) {
            Entity* ent = get(i);
            if (Vector3Distance(ent->pos, camera) < map->renderDistance) {
                LightMap* lmap = map->getLightMap(ent->pos);
                if (lmap != nullptr) {
                    glBindTexture(GL_TEXTURE_2D, lmap->getId());
                    glUniform1i(t1loc, 1);
                }
                Matrix modelmat = MatrixMultiply(MatrixRotateY(ent->rot), MatrixTranslate(ent->pos.x, ent->pos.y, ent->pos.z));
                rlSetUniformMatrix(modelloc, modelmat);
                glUniform1f(tnoloc, ent->tno);
                glUniform1f(scaleloc, ent->scale);
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, 2*3);
                OpenGLDebug("hmmm");
            }
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

extern EntityRenderer* GlobalEntityRenderer;