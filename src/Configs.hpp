#pragma once

#include "Dictionary.hpp"
#include "SimpleConfig.hpp"

using namespace SimpleConfig;

#define CFG_VERSION 0x00000200
#define CFG_PLAYER_HEIGHT 0.5f

class ConfigFile : public Config {
    protected:
    const char* _fname;
    // Dictionary<void*> _vars;
    public:
    ConfigFile(const char* fname) {
        _fname = fname;
    }
    // void var(const char* k, void* val) {
    //     _vars[k] = val;
    // }
    bool load() {
        return deserialize(_fname);
    }
    bool save() {
        // for (size_t i=0; i<_vars.length(); i++) {
        //     setRaw(_vars.keys(i), _vars.values(i));
        // }
        return serialize(_fname);
    }
};

class MainConfig : public ConfigFile {
    public:
    MainConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        setInteger("TargetFPS", -1);
        setUnsigned("WindowSizeX", 640);
        setUnsigned("WindowSizeY", 480);
        setInteger("WindowPosX", 20);
        setInteger("WindowPosY", 20);
        setBool("WindowFullscreen", false);
        setBool("WindowMaximized", true);
        setFloat("MouseSensitivity", 0.1f);
        setFloat("FOVY", 60);
        setFloat("PlayerX", 0);
        setFloat("PlayerY", CFG_PLAYER_HEIGHT);
        setFloat("PlayerZ", 0);
        setFloat("PlayerTX", 0.75f);
        setFloat("PlayerTY", 0);
        setFloat("PlayerTZ", 0);
        setFloat("PlayerUX", 0);
        setFloat("PlayerUY", 1);
        setFloat("PlayerUZ", 0);
        setFloat("RenderDistance", 60);
        setBool("CheatsEnabled", false);
        setBool("FreecamEnabled", false);
        setBool("GodmodeEnabled", false);
        setBool("NoclipEnabled", false);
        setUnsigned("RenderScale", 1920);

        // Load from file
        load();
        // Override config version
        setUnsigned("ConfigVersion", CFG_VERSION);
    }
};

class ShaderConfig : public ConfigFile {
    public:
    ShaderConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        setByte("FogColorR", 0.7f*255.0f);
        setByte("FogColorG", 0.7f*255.0f);
        setByte("FogColorB", 0.7f*255.0f);
        setByte("FogColorA", 1.0f*255.0f);
        setFloat("FogMin", 10.0f);
        setFloat("FogMax", 20.0f);
        setFloat("LightLevel", 1.0f);

        // Load from file
        load();
    }
};

class DevConfig : public ConfigFile {
    public:
    DevConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        setBool("DevEnabled", false);
        setBool("SaveMapOnExit", false);

        // Load from file
        load();
    }
};