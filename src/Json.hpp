/* JSON serialization/deserialization and in-memory access library.
 * Author: Adam "beckadamtheinventor" Beckingham
 * License: MIT
 * Usage:
 *  JSON::JSON json = JSON::JSON::deserialize(filedata);
 *  char* serialized = json.serialize();
 */
#pragma once

#include <cstring>
#include <exception>
#include <string>

#include "Dictionary.hpp"

#define JSONMap Dictionary<JSON>

namespace JSON {
    typedef double jdouble_t;
    typedef size_t jsize_t;
    typedef long long jlong_t;
    typedef int jint_t;

    enum Type {
        Empty = 0,
        Object,
        Array,
        Null,
        Boolean,
        Integer,
        Float,
        String,
        User = 0x100,
    };

    static char* dupcstr(std::string s) {
        char* r = (char*) malloc(s.length()+1);
        if (r == NULL) {
            printf("JSON::dupcstr() out of memory");
            throw std::exception();
        }
        size_t i;
        for (i=0; i<s.length(); i++) {
            r[i] = s[i];
        }
        r[i] = 0;
        return r;
    }

    class JSON {
        public:
        class JSONArray {
            static const jsize_t MIN_ALLOC = 10;
            public:
            jsize_t length;
            jsize_t allocated;
            JSON *members;
            JSONArray() : JSONArray(MIN_ALLOC) {}
            JSONArray(jsize_t size) {
                allocated = size;
                length = 0;
                members = new JSON[size]();
            }
            JSONArray(JSON* members, jsize_t size) {
                this->allocated = size;
                this->length = size;
                this->members = members;
            }
            JSONArray(JSONArray& a) : JSONArray(a.length) {
                for (jsize_t i=0; i<a.length; i++) {
                    members[i] = a.members[i];
                }
            }
            ~JSONArray() {
                if (members != NULL && allocated > 0) {
                    delete members;
                }
            }
            jsize_t trim() {
                resize(length);
                return length;
            }
            void resize(jsize_t size) {
                if (size < length) {
                    size = length;
                }
                JSON* newmembers = new JSON[size]();
                for (jsize_t i=0; i<length; i++) {
                    newmembers[i] = members[i];
                }
                delete [] members;
                members = newmembers;
                allocated = size;
            }
            JSON& append(JSON object) {
                if (length + 1 >= allocated) {
                    resize(allocated + MIN_ALLOC);
                }
                members[length] = object;
				return members[length++];
            }
            void remove(jsize_t i) {
                if (i < length) {
                    if (i == length - 1) {
                        members[length] = JSON();
                    } else for (; i<length-1; i++)
                        members[i] = members[i+1];
                    length--;
                }
            }
            JSON& get(jsize_t i) {
                if (i >= allocated) {
                    resize(i + MIN_ALLOC);
                }
                if (i >= length) {
                    length = i+1;
                }
                return members[i];
            }
            JSON& operator[](jsize_t i) {
                return get(i);
            }
            JSON& operator+=(JSON& object) {
                return append(object);
            }
        };
        private:
        jsize_t type = Type::Empty;
        union value {
            const char* s;
            jdouble_t d;
            jlong_t i;
            JSONArray* a;
            JSONMap* o;
            void* p;
        } value;
        void type_error() {
            printf("Wrong type (0x%llX) for operation\n", type);
            throw std::exception();
        }
        public:
        JSON() {}
        JSON(JSONMap* o) {
            this->type = Type::Object;
            this->value.o = o;
        }
        JSON(JSONArray* a) {
            this->type = Type::Array;
            this->value.a = a;
        }
        JSON(jlong_t i) {
            this->type = Type::Integer;
            this->value.i = i;
        }
        JSON(jdouble_t d) {
            this->type = Type::Float;
            this->value.d = d;
        }
        JSON(char* s) {
            this->type = Type::String;
            this->value.s = s;
        }
        JSON(std::string s) {
            this->type = Type::String;
            this->value.s = (char*) s.c_str();
        }

        static JSON* fromDouble(jdouble_t d) {
            JSON* j = new JSON();
            j->setFloat(d);
            return j;
        }

