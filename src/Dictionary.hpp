#pragma once

#include "DynamicArray.hpp"
#include <cstdio>
#include <string.h>

static size_t _hash(const char* s, size_t len=0) {
        if (s == nullptr) {
            return 0;
        }
        if (len == 0) {
            len = strlen(s);
        }
        size_t h = 0;
        for (size_t i=0; i<len; i++) {
            h = h*129 ^ s[i];
        }
        return h;
}
static char* _dupcstr(const char* key) {
    size_t len = strlen(key)+1;
    char *s = new char[len];
    memcpy(s, key, len);
    return s;
}

template<class T, size_t MIN_ALLOC=64, size_t BUCKETS=64>
class Dictionary {
    protected:
    class Sym {
        public:
        size_t hash;
        char* key;
        T value;
        Sym() {
            hash = 0;
            key = nullptr;
            value = T();
        }
        Sym(char *key) {
            this->hash = _hash(key);
            this->key = key;
            this->value = T();
        }
    };

    size_t len = 0;
    Sym *lastaccess = nullptr;
    DynamicArray<Sym, MIN_ALLOC>* buckets[BUCKETS] = {nullptr};

    Sym* getsym(const char *key, bool create=true) {
        size_t h = _hash(key);
        if (lastaccess != nullptr && h == lastaccess->hash) {
            if (!strcmp(key, lastaccess->key)) {
                return lastaccess;
            }
        }
        DynamicArray<Sym, MIN_ALLOC> *bucket = buckets[h % BUCKETS];
        // printf("%llX\n", bucket);
        // printf("Searching %llu items for key %s\n", bucket->length(), key);
        for (size_t i=0; i<bucket->length(); i++) {
            Sym *sym = &bucket->get(i);
            if (h == sym->hash) {
                if (!strcmp(key, sym->key)) {
                    lastaccess = sym;
                    return sym;
                }
            }
        }
        if (create) {
            Sym *sym = &bucket->append(Sym(_dupcstr(key)));
            len++;
            return sym;
        }
        return nullptr;
    }
    Sym* getsym(size_t i) {
        if (i < len) {
            for (size_t b = 0; b < BUCKETS; b++) {
                size_t l = buckets[b]->length();
                // printf("Bucket %llu length %llu index %llu\n", b, l, i);
                if (i < l) {
                    // printf("Found.\n");
                    return &buckets[b]->get(i);
                }
                i -= l;
            }
            // printf("Not Found.\n");
        }
        return nullptr;
    }

    public:
    Dictionary<T, MIN_ALLOC, BUCKETS>() {
        clear();
    }
    Dictionary<T, MIN_ALLOC, BUCKETS>(const char** keys, const T* values, size_t size) {
        clear();
        for (size_t i=0; i<size; i++) {
            add(keys[i], values[i]);
        }
    }

    void clear() {
        this->len = 0;
        this->lastaccess = nullptr;
        for (size_t i=0; i<BUCKETS; i++) {
            if (buckets[i] != nullptr) {
                delete buckets[i];
            }
            buckets[i] = new DynamicArray<Sym, MIN_ALLOC>();
        }
    }
    inline size_t length() {
        return len;
    }
    bool has(const char *key) {
        return getsym(key, false) != nullptr;
    }
    T& _get(const char* key) {
        return getsym(key)->value;
    }
    T& get(const char* key) {
        return _get(key);
    }
    inline T& add(const char* key, const T value) {
        // printf("Adding key %s new len %llu\n", key, len+1);
        return (get(key) = value);
    }
    inline T& append(const char *key, const T value) {
        return add(key, value);
    }
    inline T& operator[](const char *key) {
        return get(key);
    }
    inline bool has(size_t i) {
        return i < len;
    }
    inline T get(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return T();
        return sym->value;
    }
    inline T values(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return T();
        return sym->value;
    }
    inline char* keys(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return nullptr;
        return sym->key;
    }

};
