/*Modified version of gp-params.h from gphoto2, each gp-params instance holds information about a given camera and methods for interacting
 with it*/

#ifndef __GP_PARAMS_H__
#define __GP_PARAMS_H__



extern "C" {
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-context.h>
}
#include <iostream>
#include <string>
#include "logger.h"

using namespace std;

class GPParams {
public:
  GPParams();
  ~GPParams();
  Camera *fCamera;//camera object
  string camID;//identifier associated with camera (if known)
  int fSerialNumber;//serial number of associated camera
  GPContext *fContext;//a gphoto context to pass back any errors from the library ect.
  int errorFlag; //set when a context error occurs and allows implimentation of error handling
  string fFolder;//folder to store captured images
  logger *fLogFile;//logfile to store progress and errors for this run
  int logNum;//logfile subsection to write to for associated camera
  int capture_to_file();//capture an image and save to disk
  int set_config_int(char* option,int value); //attempt to set a camera configuration to an integer value
  int exit_camera(); //Exit camera (preforms nessesary camera cleanup on exit)
private:
  void save_captured_file(CameraFilePath camera_file_path);//save a captured image to the disk
};




#endif


/*
 * Local Variables:
 * c-file-style:"linux"
 * indent-tabs-mode:t
 * End:
 */
