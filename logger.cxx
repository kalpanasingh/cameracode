//
//  logger.cxx
//  
//
//  Created by kenny young on 2013-05-30.
//  This class manages a logFile which records information regaurding the progress of the run,
//  including any context errors thrown by the libgphoto2 library.
//  modified by Kalpana Singh  2013-08-25

#include "logger.h"
#include "funcs.h"
#include <iostream>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <fstream>

//create a logger object for a file with a given name
logger::logger(string logFileName){
    subsections=NULL;
    sectionNumber=0;
    hasSplit=false;
    logFile.open(logFileName.data(),std::fstream::in | std::fstream::out | std::fstream::app);
    gettimeofday(&startTime,NULL);
    write("Start time for this run: "+string(ctime(&(startTime.tv_sec))));
}

//create a logger object with a unique identifier logfile_n
logger::logger(){
    int value=0;
    string s="logfile_"+convertIntToString(value)+".log";
    while((access( s.data(), F_OK ))){
    value++;
    s="logfile_"+convertIntToString(value)+".log";
    }
    logger(s.data());
}

//write a string to the logfile with a relative timestamp
void logger::write(string line){
    struct timeval tv;
	long sec, usec;
	gettimeofday (&tv,NULL);
	sec = tv.tv_sec  - startTime.tv_sec;
	usec = tv.tv_usec - startTime.tv_usec;
    if (usec < 0) {sec--; usec += 1000000L;}
    logFile<<sec<<"."<<setfill('0')<<setw(6)<<usec<<":"<<line<<endl;
}

//write a string to subsection i of the logfile with a relative timestamp
void logger::write(string line,int i){
    if(!hasSplit){
     write(line);   
    }
    else{
        struct timeval tv;
        long sec, usec;
        gettimeofday (&tv,NULL);
        sec = tv.tv_sec  - startTime.tv_sec;
        usec = tv.tv_usec - startTime.tv_usec;
        if (usec < 0) {sec--; usec += 1000000L;}
        subsections[i]<<sec<<"."<<std::setfill('0')<<setw(6)<<usec<<":"<<line<<endl;
    }
}

//create i subsections of the logfile
void logger::split(int i){
    subsections=new stringstream[i];
    sectionNumber=i;
    hasSplit=true;
}

//write out each subsection to the logfile and exit subsections
void logger::combine(){
    logFile<<endl;
    for(int i=0;i<sectionNumber;i++){
        logFile<<endl<<subsections[i].str()<<endl;
    }
    logFile<<endl;
    delete[] subsections;
    subsections=NULL;
    hasSplit=false;
    sectionNumber=0;
}

logger::~logger(){
    logFile.close();
    if(subsections)
        delete[] subsections;
    
}

