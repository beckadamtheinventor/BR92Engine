#pragma once

#include "AssetPath.hpp"
#include "Json.hpp"
#include "Registry.hpp"
#include "Helpers.hpp"
#include "raylib.h"

struct RegisteredTexture {
    unsigned short id;
    float ux, uy, uw, uh;
    Image image;
};

class TextureRegistry : public Registry<RegisteredTexture> {
    public:
    size_t length() {
        return nextid();
    }
    RegisteredTexture *add(const char *id) {
        Image i = LoadImage(AssetPath::texture(id));
        if (IsImageReady(i)) {
            if (i.width != 64 || i.height != 64) {
                ImageResize(&i, 64, 64);
            }
            RegisteredTexture* tt = new RegisteredTexture();
            tt->id = nextid();
            tt->image = i;
            tiles.append(tt);
            dict[id] = tt->id;
            return tt;
        }
        return nullptr;
    }
    Texture2D build() {
        Image atlas = GenImageColor(4096, 4096, {0,0,0,0});
        unsigned int i = 0;
        for (unsigned int y=0; y<4096; y += 64) {
            for (unsigned int x=0; x<4096; x += 64) {
                RegisteredTexture* tt = of(i++);
                tt->ux = x / 4096.0f;
                tt->uy = y / 4096.0f;
                tt->uw = 64 / 4096.0f;
                tt->uh = 64 / 4096.0f;
                ImageDraw(
                    &atlas,
                    tt->image,
                    {0,0,64,64},
                    {(float)x, (float)y, 64, 64},
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
        char* datastr;
        std::ifstream fd(fname);
        if (fd.is_open()) {
            size_t count = fstreamlen(fd);
            datastr = new char[count+1];
            fd.read(datastr, count);
            datastr[count] = 0;
            fd.close();
            JSON::JSON json = JSON::deserialize(datastr);
            delete [] datastr;
            if (json.contains("elements") && json["elements"].getType() == JSON::Type::Array) {
                JSON::JSONArray& arr = json["elements"].getArray();
                for (size_t i=0; i<arr.length; i++) {
                    if (arr[i].getType() == JSON::Type::String) {
                        this->add(arr[i].getCString());
                    }
                }
            } else {
                JsonFormatError("textures.json", "Expected member \"elements\" in root containing an array of strings");
                return false;
            }
        } else {
            MissingAssetError(AssetPath::root("textures", "json"));
            return false;
        }
        delete fname;
        return true;
    }
};