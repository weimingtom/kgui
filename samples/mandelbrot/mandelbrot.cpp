/*********************************************************************************/
/* Mandelbrot - kGUI sample program                                              */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguireq.h"
#include "kguisavemovie.h"
#include "kguimatrix.h"
#include "kguixml.h"
#include "_text.h"

#define APPNAME "Mandelbrot"
#define CONFIGNAME "mancfg.xml"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

/* todo: num colors and palette for saving movie */
/* todo: custom width/height for save jpg */
/* todo: translate to other languages */

/* class for rendering a frame */

#define MAXCORES 8
typedef long double ldouble;

class Palette
{
public:
	unsigned int m_numcolors;
	Array<kGUIColor>m_colors;
	void Blend(int s,int e);
	void Adjust(int n);
	void Load(kGUIXMLItem *root);
	void Save(kGUIXMLItem *root);
};

class PalGrid : public kGUIObj
{
public:
	void SetPalEdit(class PalEdit *pe) {m_pe=pe;}
	bool UpdateInput(void);
	void Draw(void);
	void CheckScroll(void);
private:
	class PalEdit *m_pe;
};

class ColorButtonObj : public kGUIButtonObj
{
public:
	ColorButtonObj() {}
	~ColorButtonObj() {}
	void Draw(void);
private:
};

#define DEFCOLX 10
#define DEFCOLY 2

class PalEdit
{
public:
	friend class PalGrid;
	PalEdit(Palette *pal);
private:
	CALLBACKGLUEPTR(PalEdit,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	void UpdateGridSize(void);
	void GetNumColors(kGUIString *s,int button);
	void Load(kGUIFileReq *req,int pressed);
	void Save(kGUIFileReq *req,int pressed);
	CALLBACKGLUEPTRVAL(PalEdit,GetNumColors,kGUIString,int);
	CALLBACKGLUEPTRVAL(PalEdit,Load,kGUIFileReq,int);
	CALLBACKGLUEPTRVAL(PalEdit,Save,kGUIFileReq,int);

	static kGUIColor m_defrgb[DEFCOLX*DEFCOLY];
	Palette *m_pal;
	kGUIWindowObj m_window;
	kGUIScrollContainerObj m_scrollpalgrid;
	PalGrid m_palgrid;
	kGUIMenuObj m_menu;
	kGUIMenuColObj m_editmenu;
	kGUITextObj m_hcap;
	kGUITextObj m_scap;
	kGUITextObj m_vcap;
	kGUITextObj m_rcap;
	kGUITextObj m_gcap;
	kGUITextObj m_bcap;

	kGUIScrollInputBoxObj m_h;
	kGUIScrollInputBoxObj m_s;
	kGUIScrollInputBoxObj m_v;
	kGUIScrollInputBoxObj m_r;
	kGUIScrollInputBoxObj m_g;
	kGUIScrollInputBoxObj m_b;

	kGUIScrollBarObj m_hscroll;
	kGUIScrollBarObj m_sscroll;
	kGUIScrollBarObj m_vscroll;
	kGUIScrollBarObj m_rscroll;
	kGUIScrollBarObj m_gscroll;
	kGUIScrollBarObj m_bscroll;

	ColorButtonObj m_defcolors[DEFCOLX*DEFCOLY];

	bool m_adjust;
	unsigned int m_scursor;
	unsigned int m_ecursor;
};

enum
{
FRACTAL_MANDELBROT,
FRACTAL_JULIA
};

class MandelbrotFrame
{
public:
	MandelbrotFrame() {m_abort=false;m_w=0;m_h=0;m_fractal=FRACTAL_MANDELBROT;m_juliareal=-1.0f;m_juliaimag=0.0f;}
	void Init(int w,int h);
	void SetFractal(unsigned int f) {m_fractal=f;}
	void SetPalette(Palette *palette) {m_palette=palette;}
	void Draw(bool clear,ldouble x,ldouble y,ldouble zoom,int numcores);
	void SetAbort(bool abort) {m_abort=abort;}
	kGUIImageObj *GetImage(void) {return &m_image;}
	void Draw(void);
	ldouble GetJuliaReal(void) {return m_juliareal;}
	ldouble GetJuliaImag(void) {return m_juliaimag;}
	void SetJuliaReal(ldouble v) {m_juliareal=v;}
	void SetJuliaImag(ldouble v) {m_juliaimag=v;}
private:
	CALLBACKGLUE(MandelbrotFrame,Draw)
	int m_w;
	int m_h;
	unsigned int m_fractal;
	Palette *m_palette;

	bool m_abort;
	kGUIDrawSurface m_surface;
	kGUIImageObj m_image;
	kGUIThread m_thread[MAXCORES];
	ldouble m_juliareal;
	ldouble m_juliaimag;
	ldouble m_x;
	ldouble m_y;
	ldouble m_zoom;
	volatile int m_core;
	int m_numcores;
};

class Save : public kGUIWindowObj, kGUISaveMovie
{
public:
	Save(bool movie,int numkeypoints,kGUIVector3 *keypoints,Palette *pal,int numcores);
	~Save();
private:
	void Event(kGUIEvent *event);
	void BusyEvent(void);
	void PointOnCurve(kGUIVector3 *out, ldouble position, kGUIVector3 *plist,int num);
	void ReqDone(kGUIFileReq *req,int pressed);
	void SaveThread(void);

	/* function needed for Save */
	kGUIImage *RenderFrame(int framenum);

	/* static glue for callbacks */
	CALLBACKGLUEPTR(Save,Event,kGUIEvent)
	CALLBACKGLUE(Save,BusyEvent)
	CALLBACKGLUEPTRVAL(Save,ReqDone,kGUIFileReq,int);
	CALLBACKGLUE(Save,SaveThread)

	/* gui objects */
	kGUIControlBoxObj m_layout;				/* layout control for other objects */
	kGUITextObj m_inputnamecaption;
	kGUIInputBoxObj m_inputname;			/* filename to save movie to */
	kGUIButtonObj m_browsename;				/* popup for selecting a filename */
	kGUITextObj m_inputwidthcaption;
	kGUIInputBoxObj m_inputwidth;			/* pixel width for movie */
	kGUITextObj m_inputheightcaption;
	kGUIInputBoxObj m_inputheight;			/* pixel height for movie */
	kGUITextObj m_inputtweencaption;
	kGUIInputBoxObj m_inputtween;			/* tween frames between keypoints */
	kGUITextObj m_inputfpscaption;
	kGUIInputBoxObj m_inputfps;				/* frames per second for movie */
	kGUITextObj m_info;
	kGUITextObj m_info2;
	kGUIBusyRectObj m_busy;					/* busy bar while saving is active */
	kGUIButtonObj m_cancel;					/* cancel save button */
	kGUIButtonObj m_start;					/* start saving button */

	int m_numkeypoints;
	kGUIVector3 *m_keypoints;

	/* positions for movie */
	bool m_working;
	bool m_close;
	bool m_abort;
	kGUIThread m_savethread;
	MandelbrotFrame m_frame;
	int m_lastbw;
	int m_maxbw;
	int m_cur;
	int m_num;
	int m_lcur;
	kGUIDate m_starttime;
	Array<kGUIVector3>m_points;

	int m_numcolors;
	Palette *m_palette;
	int m_numcores;
	bool m_movie;
};

class Credits
{
public:
	Credits();
private:
	CALLBACKGLUEPTR(Credits,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	kGUIWindowObj m_window;
	kGUITextObj m_name;
	kGUITextObj m_desc;
};

class Help
{
public:
	Help();
private:
	CALLBACKGLUEPTR(Help,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	kGUIWindowObj m_window;
	kGUITextObj m_text;
};

class MandelbrotSample
{
public:
	MandelbrotSample();
	~MandelbrotSample();
	void StartUpdate(void);
private:
	void TimerEvent(void);
	void Event(kGUIEvent *event);
	void Update(void);
	void CalcEdges(ldouble *xmin,ldouble *xmax,ldouble *ymin,ldouble *ymax);
	void AddTrack(void) {kGUIVector3 t;t.m_x=m_x;t.m_y=m_y;t.m_z=m_zoom;m_track.SetEntry(m_tracknum++,t);}
	void GetZoom(kGUIString *s,int button);
	void GetJulia(kGUIString *s,int button);
	void DoSaveImage(kGUIFileReq *req,int pressed);
	void CancelUpdate(void);
	
	CALLBACKGLUE(MandelbrotSample,TimerEvent)
	CALLBACKGLUEPTR(MandelbrotSample,Event,kGUIEvent)
	CALLBACKGLUE(MandelbrotSample,Update);
	CALLBACKGLUEPTRVAL(MandelbrotSample,GetZoom,kGUIString,int);
	CALLBACKGLUEPTRVAL(MandelbrotSample,GetJulia,kGUIString,int);
	CALLBACKGLUEPTRVAL(MandelbrotSample,DoSaveImage,kGUIFileReq,int);

