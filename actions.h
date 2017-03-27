/*This is just a highly reduced version of the actions.h file from gphoto2, the functions it contains were essentially pulled directly
 from gphoto2 for convenience*/

#ifndef __ACTIONS_H__
#define __ACTIONS_H__
extern "C" {
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
}

#include "gp-params.h"

GPPortInfoList* _get_portinfo_list ();
int _find_widget_by_name (GPParams *, const char *, CameraWidget **, CameraWidget **);

#endif /* __ACTIONS_H__ */



/*
 * Local Variables:
 * c-file-style:"linux"
 * indent-tabs-mode:t
 * End:
 */
