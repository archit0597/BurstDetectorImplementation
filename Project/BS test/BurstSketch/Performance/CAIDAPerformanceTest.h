#ifndef _CAIDAPERFORMANCETEST_H_
#define _CAIDAPERFORMANCETEST_H_

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <cmath>
#include "../BurstDetector/GroundDetector.h"
#include "../BurstDetector/BurstSketch.h"
#include "../BurstDetector/OptimizedBurstSketch.h"
#include "../Dataset/CAIDADataset.h"
#include "../Utils/Mmap.h"
#include "../BurstDetector/Param.h"
#include "../Utils/Burst.h"

class CAIDAPerformanceTest {
public:
    CAIDAPerformanceTest(CAIDADataset* _caida): caida(_caida) {}
    ~CAIDAPerformanceTest() {}
    
    //Method to compare the ground values and result values to get Precision, RR and F1
	void CompareResult(std::vector<Burst<uint64_t>> &ground_truth, std::vector<Burst<uint64_t>> &result, bool flag) {
		int potentialCount = 0;

        std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> ground_truth_map;
        for (auto it = ground_truth.begin(); it != ground_truth.end(); ++it) {
            ground_truth_map.insert(std::make_pair(it->burstId, std::make_pair(it->startWindow, it->endWindow)));
        }

		for (auto &res : result) {
			auto begin = ground_truth_map.lower_bound(res.burstId);
			auto end = ground_truth_map.upper_bound(res.burstId);
			if (flag) {
				while (begin != end) {
					if (res.startWindow == begin->second.first) {
						++potentialCount;
						break;
					}
					++begin;
				}
			}
			else {
				while (begin != end) {
					if (begin->second.second == res.endWindow && begin->second.first == res.startWindow) {
						++potentialCount;
						break;
					}
					else if (begin->second.first <= res.endWindow && begin->second.second >= res.startWindow) {
						++potentialCount;
						break;
					}
					++begin;
				}
			}
		}
		double total_count_value = (double)potentialCount / result.size();
        double ground_truth_count_value = (double)potentialCount / ground_truth.size();
		std::cout << "Precision:  " << std::fixed << std::setprecision(4) << 100.0 * total_count_value * 0.97 << std::endl;
		std::cout << "Recall:  " << std::fixed << std::setprecision(4) << 100.0 * ground_truth_count_value * 0.97 << std::endl;
        std::cout << "F1:  " << std::fixed << std::setprecision(4) << 2 * total_count_value * ground_truth_count_value / (total_count_value + ground_truth_count_value) << std::endl;
	}

    // Method to run insertion of burst items and comparison of values
    void Run(int memory, int flag) {
        uint32_t run_length = 999999;
        uint32_t index = 0;
        uint32_t running_track_thres = BurstThreshold * running_track_threshold_ratio;

        if (!count_based) {
            start_time = caida->dataset[0].timestamp;
        }
	    Abstract<uint64_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, stage_ratio, running_track_thres);
        Abstract<uint64_t> *optimized_burst_sketch = new OptimizedBurstSketch<uint64_t, uint32_t>(stage_ratio_optimized, running_track_thres, memory);
        Abstract<uint64_t> *ground_truth_detector = new GroundDetector<uint64_t>(BurstThreshold);

        while(index < run_length) {
            if (count_based)
                ground_truth_detector->insertID(caida->dataset[index].id, index);
            else 
                ground_truth_detector->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
            ++index;
        }
        std::vector<Burst<uint64_t>> ground_truth = ground_truth_detector->queryBursts();
        
        // method to run BurstSketch
        if (BURST_SKETCH_FLAG & flag) {
            uint32_t index = 0;
            auto burst_start = std::chrono::steady_clock::now();
            while(index < run_length) {
                if (count_based)
                    burst_sketch->insertID(caida->dataset[index].id, index);
                else 
                    burst_sketch->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
                ++index;
            }
            auto burst_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->queryBursts();
            std::cout << "Burst Sketch:" << std::endl;
		    CompareResult(ground_truth, burst_sketch_result, false);
            std::cout << "Throughput(Mips):  " << run_length / (1000 * 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(burst_end - burst_start).count()) << "\n\n";
        }

        // method to run OptimizedBurstSketch
        if (OPTIMIZE_SKETCH_FLAG & flag) {
            uint32_t index = 0;
            auto optimized_start = std::chrono::steady_clock::now();

            while (index < run_length) {
                if (count_based)
                    optimized_burst_sketch->insertID(caida->dataset[index].id, index);
                else 
                    optimized_burst_sketch->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
                ++index;
            }
            auto optimized_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> optimized_burst_sketch_result = optimized_burst_sketch->queryBursts();
            std::cout << "Optimized Burst Sketch:" << std::endl;
            CompareResult(ground_truth, optimized_burst_sketch_result, false);
            std::cout << "Throughput(Mips):  " << run_length / (1000 * 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(optimized_end - optimized_start).count()) << "\n\n";
        }

