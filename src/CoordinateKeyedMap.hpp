#pragma once

#include "Dictionary.hpp"
#include "Vec3.hpp"

/* Map keyed by a 3-coordinate integer vector */
template <class T>
class CoordinateKeyedMap : public Dictionary<T> {
    protected:
    /* Return a null-terminated string representing the key */
    const char* _key(int x, int y, int z) {
        static char k[5*3+1];
        for (char i=0; i<5; i++) {
            k[i] = ((unsigned)x & 0x7f) + 1;
            x >>= 7;
        }
        for (char i=5; i<10; i++) {
            k[i] = ((unsigned)y & 0x7f) + 1;
            y >>= 7;
        }
        for (char i=10; i<15; i++) {
            k[i] = ((unsigned)z & 0x7f) + 1;
            z >>= 7;
        }
        k[15] = 0;
        return (char*)k;
    }
    int _unkey_x(const char* key) {
        unsigned int x = 0;
        for (char i=0; i<5; i++) {
            x <<= 7;
            x |= key[i] - 1;
        }
        return *(int*)&x;
    }
    int _unkey_y(const char* key) {
        unsigned int x = 0;
        for (char i=5; i<10; i++) {
            x <<= 7;
            x |= key[i] - 1;
        }
        return *(int*)&x;
    }
    int _unkey_z(const char* key) {
        unsigned int x = 0;
        for (char i=10; i<15; i++) {
            x <<= 7;
            x |= key[i] - 1;
        }
        return *(int*)&x;
    }
    public:
    inline T& get(int x, int y, int z) {
        return _get(_key(x, y, z));
    }
    inline T& operator[](Vec3I pos) {
        return get(pos.x, pos.y, pos.z);
    }
};
