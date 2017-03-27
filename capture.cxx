/*this is the main file for preforming an image-capture run originally created by Kalpana Singh. Upon running the capture function 
 * it detects attatched cameras, attempts to set the time-stamp on each file, instructs each camera to capture an image, and prints 
 * results to a logfile*/
extern "C" {
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-log.h>
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-port.h>
    
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-filesys.h>
    
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-version.h>
}

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <time.h>
#include <sys/time.h>


#include "gp-params.h"
#include "actions.h"
#include "funcs.h"

#include <omp.h>


using namespace std;

//this structure is used to pass arguments to ctx_error_func since arguments must be passed as a void* as defined in libgphoto2
struct ctx_error_args{
    logger* log;
    int* errorFlag;
};

//print any context errors passed from library which arise during camera setup to the logfile and preform other error handling on them
//also changes the message to reflect the usual cause of the error in the context of the is program.
//the string comparison is messy but without modifying libgphoto2 itself it seems to be the only way to distinguish context errors
static void
ctx_error_func (GPContext *context, const char *str, void *data) {
    ctx_error_args* args=(ctx_error_args*) data;
    logger* captureLog=args->log;
    int* errorFlag=args->errorFlag;
    if(strcmp(str,"PTP I/O error")==0){
        str="Lost Communication with camera. USB/power failure?";
        *errorFlag=-1;
    }
    else if(strcmp(str,"An error occurred in the io-library ('Could not claim the USB device'): Could not claim interface 0 (No such file or directory). Make sure no other program or kernel module (such as sdc2xx, stv680, spca50x) is using the device and you have read/write access to the device. ")==0){
        str="Failed to claim USB device. Camera may be in use by another application/kernel module. May also be caused by low voltage to camera, or bad/busy camera state. If the later, reseting the camera may solve the problem.";
        *errorFlag=-9;
    }
    captureLog->write("***CONTEXT_ERROR: "+string(str));
}


//attempt to remove lockfile in the event of abnormal termination
void sig_handler(int sigID){
    system("rm /tmp/lockfile");
    cout<<"Program recieved signal "<<sigID<<endl;
    exit(sigID);
}


