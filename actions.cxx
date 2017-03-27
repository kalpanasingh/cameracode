/*This is just a highly reduced version of the actions.c file from gphoto2, the functions it contains were essentially pulled directly
 from gphoto2 for convenience*/

#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

extern "C" {
#include <gphoto2/gphoto2-port-log.h>
#include <gphoto2/gphoto2-setting.h>
#include <gphoto2/gphoto2-filesys.h>
#ifdef HAVE_AA
#  include "gphoto2-cmd-capture.h"
#endif
  
}
#include "actions.h"
#  define _(String) (String)


#ifdef HAVE_LIBEXIF
#  include <libexif/exif-data.h>
#endif

#ifdef __GNUC__
#define __unused__ __attribute__((unused))
#else
#define __unused__
#endif

//initialize and return a list of detected ports
GPPortInfoList*
_get_portinfo_list () {
  int count, result;
  GPPortInfoList *list = NULL;
  if (gp_port_info_list_new (&list) < GP_OK)
    return NULL;
  result = gp_port_info_list_load (list);
  if (result < 0) {
    gp_port_info_list_free (list);
    return NULL;
  }
  count = gp_port_info_list_count (list);
  if (count < 0) {
    gp_port_info_list_free (list);
    return NULL;
  }
  return list;
}


//this function finds a configuration option within the configuration tree in the camera filesystem
int
_find_widget_by_name (GPParams *p, const char *name, CameraWidget **child, CameraWidget **rootconfig) {
  int ret;

  ret = gp_camera_get_config (p->fCamera, rootconfig, p->fContext);
  if (ret != GP_OK) return ret;
  ret = gp_widget_get_child_by_name (*rootconfig, name, child);
  if (ret != GP_OK)
    ret = gp_widget_get_child_by_label (*rootconfig, name, child);
  if (ret != GP_OK) {
    char    *part, *s, *newname;

    newname = strdup (name);
    if (!newname)
      return GP_ERROR_NO_MEMORY;

    *child = *rootconfig;
    part = newname;
    while (part[0] == '/')
      part++;
    while (1) {
      CameraWidget *tmp;

      s = strchr (part,'/');
      if (s)
        *s='\0';
      ret = gp_widget_get_child_by_name (*child, part, &tmp);
      if (ret != GP_OK)
        ret = gp_widget_get_child_by_label (*child, part, &tmp);
      if (ret != GP_OK)
        break;
      *child = tmp;
      if (!s) /* end of path */
        break;
      part = s+1;
      while (part[0] == '/')
        part++;
    }
    if (s) { /* if we have stuff left over, we failed */
      gp_context_error (p->fContext, _("%s not found in configuration tree."), newname);
      free (newname);
      gp_widget_free (*rootconfig);
      return GP_ERROR;
    }
    free (newname);
  }
  return GP_OK;
}


/*
 * Local Variables:
 * c-file-style:"linux"
 * indent-tabs-mode:t
 * End:
 */
