/* Simple Registry class. (integer and string keyed dictionary)
 * Author: Adam "beckadamtheinventor" Beckingham
 * License: MIT
 */
#pragma once

#include <cstdio>
#include <exception>
#include <map>
#include <string>
#include <vector>

template<class T>
class Registry {
    protected:
    std::vector<T*> _entries;
    std::map<std::string, size_t> _dict;
    size_t nextid() {
        return _entries.size();
    }
    public:
    /* Clear the registry.
     * Note that all values must be allocated with the "new" operator otherwise this will not work expectedly.
     */
    void clear() {
        _dict.clear();
        for (size_t i=0; i<_entries.size(); i++) {
            delete _entries[i];
        }
    }
    /* Get the number of registered entries. */
    size_t length() {
        return _entries.size();
    }
    /* Add a new key:value pair to the registry, returning a pointer to it.
     * Note that the value should be allocated with the "new" operator.
     */
     T* add(std::string key, T* v=nullptr) {
        return _add(key, v);
    }
    /* Use this in place of add when you need to override it eg for types needing specific initialization */
    T* _add(std::string key, T* v=nullptr) {
        size_t id = nextid();
        if (v == nullptr) {
            v = new T();
        }
        _entries.push_back(v);
        _dict.insert(std::make_pair(key, id));
        return v;
    }
    /* Create a new key:empty pair in the registry, returning a pointer to it. */
    T* create(std::string key) {
        return add(key, new T());
    }
    /* Check if the registry contains a given key. */
	bool has(std::string key) {
		return _dict.count(key) > 0;
	}
    /* Get a registry entry from a given key. */
	T& get(std::string key) {
		if (has(key)) {
			return _entries[_dict[key]];
		}
        printf("Registry key \"%s\" undefined.\n", key);
		throw std::exception();
	}
    /* Check if the registry contains an entry of a given integer id. */
	bool has(size_t id) {
		return id < _entries.size();
	}
    /* Get a registry entry given an integer id. */
	T* get(size_t id) {
		if (has(id)) {
			return _entries[id];
		}
        printf("Registry ID %llu out of range.\n", id);
		throw std::exception();
	}
    T* of(const char* key) {
		if (has(key)) {
			return _entries[_dict[key]];
		}
        return nullptr;
    }
    T* of(size_t id) {
        return get(id);
    }
};
