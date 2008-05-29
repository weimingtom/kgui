/*****************************************************************/
/**                                                             **/
/** kGUILinux                                                   **/
/**                                                             **/
/** This is the kGUI wrapper for Linux that uses X-windows      **/
/**  for the screen and input device handling                   **/
/** It uses cups (Common Unix printing System ) for printing    **/
/**                                                             **/
/*****************************************************************/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>


//printing
#include <cups/cups.h>
#include <cups/ppd.h>

#define DEBUGPRINT 0
#define USE_WM_DECORATION 0

void AppInit(void);
void AppClose(void);

#if USE_WM_DECORATION==0
typedef struct
{
unsigned long serial;
int x;
int y;
}movewindow_def;
#endif

class kGUIPrintJobCups : public kGUIPrintJob
{
public:
	kGUIPrintJobCups() {m_error=false;}
	~kGUIPrintJobCups() {}
	bool Start(kGUIPrinter *p,const char *jobname,int numpages,int numcopies,double ppi,double lm,double rm,double tm,double bm,kGUIDrawSurface *surface,bool landscape);
	void StartPage(void);
	void PrintSurface(kGUIDrawSurface *surface);
	void DrawRect(int lx,int rx,int ty,int by,kGUIColor color);
	void DrawTextList(int pointsize,const char *fontname,int num, Array<PRINTFONTCHAR_DEF> *list);
	void DrawImage(int lx,int rx,int ty,int by);
	void EndPage(void);
	bool End(void);
private:
	void DrawImage(kGUIDrawSurface *s,int lx,int rx,int ty,int by);
	kGUIString m_jobname;		/* name for report */
	kGUIString m_filename;		/* filename for postscript file */
	FILE *m_handle;			/* handle for postscript file */
	int m_numpages;
	int m_numcopies;
	int m_pagenum;

	kGUIString m_lastfont;
	int m_lastfontsize;
};


class kGUISystemX : public kGUISystem
{
public:
	kGUISystemX() {m_display=0;m_cprinters=0;}
	~kGUISystemX();
	bool Init(void);
#if USE_WM_DECORATION==0
	void HideDecoration(void);
#endif
	void Loop(void);
	void Draw(void);

	void FileShow(const char *filename);
	void ShellExec(const char *program,const char *parms,const char *dir);
	void Copy(kGUIString *s);
	void Paste(kGUIString *s);
	void Sleep(int ms);
	bool MakeDirectory(const char *name);
	long FileTime(const char *fn);
	void ReDraw(void);
	void ChangeMouse(void);
	void ShowMouse(bool show);
	void AdjustMouse(int dx,int dy) {/*printf("adjust dx=%d,dy=%d\n",dx,dy);*/m_mousex+=dx;m_mousey+=dy;}
	void GetWindowPos(int *x,int *y,int *w,int *h);
	void SetWindowPos(int x,int y);
	void SetWindowSize(int w,int h);
	void Minimize(void);
	bool IsDir(const char *fn);

	bool NeedRotatedSurfaceForPrinting(void) {return false;}
	unsigned int GetNumPrinters(void) {return m_numprinters;}
	void GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv);

	int GetDefPrinterNum(void) {return m_defprinternum;}
	kGUIPrinter *GetPrinterObj(int pid) {return m_printers.GetEntryPtr(pid);}

	class kGUIPrintJob *AllocPrintJob(void) {kGUIPrintJob *pj;pj=new kGUIPrintJobCups(); return pj;}

private:
	Display *m_display;
	int m_x11_fd;
	Window m_rootwin;
	Window m_win;
	int m_fullwidth,m_fullheight;
	int m_winx,m_winy;
	int m_winw,m_winh;
	Visual *m_visual;
	Colormap m_cmap;
	XEvent m_e;
	XImage *m_image;
	int m_screen;
	GC m_gc;
	int m_mousex,m_mousey,m_mousewheel;
	bool m_mouseleft,m_mouseright;
	int m_lastcursor;
	Atom m_wmp;
	Atom m_wdw;
	Atom m_wclip;
	kGUIString m_clipboard;
	bool m_delaypaste;
#if USE_WM_DECORATION==0
	int m_numoldwindowpositions;
	Array<movewindow_def>m_oldwindowpositions;
#endif
	//printer stuff for printing
	unsigned int m_numprinters;
	int m_defprinternum;
	ClassArray<kGUIPrinter>m_printers;
	cups_dest_t *m_cprinters;  
};

kGUISystemX *g_sys;
bool g_userabort=false;

static void sigint_handler(int sig)  //static declaration
{
//	printf("MANAGER : Signal Detected.! (%d)\n",sig);
	switch (sig)
	{  // interrupt signal detected
	case SIGINT: 
		g_userabort=true;
		printf("Break detected, program shutting down!\n");
	break;
	}
}

int main()
{
	signal(SIGINT, sigint_handler);

	g_sys=new kGUISystemX();
	
	if(g_sys->Init()==false)
	{
		fprintf(stderr, "ERROR: could not open display\n");
		exit(1);
	}
	g_sys->Loop();
	delete g_sys;
	return 0;
}

#define TOP 32
#define BOTTOM 32

