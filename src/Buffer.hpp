/* Simple Read/Write Buffer classes
* Author: Adam "beckadamtheinventor" Beckingham
* License: MIT
 */
#pragma once

#include <fstream>
#include <ios>
#include <istream>
#include <ostream>
class RWBuffer {
    protected:
    unsigned char* _data;
    size_t _len;
    size_t _offset;
    public:
    RWBuffer() : RWBuffer(0) {}
    void open(std::istream& fd) {
        fd.seekg(0, std::ios::end);
        size_t count = fd.tellg();
        fd.seekg(0, std::ios::beg);
        resize(count);
        fd.read((char*)_data, count);
    }
    void open(const char* f) {
        std::ifstream fd(f);
        RWBuffer buf;
        if (fd.is_open()) {
            open(fd);
        }
    }
    inline RWBuffer(size_t len, size_t offset=0) {
		if (len == 0) {
			_data = nullptr;
		} else {
			_data = new unsigned char [len];
		}
        _len = len;
        _offset = offset;
    }
    inline RWBuffer(unsigned char* ptr, size_t len, size_t offset=0) {
        _data = ptr;
        _len = len;
        _offset = offset;
    }
    void flush(std::ostream& fd) {
        fd.write((char*)_data, _len);
    }
    void flush(const char* f) {
        std::ofstream fd(f);
        flush(fd);
        fd.close();
    }
    inline bool eof() {
        return _offset >= _len || _data == nullptr;
    }
    inline size_t length() {
        return _data==nullptr ? 0 : _len;
    }
    inline size_t available() {
        return _data==nullptr ? 0 : _len - _offset;
    }
    void resize(size_t size) {
        if (_len != size) {
            unsigned char* newdata = new unsigned char [size];
            if (_data != nullptr) {
                for (size_t i=0; i<size; i++) {
                    if (i >= _len) {
                        break;
                    }
                    newdata[i] = _data[i];
                }
                delete _data;
            }
            _data = newdata;
            _len = size;
        }
    }
    inline bool readable() {
        return _data != nullptr;
    }
    inline bool writeable() {
        return _data != nullptr;
    }
    inline void rewind() {
        _offset = 0;
    }
	inline size_t tell() {
		return this->_offset;
	}
	inline void seek(size_t offset) {
		this->_offset = offset;
	}
	inline void seek(size_t offset, int dir) {
		if (dir == 0) {
			this->_offset = offset;
		} else if (dir == 2) {
			this->_offset = _len > offset ? 0 : _len - offset;
		} else if (dir == 1) {
			this->_offset += offset;
			if (this->_offset >= _len) {
				this->_offset = _len;
			}
		}
	}
    inline unsigned char read() {
        if (_data != nullptr && _offset < _len) {
            return _data[_offset++];
        }
        return 0;
    }
    inline bool read(unsigned char & v) {
        if (_offset < _len) {
            v = _data[_offset++];
            return true;
        }
        return false;
    }
    size_t read(unsigned char* v, size_t amount) {
        if (_data == nullptr) {
			return 0;
		}
		if (_offset + amount >= _len) {
            amount = _len - _offset;
        }
        for (size_t i=0; i<amount; i++) {
            v[i] = _data[_offset++];
        }
        return amount;
    }
    template<class V>
    inline bool readV(V* val) {
        if (available() < sizeof(V)) {
            return false;
        }
        read((unsigned char *)val, sizeof(V));
        return true;
    }

    inline bool write(unsigned char  v) {
        if (_data == nullptr) {
			return false;
		}
        if (_offset + 1 < _len) {
            _data[_offset++] = v;
            return true;
        }
        return false;
    }
    inline size_t write(unsigned char* v, size_t amount) {
        if (_data == nullptr) {
			return 0;
		}
        if (_offset + amount >= _len) {
            amount = _len - _offset;
        }
        for (size_t i=0; i<amount; i++) {
            _data[_offset++] = v[i];
        }
        return amount;
    }
    template<class V>
    bool writeV(V* val) {
        write(val, sizeof(V));
        return true;
    }
};

class RBuffer : public RWBuffer {
    inline bool write(unsigned char  v) {}
    inline size_t write(unsigned char* v, size_t amount) {}
    void flush(std::ostream& fd) {}
    void flush(const char* f) {}
    inline bool writeable() {
        return false;
    }
};

class WBuffer : public RWBuffer {
    inline unsigned char  read() {}
    inline bool read(unsigned char & v) {}
    inline size_t read(unsigned char* v, size_t amount) {}
    inline bool readable() {
        return false;
    }
};
