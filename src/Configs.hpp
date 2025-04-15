#pragma once

#include "Dictionary.hpp"
#include "json/json.hpp"
#include <fstream>

using JSON = nlohmann::json;

#define CFG_VERSION 0x00000200
#define CFG_PLAYER_HEIGHT 0.5f

class ConfigFile : public JSON {
    protected:
    const char* _fname;
    // Dictionary<void*> _vars;
    public:
    ConfigFile() {}
    ConfigFile(const char* fname) {
        _fname = fname;
    }
    // void var(const char* k, void* val) {
    //     _vars[k] = val;
    // }
    bool load() {
        try {
            std::ifstream fd(_fname);
            if (fd.is_open()) {
                fd >> *this;
                fd.close();
                return true;
            } else {
                return false;
            }
        } catch (std::exception ignored) {
            return false;
        }
    }
    bool save() {
        // for (size_t i=0; i<_vars.length(); i++) {
        //     setRaw(_vars.keys(i), _vars.values(i));
        // }
        try {
            std::ofstream fd(_fname);
            if (fd.is_open()) {
                fd << *this;
                fd.close();
                return true;
            } else {
                return false;
            }
        } catch (std::exception ignored) {
            return false;
        }
    }
};

class MainConfig : public ConfigFile {
    public:
    MainConfig() {}
    MainConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        (*this)["TargetFPS"] = -1;
        (*this)["WindowSizeX"] = 640;
        (*this)["WindowSizeY"] = 480;
        (*this)["WindowPosX"] = 20;
        (*this)["WindowPosY"] = 40;
        (*this)["WindowFullscreen"] = false;
        (*this)["WindowMaximized"] = true;
        (*this)["WindowSensitivity"] = 0.1f;
        (*this)["FOVY"] = 60.0f;
        (*this)["PlayerX"] = 0.0f;
        (*this)["PlayerY"] = CFG_PLAYER_HEIGHT;
        (*this)["PlayerZ"] = 0.0f;
        (*this)["PlayerTX"] = 0.75f;
        (*this)["PlayerTY"] = 0.0f;
        (*this)["PlayerTZ"] = 0.0f;
        (*this)["PlayerUX"] = 0.0f;
        (*this)["PlayerUY"] = 1.0f;
        (*this)["PlayerUZ"] = 0.0f;
        (*this)["RenderDistance"] = 60.0f;
        (*this)["CheatsEnabled"] = false;
        (*this)["FreecamEnabled"] = false;
        (*this)["GodmodeEnabled"] = false;
        (*this)["NoclipEnabled"] = false;
        (*this)["RenderScale"] = 1920;

        // Load from file
        load();
        // Override config version
        (*this)["ConfigVersion"] = CFG_VERSION;
    }
};

class ShaderConfig : public ConfigFile {
    public:
    ShaderConfig() {}
    ShaderConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        (*this)["FogColorR"] = 0.7f*255.0f;
        (*this)["FogColorG"] = 0.7f*255.0f;
        (*this)["FogColorB"] = 0.7f*255.0f;
        (*this)["FogColorA"] = 1.0f*255.0f;
        (*this)["FogMin"] = 10.0f;
        (*this)["FogMax"] = 20.0f;
        (*this)["LightLevel"] = 1.0f;

        // Load from file
        load();
    }
};

class DevConfig : public ConfigFile {
    public:
    DevConfig() {}
    DevConfig(const char* fname) : ConfigFile(fname) {
        // Set defaults
        (*this)["DevEnabled"] = false;
        (*this)["SaveMapOnExit"] = false;

        // Load from file
        load();
    }
};