bool kGUISystemX::Init(void)
{
	int startwidth,startheight;
	int maximages;
	XEvent e;
	XSetWindowAttributes attributes;

	m_mousex=0;
	m_mousey=0;
	m_mousewheel=0;
	m_mouseleft=false;
	m_mouseright=false;
	m_lastcursor=-1;
	m_delaypaste=false;

#if USE_WM_DECORATION==0
	/* if we are using our own decoration then we also have to control moving the window */
	/* around on the screen, in that case, because of delays inherent in the x-Windows system */
	/* we need to keep a list of the event id's when we asked to move the window position and */
	/* any mouse move events prior to that move time need to be adjusted so we know their */
	/* correct position relative to where the window currently is */
	m_numoldwindowpositions=0;
	m_oldwindowpositions.Init(8,2);
#endif

	if(!(m_display=XOpenDisplay(NULL)))
		return(false);
	m_screen = DefaultScreen(m_display);
	m_visual=DefaultVisual(m_display,m_screen);

	/* get the full screen size */
	m_fullwidth = DisplayWidth(m_display, m_screen);
	m_fullheight = DisplayHeight(m_display, m_screen)-(TOP+BOTTOM);

	/* this is the size of that app window, use fullscreen */
	/* unless overridden by the user requested dimensions */
#ifdef	DEFSCREENWIDTH
	startwidth=DEFSCREENWIDTH;
#else
	startwidth=m_fullwidth;
#endif
#ifdef	DEFSCREENHEIGHT
	startheight=DEFSCREENHEIGHT;
#else
	startheight=m_fullheight;
#endif
#ifdef DEFMAXIMAGES
	maximages=DEFMAXIMAGES;
#else
	maximages=200;
#endif

	m_winx=(m_fullwidth-startwidth)>>1;
	m_winy=(m_fullheight-startheight)>>1;
	m_winw=startwidth;
	m_winh=startheight;

	m_rootwin = RootWindow(m_display, m_screen);
	m_cmap = DefaultColormap(m_display, m_screen);
	m_x11_fd=ConnectionNumber(m_display);
#if 1
	attributes.colormap=m_cmap;
	attributes.event_mask=ExposureMask|ButtonPressMask|KeyPressMask|KeyReleaseMask|PointerMotionMask|ButtonReleaseMask|StructureNotifyMask;

	m_win=XCreateWindow(	m_display, m_rootwin, m_winx, m_winy, m_winw, m_winh, 0, 
				CopyFromParent,CopyFromParent,m_visual,CWColormap|CWEventMask,&attributes);
#else
	m_win=XCreateSimpleWindow(	m_display, m_rootwin, m_winx, m_winy, m_winw, m_winh, 0, 
					BlackPixel(m_display, m_screen), BlackPixel(m_display, m_screen));

#endif

	/* these 3 lines are needed to trap the window close event */
	m_wmp=XInternAtom(m_display,"WM_PROTOCOLS",false);
	m_wdw=XInternAtom(m_display,"WM_DELETE_WINDOW",false);
	XSetWMProtocols(m_display,m_win,&m_wdw,1);

	/* used by the clipboard */
	m_wclip=XInternAtom(m_display,"FL_CLIPBOARD",false);

	//todo: this need a kGUISystem api call for setting the same as the root window title	
	XStoreName(m_display, m_win, "GPS Turbo");

	m_gc=XCreateGC(m_display, m_win, 0, NULL);
	XSetForeground(m_display, m_gc, WhitePixel(m_display, m_screen));

	/* get number of printers and names of them */
	m_numprinters=cupsGetDests(&m_cprinters);
	m_defprinternum=0;

	if(m_numprinters)
	{
		unsigned int i;
		kGUIPrinter *p;

		m_printers.Init(m_numprinters,0);
		for(i=0;i<m_numprinters;++i)
		{
			p=m_printers.GetEntryPtr(i);
			p->SetName(m_cprinters[i].name);	/* also local 'instance' name or null too */
			if(m_cprinters[i].is_default)
				m_defprinternum=i;
		}
	}
	else
	{
		/* if no printers were found then just have 1 called "No Printers" so the */
		/* print pre-view window will not crash */
		m_numprinters=1;
		m_printers.Init(m_numprinters,0);
		m_printers.GetEntryPtr(0)->SetName("No Printers");
		m_printers.GetEntryPtr(0)->SetDefaultPageSize();
	}


	kGUI::Trace(" kGUI::Init(startwidth,startheight);\n");
	if(kGUI::Init(this,startwidth,startheight,m_fullwidth,m_fullheight,maximages)==false)
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return(false);
	}
#if USE_WM_DECORATION
	/* if we are using the window managers "Decoration" then we need to turn off ours for the */
	/* root window */
	kGUI::GetBackground()->SetFrame(false);
#else
	/* the Window Manager that is attached to the X-Windows system will automatically add */
	/* it's own "Decoration" to the window frame, we need to turn off the decoration since */
	/* we have our own. Also we can't just turn off the Redirect Flas since if we do that then */
	/* the window is not be properly layered with the other open windows and will always stay on top */
	/* and also then we cannot tab between applications either */
	HideDecoration();
#endif
	{
		kGUIDrawSurface *s;

		s=kGUI::GetCurrentSurface();
		m_image=XCreateImage(m_display,m_visual,24,ZPixmap,0,(char *)s->GetSurfacePtrABS(0,0),s->GetWidth(),s->GetHeight(),32,s->GetWidth()*4);
	}

	kGUI::Trace(" AppInit();\n");

	XMapWindow(m_display, m_win);
	XFlush(m_display);

	/* eat events until we get a MapNotify event saying out screen is up and ready to go! */
	do
	{
		XNextEvent(m_display,&e);
	}while(e.type!=MapNotify);


	/* the default position is not always set by CreateWindow so let's set it's position again! */
	SetWindowPos(m_winx,m_winy);
	XFlush(m_display);

	AppInit();

	return(true);
}

#if USE_WM_DECORATION==0

