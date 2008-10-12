// kguiapp.cpp : Defines the entry point for the application.
//

#include <windows.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <winspool.h>
/* these are needed to copy/paste to the clipboard */
#include <winuser.h>
#include <winsock.h>
#include <direct.h>

#include <errno.h>
#include <io.h>
#include <sys/utime.h>
#include <shlwapi.h>
/* used for get desktop name function */
#include <shlobj.h>

#define DEBUGPRINT 0

#define MAX_LOADSTRING 100
#define FRAMES_PER_SECOND 60

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#endif

/* these are in the users program */
void AppInit(void);
void AppClose(void);

class kGUIPrintJobMINGW : public kGUIPrintJob
{
public:
	kGUIPrintJobMINGW() {m_error=false;}
	~kGUIPrintJobMINGW() {}
	bool Start(kGUIPrinter *p,const char *jobname,int numpages,int numcopies,double ppi,double lm,double rm,double tm,double bm,kGUIDrawSurface *surface,bool landscape);
	void StartPage(void);
	void PrintSurface(kGUIDrawSurface *surface);
	void DrawRect(int lx,int rx,int ty,int by,kGUIColor color);
	void DrawTextList(int pointsize,const char *fontname,int num, Array<PRINTFONTCHAR_DEF> *list);
	void DrawImage(int lx,int rx,int ty,int by);
	void EndPage(void);
	bool End(void);
private:
	HDC m_printerHDC;
	DOCINFO m_pdi;
};

/* windows version of system */
class kGUISystemMINGW : public kGUISystem
{
public:
	void Init(HINSTANCE hInstance,int nCmdShow);
	void Draw(HWND hWnd,int ulx,int urx,int uty,int uby);

	void FileShow(const char *filename);
	void ShellExec(const char *program,const char *parms,const char *dir);
	void Copy(kGUIString *s);
	void Paste(kGUIString *s);
	void Sleep(int ms);
	bool MakeDirectory(const char *name);
	bool IsDir(const char *fn);
	long FileTime(const char *fn);
	void HideWindow(void);
	void ShowWindow(void);
	void ReDraw(void);
	void ChangeMouse(void);
	void ShowMouse(bool show);
	void GetWindowPos(int *x,int *y,int *w,int *h);
	void SetWindowPos(int x,int y);
	void SetWindowSize(int w,int h);
	void Minimize(void);
	void AdjustMouse(int dx,int dy);

	bool NeedRotatedSurfaceForPrinting(void) {return true;}
	unsigned int GetNumPrinters(void) {return m_numprinters;}
	void GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv);

	int GetDefPrinterNum(void) {return m_defprinternum;}
	kGUIPrinter *GetPrinterObj(int pid) {return m_printers.GetEntryPtr(pid);}
	int LocatePrinter(const char *name,bool ordefault);

	class kGUIPrintJob *AllocPrintJob(void) {kGUIPrintJob *pj;pj=new kGUIPrintJobMINGW(); return pj;}

	LRESULT Event(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void CreateHDC(const char *printerName,int numcopies,HDC *hdc);
private:
	void GetPrinters(void);
	int m_mousex;
	int m_mousey;
	int m_mousewheeldelta;
	int m_waskilled;
	bool m_showwindow:1;
	bool m_mouseleftbutton:1;
	bool m_mouserightbutton:1;
	int m_lasttickcount;

	HWND m_hWnd;
	WSADATA m_SockData;
//	HACCEL m_hAccelTable;
	HINSTANCE m_hInst;								// current instance
	TCHAR m_szTitle[MAX_LOADSTRING];				// The title bar text
	TCHAR m_szWindowClass[MAX_LOADSTRING];			// the main window class name
	RECT m_WinRect;
	RECT m_UpdateWinRect;

	/* printer info */
	unsigned int m_numprinters;
	unsigned int m_defprinternum;
	ClassArray<kGUIPrinter>m_printers;
};

kGUISystemMINGW *g_sys;

//since this must be a static function callback, we just pass it along to the class event handler
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return(g_sys->Event(hWnd,message,wParam,lParam));
}

int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	//MessageBox(0,"Hello, Windows","MinGW Test Program",MB_OK);

	g_sys=new kGUISystemMINGW();
	g_sys->Init(hInstance,nCmdShow);
	delete g_sys;

  return 0;
} 

void kGUISystemMINGW::Init(HINSTANCE hInstance,int nCmdShow)
{
	int startwidth,startheight;
	int fullwidth,fullheight,maximages;
	MSG msg;

	m_showwindow=false;
	m_mousex=0;
	m_mousey=0;
	m_mousewheeldelta=0;
	m_waskilled=false;
	m_mouseleftbutton=false;
	m_mouserightbutton=false;
	GetPrinters();

	// Initialize global strings
	kGUI::Trace("------------------------------------------------------------------------------\n");
	LoadString(hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
	kGUI::Trace(" LoadString(hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);\n");
	LoadString(hInstance, IDC_APP_NAME, m_szWindowClass, MAX_LOADSTRING);
	kGUI::Trace(" LoadString(hInstance, IDC_APP_NAME, m_szWindowClass, MAX_LOADSTRING);\n");

	{
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX); 

		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= (WNDPROC)WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
#if defined(IDI_LARGEICON)
		wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_LARGEICON);
#else
		wcex.hIcon=0;
#endif
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName	= 0;
		wcex.lpszClassName	= m_szWindowClass;
#if defined(IDI_SMALLICON)
		wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALLICON);