        static JSON* fromUint(jsize_t i) {
            JSON* j = new JSON();
            j->setInteger(i);
            return j;
        }

        static JSON* fromInt(jint_t i) {
            JSON* j = new JSON();
            j->setInteger(i);
            return j;
        }

        JSONMap* set(JSONMap& o) { return setObject(o); }
        JSONMap* set(JSONMap* o) { return setObject(o); }
        JSONArray* set(JSONArray& a) { return setArray(a); }
        JSONArray* set(JSONArray* a) { return setArray(a); }
        jdouble_t set(jdouble_t v) { return setFloat(v); }
        jlong_t set(jlong_t v) { return setInteger(v); }
        jint_t set(jint_t v) { return setInteger(v); }
        const char* set(const char* s) { return setString(s); }
        std::string set(std::string s) { return setString(s); }
        bool set(bool v) { return setBoolean(v); }
        void set() { return setNull(); }
        void* set(void* p, jsize_t t) { return setCustom(p, t); }

        Type getType() {
            return static_cast<Type>(type);
        }

        jsize_t getCustomType() {
            return type;
        }

        void setCustomType(jsize_t t) {
            this->type = t;
        }

        void* getCustomValue() {
            return this->value.p;
        }

        void setCustomValue(void* p) {
            this->value.p = p;
        }

        JSON& operator[](jsize_t i) {
            if (type == Type::Array) {
                return value.a->get(i);
            }
            value.a->append(JSON());
            return value.a->get(value.a->length);
        }
        JSON& operator[](std::string key) {
            const char* s = key.c_str();
            if (type == Type::Object) {
                if (value.o->has(s)) {
                    return value.o->get(s);
                }
                return value.o->add(key.c_str(), JSON());
            }
            printf("Cannot index non-object with key string\n");
            throw std::exception();
        }
        JSON& operator[](char* key) {
            if (type == Type::Object) {
                if (value.o->has(key)) {
                    return value.o->get(key);
                }
                return value.o->add(key, JSON());
            }
            printf("Cannot index non-object with key string\n");
            throw std::exception();
        }
        bool contains(std::string key) {
            if (type == Type::Object) {
                return value.o->has(key.c_str());
            }
            return false;
        }
        bool contains(const char* key) {
            if (type == Type::Object) {
                return value.o->has(key);
            }
            return false;
        }
        const char* getCString() {
            if (type != Type::String) {
                type_error();
            }
            return value.s;
        }
        std::string getString() {
            if (type != Type::String) {
                type_error();
            }
            return std::string(value.s);
        }
        jdouble_t getNumber() {
            if (type == Type::Float) {
                return value.d;
            } else if (type == Type::Integer) {
                return value.i;
            }
            type_error();
            return 0;
        }
        jdouble_t getFloat() {
            if (type != Type::Float) {
                type_error();
            }
            return value.d;
        }
        jlong_t getInteger() {
            if (type != Type::Integer) {
                type_error();
            }
            return value.i;
        }
        jsize_t getUnsigned() {
            if (type != Type::Integer) {
                type_error();
            }
            return value.i;
        }
        bool getBoolean() {
            if (type != Type::Boolean) {
                type_error();
            }
            return value.i > 0;
        }
        bool isNull() {
            return type == Type::Null;
        }
        jsize_t getArrayLength() {
            if (type != Type::Array) {
                type_error();
            }
            return value.a->length;
        }
        jsize_t getObjectLength() {
            if (type != Type::Object) {
                type_error();
            }
            return value.o->length();
        }
        JSONMap& getObject() {
            if (type != Type::Object) {
                type_error();
            }
            return* value.o;
        }
        JSONArray& getArray() {
            if (type != Type::Array) {
                type_error();
            }
            return* value.a;
        }

