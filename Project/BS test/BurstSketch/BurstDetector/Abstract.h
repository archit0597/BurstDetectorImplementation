#ifndef _ABSTRACT_H_
#define _ABSTRACT_H_

#include <vector>
#include "../Utils/Burst.h"

template<typename UNIQUE_ID_TYPE>
class Abstract {
public:
    Abstract() {}
    virtual ~Abstract() {}

    // Insert a Burst Item with ID and Time window
    virtual void insertID(UNIQUE_ID_TYPE id, uint64_t window) = 0;

    // Query to return the vector of Bursts
    virtual std::vector<Burst<UNIQUE_ID_TYPE>> queryBursts() = 0;
};

#endif