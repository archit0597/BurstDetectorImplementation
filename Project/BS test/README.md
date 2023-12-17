# BurstSketch
## How to run the source code
1- Dowload the CAIDA/Webpage dataset and change the path of the dataset in the main.cc.
2- Open the terminal and navigate to the BurstSketch folder.
3- Compile the code using the command:
   g++ -std=c++11 -o main main.cc
4- Once the build is successful, run the command to obtain the results
    ./main [datasetChoice] [testChoice]
    datasetChoice can be 1 or 2 (1 for CAIDA and 2 for WebPage)
    testChoice can be 1, 2, 3 or 4 based on the test you want to run.
    
    Example: ./main 1 1 
    The above command runs the code to obtain the memory test results on CAIDA the dataset.


©AAP
