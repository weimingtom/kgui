/**********************************************************************************/
/* kGUI - kgui.cpp                                                                */
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

/*! @file kgui.cpp 
   @brief This is the main static framework class for kGUI.
   It contains most of the draw functions, mouse functions, root control for      
   rendering objects and passing input to objects */

/*! @mainpage kGUI
  * kGUI is a cross platform framework for writing c++ applications.
  * It is designed so that your application only needs to referece the kGUI
  * classes and never needs to reference any system calls directly.
  * All of the system differences are handled by the kGUI code.
  * 
  * It consists of a GUI for screen rendering, a report generator for printing
  * and many other objects for things like threading, downloading webpages,
  * viewing webpages,database access etc.
  * 
  * It can be built with Visual Studio, or using a makefile for Linux, Macintosh
  * or using MSYS / MinGW on Windows.
  * 
  * Building: view the 'readme' file for building instructions
  * 
  * kGUI uses the following external libraries (that are included in it's source)
  * FreeType - a high quality font decoding engine
  * Jpeg - jpeg image decompression / compression library
  * LibPNG - png image decompression / compression library
  * Zlib - generic compression / decompression library
  * Cups - common unix printing system ( for Mac and Linux only, no included) */

#include "kgui.h"
#include "kguiprot.h"
#include <math.h>
#include <time.h>

#define HINTTEXTSIZE 9

/* since these are static, they need to be defined specifically */

kGUISystem *kGUI::m_sys;
kGUIWindowObj *kGUI::m_backgroundobj;
kGUIRootObj *kGUI::m_rootobj;
int kGUI::m_mousecursor;
int kGUI::m_tempmouse;
int kGUI::m_mousex;
int kGUI::m_mousey;
int kGUI::m_mousewheeldelta;
bool kGUI::m_nomouse;
bool kGUI::m_mouseleft;
bool kGUI::m_mouseright;
bool kGUI::m_xmouseleft;
bool kGUI::m_xmouseright;
bool kGUI::m_mouseleftdoubleclick;
bool kGUI::m_mouserightdoubleclick;
kGUIMutex kGUI::m_busymutex;
bool kGUI::m_allowdraw;
bool kGUI::m_trace;

/* this is the full screen size */
int kGUI::m_fullscreenwidth;
int kGUI::m_fullscreenheight;
/* this is the screen size not counting the windows header and footer on the top/bottom and sides */
int kGUI::m_screenwidth;
int kGUI::m_screenheight;

kGUIDelay kGUI::m_mouserightrelesetick;
kGUIDelay kGUI::m_mouseleftrelesetick;

int kGUI::m_lmousex;
int kGUI::m_lmousey;
int kGUI::m_dmousex;
int kGUI::m_dmousey;
bool kGUI::m_lmouseleft;
bool kGUI::m_lmouseright;

int kGUI::m_activeindex;
kGUIObj *kGUI::m_activeobjstack[MAXACTIVE];

kGUIObj *kGUI::m_activeobj;
kGUIObj *kGUI::m_forcecurrentobj;

bool kGUI::m_closeapp;

/* current number of dirty corners */
int kGUI::m_dirtyindex;
kGUICorners kGUI::m_dirtycorners[MAXDIRTY];

kGUICorners kGUI::m_clipcorners;
int kGUI::m_clipindex;
kGUICorners kGUI::m_clipstack[MAXCLIPS];
int kGUI::m_numevents;
Array<kGUICallBack *>kGUI::m_events;

kGUIDelay kGUI::m_flash;
int kGUI::m_et;
int kGUI::m_frame;

int kGUI::m_numkeys;		/* number of keys in input buffer */
Array<int>kGUI::m_keys;		/* array of keys in the input buffer */

bool kGUI::m_keyshift;
bool kGUI::m_keycontrol;

bool kGUI::m_inputidle;
int kGUI::m_hinttick;
bool kGUI::m_sethint;			/* flag is only valid to sell object to set hint */
bool kGUI::m_askhint;
kGUIText kGUI::m_hinttext;
kGUICorners kGUI::m_hintcorners;	/* valid only when hint on screen */

bool kGUI::m_drawcursor;
bool kGUI::m_drawcursorchanged;
int kGUI::m_deffontid=0;
int kGUI::m_deffontsize=16;
int kGUI::m_defreportfontid=0;
int kGUI::m_defreportfontsize=16;
char *kGUI::m_imagesizefilename;
Hash kGUI::m_imagesizehash;

kGUIDrawSurface kGUI::m_screensurface;
kGUIDrawSurface *kGUI::m_currentsurface;

Array<kGUIPoint2>kGUI::m_fatpoints;
Array<kGUIDPoint2>kGUI::m_dfatpoints;
kGUISubPixelCollector kGUI::m_subpixcollector;

kGUICallBack kGUI::m_inputcallback;
kGUISkin *kGUI::m_skin;
kGUIRandom *kGUI::m_random;

kGUICookieJar *kGUI::m_cookiejar;

bool kGUI::m_fastdraw;

kGUIString kGUI::m_strings[KGUISTRING_NUMSTRINGS];

void kGUICallBack::Set(void *o,void (*f)(void *))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

void kGUICallBackInt::Set(void *o,void (*f)(void *,int))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

void kGUICallBackIntInt::Set(void *o,void (*f)(void *,int,int))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}



void kGUIDrawSurface::Clear(kGUIColor c)
{
	int numpixels;
	kGUIColor *pp;

	numpixels=m_width*m_height;
	pp=GetSurfacePtr(0,0);
	while(numpixels)
	{
		*(pp++)=c;
		--numpixels;
	}
}

void kGUIDrawSurface::UnRotateSurface(kGUIDrawSurface *ss)
{
	int dx,dy;
	int sx,sy;
	int sw,sh;
	kGUIColor swhite;
	kGUIColor *dp;
	kGUIColor *sp;

	swhite=DrawColor(255,255,255);
	sw=ss->GetWidth();
	sh=ss->GetHeight();

	ss->SetOffsets(0,0);
	for(dy=0;dy<m_height;++dy)
	{
		dp=GetSurfacePtr(0,dy);
		for(dx=0;dx<m_width;++dx)
		{
			sy=dx;
			sx=m_height-dy;
			if((sx<sw) && (sy<sh))
				sp=ss->GetSurfacePtr(sx,sy);
			else
				sp=&swhite;
			*(dp++)=*(sp);
		}
	}
}

/* extract a section of the surface, and rotate it, used for landscape printing  */

void kGUIDrawSurface::UnRotateSurface(kGUIDrawSurface *ss,int lx,int rx,int ty,int by)
{
	int dx,dy;
	int sx,sy;
	kGUIColor swhite;
	kGUIColor *dp;
	kGUIColor *sp;

	swhite=DrawColor(255,255,255);

	dx=0;
	for(sy=ty;sy<by;++sy)
	{
		dy=0;
		sp=ss->GetSurfacePtrABS(lx,sy);
		for(sx=lx;sx<rx;++sx)
		{
			dp=GetSurfacePtrABS(dx,dy);
			*(dp)=*(sp++);
			++dy;
		}
		++dx;
	}
}

/* count up */
void kGUIDelay::Update(void)
{
	m_left+=kGUI::GetET();
}

/* count down */
bool kGUIDelay::Update(int delaytime)
{
	m_left-=kGUI::GetET();
	if(m_left<0)
	{
		m_left+=delaytime;
		return(true);
	}
	return(false);
}

kGUI::kGUI()
{
}

kGUIRootObj::kGUIRootObj()
{
	SetNumGroups(1);	/* only 1 container list */
}

void kGUIRootObj::CalcChildZone(void)
{
	SetChildZone(GetZoneX(),GetZoneY(),GetZoneW(),GetZoneH());
}

bool kGUIRootObj::UpdateInput(void)
{
	/* update all children */
	return(UpdateInputC(0));
}

void kGUIRootObj::Draw(void)
{
	DrawC(0);
}

/* close all children */
void kGUIRootObj::Close(void)
{
	kGUIObj *gobj;

	while(GetNumChildren())
	{
		gobj=GetChild(0);
		static_cast<kGUIContainerObj *>(gobj)->Close();
	}
}

void kGUI::Close(void)
{
	assert(m_inputcallback.IsValid()==false,"User Input callback should be disabled first!");

	m_numevents=0;	/* stop calling events */
	m_rootobj->Close();
	delete m_backgroundobj;
	delete m_rootobj;

	delete m_skin;	/* delete the skin */
	delete m_random;

	CloseFontEngine();
	DataHandle::PurgeStatic();
}

void kGUI::Trace(const char *message,...)
{
	kGUIString fmessage;
	FILE *f;
	va_list args;

    va_start(args, message);
	fmessage.AVSprintf(message,args);
    va_end(args);
	f=fopen("kgui.log","a");
	if(f)
	{
		fprintf(f,"%s",fmessage.GetString());
		fclose(f);
	}
}

kGUISkin *AllocDefSkin(void);

bool kGUI::Init(kGUISystem *sys,int width,int height,int fullwidth,int fullheight,int maximages)
{
	m_sys=sys;

	DataHandle::InitStatic();

	Trace(" kGUI::Init(%d,%d,%d,%d)\n",width,height,fullwidth,fullheight);
	Trace(" Calling InitFontEngine()\n");
	if(InitFontEngine()==false)
	{
		Trace(" InitFontEngine() - Init failure, aborting!\n");
		return(false);
	}
	m_frame=0;
	m_trace=false;		/* trace mode, triggers internal code to trace settings */
	m_fastdraw=false;
	m_allowdraw=false;
	m_mousecursor=MOUSECURSOR_DEFAULT;
	m_tempmouse=false;
	m_closeapp=false;
	m_numkeys=0;
	m_keys.Init(32,4);
	m_keyshift=false;

	/* set default text to english, allow it to be overwritten by user code */
	SetLanguage(KGUILANG_ENGLISH);

	/* full window area */
	m_fullscreenwidth=fullwidth;
	m_fullscreenheight=fullheight;

	/* windows "child" area which is essentially the full area minus the frame area on the top/bottom and sides */
	m_screenwidth=width;
	m_screenheight=height;

	m_imagesizefilename=0;
	m_imagesizehash.Init(16,sizeof(kGUIImageSizeCache));
	kGUIImage::InitCache(maximages);

	/* set the default skin render handler, since it can load graphics, the imagesize cache code (above) needs to be initlized first */
	m_skin=AllocDefSkin();
	m_random=new kGUIRandom();
	{
		int i,v;
		kGUIDate date;
		kGUIString ds;

		/* generate a pseudo random seed by using todays date and current time */
		date.SetToday();
		for(i=0;i<25;++i)
		{
			date.LongDate(&ds);
			m_random->AddEntropy(ds.GetString(),ds.GetLen());
			date.Time(&ds);
			m_random->AddEntropy(ds.GetString(),ds.GetLen());
			m_random->ExtractEntropy((char *)&v,sizeof(v));
		}
	}

	m_screensurface.Init(fullwidth,fullheight);
	m_currentsurface=&m_screensurface;

	m_activeindex=0;
	m_activeobj=0;
	m_forcecurrentobj=0;

	m_dirtyindex=1;
	m_dirtycorners[0].lx=0;
	m_dirtycorners[0].ty=0;
	m_dirtycorners[0].rx=fullwidth;
	m_dirtycorners[0].by=fullheight;

	m_clipindex=0;
	m_inputidle=true;
	
	m_numevents=0;
	m_events.Alloc(100);
	m_events.SetGrowSize(25);
	m_events.SetGrow(true);

	m_hinttick=0;
	m_sethint=false;
	m_askhint=false;
	m_hinttext.SetFontSize(HINTTEXTSIZE);
	m_hinttext.SetColor(DrawColor(0,0,0));

	m_hintcorners.lx=0;
	m_hintcorners.rx=0;
	m_hintcorners.ty=0;
	m_hintcorners.by=0;

	m_drawcursor=false;
	m_drawcursorchanged=false;
	m_deffontsize=16;

	Trace(" Allocating Root Render Objects()\n");
	m_rootobj=new kGUIRootObj;
	
	m_backgroundobj=new kGUIWindowObj();
	m_backgroundobj->SetBackground(true);
	m_backgroundobj->SetAllowButtons(WINDOWBUTTON_CLOSE|WINDOWBUTTON_MINIMIZE);	/* no full */

	m_rootobj->SetPos(0,0);
	m_rootobj->SetSize(fullwidth,fullheight);

	m_backgroundobj->SetPos((fullwidth-width)/2,(fullheight-height)/2);
	m_backgroundobj->SetSize(width,height);
	m_rootobj->AddObject(m_backgroundobj);

	Trace(" Sucessful kGUI Initialization\n");
	return(true);
}

