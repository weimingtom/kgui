/********************************************************************/
/* this file is NOT included into the libkgui, it is to be included */
/* by the application itself and is used to select the target system */
/********************************************************************/
#include "kgui.h"

//The application needs to define the following:
//#define APPNAME "appname"

//These are optional defines, if not defined then they default to full screen
//#define SCREENWIDTH xxx
//#define SCREENHEIGHT xxx

/* for windows (WIN32 and MINGW), the IDC and IDI defines from resource.h */
/* need to also be passed as: */
/* change the xxx to the names used in resource.h */
//#define IDC_APPNAME IDC_xxx
//#define IDI_APPNAME IDI_xxx

#if defined(LINUX)
#include "kguilinux.cpp"
#elif defined(MACINTOSH)
#include "kguimac.cpp"
#elif defined(MINGW)
#include "kguimingw.cpp"
#elif defined(WIN32)
#include "kguipc.cpp"
#else
//Unknown or unsupported platform
#error
#endif