#else
		wcex.hIconSm=0;
#endif
		kGUI::Trace(" RegisterClass(&wcex);\n");
		RegisterClassEx(&wcex);
	}

	/* get the full screen size */
	fullwidth=GetSystemMetrics(SM_CXSCREEN);
	fullheight=GetSystemMetrics(SM_CYSCREEN);

	/* this is the size of that app window, use fullscreen */
	/* unless overridden by the user requested dimensions */
#ifdef	DEFSCREENWIDTH
	startwidth=DEFSCREENWIDTH;
#else
	startwidth=fullwidth;
#endif
#ifdef	DEFSCREENHEIGHT
	startheight=DEFSCREENHEIGHT;
#else
	startheight=fullheight-35;
#endif
#ifdef DEFMAXIMAGES
	maximages=DEFMAXIMAGES;
#else
	maximages=200;
#endif

	m_lasttickcount=GetTickCount();
	
	m_hInst = hInstance; // Store instance handle in our global variable

	kGUI::Trace(" hWnd = CreateWindow(m_szWindowClass, m_szTitle, WS_POPUP | WS_VISIBLE, 0, 0, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL);\n");
 
	m_hWnd = CreateWindow(m_szWindowClass, m_szTitle, WS_POPUP | WS_VISIBLE /*|WS_EX_LAYERED*/ ,
	    (fullwidth-startwidth)/2, (fullheight-startheight)/2, startwidth, startheight, NULL, NULL, hInstance, NULL);

	if (!m_hWnd)
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return;
	}
	// Allocate BMP data for screen
	kGUI::Trace(" GetClientRect(hWnd,&m_WinRect);\n");
	GetClientRect(m_hWnd,&m_WinRect); 

	/* initialize kGUI engine */
	kGUI::Trace(" kGUI::Init(m_WinRect.right-m_WinRect.left,m_WinRect.bottom-m_WinRect.top);\n");
	if(kGUI::Init(g_sys,m_WinRect.right-m_WinRect.left,m_WinRect.bottom-m_WinRect.top,fullwidth,fullheight,maximages)==false)
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return;
	}

//	kGUI::Trace(" LoadAccelerators(hInstance, (LPCTSTR)IDC_APP_NAME);\n");
//	m_hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_APP_NAME);

	kGUI::Trace(" WSAStartup(0x0202,&m_SockData);\n");
    if(WSAStartup(0x0202,&m_SockData))
	{
		kGUI::Trace(" error initing WSAStartup;\n");
		return;
	}

	/* call the application init code */
	kGUI::Trace(" AppInit();\n");
	AppInit();

	/* show the window if the application has not already done so */
	if(m_showwindow==false)
		ShowWindow();

	kGUI::Trace(" UpdateWindow(m_hWnd);\n");
    UpdateWindow(m_hWnd);

	kGUI::Trace(" Draw(m_hWnd);\n");
    Draw(m_hWnd,0,0,0,0);

	kGUI::Trace(" SetTimer(m_hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);\n");
    SetTimer(m_hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);

	// Main message event loop:
	kGUI::Trace(" Entering Main loop: GetMessage(&msg, NULL, 0, 0);\n");
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
//		if (!TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	kGUI::Trace(" Exited Main loop, program done.\n");
}

//
//  FUNCTION: Event(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT kGUISystemMINGW::Event(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int key;
	int newtick,deltatick;

//	printf("in Event...!\n");
//	fflush(stdout);
	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
//		printf("Done Event...!\n");
//		fflush(stdout);
		return DefWindowProc(hWnd, message, wParam, lParam);
	break;
	case WM_PAINT:
		// Get the Windows update region

//		printf("Wait...!\n");
//		fflush(stdout);
//		while(kGUI::GetInDraw()==true || kGUI::GetInThread()==true);
//		printf("Done Wait...!\n");
//		fflush(stdout);

//		kGUI::SetInDraw(true);
		GetUpdateRect(hWnd, &m_UpdateWinRect, FALSE);
		hdc = BeginPaint(hWnd, &ps);
		Draw(hWnd,m_UpdateWinRect.left,m_UpdateWinRect.right,m_UpdateWinRect.top,m_UpdateWinRect.bottom);
		EndPaint(hWnd, &ps);
//		kGUI::SetInDraw(false);
	break;
	case WM_TIMER:
		newtick=GetTickCount();
		deltatick=newtick-m_lasttickcount;
		if(deltatick<0)
			deltatick=0;
		m_lasttickcount=newtick;

#if DEBUGPRINT
		kGUI::Trace("Delta tick=%d\n",deltatick);
#endif
		kGUI::Tick(deltatick);

		if(GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_RSHIFT))
			kGUI::SetKeyShift(true);
		else
			kGUI::SetKeyShift(false);
		if(GetAsyncKeyState(VK_CONTROL))
			kGUI::SetKeyControl(true);
		else
			kGUI::SetKeyControl(false);

		if(kGUI::GetTempMouse()>=0)
		{
			int t=kGUI::GetTempMouse();
			kGUI::SetTempMouse(t-1);
			if(!t)
				kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
		}
		if(kGUI::GetMouseCursor()!=MOUSECURSOR_DEFAULT)	/* if not default arrow */
			kGUI::ChangeMouse();

//		printf("Calling Input!\n");
//		fflush(stdout);
		kGUI::UpdateInput();
//		printf("Done Calling Input!\n");
//		fflush(stdout);

		m_mousewheeldelta=0;

