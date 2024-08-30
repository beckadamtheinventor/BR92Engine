#pragma once

#include "raylib.h"
#include "Helpers.hpp"
#include <cstring>
#include <fstream>

#define GLVERSIONHEADER "#version 330 core\n"

class ShaderLoader {
    public:
    static Shader load(const char* filename) {
        std::ifstream fd(filename);
        if (fd.is_open()) {
            size_t len = fstreamlen(fd);
            char* data = new char[len];
            fd.read(data, len);
            fd.close();
            char* vs_start = strstr(data, "VERTPROGRAM");
            char* fs_start = strstr(data, "FRAGPROGRAM");
            if (vs_start == nullptr || fs_start == nullptr) {
                return Shader {0};
            }
            vs_start += 12;
            fs_start += 12;
            char* vs_end = strstr(vs_start, "ENDPROGRAM");
            char* fs_end = strstr(fs_start, "ENDPROGRAM");
            if (vs_end == nullptr || fs_end == nullptr) {
                return Shader {0};
            }
            char* vs = new char[vs_end+strlen(GLVERSIONHEADER)+1-vs_start];
            char* fs = new char[fs_end+strlen(GLVERSIONHEADER)+1-fs_start];
            memcpy(vs, GLVERSIONHEADER, strlen(GLVERSIONHEADER));
            memcpy(fs, GLVERSIONHEADER, strlen(GLVERSIONHEADER));
            memcpy(&vs[strlen(GLVERSIONHEADER)], vs_start, vs_end-vs_start);
            memcpy(&fs[strlen(GLVERSIONHEADER)], fs_start, fs_end-fs_start);
            vs[vs_end+strlen(GLVERSIONHEADER)-vs_start] = 0;
            fs[fs_end+strlen(GLVERSIONHEADER)-fs_start] = 0;
            delete [] data;
            Shader shader = LoadShaderFromMemory(vs, fs);
            delete [] vs;
            delete [] fs;
            return shader;
        }
        return Shader {0};
    }
};