void kGUI::SetLanguage(unsigned int lang)
{
	switch(lang)
	{
	case KGUILANG_ENGLISH:
		/* these are not used in kgui but supplied so the user app can ( if desired ) */
		/* have a combo box so the user can select the desired language */
		SetString(KGUISTRING_LANGUAGE,"Language");
		SetString(KGUISTRING_ENGLISH,"English");
		SetString(KGUISTRING_FRENCH,"French");

		SetString(KGUISTRING_OK,"OK");
		SetString(KGUISTRING_CANCEL,"Cancel");
		SetString(KGUISTRING_YES,"Yes");
		SetString(KGUISTRING_NO,"No");
		SetString(KGUISTRING_DONE,"Done");
		SetString(KGUISTRING_TODAY,"Today");

		/* these are used on the print-preview panel */
		SetString(KGUISTRING_PRINTPREVIEW,"Print Preview");
		SetString(KGUISTRING_PRINT,"Print");
		SetString(KGUISTRING_CURRENT,"Current");
		SetString(KGUISTRING_PAGE,"Page");
		SetString(KGUISTRING_ZOOM,"Zoom");
		SetString(KGUISTRING_PORTRAIT,"Portrait");
		SetString(KGUISTRING_LANDSCAPE,"Landscape");
		SetString(KGUISTRING_SCALEPERCENT,"Scale %");
		SetString(KGUISTRING_MULTIPAGE,"Multi Page");
		SetString(KGUISTRING_PRINTRANGE,"Print Range");
		SetString(KGUISTRING_COPIES,"Copies");

		/* todo: months jan-dec etc can also be added... */
	break;
	case KGUILANG_FRENCH:
		/* these are not used in kgui but supplied so the user app can ( if desired ) */
		/* have a combo box so the user can select the desired language */
		SetString(KGUISTRING_LANGUAGE,"Langue");
		SetString(KGUISTRING_ENGLISH,"Anglais");
		SetString(KGUISTRING_FRENCH,"Français");

		SetString(KGUISTRING_OK,"xxx OK");
		SetString(KGUISTRING_CANCEL,"xxx Cancel");
		SetString(KGUISTRING_YES,"Oui");
		SetString(KGUISTRING_NO,"Non");
		SetString(KGUISTRING_DONE,"xxx Done");
		SetString(KGUISTRING_TODAY,"xxx Today");

		/* these are used on the print-preview panel */
		SetString(KGUISTRING_PRINTPREVIEW,"Imprimer Preview");
		SetString(KGUISTRING_PRINT,"Imprimer");
		SetString(KGUISTRING_CURRENT,"Current");
		SetString(KGUISTRING_PAGE,"Page");
		SetString(KGUISTRING_ZOOM,"Zoom");
		SetString(KGUISTRING_PORTRAIT,"Portrait");
		SetString(KGUISTRING_LANDSCAPE,"Landscape");
		SetString(KGUISTRING_SCALEPERCENT,"Scale %");
		SetString(KGUISTRING_MULTIPAGE,"Multi Page");
		SetString(KGUISTRING_PRINTRANGE,"Imprimer Range");
		SetString(KGUISTRING_COPIES,"Copies");
	break;
	default:
		assert(false,"Unknown language!");
	break;
	}
}

void kGUIPrinter::GetInfo(int *ppw,int *pph,int *ppih,int *ppiv)
{
	if(m_gotinfo==false)
	{
		kGUI::GetPrinterInfo(m_name.GetString(),&m_pagewidth,&m_pageheight,&m_ppihoriz,&m_ppivert);
		m_gotinfo=true;
	}
	ppw[0]=m_pagewidth;
	pph[0]=m_pageheight;
	ppih[0]=m_ppihoriz;
	ppiv[0]=m_ppivert;
};

/* return index into printer by name */
/* if not found, then return -1 or default */

int kGUI::LocatePrinter(const char *name,bool ordefault)
{
	int i,np;

	np=kGUI::GetNumPrinters();
	for(i=0;i<np;++i)
	{
		if(!strcmp(kGUI::GetPrinterObj(i)->GetName(),name))
			return(i);
	}
	if(ordefault)
		return(kGUI::GetDefPrinterNum());
	return(-1);	/* printer not found */
}

void kGUI::PushActiveObj(kGUIObj *obj)
{
	assert(m_activeindex<MAXACTIVE,"ActiveObj stack overflow");
	if(m_activeindex)
	{
		if(m_activeobjstack[m_activeindex-1]==obj)
		{
			m_activeobj=obj;
			return;
		}
	}

	/* save previous active object */
	m_activeobjstack[m_activeindex++]=m_activeobj;
	m_activeobj=obj;
}

void kGUI::PopActiveObj(void)
{
	assert(m_activeindex>0,"ActiveObj stack underflow");
	m_activeobj=m_activeobjstack[--m_activeindex];
}

/* start at the root objects and traverse the "current" objects until we */
/* can't go any deeper, then this is the current object so return it. */

kGUIObj *kGUI::GetCurrentObj(void)
{
	kGUIObj *gobj;
	kGUIContainerObj *gcobj;

	gobj=m_rootobj;
	while(gobj->IsContainer())
	{
		gcobj=static_cast<kGUIContainerObj *>(gobj);
		gobj=gcobj->GetCurrentChild();
		if(!gobj)
			break;
	}
	return(gobj);
}

void kGUI::Tick(int et)
{
	m_et=et;

	/* if no mouse or keyboard input is detected for some time then turn */
	/* on the show hint flag, with the flag on, any item under the cursor will */
	/* draw a hint box on the screen giving the user a description of the button */
	/* etc, as soon as any input is made, the hint disappears */

	if(m_inputidle==true && m_hinttick<SHOWHINTTICK)
	{
		m_hinttick+=m_et;
		if(m_hinttick>=SHOWHINTTICK)
			m_sethint=true;
	}
}

void kGUI::SetHintString(int x,int y,const char *hint)
{
	int off;

	m_hinttext.SetString(hint);
	if(m_hinttext.GetLen())
	{
		if(x<0)
			x=0;
		if(y<0)
			y=0;
		m_hintcorners.lx=x;
		m_hintcorners.ty=y;
		m_hintcorners.rx=m_hintcorners.lx+m_hinttext.GetWidth()+8;
		m_hintcorners.by=m_hintcorners.ty+m_hinttext.GetHeight()+8;

		if(m_hintcorners.rx>GetSurfaceWidth())
		{
			off=m_hintcorners.rx-GetSurfaceWidth();
			m_hintcorners.lx-=off;
			m_hintcorners.rx-=off;
		}
		if(m_hintcorners.by>GetSurfaceHeight())
		{
			off=m_hintcorners.by-GetSurfaceHeight();
			m_hintcorners.by-=off;
			m_hintcorners.ty-=off;
		}
		Dirty(&m_hintcorners);
	}
	m_sethint=false;
}

/* add window to top of linked list */

void kGUI::AddWindow(kGUIObj *window)
{
	m_rootobj->AddObject(window);
	m_rootobj->SetCurrentChild(window);

	/* is the current child a skiptab object? ( static text or disabled button ) */
	if(window->IsContainer())
	{
		kGUIContainerObj *cobj=static_cast<kGUIContainerObj *>(window);
		kGUIObj *gobj;

		gobj=cobj->GetCurrentChild();
		if(gobj)
		{
			if(gobj->SkipTab()==true)
			{
				if(cobj->Tab(1)==false)
					cobj->SetCurrentChild(0);	/* no current child! */
			}
		}
	}

	/* todo: have an active stack for each window */

	/* clear the active object stack */
	m_activeobj=0;
	m_activeindex=0;
	window->Dirty();
}

void kGUI::DelWindow(kGUIObj *window)
{
	kGUIObj *oldtopobj;
	kGUIObj *newtopobj;

	oldtopobj=kGUI::GetCurrentObj();

	window->Dirty();

	if(window->GetParent())
	{
		/* remove me from my parents list of children */
		window->GetParent()->DelObject(window);
	}

	/* clear the active object stack */
	m_activeobj=0;
	m_activeindex=0;

	newtopobj=kGUI::GetCurrentObj();
	if(oldtopobj!=newtopobj)
	{
		if(newtopobj)
			newtopobj->Dirty();
	}
}

/* return true if I am the top window */
bool kGUI::AmITheTopWindow(kGUIObj *window)
{
	if(static_cast<kGUIContainerObj *>(m_rootobj->GetChild(m_rootobj->GetNumChildren()-1))==window)
		return(true);
	return(false);
}

bool kGUI::StayTopWindow(void)
{
	/* if current top object is told to stay on top then return true */
	if(static_cast<kGUIContainerObj *>(m_rootobj->GetChild(m_rootobj->GetNumChildren()-1))->GetTop()==true)
		return(true);
	return(false);
}

void kGUI::TopWindow(kGUIObj *window)
{
	/* if I am not the last one in the list, then remove me from the */
	/* list and add me to the end again */

	DelWindow(window);
	AddWindow(window);
}

void kGUI::DrawRect(int x1,int y1,int x2,int y2,kGUIColor color)
{
	int w,h,n;
	kGUIColor *sp;
	int skip;
	int offx,offy;

	if(OffClip(x1,y1,x2,y2)==true)
		return;

	if(x1<m_clipcorners.lx)
		x1=m_clipcorners.lx;
	if(x2>m_clipcorners.rx)
		x2=m_clipcorners.rx;

	if(y1<m_clipcorners.ty)
		y1=m_clipcorners.ty;
	if(y2>m_clipcorners.by)
		y2=m_clipcorners.by;

	w=x2-x1;
	if(w<0)
		return;

	h=y2-y1;
	if(h<0)
		return;

	sp=GetSurfacePtr(x1,y1);
	skip=GetSurfaceWidth()-w;
	while(h)
	{
		n=w;
		while(n--)
			*(sp++)=color;
		sp+=skip;
		--h;
	}

	/* send to printer? */
	if(GetPrintJob())
	{
		offx=GetCurrentSurface()->GetOffsetX();
		offy=GetCurrentSurface()->GetOffsetY();

		GetPrintJob()->DrawRect(x1+offx,x2+offx,y1+offy,y2+offy,color);
	}
}

/* draw rect with alpha blending */
void kGUI::DrawCircle(int x,int y,int r,kGUIColor color,double alpha)
{
	int i;
	kGUIPoint2 points[360+1];

	if(OffClip(x-r,y-r,x+r,y+r)==true)
		return;

	for(i=0;i<=360;++i)
	{
		points[i].x=x+(int)(r*sin(i*(3.141592654f/180.0f)));
		points[i].y=y+(int)(r*cos(i*(3.141592654f/180.0f)));
	}

	if(alpha==1.0f)
		kGUI::DrawPoly(360+1,points,color);
	else
		kGUI::DrawPoly(360+1,points,color,alpha);
}

/* draw rect with alpha blending */
void kGUI::DrawCircleOutline(int x,int y,int r,int thickness,kGUIColor color,double alpha)
{
	int i,r2;
	kGUIPoint2 points[360+1+360+1];

	if(OffClip(x-r,y-r,x+r,y+r)==true)
		return;

	r2=r-thickness;
	for(i=0;i<=360;++i)
	{
		points[i].x=x+(int)(r*sin(i*(3.141592654f/180.0f)));
		points[i].y=y+(int)(r*cos(i*(3.141592654f/180.0f)));

		points[361+(360-i)].x=x+(int)(r2*sin(i*(3.141592654f/180.0f)));
		points[361+(360-i)].y=y+(int)(r2*cos(i*(3.141592654f/180.0f)));

	}

	if(alpha==1.0f)
		kGUI::DrawPoly(360+1+360+1,points,color);
	else
		kGUI::DrawPoly(360+1+360+1,points,color,alpha);
}