	bool m_dirty;
	kGUIThread m_updatethread;
	MandelbrotFrame m_frame;
	kGUIImageObj *m_image;
	kGUIMenuObj m_menu;
	kGUIMenuColObj m_filemenu;
	kGUIMenuColObj m_editmenu;
	kGUIMenuColObj m_helpmenu;
//	kGUIMenuColObj m_coremenu;
	kGUIMenuColObj m_fractalmenu;
	int m_et;
	ldouble m_x,m_y,m_zoom;
	ldouble m_zoomscale;

	Palette m_palette;

	int m_numcores;
	/* save clicked points/zoom into a tracklog */
	int m_tracknum;
	Array<kGUIVector3>m_track;
};

MandelbrotSample *g_mbs=0;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf",false);	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf",true);	/* use default font inside kgui for bold */
//	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
//	kGUI::LoadFont("fontb.ttf");	/* use default bold font */
	kGUI::SetDefFontSize(18);
	kGUI::SetDefReportFontSize(18);
	kGUIXMLCODES::Init();

	g_mbs=new MandelbrotSample();
}

void AppClose(void)
{
	delete g_mbs;
	kGUIXMLCODES::Purge();
}

enum
{
MENU_SAVEIMAGE,
MENU_SAVE,
MENU_QUIT,
MENU_CLEARTRACK,
MENU_UNDOTRACK,
MENU_SETZOOM,
MENU_SETMODE,
MENU_SETMANDELBROT,
MENU_SETJULIA,
MENU_EDITPALETTE,
MENU_SETJULIAVALUES,
#if 0
MENU_SETNUMCORES,
MENU_NUMCORES1,
MENU_NUMCORES2,
MENU_NUMCORES4,
MENU_NUMCORES8,
#endif
MENU_SHOWHELP,
MENU_CREDITS};

MandelbrotSample::MandelbrotSample()
{
	kGUIWindowObj *bg=kGUI::GetBackground();
	int w;
	int h,mh;
	bool config;
	kGUIXML prefs;

	m_et=0;
	/* get pointer to the background window object */
	bg=kGUI::GetBackground();
	bg->SetMinSize(640,480); 
	bg->SetTitle("MandelbrotSample");

	w=bg->GetChildZoneW();
	h=bg->GetChildZoneH();

	m_zoom=1.5f;
	m_x=-0.5f;
	m_y=0.0f;
	m_zoomscale=0.5f;

#if defined(WIN32) || defined(MINGW)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo( &sysinfo );
		m_numcores = sysinfo.dwNumberOfProcessors;
	}
#elif defined(LINUX)
	 m_numcores = sysconf( _SC_NPROCESSORS_ONLN );
#else
	m_numcores=2;
#endif

	m_dirty=false;

	m_menu.SetFontID(1);
	m_menu.SetNumEntries(3);
	m_menu.GetTitle(0)->SetString("File");
	m_menu.GetTitle(1)->SetString("Edit");
	m_menu.GetTitle(2)->SetString("Help");
	m_menu.SetEntry(0,&m_filemenu);
	m_menu.SetEntry(1,&m_editmenu);
	m_menu.SetEntry(2,&m_helpmenu);
	m_menu.SetEventHandler(this,CALLBACKNAME(Event));

	m_filemenu.SetIconWidth(22);
	m_editmenu.SetIconWidth(22);
	m_helpmenu.SetIconWidth(22);

	m_filemenu.SetNumEntries(3);
	m_filemenu.SetEntry(0,"Save Image",MENU_SAVEIMAGE);
	m_filemenu.SetEntry(1,"Save Movie",MENU_SAVE);
	m_filemenu.SetEntry(2,"Quit",MENU_QUIT);

	m_editmenu.SetNumEntries(6);
	m_editmenu.SetEntry(0,"Clear Track",MENU_CLEARTRACK);
	m_editmenu.SetEntry(1,"Undo",MENU_UNDOTRACK);
	m_editmenu.SetEntry(2,"Set Zoom",MENU_SETZOOM);
	m_editmenu.SetEntry(3,"Edit Palette",MENU_EDITPALETTE);
	m_editmenu.SetEntry(4,"Set Fractal Mode",MENU_SETMODE);
	m_editmenu.GetEntry(4)->SetSubMenu(&m_fractalmenu);
	m_editmenu.SetEntry(5,"Set Julia Values",MENU_SETJULIAVALUES);

	m_fractalmenu.SetNumEntries(2);
	m_fractalmenu.SetEntry(0,"Mandelbrot",MENU_SETMANDELBROT);
	m_fractalmenu.SetEntry(1,"Julia",MENU_SETJULIA);

#if 0
	m_editmenu.SetEntry(4,"Set Number CPU Cores",MENU_SETNUMCORES);
	m_editmenu.GetEntry(4)->SetSubMenu(&m_coremenu);

	m_coremenu.SetNumEntries(4);
	m_coremenu.SetEntry(0," 1 ",MENU_NUMCORES1);
	m_coremenu.SetEntry(1," 2 ",MENU_NUMCORES2);
	m_coremenu.SetEntry(2," 4 ",MENU_NUMCORES4);
	m_coremenu.SetEntry(3," 8 ",MENU_NUMCORES8);
#endif

	m_helpmenu.SetNumEntries(2);
	m_helpmenu.SetEntry(0,"Show Help",MENU_SHOWHELP);
	m_helpmenu.SetEntry(1,"Credits",MENU_CREDITS);

	m_menu.SetPos(0,0);
	bg->AddObject(&m_menu);

	prefs.SetFilename(CONFIGNAME);
	config=prefs.Load();
	if(config)
	{
		/* load previous settings */
		kGUIXMLItem *xroot;

		xroot=prefs.GetRootItem()->Locate("mandelbrot");
		if(xroot)
		{
			xroot=xroot->Locate("palette");
			if(xroot)
				m_palette.Load(xroot);
		}
	}
	else
	{
		m_palette.m_numcolors=8;
		m_palette.m_colors.Alloc(8);
		m_palette.m_colors.SetEntry(0,DrawColor(255,0,0));
		m_palette.m_colors.SetEntry(1,DrawColor(255,255,0));
		m_palette.m_colors.SetEntry(2,DrawColor(0,255,0));
		m_palette.m_colors.SetEntry(3,DrawColor(0,255,255));
		m_palette.m_colors.SetEntry(4,DrawColor(0,0,255));
		m_palette.m_colors.SetEntry(5,DrawColor(218,112,214));
		m_palette.m_colors.SetEntry(6,DrawColor(255,255,255));
		m_palette.m_colors.SetEntry(7,DrawColor(0,0,0));
		m_palette.Adjust(512);
	}

	mh=m_menu.GetZoneH();
	h-=mh;
	m_frame.Init(w,h);
	m_frame.SetPalette(&m_palette);

	m_image=m_frame.GetImage();
	m_image->SetPos(0,m_menu.GetZoneH());
	m_image->SetSize(w,h);
	m_image->SetEventHandler(this,CALLBACKNAME(Event));

	bg->AddObject(m_image);

	kGUI::ShowWindow();
	kGUI::AddUpdateTask(this,CALLBACKNAME(TimerEvent));
	bg->SetEventHandler(this,CALLBACKNAME(Event));

	m_tracknum=0;
	m_track.Init(1024,256);
	AddTrack();	/* add starting point to the track now */

	/* start the thread to update the image */
	StartUpdate();

	/* if no config was found then show help when starting */
	if(config==false)
		new Help();
}

void MandelbrotSample::StartUpdate(void)
{
	/* if thread is currently running then abort it */
	CancelUpdate();
	m_dirty=true;
	m_updatethread.Start(this,CALLBACKNAME(Update));
}

/* this function is called within it's own thread */
void MandelbrotSample::Update(void)
{
	m_frame.Draw(true,m_x,m_y,m_zoom,m_numcores);
	m_updatethread.Close(false);
}