//		printf("Wait...!\n");
//		fflush(stdout);
//		while(kGUI::GetInDraw()==true || kGUI::GetInThread()==true);
//		printf("Done Wait...!\n");
//		fflush(stdout);

//		kGUI::SetInDraw(true);
		Draw(hWnd,0,0,0,0);
//		kGUI::SetInDraw(false);

		/* is the application shutting down? */
		if(kGUI::IsAppClosed()==true)
			DestroyWindow(hWnd);
		else
			SetTimer(hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);
		break;
	case WM_DESTROY:
		/* sometimes this event get's called more than once, so allow only once! */
		if(m_waskilled==false)
		{
			m_waskilled=true;
			AppClose();
			kGUI::Close();
			PostQuitMessage(0);
		    WSACleanup();
		}
	break;
    case WM_MOUSEWHEEL:
		//values are incrments of 120
		m_mousewheeldelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;
//		m_mousewheeldelta = ( int ) ( ( short ) HIWORD( wParam ) )/120;
		kGUI::SetMouse(m_mousex,m_mousey,m_mousewheeldelta,m_mouseleftbutton,m_mouserightbutton);
	break;
	case WM_LBUTTONDOWN: 
    case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: 
    case WM_RBUTTONUP: 
    case WM_MOUSEMOVE:
		if(kGUI::GetMouseCursor()!=MOUSECURSOR_DEFAULT)	/* if not default arrow */
			kGUI::ChangeMouse();
		m_mousex=GET_X_LPARAM(lParam);
		m_mousey=GET_Y_LPARAM(lParam);
		m_mouseleftbutton=(wParam&MK_LBUTTON)!=0;
		m_mouserightbutton=(wParam&MK_RBUTTON)!=0;
		
		/* this tells windows to keep updating me even if the mouse is moved off of */
		/* my window area */
		if(m_mouseleftbutton || m_mouserightbutton)
			SetCapture(hWnd); 
		else
			ReleaseCapture(); 

		kGUI::SetMouse(m_mousex,m_mousey,m_mousewheeldelta,m_mouseleftbutton,m_mouserightbutton);
		kGUI::Tick(0);
		kGUI::UpdateInput();

//		printf("Done Event...!\n");
//		fflush(stdout);
		return DefWindowProc(hWnd, message, wParam, lParam);
	break;
	case WM_CHAR:
		key=(int)wParam;
		switch(key)
		{
		case 1:	//'ctrl-a'
			key=GUIKEY_SELECTALL;
		break;
		case 3:	//'ctrl-c'
			key=GUIKEY_COPY;
		break;
		case 22:	//'ctrl-v'
			key=GUIKEY_PASTE;
		break;
		case 24:	//'ctrl-x'
			key=GUIKEY_CUT;
		break;
		case 26:	//'ctrl-z'
			key=GUIKEY_UNDO;
		break;
		case 187:	//'ctrl +'
			key=GUIKEY_CTRL_PLUS;
		break;
		case 189:	//'ctrl -'
			key=GUIKEY_CTRL_MINUS;
		break;
		case VK_BACK:
			key=GUIKEY_BACKSPACE;
		break;
		case VK_RETURN:
			key=GUIKEY_RETURN;
		break;
		case VK_TAB:
			if(GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_RSHIFT))
				key=GUIKEY_SHIFTTAB;
			else
				key=GUIKEY_TAB;
		break;
		case VK_ESCAPE:
			key=GUIKEY_ESC;
		break;
		case VK_SHIFT:
		case VK_CONTROL:
			key=0;
		break;
		}
		if(key)
			kGUI::KeyPressed(key);
	break;
	case WM_KEYDOWN:
		key=0;
		switch (wParam)
		{
		case VK_HOME:
			key=GUIKEY_HOME;
		break;
		case VK_END:
			key=GUIKEY_END;
		break;
		case VK_LEFT:
			key=GUIKEY_LEFT;
		break;
		case VK_RIGHT:
			key=GUIKEY_RIGHT;
		break;
		case VK_UP:
			key=GUIKEY_UP;
		break;
		case VK_DOWN:
			key=GUIKEY_DOWN;
		break;
		case VK_PRIOR:
			key=GUIKEY_PGUP;
		break;
		case VK_NEXT:
			key=GUIKEY_PGDOWN;
		break;
		case VK_DELETE:
			key=GUIKEY_DELETE;
		break;
		case VK_INSERT:
			key=GUIKEY_INSERT;
		break;
		case VK_F1:
			key=GUIKEY_F1;
		break;
		case VK_F2:
			key=GUIKEY_F2;
		break;
		case VK_F3:
			key=GUIKEY_F3;
		break;
		case VK_F4:
			key=GUIKEY_F4;
		break;
		case VK_F5:
			key=GUIKEY_F5;
		break;
		case VK_F6:
			key=GUIKEY_F6;
		break;
		case VK_F7:
			key=GUIKEY_F7;
		break;
		case VK_F8:
			key=GUIKEY_F8;
		break;
		case VK_F9:
			key=GUIKEY_F9;
		break;
		case VK_F10:
			key=GUIKEY_F10;
		break;
		case VK_F11:
			key=GUIKEY_F11;
		break;
		case VK_F12:
			key=GUIKEY_F12;
		break;
		}
		if(key)
			kGUI::KeyPressed(key);
	break;
	default:
//		printf("Done Event...!\n");
//		fflush(stdout);
		return DefWindowProc(hWnd, message, wParam, lParam);
	break;
	}
