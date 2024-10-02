#pragma once

#include "AssetPath.hpp"
#include "Helpers.hpp"
#include "Json.hpp"
#include "Registry.hpp"
#include "ScriptEngine/ScriptAssemblyCompiler.hpp"
#include "ScriptEngine/ScriptBytecode.hpp"
#include "ScriptEngine/ScriptInterface.hpp"

class Script {
    public:
    ScriptBytecode code;
    unsigned short id;
    Script() {
        code.setInterface(GloablScriptInterface);
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
        std::ifstream fd(fname);
        if (fd.is_open()) {
            size_t count = fstreamlen(fd);
            char* datastr = new char[count];
            fd.read(datastr, count);
            fd.close();
            ScriptAssemblyCompiler compiler;
            unsigned char* binary;
            size_t binlen = compiler.compile(datastr, count, &binary);
            code = ScriptBytecode(binary, binlen);
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
                        Script* script = this->add(id);
                        if (o.has("script")) {
                            if (o["script"].getType() == JSON::Type::String) {
                                script->load(AssetPath::root(o["script"].getCString(), nullptr));
                            } else {
                                JsonFormatError(fname, "Elements array member script component should be string (file name)");
                                return false;
                            }
                        }
                        script->code.setInterface(interface);
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