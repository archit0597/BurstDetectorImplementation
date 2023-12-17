#ifndef _BURSTSKETCH_H_
#define _BURSTSKETCH_H_

#include <cstring>
#include "Param.h"
#include "Stage.h"
#include "BurstSketchAbstract.h"
#include "../Utils/Hash.h"

template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class RunningTrack : public RunningTrackStage<UNIQUE_ID_TYPE, VALUE_TYPE> {
public:
	RunningTrack(uint32_t threshold, int number, int length) : stage1_threshold(threshold), array_number(number), array_length(length) {
	    int index = 0, count = 0;
	    int len_id_type = sizeof(UNIQUE_ID_TYPE) * length;
	    int len_data_type = sizeof(VALUE_TYPE) * length;
		counters = new VALUE_TYPE*[array_number];
        burstIds = new UNIQUE_ID_TYPE*[array_number];
        while (index < array_number) {
            burstIds[index] = new UNIQUE_ID_TYPE[length];
            memset(burstIds[index], count, len_id_type);
            counters[index] = new VALUE_TYPE[length];
            memset(counters[index], count, len_data_type);
            ++index;
        }
	}
	
	~RunningTrack() {
		int index = 0;
        while (index < array_number) {
            delete[] burstIds[index];
            delete[] counters[index];
            ++index;
        }
        //Finally delete all arrays
        delete [] counters;
		delete [] burstIds;
	}
		
    // Method to clear all data
	void clearAll() {
		int len_id_type = sizeof(UNIQUE_ID_TYPE) * array_length;
        int len_data_type = sizeof(VALUE_TYPE) * array_length;
        int index = 0, count = 0;
        while (index < array_number) {
            memset(counters[index], count, len_data_type);
            memset(burstIds[index], count, len_id_type);
            ++index;
        }
	}

    // Method to insert a Burst item with replacement strategy
	VALUE_TYPE insert(UNIQUE_ID_TYPE id) {
		VALUE_TYPE insertedCount = 0;
		int index = 0;
        while (index < array_number) {
            uint32_t hashIndex = Hash(id, index + 20) % array_length;
            VALUE_TYPE& currentCounter = counters[index][hashIndex];
            UNIQUE_ID_TYPE& currentBurstId = burstIds[index][hashIndex];
            if (currentBurstId == id) {
                if (++currentCounter > stage1_threshold) {
                    insertedCount = currentCounter;
                }
            } else if (currentCounter == 0) {
                ++currentCounter;
                currentBurstId = id;
            } else {
                if (replaceStrategy == FREQUENT_REPLACE) {
                    --currentCounter;
                }
                if (replaceStrategy == PROBABILITY_DECAY_REPLACE) {
                    // Calculate the probability threshold based on the decay formula
                    double decayProbability = pow(1.08, currentCounter);

                    // Generate a random number and check if it is below the calculated probability threshold
                    int randomValue = rand();
                    if (!(randomValue % int(decayProbability))) {
                        // If the condition is true, decrement the counter
                        --currentCounter;
                    }
                }
                if (replaceStrategy == PROBABILITY_REPLACE) {
                    int randomValue = rand();
                    if (!(randomValue % (currentCounter + 1))) {
                        insertedCount = (++currentCounter);
                        currentBurstId = id;
                    }
                }
            }

            ++index;
        }
		return insertedCount;
	}

    // Method to clear all Burst IDs and counters
	void clearID(UNIQUE_ID_TYPE burst_id) {
		int index = 0;
        while (index < array_number) {
            uint32_t hash_val = Hash(burst_id, index + 20);
            uint32_t hash_index = hash_val % array_length;
            if (burstIds[index][hash_index] == burst_id) {
                counters[index][hash_index] = 0;
                burstIds[index][hash_index] = 0;
            }
            ++index;
        }
	}
			
private:
    UNIQUE_ID_TYPE **burstIds;
    uint32_t stage1_threshold;
    VALUE_TYPE **counters;
    int array_number;
    int array_length;
};


template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class BurstSketch : public BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE> {
public:
	BurstSketch(int memory, double runningtrack_to_snapshot_ratio, uint32_t stage1_threshold, bool flag=false) : BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>(flag) {
		int total_size = sizeof(UNIQUE_ID_TYPE) + sizeof(VALUE_TYPE);
		int data_type_size = sizeof(int32_t) + sizeof (UNIQUE_ID_TYPE) + 2 * sizeof(VALUE_TYPE);
		int length_running_track = (memory * 1024 * runningtrack_to_snapshot_ratio / (1 + runningtrack_to_snapshot_ratio)) / hash_number / total_size;
		int stage2_num = (memory * 1024 / (1 + runningtrack_to_snapshot_ratio)) / stage2_bucket_size / data_type_size;
		BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>::stage2 = new SnapShotStage<UNIQUE_ID_TYPE, VALUE_TYPE>(stage1_threshold, BurstThreshold, stage2_num, stage2_bucket_size);
		BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>::stage1 = new RunningTrack<UNIQUE_ID_TYPE, VALUE_TYPE>(stage1_threshold, hash_number, length_running_track);
	}
	~BurstSketch() {}
};

#endif