/* draw rect with alpha blending */
void kGUI::DrawRect(int x1,int y1,int x2,int y2,kGUIColor color,double alpha)
{
	int x,y;
	kGUIColor *sp;
	int skip;
	int dr,dg,db,br,bg,bb;
	int newr,newg,newb;
	double balpha=1.0f-alpha;
	int dra,dga,dba;

	if(OffClip(x1,y1,x2,y2)==true)
		return;

	if(x1<m_clipcorners.lx)
		x1=m_clipcorners.lx;
	if(x2>m_clipcorners.rx)
		x2=m_clipcorners.rx;

	if(y1<m_clipcorners.ty)
		y1=m_clipcorners.ty;
	if(y2>m_clipcorners.by)
		y2=m_clipcorners.by;

	DrawColorToRGB(color,dr,dg,db);
	dra=(int)(dr*alpha);
	dga=(int)(dg*alpha);
	dba=(int)(db*alpha);

	sp=GetSurfacePtr(x1,y1);
	skip=GetSurfaceWidth()-(x2-x1);
	for(y=y1;y<y2;++y)
	{
		for(x=x1;x<x2;++x)
		{
			DrawColorToRGB(sp[0],br,bg,bb);
			
			newr=dra+(int)(br*balpha);
			newg=dga+(int)(bg*balpha);
			newb=dba+(int)(bb*balpha);

			*(sp++)=DrawColor(newr,newg,newb);
		}
		sp+=skip;
	}
}


void kGUI::DrawDotRect(int x1,int y1,int x2,int y2,kGUIColor color1,kGUIColor color2)
{
	int x,y;
	kGUIColor *sp;
	int skip;

	if(OffClip(x1,y1,x2,y2)==true)
		return;

	if(x1<m_clipcorners.lx)
		x1=m_clipcorners.lx;
	if(x2>m_clipcorners.rx)
		x2=m_clipcorners.rx;

	if(y1<m_clipcorners.ty)
		y1=m_clipcorners.ty;
	if(y2>m_clipcorners.by)
		y2=m_clipcorners.by;

	sp=GetSurfacePtr(x1,y1);
	skip=GetSurfaceWidth()-(x2-x1);
	for(y=y1;y<y2;++y)
	{
		for(x=x1;x<x2;++x)
			*(sp++)=((x+y)&2)==0?color1:color2;
		sp+=skip;
	}
}


void kGUI::PushClip(void)
{
	assert(m_clipindex<MAXCLIPS,"Clip zone buffer overflow.");

	m_clipstack[m_clipindex]=m_clipstack[m_clipindex-1];
	++m_clipindex;
}

void kGUI::PopClip(void)
{
	assert(m_clipindex>0,"No clip zones to pop.");
	--m_clipindex;
	SetClip();
}

/* shrink the clip zone */

void kGUI::ShrinkClip(int lx,int ty,int rx,int by)
{
	kGUICorners *c;

	assert(m_clipindex>0,"Stack Underflow error!");
	c=&m_clipstack[m_clipindex-1];
	c->lx=max(c->lx,lx);
	c->ty=max(c->ty,ty);
	c->rx=min(c->rx,rx);
	c->by=min(c->by,by);

	SetClip();
}

void kGUI::ResetClip(void)
{
	int w,h;

	w=m_currentsurface->GetWidth();
	h=m_currentsurface->GetHeight();

	m_clipstack[m_clipindex-1].lx=0;
	m_clipstack[m_clipindex-1].rx=w;
	m_clipstack[m_clipindex-1].ty=0;
	m_clipstack[m_clipindex-1].by=h;

	m_clipcorners.lx=0;
	m_clipcorners.rx=w;
	m_clipcorners.ty=0;
	m_clipcorners.by=h;
}


/* set the 4 clip variables to the current entry on the clip stack */

void kGUI::SetClip(void)
{
	int w,h;

	w=m_currentsurface->GetWidth();
	h=m_currentsurface->GetHeight();

	m_clipcorners=m_clipstack[m_clipindex-1];

	if(m_clipcorners.lx<0)
		m_clipcorners.lx=0;
	if(m_clipcorners.ty<0)
		m_clipcorners.ty=0;
	if(m_clipcorners.rx>w)
		m_clipcorners.rx=w;
	if(m_clipcorners.by>h)
		m_clipcorners.by=h;
}

kGUIColor *kGUI::Draw(kGUICorners *c)
{
	int d,nd;
	kGUICorners *dc;
	kGUICorners dirtycorners[MAXDIRTY];
	kGUIColor *cp;

	if(m_allowdraw==false)
	{
		if(m_busymutex.TryLock()==false)
			return(0);
	}
	++m_frame;

	/* copy dirtycorners to local stack since callbacks during draw */
	/* could add to the dirty list */
	nd=m_dirtyindex;
	memcpy(dirtycorners,m_dirtycorners,nd*sizeof(kGUICorners));
	m_dirtyindex=0;

	/* set current draw surface to screen buffer */
	m_currentsurface=&m_screensurface;

	if(m_trace)
		Trace("Start Draw\n");
	for(d=0;d<nd;++d)
	{
		m_clipindex=1;
		dc=dirtycorners+d;
		if(dc->by>m_currentsurface->GetHeight())
			dc->by=m_currentsurface->GetHeight();
		if(dc->rx>m_currentsurface->GetWidth())
			dc->rx=m_currentsurface->GetWidth();

		m_clipstack[0]=*(dc);

		SetClip();
		if( (c->lx==c->rx) || (c->by==c->ty))
			*(c)=*(dc);
		else
		{
			if(dc->lx<c->lx)
				c->lx=dc->lx;
			if(dc->ty<c->ty)
				c->ty=dc->ty;
			if(dc->rx>c->rx)
				c->rx=dc->rx;
			if(dc->by>c->by)
				c->by=dc->by;
		}

		if(m_trace)
			Trace("Draw (%d,%d,%d,%d)\n",dc->lx,dc->rx,dc->ty,dc->by);

		m_rootobj->Draw();
		/* is there a valid hint?, if so draw it now */
		if(m_hinttext.GetLen())
		{
			DrawRectFrame(m_hintcorners.lx,m_hintcorners.ty,m_hintcorners.rx,m_hintcorners.by,DrawColor(255,255,255),DrawColor(32,32,32));
			m_hinttext.Draw(m_hintcorners.lx+2,m_hintcorners.ty+4,0,0);
		}
	}
	m_currentsurface->SetOffsets(0,0);
	if(m_trace)
		Trace("End Draw\n");

	cp=m_currentsurface->GetSurfacePtr(0,0);
	if(m_allowdraw==false)
		m_busymutex.UnLock();

	return(cp);
}

#define FLASHRATE (TICKSPERSEC/3)

/* update the mouse position and button status */
void kGUI::UpdateInput(void)
{
	int i;
	kGUIObj *gobj;
	kGUICorners c;
	int saveet=m_et;

	if(m_busymutex.TryLock()==false)
		return;

	/* update the cursor flash */

	if(m_flash.Update(FLASHRATE))
	{
		m_drawcursor=!m_drawcursor;
		m_drawcursorchanged=true;

		if(m_flash.Get()>(FLASHRATE*2))		/* if large accumulated delay, then reset it */
			m_flash.Reset();
	}
	else
		m_drawcursorchanged=false;

	m_nomouse=false;
	m_dmousex=m_mousex-m_lmousex;
	m_dmousey=m_mousey-m_lmousey;
	m_xmouseleft=(m_mouseleft!=m_lmouseleft);		/* transition flag */
	m_xmouseright=(m_mouseright!=m_lmouseright);

	m_random->AddEntropy(m_mousex^(m_mousey<<8)^(GetKey()<<16)^(m_mouseleft<<24)^(m_mouseright<<25));
	m_random->ExtractEntropy((char *)&i,sizeof(i));

	/* was it just released this frame? */
	m_mouseleftdoubleclick=false;
	m_mouseleftrelesetick.Update();
	if(m_xmouseleft&(!m_mouseleft))
	{
		if(m_mouseleftrelesetick.Get()<DBLCLICKTIME)
			m_mouseleftdoubleclick=true;
		m_mouseleftrelesetick.Reset();
	}

	/* was it just released this frame? */
	m_mouserightdoubleclick=false;
	m_mouserightrelesetick.Update();
	if(m_xmouseright&(!m_mouseright))
	{
		if(m_mouserightrelesetick.Get()<DBLCLICKTIME)
			m_mouserightdoubleclick=true;
		m_mouserightrelesetick.Reset();
	}

	/* has the mouse moved at all? */
	if(!GetKey() && !m_dmousex && !m_dmousey && m_xmouseleft==false && m_xmouseright==false)
		m_inputidle=true;
	else
	{
		m_inputidle=false;
		m_hinttick=0;
		if(m_hinttext.GetLen())
		{
			m_hinttext.Clear();
			Dirty(&m_hintcorners);
		}
	}

	/* app can have a global handler to look for keyboard shortcuts */
	/* assume that if it uses and keys that it clears them */

	if(m_inputcallback.IsValid())
		m_inputcallback.Call();

	/* priority 1: send input to active object */
	if(m_activeobj)
	{
		kGUIObj *lastactiveobj;

		m_askhint=m_sethint;
		/* if the active object changes, and no input was used, then call the new active object right away */
		do
		{
			lastactiveobj=m_activeobj;
			m_activeobj->GetCorners(&c);
			if(MouseOver(&c)==false)
				m_askhint=false;

			if(m_activeobj->UpdateInput()==true)
				goto used;
		}while((m_activeobj!=lastactiveobj) && (m_activeobj!=0));
	}

	/* if the same object is updated again then this would cause it to */
	/* animate too quickly, so zero it for pri2 and pri3 objects */
	if(m_activeobj)
		m_et=0;

	/* priority 2: send keyboard input to current object */
	gobj=GetCurrentObj();

	if(gobj)
	{
		if(kGUI::GetKey()==GUIKEY_TAB)
		{
			if(gobj->GetParent())
				gobj->GetParent()->Tab(1);
			kGUI::ClearKey();
		}
		else if(kGUI::GetKey()==GUIKEY_SHIFTTAB)
		{
			if(gobj->GetParent())
				gobj->GetParent()->Tab(-1);
			kGUI::ClearKey();
		}
		m_askhint=false;
		SetNoMouse(true);
		gobj->UpdateInput();
		SetNoMouse(false);
	}

	kGUI::ClearKey();
	
	/* priority 3: pass mouse input down to object under the mouse */
	for(i=m_rootobj->GetNumChildren()-1;i>=0;--i)
	{
		gobj=m_rootobj->GetChild(i);
		gobj->GetCorners(&c);
		if(MouseOver(&c))
		{
			m_askhint=m_sethint;
			gobj->UpdateInput();
			goto used;
		}
	}

used:;

	/* call all events */
	m_et=saveet;
	for(i=0;i<m_numevents;++i)
	{
		kGUICallBack *ev;

		ev=m_events.GetEntry(i);
		ev->Call();
	}

	m_lmousex=m_mousex;
	m_lmousey=m_mousey;
	m_lmouseleft=m_mouseleft;
	m_lmouseright=m_mouseright;
	EatKey();					/* if nobody used the key, then eat it */
//	SetInUpdate(false);
	m_busymutex.UnLock();
}

/* add this rectangle to the array of dirty rectangles */

