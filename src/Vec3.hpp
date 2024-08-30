#pragma once

#include <cmath>

#define _min(a, b) ((a)<(b)?(a):(b))
#define _max(a, b) ((a)>(b)?(a):(b))

template<class T, class S=double>
class Vec3 {
    public:
    T x, y, z;
    Vec3() : Vec3(0, 0) {}
    Vec3(T x) : Vec3(x, x, x) {}
    Vec3(T x, T y) : Vec3(x, y, 0) {}
    Vec3(T x, T y, T z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    Vec3 min(Vec3 o) {
        return Vec3(_min(x, o.x), _min(y, o.y), _min(z, o.z));
    }
    Vec3 max(Vec3 o) {
        return Vec3(_max(x, o.x), _max(y, o.y), _max(z, o.z));
    }
    Vec3 operator+(Vec3 o) {
        return Vec3(x+o.x, y+o.y, z+o.z);
    }
    Vec3 operator-(Vec3 o) {
        return Vec3(x-o.x, y-o.y, z+o.z);
    }
    Vec3 operator*(Vec3 o) {
        return Vec3(x*o.x, y*o.y, z*o.z);
    }
    Vec3 operator/(Vec3 o) {
        return Vec3(x/o.x, y/o.y, z/o.z);
    }
    Vec3 operator*(S o) {
        return Vec3(x*o, y*o, z*o);
    }
    Vec3 operator/(S o) {
        return Vec3(x/o, y/o, z/o);
    }
    operator T*() {
        return this;
    }
    T length() {
        return sqrt(x*x + y*y);
    }
};

typedef Vec3<double> Vec3D;
typedef Vec3<long long> Vec3L;
typedef Vec3<int> Vec3I;