/* MWM decorations values */
 #define MWM_DECOR_NONE          0
 #define MWM_DECOR_ALL           (1L << 0)
 #define MWM_DECOR_BORDER        (1L << 1)
 #define MWM_DECOR_RESIZEH       (1L << 2)
 #define MWM_DECOR_TITLE         (1L << 3)
 #define MWM_DECOR_MENU          (1L << 4)
 #define MWM_DECOR_MINIMIZE      (1L << 5)
 #define MWM_DECOR_MAXIMIZE      (1L << 6)

 /* KDE decoration values */
 enum {
  KDE_noDecoration = 0,
  KDE_normalDecoration = 1,
  KDE_tinyDecoration = 2,
  KDE_noFocus = 256,
  KDE_standaloneMenuBar = 512,
  KDE_desktopIcon = 1024 ,
  KDE_staysOnTop = 2048
 };

	/* this code is specifically hardcoded for the window managers listed and */
	/* needs to be updated for any future window managers */

 void kGUISystemX::HideDecoration(void)
 {
    Atom WM_HINTS;

    WM_HINTS = XInternAtom(m_display, "_MOTIF_WM_HINTS", True);
    if ( WM_HINTS != None )
    {
#define MWM_HINTS_DECORATIONS   (1L << 1)
        struct {
          unsigned long flags;
          unsigned long functions;
          unsigned long decorations;
                   long input_mode;
          unsigned long status;
        } MWMHints = { MWM_HINTS_DECORATIONS, 0,
            MWM_DECOR_NONE, 0, 0 };
        XChangeProperty(m_display, m_win, WM_HINTS, WM_HINTS, 32,
                        PropModeReplace, (unsigned char *)&MWMHints,
                        sizeof(MWMHints)/4);
    }
    WM_HINTS = XInternAtom(m_display, "KWM_WIN_DECORATION", True);
    if ( WM_HINTS != None )
    {
        long KWMHints = KDE_tinyDecoration;
        XChangeProperty(m_display, m_win, WM_HINTS, WM_HINTS, 32,
                        PropModeReplace, (unsigned char *)&KWMHints,
                        sizeof(KWMHints)/4);
    }

    WM_HINTS = XInternAtom(m_display, "_WIN_HINTS", True);
    if ( WM_HINTS != None )
    {
        long GNOMEHints = 0;
        XChangeProperty(m_display, m_win, WM_HINTS, WM_HINTS, 32,
                        PropModeReplace, (unsigned char *)&GNOMEHints,
                        sizeof(GNOMEHints)/4);
    }
    WM_HINTS = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", True);
    if ( WM_HINTS != None )
    {
        Atom NET_WMHints[2];
        NET_WMHints[0] = XInternAtom(m_display,
            "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", True);
        NET_WMHints[1] = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_NORMAL", True);
        XChangeProperty(m_display, m_win,
                        WM_HINTS, XA_ATOM, 32, PropModeReplace,
                        (unsigned char *)&NET_WMHints, 2);
    }
    XSetTransientForHint(m_display, m_win, m_rootwin);
 }
#endif