//	printf("Done Event...!\n");
//	fflush(stdout);
	return 0;
}

void kGUISystemMINGW::Draw(HWND hWnd,int ulx,int urx,int uty,int uby)
{
	int sw,sh;
	int dw,dh;
	int nl;
	kGUIColor *bmp_data;
	kGUICorners c;
	BITMAPINFO bmp;

	sw=kGUI::GetSurfaceWidth();
	sh=kGUI::GetSurfaceHeight();
	
	/* returns pointer to bitmap and c contains 4 corners of updated area */

	c.lx=ulx;
	c.rx=urx;
	c.ty=uty;
	c.by=uby;
//	printf("Calling Draw!\n");
//	fflush(stdout);
	bmp_data=kGUI::Draw(&c);
	if(!bmp_data)
		return;
	//	printf("Done Calling Draw!\n");
//	fflush(stdout);

	dw=c.rx-c.lx;	/* width of updated area */
	dh=c.by-c.ty;	/* height of updated area */
	if(!dw || !dh)
		return;		/* nothing on screen was updated */

	/* hmm, had this assert trigger when the screen size was made bigger while the program was running */
	/* perhaps change this to a clip on rx and by if they are too large? */

	if(c.lx<0 || c.ty<0 || c.rx>sw || c.by>sh)
		assert(false,"Error!");

	bmp.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmp.bmiHeader.biWidth = sw;
	bmp.bmiHeader.biHeight = -sh;
	bmp.bmiHeader.biPlanes = 1;
	bmp.bmiHeader.biBitCount = WINBPP;
	bmp.bmiHeader.biCompression = BI_RGB;
	bmp.bmiHeader.biSizeImage = 0;
	bmp.bmiHeader.biXPelsPerMeter = 0;
	bmp.bmiHeader.biYPelsPerMeter = 0;
	bmp.bmiHeader.biClrUsed = 0;
	bmp.bmiHeader.biClrImportant = 0;

//	if(kGUI::GetTrace())
//		kGUI::Trace("PCDraw(%d,%d,%d,%d\n",c.lx,c.lx+dw,c.ty,c.ty+dh);

	HDC hDC = GetDC(hWnd);
	nl=SetDIBitsToDevice(hDC,
		c.lx, c.ty,		// upper left of destination
	 	dw, dh,			// source rectangle size
		c.lx, 0,		// lower left of source
		0, dh,			// first scan line, number of scan lines in array
		bmp_data+(c.ty*sw), &bmp, DIB_RGB_COLORS);
	ReleaseDC(hWnd, hDC);
}

/**************************************************************************/

void kGUISystemMINGW::FileShow(const char *filename)
{
	ShellExecute(NULL,"open",filename,NULL,NULL,SW_SHOWNORMAL);
}

void kGUISystemMINGW::ShellExec(const char *program,const char *parms,const char *dir)
{
	SHELLEXECUTEINFO ShExecInfo = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;
	ShExecInfo.lpFile = program;		
	ShExecInfo.lpParameters = parms;	
	ShExecInfo.lpDirectory = dir;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShellExecuteEx(&ShExecInfo);
	WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
}

/* true=sucess, false=error */
bool kGUISystemMINGW::MakeDirectory(const char *name)
{
	if(_mkdir(name)==0)
		return(true);
	return(false);
}

/* paste string from the system clipboard */

void kGUISystemMINGW::Paste(kGUIString *s)
{
	if(IsClipboardFormatAvailable(CF_TEXT))
    {
		GLOBALHANDLE hmem;
		const char *string;

	    if (!OpenClipboard(m_hWnd)) 
		    return; 

		hmem = GetClipboardData(CF_TEXT);
		string=(const char *)GlobalLock(hmem);
		s->SetEncoding(ENCODING_8BIT);
		s->SetString(string);
		GlobalUnlock(hmem);
        CloseClipboard() ;

	}
}

/* copy string from system clipboard */

void kGUISystemMINGW::Copy(kGUIString *s)
{
	HGLOBAL hglbCopy; 
    LPTSTR  lptstrCopy; 
	kGUIString u;

	u.SetString(s);
	u.SetEncoding(ENCODING_UTF8);
 
    if (!OpenClipboard(m_hWnd)) 
        return; 
    
	EmptyClipboard(); 

    hglbCopy = GlobalAlloc(GMEM_MOVEABLE,u.GetLen()+1); 
    if (hglbCopy == NULL) 
    { 
       CloseClipboard(); 
       return; 
    }
 
       // Lock the handle and copy the text to the buffer. 
 
    lptstrCopy = (LPTSTR)GlobalLock(hglbCopy); 
    memcpy(lptstrCopy, s->GetString(),u.GetLen()+1);
    GlobalUnlock(hglbCopy); 
 
    // Place the handle on the clipboard. 
 
	SetClipboardData(CF_TEXT, hglbCopy); 	
    CloseClipboard(); 
}

/* get filetime */

long kGUISystemMINGW::FileTime(const char *fn)
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

void kGUISystemMINGW::ReDraw(void)
{
	if(m_showwindow==false)
		ShowWindow();

//	printf("Calling ReDraw!\n");
//	fflush(stdout);
	RedrawWindow(m_hWnd,NULL,NULL,RDW_INTERNALPAINT|RDW_UPDATENOW);
//	printf("Done Calling ReDraw!\n");
//	fflush(stdout);

}

/* change mouse pointer shape to current shape type */

