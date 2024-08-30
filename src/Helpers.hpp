#pragma once
#include <cstdarg>
#include <fstream>
void _logprint(int logLevel, const char* text, va_list args);
void logprint(int logLevel, const char* text, ...);
void CloseLog();

void JsonFormatError(const char* f, const char* m, const char* s=nullptr);
void JsonFormatError(const char* f, const char* m, long long i);
void MissingAssetError(const char* f);
void AssetFormatError(const char* p);
size_t fstreamlen(std::ifstream& fd);
size_t fstreamlen(std::fstream& fd);