void kGUISystemX::Loop(void)
{
	fd_set in_fds;
	struct timeval tv;
	struct timeval stv;
#if DEBUGPRINT
	struct timeval ustart;
	struct timeval uend;
#endif
	struct timeval ctv;
	bool forceupdate;
	int tick;
	bool shortdelay;

	gettimeofday(&stv,0);
	Draw();

	/* get events from Xwindows and also timer events */
	shortdelay=false;
	while(kGUI::IsAppClosed()==false)
	{
again:;
//		printf("loop %d\n",random());

		/* has user pressed ctrl-c on the root window?, if so quit */
		/* note: this will not trap ctrl-c when the x-window is active, that will be */
		/* trapped as a GUIKEY_COPY command */
		if(g_userabort==true)
			goto doclose;

		/* if the last event was a timer event then do a long delay */
		/* if it was a user input event then do a short delay */
		tv.tv_sec=0;
		if(shortdelay==true)
			tv.tv_usec=2;/* in millionths of a second */
		else
			tv.tv_usec=1000000/60;/* 60 re-draws per second */

		forceupdate=false;
		FD_ZERO(&in_fds);
		FD_SET(m_x11_fd,&in_fds);

		if(select(m_x11_fd+1,&in_fds,0,0,&tv))
		{
			shortdelay=true;
			/* event */
			XNextEvent(m_display, &m_e);
#if DEBUGPRINT
			printf("event=%d\n",m_e.type);
#endif
			switch(m_e.type)
			{
			case GraphicsExpose:
			case Expose:
				/* copy area from my image to the xwindow display */
				XPutImage(m_display, m_win, m_gc, m_image, m_e.xexpose.x, m_e.xexpose.y, m_e.xexpose.x, m_e.xexpose.y, m_e.xexpose.width, m_e.xexpose.height);
				XFlush(m_display);
			break;
			case ClientMessage:
				if(((unsigned int)m_e.xclient.message_type==(unsigned int)m_wmp) && ((unsigned int)m_e.xclient.data.l[0]==(unsigned int)m_wdw))
				{
					printf("Window Close button was pressed!");
					goto doclose;
				}
			break;
			case SelectionClear:
				/* The clipboard is now used by some other app ( not mine ) */
				printf("SelectionClear event called\n");
			break;
			case SelectionNotify:
			{
				Atom ret_type;
				int ret_format;
				unsigned char *ret;
				unsigned long ret_len;
				unsigned long ret_after;
				long offset;

				/* recieve requested clipboard from another application */

				printf("SelectionNotify event called\n");

				/* clipboard comes across in packets so we need to append them all */
				/* together */

				m_clipboard.Clear();
				offset=0;
				do
				{
					ret_after=0;
					XGetWindowProperty(m_display,m_e.xselection.requestor,
						m_e.xselection.property, offset, 16384, false,
						m_e.xselection.target,
						&ret_type,&ret_format,&ret_len,&ret_after,&ret);
					if(ret)
					{
						m_clipboard.Append((char *)ret,(int)ret_len);
						XFree(ret);
						ret=0;
						offset+=ret_len;
					}
				}while(ret_after);
				printf("Received clipboard ='%s'\n",m_clipboard.GetString());
				/* might have been cancelled so let's check first! */
				if(m_delaypaste==true)
				{
					kGUI::KeyPressed(GUIKEY_PASTE);
					m_delaypaste=false;
				}
			}
			break;
			case SelectionRequest:
			{
				XSelectionEvent sev;
				XSelectionRequestEvent *sreq=&m_e.xselectionrequest;

				/* another app wants my clipboard data */
				printf("SelectionRequest event called\n");
				
				sev.type=SelectionNotify;
				sev.display=sreq->display;
				sev.requestor=sreq->requestor;
				sev.selection=sreq->selection;
				sev.target=sreq->target;
				sev.time=sreq->time;
				if(sreq->target==XA_STRING)
				{
					XChangeProperty(m_display,sreq->requestor,sreq->property,sreq->target,8,PropModeReplace,(unsigned char *)m_clipboard.GetString(),m_clipboard.GetLen());
					sev.property=sreq->property;
				}
				else
					sev.property=None;
				XSendEvent(m_display,sreq->requestor,false,0,(XEvent *)&sev);
			}
			break;
			case MotionNotify:
				m_mousex=m_e.xmotion.x;
				m_mousey=m_e.xmotion.y;

#if USE_WM_DECORATION==0
				/* if the window was recently moved then we need to see if these */
				/* coords are relative to the previous position or the new one */ 
checkmouseagain:		if(m_numoldwindowpositions)
				{
					movewindow_def *owp;
	
					owp=m_oldwindowpositions.GetEntryPtr(0);
//					printf("moveserial=%d, mouse serial=%d\n",owp->serial,m_e.xany.serial);
					if(m_e.xany.serial>=owp->serial)
					{
//						printf("Popping move entry!\n");
						m_oldwindowpositions.DeleteEntry(0);
						--m_numoldwindowpositions;
						goto checkmouseagain;
					}
//					printf("adjusted %d, %d\n",(owp->x)-m_winx,(owp->y)-m_winy);
					m_mousex+=((owp->x)-m_winx);
					m_mousey+=((owp->y)-m_winy);

				}
#endif
				//printf("mx=%d,my=%d\n",m_mousex,m_mousey);
				goto again;
			break;
			case ButtonPress:
				switch(m_e.xbutton.button)
				{
				case Button1:
					m_mouseleft=true;
				break;
				case Button3:
					m_mouseright=true;
				break;
				case Button4:/* middle mouse wheel moved */
					m_mousewheel=1;
				break;
				case Button5:/* middle mouse wheel moved */
					m_mousewheel=-1;
				break;
				}
				forceupdate=true;
				/* if any delay pastes are pending then cancel them */
				m_delaypaste=false;
			break;
			case ButtonRelease:
				switch(m_e.xbutton.button)
				{
				case Button1:
					m_mouseleft=false;
				break;
				case Button3:
					m_mouseright=false;
				break;
				}
				forceupdate=true;
			break;
			case KeyPress:
			{
				XKeyEvent *ke;
				int ks;
				int key;

				ke=&m_e.xkey;
				kGUI::SetKeyShift((ke->state&ShiftMask)!=0);
				kGUI::SetKeyControl((ke->state&ControlMask)!=0);
				ks=XLookupKeysym(ke,(ke->state&ShiftMask)?1:0);

				//todo handle f1-f12
				key=0;	
				switch(ks)
				{
				case XK_Shift_L:			
				case XK_Shift_R:			
				case XK_Control_L:			
				case XK_Control_R:			
				case XK_Caps_Lock:			
				case XK_Alt_L:			
				case XK_Alt_R:
				break;
				case XK_Return:
				case XK_KP_Enter:
					key=GUIKEY_RETURN;
				break;
				case XK_KP_Space:
					key=' ';
				break;
				case XK_Tab:
					if(kGUI::GetKeyShift())
						key=GUIKEY_SHIFTTAB;
					else
						key=GUIKEY_TAB;
				break;
				case XK_Left:
					key=GUIKEY_LEFT;
				break;
				case XK_Right:
					key=GUIKEY_RIGHT;
				break;
				case XK_Up:
					key=GUIKEY_UP;
				break;
				case XK_Down:
					key=GUIKEY_DOWN;
				break;
				case XK_Prior:
					key=GUIKEY_PGUP;
				break;
				case XK_Next:
					key=GUIKEY_PGDOWN;
				break;
				case XK_End:
					key=GUIKEY_END;
				break;
				case XK_Home:
					key=GUIKEY_HOME;
				break;
				case XK_Insert:
					key=GUIKEY_INSERT;
				break;
				case XK_Delete:
					key=GUIKEY_DELETE;
				break;
				case XK_BackSpace:
					key=GUIKEY_BACKSPACE;
				break;
				case XK_Escape:
					key=GUIKEY_ESC;
				break;
				case XK_KP_0:
					key='0';
				break;
				case XK_KP_1:
					key='1';
				break;
				case XK_KP_2:
					key='2';
				break;
				case XK_KP_3:
					key='3';
				break;
				case XK_KP_4:
					key='4';
				break;
				case XK_KP_5:
					key='5';
				break;
				case XK_KP_6:
					key='6';
				break;
				case XK_KP_7:
					key='7';
				break;
				case XK_KP_8:
					key='8';
				break;
				case XK_KP_9:
					key='9';
				break;
				case XK_F1:
					key=GUIKEY_F1;
				break;
				case XK_F2:
					key=GUIKEY_F2;
				break;
				default:
					//printf("KeySym=%04x\n",ks);
					if(kGUI::GetKeyControl())
					{
						switch(ks)
						{
						case 'a':
							key=GUIKEY_SELECTALL;
						break;
						case 'c':
							key=GUIKEY_COPY;
						break;
						case 'v':
						{
							/* paste is a special case */
							Window cwindow;

							/* get window handle for who owns the clipboard */
							cwindow=XGetSelectionOwner(m_display,XA_PRIMARY);
							if(cwindow==m_win)
								key=GUIKEY_PASTE;
							else
							{
								m_delaypaste=true;
								/* paste data arrives via the regular message que */
								XConvertSelection(m_display,XA_PRIMARY,XA_STRING,m_wclip,m_win,CurrentTime);

							}
						}
						break;
						case 'x':
							key=GUIKEY_CUT;
						break;
						case 'z':
							key=GUIKEY_UNDO;
						break;
						case '+':
							key=GUIKEY_CTRL_PLUS;
						break;
						case '-':
							key=GUIKEY_CTRL_MINUS;
						break;
						}
					}
					else
						key=ks;
				break;
				}
				if(key)
				{
					kGUI::KeyPressed(key);
					/* if any delay pastes are pending then cancel them */
					m_delaypaste=false;
				}
			}
			break;
			}
			//kGUI::SetMouse(m_mousex,m_mousey,m_mousewheel,m_mouseleft,m_mouseright);
		}
		else
			shortdelay=false;

		/* calc current delta tick */
		gettimeofday(&ctv,0);

		/* calc delta tick in ms */
		tick=ctv.tv_usec-stv.tv_usec+((ctv.tv_sec-stv.tv_sec)*1000000);

		/* convert to ticks per second */
		tick=tick/(1000000/TICKSPERSEC);
		
		if(tick || forceupdate)
		{
			/* update stv */
			stv.tv_usec+=tick*(1000000/TICKSPERSEC);
			while(stv.tv_usec>=1000000)
			{
				stv.tv_usec-=1000000;
				++stv.tv_sec;
			}
#if DEBUGPRINT
			printf("Update tick=%d\n",tick);
#endif
			kGUI::SetMouse(m_mousex,m_mousey,m_mousewheel,m_mouseleft,m_mouseright);
			kGUI::Tick(tick);

			if(kGUI::GetTempMouse()>=0)
			{
				int t=kGUI::GetTempMouse();
				kGUI::SetTempMouse(t-1);
				if(!t)
					kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
			}
			kGUI::ChangeMouse();

#if DEBUGPRINT
			gettimeofday(&ustart,0);
#endif
			kGUI::UpdateInput();
#if DEBUGPRINT
			gettimeofday(&uend,0);
			tick=uend.tv_usec-ustart.tv_usec+((uend.tv_sec-ustart.tv_sec)*1000000);
			printf("Actual Update tick=%dms\n",tick);
#endif
			m_mousewheel=0;
			
			if(shortdelay==false)
				Draw();
		}
	}
doclose:;
printf("calling appclose\n");
	AppClose();
printf("done calling appclose\n");
printf("kGUI::close\n");
	kGUI::Close();
printf("done calling kguiclose\n");
}

