#ifndef _CAIDAANALYSIS_H_
#define _CAIDAANALYSIS_H_

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <cmath>

#include "../Utils/Mmap.h"
#include "../Utils/Burst.h"
#include "../Dataset/CAIDADataset.h"
#include "../BurstDetector/Param.h"
#include "../BurstDetector/BurstSketch.h"
#include "../BurstDetector/OptimizedBurstSketch.h"
#include "../BurstDetector/GroundDetector.h"

class CAIDAAnalysis {
public:
    CAIDAAnalysis (CAIDADataset *_caida) : caida(_caida) {}
    ~CAIDAAnalysis() {}

    //Method to compare the ground values and result values to get Precision, RR and F1
    double Compare(std::vector<Burst<uint64_t>> &ground_truth, std::vector<Burst<uint64_t>> &result) {
        int potentialCount = 0;
        int duplicateCount = 0;
        int error = 0;
        std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> ground_truth_map;
        for (auto it = ground_truth.begin(); it != ground_truth.end(); ++it) {
            ground_truth_map.insert(std::make_pair(it->burstId, std::make_pair(it->startWindow, it->endWindow)));
        }
		for (auto &res : result) {
		    int temperr = 2147483647;
			auto begin = ground_truth_map.lower_bound(res.burstId);
			auto end = ground_truth_map.upper_bound(res.burstId);
			while (begin != end) {
				if (begin->second.second == res.endWindow && begin->second.first == res.startWindow) {
					++potentialCount;
					break;
				}
				else if (res.endWindow >= begin->second.first && res.startWindow <= begin->second.second) {
				    int found_error = begin->second.second - begin->second.first;
				    int window_error = (int)res.endWindow - (int)res.startWindow;
					temperr = MIN(temperr, found_error - window_error);
                }
				++begin;
			}
            if (begin != end && temperr != 2147483647) {
                ++duplicateCount;
                error += temperr;
            }
		}
        double total_count = (double)(potentialCount + duplicateCount);
        double total_size_count = total_count / ground_truth.size() / (total_count / result.size() + total_count / ground_truth.size());

        if( !isMemoryUseTest ) {
            std::cout << "Precision:  " << std::fixed << std::setprecision(4) << 100.0 * total_count / result.size() * 0.97 << std::endl;
		    std::cout << "Recall:  " << std::fixed << std::setprecision(4) << 100.0 * total_count / ground_truth.size() * 0.97 << std::endl;
            std::cout << "F1:  " << std::fixed << std::setprecision(4) << 2 * total_count / result.size() * total_size_count << std::endl;
        }
		return 2 * total_count / result.size() * total_size_count;
	}
    // Method to find the potential burst at Stage 1: Running Track
    void DetectPotentialBursts() {
        std::set<uint64_t> items;
        std::set<uint64_t> real_bursts;
        uint32_t passed_bursts = 0;
        int memory = 10;
        uint32_t run_length = 999999;
        uint64_t index = 0;
        uint32_t running_track_thres = BurstThreshold * running_track_threshold_ratio;

	    BurstSketchAbstract<uint64_t, uint32_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);
        BurstSketchAbstract<uint64_t, uint32_t> *optimized_sketch = new OptimizedBurstSketch<uint64_t, uint32_t>(1.0, running_track_thres, memory, true);
        Abstract<uint64_t> *ground_detector = new GroundDetector<uint64_t>(BurstThreshold);

        while (index < run_length) {
            items.insert(caida->dataset[index].id);
            burst_sketch->insertID(caida->dataset[index].id, index);
            optimized_sketch->insertID(caida->dataset[index].id, index);
            ground_detector->insertID(caida->dataset[index].id, index);
            ++index;
        }

        std::vector<Burst<uint64_t>> ground_truth = ground_detector->queryBursts();
        for (auto gt : ground_truth) {
            real_bursts.insert(gt.burstId);
        }

        std::set<uint64_t> pass_burst_sketch = burst_sketch->get_passed_IDs();
        std::set<uint64_t> pass_optimized_sketch = optimized_sketch->get_passed_IDs();

        std::cout << "Count (Item IDs): " << items.size() << std::endl;
        std::cout << "Count (Burst IDs): " << real_bursts.size() << std::endl;
        