        JSONMap* setObject(JSONMap& o) {
            this->type = Type::Object;
            this->value.o = new JSONMap(o);
			return this->value.o;
        }
        JSONMap* setObject(JSONMap* o) {
            this->type = Type::Object;
            this->value.o = o;
			return this->value.o;
        }
        JSONArray* setArray(JSONArray& a) {
            this->type = Type::Array;
            this->value.a = new JSONArray(a);
			return this->value.a;
        }
        JSONArray* setArray(JSONArray* a) {
            this->type = Type::Array;
            this->value.a = a;
			return this->value.a;
        }
        jdouble_t setFloat(jdouble_t d) {
            this->type = Type::Float;
            this->value.d = d;
			return d;
        }
        jlong_t setInteger(jlong_t i) {
            this->type = Type::Integer;
            this->value.i = i;
			return i;
        }
        const char* setString(const char* s) {
            this->type = Type::String;
            this->value.s = s;
			return s;
        }
        std::string setString(std::string s) {
            this->type = Type::String;
            this->value.s = (char*)s.c_str();
			return s;
        }
        bool setBoolean(bool v) {
            this->type = Type::Boolean;
            this->value.i = v ? 1 : 0;
			return v;
        }
        void setNull() {
            this->type = Type::Null;
            this->value.i = 0;
        }
        void* setCustom(void* p, jsize_t t) {
            this->type = t;
            this->value.p = p;
            return p;
        }

        const char* serialize() {
            jsize_t len;
            std::string o = std::string();
            switch (type) {
                case Type::Empty:
                    break;
                case Type::Null:
                    o = "null";
                    break;
                case Type::Boolean:
                    o = value.i?"true":"false";
                    break;
                case Type::Integer:
                    o = std::to_string(value.i);
                    break;
                case Type::Float:
                    o = std::to_string(value.d);
                    break;
                case Type::String:
                    if (value.s != NULL) {
                        o.reserve(strlen(value.s)+3);
                        int j = 0;
                        o[j++] = '"';
                        for (int i=0; value.s[i]>0; i++) {
                            if (value.s[i] == '\n') {
                                o[j++] = '\\';
                                o[j++] = 'n';
                            } else if (value.s[i] == '\t') {
                                o[j++] = '\\';
                                o[j++] = 't';
                            } else if (value.s[i] == '"') {
                                o[j++] = '\\';
                                o[j++] = '"';
                            } else {
                                o[j++] = value.s[i];
                            }
                        }
                        o[j++] = '"';
                    } else {
                        o.append("\"\"");
                    }
                    break;
                case Type::Array:
                    o.reserve(value.a->length*4);
                    o.append("[");
                    for (jsize_t i=0; i<value.a->length; i++) {
                        o.append(value.a->get(i).serialize());
                        if (i+1<value.a->length) {
                            o.append(",");
                            if (value.a->get(i).getType() == Type::Array || value.a->get(i).getType() == Type::Object) {
                                o.append("\n");
                            }
                        }
                    }
                    o.append("]");
                    break;
                case Type::Object:
                    len = value.o->length();
                    o.reserve(len*6);
                    o.append("{");
                    for (jsize_t i=0; i<len; i++) {
                        o.append("\"");
                        o.append(value.o->keys(i));
                        o.append("\": ");
                        o.append(value.o->values(i).serialize());
                        if (i+1 < len) {
                            o.append(",\n");
                        }
                    }
                    o.append("}");
                    break;
                default:
                    printf("Cannot serialize custom type with default method.\n\
                        Override the serialize() method for your custom type class if you need it.");
                    throw std::exception();
                    break;
            }
            o.shrink_to_fit();
            return dupcstr(o);
        }

        static JSON deserialize(const char* data) {
            jsize_t i = 0;
            return deserialize(data, i);
        }

        private:
        static char nibble(char c) {
            if (c >= '0' && c <= '9') {
                return c - '0';
            } else if (c >= 'A' && c <= 'F') {
                return c + 10 - 'A';
            } else if (c >= 'a' && c <= 'f') {
                return c + 10 - 'a';
            } else {
                return 0;
            }
        }
        static void skipspace(const char* data, jsize_t &i) {
            while (data[i] == ' ' || data[i] == '\t' || data[i] == '\n' || data[i] == ',') {
                i++;
            }
        }
        static char nextchar(const char* data, jsize_t &i) {
            skipspace(data, i);
            char c = data[i++];
            skipspace(data, i);
            return c;
        }