void kGUISystemX::Draw(void)
{
int dw,dh;
kGUICorners c;

	c.lx=0;
	c.rx=0;
	c.ty=0;
	c.by=0;

	while(kGUI::GetInDraw()==true || kGUI::GetInThread()==true)
	{
#if DEBUGPRINT
		printf("indraw=%d, inthread=%d\n",kGUI::GetInDraw(),kGUI::GetInThread());
#endif
		kGUI::Sleep(1);
	}
	kGUI::SetInDraw(true);
	m_image->data=(char *)kGUI::Draw(&c);
	kGUI::SetInDraw(false);

	dw=c.rx-c.lx;
	dh=c.by-c.ty;
	if(dw>0 && dh>0)
	{
#if DEBUGPRINT
		printf("draw x(%d,%d) - y(%d,%d) w=%d,h=%d\n",c.lx,c.rx,c.ty,c.by,dw,dh);
#endif
		XPutImage(m_display,m_win,m_gc,m_image,c.lx,c.ty,c.lx,c.ty,dw,dh);
		XFlush(m_display);
	}
}

void kGUISystemX::FileShow(const char *filename)
{
	pid_t pid;
	char *args[2];

	char *prog=0;
	char firefox[]={"/usr/bin/firefox"};

	if(strstri(filename,".html"))
		prog=firefox;

	if(prog)
	{
		args[0]=(char *)filename;
		args[1]=0;
		pid=fork();
		if(!pid)
			execvp(prog,args);
		
	}
}

void kGUISystemX::ShellExec(const char *program,const char *parms,const char *dir)
{
}

/* true=sucess, false=error */
bool kGUISystemX::MakeDirectory(const char *name)
{
	if(mkdir(name,S_IRWXU)!=-1)
		return(true);
	return(false);
}

/* paste string from the system clipboard */

void kGUISystemX::Paste(kGUIString *s)
{
	s->SetString(&m_clipboard);
}

/* copy string from system clipboard */

void kGUISystemX::Copy(kGUIString *s)
{
	printf("Copy!\n");
	m_clipboard.SetString(s);
	XSetSelectionOwner(m_display,XA_PRIMARY,m_win,CurrentTime);
	if(XGetSelectionOwner(m_display,XA_PRIMARY)!=m_win)
		printf("Error setting Selection!\n");

	XStoreBuffer(m_display,s->GetString(),s->GetLen(),0);
}

/* get filetime */

long kGUISystemX::FileTime(const char *fn)
{
	int result;
	struct stat buf;

	result=stat(fn,&buf);
	if(result==-1)
		return(0);
	return(buf.st_mtime);
}

/* force redraw of whole screen, mainly used for 'busy' bar when program */
/* has not returned control to the system */

void kGUISystemX::ReDraw(void)
{
#if DEBUGPRINT
	printf("Force redraw to screen\n");
#endif
	Draw();
}

/* change mouse pointer shape to current shape type */

void kGUISystemX::ChangeMouse(void)
{
	Cursor c;

	static Cursor mousecursors[]={
		XC_left_ptr,		/* default */
		XC_watch,		/* busy */
		XC_sb_h_double_arrow,	/* adjust left/right */
		XC_sb_v_double_arrow,	/* adjust up/down */
		XC_fleur,		/* adjust size */
		XC_hand1};		/* hand for links on html page */
		//IDC_CROSS   Cross-hair cursor for selection
		//IDC_UPARROW   Arrow that points straight up
		//IDC_SIZEALL   A four-pointed arrow. The cursor to use to resize a window.
		//IDC_SIZENWSE   Two-headed arrow with ends at upper left and lower right
		//IDC_SIZENESW   Two-headed arrow with ends at upper right and lower left
		//IDC_SIZEWE   Horizontal two-headed arrow
		//IDC_SIZENS   Vertical two-headed arrow


	if(m_lastcursor!=kGUI::GetMouseCursor())
	{
//		printf("Setting cursor to %d\n",m_lastcursor);
		c=XCreateFontCursor(m_display,mousecursors[kGUI::GetMouseCursor()]);
		if(c)
		{
			m_lastcursor=kGUI::GetMouseCursor();
			XDefineCursor(m_display,m_win,c);
			XFlush(m_display);
		}
		else
		{
#if DEBUGPRINT
			printf("Error setting Cursor %d\n",kGUI::GetMouseCursor());
#endif
		}
	}
}

void kGUISystemX::ShowMouse(bool show)
{
}

void kGUISystemX::GetWindowPos(int *x,int *y,int *w,int *h)
{
#if 1
	XWindowAttributes wa;

	XGetWindowAttributes(m_display,m_win,&wa);
	m_winx=wa.x;
	m_winy=wa.y;
	m_winw=wa.width;
	m_winh=wa.height;
//	printf("Get %d,%d\n",m_winx,m_winy);
#endif
	*(x)=m_winx;
	*(y)=m_winy;
	*(w)=m_winw;
	*(h)=m_winh;
}

