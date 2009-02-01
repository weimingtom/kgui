/**********************************************************************************/
/* kGUI - kguireq.cpp                                                             */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://code.google.com/p/kgui/	                                              */
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

/*! @file kguireq.cpp
    @brief This file contains the generic popup request windows for asking the user for   
 input or displaying a message. It also contains a file requestor so the user   
 can select a file for loading or saving. There is also a date requestor object 
 These requestors use all the basic controls and if more complexity is needed   
 the application can easily make a copy of any of these and add their own       
 extra functionality to them */

#include "kgui.h"
#include "_text.h"
#include "kguireq.h"

enum
{
TYPE_DRIVE,
TYPE_FOLDER,
TYPE_FILE
};

class kGUIFilenameRow : public kGUIInputBoxObj
{
public:
	kGUIFilenameRow() {}
	~kGUIFilenameRow() {}
	void Set(int type,const char *fn);
	void Draw(void);
private:
	kGUIImage m_icon;
};

void kGUIFilenameRow::Set(int type,const char *fn)
{
	switch(type)
	{
	case TYPE_DRIVE:
		SetFontID(1);	/* bold */
		m_icon.SetFilename("_drive.gif");
	break;
	case TYPE_FOLDER:
		SetFontID(1);	/* bold */
		m_icon.SetFilename("_folder.gif");
	break;
	case TYPE_FILE:
		SetFontID(0);	/* normal */
		m_icon.SetFilename("_file.gif");
	break;
	}
	SetString(fn);
}

void kGUIFilenameRow::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	m_icon.Draw(0,c.lx+2,c.ty+3);

	if(kGUI::GetCurrentObj()==this)
		SetRevRange(0,GetLen());
	else
		SetRevRange(0,0);
	kGUIText::DrawSection(0,GetLen(),c.lx+20,c.lx+20,c.ty+3,GetLineHeight());
}

class kGUIFileReqRow : public kGUITableRowObj
{
public:
	kGUIFileReqRow(int rowheight) {m_objectlist[0]=&m_filename;SetRowHeight(rowheight);}
	int GetNumObjects(void) {return 1;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 
	kGUIObj *m_objectlist[1];
	bool IsDir(void) {return (m_type!=TYPE_FILE);}
	int GetType(void) {return m_type;}
	void Set(int type,const char *n) {m_type=type;m_filename.Set(type,n);}
	kGUIString *GetFilename(void) {return &m_filename;}
	void SetDoubleClick(void *codeobj,void (*code)(void *)) {m_dc.Set(codeobj,code);m_filename.SetEventHandler(this,CALLBACKNAME(FilenameEvent));}
private:
	CALLBACKGLUEPTR(kGUIFileReqRow,FilenameEvent,kGUIEvent)
	void FilenameEvent(kGUIEvent *event) {if(event->GetEvent()==EVENT_LEFTDOUBLECLICK)m_dc.Call();}
	int m_type;
	kGUIFilenameRow m_filename;
	kGUICallBack m_dc;
};

kGUIFileReq::kGUIFileReq(int type,const char *inname,const char *ext,void *codeobj,void (*code)(void *,kGUIFileReq *,int))
{
	int w,h;
	int bw;
	kGUIString in;

	m_pressed=MSGBOX_CANCEL;
	m_type=type;
	if(inname)
		in.SetString(inname);

	kGUI::SplitFilename(&in,&m_path,&m_shortfn);

	if(ext)
		m_ext.SetString(ext);
	m_donecallback.Set(codeobj,code);

	m_controls.SetPos(0,0);
	m_controls.SetMaxWidth(kGUI::GetScreenWidth());

	m_pathlabel.SetFontID(1);
	m_pathlabel.SetString("Look In:");
	m_rowheight=m_pathlabel.GetLineHeight()+8;
	m_controls.AddObject(&m_pathlabel);

	m_path.SetSize(360,m_rowheight);
	m_path.SetEventHandler(this,CALLBACKNAME(PathChangedEvent));
	m_controls.AddObject(&m_path);

	m_backimage.SetFilename("_upfolder.gif");
	m_newimage.SetFilename("_newfolder.gif");

	m_back.SetImage(&m_backimage);
	m_back.SetShowCurrent(false);	/* don't draw border around if current */
	m_back.SetFrame(false);
	m_back.SetSize(30,20);
	m_back.SetEventHandler(this,CALLBACKNAME(PressBack));
	m_controls.AddObject(&m_back);

	m_newfolder.SetImage(&m_newimage);
	m_newfolder.SetShowCurrent(false);	/* don't draw border around if current */
	m_newfolder.SetFrame(false);
	m_newfolder.SetSize(30,20);
	m_newfolder.SetEventHandler(this,CALLBACKNAME(PressNewFolder));
	m_controls.AddObject(&m_newfolder);

	m_controls.NextLine();

	m_table.SetSize(m_controls.GetZoneW(),250);
	m_table.SetNumCols(1);
	m_table.SetColWidth(0,m_controls.GetZoneW()-25);
	m_table.NoColHeaders();
	m_table.NoRowHeaders();
	m_table.NoColScrollbar();
	m_table.SetAllowDelete(false);
	m_controls.AddObject(&m_table);
	m_table.SetEventHandler(this,CALLBACKNAME(CopyFilename));
	m_controls.NextLine();

	m_fnlabel.SetFontID(1);
	m_fnlabel.SetString("Filename:");
	m_controls.AddObject(&m_fnlabel);

	m_shortfn.SetSize(360,m_rowheight);
	m_controls.AddObject(&m_shortfn);
	m_shortfn.SetEventHandler(this,CALLBACKNAME(ShortFnEdited));
	m_controls.NextLine();

	m_done.SetString(kGUI::GetString(KGUISTRING_DONE));
	bw=max(m_done.GetWidth()+16,60);
	m_done.SetSize(bw,m_rowheight);
	m_done.SetPos(m_controls.GetZoneW()-8-bw,0);
	m_done.SetEventHandler(this,CALLBACKNAME(PressDone));

	m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
	bw=max(m_cancel.GetWidth()+16,60);
	m_cancel.SetSize(bw,m_rowheight);
	m_cancel.SetPos(m_done.GetZoneX()-8-bw,0);
	m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));

	m_controls.AddObjects(2,&m_cancel,&m_done);

	m_window.SetInsideSize(m_controls.GetZoneW(),m_controls.GetZoneH());
	m_window.AddObject(&m_controls);

	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.Center();
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);

	PathChanged();

	/* save original window size so we can allow user to make bigger (not smaller) */
	w=m_window.GetZoneW();
	h=m_window.GetZoneH();
	m_origwidth=w;
	m_origheight=h;
	m_lastwidth=w;
	m_lastheight=h;
}