/* don't allow close if thread is currently active */
void MandelbrotSample::Event(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_SELECTED:
		switch(event->m_value[0].i)
		{
		case MENU_SAVEIMAGE:
		{
			Save *sm;

			sm=new Save(false,m_tracknum,m_track.GetArrayPtr(),&m_palette,m_numcores);
//			kGUIFileReq *fr;
//			fr=new kGUIFileReq(FILEREQ_SAVE,"picture.jpg","jpg",this,CALLBACKNAME(DoSaveImage));
		}
		break;
		case MENU_SAVE:
			if(m_tracknum<2)
			{
				kGUIMsgBoxReq *req;

				req=new kGUIMsgBoxReq(MSGBOX_OK,false,"Must have a minimum of 2 points before saving!");
			}
			else
			{
				Save *sm;

				sm=new Save(true,m_tracknum,m_track.GetArrayPtr(),&m_palette,m_numcores);
			}
		break;
		case MENU_QUIT:
			kGUI::CloseApp();
		break;
		case MENU_CLEARTRACK:
			if(m_tracknum>1)
			{
				m_tracknum=0;
				m_zoom=1.5f;
				m_x=-0.5f;
				m_y=0.0f;
				AddTrack();
				StartUpdate();
			}
		break;
		case MENU_UNDOTRACK:
			if(m_tracknum>1)
			{
				kGUIVector3 v;

				--m_tracknum;
				v=m_track.GetEntry(m_tracknum-1);
				m_x=v.m_x;
				m_y=v.m_y;
				m_zoom=v.m_z;
				StartUpdate();
			}
		break;
		case MENU_SETZOOM:
		{
			kGUIInputBoxReq *input;
			kGUIString def;

			input=new kGUIInputBoxReq(this,CALLBACKNAME(GetZoom),"Input new zoom scale (between 0 and 1)?");
			def.Sprintf("%f",(double)m_zoomscale);
			input->SetDefault(def.GetString());
		}
		break;
		case MENU_SETJULIAVALUES:
		{
			kGUIInputBoxReq *input;
			kGUIString def;

			input=new kGUIInputBoxReq(this,CALLBACKNAME(GetJulia),"Input Julia Values (real, imaginary)?");
			def.Sprintf("%f,%f",(double)m_frame.GetJuliaReal(),(double)m_frame.GetJuliaImag());
			input->SetDefault(def.GetString());
		}
		break;
		case MENU_SETMANDELBROT:
			CancelUpdate();
			m_frame.SetFractal(FRACTAL_MANDELBROT);
			StartUpdate();
		break;
		case MENU_SETJULIA:
			CancelUpdate();
			m_frame.SetFractal(FRACTAL_JULIA);
			StartUpdate();
		break;
		case MENU_EDITPALETTE:
		{
			PalEdit *p;

			p=new PalEdit(&m_palette);
		}
		break;
#if 0
		case MENU_NUMCORES1:
			m_numcores=1;
		break;
		case MENU_NUMCORES2:
			m_numcores=2;
		break;
		case MENU_NUMCORES4:
			m_numcores=4;
		break;
		case MENU_NUMCORES8:
			m_numcores=8;
		break;
#endif
		case MENU_SHOWHELP:
		{
			Help *c;

			c=new Help();
		}
		break;
		case MENU_CREDITS:
		{
			Credits *c;

			c=new Credits();
		}
		break;
		}
	break;
	case EVENT_SIZECHANGED:
		if(event->GetObj()==kGUI::GetBackground())
		{
			kGUIWindowObj *bg=kGUI::GetBackground();
			int wcw,wch;

			/* resize the image to match the new smaller window */
			wcw=bg->GetChildZoneW();
			wch=bg->GetChildZoneH();

			CancelUpdate();
			m_frame.Init(wcw-m_image->GetZoneX(),wch-m_image->GetZoneY());
			StartUpdate();
		}
	break;
	case EVENT_LEFTCLICK:
		if(event->GetObj()==m_image)
		{
			kGUICorners c;
			ldouble fx,fy;
			ldouble xmin,xmax,ymin,ymax;

			m_image->GetCorners(&c);
			CalcEdges(&xmin,&xmax,&ymin,&ymax);
			/* center on mouse and zoom in */
			fx=(ldouble)(kGUI::GetMouseX()-c.lx)/(c.rx-c.lx);
			fy=(ldouble)(kGUI::GetMouseY()-c.ty)/(c.by-c.ty);
			m_x=xmin+fx*(xmax-xmin);
			m_y=ymin+fy*(ymax-ymin);
			m_zoom*=m_zoomscale;

			AddTrack();
			StartUpdate();
		}
	break;
	case EVENT_RIGHTCLICK:
		if(event->GetObj()==m_image)
		{
			kGUICorners c;
			ldouble fx,fy;
			ldouble xmin,xmax,ymin,ymax;

			m_image->GetCorners(&c);
			CalcEdges(&xmin,&xmax,&ymin,&ymax);
			/* center on mouse and zoom out */
			fx=(ldouble)(kGUI::GetMouseX()-c.lx)/(c.rx-c.lx);
			fy=(ldouble)(kGUI::GetMouseY()-c.ty)/(c.by-c.ty);
			m_x=xmin+fx*(xmax-xmin);
			m_y=ymin+fy*(ymax-ymin);
			m_zoom*=(1.0f/m_zoomscale);

			AddTrack();
			StartUpdate();
		}
	break;
	}
}

void MandelbrotSample::GetZoom(kGUIString *s,int button)
{
	if(button==MSGBOX_OK && s->GetLen())
	{
		m_zoomscale=s->GetDouble();
		if(m_zoomscale<=0.0f || m_zoomscale>=1.0f)
			m_zoomscale=0.5f;
	}
}

void MandelbrotSample::GetJulia(kGUIString *s,int button)
{
	if(button==MSGBOX_OK && s->GetLen())
	{
		kGUIStringSplit ss;

		/* get 2 values */
		if(ss.Split(s,",")==2)
		{
			CancelUpdate();
			m_frame.SetJuliaReal(ss.GetWord(0)->GetDouble());
			m_frame.SetJuliaImag(ss.GetWord(1)->GetDouble());
			StartUpdate();
		}
	}
}


void MandelbrotSample::DoSaveImage(kGUIFileReq *req,int pressed)
{
	if(pressed==MSGBOX_OK)
	{
		DataHandle outdh;

		/* save current image */
		outdh.SetFilename(req->GetFilename());
		m_image->SaveJPGImage(&outdh,100);
	}
}

void MandelbrotSample::CancelUpdate(void)
{
	if(m_updatethread.GetActive())
	{
		m_frame.SetAbort(true);
		/* wait for it to finish */
		while(m_updatethread.GetActive());
		m_frame.SetAbort(false);
	}
}

MandelbrotSample::~MandelbrotSample()
{
	/* save settings to config file */
	kGUIXML prefs;
	kGUIXMLItem *xroot;

	/* generate the XML file for our saved settings */
	prefs.SetEncoding(ENCODING_UTF8);
	xroot=prefs.GetRootItem()->AddChild("mandelbrot");
	m_palette.Save(xroot);
	prefs.SetFilename(CONFIGNAME);
	prefs.Save();

	/* make sure update thread is finished */
	CancelUpdate();

	kGUI::DelUpdateTask(this,CALLBACKNAME(TimerEvent));
}

void MandelbrotSample::CalcEdges(ldouble *xmin,ldouble *xmax,ldouble *ymin,ldouble *ymax)
{
	xmin[0]=m_x-m_zoom;
	xmax[0]=m_x+m_zoom;
	ymin[0]=m_y-m_zoom;
	ymax[0]=m_y+m_zoom;
}

void MandelbrotSample::TimerEvent(void)
{
	bool draw=false;

	m_et+=kGUI::GetET();
	while(m_et>(TICKSPERSEC/10))
	{
		m_et-=TICKSPERSEC/10;
		draw=true;
	}

	/* keep redrawing image as long as it is still being updated */
	/* but cap re-draws to 30 fps */
	if(draw)
	{
		if(m_dirty)
			m_image->Dirty();

		m_dirty=m_updatethread.GetActive();
	}
}

/***************************************************************************/

void MandelbrotFrame::Init(int w,int h)
{
	m_h=h;
	m_w=w;
	m_surface.Init(w,h);
	m_surface.Clear(DrawColor(255,0,0));
	m_image.SetMemImage(0,GUISHAPE_SURFACE,m_surface.GetWidth(),m_surface.GetHeight(),m_surface.GetBPP(),(const unsigned char *)m_surface.GetSurfacePtr(0,0));
	m_image.SetSize(w,h);
}


void MandelbrotFrame::Draw(bool clear,ldouble dx,ldouble dy,ldouble zoom,int numcores)
{
	int i;
	bool finished;
	if(clear)
		m_surface.Clear(m_palette->m_colors.GetEntry(0));

	m_numcores=numcores;
	m_x=dx;
	m_y=dy;
	m_zoom=zoom;

	for(i=0;i<m_numcores;++i)
	{
		m_core=i;
		m_thread[i].Start(this,CALLBACKNAME(Draw));
		while(m_core==i);
	}

	/* now, wait for all threads to finish */
	do
	{
		finished=true;
		for(i=0;i<m_numcores;++i)
		{
			if(m_thread[i].GetActive()==true)
			{
				finished=false;
				kGUI::Sleep(1);
			}
		}
	}while(finished==false);
}

