#pragma once

#include <string>
#include <fstream>
#include <filesystem>
#include <Arduino.h>
#include <cstdarg>

#ifdef PLATFORM_NATIVE

class NativeFS {
public:
    class File {
    private:
        std::fstream file;
        std::string path;
        bool isWrite;

    public:
        File() : isWrite(false) {}
        File(const std::string& path, const char* mode) : path(path), isWrite(mode[0] == 'w') {
            if (isWrite) {
                file.open(path, std::ios::out | std::ios::binary | std::ios::app);
            } else {
                file.open(path, std::ios::in | std::ios::binary);
            }
        }
        operator bool() const { return file.is_open(); }
        size_t write(const uint8_t* buf, size_t size) {
            if (!isWrite) return 0;
            file.write(reinterpret_cast<const char*>(buf), size);
            return size;
        }
        size_t read(uint8_t* buf, size_t size) {
            if (isWrite) return 0;
            file.read(reinterpret_cast<char*>(buf), size);
            return file.gcount();
        }
        int read() {
            if (isWrite) return -1;
            char c;
            if (file.get(c)) return static_cast<unsigned char>(c);
            return -1;
        }
        void close() { file.close(); }
        size_t size() {
            auto pos = file.tellg();
            file.seekg(0, std::ios::end);
            auto size = file.tellg();
            file.seekg(pos);
            return size;
        }
        bool available() {
            return file && !file.eof();
        }
        void print(const std::string& s) {
            if (isWrite) file << s;
        }
        void print(const char* s) {
            if (isWrite) file << s;
        }
        void print(char c) {
            if (isWrite) file << c;
        }
        void print(int v) {
            if (isWrite) file << v;
        }
        void printf(const char* fmt, ...) {
            if (!isWrite) return;
            char buf[512];
            va_list args;
            va_start(args, fmt);
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            file << buf;
        }
    };

    bool begin() {
        std::filesystem::create_directories("/tmp/meshcore");
        return true;
    }
    bool exists(const char* path) { return std::filesystem::exists(path); }
    bool mkdir(const char* path) { return std::filesystem::create_directories(path); }
    bool remove(const char* path) { return std::filesystem::remove(path); }
    File open(const char* path, const char* mode = "r", bool create = false) { return File(path, mode); }
};

#define FILESYSTEM NativeFS 

#endif 