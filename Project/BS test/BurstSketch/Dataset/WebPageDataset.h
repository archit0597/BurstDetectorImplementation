#ifndef _WEBPAGEDATASET_H_
#define _WEBPAGEDATASET_H_

#include <cstdint>
#include <string>
#include "../Utils/Mmap.h"

struct WebPage_Tuple {
    uint64_t timestamp;
    uint64_t id;
};

class WebPageDataset {
public:
    WebPageDataset(std::string PATH, std::string name) {
        filename = name;
        load_result = MMap::Load(PATH.c_str());
        dataset = (WebPage_Tuple*)load_result.start;
        length = load_result.length / sizeof(WebPage_Tuple);
    }

    ~WebPageDataset() {
        MMap::UnLoad(load_result);
    }

public:
    std::string filename;
    LoadResult load_result;
    WebPage_Tuple *dataset;
    uint64_t length;
};

#endif