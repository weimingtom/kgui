#include <windows.h>

#include <winsock.h>

#define MAX_LOADSTRING 100
#define FRAMES_PER_SECOND 60

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif
#define WM_MOUSEWHEEL                   0x020A

WSADATA SockData;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
RECT WinRect;
RECT UpdateWinRect;
kGUI gui;

void AppInit(void);
void AppClose(void);
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
void Draw(HWND hWnd,int ulx,int urx,int uty,int uby);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	kGUI::Trace("------------------------------------------------------------------------------\n");
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	kGUI::Trace(" LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);\n");
	LoadString(hInstance, IDC_APPNAME, szWindowClass, MAX_LOADSTRING);
	kGUI::Trace(" LoadString(hInstance, IDC_APPNAME, szWindowClass, MAX_LOADSTRING);\n");
	MyRegisterClass(hInstance);
	kGUI::Trace(" MyRegisterClass(hInstance);\n");

	// Perform application initialization:
	kGUI::Trace(" InitInstance(hInstance, nCmdShow);\n");
	if (!InitInstance(hInstance, nCmdShow)) 
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return FALSE;
	}
	kGUI::Trace(" LoadAccelerators(hInstance, (LPCTSTR)IDC_APPNAME);\n");
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_APPNAME);

	kGUI::Trace(" WSAStartup(0x0202,&SockData);\n");
    if(WSAStartup(0x0202,&SockData))
	{
		kGUI::Trace(" error initing WSAStartup;\n");
	}

	// Main message loop:
	kGUI::Trace(" Entering Main loop: GetMessage(&msg, NULL, 0, 0);\n");
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	kGUI::Trace(" Exited Main loop, program done.\n");

	return (int) msg.wParam;
}

int lasttickcount;

//
//   FUNCTION: InitInstance(HANDLE)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

	HWND g_hWnd;

//#define WS_EX_LAYERED 0x80000
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int startwidth,startheight;
	int fullwidth,fullheight,maximages;

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
	startheight=fullheight;
#endif
#ifdef DEFMAXIMAGES
	maximages=DEFMAXIMAGES;
#else
	maximages=200;
#endif

	lasttickcount=GetTickCount();
	
//	hInst = hInstance; // Store instance handle in our global variable

	kGUI::Trace(" hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE, 0, 0, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL);\n");
 
//WS_POPUP | WS_VISIBLE
//WS_OVERLAPPEDWINDOW
	g_hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE /*|WS_EX_LAYERED*/ ,
	    (fullwidth-startwidth)/2, (fullheight-startheight)/2, startwidth, startheight, NULL, NULL, hInstance, NULL);

	//SetLayeredWindowAttributes(hWnd,RGB(0,0,0),255,LWA_COLORKEY);

	if (!g_hWnd)
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return FALSE;
	}
	// Allocate BMP data for screen
//	GetWindowRect(hWnd,&WinRect); 
	kGUI::Trace(" GetClientRect(hWnd,&WinRect);\n");
	GetClientRect(g_hWnd,&WinRect); 

	/* initialize kGUI engine */

	kGUI::Trace(" kGUI::Init(WinRect.right-WinRect.left,WinRect.bottom-WinRect.top);\n");
	if(kGUI::Init(WinRect.right-WinRect.left,WinRect.bottom-WinRect.top,fullwidth,fullheight,maximages)==false)
	{
		kGUI::Trace(" Last Function failed, program aborting!\n");
		return FALSE;
	}

	kGUI::Trace(" AppInit();\n");
	AppInit();

	kGUI::Trace(" ShowWindow(g_hWnd, nCmdShow);\n");
	ShowWindow(g_hWnd, nCmdShow);
	kGUI::Trace(" UpdateWindow(g_hWnd);\n");
    UpdateWindow(g_hWnd);

	kGUI::Trace(" Draw(g_hWnd);\n");
    Draw(g_hWnd,0,0,0,0);
	kGUI::Trace(" SetTimer(g_hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);\n");
    SetTimer(g_hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);
	kGUI::Trace(" InitInstance - Finished sucessfully\n");
	return(TRUE);
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_APPNAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