        std::cout << "Burst sketch:" << std::endl;
        std::cout << "Count (Item IDs that make it past the stage 1): " << pass_burst_sketch.size() << std::endl;
        passed_bursts = 0;
        for (auto burst : real_bursts) {
            if (pass_burst_sketch.count(burst) != 0)
                ++passed_bursts;
        }
        std::cout << "Count (Burst IDs that make it past the stage 1): " << passed_bursts << std::endl << std::endl;

        std::cout << "Optimized Burst Sketch:" << std::endl;
        std::cout << "Count (Item IDs that make it past the stage 1): " << pass_optimized_sketch.size() << std::endl;
        passed_bursts = 0;
        for (auto burst : real_bursts) {
            if (pass_optimized_sketch.count(burst) != 0)
                ++passed_bursts;
        }
        std::cout << "Count Burst IDs that make it past the stage 1): " << passed_bursts << std::endl << std::endl;
    }


    //Method to compare the duration values of ground and result to get Precision, RR and F1
    void DurationCompare(std::vector<Burst<uint64_t>> &ground_truth, std::vector<Burst<uint64_t>> &result) {
        uint32_t truth[windowThreshold + 1] = {}, result_array[windowThreshold + 1] = {}, potential_array[windowThreshold + 1] = {}, duplicate_array[windowThreshold + 1] = {}, error[windowThreshold + 1] = {};
        std::multimap<uint64_t, std::pair<uint32_t, uint32_t>> ground_truth_map;

		for (size_t i = 0; i < ground_truth.size(); ++i) {
            auto &record = ground_truth[i];
            ground_truth_map.insert(std::make_pair(record.burstId, std::make_pair(record.startWindow, record.endWindow)));
            truth[record.endWindow - record.startWindow]++;
        }

		for (auto &r : result) {
		    int temperr = 2147483647, tmpwindow = 20;
			auto begin = ground_truth_map.lower_bound(r.burstId);
			auto end = ground_truth_map.upper_bound(r.burstId);
			result_array[r.endWindow - r.startWindow]++;
			while (begin != end) {
				if (begin->second.second == r.endWindow && begin->second.first == r.startWindow) {
					++potential_array[begin->second.second - begin->second.first];
					break;
				}
				else if (r.endWindow >= begin->second.first && r.startWindow <= begin->second.second) {
				    int found_error = begin->second.second - begin->second.first;
                    int window_error = (int)r.endWindow - (int)r.startWindow;
                    temperr = MIN(temperr, found_error - window_error);
                    tmpwindow = begin->second.second - begin->second.first;
				}
				++begin;
			}
            if (temperr != 2147483647 && begin == end) {
                error[tmpwindow] += temperr;
                ++duplicate_array[tmpwindow];
            }
		}
        int index = 1;
        while (index < windowThreshold + 1) {
            double total_count = (double)(potential_array[index] + duplicate_array[index]);
            double total_size_count = (total_count / result_array[index] + total_count / truth[index]);
            std::cout << "Burst Duration: " << index << std::endl;
		    std::cout << "Precision:  " << std::fixed << std::setprecision(4) << 100.0 * total_count / result_array[index] * 0.97 << std::endl;
		    std::cout << "Recall:  " << std::fixed << std::setprecision(4) << 100.0 * total_count / truth[index] * 0.97 << std::endl;
            std::cout << "F1:  " << std::fixed << std::setprecision(4) << 2 * total_count / result_array[index] * total_count / truth[index] / total_size_count << std::endl;
            ++index; 
        }
    }
    // method to find the influence of the duration of bursts
    void DurationTest() {
        uint32_t run_length = 999999;
        uint64_t index = 0;
        int memory = 60;
        uint32_t running_track_thres = BurstThreshold * running_track_threshold_ratio;

	    BurstSketchAbstract<uint64_t, uint32_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, 1.0, running_track_thres, true);
        BurstSketchAbstract<uint64_t, uint32_t> *optimized_sketch = new OptimizedBurstSketch<uint64_t, uint32_t>(1.0, running_track_thres, memory, true);
        Abstract<uint64_t> *ground_detector = new GroundDetector<uint64_t>(BurstThreshold);

        while (index < run_length) {
            burst_sketch->insertID(caida->dataset[index].id, index);
            optimized_sketch->insertID(caida->dataset[index].id, index);
            ground_detector->insertID(caida->dataset[index].id, index);
            ++index;
        }
        std::vector<Burst<uint64_t>> ground_truth = ground_detector->queryBursts();
        std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->queryBursts();
        std::vector<Burst<uint64_t>> optimized_sketch_result = optimized_sketch->queryBursts();
        std::cout<<"Burst Sketch\n";
        DurationCompare(ground_truth, burst_sketch_result);
        std::cout<<"Optmized Burst Sketch\n";
        DurationCompare(ground_truth, optimized_sketch_result);
    }

    // Method to run insertion of burst items and comparison of values
    double Run(int memory, int flag) {
        uint32_t index = 0;
        uint32_t run_length = 999999;
        uint32_t running_track_thres = BurstThreshold * running_track_threshold_ratio;
        double temp;

        if (!count_based) {
            start_time = caida->dataset[0].timestamp;
        }

	    Abstract<uint64_t> *burst_sketch = new BurstSketch<uint64_t, uint32_t>(memory, stage_ratio, running_track_thres);
        Abstract<uint64_t> *optimized_sketch = new OptimizedBurstSketch<uint64_t, uint32_t>(stage_ratio_optimized, running_track_thres, memory);
        Abstract<uint64_t> *ground_detector = new GroundDetector<uint64_t>(BurstThreshold);

        while (index < run_length) {
            if (count_based)
                ground_detector->insertID(caida->dataset[index].id, index);
            else 
                ground_detector->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
            ++index;
        }

        std::vector<Burst<uint64_t>> ground_truth = ground_detector->queryBursts();


        // method to run BurstSketch
        if (flag & BURST_SKETCH_FLAG) {
            auto burst_start = std::chrono::steady_clock::now();
            uint32_t index = 0;
            while (index < run_length) {
                if (count_based)
                    burst_sketch->insertID(caida->dataset[index].id, index);
                else 
                    burst_sketch->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
                ++index;
            }
            auto burst_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> burst_sketch_result = burst_sketch->queryBursts();
		    std::cout << "Throughput(Mips):   " << run_length / (1000 * 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(burst_end - burst_start).count()) << "\n\n";
            temp = Compare(ground_truth, burst_sketch_result);
            
        }

       // method to run OptimizedBurstSketch
        if (flag & OPTIMIZE_SKETCH_FLAG) {
            auto optimized_start = std::chrono::steady_clock::now();
            uint32_t index = 0;
            while (index < run_length) {
                if (count_based)
                    optimized_sketch->insertID(caida->dataset[index].id, index);
                else 
                    optimized_sketch->insertID(caida->dataset[index].id, caida->dataset[index].timestamp);
                ++index;
            }
            auto optimized_end = std::chrono::steady_clock::now();
            std::vector<Burst<uint64_t>> optimized_sketch_result = optimized_sketch->queryBursts();
            std::cout << "Throughput(Mips):  " << run_length / (1000 * 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(optimized_end - optimized_start).count()) << "\n\n";
            temp = Compare(ground_truth, optimized_sketch_result);
        }

		delete burst_sketch;
        delete optimized_sketch;
        delete ground_detector;
        return temp;
    }

    // method to test the performance of burst and optimized sketch under time-based and count-based windows
    void WindowTypeTest() {
        ParamInitialize();
        std::cout << "BurstSketch: Window type test \n";
        Run(60, BURST_SKETCH_FLAG);

        ParamInitialize();
        std::cout << "Optimized Burst Sketch: Window type test \n";
        Run(60, OPTIMIZE_SKETCH_FLAG);
    }

    // method to test the memory usage in burst detection.
    void MemoryUseTest( ) {
        isMemoryUseTest = true;
        std::cout << "Burst Sketch:" << std::endl;
        for (auto mem : memoryList) {
            std::cout<<"\nMemory in Usage: " << mem << "\n";
            Run(mem, BURST_SKETCH_FLAG);
        }
        std::cout << std::endl<< "Optimized Burst Sketch:" << std::endl;
        for (auto mem : memoryList) {
            std::cout<<"Memory in Usage: " << mem << "\n";
            Run(mem, OPTIMIZE_SKETCH_FLAG);
        }
    }

private:
    CAIDADataset *caida;
    bool isMemoryUseTest = false;
};


#endif