        static JSON deserialize(const char* data, jsize_t& i) {
            JSON o;
            skipspace(data, i);
            char c = data[i];
            if (c >= 'a' && c <= 'z') {
                if (!strncmp(&data[i], "null", 4)) {
                    o.setNull();
                    i += 4;
                } else if (!strncmp(&data[i], "false", 5)) {
                    o.setBoolean(false);
                    i += 5;
                } else if (!strncmp(&data[i], "true", 4)) {
                    o.setBoolean(false);
                    i += 4;
                } else {
                    o.setNull();
                }
            } else if (c >= '0' && c <= '9' || c == '-') {
                jsize_t j;
                bool flt = false;
                bool neg = false;
                bool hex = false;
                if (c == '-') {
                    neg = true;
                    c = data[++i];
                }
                if (c == '0' && data[i+1] == 'x') {
                    hex = true;
                    i+=2;
                }
                j = i;
                while (c != ',' && c != ']' && c != '}') {
                    c = data[j++];
                    if (c == '.') {
                        flt = true;
                    }
                }
                if (hex) {
                    jlong_t num = 0;
                    for (; i<j; j++) {
                        num = (num<<4) + nibble(data[i]);
                    }
                    if (neg) {
                        num = -num;
                    }
                    o.setInteger(num);
                } else if (flt) {
                    jdouble_t num = 0;
                    jdouble_t f = 1.0;
                    flt = false;
                    for (; i<j; i++) {
                        if (data[i] >= '0' && data[i] <= '9') {
                            if (flt) {
                                f *= 0.1;
                                num += f*(data[i] - '0');
                            } else {
                                num = num * 10 + data[i] - '0';
                            }
                        } else if (data[i] == '.') {
                            flt = true;
                        }
                    }
                    if (neg) {
                        num = -num;
                    }
                    o.setFloat(num);
                } else {
                    jlong_t num = 0;
                    for (; i<j; i++) {
                        if (data[i] >= '0' && data[i] <= '9') {
                            num = num * 10 + data[i] - '0';
                        }
                    }
                    if (neg) {
                        num = -num;
                    }
                    o.setInteger(num);
                }
                i--;
            } else if (c == '"') {
                std::string s;
                i++;
                do {
                    c = data[i++];
                    if (c == '\\') {
                        if (data[i] == 'n') {
                            s += '\n';
                        } else if (data[i] == 't') {
                            s += '\t';
                        } else if (data[i] == 'r') {
                            s += '\r';
                        } else if (data[i] == '0') {
                            s += '\0';
                        } else if (data[i] == 'x') {
                            i++;
                            s += (nibble(data[i]) << 4) | nibble(data[i+1]);
                            i += 2;
                        } else {
                            s += c;
                            s += data[i++];
                        }
                    } else if (c != '"') {
                        s += c;
                    }
                } while (c != '"');
                o.setString(dupcstr(s));
            } else if (c == '[') {
                JSONArray* a = new JSONArray();
                i++;
                skipspace(data, i);
                c = data[i];
                while (c != ']') {
                    skipspace(data, i);
                    JSON j = deserialize(data, i);
                    a->append(j);
                    skipspace(data, i);
                    c = data[i];
                }
                i++;
                o.setArray(a);
            } else if (c == '{') {
                JSONMap* a = new JSONMap();
                i++;
                skipspace(data, i);
                c = data[i];
                while (c != '}') {
                    skipspace(data, i);
                    const char* key = deserialize(data, i).getCString();
                    if (data[i] == ':') {
                        i++;
                    }
                    skipspace(data, i);
                    if (data[i] == ':') {
                        i++;
                    }
                    a->add(dupcstr(std::string(key)), deserialize(data, i));
                    skipspace(data, i);
                    c = data[i];
                }
                i++;
                o.setObject(a);
            }
            return o;
        }

    };
	typedef JSON::JSON::JSONArray JSONArray;
	typedef Dictionary<JSON> JSONObject;

    static JSON deserialize(const char* s) {
        return JSON::deserialize(s);
    }

    static JSON deserialize(std::string s) {
        return JSON::deserialize(s.c_str());
    }

}
