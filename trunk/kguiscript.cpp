/**********************************************************************************/
/* kGUI - kguiscript.cpp                                                          */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://www.scale18.com/cgi-bin/page/kgui.html	                              */
/*                                                                                */
/*    kGUI is free software; you can redistribute it and/or modify                */
/*    it under the terms of the GNU Lesser General Public License as published by */
/*    the Free Software Foundation; version 2.                                    */
/*                                                                                */
/*    kGUI is distributed in the hope that it will be useful,                     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/*    GNU General Public License for more details.                                */
/*                                                                                */
/*    http://www.gnu.org/licenses/lgpl.txt                                        */
/*                                                                                */
/*    You should have received a copy of the GNU General Public License           */
/*    along with kGUI; if not, write to the Free Software                         */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"
#include "kguiscript.h"

#define FONTSCALE(x) ((int)((float)(x)*1.4f))

void kGUIFormScript::InitScript(kGUIWindowObj *window,void **script,int numentries)
{
	int e;
	kGUITabObj *lasttab=0;
	kGUIContainerObj *parent;
	ObjName_DEF on;

	m_numobjects=0;
	m_objnames.SetGrow(true);

	parent=window;
	for(e=0;e<numentries;++e)
	{
		switch(((Obj_DEF *)script[e])->type)
		{
		case OBJ_TABCTL:
		{
			TabCtl_DEF *data;
			kGUITabObj *obj;

			data=(TabCtl_DEF *)script[e];
			obj=data->obj;

			//			obj->SetName(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			obj->SetNumTabs(data->numpages);
			parent->AddObject(obj);
			parent=obj;
			lasttab=obj;
			obj->SetCurrentTab(0);
		}
		break;
		case OBJ_PAGE:
		{
			Page_DEF *data;

			data=(Page_DEF *)script[e];
			if(lasttab)
			{
				lasttab->SetTabName(data->pageindex,data->name);
				lasttab->SetCurrentTab(data->pageindex);
			}
		}
		break;
		case OBJ_ENDPAGE:
			if(lasttab)
				lasttab->SetCurrentTab(0);
			parent=window;
		break;
		case OBJ_COMMANDBUTTON:
		{
			CommandButton_DEF *data;
			kGUIButtonObj *obj;

			data=(CommandButton_DEF *)script[e];
			obj=data->obj;

//			obj->SetName(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			obj->SetString(data->caption);
			obj->SetFontSize(FONTSCALE(data->fontsize));
			parent->AddObject(obj);
		}
		break;
		case OBJ_TEXTBOX:
		{
			TextBox_DEF *data;
			kGUIInputBoxObj *obj;

			data=(TextBox_DEF *)script[e];
			obj=data->obj;

//			obj->SetTitle(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			obj->SetFontSize(FONTSCALE(data->fontsize));
			parent->AddObject(obj);
		}
		break;
		case OBJ_LABEL:
		{
			Label_DEF *data;
			kGUITextObj *obj;

			data=(Label_DEF *)script[e];
			obj=data->obj;

//			obj->SetTitle(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			obj->SetFontSize(FONTSCALE(data->fontsize));
			obj->SetString(data->caption);
			parent->AddObject(obj);
		}
		break;
		case OBJ_IMAGE:
		{
			Image_DEF *data;
			kGUIImageObj *obj;

			data=(Image_DEF *)script[e];
			obj=data->obj;

//			obj->SetTitle(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			parent->AddObject(obj);
		}
		break;
		case OBJ_COMBOBOX:
		{
			ComboBox_DEF *data;
			kGUIComboBoxObj *obj;

			data=(ComboBox_DEF *)script[e];
			obj=data->obj;

//			obj->SetName(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			parent->AddObject(obj);
		}
		break;
		case OBJ_CHECKBOX:
		{
			CheckBox_DEF *data;
			kGUITickBoxObj *obj;

			data=(CheckBox_DEF *)script[e];
			obj=data->obj;

//			obj->SetTitle(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
		//	obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			parent->AddObject(obj);
		}
		break;
		case OBJ_TABLE:
		{
			Table_DEF *data;
			kGUITableObj *obj;

			data=(Table_DEF *)script[e];
			obj=data->obj;

//			obj->SetTitle(data->name);
			on.obj=obj;
			on.name=data->name;
			m_objnames.SetEntry(m_numobjects++,on);

			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			parent->AddObject(obj);
		}
		break;
		}
	}
	window->ExpandToFit();
}


kGUIObj *kGUIFormScript::LocateObj(const char *n)
{
	int i;
	ObjName_DEF on;

	for(i=0;i<m_numobjects;++i)
	{
		on=m_objnames.GetEntry(i);
		if(!strcmp(on.name,n))
			return(on.obj);
	}
//	assert(false,"Object '%s' not found error!",n);
	return(0);
}

/***************************************************************************/

void kGUIReportScript::InitScript(kGUIReport *report,void **script,int numentries)
{
	int section;
	int e;

	section=REPORTSECTION_BODY;	/* default section */
	for(e=0;e<numentries;++e)
	{
		switch(((Obj_DEF *)script[e])->type)
		{
		case OBJ_REPORTSECTION:
		{
			RepSection_DEF *data;

			data=(RepSection_DEF *)script[e];
			section=data->section;
		}
		break;
		case OBJ_REPORTTEXT:
		case OBJ_REPORTLABEL:
		{
			RepLabel_DEF *data;
			kGUIReportTextObj *obj;

			data=(RepLabel_DEF *)script[e];
			obj=data->obj;
			obj->SetName(data->name);
			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			obj->SetFontSize(FONTSCALE(data->fontsize));
			obj->SetFrame(data->framed>0);
			obj->SetFontID(kGUI::GetDefFontID()+data->bold);
			obj->SetString(data->caption);
			report->AddObjToSection(section,obj);
		}
		break;
		case OBJ_REPORTIMAGE:
		{
			RepImage_DEF *data;
			kGUIReportImageObj *obj;

			data=(RepImage_DEF *)script[e];
			obj=data->obj;
			obj->SetName(data->name);
			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			report->AddObjToSection(section,obj);
		}
		break;
		case OBJ_REPORTRECT:
		{
			RepRect_DEF *data;
			kGUIReportRectObj *obj;

			data=(RepRect_DEF *)script[e];
			obj=data->obj;
			obj->SetName(data->name);
			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
			obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			report->AddObjToSection(section,obj);
		}
		break;
		case OBJ_REPORTCHECKBOX:
		{
			RepCheckBox_DEF *data;
			kGUIReportTickboxObj *obj;

			data=(RepCheckBox_DEF *)script[e];
			obj=data->obj;
			obj->SetName(data->name);
			obj->SetPos(data->x/SCRIPTSCALE,data->y/SCRIPTSCALE);
		//	obj->SetSize(data->w/SCRIPTSCALE,data->h/SCRIPTSCALE);
			report->AddObjToSection(section,obj);
		}
		break;
		}
	}
}