void kGUISystemX::SetWindowPos(int x,int y)
{
//	printf("Set window position to %d,%d\n",x,y);
#if USE_WM_DECORATION
	XMoveWindow(m_display,m_win,x,y);
#else
	XSetWindowAttributes attributes;
	XWindowAttributes wa;
	movewindow_def *owp;

	/* save old positions in queue */
	XGetWindowAttributes(m_display,m_win,&wa);
	owp=m_oldwindowpositions.GetEntryPtr(m_numoldwindowpositions++);
//	printf("saving old window position %d %d into slot #%d\n",wa.x,wa.y,m_numoldwindowpositions-1);
	owp->x=wa.x;
	owp->y=wa.y;

	if(y<TOP)
		y=TOP;
	else if(y>m_fullheight)
		y=m_fullheight;

	if(x<0)
		x=0;
	else if(x>(m_fullwidth-64))
		x=m_fullwidth-64;


	/*the window manager if connected will not let this window move off of the screen */
	/* so we disconnect the manager and then reattach it after we have moved it */
	attributes.override_redirect = True;
	XChangeWindowAttributes(m_display,m_win,CWOverrideRedirect,&attributes);

	owp->serial=NextRequest(m_display);
	XMoveWindow(m_display,m_win,x,y);
//	printf("move request=%d\n",owp->serial);

	/* put back */
	attributes.override_redirect = False;
	XChangeWindowAttributes(m_display,m_win,CWOverrideRedirect,&attributes);

	/* get coords after they have been set */
	XGetWindowAttributes(m_display,m_win,&wa);
//	printf("After set coords are %d,%d\n",wa.x,wa.y);
	m_winx=wa.x;
	m_winy=wa.y;
#endif
}

void kGUISystemX::SetWindowSize(int w,int h)
{
//	printf("Set window size to %d,%d\n",w,h);
	XResizeWindow(m_display,m_win,w,h);
//	XFlush(m_display);
	m_winw=w;
	m_winh=h;
}

void kGUISystemX::Minimize(void)
{
	XIconifyWindow(m_display,m_win,0);
}

/* sleep in millseconds */
void kGUISystemX::Sleep(int ms)
{
	struct timeval delay;

//	printf("Sleeping...\n");

	delay.tv_sec=ms/1000000L;	
	delay.tv_usec=ms%1000000L;	
	select(0,0,0,0,&delay);
}

/* is this a directory? */
bool kGUISystemX::IsDir(const char *fn)
{
#if 1
    struct stat sb;

    if (stat(fn, &sb) < 0)
		return (false);
    return (S_ISDIR(sb.st_mode));
#else
	int dir_handle;

	dir_handle=open(fn,O_RDONLY);				/* open directory for reading */
	if(dir_handle==-1)
		return(false);	/* cannot open dir */
	close(dir_handle);
	return(true);		/* yes! */
#endif
}

kGUISystemX::~kGUISystemX()
{
printf("closing display\n");
	if(m_display)
		XCloseDisplay(m_display);
/* do I need to free these or not? */
//	if(m_printers)
}

void kGUISystemX::GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv)
{
	ppd_file_t *ppd;
	ppd_size_t *size;
	const char *filename;

	/* todo, make printing handle different paper/page sizes from availabe list */

	filename = cupsGetPPD(name);
	if(filename)
	{
		ppd = ppdOpenFile(filename);
		if(ppd)
		{
			size   = ppdPageSize(ppd, "Letter");
			if(size)
			{
//				printf("w=%f,h=%f,l=%f,r=%f,t=%f,b=%f\n",size->width,size->length,size->left,size->right,size->top,size->bottom);
				*(pw)=(int)(size->right-size->left);
				*(ppih)=72;
				/* bottom is larger than top as 0,0 is bottom left */
				*(ph)=(int)(size->top-size->bottom);
				*(ppiv)=72;
			}
			else
				printf("Could not get pagesize info for printer '%s'\n",name);
			ppdClose(ppd);
			
		}	
		else
			printf("Could not get ppd info for printer '%s'\n",name);
	}
	else
		printf("Could not get ppd filename for printer '%s'\n",name);
}

/* printer class */

