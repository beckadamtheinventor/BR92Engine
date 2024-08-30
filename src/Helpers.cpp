
#include "raylib.h"
#include "Helpers.hpp"

std::ofstream __log_fd;
void _logprint(int logLevel, const char* text, va_list args) {
	static std::string logLevels[] = {
		"",
		"TRACE",
		"DEBUG",
		"INFO",
		"WARN",
		"ERROR",
		"FATAL",
		""
	};
	static char buffer[1024];
	int len;
	if (!__log_fd.is_open()) {
		__log_fd.open("debug.log");
	}
	len = snprintf((char*)buffer, sizeof(buffer), "[%s] ", logLevels[logLevel].c_str());
	__log_fd.write(buffer, len);
	// printf("%s", buffer);
	len = vsnprintf((char*)buffer, sizeof(buffer), text, args);
	__log_fd.write(buffer, len);
	__log_fd.put('\n');
	__log_fd.flush();
	// printf("%s\n", buffer);
}

void logprint(int logLevel, const char* text, ...) {
	va_list args;
	va_start(args, text);
	_logprint(logLevel, text, args);
	va_end(args);
}

void CloseLog() {
	__log_fd.close();
}

void JsonFormatError(const char* f, const char* m, const char* s) {
	if (s == nullptr) {
		logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s.\n", f, m);
	} else {
		logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s. (%s)\n", f, m, s);
	}
}

void JsonFormatError(const char* f, const char* m, long long i) {
	logprint(LOG_ERROR, "Json Format Error in file \"%s\":\n\t%s. (%lld)\n", f, m, i);
}


void MissingAssetError(const char* f) {
	logprint(LOG_ERROR, "Missing asset file \"%s\"\n", f);
}

void AssetFormatError(const char* p) {
	logprint(LOG_ERROR, "format error in asset file \"%s\"", p);
}

size_t fstreamlen(std::ifstream& fd) {
	fd.seekg(0, std::ios::end);
	size_t count = fd.tellg();
	fd.seekg(0, std::ios::beg);
	return count;
}
size_t fstreamlen(std::fstream& fd) {
	fd.seekg(0, std::ios::end);
	size_t count = fd.tellg();
	fd.seekg(0, std::ios::beg);
	return count;
}
