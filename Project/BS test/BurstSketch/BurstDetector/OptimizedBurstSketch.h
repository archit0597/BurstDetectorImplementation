#ifndef _OPTIMIZEDBURSTSKETCH_H_
#define _OPTIMIZEDBURSTSKETCH_H_

#include <cmath>

#include "BitMap.h"
#include "Param.h"
#include "Stage.h"
#include "BurstSketchAbstract.h"



uint32_t FP1, FP2, CNT1, CNT2;

struct OptimizedArray {
    int count_val = 0;
    BitMap offset_flags;
    uint32_t *parts;
    int length;

    OptimizedArray(int _length) : length(_length), offset_flags(_length) {
        parts = new uint32_t [length];
        memset(parts, count_val, length * sizeof(uint32_t));
    }

    ~OptimizedArray() {
        delete [] parts;
    }

    // Method to clear all data
    void clearAll() {
        memset(parts, count_val, sizeof(uint32_t) * length);
        offset_flags.ClearAllBits();
    }

    //method to handle not matching cases in buckets based on twice the threshold
    uint32_t handleNotMatchingscoreCNT2(int index, uint32_t score, uint32_t fp, uint32_t count) {
        switch (replaceStrategy) {
            case FREQUENT_REPLACE:
                --parts[index];
                break;

            case PROBABILITY_DECAY_REPLACE:
                if (!(rand() % int(pow(1.08, count)))) {
                    --parts[index];
                }
                break;

            case PROBABILITY_REPLACE:
                if (!(rand() % (count + 1))) {
                    parts[index] = score << CNT2;
                    parts[index] += count;
                    if (count != (1 << CNT2) - 1) {
                        parts[index]++;
                        count++;
                    }
                    return count;
                }
                break;
        }
        return 0;
    };
    //method to handle not matching cases in buckets based on optimized bit and threshold
    uint32_t handleNotMatchingscoreCNT1(int index, uint32_t score, uint32_t fp, uint32_t count) {
        if (replaceStrategy == PROBABILITY_DECAY_REPLACE) {
            if (!(rand() % int(pow(1.08, count)))) 
                --parts[index];
        }
        if (replaceStrategy == FREQUENT_REPLACE)
            parts[index]--;
        if (replaceStrategy == PROBABILITY_REPLACE) {
            if (!(rand() % (count + 1))) {
                uint32_t lShiftValCNT1 = (1 << CNT1);
				parts[index] = score << CNT1;
                parts[index] += count;
                if (count == lShiftValCNT1 - 1) {
                    offset_flags.SetBit(index);
                    parts[index] = score << CNT2;
                    parts[index] |= lShiftValCNT1;
                    count = lShiftValCNT1;
                }
                else {
                    parts[index]++;
                    count++;
                }
                return count;
			}
        }
        return 0;
    };
    // Method to insert a Burst item with replacement strategy
    uint32_t insert(uint32_t score, uint32_t index) {
        if (!offset_flags.GetBit(index)) {
            uint32_t val = parts[index];
            uint32_t lShiftValFP1 = (1 << FP1);
            score = score & (lShiftValFP1 - 1);
            uint32_t fp = val >> CNT1;
            uint32_t lShiftValCNT1 = (1 << CNT1);
            uint32_t count = val & (lShiftValCNT1 - 1);
            if (count == 0) { 
                parts[index] = score << CNT1;
                parts[index]++;
                return 1;
            }
            else if ((score & (lShiftValFP1 - 1)) == fp) {
                if (count == lShiftValCNT1 - 1) { 
                    offset_flags.SetBit(index);
                    count = lShiftValCNT1;
                    parts[index] = score << CNT2;
                    parts[index] |= lShiftValCNT1;
                }
                else {
                    parts[index]++;
                    count++;
                }
                return count;
            }
            // not match
            else { 
                return handleNotMatchingscoreCNT1(index, score, fp, count);    
            }
            // if it is empty
        } else { 
            uint32_t lShiftValFP2 = (1 << FP2);
            score = score & (lShiftValFP2 - 1);
            uint32_t fp = parts[index] >> CNT2;
            uint32_t lShiftValCNT2 = (1 << CNT2);
            uint32_t count = parts[index] & (lShiftValCNT2 - 1);

            if (count == 0) {
                parts[index] = score << CNT2;
                parts[index]++;
                return 1;
            }
            //matching score
            else if ((score & (lShiftValFP2 - 1)) == fp) {
                if (count != lShiftValCNT2 - 1) {
                    parts[index]++;
                    count++;
                }
                return count;
            }
            // not match
            else {
                return handleNotMatchingscoreCNT2(index, score, fp, count);
            }
        }     
    }
    // Method to clear the IDs and clear the offset flags
    void ClearID(uint32_t index) {
        parts[index] = 0;
        offset_flags.ClearAllBits();
    }
};