void MandelbrotFrame::Draw(void)
{
	int x,y,i,core;
	int nc=m_palette->m_numcolors;
	kGUIColor *cp=m_palette->m_colors.GetArrayPtr();
	kGUIColor *sp;
	ldouble creal;
	ldouble cimag;
	ldouble zreal;
	ldouble zimag;
	ldouble isquared;
	ldouble rsquared;
	ldouble xch;
	ldouble ych;
	ldouble xmin;
	ldouble xzoom;
	ldouble yzoom;

	core=m_core;
	m_core=-1;

	/* correct perspective for non square */
	if(m_w>m_h)
	{
		yzoom=m_zoom;
		xzoom=((ldouble)m_w/m_h)*m_zoom;
	}
	else
	{
		xzoom=m_zoom;
		yzoom=((ldouble)m_h/m_w)*m_zoom;
	}
	xmin=m_x-xzoom;

	xch=(xzoom*2)/m_w;		/* xch is the width of each pixel */
	ych=(yzoom*2)/m_h;		/* ych is the height of each pixel */

	switch(m_fractal)
	{
	case FRACTAL_MANDELBROT:
		/* mandelbrot set */
		cimag=m_y-yzoom;
		cimag+=ych*core;
		ych=ych*m_numcores;
		for (y=core;y<m_h;y+=m_numcores)
		{
			if(m_abort)
				goto done;

			sp=m_surface.GetSurfacePtr(0,y);
			creal=xmin;
			for (x=0;x<m_w;x++)
			{
				zimag=0.0f;
				zreal=0.0f;
				rsquared = 0.0f;
				isquared = 0.0f;
				for (i=0;i<nc;i++)
				{
					zimag = zreal * zimag * 2;
					zimag += cimag;
					zreal = rsquared - isquared;
					zreal += creal;

					rsquared = zreal * zreal;
					isquared = zimag * zimag;

					if ((rsquared + isquared) > 4.0f)
					{
						*(sp++)=cp[i];
						goto nextmx;
					}
				}
				*(sp++)=DrawColor(0,0,0);
nextmx:			creal+=xch;
			}
			cimag+=ych;
		}
	break;
	case FRACTAL_JULIA:
	{
		ldouble tempreal;
		ldouble tempimag;

		creal=m_juliareal;
		cimag=m_juliaimag;
//		creal=0.3f;
//		cimag=0.6f;
//		creal=0.687f;
//		cimag=0.312f;

		zimag=m_y-yzoom;
		zimag+=ych*core;
		ych=ych*m_numcores;
		for (y=core;y<m_h;y+=m_numcores)
		{
			if(m_abort)
				goto done;

			sp=m_surface.GetSurfacePtr(0,y);
			zreal=xmin;
			for (x=0;x<m_w;x++)
			{
				tempimag=zimag;
				tempreal=zreal;
				rsquared = tempreal * tempreal;
				isquared = tempimag * tempimag;
				for (i=0;i<nc;i++)
				{
					tempimag = tempreal * tempimag * 2;
					tempimag += cimag;
					tempreal = rsquared - isquared;
					tempreal += creal;

					rsquared = tempreal * tempreal;
					isquared = tempimag * tempimag;

					if ((rsquared + isquared) > 4.0f)
					{
						*(sp++)=cp[i];
						goto nextjx;
					}
				}
				*(sp++)=DrawColor(0,0,0);
nextjx:			zreal+=xch;
			}
			zimag+=ych;
		}
	}
	break;
	}

done:;
	m_thread[core].Close(false);
}

/***************************************************************************/

Save::Save(bool movie,int numkeypoints,kGUIVector3 *keypoints,Palette *palette,int numcores)
{
	unsigned int w,h,gap;
	int bold=1;
	kGUIWindowObj *bg=kGUI::GetBackground();

	/* save what we need to generate the movie */
	m_movie=movie;
	m_numcores=numcores;
	m_numkeypoints=numkeypoints;
	m_keypoints=keypoints;
	m_working=false;
	m_close=false;
	m_abort=false;
	
	m_palette=palette;

	if(m_movie)
	{
		m_inputname.SetString("movie.mpg");
		SetTitle("Save Movie");
	}
	else
	{
		m_inputname.SetString("picture.jpg");
		SetTitle("Save Image");
	}
	SetSize(0,0);		/* set initial window size to 0,0 */
	m_layout.SetPos(0,0);

	/* defaults */
	
	m_inputwidth.SetInt(800);
	m_inputheight.SetInt(600);
	m_inputtween.SetInt(20);
	m_inputfps.SetInt(30);

	/* todo: handle localization */
	m_inputnamecaption.SetString("Name");
	m_inputwidthcaption.SetString("Width");
	m_inputheightcaption.SetString("Height");
	if(m_movie)
	{
		m_inputtweencaption.SetString("Between Frames");
		m_inputfpscaption.SetString("Frames per Second");
		m_start.SetString("Start");
	}
	else
		m_start.SetString("Save");

	m_cancel.SetString("Cancel");

	/* calc length of longest one */
	w=m_inputnamecaption.GetWidth();
	w=MAX(w,m_inputwidthcaption.GetWidth());
	w=MAX(w,m_inputheightcaption.GetWidth());
	if(m_movie)
	{
		w=MAX(w,m_inputtweencaption.GetWidth());
		w=MAX(w,m_inputfpscaption.GetWidth());
	}
	w+=32;

	h=m_inputnamecaption.GetLineHeight()+6;

	/* add input name line */
	m_inputnamecaption.SetFontID(bold);
	m_inputnamecaption.SetPos(0,0);
	m_inputname.SetPos(w,0);
	m_inputname.SetSize(330,h);
	m_layout.AddObjects(2,&m_inputnamecaption,&m_inputname);
	m_browsename.SetString("...");
	m_browsename.SetFontID(bold);
	m_browsename.SetSize(m_browsename.GetWidth()+16,h);
	m_layout.AddObject(&m_browsename);
	m_layout.NextLine();

	/* add input width line */
	m_inputwidthcaption.SetFontID(bold);
	m_inputwidthcaption.SetPos(0,0);
	m_inputwidth.SetPos(w,0);
	m_inputwidth.SetSize(100,h);
	m_layout.AddObjects(2,&m_inputwidthcaption,&m_inputwidth);
	m_layout.NextLine();

	/* add input height line */
	m_inputheightcaption.SetFontID(bold);
	m_inputheightcaption.SetPos(0,0);
	m_inputheight.SetPos(w,0);
	m_inputheight.SetSize(100,h);
	m_layout.AddObjects(2,&m_inputheightcaption,&m_inputheight);
	m_layout.NextLine();

	if(m_movie)
	{
		/* add input tween line */
		m_inputtweencaption.SetFontID(bold);
		m_inputtweencaption.SetPos(0,0);
		m_inputtween.SetPos(w,0);
		m_inputtween.SetSize(100,h);
		m_layout.AddObjects(2,&m_inputtweencaption,&m_inputtween);
		m_layout.NextLine();

		/* add input fps line */
		m_inputfpscaption.SetFontID(bold);
		m_inputfpscaption.SetPos(0,0);
		m_inputfps.SetPos(w,0);
		m_inputfps.SetSize(100,h);
		m_layout.AddObjects(2,&m_inputfpscaption,&m_inputfps);
		m_layout.NextLine();

		/* add info line */
		m_info.SetFontID(bold);
		m_info.SetSize(m_layout.GetZoneW(),h);
		m_layout.AddObject(&m_info);
		m_layout.NextLine();
		/* add info line */
		m_info2.SetFontID(bold);
		m_info2.SetSize(m_layout.GetZoneW(),h);
		m_layout.AddObject(&m_info2);
		m_layout.NextLine();

		m_lcur=-1;

		/* add the busy rectangle */
		m_maxbw=m_lastbw=m_layout.GetZoneW();
		m_busy.SetSize(m_lastbw,h);
		m_layout.AddObject(&m_busy);
		m_layout.NextLine();
	}

	/* add the cancel and start button */

	m_cancel.SetSize(m_cancel.GetWidth()+32,h);
	m_start.SetSize(m_start.GetWidth()+32,h);
	m_layout.AddObject(&m_cancel);
	m_layout.AddObject(&m_start);
	m_layout.NextLine();

	/* get max width so far so we can right align the cancel/start */
	gap=m_layout.GetChildZoneW()-m_start.GetZoneRX();
	m_cancel.SetZoneX(m_cancel.GetZoneX()+gap);
	m_start.SetZoneX(m_start.GetZoneX()+gap);

	/* add layout object to the window */
	AddObject(&m_layout);
	/* expand the window to fit the layout object */
	ExpandToFit();
	/* center the window */
	SetPos((bg->GetZoneW()-GetZoneW())/2,(bg->GetZoneH()-GetZoneH())/2);

	/* add our event handlers for handling button presses etc. */
	SetEventHandler(this,CALLBACKNAME(Event));
	m_inputname.SetEventHandler(this,CALLBACKNAME(Event));
	m_browsename.SetEventHandler(this,CALLBACKNAME(Event));
	m_inputwidth.SetEventHandler(this,CALLBACKNAME(Event));
	m_inputheight.SetEventHandler(this,CALLBACKNAME(Event));
	if(m_movie)
	{
		m_inputtween.SetEventHandler(this,CALLBACKNAME(Event));
		m_inputfps.SetEventHandler(this,CALLBACKNAME(Event));
	}
	m_cancel.SetEventHandler(this,CALLBACKNAME(Event));
	m_start.SetEventHandler(this,CALLBACKNAME(Event));

	/* add window to display */
	SetTop(true);
	kGUI::AddWindow(this);
	/* add busy event handler */
	kGUI::AddUpdateTask(this,CALLBACKNAME(BusyEvent));
}

