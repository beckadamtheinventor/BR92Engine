#pragma once

#include <cstdio>
#include <string.h>

template<class T, size_t MIN_ALLOC=256>
class DynamicArray {
    size_t len;
    size_t alloc;
    T *items;
    public:
    DynamicArray<T, MIN_ALLOC>() {
        len = 0;
        alloc = MIN_ALLOC;
        items = new T[alloc];
    }
    DynamicArray<T, MIN_ALLOC>(size_t size) {
        len = 0;
        alloc = size;
        items = new T[alloc];
    }
    DynamicArray<T, MIN_ALLOC>(T* elements, size_t size) {
        len = alloc = size;
        items = new T[alloc];
        for (size_t i=0; i<size; i++) {
            items[i] = elements[i];
        }
    }
    void clear() {
        for (size_t i=0; i<len; i++) {
            items[i] = T();
        }
        len = 0;
    }

    size_t length() {
        return len;
    }
    size_t available() {
        return alloc - len;
    }
    void resize(size_t size) {
        if (size > 0) {
            T *newitems = new T[size];
            if (items != nullptr) {
                for (size_t i=0; i<size; i++) {
                    if (i < len) {
                        newitems[i] = items[i];
                    }
                }
                delete items;
            }
            items = newitems;
            alloc = size;
        } else {
            delete items;
            items = nullptr;
            alloc = len = 0;
        }
        // printf("[DynamicArray.resize(size_t)] Set size to %llu\n", size);
    }
    T& get(size_t i) {
        if (i >= alloc) {
            resize(i + MIN_ALLOC);
        }
        if (i >= len) {
            len = i + 1;
        }
        // printf("[DynamicArray.get(size_t)] Returning index %llu new allocated size: %llu\n", i, alloc);
        return items[i];
    }
    T& add(size_t i, T value) {
        return (get(i) = value);
    }
    inline T& operator[](size_t i) {
        return get(i);
    }
    T& append(T value) {
        return add(len, value);
    }
    void remove(size_t i) {
        if (len == 0) {
            return;
        }
        delete items[i];
        for (size_t j=i; j<len-1; j++) {
            items[j] = items[j+1];
        }
        items[len-1] = T();
    }
    void trim() {
        resize(len);
    }
    T* collapse() {
        if (len == 0) {
            return nullptr;
        }
        T* newitems = new T[len];
        for (size_t i=0; i<len; i++) {
            newitems[i] = items[i];
        }
        return newitems;
    }
    operator T*() {
        return items;
    }
};
