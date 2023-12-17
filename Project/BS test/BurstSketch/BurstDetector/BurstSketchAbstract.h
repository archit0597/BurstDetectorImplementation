#ifndef _BURSTSKETCHABSTRACT_H_
#define _BURSTSKETCHABSTRACT_H_

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include "Abstract.h"
#include "Stage.h"

template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class BurstSketchAbstract : public Abstract<UNIQUE_ID_TYPE> {
public:
    BurstSketchAbstract(bool flag = false) : windowCount(0), stage1(nullptr), stage2(nullptr), analysis_flag(flag) {
        if (!count_based) {
            last_timestamp = start_time;
        }
        else {
            last_timestamp = 0;
        }
    }
     
    virtual ~BurstSketchAbstract() {
		if (stage1 != nullptr)
			delete stage1;
		if (stage2 != nullptr)
			delete stage2;
	}

    void insertID(UNIQUE_ID_TYPE id, uint64_t timestamp) {
        VALUE_TYPE count;
        if (count_based) {
            while (last_timestamp + windowSize < timestamp) {
                // transition to next window
                last_timestamp += windowSize;
                stage1->clearAll();
                stage2->window_transition(windowCount++);
            }
        }
        else {
            while (last_timestamp + windowTime < timestamp) {
                // transition to next window
                last_timestamp += windowTime;
                stage1->clearAll();
                stage2->window_transition(windowCount++);
            }
        }
        if (stage2->lookup(id)) {
            return;
        }
        count = stage1->insert(id);
        if (count != 0 && stage2->insert(id, timestamp, count)) {
            stage1->clearID(id);
            if (analysis_flag)
                passed.insert(id);
        }
    }

    std::vector<Burst<UNIQUE_ID_TYPE>> queryBursts() {
        stage1->clearAll();
        stage2->window_transition(windowCount++);
        return stage2->results;
    }

    std::set<UNIQUE_ID_TYPE> get_passed_IDs() {
        return passed;
    }

protected:
    uint64_t last_timestamp;
	uint32_t windowCount;
    RunningTrackStage<UNIQUE_ID_TYPE, VALUE_TYPE> *stage1;
    SnapShotStage<UNIQUE_ID_TYPE, VALUE_TYPE> *stage2;
    std::set<UNIQUE_ID_TYPE> passed;
    bool analysis_flag;
};

#endif
