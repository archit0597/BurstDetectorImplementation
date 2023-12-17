#ifndef MMAP_H
#define MMAP_H

#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>

struct LoadResult{
    void* start;
    uint64_t length;
};

class MMap {
public:
    static LoadResult Load(const char* PATH);
    static void UnLoad(LoadResult result);

private:
    static const int32_t DEFAULT_OPEN_FLAGS = O_RDONLY;
    static const int32_t DEFAULT_MMAP_PROT = PROT_READ;
    static const int32_t DEFAULT_MMAP_FLAGS = MAP_PRIVATE;
    static const size_t DEFAULT_MMAP_OFFSET = 0;
};

LoadResult MMap::Load(const char* PATH){
    LoadResult ret;

    int32_t fd = open(PATH, DEFAULT_OPEN_FLAGS);
    if(fd == -1) {
        std::cerr << "Cannot open " << PATH << std::endl;
        throw;
    }

    struct stat sb;
    if(fstat(fd, &sb) == -1){
        std::cerr << "Fstat Error" << std::endl;
        throw;
    }

    ret.length = sb.st_size;
    ret.start = mmap(nullptr, ret.length, DEFAULT_MMAP_PROT, DEFAULT_MMAP_FLAGS, fd, DEFAULT_MMAP_OFFSET);

    if (ret.start == MAP_FAILED) {
        std::cerr << "Cannot mmap " << PATH << " of length " << ret.length << std::endl;
        throw;
    }

    return ret;
}

void MMap::UnLoad(LoadResult result){
    munmap(result.start, result.length);
}

#endif