#pragma once

#include "AssetPath.hpp"
#include "Helpers.hpp"
#include "json/json.hpp"
#include "Registry.hpp"
#include "ScriptEngine/ScriptAssemblyCompiler.hpp"
#include "ScriptEngine/ScriptBytecode.hpp"
#include "ScriptEngine/ScriptInterface.hpp"
#include "raylib.h"
#include <fstream>
#include <ios>

using JSON = nlohmann::json;

class Script {
    public:
    ScriptBytecode code;
    unsigned short id;
    Script() {
        code.setInterface(GloablScriptInterface);
        id = 0;
    }
    Script(const unsigned char* bytecode, size_t len) {
        load(bytecode, len);
    }
    Script(const char* fname) {
        load(fname);
    }
    void load(const unsigned char* bytecode, size_t len) {
        code = ScriptBytecode(bytecode, len);
    }
    bool load(const char* fname) {
        std::ifstream fd(fname, std::ios::binary);
        if (fd.is_open()) {
            size_t count = fstreamlen(fd);
            char* datastr = new char[count];
            fd.read(datastr, count);
            fd.close();
            ScriptAssemblyCompiler compiler;
            unsigned char* binary;
            size_t binlen = compiler.compile(datastr, count, &binary);
            code = ScriptBytecode(binary, binlen);
            std::ofstream ofd(std::string(fname)+".bin", std::ios::binary);
            if (ofd.is_open()) {
                ofd.write((char*)binary, binlen);
                ofd.close();
            }
            delete [] datastr;
            return true;
        }
        return false;
    }
};

class ScriptRegistry : public Registry<Script> {
    public:
    bool load(const char* fname, ScriptInterface* interface) {
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
                        Script* script = this->add(id);
                        script->id = nextid() - 1;
                        if (o.contains("script")) {
                            if (o["script"].is_string()) {
                                script->load(AssetPath::root(o["script"].get<std::string>().c_str(), nullptr));
                            } else {
                                JsonFormatError(fname, "Elements array member script component should be string (file name)");
                                return false;
                            }
                        }
                        script->code.setInterface(interface);
                        TraceLog(LOG_INFO, "Loaded Script #%u", script->id);
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

extern ScriptRegistry* GlobalScriptRegistry;