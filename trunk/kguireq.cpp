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
	kGUIText::DrawSection(0,GetLen(),c.lx+20,c.lx+20,c.ty+3,GetHeight());
}

class kGUIFileReqRow : public kGUITableRowObj
{
public:
	kGUIFileReqRow() {m_objectlist[0]=&m_filename;SetRowHeight(20);}
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
	int th;
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
	th=m_pathlabel.GetHeight()+8;
	m_controls.AddObject(&m_pathlabel);

	m_path.SetSize(360,th);
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

	m_table.SetSize(460,250);
	m_table.SetNumCols(1);
	m_table.SetColWidth(0,435);
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

	m_shortfn.SetSize(360,th);
	m_controls.AddObject(&m_shortfn);
	m_shortfn.SetEventHandler(this,CALLBACKNAME(ShortFnEdited));
	m_controls.NextLine();

	m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
	m_cancel.SetSize(60,th);
	m_cancel.SetPos(320,0);
	m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));

	m_done.SetString(kGUI::GetString(KGUISTRING_DONE));
	m_done.SetSize(60,th);
	m_done.SetPos(390,0);
	m_done.SetEventHandler(this,CALLBACKNAME(PressDone));
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

		row=m_table.GetCursorRow();
		obj=m_table.GetChild(0,row);
		frr=static_cast<kGUIFileReqRow *>(obj);

		if(frr->GetType()==TYPE_FILE)
			m_shortfn.SetString(frr->GetFilename());
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
		row=new kGUIFileReqRow();
		row->Set(!m_path.GetLen()?TYPE_DRIVE:TYPE_FOLDER,dir.GetDirname(i));
		row->SetDoubleClick(this,CALLBACKNAME(DoubleClick));
		
		m_table.AddRow(row);
	}
	/* files next */
	for(i=0;i<dir.GetNumFiles();++i)
	{
		row=new kGUIFileReqRow();
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

/***********************************************************************/


typedef struct
{
	int month;	/* 0 - 11 */
	int mday;	/* 1 - 31 or -1 if floating */
	int mweek;	/* if mday is floating, then week 0-4 */
	int wday;	/* if mday is floating then day of week 0-6 */
	const char *name;
	const char *picname;
}DAYLIST_DEF;

DAYLIST_DEF daylist[]={
{ 0,0,-1,-1,"Easter Sunday","easter"},			/* Calculated and filled in */
{ 0,0,-1,-1,"Good Friday (Canada)","goodf"},		/* Calculated and filled in */
{ 0,0,-1,-1,"Easter Monday (Canada)","easter"},		/* Calculated and filled in */
{ 0,0,-1,-1,"Chinese New Year","cnewyear"},		/* Calculated and filled in */
{ 1-1, 1,-1,-1,"New Years Day","newyear"},		/* Jan 1 */
{ 1-1,-1, 2, 1,"Martin Luther King Jr. Day (USA)","king"},	/* 3rd Monday in January */
{ 2-1,-1, 2, 1,"President's Day (USA)","presidents"},		/* 3rd Monday in February */
{ 2-1, 2,-1,-1,"Groundhog Day","groundhog"},		/* Feb 2 */
{ 2-1,14,-1,-1,"Valentine's Day","valentines"},		/* Feb 14 */
{ 2-1,22,-1,-1,"Washington's Birthday (USA)","washington"},	/* Feb 22*/
{ 3-1,17,-1,-1,"St Patrick's Day","stpat"},		/* March 17 */
{ 4-1, 1,-1,-1,"April Fools Day","aprilfools"},		/* Apr 1 */
{ 5-1, 5,-1,-1,"Cinco de Mayo","cinco"},		/* May 5th */
{ 5-1,-2, 0, 1,"Memorial Day (USA)","memorial"},	/* Last Monday in May */
{ 5-1,-2, 1, 1,"Victoria Day (Canada)","victoria"},	/* 2nd to last Monday in May */
{ 5-1,-1, 1, 0,"Mother's Day","mothers"},			/* 2nd Sunday in May */
{ 6-1,-1, 2, 0,"Fathers's Day","fathers"},		/* 3rd Sunday in June */
{ 7-1, 1,-1,-1,"Canada Day","canada"},			/* July 1st */
{ 7-1, 4,-1,-1,"Independence Day (USA)","usa"},		/* July 4th */
{ 9-1,-1, 0, 1,"Labour Day","labour"},			/* 1st Monday in September */
{10-1,31,-1,-1,"Halloween","halloween"},		/* Oct 31 */
{10-1,-1, 1, 1,"Thanksgiving (Canada) / Columbus Day (USA)","thanksgiving"}, /* 2nd Monday in October */
{11-1,-1, 3, 4,"Thanksgiving (USA)","thanksgiving"}, 	/* 4th Thursday in November */
{11-1,11,-1,-1,"Rememberance Day / Veteran's Day","rememberance"},	/* November 11th */
{12-1,24,-1,-1,"Christmas Eve","cheve"},			/* Dec 24 */
{12-1,25,-1,-1,"Christmas Day","tree"},			/* Dec 25 */
{12-1,26,-1,-1,"Boxing Day","boxday"},			/* Dec 26 */
{12-1,31,-1,-1,"New Years Eve","newyearseve"}};		/* Dec 31 */

typedef struct
{
	int month,day,year;
	const char *animal;
}DCONST_DEF;

DCONST_DEF chinesenewyears[]={
	{1,29,2006,"Dog"},
	{2,18,2007,"Pig"},
	{2,7,2008,"Rat"},
	{1,26,2009,"Ox"},
	{2,14,2010,"Tiger"},
	{2,3,2011,"Rabbit"},
	{1,23,2012,"Dragon"},
	{2,10,2013,"Snake"},
	{1,31,2014,"Horse"},
	{2,19,2015,"Goat"},
	{2,8,2016,"Monkey"},
	{1,28,2017,"Rooster"},
	{2,16,2018,"Dog"},
	{2,5,2019,"Pig"},
	{1,25,2020,"Rat"}};


/* called by set whenever the month changes */
void kGUIDateGridObj::GetHolidayString(kGUIDate *date,kGUIString *s)
{
	unsigned int i;
	int c,y,n,k,j,l,m,w;
	DAYLIST_DEF *dl;
	int istoday;	
	kGUIDate d;
	time_t ntime;
	struct tm *tinfo;
	struct tm ti;

	s->Clear();
	d.SetToday();
	if((date->GetDay()==d.GetDay()) && (date->GetMonth()==d.GetMonth()) && (date->GetYear()==d.GetYear()))
		s->SetString(kGUI::GetString(KGUISTRING_TODAY));	/* todo, add to button string list for translation */

	/* convert d,m,y to numeric value */
	memset(&ti,0,sizeof(ti));
	ti.tm_mday=date->GetDay();
	ti.tm_mon=date->GetMonth()-1;
	ti.tm_year=date->GetYear()-1900;
	ntime=mktime(&ti);

	tinfo=localtime( &ntime );	/* convert back to d/m/y */
	ti=*(tinfo);

	y=date->GetYear();
	c = y / 100;
	n = y - 19 * ( y / 19 );
	k = ( c - 17 ) / 25;
	i = c - c / 4 - ( c - k ) / 3 + 19 * n + 15;
	i = i - 30 * ( i / 30 );
	i = i - ( i / 28 ) * ( 1 - ( i / 28 ) * ( 29 / ( i + 1 ) ) * ( ( 21 - n ) / 11 ) );
   	j = y + y / 4 + i + 2 - c + c / 4;
   	j = j - 7 * ( j / 7 );
   	l = i - j;

   	m = (3 + ( l + 40 ) / 44);
   	daylist[0].month = m-1;		/* Easter Sunday */
   	daylist[0].mday=l + 28 - 31 * ( m / 4 );

   	d.SetMonth(m);
	d.SetYear(date->GetYear());
	d.SetDay(daylist[0].mday);

	/* good friday */
	d.AddDays(-2);
	daylist[1].month = d.GetMonth()-1;
	daylist[1].mday= d.GetDay();

	/* easter monday */
	d.AddDays(3);
	daylist[2].month = d.GetMonth()-1;
	daylist[2].mday= d.GetDay();

	/* calculate chinese new year for this year */

	for(i=0;i<sizeof(chinesenewyears)/sizeof(DCONST_DEF );++i)
	{
		if(chinesenewyears[i].year==y)
		{
			daylist[3].month=chinesenewyears[i].month-1;
			daylist[3].mday=chinesenewyears[i].day;
			m_ccuryear.Sprintf("Chinese New Year, Year of the %s",chinesenewyears[i].animal);
			daylist[3].name=m_ccuryear.GetString();
			break;
		}
	}

	dl=daylist;
	for(i=0;i<sizeof(daylist)/sizeof(DAYLIST_DEF);++i)
	{
		istoday=0;
		if(dl->month==ti.tm_mon)
		{
			if(dl->mday>=0)	/* fixed day not floating */
			{
				if(dl->mday==ti.tm_mday)
					istoday=1;
			}
			else if(dl->mday==-1)	/* is floating, weekday day of nth week */
			{
				if((dl->wday==ti.tm_wday) )
				{
					w=(ti.tm_mday-1)/7;
					if(dl->mweek==w)
						istoday=1;
				}
			}
			else /* is floating, weekday day of nth week ( from end of month back ) */
			{
				if((dl->wday==ti.tm_wday) && (dl->month==ti.tm_mon) )
				{
					w=(31-ti.tm_mday)/7;
					if(dl->mweek==w)
						istoday=1;
				}
			}
		}
		
		if(istoday)
		{
			if(s->GetLen())
				s->Append(", ");
			s->Append(dl->name);
		}
		++dl;
	}
}

void kGUIDateGridObj::Set(int vm,int vy,int d,int m,int y)
{
	kGUIDate dd;
	int c;
	int w;

	m_curday=d;
	m_curmonth=m;
	m_curyear=y;

	m_viewmonth=vm;

	dd.SetDay(1);
	dd.SetMonth(vm);
	dd.SetYear(vy);

	w=dd.GetDayofWeek();
	if(!w)
		w=7;
	dd.AddDays(-w);
	for(c=0;c<(6*7);++c)
	{
		m_day[c]=dd.GetDay();
		m_month[c]=dd.GetMonth();
		m_year[c]=dd.GetYear();
		GetHolidayString(&dd,&m_holidays[c]);
		if(m_validdaycallback.IsValid())
			m_validdaycallback.Call(&dd,&m_enable[c]);
		else
			m_enable[c]=true;

		dd.AddDays(1);
	}
	Dirty();
}

void kGUIDateGridObj::Draw(void)
{
	int r,y,i;
	int x1,x2,y1,y2;
	double px,py;
	kGUICorners cc;
	kGUIColor c;

	GetCorners(&cc);
	px=(cc.rx-cc.lx)/7.0f;
	py=(cc.by-cc.ty)/7.0f;

	m_text.SetHAlign(FT_CENTER);
	m_text.SetVAlign(FT_MIDDLE);
	m_text.SetColor(DrawColor(0,0,0));

	/* draw header */
	for(r=0;r<7;++r)
	{
		x1=cc.lx+(int)(px*r);
		y1=cc.ty;
		x2=cc.lx+(int)(px*(r+1));
		y2=cc.ty+(int)py;
		kGUI::DrawRectFrame(x1,y1,x2,y2,DrawColor(150,150,255),DrawColor(0,0,0));
		m_text.SetString(kGUIDate::GetWeekDay3(r));
		m_text.Draw(x1,y1,x2-x1,y2-y1);
	}

	/* draw days */
	i=0;
	for(y=0;y<6;++y)
	{
		y1=cc.ty+(int)((y+1)*py);
		y2=cc.ty+(int)((y+2)*py);
		for(r=0;r<7;++r)
		{
			x1=cc.lx+(int)(px*r);
			x2=cc.lx+(int)(px*(r+1));
			if((m_month[i]==m_curmonth) && (m_day[i]==m_curday) && (m_year[i]==m_curyear))
				c=DrawColor(192,150,150);
			else if(m_month[i]==m_viewmonth)
			{
				if(m_holidays[i].GetLen())
					c=DrawColor(220,255,220);
				else
					c=DrawColor(220,220,220);
			}
			else
				c=DrawColor(96,96,96);

			kGUI::DrawRectFrame(x1,y1,x2,y2,c,DrawColor(0,0,0));
			m_text.Sprintf("%d",m_day[i]);
			m_text.Draw(x1,y1,x2-x1,y2-y1);
			if(m_enable[i]==false)
			{
				kGUI::DrawLine(x1,y1,x2,y2,DrawColor(255,0,0));
				kGUI::DrawLine(x2,y1,x1,y2,DrawColor(255,0,0));
			}
			++i;
		}
	}
}

bool kGUIDateGridObj::UpdateInput(void)
{
	kGUICorners c;
	double px,py;
	int d;

	GetCorners(&c);
	if(kGUI::MouseOver(&c)==true)
	{
		px=(c.rx-c.lx)/7.0f;
		py=(c.by-c.ty)/7.0f;

		d=(int)((kGUI::GetMouseY()-c.ty)/py);
		if(d>0)
		{
			d=(d-1)*7;
			d+=(int)((kGUI::GetMouseX()-c.lx)/px);
			if(d>=0 && d<(6*7))
			{
				if(m_enable[d]==true)
				{
					if(kGUI::GetMouseDoubleClickLeft()==true || kGUI::GetMouseClickLeft()==true)
					{
						kGUIDate cd;
						bool click;

						click=kGUI::GetMouseDoubleClickLeft();

						cd.Set(m_day[d],m_month[d],m_year[d]);
						m_clickcallback.Call(&cd,&click);
						return(true);
					}
				}
				kGUI::SetHintString(kGUI::GetMouseX(),kGUI::GetMouseY()+20,m_holidays[d].GetString());
			}
		}
		return(true);
	}
	return(false);
}

/**************************************************************************/

/* todo: allow limits, like no past dates or no future dates etc */

kGUIDateReq::kGUIDateReq(kGUIDate *date,void *codeobj,void (*code)(void *,kGUIDate *date))
{
	int y;

	/* save pointer to user supplied date so it can be updated when done */

	/* get default date from user supplied date */
	m_date.SetDay(date->GetDay());
	m_date.SetMonth(date->GetMonth());
	m_date.SetYear(date->GetYear());

	m_selector.SetClickCallback(this,CALLBACKNAME(Clicked));
	m_donecallback.Set(codeobj,code);

	m_selector.SetPos(0,0);
	m_window.AddObject(&m_selector);

	y=m_selector.GetZoneY()+m_selector.GetZoneH()+10;
	m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
	m_cancel.SetSize(60,20);
	m_cancel.SetPos(260,y);
	m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));
	m_window.AddObject(&m_cancel);

	m_done.SetString(kGUI::GetString(KGUISTRING_OK));
	m_done.SetSize(60,20);
	m_done.SetPos(260+20+60,y);
	m_done.SetEventHandler(this,CALLBACKNAME(PressDone));
	m_window.AddObject(&m_done);

	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetSize(100,100);
	m_window.ExpandToFit();
	m_window.Center();
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);
}