void Save::ReqDone(kGUIFileReq *req,int pressed)
{
	if(pressed==MSGBOX_OK)
		m_inputname.SetString(req->GetFilename());
}

void Save::BusyEvent(void)
{
	int w;

	if(m_working==false)
		w=0;
	else
		w=(int)(((ldouble)m_maxbw*(ldouble)m_cur)/(ldouble)m_num);
	if(w!=m_lastbw)
	{
		/* only re-draw if it changes the pixel width */
		m_busy.SetZoneW(w);
		m_lastbw=w;
		m_busy.Dirty();
	}
	if(m_working)
	{
		if(m_cur!=m_lcur)
		{
			kGUIDate now;
			int elapsed;
			int estimate;
			kGUIString e1s;
			kGUIString e2s;

			now.SetToday();
			elapsed=m_starttime.GetDiffSeconds(&now);
			kGUIDate::PrintElapsed(elapsed,&e1s);

			estimate=(int)(((ldouble)m_num/m_cur)*elapsed);
			if(estimate<0)
				estimate=0;
			kGUIDate::PrintElapsed(estimate,&e2s);

			m_info.Sprintf("Generating Frame # %d of %d",m_cur,m_num);
			m_info2.Sprintf("Elapsed time=%s, Estimate=%s",e1s.GetString(),e2s.GetString());
			m_lcur=m_cur;
		}
	}
	if(m_close==true)
		Close();
}

void Save::Event(kGUIEvent *event)
{
	kGUIObj *obj;
	int value;

	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::DelUpdateTask(this,CALLBACKNAME(BusyEvent));
		if(m_movie)
		{
			m_frame.SetAbort(true);
			m_abort=true;
			while(m_savethread.GetActive());
		}
		delete this;
	break;
	case EVENT_PRESSED:
		obj=event->GetObj();
		if(obj==&m_browsename)
		{
			kGUIFileReq *fr;

			if(m_movie)
				fr=new kGUIFileReq(FILEREQ_SAVE,m_inputname.GetString(),".mov;.mpg;.avi",this,CALLBACKNAME(ReqDone));
			else
				fr=new kGUIFileReq(FILEREQ_SAVE,m_inputname.GetString(),".jpg",this,CALLBACKNAME(ReqDone));
		}
		else if(obj==&m_cancel)
			Close();
		else if(obj==&m_start)
		{
			if(m_movie)
			{
				m_working=true;

				/* lock all editable fields now */
				m_inputname.SetLocked(true);
				m_browsename.SetEnabled(false);
				m_inputwidth.SetLocked(true);
				m_inputheight.SetLocked(true);
				m_inputtween.SetLocked(true);
				m_inputfps.SetLocked(true);

				m_start.SetEnabled(false);

				m_starttime.SetToday();
				m_savethread.Start(this,CALLBACKNAME(SaveThread));
			}
			else
			{
				kGUIVector3 v;

				m_frame.Init(m_inputwidth.GetInt(),m_inputheight.GetInt());
				m_frame.SetPalette(m_palette);

				v=m_keypoints[m_numkeypoints-1];

				m_frame.Draw(false,v.m_x,v.m_y,v.m_z,m_numcores);
				{
					DataHandle outdh;

					outdh.SetFilename(m_inputname.GetString());
					m_frame.GetImage()->SaveJPGImage(&outdh,100);
				}
				Close();
			}
		}
	break;
	case EVENT_AFTERUPDATE:
		obj=event->GetObj();
		if(obj==&m_inputwidth)
		{
			value=m_inputwidth.GetInt();
			if(value<64)
				m_inputwidth.SetInt(64);		/* min 64 wide */
			else if(value&1)
				m_inputwidth.SetInt(value+1);	/* make even */
		}
		else if(obj==&m_inputheight)
		{
			value=m_inputheight.GetInt();
			if(value<64)
				m_inputheight.SetInt(64);		/* min 64 wide */
			else if(value&1)
				m_inputheight.SetInt(value+1);	/* make even */
		}
		else if(obj==&m_inputtween)
		{
			value=m_inputtween.GetInt();
			if(value<0)
				m_inputtween.SetInt(0);			/* min 0 */
		}
		else if(obj==&m_inputfps)
		{
			value=m_inputfps.GetInt();
			if(value<1)
				m_inputfps.SetInt(1);		/* min 1 fps */
			else if(value>60)
				m_inputfps.SetInt(60);		/* max 60 fps */
		}
	break;
	}
}

/* this is running in it's own thread */
void Save::SaveThread(void)
{
	int i;
	int fps,w,h;
	kGUIVector3 v;

	/* build list of points for saving */
	m_num=m_numkeypoints*(m_inputtween.GetInt()+1);
	m_points.Alloc(m_num);
	for(i=0;i<m_num;++i)
	{
		PointOnCurve(&v,(ldouble)i/m_num,m_keypoints,m_numkeypoints);
		m_points.SetEntry(i,v);
	}

	/* allocate our save image generating each frame into */
	w=m_inputwidth.GetInt();
	h=m_inputheight.GetInt();
	fps=m_inputfps.GetInt();

	m_frame.Init(w,h);
	m_frame.SetPalette(m_palette);

	/* start saving the movie */
	SaveMovie(fps,w,h,m_inputname.GetString());

	m_savethread.Close(false);
	if(m_abort==false)
		m_close=true;
}

void Save::PointOnCurve(kGUIVector3 *out, ldouble position, kGUIVector3 *plist,int num)
{
	int segment;
	ldouble frac;
	ldouble f2;
	ldouble f3;
	kGUIVector3 *p0;
	kGUIVector3 *p1;
	kGUIVector3 *p2;
	kGUIVector3 *p3;

	segment=(int)(position*(num-1));
	frac=(position*(num-1))-segment;
	f2 = frac * frac;
	f3 = f2 * frac;

	if(segment)
		p0=plist+segment-1;
	else
		p0=plist;
	p1=plist+segment;
	p2=plist+segment+1;
	if(segment==(num-2))
		p3=p2;
	else
		p3=plist+segment+2;
	out->m_x = 0.5f * ( ( 2.0f * p1->m_x ) + ( -p0->m_x + p2->m_x ) * frac + ( 2.0f * p0->m_x - 5.0f * p1->m_x + 4 * p2->m_x - p3->m_x ) * f2 + ( -p0->m_x + 3.0f * p1->m_x - 3.0f * p2->m_x + p3->m_x ) * f3 );
	out->m_y = 0.5f * ( ( 2.0f * p1->m_y ) + ( -p0->m_y + p2->m_y ) * frac + ( 2.0f * p0->m_y - 5.0f * p1->m_y + 4 * p2->m_y - p3->m_y ) * f2 + ( -p0->m_y + 3.0f * p1->m_y - 3.0f * p2->m_y + p3->m_y ) * f3 );
	out->m_z = 0.5f * ( ( 2.0f * p1->m_z ) + ( -p0->m_z + p2->m_z ) * frac + ( 2.0f * p0->m_z - 5.0f * p1->m_z + 4 * p2->m_z - p3->m_z ) * f2 + ( -p0->m_z + 3.0f * p1->m_z - 3.0f * p2->m_z + p3->m_z ) * f3 );
}

kGUIImage *Save::RenderFrame(int framenum)
{
	kGUIVector3 v;

	if(m_abort)
		return(0);

	m_cur=framenum;

	if(framenum<m_num)
	{
		v=m_points.GetEntry(framenum);

		m_frame.Draw(false,v.m_x,v.m_y,v.m_z,m_numcores);
		return(m_frame.GetImage());
	}
	else
		return(0);
}


Save::~Save()
{
	/* if the save thread is currently active then we need to abort it */
	if(m_savethread.GetActive())
	{
		m_abort=true;
		/* wait for thread to finish aborting */
		while(m_savethread.GetActive());
	}
}

/****************************************************************/

Credits::Credits()
{
	int dh;
	int warea;

	m_window.SetAllowButtons(WINDOWBUTTON_CLOSE);
	m_window.SetSize(600,100);
	m_name.SetPos(0,0);
	m_name.SetFontSize(20);
	m_name.SetString("kGUI Mandelbrot Sample v1.0");
	m_name.SetColor(DrawColor(255,0,0));
	m_window.AddObject(&m_name);
	
	warea=m_window.GetChildZoneW()-10;
	m_desc.SetFontSize(14);
	m_desc.SetPos(0,m_name.GetLineHeight());
	m_desc.SetString("Programmed by Kevin Pickell\r\n");
	m_desc.ASprintf("Started: Nov 2008, Current Build: %s\r\n",__DATE__);
	m_desc.ASprintf("(C) LGPL - GNU Lesser General Public License\r\n\r\n");
	m_desc.ASprintf("Libraries/Code included in this program:\r\n\r\n");
	m_desc.ASprintf("kGUI: %s\r\n",kGUI::GetVersion());

	m_desc.ASprintf("FreeType: %s\r\n",kGUI::GetFTVersion());
	m_desc.ASprintf("JpegLib: %d%c\r\n",kGUI::GetJpegVersion()/10,(char)(kGUI::GetJpegVersion()%10)+'a'-1);
	
//	m_desc.ASprintf("ZLIB: %s\r\n",ZLIB_VERSION);
	m_desc.ASprintf("ffmpeg: %s\r\n",kGUISaveMovie::GetVersion());

	m_desc.CalcLineList(warea);
	dh=m_desc.CalcHeight(warea);
	m_desc.SetSize(warea,dh);
	m_window.AddObject(&m_desc);

	m_window.SetTitle("Credits");
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetTop(true);
	m_window.ExpandToFit();
	m_window.Center();

	kGUI::AddWindow(&m_window);
}

