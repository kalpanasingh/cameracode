//
//  funcs.cxx
//  
//
//  Created by Kalpana Singh on 2013-05-22.
//
//this file just defines a few simple useful functions
//Modified on 2013-10-23
//
#include "funcs.h"
#include <string>
#include <sstream>
#include <time.h>

using namespace std;

string convertIntToString(int i){
    std::stringstream sstream;
    sstream << i;
    return sstream.str();
}

string makeTimestamp(time_t t){
    time_t time=t;
    char buff[20];
    strftime(buff, 20, "%Y-%m-%dT%H:%M:%S", localtime(&time));
    return string(buff);
}

string serialToCamID(int serial){
    string camID;
    switch(serial){
            //enter correct serial to cam number correspondense here:
        case 5067999:
            camID="CameraP";
            break;
        case 5068091:
            camID="Camera1";
            break;
        case 5062213:
            camID="Camera2";
            break;
        case 5037503:
            camID="Camera3";
            break;
        case 5037501:
            camID="Camera4";
            break;
        case 5068000:
            camID="Camera5";
            break;
        default:
            camID="unknown";
            break;
    }
    return camID;
}
