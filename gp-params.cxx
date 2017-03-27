/*Modified version of gp-params.c from gphoto2, each gp-params instance holds information about a given camera and methods for interacting
 with it*/
#include "gp-params.h"
#  define _(String) (String)
#include "actions.h"
#include "funcs.h"
#include "logger.h"

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <utime.h>
#include <csignal>

#include <cstring>
#include <sstream>


//print any context errors which arise during a camera run to the appropriate section of the logfile and set errorFlag
//also changes the message to reflect the usual cause of the error in the context of the is program.
//the string comparison is messy but without modifying libgphoto2 itself it seems to be the only way to distinguish context errors
static void
param_ctx_error_func (GPContext *context, const char *str, void *data) {
    GPParams* params=(GPParams*) data;
    if(strcmp(str,"PTP I/O error")==0){
        params->errorFlag=1;
        str="Lost Communication with camera. USB/power failure?";
    }
    else if(strcmp(str,"You need to specify a folder starting with /store_xxxxxxxxx/")==0){
        str="Error saving to disk. Disk full?";
        params->errorFlag=4;
    }
    else if(strcmp(str,"An error occurred in the io-library ('Could not claim the USB device'): Could not claim interface 0 (No such file or directory). Make sure no other program or kernel module (such as sdc2xx, stv680, spca50x) is using the device and you have read/write access to the device. ")==0){
        str="Failed to claim USB device. Camera may be in use by another application/kernel module.";
        params->errorFlag=9;
    }
    else if(strcmp(str,"PTP Store Not Available")==0){
        str="Libgphoto2 returned \"PTP Store Not Available\", this could indicated low voltage.";
        params->errorFlag=3;
    }

    params->fLogFile->write("***CONTEXT_ERROR: "+string(str),params->logNum);
    
}

GPParams::GPParams () {
  fSerialNumber=0;
  camID="unknown";
  logNum=0;
  errorFlag=0;
  gp_camera_new (&fCamera);
  fFolder= "/";
  fContext = gp_context_new ();
  fLogFile=NULL;
  gp_context_set_error_func (fContext, param_ctx_error_func, this);
  
}

GPParams::~GPParams  () {
    if (fCamera)
    gp_camera_unref (fCamera);
  if (fContext)
    gp_context_unref (fContext);
 }