//this function preforms all nessesary steps to detect cameras and attatch an instance of GPParams to each discovered camera to allow nessesary camera control
int list_detect_action (GPParams **params, logger* captureLog) {
    int i, count, retval=0, errorFlag=0;
    
    //search system for ports and store them in a list
    captureLog->write("Detecting ports.");
    GPPortInfoList *portList=_get_portinfo_list();
    
    //variable declaration/initialization
    CameraList *CameraList;
    GPContext *Context = gp_context_new ();
    ctx_error_args errorArgs;
    errorArgs.log=captureLog;
    errorArgs.errorFlag=&errorFlag;
    gp_context_set_error_func (Context, ctx_error_func, &errorArgs);
    const char *name = NULL, *value = NULL;
    char ID[8];
    stringstream logBuffer;
    CameraAbilitiesList *CameraAbilityList = NULL;
    
    //Attempt to load camera drivers from system
    captureLog->write("Detecting camera drivers.");
    retval=gp_list_new (&CameraList);
    if(retval==0)retval=gp_abilities_list_new (&CameraAbilityList);
    if(retval==0)retval=gp_abilities_list_load(CameraAbilityList,Context);
    if(retval<0){
        captureLog->write("Camera-driver loading failed with libgphoto2 error:"+string(gp_result_as_string(retval))+" libgphoto2 installation issue?");
        if(CameraList) gp_list_free(CameraList);
        if(errorFlag==0) errorFlag=-2;
        return errorFlag;
    }
    
    //attempt to detect supported cameras based on supplied portlist and CameraAbilityList, Store detected cameras in CameraList
    captureLog->write("Detecting supported cameras.");
    retval=gp_abilities_list_detect (CameraAbilityList, portList, CameraList, Context);
    if(retval<0){
        captureLog->write("Camera detection failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
        if(errorFlag==0) errorFlag=-10;
        return errorFlag;
    }
    
    //count detected cameras
    count = gp_list_count (CameraList);
    if(count<0){
        captureLog->write("Camera counting failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
        if(errorFlag==0) errorFlag=-10;
        return errorFlag;
    }
    captureLog->write(convertIntToString(count)+" supported cameras detected.");
    
    //this loop gets some info and preforms initialization on each detected cameras
    for (i = 0; i < count; i++) {
        retval=gp_list_get_name  (CameraList, i, &name);
        if(retval==0) retval=gp_list_get_value (CameraList, i, &value);
        if(retval!=0){
            if(CameraList) gp_list_free(CameraList);
            captureLog->write("Camera model lookup failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
            if(errorFlag==0) errorFlag=-10;
            return errorFlag;
        }
        
        //log camera model and port id
        logBuffer<<i+1<<" "<<name<<" "<<value;
        captureLog->write(logBuffer.str());
        logBuffer.str("");
        
        //associate the camera fCamera of each gp_params object with a different port, and hence a differnt camera from those detected
        GPPortInfo info;
        int p;
        p = gp_port_info_list_lookup_path (portList, value);
        if(p>=0)retval = gp_port_info_list_get_info (portList, p, &info);
        else retval=p;
        if(retval==0) retval=gp_camera_set_port_info (params[i]->fCamera, info);
        if(retval!=0){
            captureLog->write("Port-setting failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
            if(errorFlag==0) errorFlag=-10;
            return errorFlag;
        }
        
        
        //initialize camera for communication
        retval=gp_camera_init(params[i]->fCamera, Context);
        if(retval!=0){
            captureLog->write("Initialization of camera at port "+string(value)+" failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
            if(errorFlag==0){
                if(retval==GP_ERROR_IO_USB_CLAIM){
                    errorFlag=-9;
                }
                else if(retval==GP_ERROR_BAD_PARAMETERS){
                    errorFlag=-1;
                }
                else{
                    errorFlag=-10;
                }
            }
            return errorFlag;
        }
        
        //load a summary of camera information defined by libgphoto2
        CameraText text;
        retval=gp_camera_get_summary (params[i]->fCamera, &text, Context);
        if(retval!=0){
            captureLog->write("Camera summary lookup failed with libgphoto2 error:"+string(gp_result_as_string(retval)));
            if(errorFlag==0) errorFlag=-10;
            return errorFlag;
        }
        
        //parse summary to find serial number, save this and also use it to find camID (i.e. camera1,2,3,4,5,p) if camera is known
        string SerialLine(text.text);
        SerialLine.copy(ID,7, SerialLine.find("  Serial Number: ")+17);
        params[i]->fSerialNumber=atoi(ID);
        params[i]->camID=serialToCamID(params[i]->fSerialNumber);
    }
    if(CameraList) gp_list_free (CameraList);
    return count;
}

//strut defining return values for the main capture function, these are returned to the calling python script
struct captureReturn{
    int errorFlag;//an integer indicating any errors that occured during the capture run
    int captureTime;//time of the start of the capture run
    int activeCams;//number of detected cameras for this capture run
};


captureReturn capture (char* dirName) {
    string captureFolder = std::string(dirName);
    stringstream logBuffer;
    logger* captureLog;
    int i=0, count, retval=0, retFlag=0;
    time_t currentTime;
    captureReturn ret={};
    
    //initialize the members of the return struct
    ret.captureTime=0;
    ret.errorFlag=0;
    ret.activeCams=0;
    
    //signal handling, simply attempts to remove lockfile before exiting
    signal(SIGINT,sig_handler);
    signal(SIGSEGV,sig_handler);
    signal(SIGTERM,sig_handler);
    signal(SIGABRT,sig_handler);
    
    //check gphoto2 version if it isn't 2.5.x don't run
    string version;
    version=string(*gp_library_version(GP_VERSION_SHORT));
    if(version.substr(0,3)!="2.5"){
        cout<<"libgphoto2 version is "<<version<<" please link against 2.5.2 instead."<<endl;
        ret.errorFlag=2;
        return ret;
    }
    
    
    //check for existence of lockfile and exit program if present
    if(gp_system_is_file( "/tmp/lockfile")) {
        cout<<"Lockfile present. Program already running? If not try rm /tmp/lockfile and run again.\n";
        ret.errorFlag=7;
        return ret;
    }
    
    //create a lockfile to prevent multiple instances of program running at once
    system("touch /tmp/lockfile");
    

    if(!gp_system_is_dir(captureFolder.data())){
        cout<<"Failed to create capture folder for this run with libgphoto2 error, most likely permission issue. try running as root?"<<endl;
        system("rm /tmp/lockfile");
        ret.errorFlag=4;
        return ret;
    }
    
    //create a logfile for this run in the new directory
    try {
        captureLog=new logger(std::string(captureFolder)+gp_system_dir_delim+"logfile.log");
    } catch (bad_alloc& ) {
        printf("not enough memory\n");
        cout<<"Allocation failed. Not enough memory?"<<endl;
        system("rm /tmp/lockfile");
        ret.errorFlag=8;
        return ret;
    }

    //set returned time stamp to match starttime in log file
    struct timeval logTime=captureLog->startTime;
    ret.captureTime=logTime.tv_sec;

    //Each GPParam object will be associated to a camera:
    GPParams *gp_params[6];
    for (i=0 ; i< 6 ;++i){
        try {
            gp_params[i] = new GPParams();
        } catch (bad_alloc& ) {
            printf("not enough memory\n");
            captureLog->write("Allocation failed. Not enough memory?");
            system("rm /tmp/lockfile");
            ret.errorFlag=8;
            return ret;
        }
    }
    //detect cameras and associate them to a gp_params instance, return the number of cameras detected
    count = list_detect_action (gp_params, captureLog);
    if (count<0){
        system("rm /tmp/lockfile");
        ret.errorFlag=-1*count;
        return ret;
    }
    else if (count ==0) {
        captureLog->write("No camera found.");
        system("rm /tmp/lockfile");
        ret.errorFlag=5;
        return ret;
    }
    ret.activeCams=count;
    
    //create subsections of captureLog to separate information about various cameras
    try{
        captureLog->split(count);
    }
    catch (bad_alloc& ) {
        printf("not enough memory\n");
        captureLog->write("Allocation failed. Not enough memory?");
        system("rm /tmp/lockfile");
        ret.errorFlag=8;
        return ret;
    }
    
    //manage each camera on a seperate thread
#pragma omp parallel num_threads(6) shared (gp_params, captureLog)
#pragma omp for ordered
    for (i=0 ; i< count ;++i) {
        stringstream threadedLogBuffer;
        threadedLogBuffer<<"Camera ID : "<<gp_params[i]->fSerialNumber<<"("<<gp_params[i]->camID<<")";
        gp_params[i]->logNum=i;
        captureLog->write(threadedLogBuffer.str(),i);
        threadedLogBuffer.str("");
        
        gp_params[i]->fFolder=std::string(captureFolder);
        gp_params[i]->fLogFile=captureLog;
        
        //update camera time before image capture
        //the elaborate procedure is used to get the local time (rather than say the localtime() funtion) because
        //libgphoto2 takes an integer argument for setting the time and timezone must be accounted for manually
        captureLog->write("Updating time on camera.",i);;
        tzset();
        bool DST=(daylight != 0);
        currentTime=time(NULL)-timezone+DST*3600;
        retval=gp_params[i]->set_config_int("datetime",currentTime);
        if(retval!=0){
            retFlag=retval;
            captureLog->write("failed to set time.",i);
        }
        else{
            char* time=asctime(gmtime(&currentTime));
            time[strlen(time)-1]='\0';
            captureLog->write("Set time to:"+string(time)+".",i);;
        }
        
        //Capture Image:
        captureLog->write("Capturing image:",i);
        retval=gp_params[i]->capture_to_file();
        if(retval!=0){
            retFlag=retval;
            captureLog->write("Failed to capture image.",i);
        }
        
        //Exit camera (preforms nessesary camera cleanup on exit), failing to do this would likely lead to delays or other problems on subsequent runs
        captureLog->write("Exiting camera.",i);;
        retval=gp_params[i]->exit_camera();
        if(retval!=0){
            retFlag=retval;
            captureLog->write("Failed to exit camera.",i);
        }
    }
    captureLog->combine();
    
    system("rm /tmp/lockfile");
    
    
    ret.errorFlag=retFlag;
    return ret;
}