void Credits::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

/****************************************************************/

Help::Help()
{
	int dh;
	int warea;

	m_window.SetAllowButtons(WINDOWBUTTON_CLOSE);
	m_window.SetSize((int)(kGUI::GetBackground()->GetZoneW()*0.8f),100);
	
	warea=m_window.GetChildZoneW()-10;
	m_text.SetPos(0,0);

	m_text.SetString("Click with the left mouse button to center image and zoom in\n"
					 "Click with the right mouse button to center image and zoom out.\n"
					 "\n"
					 "Easy Palette Editing\n"
					 "Select the Palette Editor from the File Menu\n"
					 "Set the number of colors to 8 using Set Number of Colors from the Palette Editor Menu\n"
					 "Click on each color in the box and then a color from the quick color buttons\n"
					 "Set the number of colors to 512 using the Adjust Number of Colors from the Palette Editor Menu\n"
					 "Close the Palette Editor");

	m_text.CalcLineList(warea);
	dh=m_text.CalcHeight(warea);
	m_text.SetSize(warea,dh);
	m_window.AddObject(&m_text);

	m_window.SetTitle("Help");
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetTop(true);
	m_window.ExpandToFit();
	m_window.Center();

	kGUI::AddWindow(&m_window);
}

void Help::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

/*****************************************************************/

#define BEDGE 1

void ColorButtonObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	/* if I am not the current child then unpress me now! */
	if(GetParent())
	{
		if(GetParent()->GetCurrentChild()!=this)
			ClearPressed();
	}

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRectBevel(c.lx,c.ty,c.rx,c.by,GetPressed());
		kGUI::DrawRect(c.lx+BEDGE,c.ty+BEDGE,c.rx-BEDGE,c.by-BEDGE,GetColor());
	}
	kGUI::PopClip();
}
#define PALBOXSIZE 20

enum
{
PALMENU_SETNUMCOLORS,
PALMENU_SETNUMCOLORS2,
PALMENU_BLENDRANGE,
PALMENU_LOAD,
PALMENU_SAVE};

kGUIColor PalEdit::m_defrgb[DEFCOLX*DEFCOLY]={
	DrawColor(128,0,0),
	DrawColor(255,0,0),
	DrawColor(0,128,0),
	DrawColor(0,255,0),
	DrawColor(0,0,128),
	DrawColor(0,0,255),
	DrawColor(128,128,0),
	DrawColor(255,255,0),
	DrawColor(128,0,128),
	DrawColor(255,0,255),
	DrawColor(0,128,128),
	DrawColor(0,255,255),
	DrawColor(255,69,0),	/* orange */
	DrawColor(139,69,19),	/* saddle brown */
	DrawColor(218,112,214),	/* orchid */
	DrawColor(0,0,0),
	DrawColor(64,64,64),
	DrawColor(128,128,128),
	DrawColor(192,192,192),
	DrawColor(255,255,255)};

PalEdit::PalEdit(Palette *pal)
{
	int x,y;
	int i,bx,by;

	m_pal=pal;
	m_window.SetAllowButtons(WINDOWBUTTON_CLOSE);
	m_window.SetSize(600,100);

	m_palgrid.SetPalEdit(this);
	m_palgrid.SetPos(0,0);
	UpdateGridSize();
	m_scrollpalgrid.AddObject(&m_palgrid);

	m_scrollpalgrid.SetPos(0,0);
	m_scrollpalgrid.SetInsideSize(16*PALBOXSIZE,16*PALBOXSIZE);
	m_window.AddObject(&m_scrollpalgrid);

	m_menu.SetFontID(1);
	m_menu.SetNumEntries(1);
	m_menu.GetTitle(0)->SetString("Menu");
	m_menu.SetEntry(0,&m_editmenu);
	m_menu.SetEventHandler(this,CALLBACKNAME(WindowEvent));

	m_editmenu.SetIconWidth(22);
	m_editmenu.SetNumEntries(5);
	m_editmenu.SetEntry(0,"Set Number of Colors",PALMENU_SETNUMCOLORS);
	m_editmenu.SetEntry(1,"Adjust Number of Colors",PALMENU_SETNUMCOLORS2);
	m_editmenu.SetEntry(2,"Blend Range",PALMENU_BLENDRANGE);
	m_editmenu.SetEntry(3,"Load Palette",PALMENU_LOAD);
	m_editmenu.SetEntry(4,"Save Palette",PALMENU_SAVE);

	x=m_scrollpalgrid.GetZoneRX();
	m_menu.SetPos(x,m_palgrid.GetZoneY());
	m_window.AddObject(&m_menu);

	m_hcap.SetFontID(1);
	m_hcap.SetString("H");
	m_hcap.SetPos(x,m_menu.GetZoneBY()+4);
	m_window.AddObject(&m_hcap);

	m_h.SetFontSize(12);
	m_h.SetSize(76,18);
	m_h.SetMaxLen(3);
	m_h.SetInputType(GUIINPUTTYPE_INT);
	m_h.SetPos(x+24,m_menu.GetZoneBY()+4+2);
	m_h.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_h);

	m_hscroll.SetHorz();
	m_hscroll.SetPos(x+24+76+16,m_menu.GetZoneBY()+4+2);
	m_hscroll.SetSize(128,18);
	m_hscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_hscroll);

	m_scap.SetFontID(1);
	m_scap.SetString("S");
	m_scap.SetPos(x,m_hcap.GetZoneBY()+4);
	m_window.AddObject(&m_scap);

	m_s.SetFontSize(12);
	m_s.SetSize(76,18);
	m_s.SetMaxLen(3);
	m_s.SetInputType(GUIINPUTTYPE_INT);
	m_s.SetPos(x+24,m_hcap.GetZoneBY()+4+2);
	m_s.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_s);

	m_sscroll.SetHorz();
	m_sscroll.SetPos(x+24+76+16,m_hcap.GetZoneBY()+4+2);
	m_sscroll.SetSize(128,18);
	m_sscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_sscroll);

	m_vcap.SetFontID(1);
	m_vcap.SetString("V");
	m_vcap.SetPos(x,m_scap.GetZoneBY()+4);
	m_window.AddObject(&m_vcap);

	m_v.SetFontSize(12);
	m_v.SetSize(76,18);
	m_v.SetMaxLen(3);
	m_v.SetInputType(GUIINPUTTYPE_INT);
	m_v.SetPos(x+24,m_scap.GetZoneBY()+4+2);
	m_v.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_v);

	m_vscroll.SetHorz();
	m_vscroll.SetPos(x+24+76+16,m_scap.GetZoneBY()+4+2);
	m_vscroll.SetSize(128,18);
	m_vscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_vscroll);

	m_rcap.SetFontID(1);
	m_rcap.SetString("R");
	m_rcap.SetPos(x,m_vcap.GetZoneBY()+4);
	m_window.AddObject(&m_rcap);

	m_r.SetFontSize(12);
	m_r.SetSize(76,18);
	m_r.SetMaxLen(3);
	m_r.SetInputType(GUIINPUTTYPE_INT);
	m_r.SetPos(x+24,m_vcap.GetZoneBY()+4+2);
	m_r.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_r);

	m_rscroll.SetHorz();
	m_rscroll.SetPos(x+24+76+16,m_vcap.GetZoneBY()+4+2);
	m_rscroll.SetSize(128,18);
	m_rscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_rscroll);

	m_gcap.SetFontID(1);
	m_gcap.SetString("G");
	m_gcap.SetPos(x,m_rcap.GetZoneBY()+4);
	m_window.AddObject(&m_gcap);

	m_g.SetFontSize(12);
	m_g.SetSize(76,18);
	m_g.SetMaxLen(3);
	m_g.SetInputType(GUIINPUTTYPE_INT);
	m_g.SetPos(x+24,m_rcap.GetZoneBY()+4+2);
	m_g.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_g);

	m_gscroll.SetHorz();
	m_gscroll.SetPos(x+24+76+16,m_rcap.GetZoneBY()+4+2);
	m_gscroll.SetSize(128,18);
	m_gscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_gscroll);

	m_bcap.SetFontID(1);
	m_bcap.SetString("B");
	m_bcap.SetPos(x,m_gcap.GetZoneBY()+4);
	m_window.AddObject(&m_bcap);

	m_b.SetFontSize(12);
	m_b.SetSize(76,18);
	m_b.SetMaxLen(3);
	m_b.SetInputType(GUIINPUTTYPE_INT);
	m_b.SetPos(x+24,m_gcap.GetZoneBY()+4+2);
	m_b.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_b);

	m_bscroll.SetHorz();
	m_bscroll.SetPos(x+24+76+16,m_gcap.GetZoneBY()+4+2);
	m_bscroll.SetSize(128,18);
	m_bscroll.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.AddObject(&m_bscroll);

	/* default color buttons */

	i=0;
	y=m_bcap.GetZoneBY()+4+2;
	for(by=0;by<DEFCOLY;++by)
	{
		for(bx=0;bx<DEFCOLX;++bx)
		{
			m_defcolors[i].SetPos(x+bx*26,y+by*22);
			m_defcolors[i].SetSize(24,20);
			m_defcolors[i].SetColor(m_defrgb[i]);
			m_defcolors[i].SetEventHandler(this,CALLBACKNAME(WindowEvent));
			m_window.AddObject(&m_defcolors[i]);
			++i;
		}
	}

	m_window.SetTitle("Palette Editor");
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetTop(true);
	m_window.ExpandToFit();
	m_window.Center();

	kGUI::AddWindow(&m_window);
	m_scursor=0;
	m_ecursor=0;
	m_palgrid.CheckScroll();
}