void kGUIDateReq::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::ReDraw();
		m_donecallback.Call(&m_date);
		delete this;
	}
}

void kGUIDateReq::Clicked(kGUIDate *date,bool *doubleclicked)
{
	m_date.Copy(date);
	m_date.LongDate(m_window.GetTitle());

	if(doubleclicked[0]==true)
		m_window.Close();
}

void kGUIDateReq::PressCancel(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		m_window.Close();
}

void kGUIDateReq::PressDone(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		m_window.Close();
}

/******************************************************************************************/

kGUIDateSelectorObj::kGUIDateSelectorObj()
{
	kGUIDate today;

	SetSize(400,300);
	SetNumGroups(1);
	today.SetToday();

	/* get default date from user supplied date */
	m_date.SetDay(today.GetDay());
	m_date.SetMonth(today.GetMonth());
	m_date.SetYear(today.GetYear());

	m_viewmonth=today.GetMonth();
	m_viewyear=today.GetYear();

	m_vmonth.SetPos((GetZoneW()-130)>>1,10);
	m_vmonth.SetSize(130,20);
	m_vmonth.SetEventHandler(this,CALLBACKNAME(ChangeMonth));

	m_vyear.SetPos((GetZoneW()-130)>>1,40);
	m_vyear.SetSize(130,20);
	m_vyear.SetEventHandler(this,CALLBACKNAME(ChangeYear));

	AddObject(&m_vmonth);
	AddObject(&m_vyear);

	/* position day buttons */
	m_grid.SetPos(0,80);
	m_grid.SetSize(GetChildZoneW(),GetChildZoneH()-80);
	AddObject(&m_grid);
	m_grid.SetClickCallback(this,CALLBACKNAME(Clicked));
	DayChanged();
}

void kGUIDateSelectorObj::Clicked(kGUIDate *date,bool *doubleclicked)
{
	m_date.Copy(date);

	m_viewmonth=date->GetMonth();
	m_viewyear=date->GetYear();
	DayChanged();
	m_clickcallback.Call(date,doubleclicked);
}


void kGUIDateSelectorObj::ChangeMonth(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_viewmonth+=event->m_value[0].i;
		if(m_viewmonth==13)
		{
			m_viewmonth=1;
			++m_viewyear;
		}
		else if(m_viewmonth==0)
		{
			m_viewmonth=12;
			--m_viewyear;
		}
		DayChanged();
	}
}

void kGUIDateSelectorObj::ChangeYear(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_viewyear+=event->m_value[0].i;
		DayChanged();
	}
}

void kGUIDateSelectorObj::DayChanged(void)
{
	kGUIString title;

	m_grid.Set(m_viewmonth,m_viewyear,m_date.GetDay(),m_date.GetMonth(),m_date.GetYear());

	m_vmonth.SetString(kGUIDate::GetMonthName(m_viewmonth));
	m_vyear.Sprintf("%d",m_viewyear);

	m_date.LongDate(&title);
//	m_window.SetTitle(title.GetString());
}
