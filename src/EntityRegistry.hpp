#pragma once

#include "Helpers.hpp"
#include "Json.hpp"
#include "Registry.hpp"
#include "ScriptEngine/ScriptAssemblyCompiler.hpp"
#include "TextureRegistry.hpp"
#include "ScriptRegistry.hpp"
#include <fstream>

class EntityType {
    public:
    char* name=nullptr;
    unsigned short id;
    unsigned short script=0;
    unsigned char nframes=1;
    union {
        unsigned char flags=0;
        struct {
            bool canmove : 1;
            bool facesplayer : 1;
        };
    };
    float frametime=0.0f;
    unsigned short textures[16] = {0};
};

class EntityRegistry : public Registry<EntityType> {
    public:
    bool load(const char* fname, TextureRegistry* GlobalTextureRegistry) {
        fname = AssetPath::clone(fname);
        char* datastr;
        std::ifstream fd(fname);

        if (fd.is_open()) {
            size_t count = fstreamlen(fd);
            datastr = new char[count+1];
            fd.read(datastr, count);
            datastr[count] = 0;
            fd.close();
            JSON::JSON json = JSON::deserialize(datastr);
            delete datastr;
            if (json.contains("elements") && json["elements"].getType() == JSON::Type::Array) {
                JSON::JSONArray& arr = json["elements"].getArray();
                for (size_t i=0; i<arr.length; i++) {
                    if (arr[i].getType() == JSON::Type::Object) {
                        JSON::JSONObject& o = arr[i].getObject();
                        const char* id;
                        if (o.has("id") && o["id"].getType() == JSON::Type::String) {
                            id = o["id"].getCString();
                        } else {
                            JsonFormatError(fname, "Elements array contains invalid member (missing string id)");
                            return false;
                        }
                        EntityType* ent = this->add(id);
                        ent->name = nullptr;
                        memset(ent->textures, 0, sizeof(ent->textures));
                        if (o.has("name")) {
                            if (o["name"].getType() == JSON::Type::String) {
                                ent->name = (char*)o["name"].getCString();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value for field", "name");
                                return false;
                            }
                        }
                        if (o.has("textures")) {
                            RegisteredTexture* tex;
                            if (o["textures"].getType() == JSON::Type::String) {
                                tex = GlobalTextureRegistry->of(o["textures"].getCString());
                                if (tex == nullptr) {
                                    JsonFormatError(fname, "Entity texture array contains missing texture ID", o["texture"].getCString());
                                }
                            } else if (o["textures"].getType() == JSON::Type::Integer) {
                                tex = GlobalTextureRegistry->of(o["textures"].getInteger());
                                if (tex == nullptr) {
                                    JsonFormatError(fname, "Entity texture array contains missing texture ID", o["texture"].getInteger());
                                }
                            } else if (o["textures"].getType() == JSON::Type::Array) {
                                JSON::JSONArray& tarr = o["textures"].getArray();
                                for (size_t i=0; i<tarr.length; i++) {
                                    if (i >= 16) {
                                        break;
                                    }
                                    if (tarr[i].getType() == JSON::Type::String) {
                                        tex = GlobalTextureRegistry->of(tarr[i].getCString());
                                        if (tex == nullptr) {
                                            JsonFormatError(fname, "Entity texture array contains missing texture ID", tarr[i].getCString());
                                        }
                                        ent->textures[i] = tex->id;
                                    } else if (tarr[i].getType() == JSON::Type::Integer) {
                                        tex = GlobalTextureRegistry->of(tarr[i].getInteger());
                                        if (tex == nullptr) {
                                            JsonFormatError(fname, "Entity texture array contains missing texture ID", tarr[i].getInteger());
                                        }
                                        ent->textures[i] = tex->id;
                                    } else {
                                        JsonFormatError(fname, "Entity textures array member contains invalid value (should be string/int)");
                                    }
                                }
                                ent->nframes = tarr.length;
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be string/int or array of string/int) for field", "textures");
                            }
                            if (o.has("frametime")) {
                                if (o["frametime"].getType() == JSON::Type::Float) {
                                    ent->frametime = o["frametime"].getFloat();
                                } else {
                                    JsonFormatError(fname, "Elements array member contains invalid value (should be float) for field", "frametime");
                                }
                            }
                        }
                        if (o.has("canmove")) {
                            if (o["canmove"].getType() == JSON::Type::Boolean) {
                                ent->canmove = o["canmove"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be bool) for field", "canmove");
                            }
                        }
                        if (o.has("facesplayer")) {
                            if (o["facesplayer"].getType() == JSON::Type::Boolean) {
                                ent->facesplayer = o["facesplayer"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value (should be bool) for field", "facesplayer");
                            }
                        }
                        if (o.has("script")) {
                            if (o["script"].getType() == JSON::Type::String) {
                                Script* script = GlobalScriptRegistry->of(o["script"].getCString());
                                if (script == nullptr) {
                                    JsonFormatError(fname, "Elements array member references non-existant script id", o["script"].getCString());
                                }
                                ent->script = script->id;
                            } else if (o["script"].getType() == JSON::Type::Integer) {
                                Script* script = GlobalScriptRegistry->of(o["script"].getInteger());
                                if (script == nullptr) {
                                    JsonFormatError(fname, "Elements array member references non-existant script id", o["script"].getInteger());
                                }
                                ent->script = script->id;
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid valid (should be string/int) for field", "script");
                            }
                        }
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
