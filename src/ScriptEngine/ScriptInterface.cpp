
#include "ScriptInterface.hpp"
#include "../Registries.hpp"
#include "../MapData.hpp"

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