void kGUI::Dirty(const kGUICorners *c)
{
	int d,num;
	kGUICorners *dc;

	//todo, if current in a draw then toggle and use other draw box for the next frame

	assert(c->lx>=0 && c->ty>=0 && c->rx<=GetSurfaceWidth() && c->by<=GetSurfaceHeight(),"Dirty area too large!");

	/* null area? */
	if(ValidArea(c)==false)
		return;

#if 0
	m_dirtyindex=1;
	m_dirtycorners[0].lx=0;
	m_dirtycorners[0].ty=0;
	m_dirtycorners[0].rx=GetSurfaceWidth();
	m_dirtycorners[0].by=GetSurfaceHeight();
	return;
#endif
	
#if 1
	/* check to see if the dirty area is already inside an entry */
	dc=m_dirtycorners;
	for(d=0;d<m_dirtyindex;++d)
	{
		if(Inside(c,dc)==true)
			return;	/* this dirty area is already covered */
		++dc;
	}

	/* remove all entries in the table that are totally inside this new area */

	dc=m_dirtycorners;
	for(d=0;d<m_dirtyindex;)
	{
		if(Inside(dc,c)==true)
		{
			num=(m_dirtyindex-1)-d;
			if(num>0)
				memcpy(dc,dc+1,num*sizeof(kGUICorners));
			--m_dirtyindex;
		}
		else
		{
			++d;
			++dc;
		}
	}

	/* check to see if the dirty area overlaps an enrty */
	dc=m_dirtycorners;
	for(d=0;d<m_dirtyindex;++d)
	{
		if(Overlap(c,dc)==true)
		{
			kGUICorners newarea;
			/* yes, recursively add the area incase the new spot overlaps a */
			/* different entry */

			/* is there a new top area? */
			if(c->ty<dc->ty)
			{
				newarea.ty=c->ty;
				newarea.by=dc->ty;
				newarea.lx=c->lx;
				newarea.rx=c->rx;
				Dirty(&newarea);
			}
			/* is there a bottom top area? */
			if(c->by>dc->by)
			{
				newarea.ty=dc->by;
				newarea.by=c->by;
				newarea.lx=c->lx;
				newarea.rx=c->rx;
				Dirty(&newarea);
			}
			/* is there a left middle area? */
			if(c->lx<dc->lx)
			{
				newarea.lx=c->lx;
				newarea.rx=dc->lx;
				newarea.ty=max(c->ty,dc->ty);
				newarea.by=min(c->by,dc->by);
				Dirty(&newarea);
			}
			/* is there a right middle area? */
			if(c->rx>dc->rx)
			{
				newarea.lx=dc->rx;
				newarea.rx=c->rx;
				newarea.ty=max(c->ty,dc->ty);
				newarea.by=min(c->by,dc->by);
				Dirty(&newarea);
			}
			return;
		}
		++dc;
	}
#endif

#if 1
	/* if this rectangle is adjacent to an existing one, then just expand it */
	/* todo, if this is adjacent to an existing one the just expand */
	dc=m_dirtycorners;
	for(d=0;d<m_dirtyindex;++d)
	{
		/* can these be joined on the top/bottom? */
		if((c->lx==dc->lx) && (c->rx==dc->rx))
		{
			if((c->by==dc->ty) || (c->ty==dc->by))
			{
				int newtop,newbottom;

				newtop=min(c->ty,dc->ty);
				newbottom=max(c->by,dc->by);
				dc->ty=newtop;
				dc->by=newbottom;
				return;
			}
		}
		/* can these be joined on the sides? */
		if((c->ty==dc->ty) && (c->by==dc->by))
		{
			if((c->lx==dc->rx) || (c->rx==dc->lx))
			{
				int newleft,newright;

				newleft=min(c->lx,dc->lx);
				newright=max(c->rx,dc->rx);
				dc->lx=newleft;
				dc->rx=newright;
				return;
			}

		}
		++dc;
	}
#endif

	/* new area, add it to the dirty list */

	if(m_dirtyindex==MAXDIRTY)
	{
		m_dirtyindex=1;
		m_dirtycorners[0].lx=0;
		m_dirtycorners[0].ty=0;
		m_dirtycorners[0].rx=GetSurfaceWidth();
		m_dirtycorners[0].by=GetSurfaceHeight();
	}
	else
		m_dirtycorners[m_dirtyindex++]=c[0];
}

bool kGUI::Overlap(int x,int y,const kGUICorners *c1)
{
	if(x>=c1->lx && x<=c1->rx && y>=c1->ty && y<=c1->by)
		return true;
	return false;
}

bool kGUI::Overlap(const kGUICorners *c1,const kGUICorners *c2)
{
	if(c1->lx>=c2->rx)
		return(false);	/* off right */
	if(c2->lx>=c1->rx)
		return(false);	/* off left */
	if(c1->ty>=c2->by)
		return(false);	/* off bottom */
	if(c2->ty>=c1->by)
		return(false);	/* off top */
	return(true);
}

/* if corners from rect c1 are all inside c2 then return true */
bool kGUI::Inside(const kGUICorners *c1,const kGUICorners *c2)
{
	if((c1->lx>=c2->lx) && (c1->rx<=c2->rx) && (c1->ty>=c2->ty) && (c1->by<=c2->by))
		return(true);
	return(false);
}

/* start at bottom left, work across and up looking for a spot that is not */
/* already occupied by a minimized window */

void kGUI::FindMinimizeSpot(kGUICorners *corners)
{
	unsigned int i;
	int x,y;
	int w,h;
	bool hit;
	kGUIObj *gobj;

	kGUICorners c;

	kGUI::GetSkin()->GetMinimizedWindowSize(&w,&h);
	x=0;
	y=kGUI::GetScreenHeight()-h-2;
	do
	{
		/* scan all minimized windows looking for a clear area */ 

		corners->lx=x;
		corners->ty=y;
		corners->rx=x+w;
		corners->by=y+h;
	
		hit=false;

		for(i=0;(i<m_rootobj->GetNumChildren()) && (hit==false);++i)
		{
			gobj=m_rootobj->GetChild(i);

			/* only check if it is a minimized window */
			if(gobj->GetZoneW()==w && gobj->GetZoneH()==h)
			{
				gobj->GetCorners(&c);
				if(kGUI::Overlap(corners,&c))
					hit=true;
			}
		}
		if(hit==false)
			return;
		x=x+w+2;
		if((x+w)>kGUI::GetSurfaceWidth())
		{
			x=0;
			y-=h-2;
		}
	}while(1);
}

#if 0
double testhue=192;
double testsat=0.7;
#define CORNERSIZE 8

void kGUI::DrawCurve(int x,int y,int r,int t,double sa,double ea,double sv,double ev)
{
	double a;
	int l;
	int wx,wy;
	kGUIColor *sp;
	double da=ea-sa;
	double dv=ev-sv;
	double v;

	for(l=0;l<t;++l)
	{
		for(a=sa;a<ea;++a)
		{
			wx=(int)((cos(a*3.14159/180.0)*(r-l))+x);
			if(wx>=m_clipcorners.lx && wx<m_clipcorners.rx)
			{
				wy=(int)((sin(a*3.14159/180.0)*(r-l))+y);
				if(wy>=m_clipcorners.ty && wy<m_clipcorners.by)
				{
					sp=GetSurfacePtr(wx,wy);
					v=(a-sa)/da;	/* 0 to 1 */
					v=(v*dv)+sv;
					*(sp)=HSVToColor(testhue,testsat,v);
				}
			}
		}
	}

}

void kGUI::DrawButtonFrame(int lx,int ty,int rx,int by,bool pressed)
{
	double v;
	double v2;
	kGUIColor c;
	int t;

	if(OffClip(lx,ty,rx,by)==true)
		return;

	if(!pressed)
	{
		v=0.36;
		v2=0.70;
	}
	else
	{
		v=0.70;
		v2=0.36;
	}

	for(t=0;t<2;++t)
	{
		c=HSVToColor(testhue,testsat,v);
		/* top */
		DrawRect(lx+CORNERSIZE,ty,rx-CORNERSIZE,ty+1,c);
		/* left */
		DrawRect(lx,ty+CORNERSIZE,lx+1,by-CORNERSIZE,c);

		/* TL Corner */
		DrawCurve(lx+CORNERSIZE,ty+CORNERSIZE,CORNERSIZE,1,180.0,270.0,v,v);
		/* TR Corner */
		DrawCurve(rx-CORNERSIZE-1,ty+CORNERSIZE,CORNERSIZE,1,270.0,360.0,v,v2);

		c=HSVToColor(testhue,testsat,v2);
		/* right */
		kGUI::DrawRect(rx-2,ty+CORNERSIZE,rx-1,by-CORNERSIZE,c);
		/* bottom */
		kGUI::DrawRect(lx+CORNERSIZE,by-2,rx-CORNERSIZE,by-1,c);

		/* BL Corner */
		DrawCurve(lx+CORNERSIZE,by-CORNERSIZE-1,CORNERSIZE,1,90.0,180.0,v2,v);
		/* BR Corner */
		DrawCurve(rx-CORNERSIZE-1,by-CORNERSIZE-1,CORNERSIZE,1,0.0,90.0,v2,v2);

		++lx;
		++ty;
		--rx;
		--by;
		v+=0.15f;
		v2+=0.15f;
	}
}
#endif

void kGUI::DrawRectBevel(int lx,int ty,int rx,int by,bool pressed)
{
	if(OffClip(lx,ty,rx,by)==true)
		return;

	if(!pressed)
	{
		/* top */
		kGUI::DrawRect(lx,ty,rx-1,ty+1,DrawColor(255,255,255));
		/* left */
		kGUI::DrawRect(lx,ty+1,lx+1,by-1,DrawColor(255,255,255));

		/* right1 */
		kGUI::DrawRect(rx-2,ty+1,rx-1,by-1,DrawColor(172,168,153));
		/* bottom1 */
		kGUI::DrawRect(lx+1,by-2,rx-1,by-1,DrawColor(172,168,153));

		/* right2 */
		kGUI::DrawRect(rx-1,ty+1,rx,by-1,DrawColor(0,0,0));
		/* bottom2 */
		kGUI::DrawRect(lx,by-1,rx,by,DrawColor(0,0,0));

		kGUI::DrawRect(lx+1,ty+1,rx-2,by-2,DrawColor(236,233,216));
	}
	else
	{
		/* top */
		kGUI::DrawRect(lx,ty,rx-1,ty+1,DrawColor(0,0,0));
		/* top1 */
		kGUI::DrawRect(lx+1,ty+1,rx-2,ty+2,DrawColor(172,168,153));
		/* left */
		kGUI::DrawRect(lx,ty+1,lx+1,by-1,DrawColor(0,0,0));
		/* left1 */
		kGUI::DrawRect(lx+1,ty+2,lx+2,by-2,DrawColor(172,168,153));

		/* right */
		kGUI::DrawRect(rx-1,ty+1,rx,by-1,DrawColor(255,255,255));
		/* bottom */
		kGUI::DrawRect(lx,by-1,rx,by,DrawColor(255,255,255));

		kGUI::DrawRect(lx+2,ty+2,rx-1,by-1,DrawColor(236,233,216));
	}
}

void kGUI::DrawRectBevelIn(int lx,int ty,int rx,int by)
{
	DrawRectBevelIn(lx,ty,rx,by,DrawColor(255,255,255));
}

void kGUI::DrawRectBevelIn(int lx,int ty,int rx,int by,kGUIColor c)
{
	if(OffClip(lx,ty,rx,by)==true)
		return;

	/* top */
	kGUI::DrawRect(lx,ty,rx-1,ty+1,DrawColor(172,168,153));
	/* top1 */
	kGUI::DrawRect(lx+1,ty+1,rx-2,ty+2,DrawColor(0,0,0));
	/* left */
	kGUI::DrawRect(lx,ty+1,lx+1,by-1,DrawColor(172,168,153));
	/* left1 */
	kGUI::DrawRect(lx+1,ty+2,lx+2,by-2,DrawColor(0,0,0));

	/* right1 */
	kGUI::DrawRect(rx-2,ty+1,rx-1,by-1,DrawColor(236,233,216));
	/* bottom1 */
	kGUI::DrawRect(lx+1,by-2,rx-1,by-1,DrawColor(236,233,216));

	/* right2 */
	kGUI::DrawRect(rx-1,ty+1,rx,by-1,DrawColor(255,255,255));
	/* bottom2 */
	kGUI::DrawRect(lx,by-1,rx,by,DrawColor(255,255,255));

	kGUI::DrawRect(lx+2,ty+2,rx-2,by-2,c);
}


/* single pixel frame, using ocolor, inside is icolor */

