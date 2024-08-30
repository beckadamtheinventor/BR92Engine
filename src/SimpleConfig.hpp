#ifndef __SIMPLE_CONFIG_HPP__
#define __SIMPLE_CONFIG_HPP__
#ifndef __cplusplus
#error This header does not currently support C, only C++.
#endif
/* Bare-bones single-header binary serialized config library.
 * Authors: Adam "beckadamtheinventor" Beckingham
 *
 * Usage:
    SimpleConfig::Config config;
    // make sure to set defaults before loading from file
    config.add("SomeBoolValue", Value::fromBool(true));
    config.setFloat("SomeFloatValue", 13.37f);
    // loading from file
    config.deserialize("file.dat");
    // saving to file
    config.serialize("file.dat");
 */

// define CONFIG_FILE_HEADER before including this file to use a custom config file header 
#ifndef CONFIG_FILE_HEADER
#define CONFIG_FILE_HEADER "SmpleCfg"
#endif


#include "Dictionary.hpp"
#include <cstdio>
#include <ios>
#include <cmath>
#include <fstream>
#include <istream>
#include <ostream>
#include <string>

namespace SimpleConfig {
    static char* _dupcstr(const char* str, size_t len=0) {
        if (len == 0) {
            len = strlen(str);
        }
        char* s = new char[len + 1];
        memcpy(s, str, len);
        s[len] = 0;
        return s;
    }
    static char _parsehex(char c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'A' && c <= 'F') {
            return c - 'A';
        } else if (c >= 'a' && c <= 'f') {
            return c - 'a';
        }
        return -1;
    }
    static char* _deserializeString(std::istream* in) {
        char* s = new char[256];
        char c;
        size_t i = 0;
        while ((c = in->get()) != '"') {
            // return failiure if no end quote
            if (in->eof()) {
                return nullptr;
            }
            if (c == '\\') {
                c = in->get();
                if (c == 'x') {
                    char u = _parsehex(in->get());
                    if (u != -1) {
                        char l = _parsehex(in->get());
                        if (l == -1) {
                            s[i++] = u;
                        } else {
                            s[i++] = (u << 4) | l;
                        }
                    }
                    continue;
                } else if (c == '0') {
                    s[i++] = 0;
                    continue;
                }
            }
            s[i++] = c;
        }
        s[i] = 0;
        return s;
    }
    static void _skipspace(std::istream* in) {
        while (in->peek() == ' ' || in->peek() == '\t' || in->peek() == '\n') {
            in->get();
        }
    }
    class Value {
        public:
        enum Type {
            TNONE = 0,
            TTRUE,
            TFALSE,
            TINTEGER,
            TUNSIGNED,
            TDOUBLE,
            TSTRING,
            TFLOAT,
            TCHAR,
            TBYTE,
        };
        static const size_t INVALID_TYPE = TBYTE + 1;
        Type type;
        unsigned int dummy;
        union {
            long long i;
            size_t u;
            double d;
            float f;
            char c;
            unsigned char uc;
            char* s;
        };
        Value() {
            type = TNONE;
            this->i = 0;
        }
        static Value deserialize(char* buf, size_t l) {
            Value v = Value();
            if (l == 0) {
                return v;
            }
            char c = buf[0];
            if (c < INVALID_TYPE) {
                v.type = (Type) c;
                switch (c) {
                    case TNONE:
                    case TTRUE:
                    case TFALSE:
                        break;
                    case TINTEGER:
                        if (l >= sizeof(long long) + 1) {
                            v.i = *(long long*) &buf[1];
                        }
                        break;
                    case TUNSIGNED:
                        if (l >= sizeof(size_t) + 1) {
                            v.u = *(size_t*) &buf[1];
                        }
                        break;
                    case TDOUBLE:
                        if (l >= sizeof(double) + 1) {
                            v.d = *(double*) &buf[1];
                        }
                        break;
                    case TSTRING:
                        if (l < 2) {
                            v.s = new char[1] {0};
                        } else {
                            v.s = _dupcstr((char*)&buf[1], l-1);
                        }
                        break;
                    case TFLOAT:
                        if (l >= sizeof(float) + 1) {
                            v.f = *(float*) &buf[1];
                        }
                        break;
                    case TCHAR:
                        if (l >= sizeof(char) + 1) {
                            v.c = buf[1];
                        }
                    case TBYTE:
                        if (l >= sizeof(char) + 1) {
                            v.uc = buf[1];
                        }
                    default:
                        break;
                }
            }
            return v;
        }
        static Value deserializeText(std::istream *in, bool &success) {
            Value v = Value();
            char c = in->get();
            success = true;
            if (c == 't') {
                v.type = TTRUE;
            } else if (c == 'f') {
                v.type = TFALSE;
            } else if (c == '\'') {
                v.type = TUNSIGNED;
                v.u = in->get();
                if (in->get() != '\'') {
                    success = false;
                }
            } else if (c == '-' || c >= '0' && c <= '9') {
                size_t n = 0;
                size_t f = 0;
                size_t fl = 0;
                bool negative = false;
                if (c == '-') {
                    negative = true;
                    c = in->get();
                }
                while (c >= '0' && c <= '9') {
                    n = n * 10 + c - '0';
                    c = in->get();
                    if (c == '.') {
                        fl++;
                    }
                }
                if (fl > 0) {
                    while (c >= '0' && c <= '9') {
                        f = f * 10 + c - '0';
                        fl++;
                        c = in->get();
                    }
                    v.type = TDOUBLE;
                    v.d = (double)n + (double)f * powl(0.1, fl-1);
                } else {
                    if (negative) {
                        v.type = TINTEGER;
                        v.i = -(long long)n;
                    } else {
                        v.type = TUNSIGNED;
                        v.u = n;
                    }
                    
                }
                in->unget();
            } else if (c == '"') {
                v.type = TSTRING;
                v.s = _deserializeString(in);
                if (v.s == nullptr) {
                    success = false;
                }
            }
            return v;
        }
        static Value fromInteger(long long i) {
            Value v = Value();
            v.type = TINTEGER;
            v.i = i;
            return v;
        }
        static Value fromUnsigned(size_t u) {
            Value v = Value();
            v.type = TUNSIGNED;
            v.u = u;
            return v;
        }
        static Value fromDouble(double f) {
            Value v = Value();
            v.type = TDOUBLE;
            v.d = f;
            return v;
        }
        static Value fromFloat(float f) {
            Value v = Value();
            v.type = TFLOAT;
            v.f = f;
            return v;
        }
        static Value fromBool(bool b) {
            Value v = Value();
            v.type = b ? TTRUE : TFALSE;
            v.i = 0;
            return v;
        }
        static Value fromString(const char* s) {
            Value v = Value();
            v.type = TSTRING;
            v.s = _dupcstr(s);
            return v;
        }
        static Value fromString(std::string s) {
            return fromString(s.c_str());
        }
        static Value fromChar(char c) {
            Value v = Value();
            v.type = TCHAR;
            v.c = c;
            return v;
        }
        static Value fromByte(char c) {
            Value v = Value();
            v.type = TBYTE;
            v.uc = c;
            return v;
        }
        size_t serialize(char* buf) {
            size_t l;
            buf[0] = (char) type;
            switch (type) {
                case TTRUE:
                case TFALSE:
                case TNONE:
                    return 1;
                case TDOUBLE:
                    *(double*)&buf[1] = this->d;
                    return sizeof(double)+1;
                case TINTEGER:
                    *(long long*)&buf[1] = this->i;
                    return sizeof(long long)+1;
                case TUNSIGNED:
                    *(size_t*)&buf[1] = this->u;
                    return sizeof(size_t)+1;
                case TSTRING:
                    l = strlen(this->s);
                    if (l > 0) {
                        if (l > 255) {
                            printf("[SimpleConfig::Value.serialize()] Truncating string of length %llu (max 255)\n", l);
                            l = 255;
                        }
                        memcpy(&buf[1], this->s, l);
                    }
                    return l + 1;
                case TFLOAT:
                    *(float*)&buf[1] = this->f;
                    return sizeof(float)+1;
                case TCHAR:
                    buf[1] = this->c;
                    return 2;
                case TBYTE:
                    buf[1] = this->uc;
                    return 2;
                default:
                    break;
            }
            return 1;
        }
        bool isBool() {
            return type == TTRUE || type == TFALSE;
        }
        bool isInteger() {
            return type == TINTEGER || type == TUNSIGNED;
        }
        bool isDouble() {
            return type == TDOUBLE;
        }
        bool isFloat() {
            return type == TFLOAT;
        }
        bool isFloating() {
            return isFloat() || isDouble();
        }
        bool isNumber() {
            return isInteger() || isFloating();
        }
        bool isString() {
            return type == TSTRING;
        }
        bool isChar() {
            return type == TCHAR;
        }
        bool isByte() {
            return type == TBYTE || isChar();
        }
        bool getBool() {
            if (type == TFALSE) {
                return false;
            }
            return true;
        }
        long long getInteger() {
            if (type == TINTEGER) {
                return this->i;
            } else if (type == TUNSIGNED) {
                return this->u;
            } else if (type == TDOUBLE) {
                return this->d;
            } else if (type == TFLOAT) {
                return this->f;
            }
            return 0;
        }
        size_t getUnsigned() {
            if (type == TINTEGER) {
                return this->i;
            } else if (type == TUNSIGNED) {
                return this->u;
            } else if (type == TDOUBLE) {
                return this->d;
            } else if (type == TFLOAT) {
                return this->f;
            }
            return 0;
        }
        double getDouble() {
            if (type == TDOUBLE) {
                return this->d;
            } else if (type == TINTEGER) {
                return this->i;
            } else if (type == TUNSIGNED) {
                return this->u;
            } else if (type == TFLOAT) {
                return this->f;
            }
            return NAN;
        }
        float getFloat() {
            if (type == TDOUBLE) {
                return this->d;
            } else if (type == TINTEGER) {
                return this->i;
            } else if (type == TUNSIGNED) {
                return this->u;
            } else if (type == TFLOAT) {
                return this->f;
            }
            return NAN;
        }
        const char* getString() {
            if (type == TSTRING) {
                return this->s;
            }
            return nullptr;
        }
        char getChar() {
            if (type == TCHAR) {
                return this->c;
            }
            return 0;
        }
        unsigned char getByte() {
            if (isByte()) {
                return this->uc;
            }
            return 0;
        }
    };
    class Config {
        Dictionary<Value> *dict;
        public:
        Config() {
            dict = new Dictionary<Value>();
        }

        bool getBool(const char* key) {
            return dict->get(key).getBool();
        }
        long long getInteger(const char* key) {
            return dict->get(key).getInteger();
        }
        size_t getUnsigned(const char* key) {
            return dict->get(key).getUnsigned();
        }
        double getDouble(const char* key) {
            return dict->get(key).getDouble();
        }
        float getFloat(const char* key) {
            return dict->get(key).getFloat();
        }
        const char* getString(const char* key) {
            return dict->get(key).getString();
        }
        char getChar(const char* key) {
            return dict->get(key).getChar();
        }
        unsigned char getByte(const char* key) {
            return dict->get(key).getByte();
        }
        void set(const char* key, Value v) {
            dict->get(key) = v;
        }
        void setBool(const char* key, bool v) {
            dict->get(key) = Value::fromBool(v);
        }
        void setInteger(const char* key, long long v) {
            dict->get(key) = Value::fromInteger(v);
        }
        void setUnsigned(const char* key, size_t v) {
            dict->get(key) = Value::fromUnsigned(v);
        }
        void setDouble(const char* key, double v) {
            dict->get(key) = Value::fromDouble(v);
        }
        void setFloat(const char* key, float v) {
            dict->get(key) = Value::fromFloat(v);
        }
        void setChar(const char* key, char v) {
            dict->get(key) = Value::fromChar(v);
        }
        void setByte(const char* key, unsigned char v) {
            dict->get(key) = Value::fromByte(v);
        }
        void setString(const char* key, const char* v) {
            dict->get(key) = Value::fromString(v);
        }
        bool setRaw(const char* key, const void* v) {
            if (!dict->has(key)) {
                return false;
            }
            Value& val = dict->get(key);
            switch (val.type) {
                case Value::TNONE:
                    break;
                case Value::TTRUE:
                case Value::TFALSE:
                    val.type = *(bool*)v ? Value::TTRUE : Value::TFALSE;
                    break;
                case Value::TINTEGER:
                    val.i = *(long long*)v;
                    break;
                case Value::TUNSIGNED:
                    val.i = *(size_t*)v;
                    break;
                case Value::TDOUBLE:
                    val.i = *(double*)v;
                    break;
                case Value::TSTRING:
                    if (val.s != nullptr) {
                        delete[] val.s;
                    }
                    val.s = _dupcstr(*(char**)v);
                    break;
                case Value::TFLOAT:
                    val.f = *(float*)v;
                    break;
                case Value::TCHAR:
                    val.c = *(char*)v;
                    break;
                case Value::TBYTE:
                    val.uc = *(unsigned char*)v;
                    break;
                default:
                    return false;
            }
            return true;
        }


        inline size_t length() {
            return dict->length();
        }
        inline void add(const char* key, const Value val) {
            dict->add(key, val);
        }

        // Deserialize into this object from file fname. Returns true if successfully loaded.
        // Note: Will fail if the header is incorrect.
        bool deserialize(const char* fname) {
            std::ifstream fd;
            fd.open(fname);
            if (!fd.is_open()) {
                return false;
            }
            char buf[sizeof(CONFIG_FILE_HEADER)];
            fd.read(buf, sizeof(CONFIG_FILE_HEADER));
            bool res = !memcmp(buf, (char*)CONFIG_FILE_HEADER, sizeof(CONFIG_FILE_HEADER));
            if (res) {
                // if header, try decoding as binary
                res = deserialize(&fd);
                if (!res) {
                    dict->clear();
                }
            }
            if (!res) {
                // if no header or failed to decode as binary, try decoding as text
                res = deserializeText(&fd);
                if (!res) {
                    dict->clear();
                }
            }
            fd.close();
            return res;
        }
        
        // Deserialize into this object from istream* in. Returns true if successfully loaded.
        // Note: the input stream should not start with a header.
        bool deserialize(std::istream *in) {
            char buf[256];
            while (!in->eof()) {
                size_t l = in->get();
                char* key = new char[l+1];
                if (l > 0) {
                    in->read(key, l);
                }
                key[l] = 0;
                l = in->get() + 1;
                in->read(buf, l);
                Value value = Value::deserialize(buf, l);
                dict->get(key) = value;
            }
            return true;
        }

        // Deserialize from text into this object from istream* in. Returns true if successfully loaded.
        // Note: text format does not have a header.
        bool deserializeText(std::istream *in) {
            char buf[256];
            char c = 0;
            while (!in->eof()) {
                _skipspace(in);
                c = in->get();
                if (c == '"') {
                    char* key = _deserializeString(in);
                    if (key == nullptr) {
                        return false;
                    }
                    _skipspace(in);
                    c = in->get();
                    if (c == ':') {
                        _skipspace(in);
                        // define key with value
                        bool success = false;
                        Value v = Value::deserializeText(in, success);
                        if (!success) {
                            return false;
                        }
                        dict->get(key) = v;
                        if (in->peek() == ',') {
                            in->get();
                        }
                    } else if (c == ',') {
                        // define key as empty
                        dict->add(key, Value());
                    } else {
                        break;
                    }
                }
            }
            return true;
        }

        // Serialize this object into file fname. Returns true if successful.
        // Note: writes a header.
        bool serialize(const char* fname) {
            std::ofstream fd;
            fd.open(fname, std::ios::binary | std::ios::out);
            fd.write(CONFIG_FILE_HEADER, sizeof(CONFIG_FILE_HEADER));
            bool res = serialize(&fd);
            fd.flush();
            fd.close();
            return res;
        }

        // Serialize this object into ostream *out. Returns true if successful.
        // Note: this does not write a header.
        bool serialize(std::ostream *out) {
            char buf[256];
            size_t nkeys = dict->length();
            for (size_t i=0; i<nkeys; i++) {
                // printf("Index %llu\n", i);
                char* key = dict->keys(i);
                // printf("Key: %s\n", key);
                Value val = dict->values(i);
                // printf("Value Type: %u Value: %016llX\n", val.type, val.u);
                size_t vl = val.serialize(buf);
                // printf("Serialized Value Length: %llu\n", vl);
                // if the value's serialized length is zero, it is invalid, as there is no type byte.
                if (vl > 0) {
                    size_t kl = strlen(key);
                    out->put(kl);
                    // handle zero-length string keys, probably a bad thing to support.
                    if (kl > 0) {
                        out->write(key, kl);
                    }
                    out->put(vl - 1);
                    out->write(buf, vl);
                } else {
                    printf("[SimpleConfig::Config.serialize()] Failed to serialize value with key \"%s\"\n", key);
                }
            }
            return true;
        }

    };
}
#endif