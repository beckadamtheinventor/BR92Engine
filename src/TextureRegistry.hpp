#pragma once

#include "AssetPath.hpp"
#include "json/json.hpp"
#include "Registry.hpp"
#include "Helpers.hpp"
#include "raylib.h"

using JSON = nlohmann::json;

struct RegisteredTexture {
    unsigned short id;
    float ux=0, uy=0, uw=0, uh=0;
    Image image;
};

#define TILE_WIDTH 128
#define ATLAS_WIDTH (TILE_WIDTH*64)
#define ATLAS_WIDTH_F ((float)ATLAS_WIDTH)

class TextureRegistry : public Registry<RegisteredTexture> {
    public:
    size_t length() {
        return nextid();
    }
    RegisteredTexture *add(std::string id) {
        return add(strdup(id.c_str()));
    }
    RegisteredTexture *add(const char *id) {
        Image i = LoadImage(AssetPath::texture(id));
        if (IsImageReady(i)) {
            if (i.width != TILE_WIDTH || i.height != TILE_WIDTH) {
                ImageResize(&i, TILE_WIDTH, TILE_WIDTH);
            }
            RegisteredTexture* tt = _add(id);
            tt->image = i;
            tt->id = length() - 1;
            return tt;
        }
        return nullptr;
    }
    Texture2D build() {
        Image atlas = GenImageColor(ATLAS_WIDTH, ATLAS_WIDTH, {0,0,0,0});
        unsigned int i = 0;
        for (unsigned int y=0; y<ATLAS_WIDTH; y += TILE_WIDTH) {
            for (unsigned int x=0; x<ATLAS_WIDTH; x += TILE_WIDTH) {
                RegisteredTexture* tt = of(i++);
                tt->ux = x / ATLAS_WIDTH_F;
                tt->uy = y / ATLAS_WIDTH_F;
                tt->uw = TILE_WIDTH / ATLAS_WIDTH_F;
                tt->uh = TILE_WIDTH / ATLAS_WIDTH_F;
                ImageDraw(
                    &atlas,
                    tt->image,
                    {0,0,TILE_WIDTH,TILE_WIDTH},
                    {(float)x, (float)y, TILE_WIDTH, TILE_WIDTH},
                    WHITE
                );
                UnloadImage(tt->image);
                if (!has(i)) {
                    break;
                }
            }
            if (!has(i)) {
                break;
            }
        }
        Texture2D atlastex = LoadTextureFromImage(atlas);
        GenTextureMipmaps(&atlastex);
        // ExportImage(atlas, "atlas.png");
        UnloadImage(atlas);
        return atlastex;
    }

    bool load(const char* fname) {
        fname = AssetPath::clone(fname);
        add("none");
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
                    if (arr[i].is_string()) {
                        std::string name = arr[i].get<std::string>();
                        RegisteredTexture* rt = this->add(name);
                        TraceLog(LOG_INFO, "Loaded texture %u (%s)", rt->id, name.c_str());
                    }
                }
            } else {
                JsonFormatError(fname, "Expected member \"elements\" in root containing an array of strings");
                return false;
            }
        } else {
            MissingAssetError(AssetPath::root("textures", "json"));
            return false;
        }
        delete [] fname;
        return true;
    }
};