void kGUI::DrawRectFrame(int lx,int ty,int rx,int by,kGUIColor icolor,kGUIColor ocolor,double alpha)
{
	if(OffClip(lx,ty,rx,by)==true)
		return;

	if(alpha==1.0f)
	{
		/* top */
		kGUI::DrawRect(lx,ty,rx,ty+1,ocolor);
		/* bottom */
		kGUI::DrawRect(lx,by-1,rx,by,ocolor);
		/* left */
		kGUI::DrawRect(lx,ty,lx+1,by,ocolor);
		/* right */
		kGUI::DrawRect(rx-1,ty,rx,by,ocolor);
		/* inside */
		kGUI::DrawRect(lx+1,ty+1,rx-1,by-1,icolor);
	}
	else
	{
		/* top */
		kGUI::DrawRect(lx,ty,rx,ty+1,ocolor,alpha);
		/* bottom */
		kGUI::DrawRect(lx,by-1,rx,by,ocolor,alpha);
		/* left */
		kGUI::DrawRect(lx,ty,lx+1,by,ocolor,alpha);
		/* right */
		kGUI::DrawRect(rx-1,ty,rx,by,ocolor,alpha);
		/* inside */
		kGUI::DrawRect(lx+1,ty+1,rx-1,by-1,icolor,alpha);
	}
}

void kGUI::DrawCurrentFrame(int lx,int ty,int rx,int by)
{
	kGUIColor color1=DrawColor(64,64,64);
	kGUIColor color2=DrawColor(255,255,255);

	if(OffClip(lx,ty,rx,by)==true)
		return;

	/* top */
	kGUI::DrawDotRect(lx,ty,rx,ty+1,color1,color2);
	/* bottom */
	kGUI::DrawDotRect(lx,by-1,rx,by,color1,color2);
	/* left */
	kGUI::DrawDotRect(lx,ty,lx+1,by,color1,color2);
	/* right */
	kGUI::DrawDotRect(rx-1,ty,rx,by,color1,color2);
}


/* draw a group of lines using the colors from the raw image */

void kGUI::DrawLineRect(int lx,int ty,int rx,int by,const unsigned char *image)
{
	int x,y;
	int r,g,b;

	if(OffClip(lx,ty,rx,by)==true)
		return;

	if(image[0]==1 && image[1]>=(by-ty))	/* horizontal lines? */
	{
		image+=2;	/* skip header */
		for(y=ty;y<by;++y)
		{
			r=*(image++);
			g=*(image++);
			b=*(image++);
			kGUI::DrawRect(lx,y,rx,y+1,DrawColor(r,g,b));
		}
	}
	else if(image[1]==1 && image[0]>=(rx-lx))	/* vertical lines? */
	{
		image+=2;	/* skip header */
		for(x=lx;x<rx;++x)
		{
			r=*(image++);
			g=*(image++);
			b=*(image++);
			kGUI::DrawRect(x,ty,x+1,by,DrawColor(r,g,b));
		}
	}
	else
	{
		assert(false,"Unknown graphics format");
	}
}

/* this is SLOW, don't use in loops, use drawrect, drawline etc */
void kGUI::DrawPixel(int x,int y,kGUIColor c)
{
	kGUIColor *sp;

	sp=GetSurfacePtrC(x,y);
	if(sp)
		*(sp)=c;
}

bool kGUI::DrawLine(int x1,int y1,int x2,int y2,kGUIColor c)
{
	int dx,dy,minx,maxx,miny,maxy;
	kGUIColor *sp;

	if(x1<=x2)
	{
		minx=x1;
		maxx=x2;
	}
	else
	{
		maxx=x1;
		minx=x2;
	}
	if(y1<=y2)
	{
		miny=y1;
		maxy=y2;
	}
	else
	{
		maxy=y1;
		miny=y2;
	}
	if(OffClip(minx,miny,maxx,maxy)==true)
		return(false);

	dx=x2-x1;
	dy=y2-y1;

	if(abs(dx)>abs(dy))
	{
		int x,stepx;
		double stepy=(double)dy/(double)abs(dx);
		double y;

		if(dx>0)
			stepx=1;
		else
			stepx=-1;
		y=(double)y1;
		for(x=x1;x!=x2;x+=stepx)
		{
			sp=GetSurfacePtrC(x,(int)y);
			if(sp)
				*(sp)=c;
			y+=stepy;
		}
	}
	else
	{
		int y,stepy;
		double stepx=(double)dx/(double)abs(dy);
		double x;

		if(dy>0)
			stepy=1;
		else if(dy<0)
			stepy=-1;
		else
			stepy=0;
		x=(double)x1;
		for(y=y1;y!=y2;y+=stepy)
		{
			sp=GetSurfacePtrC((int)x,y);
			if(sp)
				*(sp)=c;
			x+=stepx;
		}
	}
	return(true);
}


bool kGUI::DrawLine(int x1,int y1,int x2,int y2,kGUIColor c,double alpha)
{
	int dx,dy,minx,maxx,miny,maxy;
	kGUIColor *sp;
	double balpha=1.0f-alpha;
	int dr,dg,db,br,bg,bb;
	int newr,newg,newb;
	int dra,dga,dba;

	if(x1<=x2)
	{
		minx=x1;
		maxx=x2;
	}
	else
	{
		maxx=x1;
		minx=x2;
	}
	if(y1<=y2)
	{
		miny=y1;
		maxy=y2;
	}
	else
	{
		maxy=y1;
		miny=y2;
	}
	if(OffClip(minx,miny,maxx,maxy)==true)
		return(false);

	DrawColorToRGB(c,dr,dg,db);
	dra=(int)(dr*alpha);
	dga=(int)(dg*alpha);
	dba=(int)(db*alpha);

	dx=x2-x1;
	dy=y2-y1;

	if(abs(dx)>abs(dy))
	{
		int x,stepx;
		double stepy=(double)dy/(double)abs(dx);
		double y;

		if(dx>0)
			stepx=1;
		else
			stepx=-1;
		y=(double)y1;
		for(x=x1;x!=x2;x+=stepx)
		{
			sp=GetSurfacePtrC(x,(int)y);
			if(sp)
			{
				DrawColorToRGB(sp[0],br,bg,bb);
			
				newr=dra+(int)(br*balpha);
				newg=dga+(int)(bg*balpha);
				newb=dba+(int)(bb*balpha);
				*(sp)=DrawColor(newr,newg,newb);
			}
			y+=stepy;
		}
	}
	else
	{
		int y,stepy;
		double stepx=(double)dx/(double)abs(dy);
		double x;

		if(dy>0)
			stepy=1;
		else if(dy<0)
			stepy=-1;
		else
			stepy=0;
		x=(double)x1;
		for(y=y1;y!=y2;y+=stepy)
		{
			sp=GetSurfacePtrC((int)x,y);
			if(sp)
			{
				DrawColorToRGB(sp[0],br,bg,bb);
			
				newr=dra+(int)(br*balpha);
				newg=dga+(int)(bg*balpha);
				newb=dba+(int)(bb*balpha);

				*(sp)=DrawColor(newr,newg,newb);
			}
			x+=stepx;
		}
	}
	return(true);
}

/* return true if whole area is outside clip range */

bool kGUI::OffClip(int lx,int ty,int rx,int by)
{
	if(lx>=m_clipcorners.rx)
		return(true);	/* off right */
	if(rx<=m_clipcorners.lx)
		return(true);	/* off left */
	if(ty>=m_clipcorners.by)
		return(true);	/* off bottom */
	if(by<=m_clipcorners.ty)
		return(true);	/* off top */
	return(false);
}

kGUIFontInfo::kGUIFontInfo()
{
	m_fontid=kGUI::GetDefFontID();
	m_size=kGUI::GetDefFontSize();
	m_color=DrawColor(0,0,0);
}

void kGUI::SetImageSizeCacheFilename(char *fn)
{
	m_imagesizefilename=fn;
	LoadImageSizeCache();
}

void kGUI::LoadImageSizeCache(void)
{
	int i,num;
	char *fileptr;
	char *fp;
	char *picfn;
	kGUIImageSizeCache info;

	fileptr=(char *)LoadFile(m_imagesizefilename);
	if(fileptr)
	{
		fp=fileptr;
		num=atoi(fp);
		fp+=strlen(fp)+1;
		for(i=0;i<num;++i)
		{
			picfn=fp;
			fp+=strlen(fp)+1;
			info.time=atoi(fp);
			fp+=strlen(fp)+1;
			info.width=atoi(fp);
			fp+=strlen(fp)+1;
			info.height=atoi(fp);
			fp+=strlen(fp)+1;
			m_imagesizehash.Add(picfn,&info);
		}
		delete []fileptr;
	}
}

void kGUI::SaveImageSizeCache(void)
{
	FILE *fp;
	int i,num;
	kGUIImageSizeCache *info;
	HashEntry *he;

	if(!m_imagesizefilename)
		return;
	fp=fopen(m_imagesizefilename,"wb");
	if(fp)
	{
		num=m_imagesizehash.GetNum();
		he=m_imagesizehash.GetFirst();
		fprintf(fp,"%d%c",num,0);
		for(i=0;i<num;++i)
		{
			info=(kGUIImageSizeCache *)(he->m_data);
			fprintf(fp,"%s%c%d%c%d%c%d%c",he->m_string,0,(int)info->time,0,info->width,0,info->height,0);
			he=he->m_next;
		}
		fclose(fp);
	}
}

void kGUI::SetMouse(int x,int y,int wheeldelta,bool left,bool right)
{
	m_mousex=x;
	m_mousey=y;
	m_mousewheeldelta=wheeldelta;
	m_mouseleft=left;
	m_mouseright=right;
}

/* an object has been changed, call all parents and if they have active */
/* after update functions then call them all */
void kGUI::CallAAParents(kGUIObj *obj)
{
	kGUIContainerObj *p;

	p=obj->GetParent();
	while(p)
	{
		p->CallEvent(EVENT_AFTERUPDATE);
		p=p->GetParent();
	}
}

/***********************************************************************/

void kGUI::FileDelete(const char *filename)
{
	remove(filename);
}

void kGUI::FileCopy(const char *fromname,const char *toname)
{
	unsigned long fs;
	unsigned char *buf;
	FILE *cf;

	buf=LoadFile(fromname,&fs);
	if(buf)
	{
		cf=fopen(toname,"wb");
		if(cf)
		{
			fwrite(buf,1,fs,cf);
			fclose(cf);
		}
	}
	delete []buf;
}


bool kGUI::FileExists(const char *filename)
{
	FILE *f;

	if(!filename[0])
		return(false);

	f=fopen(filename,"rb");
	if(!f)
		return(false);
	fclose(f);
	return(true);
}

void kGUI::FileRename(const char *from,const char *to)
{
	rename(from,to);
}

/* checks all bigfiles first */

long long kGUI::FileSize(const char *filename)
{
	long long fs;
	DataHandle dh;

	dh.SetFilename(filename);
	fs=dh.GetSize();
	return(fs);
}

/* checks all bigfiles first */

long kGUI::FileCRC(const char *filename)
{
	long long i;
	long long fs;
	long hashcode=0;
	unsigned char byte;
	DataHandle dh;

	dh.SetFilename(filename);
	if(dh.Open()==false)
		return(0);
	fs=dh.GetSize();

	for(i=0;i<fs;++i)
	{
		dh.Read(&byte,(unsigned long)1);
		hashcode=(long)byte^(hashcode<<6);
		hashcode^=hashcode>>24;
		hashcode&=0xffffff;
	}
	dh.Close();
	return(hashcode);
}

long kGUI::FileTime(const char *filename)
{
	long ft;
	DataHandle dh;

	dh.SetFilename(filename);
	ft=dh.GetTime();
	return(ft);
}

unsigned char *kGUI::LoadFile(const char *filename,unsigned long *filesize)
{
	unsigned char *fm;
	unsigned long fs;
	DataHandle dh;

	m_random->AddEntropy(filename,(int)strlen(filename));

	if(filesize)
		filesize[0]=0;
	dh.SetFilename(filename);
	if(dh.Open()==false)
		return(0);	/* file not found or empty file */
	
	fs=dh.GetLoadableSize();
	/* allocate space for file to load info */
	fm=new unsigned char[(unsigned long)fs+1];
	if(!fm)
	{
		dh.Close();
		return(0);
	}
	fm[fs]=0;	/* allocate an extra byte and put a null at the end */
	dh.Read(fm,fs);
	dh.Close();
	if(filesize)
		filesize[0]=(unsigned long)fs;
	return(fm);
}

/*******************************************************************/

#define MINx(x,y) ((x)>(y)?(y):(x))
#define MAXx(x,y) ((x)<(y)?(y):(x))