/* return false if error starting print job */
bool kGUIPrintJobCups::Start(kGUIPrinter *p,const char *jobname,int numpages,int numcopies,double ppi,double lm,double rm,double tm,double bm,kGUIDrawSurface *surface,bool landscape)
{
	int ppw;
	int pph;
	double paiw,paih;
	double hardleft,hardright,hardtop,hardbottom;

	m_pagenum=1;
	m_numpages=numpages;
	m_numcopies=numcopies;
	m_jobname.SetString(jobname);
	/* calc printable area of page in inches */
	paiw=surface->GetWidth()/ppi;
	paih=surface->GetHeight()/ppi;

	m_landscape=landscape;
	m_p=p;
	m_error=false;

	{
		ppd_file_t *ppd;
		ppd_size_t *size;
		const char *ppdfilename;

		ppdfilename = cupsGetPPD(p->GetName());
		if(!ppdfilename)
			return(false);
		ppd = ppdOpenFile(ppdfilename);
		if(!ppd)
			return(false);

		/* todo, make printing handle different paper/page sizes from availabe list */
		size   = ppdPageSize(ppd, "Letter");
		if(!size)
			return(false);

		/* get printer page size in pixels */
		ppw = (int)size->width;
		pph = (int)size->length;

		/* todo, get actual ppi */
		/* get printer pixels per inch */	
		m_ppiw = 72;
		m_ppih = 72;

		/* get physical unprintable margins on printer in inches */
		hardleft = (double)size->left/(double)m_ppiw;
		hardright = (double)(size->width-size->right)/(double)m_ppiw;
		hardtop  = (double)(size->length-size->top)/(double)m_ppih;
		hardbottom  = (double)size->bottom/(double)m_ppih;
		ppdClose(ppd);
	}

	/* convert margins from inches to pixels */
	if(landscape==false)
	{
		if(lm<hardleft)
			lm=hardleft;
		if(rm<hardright)
			rm=hardright;
		if(tm<hardtop)
			tm=hardtop;
		if(bm<hardbottom)
			bm=hardbottom;

		m_lmp=(int)(lm*m_ppiw);
		m_rmp=(int)(rm*m_ppiw);
		m_tmp=(int)(tm*m_ppih);
		m_bmp=(int)(bm*m_ppih);
//		printf("l=%d,r=%d,t=%d,mb=%d\n",m_lmp,m_rmp,m_tmp,m_bmp);

		/* printer draw area in pixels */
		m_dw=ppw-(m_lmp+m_rmp);
		m_dh=pph-(m_tmp+m_bmp);
//		printf("dw=%d, dh=%d\n",m_dw,m_dh);

		/* surface area of page in pixels */
		m_sw=surface->GetWidth();
		m_sh=surface->GetHeight();

		/* scale factor to convert from surface pixels to printer pixels */
		m_scalew=(double)m_dw/(double)m_sw;
		m_scaleh=(double)m_dh/(double)m_sh;
	}
	else
	{
		if(lm<hardtop)
			lm=hardtop;
		if(rm<hardbottom)
			rm=hardbottom;
		if(tm<hardright)
			tm=hardright;
		if(bm<hardleft)
			bm=hardleft;

		/* rotate */
		m_lmp=(int)(bm*m_ppih);
		m_rmp=(int)(tm*m_ppih);
		m_tmp=(int)(lm*m_ppiw);
		m_bmp=(int)(rm*m_ppiw);

		/* draw area in pixels */
		m_dw=pph-(m_lmp+m_rmp);
		m_dh=ppw-(m_tmp+m_bmp);

		m_sw=surface->GetWidth();
		m_sh=surface->GetHeight();

		/* scale factor to convert from surface pixels to printer pixels */
		m_scalew=(double)m_dw/(double)m_sw;
		m_scaleh=(double)m_dh/(double)m_sh;
	}

//	m_filename.Sprintf("%s.ps",jobname);
//	m_filename.Replace(" ","_");
	m_filename.SetString("cups.ps");

	m_handle=fopen(m_filename.GetString(),"wb");
	if(m_handle)
	{
		fprintf(m_handle,"%%!PS-Adobe-3.0\n");
		fprintf(m_handle,"%%%%BoundingBox: %d %d %d %d\n",0,0,ppw,pph);
		fprintf(m_handle,"%%%%Pages: %d\n",m_numpages);
		fprintf(m_handle,"%%%%EndComments\n");
		return(true);
	}
	else
		return(false);
}

void kGUIPrintJobCups::StartPage(void)
{
	m_lastfont.Clear();
	m_lastfontsize=-1;

	fprintf(m_handle,"%%%%Page: %d %d\n",m_pagenum++,1 /*(m_numpages */);
	fprintf(m_handle,"gsave\n");
}

void kGUIPrintJobCups::DrawRect(int lx,int rx,int ty,int by,kGUIColor color)
{
	int r,g,b;
	double left,right,top,bottom;

	DrawColorToRGB(color,r,g,b);

	if(m_landscape==false)
	{
		left=(lx*m_scalew)+m_lmp;
		right=(rx*m_scalew)+m_lmp;
		top=(m_dh+m_tmp+m_bmp)-((ty*m_scaleh)+m_tmp);
		bottom=(m_dh+m_tmp+m_bmp)-((by*m_scaleh)+m_tmp);
	}
	else
	{
		left=(m_dh+m_tmp+m_bmp)-((ty*m_scaleh)+m_tmp);
		right=(m_dh+m_tmp+m_bmp)-((by*m_scaleh)+m_tmp);
		top=(m_dw+m_rmp)-(rx*m_scalew);
		bottom=(m_dw+m_rmp)-(lx*m_scalew);
	}

	fprintf(m_handle,"newpath\n");
	fprintf(m_handle,"%f %f moveto\n",left,top);
	fprintf(m_handle,"%f %f lineto\n",right,top);
	fprintf(m_handle,"%f %f lineto\n",right,bottom);
	fprintf(m_handle,"%f %f lineto\n",left,bottom);
	fprintf(m_handle,"%f %f lineto\n",left,top);

	fprintf(m_handle,"closepath\n");
	fprintf(m_handle,"%f %f %f setrgbcolor\n",(double)r/256.0f,(double)g/256.0f,(double)b/256.0f);
	fprintf(m_handle,"fill\n");
}

void kGUIPrintJobCups::DrawTextList(int pointsize,const char *fontname,int num, Array<PRINTFONTCHAR_DEF> *list)
{
	int i;
	int r,g,b;
	PRINTFONTCHAR_DEF pfc;
	double left,right,top,bottom;
	bool setfont=false;
	int correctedpointsize;
	int correctedpointsizepix;
	kGUIString correctedfontname;

	correctedfontname.SetString(fontname);
	correctedfontname.Replace(" ","-");
	correctedpointsize=(int)(pointsize*m_scaleh/* *1.10f*/);
	correctedpointsizepix=correctedpointsize;	//(int)(correctedpointsize*0.85f);

	if(strcmp(correctedfontname.GetString(),m_lastfont.GetString()))
	{
		fprintf(m_handle,"/%s findfont\n",correctedfontname.GetString());
		m_lastfont.SetString(correctedfontname.GetString());
		setfont=true;
	}
	if((correctedpointsize!=m_lastfontsize) || setfont)
	{
		fprintf(m_handle,"%d scalefont\n",correctedpointsize);
		m_lastfontsize=correctedpointsize;
		setfont=true;
	} 
	if(setfont==true)
		fprintf(m_handle,"setfont\n");

	for(i=0;i<num;++i)
	{
		pfc=list->GetEntry(i);

		if(m_landscape==false)
		{
			left=(pfc.lx*m_scalew)+m_lmp;
			right=(pfc.rx*m_scalew)+m_lmp;
			top=(m_dh+m_tmp+m_bmp)-((pfc.ty*m_scaleh)+m_tmp);
			bottom=(m_dh+m_tmp+m_bmp)-(((pfc.by+((pfc.by-pfc.ty)*0.10f))*m_scaleh)+m_tmp);
		}
		else
		{
			left=(m_dh+m_tmp+m_bmp)-((pfc.ty*m_scaleh)+m_tmp);
			right=(m_dh+m_tmp+m_bmp)-(((pfc.by+((pfc.by-pfc.ty))*0.10f)*m_scaleh)+m_tmp);
			bottom=(m_dw+m_rmp)-(pfc.rx*m_scalew);
			top=(m_dw+m_rmp)-(pfc.lx*m_scalew);
		}

		DrawColorToRGB(pfc.fgcol,r,g,b);

		if(pfc.c!='(' && pfc.c!=')')
		{
			//todo: only send rgbcolor if changed
			fprintf(m_handle,"%f %f %f setrgbcolor\n",(double)r/256.0f,(double)g/256.0f,(double)b/256.0f);
			if(m_landscape)
			{
				fprintf(m_handle,"%f %f moveto\n",left-correctedpointsizepix,top);
				fprintf(m_handle,"270 rotate\n");
			}
			else
				fprintf(m_handle,"%f %f moveto\n",left,top-correctedpointsizepix);
			fprintf(m_handle,"(%c) show\n",pfc.c);
			if(m_landscape)
				fprintf(m_handle,"-270 rotate\n");
		}
	}
}

