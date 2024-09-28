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
};

extern ScriptInterface* GloablScriptInterface;

#endif