//attempt to set a camera configuration option to an integer value, this is used to set the time on the cameras
int GPParams::set_config_int(char* option,int value){
    errorFlag=0;
    CameraWidget *rootconfig,*child;
    int retval,t;
    t=value;
    
    retval = _find_widget_by_name (this, option, &child, &rootconfig);
    if(retval==0) retval = gp_widget_set_value (child, &t);
    if(retval==0) retval = gp_camera_set_config (this->fCamera, rootconfig, this->fContext);
    if (retval != 0){
    fLogFile->write("Configuration of option "+string(option)+" failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
    if(errorFlag==0) errorFlag=6;
    }

    if(rootconfig) gp_widget_free (rootconfig);
    
    return errorFlag;
}

//save a given file from the camera to disk, the location to save is defined within this function
void GPParams::save_captured_file(CameraFilePath camera_file_path){
    int fd, retval=0;
    stringstream logBuffer;
    string saveName;
	CameraFile *file;
    time_t createTime;
    
    //find the file extension of the captured image
    string camFileName=string(camera_file_path.name);
    unsigned int dotPos=camFileName.find_last_of('.');
    string suffix=camFileName.substr(dotPos,-1);
    
    string saveID=camID;
    
    
    //if camID is unknown save by serial number instead
    if(strcmp(saveID.data(),"unknown")==0){
        saveID=convertIntToString(fSerialNumber);
    }
    
    //enter desired directory and file name information here
    saveName =fFolder
    + gp_system_dir_delim
    + saveID;
    
    //create new file for picture on disk
    if(errorFlag==0){
        fLogFile->write("Saving file to disk as: "+saveName,logNum);
        
        fd = open(saveName.data(), O_CREAT | O_WRONLY, 0644);
        if (fd<0){
            fLogFile->write("Failed to create new file on disk. Premissions issue?",logNum);
            errorFlag=4;
            return;
        }
        retval = gp_file_new_from_fd(&file, fd);
        if(retval!=0){
            fLogFile->write("Saving image to local disk failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
            if(errorFlag==0) errorFlag=10;
        }
    }
    
    
    //retrieve camera file and save to disk
    if(errorFlag==0){
        logBuffer<<"Retrieving file "<<camera_file_path.folder<<"/"<<camera_file_path.name<<" from camera.";
        fLogFile->write(logBuffer.str(),logNum);
        logBuffer.str("");
        
        retval = gp_camera_file_get(fCamera, camera_file_path.folder, camera_file_path.name,
                                    GP_FILE_TYPE_NORMAL, file, fContext);
        if(retval!=0){
            fLogFile->write("Retrieving file from camera failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
            if(errorFlag==0){
                if(retval==GP_ERROR_FILE_NOT_FOUND||retval==GP_ERROR_DIRECTORY_NOT_FOUND){
                    fLogFile->write("Camera does not appear to contain captured image at expected path.");
                    errorFlag=3;
                }
                else{
                    errorFlag=10;
                }
            }
        }
    }
    
    //add timestamp and suffix to file
    if(errorFlag==0){
        fLogFile->write("Appending timestamp and suffix to file name.",logNum);
        retval=gp_file_get_mtime(file,&createTime);
        string newName=saveName
        +"_"
        +makeTimestamp(createTime)
        +suffix;
        
        rename(saveName.data(),newName.data());
        fLogFile->write("New file name is "+newName,logNum);
    }
    
    //delete file from camera RAM
    if(errorFlag==0){
        logBuffer<<"Deleting file "<<camera_file_path.folder<<"/"<<camera_file_path.name<<" from camera.";
        fLogFile->write(logBuffer.str(),logNum);
        logBuffer.str("");
        retval = gp_camera_file_delete(fCamera, camera_file_path.folder, camera_file_path.name,
                                       fContext);
        if(retval!=0){
            fLogFile->write("Deleting file from camera failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
            if(errorFlag==0) errorFlag=10;
        }
    }
    if(file) gp_file_free(file);
    return;
}


//attempt to capture an image from camera to disk
int GPParams::capture_to_file() {
    errorFlag=0;
	int retval=0;
    stringstream logBuffer;
    string saveName;
	CameraFilePath camera_file_path;
    CameraEventType evtype;
    void *data;
    long waittime=200;
    
    //capture image, image is initially saved to camera ram, the name of the file on camera is written to camera_file_path
    if(errorFlag==0){
        fLogFile->write("Capturing image to camera.",logNum);
        retval=gp_camera_capture(fCamera, GP_CAPTURE_IMAGE, &camera_file_path, fContext);
        if(retval !=0){
            fLogFile->write("Capture failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
            if(errorFlag==0) errorFlag=10;
            return errorFlag;
        }
    
        logBuffer<<"File saved to camera as:"<<camera_file_path.folder<<"/"<<camera_file_path.name;
        fLogFile->write(logBuffer.str(),logNum);
        logBuffer.str("");
    }
    //save first captured file to camera
    save_captured_file(camera_file_path);
    //give the camera some time to drain it's event que, mainly in order to download extra images
    //(e.g. if NEF+JPG capture mode is used), the 10 iterations is arbitrary but seems to be more
    //than sufficient to drain the que for the desired cases.
    if(errorFlag==0) {
        int eventCheck=10;
        for(int i=0;i<eventCheck;i++){
            retval=gp_camera_wait_for_event (fCamera, waittime, &evtype,&data,fContext);
            if(evtype==GP_EVENT_FILE_ADDED) {
                camera_file_path= *(CameraFilePath*) data;
                logBuffer<<"Loading additional file from camera:"<<camera_file_path.folder<<"/"<<camera_file_path.name;
                fLogFile->write(logBuffer.str(),logNum);
                logBuffer.str("");
                save_captured_file (camera_file_path);
            }
            else if(evtype==GP_EVENT_CAPTURE_COMPLETE||evtype==GP_EVENT_TIMEOUT){
                break;
            }
        }
    }
    if(data) free(data);
    
    return errorFlag;
}

//Exit camera (preforms nessesary camera cleanup on exit), failing to do this would likely lead to delays
//or other problems on subsequent runs if the camera is not restarted between them
int GPParams::exit_camera(){
    errorFlag=0;
    int retval=0;
    retval=gp_camera_exit(fCamera,fContext);
    if(retval!=0){
        if(errorFlag==0) errorFlag=10;
        fLogFile->write("Camera exit failed with libgphoto2 error: "+string(gp_result_as_string(retval)),logNum);
    }
    return errorFlag;
}