void kGUISystemMINGW::ChangeMouse(void)
{
	static LPSTR mousecursors[]={
		IDC_ARROW,		/* default */
		IDC_WAIT,		/* busy */
		IDC_SIZEWE,		/* adjust left/right */
		IDC_SIZENS,		/* adjust up/down */
		IDC_SIZENWSE,	/* adjust size */
		IDC_HAND};		/* hand for links on html page */
		//IDC_CROSS   Cross-hair cursor for selection
		//IDC_UPARROW   Arrow that points straight up
		//IDC_SIZEALL   A four-pointed arrow. The cursor to use to resize a window.
		//IDC_SIZENWSE   Two-headed arrow with ends at upper left and lower right
		//IDC_SIZENESW   Two-headed arrow with ends at upper right and lower left
		//IDC_SIZEWE   Horizontal two-headed arrow
		//IDC_SIZENS   Vertical two-headed arrow
		SetCursor(LoadCursor(NULL,mousecursors[kGUI::GetMouseCursor()]));
}

void kGUISystemMINGW::ShowMouse(bool show)
{
	ShowCursor(show);
}

void kGUISystemMINGW::GetWindowPos(int *x,int *y,int *w,int *h)
{
	WINDOWPLACEMENT    wp;

    wp.length = sizeof wp;

    if ( GetWindowPlacement(m_hWnd,&wp) )
	{
		x[0]=wp.rcNormalPosition.left;
		y[0]=wp.rcNormalPosition.top;
		w[0]=wp.rcNormalPosition.right-wp.rcNormalPosition.left;
		h[0]=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;
	}
}

void kGUISystemMINGW::AdjustMouse(int dx,int dy)
{
	m_mousex+=dx;
	m_mousey+=dy;
}

void kGUISystemMINGW::SetWindowPos(int x,int y)
{
	WINDOWPLACEMENT    wp;
	int w,h;

    wp.length = sizeof wp;

    if ( GetWindowPlacement(m_hWnd,&wp) )
	{
		w=wp.rcNormalPosition.right-wp.rcNormalPosition.left;
		h=wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;

		/* clip */
		if((x+w)<25)
			x=25-w;
		else if(x>(GetSystemMetrics(SM_CXSCREEN)-25))
			x=GetSystemMetrics(SM_CXSCREEN)-25;
		if(y<0)
			y=0;
		else if(y>(GetSystemMetrics(SM_CYSCREEN)-25))
			y=GetSystemMetrics(SM_CYSCREEN)-25;

		wp.rcNormalPosition.left=x;
		wp.rcNormalPosition.top=y;
		wp.rcNormalPosition.right=x+w;
		wp.rcNormalPosition.bottom=y+h;
		SetWindowPlacement(m_hWnd,&wp);
	}
}

void kGUISystemMINGW::SetWindowSize(int w,int h)
{
	WINDOWPLACEMENT    wp;

    wp.length = sizeof wp;

    if ( GetWindowPlacement(m_hWnd,&wp) )
	{
		wp.rcNormalPosition.right=wp.rcNormalPosition.left+w;
		wp.rcNormalPosition.bottom=wp.rcNormalPosition.top+h;
		SetWindowPlacement(m_hWnd,&wp);
	}
}

void kGUISystemMINGW::ShowWindow(void)
{
	m_showwindow=true;
	::ShowWindow(m_hWnd,SW_SHOW);
}

void kGUISystemMINGW::HideWindow(void)
{
	m_showwindow=false;
	::ShowWindow(m_hWnd,SW_HIDE);
}

void kGUISystemMINGW::Minimize(void)
{
	::ShowWindow(m_hWnd,SW_MINIMIZE);
}

void kGUISystemMINGW::Sleep(int ms)
{
//	printf("In Sleep...%d!\n",ms);
//	fflush(stdout);
	::Sleep(ms);
//	printf("Done Sleep...!\n");
//	fflush(stdout);

//	if(kGUI::IsAppClosed()==true)
//		exit(1);
}

/* is this a directory? */
bool kGUISystemMINGW::IsDir(const char *fn)
{
	bool isdir;
	intptr_t dir_handle;
	struct _finddata_t  dir_info;

	dir_handle = _findfirst(fn, &dir_info);
	if(dir_handle<0)
		return(false);			/* path must be unmounted or something is wrong! */
	if(dir_info.attrib&16)
		isdir=true;
	else
		isdir=false;
	_findclose(dir_handle);
	return(isdir);
}

/**********************************************************************/

