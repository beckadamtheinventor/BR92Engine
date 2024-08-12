/* Simple Array2D class.
 * Author: Adam "beckadamtheinventor" Beckingham
 * License: MIT
 */
#pragma once

#include <cstdio>
#include <exception>
#include <stdio.h>

struct ArrayIndex {
    int x, y;
};

template<class T>
class Array2D {
    int w, h;
    size_t l;
    T* values;
    public:
    Array2D<T>() : Array2D<T>(0, 0) {}
	/* Construct a new Array2D of a given width and height. */
    Array2D<T>(int width, int height) {
		values = nullptr;
        resize(width, height);
    }
	/* Construct a new Array2D from existing data */
    Array2D<T>(int width, int height, T* data) : Array2D<T>(width, height) {
        for (int i=0; i<l; i++) {
            values[i] = data[i];
        }
    }
	/* Return width of the Array */
    int width() {
        return w;
    }
	/* Return height of the Array */
    int height() {
        return h;
    }
	/* Return size of the Array. */
    int size() {
        return l;
    }
	/* Resize the Array. Note: destroys the data. */
    void resize(int width, int height) {
        w = width;
        h = height;
        l = w * h;
        if (l > 0) {
			if (values != nullptr) {
				delete values;
			}
            values = new T[l];
        } else {
            values = nullptr;
        }
    }
	/* Get/Set an item in the Array.
	   Note: brackets can be used to easily construct the index ({x, y}) */
    T& operator[](ArrayIndex i) {
        return get(i);
    }
    T& get(ArrayIndex i) {
        if (i.x < w && i.y < h) {
            return values[i.y * w + i.x];
        }
        printf("Array2D Index out of range\n");
        throw std::exception();
    }

    operator T*() {
        return values;
    }
};
