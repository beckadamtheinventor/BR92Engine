#pragma once

#include "json/json.hpp"
#include "Registry.hpp"
#include "TextureRegistry.hpp"
#include "Helpers.hpp"
#include "raylib.h"
#include <fstream>

using JSON = nlohmann::json;

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
    void Clear() {
        id = flags = floor = ceiling = wall = 0;
        light = tintr = tintg = tintb = 0;
    }
};
#pragma endregion

#pragma region MapTileRegistry
class MapTileRegistry : public Registry<MapTile> {
    public:
    bool load(const char* fname, TextureRegistry* GlobalTextureRegistry) {
        // load map tiles into registry
        fname = AssetPath::clone(fname);
        RegisteredTexture* nonetex = GlobalTextureRegistry->of("none");
        MapTile* nonetile = this->add("none");
        nonetile->floor = nonetile->ceiling = nonetile->wall = nonetex->id;
        nonetile->flags = 0;
        nonetile->light = nonetile->tintr = nonetile->tintg = nonetile->tintb = 0;

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
                int idx = 0;
                for (auto o : json["elements"]) {
                    std::string id;
                    if (o.contains("id") && o["id"].is_string()) {
                        id = o["id"].get<std::string>();
                    } else {
                        JsonFormatError(fname, "Elements array contains invalid member (missing string id)");
                        return false;
                    }
                    MapTile* tile = this->add(id);
                    tile->Clear();
                    tile->id = idx++;
                    tile->light = 0;
                    if (o.contains("f")) {
                        tile->isSpawnable = true;
                        tile->isSolid = false;
                        tile->isWall = false;
                        tile->blocksLight = false;
                        tile->solidFloor = true;
                        if (o["f"].is_string()) {
                            std::string f = o["f"].get<std::string>();
                            if (GlobalTextureRegistry->has(f.c_str())) {
                                tile->floor = GlobalTextureRegistry->of(f.c_str())->id;
                            } else {
                                JsonFormatError(fname, "Elements array member contains unknown texture id", f.c_str());
                                return false;
                            }
                        } else if (o["f"].is_number_unsigned()) {
                            int i = o["f"].get<int>();
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
                    if (o.contains("c")) {
                        tile->isSolid = false;
                        tile->isWall = false;
                        tile->blocksLight = false;
                        tile->solidCeiling = true;
                        if (o["c"].is_string()) {
                            std::string f = o["c"].get<std::string>();
                            if (GlobalTextureRegistry->has(f.c_str())) {
                                tile->ceiling = GlobalTextureRegistry->of(f.c_str())->id;
                            } else {
                                JsonFormatError(fname, "Elements array member contains unknown texture id", f.c_str());
                                return false;
                            }
                        } else if (o["c"].is_number_unsigned()) {
                            long long i = o["c"].get<int>();
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
                    if (o.contains("w")) {
                        tile->isWall = true;
                        tile->isSolid = true;
                        tile->blocksLight = true;
                        tile->solidFloor = tile->solidCeiling = true;
                        if (o["w"].is_string()) {
                            std::string f = o["w"].get<std::string>();
                            if (GlobalTextureRegistry->has(f.c_str())) {
                                tile->wall = GlobalTextureRegistry->of(f.c_str())->id;
                            } else {
                                JsonFormatError(fname, "Elements array member contains unknown texture id", f.c_str());
                                return false;
                            }
                        } else if (o["w"].is_number_unsigned()) {
                            long long i = o["w"].get<int>();
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
                    if (o.contains("solid")) {
                        if (o["solid"].is_boolean()) {
                            tile->isSolid = o["solid"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solid");
                            return false;
                        }
                    }
                    if (o.contains("spawnable")) {
                        if (o["spawnable"].is_boolean()) {
                            tile->isSpawnable = o["spawnable"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "spawnable");
                            return false;
                        }
                    }
                    if (o.contains("wall")) {
                        if (o["wall"].is_boolean()) {
                            tile->isSolid = o["wall"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "wall");
                            return false;
                        }
                    }
                    if (o.contains("blockslight")) {
                        if (o["blockslight"].is_boolean()) {
                            tile->blocksLight = o["blockslight"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "blockslight");
                            return false;
                        }
                    }
                    if (o.contains("solidfloor")) {
                        if (o["solidfloor"].is_boolean()) {
                            tile->solidFloor = o["solidfloor"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solidfloor");
                            return false;
                        }
                    }
                    if (o.contains("solidceiling")) {
                        if (o["solidceiling"].is_boolean()) {
                            tile->solidCeiling = o["solidceiling"].get<bool>();
                        } else {
                            JsonFormatError(fname, "Elements array member contains invalid value type (should be bool) for field", "solidceiling");
                            return false;
                        }
                    }
                    if (o.contains("light")) {
                        if (o["light"].is_number()) {
                            tile->light = o["light"].get<int>();
                        } else {
                            JsonFormatError(fname, "Elements array mamber contains invalid value type (should be integer) for field", "light");
                            return false;
                        }
                    }
                    if (o.contains("tint")) {
                        if (o["tint"].is_array()) {
                            auto arr = o["tint"];
                            if (arr.size() != 3) {
                                JsonFormatError(fname, "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
                                return false;
                            }
                            if (!(arr[0].is_number() &&
                                    arr[1].is_number() &&
                                    arr[2].is_number())) {
                                    JsonFormatError(fname, "Elements array member conatins invalid value type (should be 3-component integer array) for field", "tint");
                                    return false;
                            }
                            tile->tintr = arr[0].get<int>();
                            tile->tintg = arr[1].get<int>();
                            tile->tintb = arr[2].get<int>();
                        } else {
                            JsonFormatError(fname, "Elements array mamber contains invalid value type (should be 3-component integer array) for field", "tint");
                            return false;
                        }
                    } else {
                        tile->tintr = 255;
                        tile->tintg = 255;
                        tile->tintb = 255;
                    }
                    TraceLog(LOG_INFO, "Loaded tile #%u wall %u floor %u ceiling %u spawnable %s", tile->id, tile->wall, tile->floor, tile->ceiling, tile->isSpawnable ? "true" : "false");
                }
            } else {
                JsonFormatError(fname, "Expected member \"elements\" in root containing an array of objects");
                return false;
            }
        } else {
            MissingAssetError(fname);
            return false;
        }
        TraceLog(LOG_INFO, "Loaded %u tiles", length());
        delete [] fname;
        return true;
    }
};
#pragma endregion