void kGUIFileReq::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::MakeFilename(&m_path,&m_shortfn,&m_longfn);
		kGUI::ReDraw();
		m_donecallback.Call(this,m_pressed);
		delete this;
	break;
	case EVENT_SIZECHANGED:
	{
		int neww,newh;
		int dw,dh;
		bool clipped=false;

		neww=m_window.GetZoneW();
		newh=m_window.GetZoneH();
		
		if(neww<m_origwidth)
		{
			clipped=true;
			neww=m_origwidth;
		}
		if(newh<m_origheight)
		{
			clipped=true;
			newh=m_origheight;
		}

		/* deltas */
		dw=neww-m_lastwidth;
		dh=newh-m_lastheight;

		m_controls.SetZoneW(m_controls.GetZoneW()+dw);
		m_table.SetZoneW(m_table.GetZoneW()+dw);
		m_table.SetColWidth(0,m_table.GetColWidth(0)+dw);
		m_path.SetZoneW(m_path.GetZoneW()+dw);
		m_shortfn.SetZoneW(m_shortfn.GetZoneW()+dw);

		m_back.SetZoneX(m_back.GetZoneX()+dw);
		m_newfolder.SetZoneX(m_newfolder.GetZoneX()+dw);
		m_cancel.SetZoneX(m_cancel.GetZoneX()+dw);
		m_done.SetZoneX(m_done.GetZoneX()+dw);

		m_controls.SetZoneH(m_controls.GetZoneH()+dh);
		m_table.SetZoneH(m_table.GetZoneH()+dh);
		m_fnlabel.SetZoneY(m_fnlabel.GetZoneY()+dh);
		m_shortfn.SetZoneY(m_shortfn.GetZoneY()+dh);
		m_cancel.SetZoneY(m_cancel.GetZoneY()+dh);
		m_done.SetZoneY(m_done.GetZoneY()+dh);

		m_lastwidth=neww;
		m_lastheight=newh;
		if(clipped)
			m_window.SetSize(neww,newh);
	}
	break;
	}
}

void kGUIFileReq::SetFilename(const char *fn)
{
	kGUIString in;

	in.SetString(fn);
	kGUI::SplitFilename(&in,&m_path,&m_shortfn);
}

void kGUIFileReq::CopyFilename(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_MOVED)
	{
		int row;
		kGUIObj *obj;
		kGUIFileReqRow *frr;

		if(m_table.GetNumChildren())
		{
			row=m_table.GetCursorRow();
			obj=m_table.GetChild(0,row);
			frr=static_cast<kGUIFileReqRow *>(obj);

			if(frr->GetType()==TYPE_FILE)
				m_shortfn.SetString(frr->GetFilename());
		}
		else
			m_shortfn.Clear();
	}
}