/* copy area in current surface to the printer */

void kGUIPrintJobCups::DrawImage(int lx,int rx,int ty,int by)
{
	DrawImage(kGUI::GetCurrentSurface(),lx,rx,ty,by);
}

void kGUIPrintJobCups::DrawImage(kGUIDrawSurface *s,int lx,int rx,int ty,int by)
{
	int x,y,n;
	kGUIColor color;
	int r,g,b;
	double dlx,drx,dty,dby;

	/* calc destination coords on printer */
	if((m_landscape==false))
	{
		dlx=(lx*m_scalew)+m_lmp;
		drx=(rx*m_scalew)+m_lmp;
		dty=(m_dh+m_tmp+m_bmp)-((ty*m_scaleh)+m_tmp);
		dby=(m_dh+m_tmp+m_bmp)-((by*m_scaleh)+m_tmp);

		fprintf(m_handle,"gsave\n");
		fprintf(m_handle,"%f %f translate\n",dlx,dby);
		fprintf(m_handle,"%f %f scale\n",drx-dlx,dty-dby);

		fprintf(m_handle,"%d %d 8 [%d 0 0 %d 0 0]\n",rx-lx,by-ty,rx-lx,by-ty);
		fprintf(m_handle,"{<\n");

		for(y=by-1;y>=ty;--y)
		{
			n=0;
			for(x=lx;x<rx;++x)
			{
				color=*(s->GetSurfacePtrABS(x,y));
				DrawColorToRGB(color,r,g,b);
				fprintf(m_handle,"%02x%02x%02x",r,g,b);
				if(++n==32)
				{
					n=0;
					fprintf(m_handle,"\n");
				}
			}
			if(n)
				fprintf(m_handle,"\n");
		}

		fprintf(m_handle,">}\n");
		fprintf(m_handle,"false 3 colorimage\n");
		fprintf(m_handle,"grestore\n");
	}
	else
	{
		dlx=(m_dh+m_tmp+m_bmp)-((ty*m_scaleh)+m_tmp);
		drx=(m_dh+m_tmp+m_bmp)-((by*m_scaleh)+m_tmp);
		dty=(m_dw+m_rmp)-(rx*m_scalew);
		dby=(m_dw+m_rmp)-(lx*m_scalew);

		fprintf(m_handle,"gsave\n");
		fprintf(m_handle,"%f %f translate\n",dlx,dby);
		fprintf(m_handle,"%f %f scale\n",drx-dlx,dty-dby);

		fprintf(m_handle,"%d %d 8 [%d 0 0 %d 0 0]\n",by-ty,rx-lx,by-ty,rx-lx);
		fprintf(m_handle,"{<\n");

		for(x=lx;x<rx;++x)
		{
			for(y=ty;y<by;++y)
			{
				color=*(s->GetSurfacePtrABS(x,y));
				DrawColorToRGB(color,r,g,b);
				fprintf(m_handle,"%02x%02x%02x",r,g,b);
			}
			fprintf(m_handle,"\n");
		}

		fprintf(m_handle,">}\n");
		fprintf(m_handle,"false 3 colorimage\n");
		fprintf(m_handle,"grestore\n");
	}
}

void kGUIPrintJobCups::PrintSurface(kGUIDrawSurface *surface)
{
	int x,y;
	int lx,rx,ty,by;
	int sw,sh;

	/* split into 64x64 images as too large of an image will crash postscript */
	sw=surface->GetWidth();
	sh=surface->GetHeight();
	for(y=0;y<sh;y+=64)
	{
		ty=y;
		by=y+64;
		if(by>sh)
			by=sh;
		for(x=0;x<sw;x+=64)
		{
			lx=x;
			rx=x+64;
			if(rx>sw)
				rx=sw;
			DrawImage(surface,lx,rx,ty,by);
		}
	}
}

void kGUIPrintJobCups::EndPage(void)
{
	fprintf(m_handle,"grestore\n");
	fprintf(m_handle,"showpage\n");
}

/* false=error, true=ok */
bool kGUIPrintJobCups::End(void)
{
	int jobid;
	int num_options=0;
	cups_option_t *options=0;
	kGUIString numcopies;

	fprintf(m_handle,"%%%%Trailer\n");
	fprintf(m_handle,"%%%%Pages: %d\n",m_numpages);
	fprintf(m_handle,"%%eof\n");
	fclose(m_handle);
#if 1
	numcopies.Sprintf("%d",m_numcopies);
	num_options = cupsAddOption("copies", numcopies.GetString(), num_options, &options);
	jobid = cupsPrintFile(m_p->GetName(),m_filename.GetString(),m_jobname.GetString(), num_options, options);
	cupsFreeOptions(num_options, options);
	kGUI::FileDelete(m_filename.GetString());

        if (jobid == 0)
        {
		printf("%s\n",ippErrorString(cupsLastError()));
		m_error=true;
	}
#endif

	return(!m_error);
}