/* get number of printers, default printer number and printer names */
void kGUISystemMINGW::GetPrinters(void)
{
	DWORD numprinters;
	DWORD defprinter=0;
	DWORD				dwSizeNeeded=0;
	DWORD				dwItem;
	LPPRINTER_INFO_2	printerinfo = NULL;

	// Get buffer size
	kGUI::Trace("EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS , NULL, 2, NULL, 0, &dwSizeNeeded, &numprinters );\n");

	EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS , NULL, 2, NULL, 0, &dwSizeNeeded, &numprinters );

	// allocate memory
	//printerinfo = (LPPRINTER_INFO_2)HeapAlloc ( GetProcessHeap (), HEAP_ZERO_MEMORY, dwSizeNeeded );
	kGUI::Trace("printerinfo = (LPPRINTER_INFO_2)new char[%d];\n",dwSizeNeeded);
	printerinfo = (LPPRINTER_INFO_2)new char[dwSizeNeeded];

	kGUI::Trace("EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,NULL,2,(LPBYTE)printerinfo,dwSizeNeeded,&dwSizeNeeded,&numprinters);\n");
	if ( EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,		// what to enumerate
			    NULL,			// printer name (NULL for all)
			    2,				// level
			    (LPBYTE)printerinfo,		// buffer
			    dwSizeNeeded,		// size of buffer
			    &dwSizeNeeded,		// returns size
			    &numprinters			// return num. items
			  ) == 0 )
	{
		kGUI::Trace("No printers found!;\n");
		numprinters=0;
	}

	{
		DWORD size=0;	/* if no default printer is set then GetDefaultPrinter will leave this unitlzd */

		// Get the size of the default printer name.
		kGUI::Trace("GetDefaultPrinter(NULL, &size);\n");
		GetDefaultPrinter(NULL, &size);
		kGUI::Trace("done GetDefaultPrinter size=%d;\n",size);
		if(size)
		{
		   // Allocate a buffer large enough to hold the printer name.
			kGUI::Trace("TCHAR* buffer = new TCHAR[%d];\n",size);
			TCHAR* buffer = new TCHAR[size];

			  // Get the printer name.
			kGUI::Trace("GetDefaultPrinter(buffer, &size);\n");
			GetDefaultPrinter(buffer, &size);

			for ( dwItem = 0; dwItem < numprinters; dwItem++ )
			{
				kGUI::Trace("printername[%d]='%s'\n",dwItem,printerinfo[dwItem].pPrinterName);
				if(!strcmp(buffer,printerinfo[dwItem].pPrinterName))
					defprinter=dwItem;
			}
			kGUI::Trace("delete buffer;\n");
			delete buffer;
		}
	}
	kGUI::Trace("copying printer names numprinters=%d,defaultprinter=%d;\n",numprinters,defprinter);

	/* copy printer info */
	m_numprinters=numprinters;
	m_defprinternum=defprinter;

	if(m_numprinters)
	{
		m_printers.Init(m_numprinters,0);
		for(unsigned int i=0;i<m_numprinters;++i)
			m_printers.GetEntryPtr(i)->SetName(printerinfo[i].pPrinterName);
	}
	if(printerinfo)
		delete []printerinfo;

	/* if no printers, then add a default 'no printer' so print code doesn't crash */
	if(!m_numprinters)
	{
		m_numprinters=1;
		m_printers.Init(m_numprinters,0);
		m_printers.GetEntryPtr(0)->SetName("No Printers");
		m_printers.GetEntryPtr(0)->SetDefaultPageSize();
	}
	kGUI::Trace("exiting GetPrinters();\n");
}

int numcopiesreq;		/* num copies asked to print */
int numcopiessent;		/* num printer per data send ( usually 1 or num asked for ) */

void kGUISystemMINGW::CreateHDC(const char *printerName,int numcopies,HDC *hdc)
{
	HANDLE hPrinter;

	if (OpenPrinter(const_cast<TCHAR*>(printerName),&hPrinter,NULL))
	{
		LPPRINTER_INFO_2 lpPrinterInfo2=NULL;
		DWORD sizeOfPrintInfo=0;
		DWORD cbReturned=0;

		GetPrinter(hPrinter, 2, NULL, 0, &sizeOfPrintInfo);

		lpPrinterInfo2=(LPPRINTER_INFO_2)new char[sizeOfPrintInfo];

		if (GetPrinter(hPrinter, // handle to printer
						2, // information level
						(LPBYTE)lpPrinterInfo2, // printer information buffer
						sizeOfPrintInfo, // size of buffer
						&cbReturned)) // bytes received or required
		{
			numcopiesreq=numcopies;
			if(lpPrinterInfo2->pDevMode->dmFields&DM_COPIES)
			{
//				multicopies=true;
				numcopiessent=numcopies;
				lpPrinterInfo2->pDevMode->dmCopies=numcopies;
			}
			else
			{
//				multicopies=false;
				numcopiessent=1;
				lpPrinterInfo2->pDevMode->dmCopies=1;
			}
			*(hdc)=CreateDC(lpPrinterInfo2->pDriverName, lpPrinterInfo2->pPrinterName, NULL, lpPrinterInfo2->pDevMode);
		}

		delete[] lpPrinterInfo2;
		lpPrinterInfo2=NULL;
	}
} 

void kGUISystemMINGW::GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv)
{
	HDC printerHDC;

	CreateHDC(name,1,&printerHDC);
	pw[0] = GetDeviceCaps(printerHDC,HORZRES);
	ph[0] = GetDeviceCaps(printerHDC,VERTRES);
    ppih[0] = GetDeviceCaps(printerHDC,LOGPIXELSX);
    ppiv[0] = GetDeviceCaps(printerHDC,LOGPIXELSY);
}

/* printer class */