void Draw(HWND hWnd,int ulx,int urx,int uty,int uby)
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
	bmp_data=kGUI::Draw(&c);
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
#if 0
	if(nl!=dh)
	{
		/* not drawn!, not sure why */
		DWORD dw = GetLastError(); 

		if(dw)	/* 0=ok, must have draw area off screen since moving around */
		{
			LPVOID lpMsgBuf;

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );

			passert(false,"%s",lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
	}
#endif
	ReleaseDC(hWnd, hDC);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//

static	int mx=0,my=0,wheeldelta=0;
 int g_mcx=0,g_mcy=0;
static int waskilled=0;
static bool lbutton=false,rbutton=false;
//static int lmc=-1;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int key;
	int newtick,deltatick;

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		return DefWindowProc(hWnd, message, wParam, lParam);
	break;
	case WM_PAINT:
	{
		// Get the Windows update region
		while(kGUI::GetInDraw()==true || kGUI::GetInThread()==true);

		{
			kGUI::SetInDraw(true);
			GetUpdateRect(hWnd, &UpdateWinRect, FALSE);
			hdc = BeginPaint(hWnd, &ps);
			Draw(hWnd,UpdateWinRect.left,UpdateWinRect.right,UpdateWinRect.top,UpdateWinRect.bottom);
//			Draw(hWnd,0,0,0,0);
			EndPaint(hWnd, &ps);
			kGUI::SetInDraw(false);
		}
	}
	break;
	case WM_TIMER:
		newtick=GetTickCount();
		deltatick=newtick-lasttickcount;
		if(deltatick<0)
			deltatick=0;
		lasttickcount=newtick;

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

		kGUI::UpdateInput();

		wheeldelta=0;

		while(kGUI::GetInDraw()==true || kGUI::GetInThread()==true);

		{
			kGUI::SetInDraw(true);
			Draw(hWnd,0,0,0,0);
			kGUI::SetInDraw(false);
		}
		if(kGUI::IsAppClosed()==true)
			DestroyWindow(hWnd);
		else
			SetTimer(hWnd, 0, 1000 / FRAMES_PER_SECOND, NULL);
		break;
	case WM_DESTROY:
		if(!waskilled)
		{
			waskilled=1;	/* sometimes this get's called more than once, so allow only once! */
			AppClose();
			kGUI::Close();
			PostQuitMessage(0);
		    WSACleanup();
		}
	break;
    case WM_MOUSEWHEEL:
		wheeldelta = ( int ) ( ( short ) HIWORD( wParam ) )/64;
		kGUI::SetMouse(mx+g_mcx,my+g_mcy,wheeldelta,lbutton,rbutton);
	break;
	case WM_LBUTTONDOWN: 
    case WM_LBUTTONUP: 
	case WM_RBUTTONDOWN: 
    case WM_RBUTTONUP: 
    case WM_MOUSEMOVE:
		if(kGUI::GetMouseCursor()!=MOUSECURSOR_DEFAULT)	/* if not default arrow */
			kGUI::ChangeMouse();
		mx=GET_X_LPARAM(lParam);
		my=GET_Y_LPARAM(lParam);
		lbutton=(wParam&MK_LBUTTON)!=0;
		rbutton=(wParam&MK_RBUTTON)!=0;
		
		/* this tells windows to keep updating me even if the mouse is moved off of */
		/* my window area */
		if(lbutton || rbutton)
			SetCapture(hWnd); 
		else
		{
			g_mcx=0;
			g_mcy=0;
			ReleaseCapture(); 
		}
		kGUI::SetMouse(mx+g_mcx,my+g_mcy,wheeldelta,lbutton,rbutton);
//		OutputDebugString("mouseinput\n");
		kGUI::Tick(0);
		kGUI::UpdateInput();

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
		case VK_CONTROL:	/* ignore these keys... */
		case VK_SHIFT:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
		case VK_SLEEP:
		break;
		default:
//			key=(int)wParam;	/* control keys */
			key=0;
		break;
		}
		if(key)
			kGUI::KeyPressed(key);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