void kGUI::RGBToHSV(unsigned char cr, unsigned char cg, unsigned char cb,double *ph,double *ps,double *pv)
{
	double r,g,b;
    double max, min, delta;
    
    /* convert RGB to [0,1] */
    
    r = (double)cr/255.0f;
    g = (double)cg/255.0f;
    b = (double)cb/255.0f;

    max = MAXx(r,(MAXx(g,b)));
    min = MINx(r,(MINx(g,b)));
	      
    pv[0] = max;
    
    /* Calculate saturation */
    
    if (max != 0.0)
		ps[0] = (max-min)/max;
    else
		ps[0] = 0.0; 

	if (ps[0] == 0.0)
	{
		ph[0] = 0.0f;	//UNDEFINED;
		return;
    }
    /* chromatic case: Saturation is not 0, so determine hue */
    delta = max-min;

    if (r==max)
	{
		ph[0] = (g-b)/delta;
    }
    else if (g==max)
	{
		ph[0] = 2.0 + (b-r)/delta;
    }
    else if (b==max)
	{
		ph[0] = 4.0 + (r-g)/delta;
    }
    ph[0] = ph[0] * 60.0;
    if (ph[0] < 0.0)
		ph[0] += 360.0;
}

void kGUI::HSVToRGB(double h,double s,double v,unsigned char *pr,unsigned char *pg,unsigned char *pb)
{
	int i;
	double f, p, q, t;
	double r,g,b;

	if( s == 0 )
	{
		// achromatic (grey)
		r = g = b = v;
	}
	else
	{
		h /= 60;			// sector 0 to 5
		i = (int)floor( h );
		f = h - i;			// factorial part of h
		p = v * ( 1 - s );
		q = v * ( 1 - s * f );
		t = v * ( 1 - s * ( 1 - f ) );
		switch( i )
		{
		case 0:
			r = v;
			g = t;
			b = p;
		break;
		case 1:
			r = q;
			g = v;
			b = p;
		break;
		case 2:
			r = p;
			g = v;
			b = t;
		break;
		case 3:
			r = p;
			g = q;
			b = v;
		break;
		case 4:
			r = t;
			g = p;
			b = v;
		break;
		default:		// case 5:
			r = v;
			g = p;
			b = q;
		break;
		}
	}
	r*=255;
	g*=255;
	b*=255;

	pr[0]=(unsigned char)r;
	pg[0]=(unsigned char)g;
	pb[0]=(unsigned char)b;
}

/***********************************************************************/

void kGUI::AddEvent(void *codeobj,void (*code)(void *))
{
	kGUICallBack *ev;

	ev=new kGUICallBack();
	ev->Set(codeobj,code);
	m_events.SetEntry(m_numevents++,ev);
}

void kGUI::DelEvent(void *codeobj,void (*code)(void *))
{
	int i;
	kGUICallBack *ev;

	for(i=0;i<m_numevents;++i)
	{
		ev=m_events.GetEntry(i);
		if(ev->Is(codeobj,code))
		{
			delete ev;
			m_events.DeleteEntry(i);
			--m_numevents;
			return;
		}
	}
	assert(false,"event not in list error!");
}

/* this is only to be used by external threads that need to access the gui */
void kGUI::GetAccess(void)
{
	m_busymutex.Lock();
}

bool kGUI::TryAccess(void)
{
	bool rc;

	/* return false if not able to get access right away */
	rc=m_busymutex.TryLock();
	if(rc==false)
		Sleep(1);
	return(rc);
}


void kGUI::ReleaseAccess(void)
{
	m_busymutex.UnLock();
}

/***********************************************************************/

/* generates URLbase and URLroot for relative addressing */
void kGUI::ExtractURL(kGUIString *url,kGUIString *parentbase,kGUIString *parentroot)
{
	unsigned int l;
	unsigned int i;
	unsigned int c;
	bool isfile;
	char dirs[]={DIRCHAR};

	if(!strncmp(url->GetString(),"file://",7))
		isfile=true;
	else
		isfile=false;

	parentroot->SetString(url);
	parentbase->SetString(url);
	l=parentroot->GetLen();
	if(l)
	{
		for(i=0;i<l;++i)
		{
			if(parentroot->GetChar(i)=='?' || parentroot->GetChar(i)=='#')
			{
				parentroot->Clip(i);
				break;
			}
		}
		l=parentroot->GetLen();

		/* build URL root */
		while(l>1)
		{
			if(parentroot->GetChar(l-1)=='/')
			{
				if(l==7)
				{
					parentroot->Append("/");		//was "http://www.xxx.com"
				}
				else
					parentroot->Clip(l);
				break;
			}
			else if(parentroot->GetChar(l-1)==dirs[0] && isfile==true)	/* file://c:\page.html */
			{
				parentroot->Clip(l);
				break;
			}
			else
				--l;
		}

		/* build URL base */
		parentbase->SetString(parentroot->GetString());
		l=0;
		c=0;
		while(c<parentbase->GetLen())
		{
			if(parentbase->GetChar(c)=='/')
			{
				if(++l==3)
				{
					parentbase->Clip(c);
					break;
				}
			}
			else if(parentroot->GetChar(c)==dirs[0] && isfile==true)	/* file://c:\page.html */
			{
				parentbase->Clip(c+1);
				break;
			}
			++c;
		}
	}
}

/* pass in the referer(parent) url, then the relative url and generate a new url */

void kGUI::MakeURL(kGUIString *parent,kGUIString *in,kGUIString *out)
{
	kGUIString parentbase;
	kGUIString parentroot;

	/* use same encoding as input string */
	out->SetEncoding(in->GetEncoding());

	ExtractURL(parent,&parentbase,&parentroot);
	MakeURL(&parentbase,&parentroot,in,out);
}

void kGUI::MakeURL(kGUIString *parentbase,kGUIString *parentroot,kGUIString *in,kGUIString *out)
{
	kGUIString temp;

	/* if url begins with "./fred.html" change to "fred.html" */
	if(!strncmp(in->GetString(),"./",2))
	{
		/* only change 1 */
		in->Replace("./",0,0,0,1);	
	}
	in->Replace("\\","/");
	in->Trim(TRIM_QUOTES);
	
	/* is it a global URL already? */
	if(!strnicmp(in->GetString(),"http://",7))
		temp.SetString(in);
	else if(!strnicmp(in->GetString(),"file://",7))
		temp.SetString(in);
	else if(!strnicmp(in->GetString(),"https://",8))
		temp.SetString(in);
	else if(in->GetChar(0)=='/')
	{
		if(in->GetChar(1)=='/')
		{
			const char *ss;

			/* copy "protocol:" ( usually http or https or file ) from parent then use rest from in*/
			temp.SetString(parentbase->GetString());
			ss=strstr(temp.GetString(),"//");
			if(ss)
				temp.Clip((unsigned int)(ss-temp.GetString()));
			temp.Append(in->GetString());
		}
		else
			temp.Sprintf("%s%s",parentbase->GetString(),in->GetString());
	}
	else if(in->GetChar(0)=='.' && in->GetChar(1)=='.')
	{
		int numback;
		int c;
		kGUIString temp2;

		temp2.SetString(in);
		numback=temp2.Replace("../","");		/* number of directories back */
		c=parentroot->GetLen()-2;
		if(c<0)
		{
			// error cannot go back any more!
			temp.SetString(temp2.GetString());
		}
		else
		{
			while(c>0 && numback)
			{
				if(parentroot->GetChar(c)=='/')
				{
					if(--numback==0)
						break;
				}
				--c;
			}
			temp.SetString(parentroot->GetString(),c+1);
			temp.Append(temp2.GetString());
		}
	}
	else
		temp.Sprintf("%s%s",parentroot->GetString(),in->GetString());

	/* remove any trailing spaces */
	out->SetString(&temp);
	out->Trim();

	if(!strncmp(out->GetString(),"file://",7))
		out->Replace("/",DIRCHAR,7);
}

void kGUI::SplitFilename(kGUIString *longfn,kGUIString *path,kGUIString *shortfn)
{
	unsigned int pathlen;
	const char *cp;

	/* is longfn a path or a file reference? */
	if(kGUI::IsDir(longfn->GetString()))
		pathlen=longfn->GetLen();
	else
	{
		pathlen=0;
		if(longfn->GetLen()>1)
		{
			if(longfn->GetChar(1)==':')
				pathlen=2;
		}
		/* skip ahead to last "/"  */
		do
		{
			cp=strstr(longfn->GetString()+pathlen,"\\");
			if(!cp)
				cp=strstr(longfn->GetString()+pathlen,"/");
			if(!cp)
				break;
			pathlen=(unsigned int)((cp+1)-longfn->GetString());
		}while(1);
	}
	/* make path and shortfn */
	if(path)
		path->SetString(longfn->GetString(),pathlen);
	if(shortfn)
		shortfn->SetString(longfn->GetString()+pathlen);
}

void kGUI::ExtractPath(kGUIString *longfn,kGUIString *path)
{
	SplitFilename(longfn,path,0);
}

/* given a filename with a path prefix, return just the local filename */

void kGUI::ExtractFilename(kGUIString *longfn,kGUIString *shortfn)
{
	SplitFilename(longfn,0,shortfn);
}

void kGUI::MakeFilename(kGUIString *path,kGUIString *shortfn,kGUIString *longfn)
{
	if(path->GetLen())
		longfn->Sprintf("%s" DIRCHAR "%s" ,path->GetString(),shortfn->GetString());
	else
		longfn->SetString(shortfn);

	longfn->Replace(DIRCHAR DIRCHAR ,DIRCHAR);
}

/* recalc child areas since border art might have changed size */
void kGUI::SkinChanged(void)
{
	unsigned int i;
	kGUIObj *gobj;
	kGUIContainerObj *gcobj;

	for(i=0;i<m_rootobj->GetNumChildren();++i)
	{
		gobj=m_rootobj->GetChild(i);
		if(gobj->IsContainer())
		{
			gcobj=static_cast<kGUIContainerObj *>(gobj);
			gcobj->SkinChanged();
		}
	}
	/* redraw the whole screen */
	GetBackground()->Dirty();
}

/***********************************************************************/

kGUIMsgBoxReq::kGUIMsgBoxReq(int buttons,bool spr,const char *message,...)
{
	m_docall=false;
	if(spr==true)
	{
		kGUIString fmessage;
		va_list args;

	    va_start(args, message);
		fmessage.AVSprintf(message,args);
		va_end(args);
		Init(fmessage.GetString(),buttons);
	}
	else
		Init(message,buttons);
}

kGUIMsgBoxReq::kGUIMsgBoxReq(int buttons,void *codeobj,void (*code)(void *,int result),bool spr,const char *message,...)
{
	m_docall=true;
	m_donecallback.Set(codeobj,code);

	if(spr)
	{
		kGUIString fmessage;
		va_list args;

		va_start(args, message);
		fmessage.AVSprintf(message,args);
		va_end(args);
		Init(fmessage.GetString(),buttons);
	}
	else
		Init(message,buttons);
}

