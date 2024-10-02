#ifndef __SCRIPT_INTERFACE_HPP__
#define __SCRIPT_INTERFACE_HPP__

#include <stdint.h>

class ScriptInterface {
    public:
    ScriptInterface();
    bool isSolid(unsigned short id);
    bool isSpawnable(unsigned short id);
    bool isWall(unsigned short id);
    unsigned short tileFloor(unsigned short id);
    unsigned short tileCeiling(unsigned short id);
    unsigned short tileWall(unsigned short id);
    float tileLightLevel(unsigned short id);
    unsigned short getTileId(int x, int y, int z);
    unsigned long getLightColor(int x, int y, int z);
    float cameraX();
    float cameraY();
    float cameraZ();
    float entityX(unsigned int id);
    float entityY(unsigned int id);
    float entityZ(unsigned int id);
    void entityMoveTowards(unsigned int id, float x, float y, float z, float speed);
    void entityRotate(unsigned int id, float r);
    void entityTeleport(unsigned int id, float x, float y, float z);
    bool canSeePlayer(unsigned int id);
    float getEntityTimer(unsigned int id);
    void setEntityTimer(unsigned int id, float v);
    void randomTeleportEntity(unsigned int id, float min_dist, float max_dist, bool avoid_player);
    float getDeltaTime();
};

extern ScriptInterface* GloablScriptInterface;

#endif