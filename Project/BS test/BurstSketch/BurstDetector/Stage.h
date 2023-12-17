#ifndef _STAGE_H_
#define _STAGE_H_

#include <bitset>
#include <vector>
#include <climits>
#include "../Utils/Burst.h"
#include "Abstract.h"
#include "../Utils/Hash.h"
#include "Param.h"

// Define the RunningTrackStage class
template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class RunningTrackStage {
public:
    RunningTrackStage() {}
    virtual ~RunningTrackStage() {}

    // Clear all elements from the running track cache
    virtual void clearAll() = 0;

    // Clear an element from the running track cache by its ID
    virtual void clearID(UNIQUE_ID_TYPE id) = 0;

    // Insert an element into the running track cache and return its minimal counter value.
    virtual VALUE_TYPE insert(UNIQUE_ID_TYPE id) = 0;
};


// Define the SnapShotStage class
template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class SnapShotStage {
public:
    SnapShotStage(uint32_t s1_thres, uint32_t s2_thres, int b_num, int b_size) : flag(0), stage_one_thres(s1_thres), stage_two_thres(s2_thres), bucketCount(b_num), bucketSize(b_size) {
        slots = new Slot*[bucketCount];

        int index = 0;
        while (index < bucketCount) {
            int j = 0;
            slots[index] = new Slot[bucketSize];
            memset(slots[index], 0, sizeof(Slot) * bucketSize);
            while (j < bucketSize) {
                slots[index][j].window = -1;
                 ++j;
            }
            ++index;
        }
    }

    ~SnapShotStage() {
        int index = 0;
        while (index<bucketCount)
        {
            delete [] slots[index];
            index++;
        }
        delete [] slots;
    }

    // Lookup an element in the stage two cache by its ID
    bool lookup(UNIQUE_ID_TYPE burstId) {
        int bucketPos = Hash(burstId, 32) % bucketCount;
        int index = 0;
        while (index<bucketSize)
        {
            if (slots[bucketPos][index].id == burstId) {
                ++slots[bucketPos][index].counters[flag];
                return true;
            }
            index++;
        }
        return false;
    }

    // Insert an element into the stage two cache and return a boolean value indicating the success of the operation.
    bool insert(UNIQUE_ID_TYPE burstId, int32_t window, VALUE_TYPE count) {
        int bucketPos = Hash(burstId, 32) % bucketCount;
        int slotPos = 0;
        VALUE_TYPE min_counter = std::numeric_limits<VALUE_TYPE>::max();
        bool non_bursting = false;
        // search bucket for a suitable slot
        // id is guaranteed not to be in the stage 2
        int index = 0;
        while (index < bucketSize)
        {
            VALUE_TYPE slots_min_counter = slots[bucketPos][index].counters[flag];
            
            if (slots[bucketPos][index].window == -1) {
                if (!non_bursting) {
                    non_bursting = true;
                    min_counter = std::numeric_limits<VALUE_TYPE>::max();
                }
                if (slots_min_counter < min_counter) {
                    min_counter = slots_min_counter;
                    slotPos = index;
                }
            }
            else if (!non_bursting) {
                if (slots_min_counter < min_counter) {
                    min_counter = slots_min_counter;
                    slotPos = index;
                }
            }
            // empty
            if (slots[bucketPos][index].id == 0) { 
                slotPos = index;
                break;
            }
            index++;
        }

        // Evict empty
        if (slots[bucketPos][slotPos].id == 0 || count > slots[bucketPos][slotPos].counters[flag]) {
            slots[bucketPos][slotPos].counters[flag] = count;
            slots[bucketPos][slotPos].counters[flag ^ 1] = 0;
            slots[bucketPos][slotPos].id = burstId;
            slots[bucketPos][slotPos].window = -1;
            return true;
        }
        
        return false;
    }

    // Transition the stage two cache to a new window and perform the necessary updates.
    void window_transition(uint32_t window) {

        int index = 0;
        while (index < bucketCount) {
            int j = 0;
            while (j < bucketSize) {
                if (slots[index][j].window != -1 && slots[index][j].id != 0 && window - slots[index][j].window > windowThreshold) {
                    slots[index][j].window = -1;
                } else if (slots[index][j].id != 0 && slots[index][j].counters[1] < stage_one_thres && slots[index][j].counters[0] < stage_one_thres) {
                    memset(&slots[index][j], 0, sizeof(Slot));
                    slots[index][j].window = -1;
                }
                ++j;
            }
            ++index;
        }
        
        // find burst
        for (int i = 0; i < bucketCount; ++i) {
            for (int j = 0; j < bucketSize; ++j) {
                if (slots[i][j].id == 0) 
                    continue;
                if (slots[i][j].window != -1 && slots[i][j].counters[flag] <= slots[i][j].counters[flag ^ 1] / lambda) {
                    results.emplace_back(Burst<UNIQUE_ID_TYPE>(slots[i][j].window, window, slots[i][j].id));
                    slots[i][j].window = -1;
                }
                else if (slots[i][j].counters[flag] < stage_two_thres) {
                    slots[i][j].window = -1;
                }
                else if (slots[i][j].counters[flag] >= lambda * slots[i][j].counters[flag ^ 1] && slots[i][j].counters[flag] >= stage_two_thres) {
                    slots[i][j].window = window;
                }
            }
        }

        flag ^= 1;
        int i = 0;
        while (i < bucketCount) {
            int size = 0;
            while (size < bucketSize) {
                slots[i][size].counters[flag] = 0;
                ++size;
            }
            ++i;
        }     
    }

private:
    struct Slot {
        UNIQUE_ID_TYPE id;
        VALUE_TYPE counters[2];
        int32_t window;
    };

    uint32_t stage_one_thres, stage_two_thres;
    int bucketCount, bucketSize;
    uint8_t flag;
    Slot **slots;
    
public:
    std::vector<Burst<UNIQUE_ID_TYPE>> results;
};

#endif