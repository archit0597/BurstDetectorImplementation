#ifndef _GROUNDDETECTOR_H_
#define _GROUNDDETECTOR_H_

#include <vector>
#include <map>
#include "../Utils/Burst.h"
#include "../BurstDetector/Abstract.h"
#include "../BurstDetector/Param.h"


template<typename UNIQUE_ID_TYPE>

struct DetectGrItem {
    
    int32_t window;
    uint32_t ground_counters[2];
    UNIQUE_ID_TYPE ground_id;

    DetectGrItem() {}
    DetectGrItem(UNIQUE_ID_TYPE ground_id) : ground_id(ground_id) {
        window = -1;
        ground_counters[0] = ground_counters[1] = 0;
    }

    DetectGrItem(const DetectGrItem &item) {
        ground_id = item.ground_id;
        window = item.window;
        ground_counters[0] = item.ground_counters[0];
        ground_counters[1] = item.ground_counters[1];
    }
};

template<typename UNIQUE_ID_TYPE>
class GroundDetector : public Abstract<UNIQUE_ID_TYPE> {

public:
    GroundDetector(uint32_t thres) : ltimestamp(0), CountWindow(0), threshold(thres), flag(0) {
        if (!count_based) {
            ltimestamp = start_time;
        }
    }

	~GroundDetector() {}

    //Method to transition the values from Stage1 to Stage2 to detect potential bursts
    void groundTransition(uint32_t window) {
        for (size_t i = 0; i < ground_truth_detect.size(); ++i) {
            auto &index = ground_truth_detect[i];
            int currentWindow = index.window;
            uint32_t counter_flag = index.ground_counters[flag];
            if (currentWindow != -1 && window - currentWindow > windowThreshold) {
                currentWindow = -1;
            }
            uint32_t lambda_value = counter_flag * lambda;
            if (currentWindow != -1  && lambda_value <= index.ground_counters[flag ^ 1]) {
                Groundoutput.emplace_back(Burst<UNIQUE_ID_TYPE>(currentWindow, window, index.ground_id));
                currentWindow = -1;
            }
            uint32_t lambda_value_flag = lambda * index.ground_counters[flag ^ 1];
            if (counter_flag >= threshold && counter_flag >= lambda_value_flag) {
                currentWindow = window;
            }
            if (counter_flag < threshold) {
                currentWindow = -1;
            }
            index.window = currentWindow;
        }

        flag ^= 1;

        for (auto &index : ground_truth_detect) {
            index.ground_counters[flag] = 0;
        }
    }

    // Method to insert a Burst Item and do transition
    void insertID(UNIQUE_ID_TYPE ground_id, uint64_t timestamp) {

        if (count_based) {
            while (ltimestamp + windowSize < timestamp) {
                groundTransition(CountWindow++);
                ltimestamp += windowSize;
            }
        }
        else {
           while (ltimestamp + windowTime < timestamp) {
                groundTransition(CountWindow++);
                ltimestamp += windowTime;
            }
        }

        if (indexid.end() == indexid.find(ground_id)) {
            indexid[ground_id] = ground_truth_detect.size();
            ground_truth_detect.emplace_back(DetectGrItem<UNIQUE_ID_TYPE>(ground_id));
        }
        ground_truth_detect[indexid[ground_id]].ground_counters[flag]++;
    }

    std::vector<Burst<UNIQUE_ID_TYPE>> queryBursts() {
        groundTransition(CountWindow++);
        return Groundoutput;
    }

private:
    uint32_t threshold;
    uint32_t CountWindow;
    uint8_t flag;
    uint64_t ltimestamp;
    std::vector<DetectGrItem<UNIQUE_ID_TYPE>> ground_truth_detect;
    std::map<UNIQUE_ID_TYPE, int> indexid;

public:
    std::vector<Burst<UNIQUE_ID_TYPE>> Groundoutput;
};

#endif
