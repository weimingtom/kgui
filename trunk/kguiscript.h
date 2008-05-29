#ifndef __KGUISCRIPT__
#define __KGUISCRIPT__

#include "kguireport.h"

/*****************************************************************************/
/**																			**/
/** These structures are used for generating kgui objects based on an		**/
/**	array of script structures.												**/
/**																			**/
/** These structures are currently exported by custom code in				**/
/** Microsoft Access. The output can then be hand tweaked.					**/
/**																			**/
/**																			**/
/*****************************************************************************/

/* form scripts first */

#define SCRIPTSCALE 14

enum
{
OBJ_TABCTL,
OBJ_PAGE,
OBJ_ENDPAGE,
OBJ_COMMANDBUTTON,
OBJ_TEXTBOX,
OBJ_LABEL,
OBJ_IMAGE,
OBJ_COMBOBOX,
OBJ_CHECKBOX,
OBJ_TABLE,
};

typedef struct
{
int type;
}Obj_DEF;

typedef struct
{
int type;
kGUITabObj *obj;
const char *name;
int x;
int y;
int w;
int h;
int numpages;
}TabCtl_DEF;

typedef struct
{
int type;
const char *name;
int pageindex;
}Page_DEF;

typedef struct
{
int type;
}EndPage_DEF;

typedef struct
{
int type;
kGUIButtonObj *obj;
const char *name;
int x;
int y;
int w;
int h;
const char *caption;
int fontsize;
}CommandButton_DEF;

typedef struct
{
int type;
kGUIComboBoxObj *obj;
const char *name;
int x;
int y;
int w;
int h;
const char *string;
}ComboBox_DEF;

typedef struct
{
int type;
kGUIInputBoxObj *obj;
const char *name;
int x;
int y;
int w;
int h;
int fontsize;
}TextBox_DEF;

typedef struct
{
int type;
kGUITickBoxObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}CheckBox_DEF;

typedef struct
{
int type;
kGUITextObj *obj;
const char *name;
int x;
int y;
int w;
int h;
int fontsize;
const char *caption;
}Label_DEF;

typedef struct
{
int type;
kGUIImageObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}Image_DEF;

typedef struct
{
int type;
kGUITableObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}Table_DEF;

typedef struct
{
	kGUIObj *obj;
	const char *name;
}ObjName_DEF;

class kGUIFormScript
{
public:
	void InitScript(kGUIWindowObj *window,void **script,int numentries);
	kGUIObj *LocateObj(const char *n);
	kGUITextObj *LocateTextObj(const char *n) {return static_cast<kGUITextObj *>(LocateObj(n));}
	kGUIInputBoxObj *LocateInputboxObj(const char *n) {return static_cast<kGUIInputBoxObj *>(LocateObj(n));}
	kGUIComboBoxObj *LocateComboboxObj(const char *n) {return static_cast<kGUIComboBoxObj *>(LocateObj(n));}
	kGUITickBoxObj *LocateTickboxObj(const char *n) {return static_cast<kGUITickBoxObj *>(LocateObj(n));}
	kGUIImageObj *LocateImageObj(const char *n) {return static_cast<kGUIImageObj *>(LocateObj(n));}
	kGUIButtonObj *LocateButtonObj(const char *n) {return static_cast<kGUIButtonObj *>(LocateObj(n));}
	kGUITableObj *LocateTableObj(const char *n);
private:
	int m_numobjects;
	Array<ObjName_DEF>m_objnames;
};

/* report scripts next */

enum
{
OBJ_REPORTSECTION,
OBJ_REPORTLABEL,
OBJ_REPORTTEXT,
OBJ_REPORTCHECKBOX,
OBJ_REPORTIMAGE,
OBJ_REPORTRECT
};

#if 0
typedef struct
{
int type;
kGUIReportTextObj *obj;
char *name;
int x;
int y;
int w;
int h;
int fontsize;
bool framed;
}RepTextBox_DEF;
#endif

typedef struct
{
int type;
kGUIReportTickboxObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}RepCheckBox_DEF;

typedef struct
{
int type;
kGUIReportTextObj *obj;
const char *name;
int x;
int y;
int w;
int h;
int fontsize;
int bold;
int framed;
char *caption;
}RepLabel_DEF;

typedef struct
{
int type;
kGUIReportImageObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}RepImage_DEF;

typedef struct
{
int type;
kGUIReportRectObj *obj;
const char *name;
int x;
int y;
int w;
int h;
}RepRect_DEF;

typedef struct
{
int type;
int section;
}RepSection_DEF;

class kGUIReportScript
{
public:
	void InitScript(kGUIReport *report,void **script,int numentries);
private:
};
#endif