void kGUIMsgBoxReq::Init(const char *message,int buttons)
{
	int x,y,w,h,tw,th,maxh,brx;
	int numbuttons;
	Array<kGUIObj *>bp;

	m_window.SetAllowButtons(WINDOWBUTTON_CLOSE);

	//	assert(kGUI::GetActiveObj()==0,"Active object is set!");
	m_text.SetPos(0,0);
	m_text.SetString(message);
	m_text.SetUseBGColor(false);		/* transparent bg color */

	/* calculate size of message handle multi lines */
	tw=m_text.CalcLineList(kGUI::GetSurfaceWidth()-64)+24;	/* plus room for scrollbar */	
	th=m_text.CalcHeight(tw);
	maxh=kGUI::GetSurfaceHeight()-100;
	if(th>maxh)
		th=maxh;

	m_text.SetSize(tw+6,th+6);	
	m_text.SetLocked(true);
	m_window.AddObject(&m_text);
	w=tw+20;
	h=th;

	numbuttons=0;
	bp.Init(10,1);
	y=h+20;
	x=0;
	assert(buttons!=0,"Must have at least one button defined!");
	if(buttons&MSGBOX_YES)
	{
		bp.SetEntry(numbuttons++,&m_yes);
		m_yes.SetString(kGUI::GetString(KGUISTRING_YES));
		m_yes.SetSize(60,20);
		m_yes.SetPos(x,y);
		m_yes.SetEventHandler(this,CALLBACKNAME(PressYes));
		m_window.AddObject(&m_yes);
		x+=60+20;
	}
	if(buttons&MSGBOX_NO)
	{
		bp.SetEntry(numbuttons++,&m_no);
		m_no.SetString(kGUI::GetString(KGUISTRING_NO));
		m_no.SetSize(60,20);
		m_no.SetPos(x,y);
		m_no.SetEventHandler(this,CALLBACKNAME(PressNo));
		m_window.AddObject(&m_no);
		x+=60+20;
	}
	if(buttons&MSGBOX_OK)
	{
		bp.SetEntry(numbuttons++,&m_ok);
		m_ok.SetString(kGUI::GetString(KGUISTRING_OK));
		m_ok.SetSize(60,20);
		m_ok.SetPos(x,y);
		m_ok.SetEventHandler(this,CALLBACKNAME(PressOK));
		m_window.AddObject(&m_ok);
		x+=60+20;
		m_ok.SetCurrent();
	}
	if(buttons&MSGBOX_CANCEL)
	{
		bp.SetEntry(numbuttons++,&m_cancel);
		m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
		m_cancel.SetSize(60,20);
		m_cancel.SetPos(x,y);
		m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));
		m_window.AddObject(&m_cancel);
		x+=60+20;
	}

	if(buttons&MSGBOX_DONE)
	{
		bp.SetEntry(numbuttons++,&m_done);
		m_done.SetString(kGUI::GetString(KGUISTRING_DONE));
		m_done.SetSize(60,20);
		m_done.SetPos(x,y);
		m_done.SetEventHandler(this,CALLBACKNAME(PressDone));
		m_window.AddObject(&m_done);
		x+=60+20;
	}
	brx=x;		/* right edge of last button */
	if(x>w)
		w=x;
	h=y+30+40;

	if(w>kGUI::GetSurfaceWidth())
		w=kGUI::GetSurfaceWidth();

	/* move all buttons over to center them all */
	if(brx<w)
	{
		int i;
		int offset=(w-brx)>>1;
		kGUIObj *o;

		for(i=0;i<numbuttons;++i)
		{
			o=bp.GetEntry(i);
			o->SetZoneX(o->GetZoneX()+offset);
		}
	}

	x=(kGUI::GetSurfaceWidth()-w)/2;
	y=(kGUI::GetSurfaceHeight()-h)/2;
	if(y<0)
		y=0;

	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetSize(w,h);
	m_window.SetPos(x,y);
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);
	m_result=MSGBOX_ABORT;
}

void kGUIMsgBoxReq::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::ReDraw();		/* redraw with the window gone incase this callback takes a long time */
		if(m_docall==true)
			m_donecallback.Call(m_result);
		delete this;
	break;
	}
}



void kGUIMsgBoxReq::PressNo(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_result=MSGBOX_NO;
		m_window.Close();
	}
}

void kGUIMsgBoxReq::PressYes(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_result=MSGBOX_YES;
		m_window.Close();
	}
}

void kGUIMsgBoxReq::PressCancel(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_result=MSGBOX_CANCEL;
		m_window.Close();
	}
}

void kGUIMsgBoxReq::PressOK(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_result=MSGBOX_OK;
		m_window.Close();
	}
}

void kGUIMsgBoxReq::PressDone(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_result=MSGBOX_DONE;
		m_window.Close();
	}
}

/********************************************************************************/

kGUIInputBoxReq::kGUIInputBoxReq(void *codeobj,void (*code)(void *,kGUIString *result,int closebutton),const char *message,...)
{
	int x,y,w,h;
	kGUIString fmessage;
	va_list args;

	m_window.SetAllowButtons(WINDOWBUTTON_CLOSE);

    va_start(args, message);
	fmessage.AVSprintf(message,args);
    va_end(args);

	//assert(kGUI::GetActiveObj()==0,"Active object is set!");

	m_donecallback.Set(codeobj,code);

	m_text.SetPos(0,0);
	m_text.SetString(fmessage.GetString());
	m_window.AddObject(&m_text);
	w=m_text.GetZoneW()+20;
	h=m_text.GetZoneH();

	y=h+20;
	x=20;

	m_input.SetPos(0,y);
	m_input.SetSize(300,20);
	m_input.SetString("");
	m_input.SetMaxLen(256);

	m_window.AddObject(&m_input);
	if(w<300)
		w=320;
	y+=20+20;

	m_ok.SetString(kGUI::GetString(KGUISTRING_OK));
	m_ok.SetSize(60,20);
	m_ok.SetPos(x,y);
	m_ok.SetEventHandler(this,CALLBACKNAME(PressOK));
	m_window.AddObject(&m_ok);
	x+=60+20;

	m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
	m_cancel.SetSize(60,20);
	m_cancel.SetPos(x,y);
	m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));
	m_window.AddObject(&m_cancel);
	x+=60+20;

	if(x>w)
		w=x;
	h=y+30+40;

	m_closebutton=MSGBOX_ABORT;
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetSize(w,h);
	m_window.SetPos( (kGUI::GetSurfaceWidth()-w)/2,(kGUI::GetSurfaceHeight()-h)/2);
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);

	m_input.Activate();
}

void kGUIInputBoxReq::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::ReDraw();		/* redraw with the window gone incase this callback takes a long time */
		m_donecallback.Call(&m_input,m_closebutton);

		delete this;
	break;
	}
}



void kGUIInputBoxReq::PressCancel(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_closebutton=MSGBOX_CANCEL;
		m_input.Clear();
		m_window.Close();
	}
}

void kGUIInputBoxReq::PressOK(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_closebutton=MSGBOX_OK;
		m_window.Close();
	}
}

/**************************************************************************/

void kGUI::RotatePoints(int nvert,kGUIPoint2 *inpoints,kGUIPoint2 *outpoints,double angle)
{
	int i;
	kGUIPoint2 in;

	for(i=0;i<nvert;++i)
	{
		in=inpoints[i];	/* incase in=out */
		outpoints[i].x=(int)(cos(angle)*in.x-sin(angle)*in.y);
		outpoints[i].y=(int)(sin(angle)*in.x+cos(angle)*in.y);
	}
}

void kGUI::TranslatePoints(int nvert,kGUIPoint2 *inpoints,kGUIPoint2 *outpoints,kGUIPoint2 *xlate)
{
	int i;
	for(i=0;i<nvert;++i)
	{
		outpoints[i].x=inpoints[i].x+xlate->x;
		outpoints[i].y=inpoints[i].y+xlate->y;
	}
}

/* p2-p1 , angle is in radians */
double kGUI::CalcAngle(double dx,double dy)
{
	double angle=0.0f;

    // Calculate angle
    if (dx == 0.0f)
    {
        if (dy == 0.0)
            angle = 0.0;
        else if (dy > 0.0)
            angle = 3.141592654f / 2.0;
        else
            angle = 3.141592654f * 3.0 / 2.0;
    }
    else if (dy == 0.0f)
    {
        if  (dx > 0.0)
            angle = 0.0;
        else
            angle = 3.141592654f;
    }
    else
    {
        if  (dx < 0.0)
            angle = atan(dy/dx) + 3.141592654f;
        else if (dy < 0.0)
            angle = atan(dy/dx) + (2*3.141592654f);
        else
            angle = atan(dy/dx);
    }
    return (angle);
}

/* longest + 1/4 of shorter */

int kGUI::FastHypot(int dx,int dy)
{
	int adx,ady;

	if(dx>=0)
		adx=dx;
	else
		adx=-dx;
	if(dy>=0)
		ady=dy;
	else
		ady=-dy;

	if(adx>ady)
		return(adx+(ady>>2));
	return(ady+(adx>>2));
}

/***************************************************************************************/

void kGUIBitStream::Set(const void *buf)
{
	m_buf=(const unsigned char *)buf;
	m_start=m_buf;
	m_bits=8;
	if(m_revi==false)
		m_bit=1;	/* start at bottom bit */
	else
		m_bit=128;	/* start at top bit */
}

static unsigned int bitmasks[33]={0,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f, 
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff, 
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff, 
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 
	0x00ffffff, 0x003fffff, 0x007fffff, 0x00ffffff, 
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};

unsigned int kGUIBitStream::ReadU(int nbits)
{
	unsigned int val=0;
	unsigned int valbit=1;

	/* optimize for a popular case */
	/* are all the bits available in the remaining byte? */
	if((nbits<=m_bits) && m_revi==false && m_revo==false)
	{
		val=(m_buf[0]>>(8-m_bits))&bitmasks[nbits];
		m_bits-=nbits;
		m_bit<<=nbits;

		if(m_bit==256)
		{
			++m_buf;
			m_bit=1;
			m_bits=8;
		}
		return(val);
	}

	/* not optimal case so get a bit at a time */
	while(nbits--)
	{
		if(m_revo==false)
		{
			if(m_buf[0]&m_bit)
				val|=valbit;
			valbit<<=1;
		}
		else
		{
			val<<=1;
			if(m_buf[0]&m_bit)
				val|=1;
		}
		--m_bits;
		if(m_revi==false)
		{
			m_bit<<=1;
			if(m_bit==256)
			{
				++m_buf;
				m_bit=1;
				m_bits=8;
			}
		}
		else
		{
			m_bit>>=1;
			if(!m_bit)
			{
				++m_buf;
				m_bit=128;
				m_bits=8;
			}
		}
	}
	return(val);
}

int kGUIBitStream::ReadS(int nbits)
{
	bool neg;
	int value;

	neg=(ReadU(1)==1);
	value=(int)ReadU(nbits-1);
	if(neg==true)
		value=(1<<nbits)-value;
	return(value);
}

unsigned int kGUIBitStream::XReadU(int nbits)
{
	int i;
	unsigned int inbit;
	unsigned int outbit;
	unsigned int inval;
	unsigned int outval;

	inval=ReadU(nbits);
	if(nbits==1)
		return(inval);		/* no bits to swap since only 1 bit was read */

	outval=0;
	inbit=1<<(nbits-1);
	outbit=1;
	for(i=0;i<nbits;++i)
	{
		if(inval&inbit)
			outval|=outbit;
		inbit>>=1;
		outbit<<=1;
	}
	return(outval);
}

int kGUIBitStream::XReadS(int nbits)
{
	bool neg;
	int value;

	value=(int)XReadU(nbits-1);
	neg=(ReadU(1)==1);
	if(neg==true)
		value=(1<<nbits)-value;
	return(value);
}

int kGUIBitStream::NumRead(void)
{
	int bitsread;

	bitsread=(int)(m_buf-m_start)<<3;
	bitsread+=(8-m_bits);
	return(bitsread);
}

/**************************************************/