template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class OptimizedStageOne : public RunningTrackStage<UNIQUE_ID_TYPE, VALUE_TYPE> {
public:
	OptimizedStageOne(int num, int length, uint32_t thres) : array_number(num), array_length(length), running_track_thres(thres) {
    int index = 0;
	CNT1 = log2(thres - 1) + 1;
    CNT2 = 2 * CNT1;
    FP2 = optimized_bit - CNT2;
	FP1 = optimized_bit - CNT1;
       	buckets = new OptimizedArray*[array_number];
        while (index < array_number)
        {
            buckets[index] = new OptimizedArray(array_length);
            ++index;
        }
     }
	
	~OptimizedStageOne() {
        int index = 0;
        while(index<array_number){
            delete buckets[index];
            ++index;
        }
        delete [] buckets;   
    }
    // Method to clear all data
    void clearAll() {
        int index = 0;
        while (index<array_number)
        {
            buckets[index]->clearAll();
            ++index;
        }
    }
    // Method to clear the IDs based on hash val
    void clearID(UNIQUE_ID_TYPE id) {
        int index = 0;
        while(index<array_number){
            uint32_t hashVal = Hash(id, index + 20) % array_length;
            buckets[index]->ClearID(hashVal);
            ++index;
        } 
    }
    // Method to insert a Burst item based in burstId and hash val
    VALUE_TYPE insert(UNIQUE_ID_TYPE id) {
        VALUE_TYPE min_val = std::numeric_limits<VALUE_TYPE>::max();
        int index = 0;
        uint32_t val = Hash(id, 33);
        while (index<array_number)
        {
            uint32_t hashIndex = Hash(id, index + 20) % array_length;
            min_val = std::min(min_val, (VALUE_TYPE)buckets[index]->insert(val, hashIndex));
            ++index;
        }
        return min_val >= running_track_thres ? min_val : 0;
    }

private:
	uint32_t running_track_thres;
    OptimizedArray **buckets;
    int array_number;
    int array_length;
};

template<typename UNIQUE_ID_TYPE, typename VALUE_TYPE>
class OptimizedBurstSketch : public BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE> {
public:
    OptimizedBurstSketch(double runningtrack_to_snapshot_ratio, uint32_t running_track_thres, int memory, bool flag=false) : BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>(flag) {
        int stage1_length = (memory * 1024 * runningtrack_to_snapshot_ratio / (1 + runningtrack_to_snapshot_ratio)) / hash_number * 8 / (optimized_bit + 1);
        int stage2_num = (memory * 1024 / (1 + runningtrack_to_snapshot_ratio)) / stage2_bucket_size / (sizeof(int32_t) + sizeof(UNIQUE_ID_TYPE) + 2 * sizeof(VALUE_TYPE));
        BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>::stage2 = new SnapShotStage<UNIQUE_ID_TYPE, VALUE_TYPE>(running_track_thres, BurstThreshold, stage2_num, stage2_bucket_size);
        BurstSketchAbstract<UNIQUE_ID_TYPE, VALUE_TYPE>::stage1 = new OptimizedStageOne<UNIQUE_ID_TYPE, VALUE_TYPE>(hash_number, stage1_length, running_track_thres);  
    }
    ~OptimizedBurstSketch() {}
};
#endif
