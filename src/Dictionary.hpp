/* Simple string keyed dictionary class. Saves keys for later use, ideal for serialization/deserialization.
 * Relies on std::vector
 * Author: Adam "beckadamtheinventor" Beckingham
 * License: MIT
 */
#pragma once

#include <cstdio>
#include <string.h>
#include <vector>

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
inline static char* _dupcstr(const char* str, size_t len=0) {
    if (len == 0) {
        len = strlen(str)+1;
    }
    char *s = new char[len];
    memcpy(s, str, len);
    return s;
}

template<class T, size_t BUCKETS=64>
class Dictionary {
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
    std::vector<Sym>* buckets[BUCKETS] = {nullptr};

    Sym* getsym(const char *key, bool create=true) {
        size_t h = _hash(key);
        if (lastaccess != nullptr && h == lastaccess->hash) {
            if (!strcmp(key, lastaccess->key)) {
                return lastaccess;
            }
        }
        std::vector<Sym> *bucket = buckets[h % BUCKETS];
        // printf("%llX\n", bucket);
        // printf("Searching %llu items for key %s\n", bucket->length(), key);
        for (size_t i=0; i<bucket->size(); i++) {
            Sym *sym = &bucket->at(i);
            if (h == sym->hash) {
                if (!strcmp(key, sym->key)) {
                    lastaccess = sym;
                    return sym;
                }
            }
        }
		if (create) {
            bucket->push_back(Sym(_dupcstr(key)));
			Sym *sym = &bucket->at(bucket->size()-1);
			len++;
			return sym;
		}
		return nullptr;
    }
    Sym* getsym(size_t i) {
        if (i < len) {
            for (size_t b = 0; b < BUCKETS; b++) {
                size_t l = buckets[b]->size();
                // printf("Bucket %llu length %llu index %llu\n", b, l, i);
                if (i < l) {
                    // printf("Found.\n");
                    return &buckets[b]->at(i);
                }
                i -= l;
            }
            // printf("Not Found.\n");
        }
        return nullptr;
    }

    public:
	/* Construct an empty Dictionary. */
    Dictionary<T, BUCKETS>() {
        clear();
    }
	/* Construct a Dictionary from existing keys and values. */
    Dictionary<T, BUCKETS>(const char** keys, const T* values, size_t count) {
        clear();
        for (size_t i=0; i<count; i++) {
            add(keys[i], values[i]);
        }
    }
	/* Clear the Dictionary, removing all keys and values. */
    void clear() {
        this->len = 0;
        this->lastaccess = nullptr;
        for (size_t i=0; i<BUCKETS; i++) {
            if (buckets[i] != nullptr) {
                delete buckets[i];
            }
            buckets[i] = new std::vector<Sym>();
        }
    }
	/* Return the number of key:value pairs in the Dictionary. */
    inline size_t length() {
        return len;
    }
	/* Returns true if the key is found in the Dictionary. */
    inline bool has(const char *key) {
        return getsym(key, false) != nullptr;
    }
	/* Get/Set a key:value pair in the Dictionary.
	   key:value pair (default constructor for T value) is created if it doesn't exist. */
    inline T& get(const char* key) {
        return getsym(key)->value;
    }
	/* Add a key:value pair to the Dictionary. */
    inline T& add(const char* key, const T value) {
        // printf("Adding key %s new len %llu\n", key, len+1);
        return (get(key) = value);
    }
	/* Add a key:value pair to the Dictionary. */
    inline T& append(const char *key, const T value) {
        return add(key, value);
    }
	/* Return a value from the Dictionary given a key. */
    inline T& operator[](const char *key) {
        return get(key);
    }
	/* Returns true if index is less than the number of key:value pairs.
	   Note that the index of a given key:value pair will likely change when the Dictionary is modified. */
    inline bool has(size_t i) {
        return i < len;
    }
	/* Get/Set key:value pair index in the Dictionary.
	   Note that the index of a given key:value pair will likely change when the Dictionary is modified. */
    inline T get(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return T();
        return sym->value;
    }
	/* Return a value by index.
	   Note that the index of a given key:value pair will likely change when the Dictionary is modified. */
    inline T values(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return T();
        return sym->value;
    }
	/* Return a key by index.
	   Note that the index of a given key:value pair will likely change when the Dictionary is modified. */
    inline char* keys(size_t i) {
        Sym *sym = getsym(i);
        if (sym == nullptr)
            return nullptr;
        return sym->key;
    }

};
