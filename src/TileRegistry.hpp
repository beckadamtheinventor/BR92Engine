#pragma once

#include "Json.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "Helpers.hpp"
#include <fstream>

#pragma region MapTile
class MapTile {
    public:
    unsigned short id;
    union {
        unsigned short flags;
        struct {
            bool isSolid : 1;
            bool isWall : 1;
            bool isSpawnable : 1;
            bool blocksLight : 1;
            bool solidFloor : 1;
            bool solidCeiling : 1;
        };
    };
    unsigned char light, tintr, tintg, tintb;
    unsigned short floor, ceiling, wall;
};
#pragma endregion

#pragma region MapTileRegistry
class MapTileRegistry : public Registry<MapTile> {
    public:
    bool load(const char* fname, TextureRegistry* GlobalTextureRegistry) {
        // load map tiles into registry
        fname = AssetPath::clone(fname);
        RegisteredTexture* nonetex = GlobalTextureRegistry->of("none");
        MapTile* tile = this->add("none");
        tile->floor = tile->ceiling = tile->wall = nonetex->id;
        tile->flags = 0;
        tile->light = tile->tintr = tile->tintg = tile->tintb = 0;

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
                        MapTile* tile = this->add(id);
                        tile->light = 0;
                        if (o.has("f")) {
                            tile->isSpawnable = true;
                            tile->isSolid = false;
                            tile->isWall = false;
                            tile->blocksLight = false;
                            if (o["f"].getType() == JSON::Type::String) {
                                const char* f = o["f"].getCString();
                                if (GlobalTextureRegistry->has(f)) {
                                    tile->floor = GlobalTextureRegistry->of(f)->id;
                                } else {
                                    JsonFormatError(fname, "Elements array member contains unknown texture id", f);
                                    return false;
                                }
                            } else if (o["f"].getType() == JSON::Type::Integer) {
                                long long i = o["f"].getInteger();
                                if (i >= 0 && i < 65536) {
                                    if (GlobalTextureRegistry->has(i)) {
                                        tile->floor = i;
                                    } else {
                                        JsonFormatError(fname, "Elements array member contains unknown tile id number", i);
                                        return false;
                                    }
                                } else {
                                    JsonFormatError(fname, "Elements array member contains out of bound tile id number", i);
                                    return false;
                                }
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value for field", "f");
                                return false;
                            }
                        }
                        if (o.has("c")) {
                            tile->isSolid = false;
                            tile->isWall = false;
                            tile->blocksLight = false;
                            if (o["c"].getType() == JSON::Type::String) {
                                const char* f = o["c"].getCString();
                                if (GlobalTextureRegistry->has(f)) {
                                    tile->ceiling = GlobalTextureRegistry->of(f)->id;
                                } else {
                                    JsonFormatError(fname, "Elements array member contains unknown texture id", f);
                                    return false;
                                }
                            } else if (o["c"].getType() == JSON::Type::Integer) {
                                long long i = o["c"].getInteger();
                                if (i >= 0 && i < 65536) {
                                    if (GlobalTextureRegistry->has(i)) {
                                        tile->ceiling = i;
                                    } else {
                                        JsonFormatError(fname, "Elements array member contains unknown tile id number", i);
                                        return false;
                                    }
                                } else {
                                    JsonFormatError(fname, "Elements array member contains out of bound tile id number", i);
                                    return false;
                                }
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type for field", "c");
                                return false;
                            }
                        }
                        if (o.has("w")) {
                            tile->isWall = true;
                            tile->isSolid = true;
                            tile->blocksLight = true;
                            if (o["w"].getType() == JSON::Type::String) {
                                const char* f = o["w"].getCString();
                                if (GlobalTextureRegistry->has(f)) {
                                    tile->wall = GlobalTextureRegistry->of(f)->id;
                                } else {
                                    JsonFormatError(fname, "Elements array member contains unknown texture id", f);
                                    return false;
                                }
                            } else if (o["w"].getType() == JSON::Type::Integer) {
                                long long i = o["w"].getInteger();
                                if (i >= 0 && i < 65536) {
                                    if (GlobalTextureRegistry->has(i)) {
                                        tile->wall = i;
                                    } else {
                                        JsonFormatError(fname, "Elements array member contains unknown tile id number", i);
                                        return false;
                                    }
                                } else {
                                    JsonFormatError(fname, "Elements array member contains out of bound tile id number", i);
                                    return false;
                                }
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be integer or string) for field", "w");
                                return false;
                            }
                        }
                        if (o.has("solid")) {
                            if (o["solid"].getType() == JSON::Type::Boolean) {
                                tile->isSolid = o["solid"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solid");
                                return false;
                            }
                        }
                        if (o.has("spawnable")) {
                            if (o["spawnable"].getType() == JSON::Type::Boolean) {
                                tile->isSpawnable = o["spawnable"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "spawnable");
                                return false;
                            }
                        }
                        if (o.has("wall")) {
                            if (o["wall"].getType() == JSON::Type::Boolean) {
                                tile->isSolid = o["wall"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "wall");
                                return false;
                            }
                        }
                        if (o.has("blockslight")) {
                            if (o["blockslight"].getType() == JSON::Type::Boolean) {
                                tile->blocksLight = o["blockslight"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "blockslight");
                                return false;
                            }
                        }
                        if (o.has("solidfloor")) {
                            if (o["solidfloor"].getType() == JSON::Type::Boolean) {
                                tile->solidFloor = o["solidfloor"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solidfloor");
                                return false;
                            }
                        }
                        if (o.has("solidceiling")) {
                            if (o["solidceiling"].getType() == JSON::Type::Boolean) {
                                tile->solidCeiling = o["solidceiling"].getBoolean();
                            } else {
                                JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solidceiling");
                                return false;
                            }
                        }
                        if (o.has("light")) {
                            if (o["light"].getType() == JSON::Type::Integer) {
                                tile->light = o["light"].getInteger();
                            } else {
                                JsonFormatError(fname, "Elements array mamber contains invalid value type (should be integer) for field", "light");
                                return false;
                            }
                        }
                        if (o.has("tint")) {
                            if (o["tint"].getType() == JSON::Type::Array) {
                                JSON::JSONArray& arr = o["tint"].getArray();
                                if (arr.length != 3) {
                                    JsonFormatError(fname, "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
                                    return false;
                                }
                                if (arr.members[0].getType() != JSON::Type::Integer ||
                                    arr.members[1].getType() != JSON::Type::Integer ||
                                    arr.members[2].getType() != JSON::Type::Integer) {
                                        JsonFormatError(fname, "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
                                        return false;
                                }
                                tile->tintr = arr.members[0].getInteger();
                                tile->tintg = arr.members[1].getInteger();
                                tile->tintb = arr.members[2].getInteger();
                            } else {
                                JsonFormatError(fname, "Elements array mamber contains invalid value type (should be 3-component integer array) for field", "tint");
                                return false;
                            }
                        } else {
                            tile->tintr = 255;
                            tile->tintg = 255;
                            tile->tintb = 255;
                        }
                    }
                }
            } else {
                JsonFormatError(fname, "Expected member \"elements\" in root containing an array of objects");
                return false;
            }
        } else {
            MissingAssetError(fname);
            return false;
        }
        delete [] fname;
        return true;
    }
};
#pragma endregion
