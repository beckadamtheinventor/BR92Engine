#pragma once

#include "Helpers.hpp"
#include "json/json.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "ScriptRegistry.hpp"
#include "raylib.h"
#include <fstream>

using JSON = nlohmann::json;

class EntityType {
    public:
    char* name;
    unsigned short id;
    unsigned short script;
    unsigned short script_init;
    unsigned char nframes;
    union {
        unsigned char flags;
        struct {
            bool canmove : 1;
            bool facesplayer : 1;
        };
    };
    float frametime;
    float scale;
    unsigned short textures[16];
    EntityType() {
        Clear();
    }
    void Clear() {
        name = nullptr;
        id = script = script_init = 0;
        nframes = 1;
        flags = 0;
        frametime = 0.0f;
        scale = 1.0f;
        memset(textures, 0, sizeof(textures));
    }
};

class EntityRegistry : public Registry<EntityType> {
    public:
    bool load(const char* fname, TextureRegistry* GlobalTextureRegistry) {
        fname = AssetPath::clone(fname);
        this->add("none");
        std::ifstream fd(fname);
        if (fd.is_open()) {
            JSON json;
            try {
                fd >> json;
            } catch (nlohmann::detail::parse_error err) {
                TraceLog(LOG_ERROR, "Failed to load json from file %s! %s", fname, err.what());
                return false;
            }
            fd.close();
            if (json.contains("elements") && json["elements"].is_array()) {
                auto arr = json["elements"];
                for (size_t i=0; i<arr.size(); i++) {
                    if (arr[i].is_object()) {
                        auto o = arr[i];
                        std::string id;
                        if (o.contains("id") && o["id"].is_string()) {
                            id = o["id"].get<std::string>();
                        } else {
                            JsonFormatError(fname, "Elements array contains invalid member (missing string id)");
                            return false;
                        }
                        EntityType* ent = this->add(id);
                        ent->Clear();
                        ent->id = nextid() - 1;
                        memset(ent->textures, 0, sizeof(ent->textures));
                        if (o.contains("name")) {
                            if (o["name"].is_string()) {
                                ent->name = strdup(o["name"].get<std::string>().c_str());
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value for field", "name");
                                return false;
                            }
                        }
                        if (o.contains("textures")) {
                            RegisteredTexture* tex;
                            if (o["textures"].is_string()) {
                                tex = GlobalTextureRegistry->of(o["textures"].get<std::string>().c_str());
                                if (tex == nullptr) {
                                    JsonFormatError(fname, "Entity texture array contains missing texture ID", o["texture"].get<std::string>().c_str());
                                }
                            } else if (o["textures"].is_number()) {
                                tex = GlobalTextureRegistry->of(o["textures"]);
                                if (tex == nullptr) {
                                    JsonFormatError(fname, "Entity texture array contains missing texture ID", o["texture"].get<unsigned int>());
                                }
                            } else if (o["textures"].is_array()) {
                                auto tarr = o["textures"];
                                for (size_t i=0; i<tarr.size(); i++) {
                                    if (i >= 16) {
                                        break;
                                    }
                                    if (tarr[i].is_string()) {
                                        tex = GlobalTextureRegistry->of(tarr[i].get<std::string>().c_str());
                                        if (tex == nullptr) {
                                            JsonFormatError(fname, "Entity texture array contains missing texture ID", tarr[i].get<std::string>().c_str());
                                        }
                                        ent->textures[i] = tex->id;
                                    } else if (tarr[i].is_number()) {
                                        tex = GlobalTextureRegistry->of(tarr[i].get<unsigned int>());
                                        if (tex == nullptr) {
                                            JsonFormatError(fname, "Entity texture array contains missing texture ID", tarr[i].get<unsigned int>());
                                        }
                                        ent->textures[i] = tex->id;
                                    } else {
                                        JsonFormatError(fname, "Entity textures array member contains invalid value (should be string/int)");
                                    }
                                }
                                ent->nframes = tarr.size();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be string/int or array of string/int) for field", "textures");
                            }
                            if (o.contains("frametime")) {
                                if (o["frametime"].is_number()) {
                                    ent->frametime = o["frametime"].get<float>();
                                } else {
                                    JsonFormatError(fname, "Elements array member contains invalid value (should be float) for field", "frametime");
                                }
                            }
                        }
                        if (o.contains("scale")) {
                            if (o["scale"].is_number()) {
                                ent->scale = o["scale"].get<float>();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be float/int) for field", "scale");
                            }
                        }
                        if (o.contains("canmove")) {
                            if (o["canmove"].is_boolean()) {
                                ent->canmove = o["canmove"].get<bool>();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be bool) for field", "canmove");
                            }
                        }
                        if (o.contains("facesplayer")) {
                            if (o["facesplayer"].is_boolean()) {
                                ent->facesplayer = o["facesplayer"].get<bool>();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be bool) for field", "facesplayer");
                            }
                        }
                        if (o.contains("script")) {
                            if (o["script"].is_object()) {
                                auto oo = o["script"];
                                if (oo.contains("init")) {
                                    if (oo["init"].is_string()) {
                                        Script* script = GlobalScriptRegistry->of(oo["init"].get<std::string>().c_str());
                                        if (script == nullptr) {
                                            JsonFormatError(fname, "Elements array member references non-existent script id", oo["init"].get<std::string>().c_str());
                                        }
                                        ent->script_init = script->id;
                                    } else if (oo["init"].is_number_unsigned()) {
                                        Script* script = GlobalScriptRegistry->of(oo["init"].get<unsigned int>());
                                        if (script == nullptr) {
                                            JsonFormatError(fname, "Elements array member references non-existent script id", oo["init"].get<unsigned int>());
                                        }
                                        ent->script_init = script->id;
                                    } else {
                                        JsonFormatError(fname, "Elements array member contains invalid valid (should be string/int) for field", "script>init");
                                    }
                                }
                                if (oo.contains("update")) {
                                    if (oo["update"].is_string()) {
                                        Script* script = GlobalScriptRegistry->of(oo["update"].get<std::string>().c_str());
                                        if (script == nullptr) {
                                            JsonFormatError(fname, "Elements array member references non-existent script id", oo["update"].get<std::string>().c_str());
                                        }
                                        ent->script = script->id;
                                    } else if (oo["update"].is_number_unsigned()) {
                                        Script* script = GlobalScriptRegistry->of(oo["update"].get<unsigned int>());
                                        if (script == nullptr) {
                                            JsonFormatError(fname, "Elements array member references non-existent script id", oo["update"].get<unsigned int>());
                                        }
                                        ent->script = script->id;
                                    } else {
                                        JsonFormatError(fname, "Elements array member contains invalid valid (should be string/int) for field", "script>update");
                                    }
                                }
                            } else if (o["script"].is_string()) {
                                Script* script = GlobalScriptRegistry->of(o["script"].get<std::string>().c_str());
                                if (script == nullptr) {
                                    JsonFormatError(fname, "Elements array member references non-existent script id", o["script"].get<std::string>().c_str());
                                }
                                ent->script = script->id;
                            } else if (o["script"].is_number_unsigned()) {
                                Script* script = GlobalScriptRegistry->of(o["script"].get<unsigned int>());
                                if (script == nullptr) {
                                    JsonFormatError(fname, "Elements array member references non-existent script id", o["script"].get<unsigned int>());
                                }
                                ent->script = script->id;
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid valid (should be string/int) for field", "script");
                            }
                        }
                        TraceLog(LOG_INFO, "Loaded EntityType #%u script init: %u script update: %u", ent->id, ent->script_init, ent->script);
                    }
                }
            }
        } else {
            MissingAssetError(fname);
            return false;
        }
        delete [] fname;
        return true;
    }
};

extern EntityRegistry* GlobalEntityRegistry;
