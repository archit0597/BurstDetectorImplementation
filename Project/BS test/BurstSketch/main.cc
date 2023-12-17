#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "Performance/CAIDAPerformanceTest.h"
#include "Performance/WebpagePerformaceTest.h"
#include "Analysis/CAIDAAnalysis.h"
#include "Analysis/WebPageAnalysis.h"

using namespace std;

const char *caida_file_path = "/Users/voldemort/Library/CloudStorage/OneDrive-FloridaStateUniversity/FSU/Fall23/DBMS/Project/Project/Datasets/equinix-chicago.dirA.20160317-130400.UTC.anon.times";
const char *web_file_path = "/Users/voldemort/Library/CloudStorage/OneDrive-FloridaStateUniversity/FSU/Fall23/DBMS/Project/Project/Datasets/WebPage.times";

void MemoryTest(int choice) {
    if(choice==1) {
        CAIDADataset caida(caida_file_path, "caida");
        CAIDAPerformanceTest performanceTest(&caida);
        // method to test the algorithms on various memory sizes.
        performanceTest.MemoryTest();
    }
    else {
        WebPageDataset webpage(web_file_path, "webpage");
        WebpagePerformanceTest performanceTest(&webpage);
        // method to test the algorithms on various memory sizes.
        performanceTest.MemoryTest();
    } 
}

void ParameterTest(int choice) {
    if (choice == 1) {
        CAIDADataset caida(caida_file_path, "caida");
        CAIDAPerformanceTest performanceTest(&caida);
        //method to test the number of hash functions.
        performanceTest.HashTest();
        // method to test the ratio of the size of Stage 1 to the size of Stage 2.
        performanceTest.StageRatioTest();
        // method to test the ratio of the Running Track threshold to the burst threshold.
        performanceTest.ThresholdRatioTest();
        //method to test the number of cells in a bucket.
        performanceTest.BucketCellTest();
        // method to test the replacement strategy in Stage 1.
        performanceTest.ReplacementStrategyTest();
    }
    else {
        WebPageDataset webpage(web_file_path, "webpage");
        WebpagePerformanceTest performanceTest(&webpage);
        //method to test the number of hash functions.
        performanceTest.HashTest();
        // method to test the ratio of the size of Stage 1 to the size of Stage 2.
        performanceTest.StageRatioTest();
        // method to test the ratio of the Running Track threshold to the burst threshold.
        performanceTest.ThresholdRatioTest();
        //method to test the number of cells in a bucket.
        performanceTest.BucketCellTest();
        // method to test the replacement strategy in Stage 1.
        performanceTest.ReplacementStrategyTest();
    }
}

void AnalysisTest(int choice) {
    if(choice == 1) {
        CAIDADataset caida(caida_file_path, "caida");
        CAIDAAnalysis analysis(&caida);
        // method to test the performance of burst and optimized sketch under time-based and count-based windows
        analysis.WindowTypeTest();
        // method to find the potential burst at Stage 1: Running Track
        analysis.DetectPotentialBursts();
        // method to test the memory usage in burst detection.
        analysis.MemoryUseTest();
    }
    else {
        WebPageDataset webpage(web_file_path, "webpage");
        WebpageAnalysis analysis(&webpage);
        // method to test the performance of burst and optimized sketch under time-based and count-based windows
        analysis.WindowTypeTest();
        // method to find the potential burst at Stage 1: Running Track
        analysis.DetectPotentialBursts();
        // method to test the memory usage in burst detection.
        analysis.MemoryUseTest();
    }
}

void BurstDurationTest(int choice) {
    if(choice == 1){
        CAIDADataset caida(caida_file_path, "caida");
        CAIDAAnalysis analysis(&caida);
        // method to find the influence of the duration of bursts
        analysis.DurationTest();
    }
    else{
        WebPageDataset webpage(web_file_path, "webpage");
        WebpageAnalysis analysis(&webpage);
        // method to find the influence of the duration of bursts
        analysis.DurationTest();
    }
}

int main(int argc, char *argv[]) {
    int datasetChoice = std::stoi(argv[1]);
    int testChoice = std::stoi(argv[2]);
    if (datasetChoice == 1) {
        switch (testChoice) {
            case 1:
                std::cout<<"Starting Memory Test.....\n\n";
                MemoryTest(1);
                break;
            case 2:
                std::cout<<"Starting Parameter Settings Experiment.....\n\n";
                ParameterTest(1);
                break;
            case 3:
                std::cout<<"Starting Analysis test.....\n\n";
                AnalysisTest(1);
                break;
            case 4:    
                std::cout<<"Starting Duration test.....\n\n";
                BurstDurationTest(1);
                break;
            default:
                std::cout<<"Wrong option selected....exiting now....";
                break;
        }
    }
    else if (datasetChoice == 2){
        switch (testChoice) {
            case 1:
                std::cout<<"Starting Memory Test.....\n\n";
                MemoryTest(2);
                break;
            case 2:
                std::cout<<"Starting Parameter Settings Experiment.....\n\n";
                ParameterTest(2);
                break;
            case 3:
                std::cout<<"Starting Analysis test.....\n\n";
                AnalysisTest(2);
                break;
            case 4:    
                std::cout<<"Starting Duration test.....\n\n";
                BurstDurationTest(2);
                break;
            default:
                std::cout<<"Wrong option selected....exiting now....";
                break;
        }
    }
    else{
        std::cout<<"Wrong option selected....exiting now....";
    }
    
    return 0;
}