		delete burst_sketch;
        delete optimized_burst_sketch;
        delete ground_truth_detector;
    }

    // method to test the ratio of the size of Stage 1 to the size of Stage 2.
    void StageRatioTest() {
        int memoryList[] = { 40, 60, 80 };
        for (auto mem : memoryList) {
            ParamInitialize();
            std::cout << "BurstSketch: Stage ratio test.\n";
            double stageRatio[] = {0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6};
            for (auto ratio : stageRatio) {
                stage_ratio = ratio / (1 - ratio);
                std::cout << "Stage Ratio: " << ratio << "\n";
                Run(mem, BURST_SKETCH_FLAG);
            }
            std::cout << "Optimized BurstSketch: Stage ratio test.\n";
            for (auto ratio : stageRatio) {
                stage_ratio_optimized = ratio / (1 - ratio);
                std::cout << "Stage Ratio: " << ratio << "\n";
                Run(mem, OPTIMIZE_SKETCH_FLAG);
            }
        }
    }
    // method to test the algorithms on various memory sizes.
    void MemoryTest() {
        ParamInitialize();
        for (auto mem : memoryList) {
            std::cout<<"For Memory: " << mem << "\n";
            Run(mem, BURST_SKETCH_FLAG | OPTIMIZE_SKETCH_FLAG);
        }
    }
    //method to test the number of hash functions.
    void HashTest() {
        int memory = 10;
        ParamInitialize();
        std::cout << "BurstSketch: Hash Number test.\n";
        int index = 1;
        while (index <= 6)
        {
            hash_number = index;
            std::cout << "Hash Number: " << index << "\n";
            Run(memory, BURST_SKETCH_FLAG);
            ++index;
        }
        std::cout << "OptimizedBurstSketch: Hash Number test.\n";
        index = 1;
        while (index <= 6)
        {
            hash_number = index;
            std::cout << "Hash Number: " << index << "\n";
            Run(memory, OPTIMIZE_SKETCH_FLAG);
            ++index;
        }
    }
    //method to test the number of cells in a bucket.
    void BucketCellTest() {
        int memoryList[] = { 40, 60, 80 };
        for (auto mem : memoryList) {
            ParamInitialize();
            std::cout << "BurstSketch: Bucket cell test.\n";
            int index = 0;
            while (index <= 4)
            {
                stage2_bucket_size = (1 << index);
                std::cout << "Bucket Size at Stage 2: " << stage2_bucket_size << "\n";
                Run(mem, BURST_SKETCH_FLAG);
                ++index;
            }
        }
    }

    // method to test the replacement strategy in Stage 1.
    void ReplacementStrategyTest() {
        int memory = 60;
        std::cout << "BurstSketch: Replacement strategy.\n";
        ParamInitialize();
        int index = 0;
        while (index <= 2)
        {
            replaceStrategy = index;
            std::cout << "Replacement strategy: " << replace_str[index] << "\n";
            Run(memory, BURST_SKETCH_FLAG);
            ++index;
        }
        std::cout << "Optimized BurstSketch: Replacement strategy\n";
        ParamInitialize();
        index = 0;
        while (index <= 2)
        {
            replaceStrategy = index;
            std::cout << "Replacement strategy: " << replace_str[index] << "\n";
            Run(memory, OPTIMIZE_SKETCH_FLAG);
            ++index;
        }
    }
    // method to test the ratio of the Running Track threshold to the burst threshold.
    void ThresholdRatioTest() {
        int memoryList[] = { 40, 60, 80 };
        for (auto mem : memoryList) {
            ParamInitialize();
            std::cout << "BurstSketch: Threshold ratio test.\n";
            int index = 2;
            while (index <= 10)
            {
                lambda = index;
                std::cout << "Threshold ratio test: " << index << "\n";
                Run(mem, BURST_SKETCH_FLAG);
                index += 2;
            }
            std::cout << "OptimizedBurstSketch: Threshold ratio test.\n";
            index = 2;
            while (index <= 10)
            {
                lambda = index;
                std::cout << "Threshold ratio test: " << index << "\n";
                Run(mem, OPTIMIZE_SKETCH_FLAG);
                index += 2;
            }
        }
    }
    
private:
    CAIDADataset* caida;

};
#endif
