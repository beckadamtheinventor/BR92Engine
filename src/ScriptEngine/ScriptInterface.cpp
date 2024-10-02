
#include "ScriptInterface.hpp"
#include "../Registries.hpp"
#include "../MapData.hpp"
#include "../Engine.hpp"
#include "raylib.h"
#include "raymath.h"

ScriptInterface* GloablScriptInterface=nullptr;

ScriptInterface::ScriptInterface() {}

bool ScriptInterface::isSolid(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return false;
    }
    return tile->isSolid;
}

bool ScriptInterface::isSpawnable(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return false;
    }
    return tile->isSpawnable;
}

bool ScriptInterface::isWall(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return false;
    }
    return tile->isWall;
}

unsigned short ScriptInterface::tileFloor(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return 0;
    }
    return tile->floor;
}

unsigned short ScriptInterface::tileCeiling(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return 0;
    }
    return tile->ceiling;
}

unsigned short ScriptInterface::tileWall(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return 0;
    }
    return tile->wall;
}

float ScriptInterface::tileLightLevel(unsigned short id) {
    MapTile* tile = GlobalMapTileRegistry->of(id);
    if (tile == nullptr) {
        return 0.0f;
    }
    return tile->light / 128.0f;
}

unsigned short ScriptInterface::getTileId(int x, int y, int z) {
    return GlobalMapData->get(x, y, z);
}

unsigned long ScriptInterface::getLightColor(int x, int y, int z) {
    Color* c = GlobalMapData->getLight(x, y, z);
    if (c == nullptr) {
        return 0.0f;
    }
    return *(unsigned long*)c;
}

float ScriptInterface::cameraX() {
    return GlobalEngine->camera.position.x;
}

float ScriptInterface::cameraY() {
    return GlobalEngine->camera.position.y;
}

float ScriptInterface::cameraZ() {
    return GlobalEngine->camera.position.z;
}

float ScriptInterface::entityX(unsigned int id) {
    if (id >= GlobalEntityRenderer->length()) {
        return 0;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return 0;
    }
    return ent->pos.x;
}

float ScriptInterface::entityY(unsigned int id) {
    if (id >= GlobalEntityRenderer->length()) {
        return 0;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return 0;
    }
    return ent->pos.y;
}

float ScriptInterface::entityZ(unsigned int id) {
    if (id >= GlobalEntityRenderer->length()) {
        return 0;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return 0;
    }
    return ent->pos.z;
}

void ScriptInterface::entityMoveTowards(unsigned int id, float x, float y, float z, float speed) {
    if (id >= GlobalEntityRenderer->length()) {
        return;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return;
    }
    Vector3 dir = Vector3Scale(Vector3Normalize(Vector3Subtract({x, y, z}, ent->pos)), GlobalEngine->deltatime*speed);
    dir.y = 0;
    ent->Move(GlobalMapData->MoveTo(ent->pos, dir), true);
}

void ScriptInterface::entityRotate(unsigned int id, float r) {
    if (id >= GlobalEntityRenderer->length()) {
        return;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return;
    }
    ent->Rotate(r, true);
}
void ScriptInterface::entityTeleport(unsigned int id, float x, float y, float z) {
    if (id >= GlobalEntityRenderer->length()) {
        return;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return;
    }
    ent->Move({x, y, z}, true);
}

bool ScriptInterface::canSeePlayer(unsigned int id) {
    if (id >= GlobalEntityRenderer->length()) {
        return 0;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return false;
    }
    Vector3 dir = Vector3Normalize(Vector3Subtract(GlobalEngine->camera.position, ent->pos));
    HitInfo hit;
    GlobalMapData->RayCast(ent->pos, dir, hit);
    float dist = Vector3Distance(GlobalEngine->camera.position, ent->pos);
    return (hit.distance >= dist);
}

float ScriptInterface::getEntityTimer(unsigned int id) {
    if (id >= GlobalEntityRenderer->length()) {
        return 0;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return 0;
    }
    return ent->timer;
}

void ScriptInterface::setEntityTimer(unsigned int id, float v) {
    if (id >= GlobalEntityRenderer->length()) {
        return;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent != nullptr) {
        ent->timer = v;
    }
}

void ScriptInterface::randomTeleportEntity(unsigned int id, float min_dist, float max_dist, bool avoid_player) {
    if (id >= GlobalEntityRenderer->length()) {
        return;
    }
    Entity* ent = GlobalEntityRenderer->get(id);
    if (ent == nullptr) {
        return;
    }
    TraceLog(LOG_INFO, "Randomly teleporting entity %u", id);
    float dist;
    bool can_see_player = false;
    do {
        unsigned int i = rand() % GlobalMapData->spawnableSpaces.length();
        ent->Move(GlobalMapData->spawnableSpaces[i], true);
        dist = Vector3Distance(ent->pos, GlobalEngine->camera.position);
        if (avoid_player) {
            can_see_player = canSeePlayer(id);
        }
    } while (dist <= min_dist || dist >= max_dist || can_see_player);
}

float ScriptInterface::getDeltaTime() {
    return GlobalEngine->deltatime;
}