/* return false if error starting print job */
bool kGUIPrintJobMINGW::Start(kGUIPrinter *p,const char *jobname,int numpages,int numcopies,double ppi,double lm,double rm,double tm,double bm,kGUIDrawSurface *surface,bool landscape)
{
	int ppw;
	int pph;
	double paiw,paih;
	double hardleft,hardtop;

	/* calc printable area of page in inches */
	paiw=surface->GetWidth()/ppi;
	paih=surface->GetHeight()/ppi;

	m_landscape=landscape;
	m_p=p;
	m_error=false;
	g_sys->CreateHDC(p->GetName(),numcopies,&m_printerHDC);

	::ZeroMemory (&m_pdi, sizeof (DOCINFO));
	m_pdi.cbSize = sizeof (DOCINFO);
	m_pdi.lpszDocName = jobname;

	/* get printer page size in pixels */
	ppw = GetDeviceCaps(m_printerHDC,HORZRES);
	pph = GetDeviceCaps(m_printerHDC,VERTRES);

	/* get printer pixels per inch */	
	m_ppiw = GetDeviceCaps(m_printerHDC,LOGPIXELSX);
	m_ppih = GetDeviceCaps(m_printerHDC,LOGPIXELSY);

	/* get physical unprintable margins on printer in inches */
	hardleft = (double)GetDeviceCaps(m_printerHDC,PHYSICALOFFSETX)/(double)m_ppiw;
	hardtop  = (double)GetDeviceCaps(m_printerHDC,PHYSICALOFFSETY)/(double)m_ppih;

	/* convert margins from inches to pixels */
	if(landscape==false)
	{
		lm-=hardleft;
		if(lm<0)
			lm=0;
		rm-=hardleft;
		if(rm<0)
			rm=0;
		tm-=hardtop;
		if(tm<0)
			tm=0;
		bm-=hardtop;
		if(bm<0)
			bm=0;

		m_lmp=(int)(lm*m_ppiw);
		m_rmp=(int)(rm*m_ppiw);
		m_tmp=(int)(tm*m_ppih);
		m_bmp=(int)(bm*m_ppih);

		/* printer draw area in pixels */
		m_dw=ppw-(m_lmp+m_rmp);
		m_dh=pph-(m_tmp+m_bmp);

		/* surface area of page in pixels */
		m_sw=surface->GetWidth();
		m_sh=surface->GetHeight();

		/* scale factor to convert from surface pixels to printer pixels */
		m_scalew=(double)m_dw/(double)m_sw;
		m_scaleh=(double)m_dh/(double)m_sh;
	}
	else
	{
		lm-=hardtop;
		if(lm<0)
			lm=0;
		rm-=hardtop;
		if(rm<0)
			rm=0;
		tm-=hardleft;
		if(tm<0)
			tm=0;
		bm-=hardleft;
		if(bm<0)
			bm=0;

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

	if(StartDoc (m_printerHDC,&m_pdi) <= 0)
	{
		m_error=true;
		return(false);
	}
	return(true);
}

void kGUIPrintJobMINGW::StartPage(void)
{
	if(m_error==true)	/* some error occured previously, abort */
		return;

	::StartPage (m_printerHDC);
}

void kGUIPrintJobMINGW::DrawRect(int lx,int rx,int ty,int by,kGUIColor color)
{
	int r,g,b;
	HBRUSH hBrush;
	RECT  rect;

	DrawColorToRGB(color,r,g,b);
	hBrush= CreateSolidBrush(RGB(r,g,b));

	if(m_landscape==false)
	{
		rect.left=(LONG)(lx*m_scalew)+m_lmp;
		rect.right=(LONG)(rx*m_scalew)+m_lmp;
		rect.top=(LONG)(ty*m_scaleh)+m_tmp;
		rect.bottom=(LONG)(by*m_scaleh)+m_tmp;
	}
	else
	{
		rect.left=(LONG)(ty*m_scaleh)+m_tmp;
		rect.right=(LONG)(by*m_scaleh)+m_tmp;
		rect.top=(m_dw+m_rmp)-(LONG)(rx*m_scalew);
		rect.bottom=(m_dw+m_rmp)-(LONG)(lx*m_scalew);
	}

	FillRect(m_printerHDC,&rect,hBrush);
	DeleteObject(hBrush);
}

void kGUIPrintJobMINGW::DrawTextList(int pointsize,const char *fontname,int num, Array<PRINTFONTCHAR_DEF> *list)
{
	int i;
	int r,g,b;
	int oldbg;
	RECT  rect;
	int rot;
	HFONT font;
	HFONT oldfont;
	PRINTFONTCHAR_DEF pfc;
	char c;

	if(m_landscape==false)
		rot=0;
	else
		rot=900;

	font=CreateFont((int)(pointsize*m_scaleh*1.10f),	//height
		0,	//width
		rot,	//escapment
		rot,	//orentation
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		0,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_ROMAN,
		fontname);

	oldfont=(HFONT)SelectObject(m_printerHDC,font);
	oldbg=SetBkMode(m_printerHDC,TRANSPARENT);

	for(i=0;i<num;++i)
	{
		pfc=list->GetEntry(i);

		if(m_landscape==false)
		{
			rect.left=(LONG)(pfc.lx*m_scalew)+m_lmp;
			rect.right=(LONG)(pfc.rx*m_scalew)+m_lmp;
			rect.top=(LONG)(pfc.ty*m_scaleh)+m_tmp;
			rect.bottom=(LONG)((pfc.by+((pfc.by-pfc.ty)*0.10f))*m_scaleh)+m_tmp;
		}
		else
		{
			rect.left=(LONG)(pfc.ty*m_scaleh)+m_tmp;
			rect.right=(LONG)((pfc.by+((pfc.by-pfc.ty))*0.10f)*m_scaleh)+m_tmp;
			rect.bottom=(m_dw+m_rmp)-(LONG)(pfc.rx*m_scalew);
			rect.top=(m_dw+m_rmp)-(LONG)(pfc.lx*m_scalew);
		}

		DrawColorToRGB(pfc.fgcol,r,g,b);

		SetTextColor(m_printerHDC,RGB(r,g,b));

		c=pfc.c;
		DrawText(m_printerHDC,&c,1,&rect,DT_NOPREFIX);
	}

	SetBkMode(m_printerHDC,oldbg);
	SelectObject(m_printerHDC,oldfont);
	DeleteObject(font);
}

/* copy area in current surface to the printer */

void kGUIPrintJobMINGW::DrawImage(int lx,int rx,int ty,int by)
{
	int dlx,drx,dty,dby;
	BITMAPINFO bmpi;

	/* calc destination coords on printer */
	if(m_landscape==false)
	{
		dlx=(int)(lx*m_scalew)+m_lmp;
		drx=(int)(rx*m_scalew)+m_lmp;
		dty=(int)(ty*m_scaleh)+m_tmp;
		dby=(int)(by*m_scaleh)+m_tmp;

		bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpi.bmiHeader.biWidth = kGUI::GetCurrentSurface()->GetWidth();
		bmpi.bmiHeader.biHeight = -(by-ty);
		bmpi.bmiHeader.biPlanes = 1;
		bmpi.bmiHeader.biBitCount = WINBPP;
		bmpi.bmiHeader.biCompression = BI_RGB;
		bmpi.bmiHeader.biSizeImage = 0;
		bmpi.bmiHeader.biXPelsPerMeter = 0;
		bmpi.bmiHeader.biYPelsPerMeter = 0;
		bmpi.bmiHeader.biClrUsed = 0;
		bmpi.bmiHeader.biClrImportant = 0;

		StretchDIBits(m_printerHDC,
			dlx,dty,			//upper left destination
			drx-dlx,dby-dty,	//destination width/height
			0, 0,				// upper left of source
 			rx-lx, by-ty,		// source rectangle size
			kGUI::GetCurrentSurface()->GetSurfacePtrABS(lx,ty), &bmpi, DIB_RGB_COLORS,SRCCOPY);
	}
	else
	{
		kGUIDrawSurface rotatedsurface;
		int ow,oh;
		int rw,rh;

		ow=rx-lx;	/* original width */
		oh=by-ty;	/* original height */

		rw=oh;		/* rotated */
		rh=ow;
		while(rw&3)
			++rw;	/* convert to multiple of 4 */

		rotatedsurface.Init(rw,rh);

		rotatedsurface.UnRotateSurface(kGUI::GetCurrentSurface(),lx,rx,ty,by);

		dlx=(int)(ty*m_scaleh)+m_tmp;
		drx=(int)(by*m_scaleh)+m_tmp;
		dty=(m_dw+m_rmp)-(int)(rx*m_scalew);
		dby=(m_dw+m_rmp)-(int)(lx*m_scalew);

		bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpi.bmiHeader.biWidth = rw;
		bmpi.bmiHeader.biHeight = rh;
		bmpi.bmiHeader.biPlanes = 1;
		bmpi.bmiHeader.biBitCount = WINBPP;
		bmpi.bmiHeader.biCompression = BI_RGB;
		bmpi.bmiHeader.biSizeImage = 0;
		bmpi.bmiHeader.biXPelsPerMeter = 0;
		bmpi.bmiHeader.biYPelsPerMeter = 0;
		bmpi.bmiHeader.biClrUsed = 0;
		bmpi.bmiHeader.biClrImportant = 0;

		StretchDIBits(m_printerHDC,
			dlx,dty,			//upper left destination
			drx-dlx,dby-dty,	//destination width/height
			0, 0,				// upper left of source
 			oh, ow,				// source rectangle size (swapped)
			rotatedsurface.GetSurfacePtrABS(0,0), &bmpi, DIB_RGB_COLORS,SRCCOPY);
	}
}

/* image is pre-rotated if landscape mode is selected */
void kGUIPrintJobMINGW::PrintSurface(kGUIDrawSurface *surface)
{
	int sw,sh;
	kGUIColor *bitmap;
	BITMAPINFO bmpi;

	bitmap=surface->GetSurfacePtrABS(0,0);
	sw=surface->GetWidth();
	sh=surface->GetHeight();

	if(m_error==true)	/* some error occured previously, abort */
		return;

	bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpi.bmiHeader.biWidth = sw;
	bmpi.bmiHeader.biHeight = -sh;
	bmpi.bmiHeader.biPlanes = 1;
	bmpi.bmiHeader.biBitCount = WINBPP;
	bmpi.bmiHeader.biCompression = BI_RGB;
	bmpi.bmiHeader.biSizeImage = 0;
	bmpi.bmiHeader.biXPelsPerMeter = 0;
	bmpi.bmiHeader.biYPelsPerMeter = 0;
	bmpi.bmiHeader.biClrUsed = 0;
	bmpi.bmiHeader.biClrImportant = 0;

	StretchDIBits(m_printerHDC,
		m_lmp,m_tmp,		//upper left destination
		m_dw,m_dh,			//destination width/height
		0, 0,				// upper left of source
 		sw, sh,				// source rectangle size
		bitmap, &bmpi, DIB_RGB_COLORS,SRCCOPY);
}

void kGUIPrintJobMINGW::EndPage(void)
{
	if (::EndPage (m_printerHDC)<=0)
		m_error=true;
}

/* false=error, true=ok */
bool kGUIPrintJobMINGW::End(void)
{
	if(m_error==true) /* some error occured previously, abort */
        AbortDoc (m_printerHDC);
	else
		EndDoc (m_printerHDC);

	return(!m_error);
}
