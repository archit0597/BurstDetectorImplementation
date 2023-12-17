#ifndef _PARAM_H_
#define _PARAM_H_
#include <cstdint>

// Enumeration of replace strategies
#define FREQUENT_REPLACE     0x0
#define PROBABILITY_DECAY_REPLACE  0x1
#define PROBABILITY_REPLACE 0X2

// Array of replace strategy names
const char* replace_str[] = {
	"frequent", 
	"probabilistic decay", 
	"probabilistic replace"
};

// Bitmasks for cache configurations
#define BURST_SKETCH_FLAG      0x1
#define OPTIMIZE_SKETCH_FLAG    0x2

// Memory configurations
// 2.4117051348181258E-314
const static double stageRatio[] = {
    0.2, 0.3, 0.4, 0.5, 0.6
};

const static double memoryList[] = {
    20, 40, 60, 80, 100
};

// Cache configuration parameters
double stage_ratio = 1.0;
double stage_ratio_optimized = 1.0;

uint32_t windowSize = 40000;
const int windowThreshold = 2;
const int optimized_bit = 32;
const uint64_t windowTime = 8000000000000;

uint64_t BurstThreshold = 50;
int lambda = 2;
double running_track_threshold_ratio = 0.2;
int hash_number = 1;
int stage2_bucket_size = 4;
int replaceStrategy = FREQUENT_REPLACE;
bool count_based = true;
uint64_t start_time;

// Function to initialize parameters to their default values
void ParamInitialize() {
	stage_ratio = 1;
	BurstThreshold = 50;
	stage2_bucket_size = 4;
	running_track_threshold_ratio = 0.2;
	count_based = true;
	replaceStrategy = FREQUENT_REPLACE;
	lambda = 2;
	hash_number = 1;
	stage_ratio_optimized = 1;
}
#endif