void kGUIFileReq::DoubleClick(void)
{
	int row;
	kGUIObj *obj;
	kGUIFileReqRow *frr;

	row=m_table.GetCursorRow();
	obj=m_table.GetChild(0,row);
	frr=static_cast<kGUIFileReqRow *>(obj);

	if(frr->IsDir()==true)
	{
		kGUIString newpath;

		kGUI::MakeFilename(&m_path,frr->GetFilename(),&newpath);
		if(newpath.GetLen()>1)
			newpath.Append(DIRCHAR);

		m_path.SetString(&newpath);
		
		PathChanged();
	}
	else
	{
		m_shortfn.SetString(frr->GetFilename());
		kGUI::MakeFilename(&m_path,&m_shortfn,&m_longfn);
		m_pressed=MSGBOX_OK;
		m_window.Close();
	}
}

/* the short filename has been manually edited, check for user moving directories */
void kGUIFileReq::ShortFnEdited(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		if(!strcmp(m_shortfn.GetString(),".."))
		{
			m_shortfn.Clear();
			GoBack();
			return;
		}
		if(strstr(m_shortfn.GetString(),DIRCHAR) )
		{
			/* replace path and re-split */
			m_longfn.SetString(&m_shortfn);
			kGUI::SplitFilename(&m_longfn,&m_path,&m_shortfn);
			PathChanged();
		}
		else
		{
			kGUIString oldpath;

			/* detects directories and splits correctly */
			oldpath.SetString(&m_path);
			kGUI::MakeFilename(&m_path,&m_shortfn,&m_longfn);
			kGUI::SplitFilename(&m_longfn,&m_path,&m_shortfn);
			if(strcmp(oldpath.GetString(),m_path.GetString()))
				PathChanged();
		}
	}
}

void kGUIFileReq::PathChangedEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
		PathChanged();
}

void kGUIFileReq::PathChanged(void)
{
	int i;
	kGUIDir dir;
	kGUIFileReqRow *row;

	m_table.DeleteChildren();
	if(!m_path.GetLen())
		dir.LoadDrives();
	else
		dir.LoadDir(m_path.GetString(),false,false,m_ext.GetString());
	/* directories first */
	for(i=0;i<dir.GetNumDirs();++i)
	{
		row=new kGUIFileReqRow(m_rowheight);
		row->Set(!m_path.GetLen()?TYPE_DRIVE:TYPE_FOLDER,dir.GetDirname(i));
		row->SetDoubleClick(this,CALLBACKNAME(DoubleClick));
		
		m_table.AddRow(row);
	}
	/* files next */
	for(i=0;i<dir.GetNumFiles();++i)
	{
		row=new kGUIFileReqRow(m_rowheight);
		row->Set(TYPE_FILE,dir.GetFilename(i));
		row->SetDoubleClick(this,CALLBACKNAME(DoubleClick));
		m_table.AddRow(row);
	}
}

void kGUIFileReq::PressBack(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		GoBack();
}

void kGUIFileReq::GoBack(void)
{
	int sl;
	char dirc[]={DIRCHAR};

	sl=m_path.GetLen()-1;
#if defined(WIN32) || defined(MINGW)
	if(sl<=2)	/* "c:\" or "d:\" etc.... */
		m_path.Clear();
#elif defined(LINUX) || defined(MACINTOSH)
	if(sl<1)	/* if nothing, then make single DIRCHAR */
		m_path.SetString(DIRCHAR);
#else
#error
#endif
	else
	{
		/* if the last character is a DIRCHAR then ignore it and go back one more */
		if(m_path.GetChar(sl)==dirc[0])
			--sl;

		/* cut off last subdir */
		while(sl>0)
		{
			if(m_path.GetChar(sl)==dirc[0])
				break;
			--sl;
		};
		m_path.Clip(sl);
#if defined(LINUX) || defined(MACINTOSH)
		m_path.Append(DIRCHAR);
#endif
	}
	PathChanged();
}

void kGUIFileReq::PressNewFolder(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		kGUIMsgBoxReq *msg;

		msg=new kGUIMsgBoxReq(MSGBOX_OK,false,"Todo!");
	}
}

void kGUIFileReq::PressCancel(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_pressed=MSGBOX_CANCEL;
		m_window.Close();
	}
}

void kGUIFileReq::PressDone(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		kGUI::MakeFilename(&m_path,&m_shortfn,&m_longfn);

		if(m_longfn.GetLen())
			m_pressed=MSGBOX_OK;
		else
			m_pressed=MSGBOX_CANCEL;
		m_window.Close();
	}
}