void PalEdit::UpdateGridSize(void)
{
	int rows;

	rows=m_pal->m_numcolors/16;
	if(m_pal->m_numcolors&15)
		++rows;
	if(rows<16)
		rows=16;
	m_palgrid.SetSize(16*PALBOXSIZE,rows*PALBOXSIZE);
	m_scrollpalgrid.SetMaxHeight(rows*PALBOXSIZE);
}

void PalEdit::WindowEvent(kGUIEvent *event)
{
	kGUIObj *obj=event->GetObj();
	kGUIColor c;
	int r,g,b;
	int h,s,v;
	unsigned char ur;
	unsigned char ug;
	unsigned char ub;

	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		/* since palette can have changed, trigger a redraw */
		g_mbs->StartUpdate();
		delete this;
	break;
	case EVENT_AFTERUPDATE:
		if((obj==&m_r) || (obj==&m_g) || (obj==&m_b))
		{
			r=MIN(255,MAX(0,m_r.GetInt()));
			g=MIN(255,MAX(0,m_g.GetInt()));
			b=MIN(255,MAX(0,m_b.GetInt()));
			c=DrawColor(r,g,b);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
		else if((obj==&m_h) || (obj==&m_s) || (obj==&m_v))
		{
			h=MIN(255,MAX(0,m_h.GetInt()));
			s=MIN(255,MAX(0,m_s.GetInt()));
			v=MIN(255,MAX(0,m_v.GetInt()));
			kGUI::HSVToRGB(h/255.0f,s/255.0f,v/255.0f,&ur,&ug,&ub);

			c=DrawColor(ur,ug,ub);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
		else if((obj==&m_rscroll) || (obj==&m_gscroll) || (obj==&m_bscroll))
		{
			r=m_r.GetInt();
			if(obj==&m_rscroll)
				r+=event->m_value[0].i;
			g=m_g.GetInt();
			if(obj==&m_gscroll)
				g+=event->m_value[0].i;
			b=m_b.GetInt();
			if(obj==&m_bscroll)
				b+=event->m_value[0].i;
			r=MIN(255,MAX(0,r));
			g=MIN(255,MAX(0,g));
			b=MIN(255,MAX(0,b));
			c=DrawColor(r,g,b);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
		else if((obj==&m_hscroll) || (obj==&m_sscroll) || (obj==&m_vscroll))
		{
			h=m_h.GetInt();
			if(obj==&m_hscroll)
				h+=event->m_value[0].i;
			s=m_s.GetInt();
			if(obj==&m_sscroll)
				s+=event->m_value[0].i;
			v=m_v.GetInt();
			if(obj==&m_vscroll)
				v+=event->m_value[0].i;
			h=MIN(255,MAX(0,h));
			s=MIN(255,MAX(0,s));
			v=MIN(255,MAX(0,v));

			kGUI::HSVToRGB(h/255.0f,s/255.0f,v/255.0f,&ur,&ug,&ub);
			c=DrawColor(ur,ug,ub);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
	break;
	case EVENT_PRESSED:
		if((obj==&m_r) || (obj==&m_g) || (obj==&m_b))
		{
			r=m_r.GetInt();
			if(obj==&m_r)
				r+=event->m_value[0].i;
			g=m_g.GetInt();
			if(obj==&m_g)
				g+=event->m_value[0].i;
			b=m_b.GetInt();
			if(obj==&m_b)
				b+=event->m_value[0].i;
			r=MIN(255,MAX(0,r));
			g=MIN(255,MAX(0,g));
			b=MIN(255,MAX(0,b));
			c=DrawColor(r,g,b);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
		else if((obj==&m_h) || (obj==&m_s) || (obj==&m_v))
		{
			h=m_h.GetInt();
			if(obj==&m_h)
				h+=event->m_value[0].i;
			s=m_s.GetInt();
			if(obj==&m_s)
				s+=event->m_value[0].i;
			v=m_v.GetInt();
			if(obj==&m_v)
				v+=event->m_value[0].i;
			h=MIN(255,MAX(0,h));
			s=MIN(255,MAX(0,s));
			v=MIN(255,MAX(0,v));
			kGUI::HSVToRGB(h/255.0f,s/255.0f,v/255.0f,&ur,&ug,&ub);
			c=DrawColor(ur,ug,ub);
			m_pal->m_colors.SetEntry(m_ecursor,c);
			m_palgrid.Dirty();
			m_palgrid.CheckScroll();
		}
		else
		{
			for(unsigned int i=0;i<(DEFCOLX*DEFCOLY);++i)
			{
				if(obj==&m_defcolors[i])
				{
					c=m_defrgb[i];
					m_pal->m_colors.SetEntry(m_ecursor,c);
					m_palgrid.Dirty();
					m_palgrid.CheckScroll();
					break;
				}
			}
		}

	break;
	case EVENT_SELECTED:
		switch(event->m_value[0].i)
		{
		case PALMENU_SETNUMCOLORS:
			m_adjust=false;
			goto getc;
		break;
		case PALMENU_SETNUMCOLORS2:
			m_adjust=true;
			{
getc:;
				kGUIInputBoxReq *input;
				kGUIString def;

				input=new kGUIInputBoxReq(this,CALLBACKNAME(GetNumColors),"Input Number of Colors?");
				def.Sprintf("%d",m_pal->m_numcolors);
				input->SetDefault(def.GetString());
			}
		break;
		case PALMENU_BLENDRANGE:
		{
			int s,e;

			if(m_scursor<m_ecursor)
			{
				s=m_scursor;
				e=m_ecursor;
			}
			else
			{
				s=m_ecursor;
				e=m_scursor;
			}
			if(s!=e)
			{
				m_pal->Blend(s,e);
				m_palgrid.Dirty();
				m_palgrid.CheckScroll();
			}
		}
		break;
		case PALMENU_LOAD:
		{
			kGUIFileReq *fr;

			fr=new kGUIFileReq(FILEREQ_OPEN,0,".pal",this,CALLBACKNAME(Load));
		}
		break;
		case PALMENU_SAVE:
		{
			kGUIFileReq *fr;

			fr=new kGUIFileReq(FILEREQ_SAVE,"palette.pal",".pal",this,CALLBACKNAME(Save));
		}
		break;
		}
	break;
	}
}

void PalEdit::Load(kGUIFileReq *req,int pressed)
{

	if(pressed==MSGBOX_OK)
	{
		kGUIXML xml;
		xml.SetFilename(req->GetFilename());
		if(xml.Load()==true)
		{
			kGUIXMLItem *xroot;

			xroot=xml.GetRootItem()->Locate("mandelbrot");
			if(xroot)
			{
				xroot=xroot->Locate("palette");
				if(xroot)
				{
					m_pal->Load(xroot);
					/* ok, redraw */
					m_palgrid.Dirty();
					m_palgrid.CheckScroll();
					return;
				}
			}
		}
		/* print, error opening file */
	}
}

void PalEdit::Save(kGUIFileReq *req,int pressed)
{
	if(pressed==MSGBOX_OK)
	{
		kGUIXML xml;
		kGUIXMLItem *xroot;

		/* generate the XML file for our saved settings */
		xml.SetEncoding(ENCODING_UTF8);
		xroot=xml.GetRootItem()->AddChild("mandelbrot");
		m_pal->Save(xroot);

		/* save the xml file */
		xml.SetFilename(req->GetFilename());
		xml.Save();
	}
}

void PalEdit::GetNumColors(kGUIString *s,int button)
{
	if(button==MSGBOX_OK && s->GetLen())
	{
		int nc=MAX(1,s->GetInt());
	
		if(m_adjust)
			m_pal->Adjust(nc);
		else
			m_pal->m_colors.Alloc(nc);
		m_pal->m_numcolors=nc;
		UpdateGridSize();
		m_scursor=0;
		m_ecursor=0;
		m_palgrid.Dirty();
		m_palgrid.CheckScroll();
	}
}

/* expand or shrink palette to new size */
void Palette::Adjust(int n)
{
	int oldnum=m_numcolors;
	int i,le,de;
	Array<kGUIColor>oldcolors;

	if(oldnum==1)
		return;

	/* copy old palette first */
	oldcolors.Alloc(oldnum);
	for(i=0;i<oldnum;++i)
		oldcolors.SetEntry(i,m_colors.GetEntry(i));

	m_colors.Alloc(n);
	le=0;
	for(i=0;i<oldnum;++i)
	{
		de=(int)((double)i*(n-1)/(oldnum-1));		/* dest entry slot */
		m_colors.SetEntry(de,oldcolors.GetEntry(i));
		if((de-1)>le)
			Blend(le,de);
		le=de;
	}
	m_numcolors=n;
}

/* interpolate colors between s and e */
void Palette::Blend(int s, int e)
{
	int i,num;

	kGUIColor c;
	int sr,sg,sb;
	int er,eg,eb;
	int dr,dg,db;
	int newr,newg,newb;

	c=m_colors.GetEntry(s);
	DrawColorToRGB(c,sr,sg,sb);
	c=m_colors.GetEntry(e);
	DrawColorToRGB(c,er,eg,eb);
	dr=er-sr;
	dg=eg-sg;
	db=eb-sb;
	num=e-s;
	for(i=0;i<num;++i)
	{
		newr=(int)(((double)dr*i/num)+sr+0.5f);
		newg=(int)(((double)dg*i/num)+sg+0.5f);
		newb=(int)(((double)db*i/num)+sb+0.5f);
		m_colors.SetEntry(s+i,DrawColor(newr,newg,newb));
	}
}

/* load palette from XML parent */
void Palette::Load(kGUIXMLItem *root)
{
	unsigned int i;
	kGUIXMLItem *xitem;
	kGUIColor c;
	int r,g,b;

	m_numcolors=root->GetNumChildren();
	m_colors.Alloc(m_numcolors);
	for(i=0;i<m_numcolors;++i)
	{
		xitem=root->GetChild(i);
		r=xitem->Locate("r")->GetValueInt();
		g=xitem->Locate("g")->GetValueInt();
		b=xitem->Locate("b")->GetValueInt();
		c=DrawColor(r,g,b);
		m_colors.SetEntry(i,c);
	}
}

/* save palette to XML parent */
void Palette::Save(kGUIXMLItem *root)
{
	kGUIXMLItem *proot;
	kGUIXMLItem *xitem;
	unsigned int i;
	kGUIColor c;
	int r,g,b;

	proot=root->AddChild("palette");
	for(i=0;i<m_numcolors;++i)
	{
		c=m_colors.GetEntry(i);
		DrawColorToRGB(c,r,g,b);
		xitem=proot->AddChild("entry");
		xitem->AddParm("r",r);
		xitem->AddParm("g",g);
		xitem->AddParm("b",b);
	}
}

bool PalGrid::UpdateInput(void)
{
	bool over;
	kGUICorners c;

	GetCorners(&c);
	over=kGUI::MouseOver(&c);
	if(kGUI::GetMouseLeft())
	{
		if(over)
		{
			int offx,offy;

			if(this!=kGUI::GetActiveObj())
			{
				kGUI::PushActiveObj(this);
				SetCurrent();
			}
			/* move cursor to area under mouse */
			offx=kGUI::GetMouseX()-c.lx;
			offy=kGUI::GetMouseY()-c.ty;

			offx=offx/PALBOXSIZE;
			offy=offy/PALBOXSIZE;
			m_pe->m_ecursor=offy*16+offx;
			if(kGUI::GetKeyShift()==false && kGUI::GetMouseClickLeft())
				m_pe->m_scursor=m_pe->m_ecursor;
			CheckScroll();
			Dirty();
		}
		else
			kGUI::PopActiveObj();
	}

	switch(kGUI::GetKey())
	{
	case GUIKEY_LEFT:
		if(m_pe->m_ecursor)
		{
			--(m_pe->m_ecursor);
			if(kGUI::GetKeyShift()==false)
				m_pe->m_scursor=m_pe->m_ecursor;

			CheckScroll();
			Dirty();
			return(true);
		}
	break;
	case GUIKEY_RIGHT:
		if(m_pe->m_ecursor+1<m_pe->m_pal->m_numcolors)
		{
			++(m_pe->m_ecursor);
			if(kGUI::GetKeyShift()==false)
				m_pe->m_scursor=m_pe->m_ecursor;
			CheckScroll();
			Dirty();
			return(true);
		}
	break;
	case GUIKEY_UP:
		if(m_pe->m_ecursor>=16)
		{
			m_pe->m_ecursor-=16;
			if(kGUI::GetKeyShift()==false)
				m_pe->m_scursor=m_pe->m_ecursor;
			CheckScroll();
			Dirty();
			return(true);
		}
	break;
	case GUIKEY_DOWN:
		if(m_pe->m_ecursor+16<m_pe->m_pal->m_numcolors)
		{
			m_pe->m_ecursor+=16;
			if(kGUI::GetKeyShift()==false)
				m_pe->m_scursor=m_pe->m_ecursor;
			CheckScroll();
			Dirty();
			return(true);
		}
	break;
	}

	return(false);
}

void PalGrid::CheckScroll(void)
{
	int ctopy;
	int cbottomy;
	kGUIColor c;
	int r,g,b;
	int h,s,v;
	double dh,ds,dv;

	c=m_pe->m_pal->m_colors.GetEntry(m_pe->m_ecursor);
	DrawColorToRGB(c,r,g,b);

	/* load rgb value under cursor */
	m_pe->m_r.SetInt(r);
	m_pe->m_g.SetInt(g);
	m_pe->m_b.SetInt(b);

	m_pe->m_rscroll.SetValues(r,1,255-r);
	m_pe->m_rscroll.Dirty();
	m_pe->m_gscroll.SetValues(g,1,255-g);
	m_pe->m_gscroll.Dirty();
	m_pe->m_bscroll.SetValues(b,1,255-b);
	m_pe->m_bscroll.Dirty();

	kGUI::RGBToHSV(r,g,b,&dh,&ds,&dv);
	h=(int)(dh*255.0f);
	s=(int)(ds*255.0f);
	v=(int)(dv*255.0f);
	m_pe->m_h.SetInt(h);
	m_pe->m_s.SetInt(s);
	m_pe->m_v.SetInt(v);

	m_pe->m_hscroll.SetValues(h,1,255-h);
	m_pe->m_hscroll.Dirty();
	m_pe->m_sscroll.SetValues(s,1,255-s);
	m_pe->m_sscroll.Dirty();
	m_pe->m_vscroll.SetValues(v,1,255-v);
	m_pe->m_vscroll.Dirty();


	/* scroll to make sure cursor is on screen */
	ctopy=(m_pe->m_ecursor/16)*PALBOXSIZE;
	cbottomy=ctopy+PALBOXSIZE;

	if(ctopy<m_pe->m_scrollpalgrid.GetChildScrollY())
		m_pe->m_scrollpalgrid.GotoY(ctopy);
	else if(cbottomy>(m_pe->m_scrollpalgrid.GetChildScrollY()+(16*PALBOXSIZE)))
		m_pe->m_scrollpalgrid.GotoY(cbottomy-(16*PALBOXSIZE));
}

void PalGrid::Draw(void)
{
	kGUICorners c;
	Palette *p;

	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	if(kGUI::ValidClip())
	{
		unsigned int i;
		unsigned int x;
		unsigned int y;
		unsigned int scursor;
		unsigned int ecursor;

		p=m_pe->m_pal;
		
		scursor=m_pe->m_scursor;
		ecursor=m_pe->m_ecursor;
		if(ecursor<scursor)
		{
			ecursor=m_pe->m_scursor;
			scursor=m_pe->m_ecursor;
		}

		x=0;
		y=0;
		for(i=0;i<p->m_numcolors;++i)
		{
			kGUI::DrawRect(c.lx+x,c.ty+y,c.lx+x+PALBOXSIZE,c.ty+y+PALBOXSIZE,(i>=scursor && i<=ecursor)?DrawColor(255,0,0):DrawColor(0,0,0));
			kGUI::DrawRect(c.lx+x+1,c.ty+y+1,c.lx+x+PALBOXSIZE-1,c.ty+y+PALBOXSIZE-1,p->m_colors.GetEntry(i));
			x+=PALBOXSIZE;
			if(x==(16*PALBOXSIZE))
			{
				x=0;
				y+=PALBOXSIZE;
			}
		}
	}
}
