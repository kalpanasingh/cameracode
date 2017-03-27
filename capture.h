/*this is the main file for preforming an image-capture run. Upon running the capture function is detects attatched cameras,
 attempts to set the time-stamp on each file, instructs each camera to capture an image, and prints results to a logfile*/

#ifndef _capture_h
#define _capture_h
#include <string>
//strut defining return values for the main capture function, these are returned to the calling python script
struct captureReturn{
    int errorFlag;//an integer indicating any errors that occured during the capture run
    int captureTime;//time of the start of the capture run
    int activeCams;//number of detected cameras for this capture run
};

captureReturn capture(char* dirName);//call this function to preform an image capture run

#endif
