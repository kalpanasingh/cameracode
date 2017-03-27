//
//  logger.h
//  
//
//  Created by kenny young on 2013-05-30.
//  This class manages a logFile which records information regaurding the progress of the run,
//  including any context errors thrown by the libgphoto2 library.
//

#ifndef ____logger__
#define ____logger__
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
using namespace std;

class logger {
public:
    logger();
    logger(string logFileName);
    ~logger();
    struct timeval startTime;
    void write(string line);
    void write(string line,int i);
    void split(int i);
    void combine();
    
private:
    fstream logFile;
    stringstream *subsections;
    int sectionNumber;
    bool hasSplit;
};

#endif