static const char *wday3[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

int kGUIDate::m_gmtdelta=-1;

void kGUIDate::CalcGMTDelta(void)
{
	/* only calc once */
	if(m_gmtdelta==-1)
	{
		/* calc different between localtime and GMT time */
		time_t time_of_day;
		struct tm localtinfo;
		struct tm gmttinfo;
		time_t ltime;
		time_t gtime;

		time_of_day = time( NULL );

		/* according to windows docs localtime is threadsafe, not sure about linux maybe localtime_r */
		localtinfo=*(localtime( &time_of_day ));
		gmttinfo=*(gmtime( &time_of_day ));

		ltime=mktime(&localtinfo);
		gtime=mktime(&gmttinfo);
		m_gmtdelta=(int)difftime(ltime,gtime);
	}
}

void kGUIDate::GMTtoLocal(void)
{
	CalcGMTDelta();
	AddSeconds(m_gmtdelta);
}

void kGUIDate::LocaltoGMT(void)
{
	CalcGMTDelta();
	AddSeconds(-m_gmtdelta);
}

const char *kGUIDate::GetWeekDay3(int d)	/* 0=Sun, 1=Mon .... 6=Sat */
{
	assert((d>=0) && (d<=6),"Illegal Day Index!");
	return wday3[d];
}

static const char *wday[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

const char *kGUIDate::GetWeekDay(int d)		/* 0=Sunday, 1=Monday .... 6=Saturday */
{
	assert((d>=0) && (d<=6),"Illegal Day Index!");
	return wday[d];
}

static const char *monthnames[12]={
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"};

const char *kGUIDate::GetMonthName(int m)		/* 1=January,... 12=December */
{
	assert((m>=1) && (m<=12),"Illegal Month Index!");
	return monthnames[m-1];
}

kGUIDate::kGUIDate()
{
	m_day=0;
	m_month=0;
	m_year=0;

	m_hour=0;
	m_minute=0;
	m_second=0;
}

/* mysql format yyyy-mm-dd or yyyy-mm-ddTh:mm:ssZ */
void kGUIDate::Set(const char *datestring)
{
	bool err;

	err=Setz(datestring);
	passert(err==true,"Illegal datestring format '%s'",datestring);
}

bool kGUIDate::Setz(const char *datestring)
{
	const char *ds;
    static const char *mon_name[12]={
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
	bool gotm;

	/* formats excepted are: */
	/* yyyy-mm-dd */
	/* Wdy, DD-Mon-YY HH:MM:SS GMT */
	/* Wdy, D Mon YY HH:MM:SS GMT */
	ds=strstr(datestring,",");
	if(ds)
	{
		/* Wdy, DD-Mon-YY HH:MM:SS GMT */
		/* Wdy, DD-Mon-YYYY HH:MM:SS GMT */
		++ds;
		while(ds[0]==' ')
			++ds;
		if(ds[0]<'0' || ds[0]>'9')
			goto baddate;
		m_day=atoi(ds);
		while(ds[0]>='0' && ds[0]<='9')
			++ds;
		if(ds[0]!='-' && ds[0]!=' ')
			goto baddate;
		++ds;
		while(ds[0]==' ')
			++ds;
		gotm=false;

		/* 3 letter month? */
		if((ds[3]=='-') || (ds[3]==' '))
		{
			for(m_month=0;(m_month<12) && (gotm==false);++m_month)
			{
				if(!strnicmp(ds,mon_name[m_month],3))
				{
					gotm=true;
					ds+=4;
				}
			}
		}
		else	/* full month string? */
		{
			const char *ms;
			int ml;

			for(m_month=0;(m_month<12) && (gotm==false);++m_month)
			{
				ms=monthnames[m_month];
				ml=(int)strlen(ms);
				if(!strnicmp(ds,ms,ml))
				{
					gotm=true;
					ds+=ml+1;
				}
			}

		}
		if(!gotm)
			goto baddate;

		m_year=atoi(ds);
		if(m_year<100)
			m_year+=1900;
	
		/* now get time part */
		ds=strstr(ds," ");
		if(!ds)
			goto baddate;
		while(ds[0]==' ')
			++ds;	/* skip spaces */
		if(ds[2]!=':')
			goto baddate;
		if(ds[5]!=':')
			goto baddate;

		m_hour=atoi(ds);
		m_minute=atoi(ds+3);
		m_second=atoi(ds+6);

		/* if GMT time then covert to localtime */
		if(strstr(ds,"GMT"))
			GMTtoLocal();
	}
	else
	{
		/* yyyy-mm-dd */

		if(strlen(datestring)<10)
		{
baddate:;
			SetToday();
			return(false);
		}
		m_year=atoi(datestring);
		ds=strstr(datestring,"-");		/* allow 4 or 5 digit ( or more ) year dates, no y10k bug! */
		if(!ds)
			ds=strstr(datestring," ");	/* allow 4 or 5 digit ( or more ) year dates, no y10k bug! */
		if(!ds)
			goto baddate;
		++ds;	/* skip '-' or ' ' */

		m_month=atoi(ds);
		ds+=3;
		m_day=atoi(ds);
		ds+=2;

		m_hour=0;
		m_minute=0;
		m_second=0;

		if(ds[0]=='T' || (ds[0]==' ' && strstr(ds,":")) )
		{
			++ds;
			m_hour=atoi(ds);
			ds=strstr(ds,":");
			if(!ds)
				return(true);
			++ds;
			m_minute=atoi(ds);
			ds=strstr(ds,":");
			if(!ds)
				return(true);
			++ds;
			m_second=atoi(ds);
		}
	}
	return(true);
}

/* extract the filedate from a filename */

bool kGUIDate::GetFileDate(const char *filename)
{
	long filedate;
	time_t time_of_day;
	struct tm *tinfo;

	filedate=kGUI::FileTime(filename);
	if(!filedate)
		return(false);

	time_of_day = filedate;
	tinfo=localtime( &time_of_day );
	
	m_day=tinfo->tm_mday;
	m_month=tinfo->tm_mon+1;
	m_year=tinfo->tm_year+1900;

	m_hour=tinfo->tm_hour;
	m_minute=tinfo->tm_min;
	m_second=tinfo->tm_sec;
	return(true);
}

void kGUIDate::SetToday(void)
{
	time_t time_of_day;
	struct tm *tinfo;

	time_of_day = time( NULL );
	tinfo=localtime( &time_of_day );
	
	m_day=tinfo->tm_mday;
	m_month=tinfo->tm_mon+1;
	m_year=tinfo->tm_year+1900;

	m_hour=tinfo->tm_hour;
	m_minute=tinfo->tm_min;
	m_second=tinfo->tm_sec;
}

void kGUIDate::Set(int d,int m,int y)
{
	m_day=d;
	m_month=m;
	m_year=y;
}

/* return 0=6 for weekday */
int kGUIDate::GetDayofWeek(void)
{
	time_t ntime;
	struct tm *tinfo;
	struct tm ti;

	/* convert d,m,y to numeric value */
	memset(&ti,0,sizeof(ti));
	ti.tm_mday=m_day;
	ti.tm_mon=m_month-1;
	ti.tm_year=m_year-1900;
	ntime=mktime(&ti);

	tinfo=localtime( &ntime );	/* convert back to d/m/y */
	return(tinfo->tm_wday);
}

void kGUIDate::AddSeconds(int numseconds)
{
	time_t ntime;
	struct tm *tinfo;
	struct tm ti;

	/* convert d,m,y to numeric value */
	memset(&ti,0,sizeof(ti));
	ti.tm_mday=m_day;
	ti.tm_mon=m_month-1;
	ti.tm_year=m_year-1900;

	ti.tm_hour=m_hour;
	ti.tm_min=m_minute;
	ti.tm_sec=m_second;

	ntime=mktime(&ti);

	ntime+=numseconds;

	tinfo=localtime( &ntime );	/* convert back to d/m/y */
	if(!tinfo)
		return;	/* cannot add time as it would be <1969 or >2038, leave as is */
//	passert(tinfo!=0,"Error: invalid time %d!\n",ntime);

	m_day=tinfo->tm_mday;
	m_month=tinfo->tm_mon+1;
	m_year=tinfo->tm_year+1900;
	m_hour=tinfo->tm_hour;
	m_minute=tinfo->tm_min;
	m_second=tinfo->tm_sec;

}


void kGUIDate::AddDays(int numdays)
{
	time_t ntime;
	struct tm *tinfo;
	struct tm ti;

	/* convert d,m,y to numeric value */
	memset(&ti,0,sizeof(ti));
	ti.tm_mday=m_day;
	ti.tm_mon=m_month-1;
	ti.tm_year=m_year-1900;
	ntime=mktime(&ti);
	passert(ntime!=0,"Error, not a valid day (Y/M/D) %d-%d-%d\n",m_year,m_month,m_day);

	ntime+=numdays*(60*60*24);	/* add or subtract days */

	tinfo=localtime( &ntime );	/* convert back to d/m/y */
	passert(tinfo!=0,"Error, not a valid day (Y/M/D) %d-%d-%d + %d days\n",m_year,m_month,m_day,numdays);
	m_day=tinfo->tm_mday;
	m_month=tinfo->tm_mon+1;
	m_year=tinfo->tm_year+1900;
}

/* get difference between dates in seconds */
int kGUIDate::GetDiffSeconds(kGUIDate *d2)
{
	time_t ntime1;
	struct tm ti1;
	time_t ntime2;
	struct tm ti2;

	/* convert d,m,y to numeric value */
	memset(&ti1,0,sizeof(ti1));
	memset(&ti2,0,sizeof(ti2));

	ti1.tm_mday=m_day;
	ti1.tm_mon=m_month-1;
	ti1.tm_year=m_year-1900;
	ti1.tm_hour=m_hour;
	ti1.tm_min=m_minute;
	ti1.tm_sec=m_second;
	ntime1=mktime(&ti1);

	ti2.tm_mday=d2->m_day;
	ti2.tm_mon=d2->m_month-1;
	ti2.tm_year=d2->m_year-1900;
	ti2.tm_hour=d2->m_hour;
	ti2.tm_min=d2->m_minute;
	ti2.tm_sec=d2->m_second;
	ntime2=mktime(&ti2);

	/* return delta in seconds */
	return((int)(ntime2-ntime1));
}

int kGUIDate::GetDiffMinutes(kGUIDate *d2)
{
	return(GetDiffSeconds(d2)/60);
}

int kGUIDate::GetDiffHours(kGUIDate *d2)
{
	return(GetDiffSeconds(d2)/(60*60));
}

int kGUIDate::GetDiffDays(kGUIDate *d2)
{
	return(GetDiffSeconds(d2)/(60*60*24));
}

void kGUIDate::ShortDate(kGUIString *s)
{
	assert(m_month!=0 && m_day!=0,"Invalid Date");
	s->Sprintf("%d-%02d-%02d",m_year,m_month,m_day);
}

void kGUIDate::MedDate(kGUIString *s)
{
	assert(m_month!=0 && m_day!=0,"Invalid Date");
	s->Sprintf("%c%c%c %d, %d",monthnames[m_month-1][0],monthnames[m_month-1][1],monthnames[m_month-1][2],m_day,m_year);
}

void kGUIDate::LongDate(kGUIString *s)
{
	assert(m_month!=0 && m_day!=0,"Invalid Date");
	s->Sprintf("%s %d, %d",monthnames[m_month-1],m_day,m_year);
}

void kGUIDate::ShortDateTime(kGUIString *s,bool use24)
{
	kGUIString t;

	assert(m_month!=0 && m_day!=0,"Invalid Date");
	s->Sprintf("%d-%02d-%02d",m_year,m_month,m_day);
	Time(&t,use24);
	s->Append(" ");
	s->Append(&t);
}


void kGUIDate::Time(kGUIString *s,bool use24)
{
	int h,ampm;
	static const char *ampmstring[]={"am","pm"};

	if(use24==true)
		s->Sprintf("%02d:%02d:%02d",m_hour,m_minute,m_second);
	else
	{
		if(m_hour<12)
		{
			ampm=0;
			h=m_hour;
		}
		else
		{
			ampm=1;
			h=m_hour-12;
		}
		if(!h)
			h=12;
		s->Sprintf("%d:%02d %s",h,m_minute,ampmstring[ampm]);
	}
}

char lc(char c)
{
	if(c>='A' && c<='Z')
		return((c-'A')+'a');
	return(c);
}

char uc(char c)
{
	if(c>='a' && c<='z')
		return((c-'a')+'A');
	return(c);
}

char *strstri(const char *lword,const char *sword)
{
	int i,j,p;
	int delta;
	int llen=(int)strlen(lword);
	int slen=(int)strlen(sword);

	p=(llen-slen)+1;	/* number of possible positions to compare */
	if(p<0)
		return(0);	/* can't match! as lword needs to >=len of sword */

	for(j=0;j<p;++j)
	{
		delta=0;
		for(i=0;((i<slen) && (!delta));++i)
			delta=lc(lword[j+i])-lc(sword[i]);
		if(!delta)
			return((char *)(lword+j));	/* match occured here! */

	}
	return(0);	/* no matches */
}

unsigned char strcmpin(const char *lword,const char *sword,int n)
{
	int i;
	unsigned char delta;

	for(i=0;i<n;++i)
	{
		delta=lc(lword[i])-lc(sword[i]);
		if(delta)
			return(delta);
	}
	return(0);	/* match! */
}

//#define MAXESTRING 8192

void fatalerror(const char *string)
{
	FILE *ef;
	ef=fopen("errors.txt","a");
	if(ef)
	{
		fprintf(ef,"%s\n",string);
		fclose(ef);
	}
	printf("%s",string);
	exit(1);
}
