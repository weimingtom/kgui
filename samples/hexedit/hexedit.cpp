/*********************************************************************************/
/* HEXEdit - kGUI sample program.                                                */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 20-May-2009                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguireq.h"

#define APPNAME "HEXEdit Sample"

#if defined(WIN32) || defined(MINGW)
#include "resource.h"
#endif

#include "kguisys.cpp"

#if 1

#define loslib_c
#define LUA_CORE

#define fputs myfputs

kGUIString debug;

void myfputs(const char *c,FILE *f)
{
	debug.Append(c);
}

#include "lua\lua.c"
#include "lua\lstate.c"
#include "lua\lapi.c"
#include "lua\lauxlib.c"
#include "lua\linit.c"
#include "lua\lgc.c"
#include "lua\lmem.c"
#include "lua\lfunc.c"
#include "lua\ldo.c"
#include "lua\lstring.c"
#include "lua\llex.c"
#include "lua\ltm.c"
#include "lua\ltable.c"
#include "lua\lobject.c"
#include "lua\lvm.c"
#include "lua\lzio.c"
#include "lua\ldump.c"
#include "lua\lundump.c"
#include "lua\lparser.c"
#include "lua\lcode.c"
#include "lua\lopcodes.c"

#include "lua\ldblib.c"
#include "lua\lmathlib.c"
#include "lua\lstrlib.c"
#include "lua\loslib.c"
#include "lua\liolib.c"
#include "lua\ltablib.c"
#include "lua\loadlib.c"
#include "lua\lbaselib.c"
#include "lua\ldebug.c"

#endif

class HexListObj : public kGUIObj
{
public:
	HexListObj();
	~HexListObj();
	void SetDatahandle(DataHandle *dh);
	void ZoneChanged(void);
	void Draw(void);
	bool UpdateInput(void);
	void SetEdit(bool e) {m_nibble=0;m_edit=e;m_cursorflash=0;m_drawcursor=1;Dirty();}
private:
	unsigned long long m_size;
	bool m_edit;
	int m_nibble;
	int m_cursorflash;
	int m_drawcursor;
	int m_scrolly;
	int m_cursorx;
	int m_cursory;
	DataHandle *m_dh;
	int m_cw;
	int m_lh;
	int m_numrows;
	kGUIText m_t;
};

class HEXEditSample
{
public:
	HEXEditSample();
	~HEXEditSample();
	kGUIInputBoxObj m_debug;
private:
	CALLBACKGLUEPTR(HEXEditSample,EditEvent,kGUIEvent);			/* make a static connection to the callback */
	void EditEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(HEXEditSample,OpenMenuEvent,kGUIEvent);		/* make a static connection to the callback */
	void OpenMenuEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(HEXEditSample,MenuEvent,kGUIEvent);			/* make a static connection to the callback */
	void MenuEvent(kGUIEvent *event);
	CALLBACKGLUEPTRVAL(HEXEditSample,Load,kGUIFileReq,int);		/* make a static connection to the callback */
	void Load(kGUIFileReq *req,int status);

	kGUITextObj m_menucaption;
	kGUIMenuColObj m_popmenu;
	kGUIInputBoxObj m_filename;
	kGUITickBoxObj m_edit;
	kGUITextObj m_editcaption;
	DataHandle m_hexdh;
	HexListObj m_hexlist;
};

HEXEditSample *g_hex;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui for bold */
	kGUI::SetDefFontSize(15);
	kGUI::SetDefReportFontSize(20);

	g_hex=new HEXEditSample();

	{
   lua_State* luaVM = lua_open();
 
   if (NULL == luaVM)
   {
      printf("Error Initializing lua\n");
      return;
   }
	luaL_openlibs(luaVM);

   // Do stuff with lua code.
   char* strLuaInput = "a = 1 + 1;\nprint( a);\n";
 
   luaL_dostring(luaVM, strLuaInput);
    
   lua_close( luaVM );
	}
}

void AppClose(void)
{
	delete g_hex;
}

enum
{
MENU_LOAD,
MENU_EXIT,
MENU_NUMENTRIES
};

enum
{
COLMENU_INSERTCOLBEFORE,
COLMENU_INSERTCOLAFTER,
COLMENU_DELETECOL,
COLMENU_SORTASC,
COLMENU_SORTDESC,
COLMENU_SORTASC2,
COLMENU_SORTDESC2,
COLMENU_NUMENTRIES};

HEXEditSample::HEXEditSample()
{
	kGUIWindowObj *background;

	g_hex=this;
	background=kGUI::GetBackground();
 	background->SetTitle("HexEdit");

	/* this is static text that when clicked on opens the popup menu */
	m_menucaption.SetFontSize(20);
	m_menucaption.SetString("Menu");
	m_menucaption.SetEventHandler(this,CALLBACKNAME(OpenMenuEvent));
	background->AddObject(&m_menucaption);

	/* let's populate the popup menu */
	m_popmenu.SetNumEntries(MENU_NUMENTRIES);
	m_popmenu.SetEntry(MENU_LOAD,"Load");
	m_popmenu.SetEntry(MENU_EXIT,"Exit");
	m_popmenu.SetEventHandler(this,CALLBACKNAME(MenuEvent));

	m_filename.SetFontSize(13);
	m_filename.SetPos(0,m_menucaption.GetZoneBY()+4);
	m_filename.SetSize(640,20);
	m_filename.SetString("kgui.log");
	m_filename.SetLocked(true);
	background->AddObject(&m_filename);
	
	m_edit.SetPos(0,m_filename.GetZoneBY()+4);
	m_edit.SetEventHandler(this,CALLBACKNAME(EditEvent));
	background->AddObject(&m_edit);

	m_editcaption.SetFontSize(20);
	m_editcaption.SetPos(m_edit.GetZoneRX()+4,m_filename.GetZoneBY()+4);
	m_editcaption.SetString("Edit");
	background->AddObject(&m_editcaption);

	m_hexdh.SetFilename(m_filename.GetString());
	m_hexlist.SetPos(0,m_editcaption.GetZoneBY()+4);
	m_hexlist.SetSize(640,480);
	background->AddObject(&m_hexlist);
	m_hexlist.SetDatahandle(&m_hexdh);
	m_hexlist.SetCurrent();
	background->Shrink();
	kGUI::ShowWindow();
}

void HEXEditSample::OpenMenuEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_LEFTCLICK)
		m_popmenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

void HEXEditSample::MenuEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		switch(event->m_value[0].i)
		{
		case MENU_LOAD:
		{
			kGUIFileReq *loadreq;

			loadreq=new kGUIFileReq(FILEREQ_OPEN,"",0,this,CALLBACKNAME(Load));
		}
		break;
		case MENU_EXIT:
			kGUI::CloseApp();
		break;
		}
	}
}

/* you can have a unique event handler for each object, or you can have one to handle many objects */
void HEXEditSample::EditEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_AFTERUPDATE:
		m_hexlist.SetEdit(m_edit.GetSelected());
		m_hexlist.SetCurrent();
	break;
	}
}

void HEXEditSample::Load(kGUIFileReq *req,int status)
{
	if(status==MSGBOX_OK)
	{
		if(m_hexdh.GetOpen())
			m_hexdh.Close();
		m_hexdh.SetFilename(req->GetFilename());
		m_filename.SetString(req->GetFilename());
		m_hexlist.SetDatahandle(&m_hexdh);
	}
}

HEXEditSample::~HEXEditSample()
{
}

HexListObj::HexListObj()
{
	m_dh=0;

	m_t.SetBGColor(DrawColor(255,0,0));
	m_t.SetFontID(0);
	m_t.SetFontSize(13);
	m_t.SetString("a");
	m_cw=m_t.GetWidth();
	m_lh=m_t.GetLineHeight()+2;
}

void HexListObj::ZoneChanged(void)
{
	m_numrows=(GetZoneH()-4)/m_lh;
}

void HexListObj::SetDatahandle(DataHandle *dh)
{
	if(dh->Open())
	{
		m_dh=dh;
		m_size=dh->GetSize();
	}
	else
	{
		m_dh=0;
		m_size=0;
	}
	m_scrolly=0;
	m_cursorx=0;
	m_cursory=0;
	m_nibble=0;
	m_edit=false;
	Dirty();
}

HexListObj::~HexListObj()
{
	if(m_dh)
		m_dh->Close();
}

void HexListObj::Draw(void)
{
	unsigned long long offset;
	unsigned long long cursor;
	unsigned int row;
	int x;
	int y;
	kGUICorners c;
	static const char hex[16+1]={"0123456789abcdef"};
	unsigned char byte[16];

	GetCorners(&c);
	kGUI::DrawRectBevelIn(c.lx,c.ty,c.rx,c.by);
	offset=m_scrolly*16;
	cursor=m_cursory*16+m_cursorx;
	y=c.ty+2;
	m_dh->Seek(offset);
	do
	{
		x=c.lx+2;

		m_t.SetUseBGColor(false);
		m_t.Sprintf("%c%c%c%c%c%c%c%c:",hex[(offset>>28)&15],hex[(offset>>24)&15],hex[(offset>>20)&15],hex[(offset>>16)&15],hex[(offset>>12)&15],hex[(offset>>8)&15],hex[(offset>>4)&15],hex[offset&15]);
		m_t.Draw(x,y,0,0);
		x+=m_cw*10;

		for(row=0;row<16;++row)
		{
			if((offset+row)<m_size)
			{
				m_dh->Read(&byte[row],(unsigned long)1);
				if((offset+row)==cursor)
				{
					if(m_edit)
					{
						m_t.SetUseBGColor(m_nibble==0 && m_drawcursor);
						m_t.Sprintf("%c",hex[byte[row]>>4]);
						m_t.Draw(x,y,0,0);
						m_t.SetUseBGColor(m_nibble==1 && m_drawcursor);
						m_t.Sprintf("%c",hex[byte[row]&15]);
						m_t.Draw(x+m_cw,y,0,0);
					}
					else
					{
						m_t.SetUseBGColor(true);
						m_t.Sprintf("%c%c",hex[byte[row]>>4],hex[byte[row]&15]);
						m_t.Draw(x,y,0,0);
					}
				}
				else
				{
					m_t.SetUseBGColor(false);
					m_t.Sprintf("%c%c",hex[byte[row]>>4],hex[byte[row]&15]);
					m_t.Draw(x,y,0,0);
				}
			}
			x+=m_cw*3;
		}

		x+=m_cw*2;
		for(row=0;row<16;++row)
		{
			if((offset+row)<m_size)
			{
				if((offset+row)==cursor && m_edit==false)
					m_t.SetUseBGColor(true);
				else
					m_t.SetUseBGColor(false);
				m_t.Sprintf("%c",byte[row]);
				m_t.Draw(x,y,0,0);
			}
			x+=m_cw;
		}
		offset+=16;

		y+=m_lh;
	}while(y<c.by);
}

bool HexListObj::UpdateInput(void)
{
	bool used=false;

	if(m_size)
	{
		int key;
		int wd;

		if(m_edit)
		{
			m_cursorflash+=kGUI::GetET();
			while(m_cursorflash>=(TICKSPERSEC/3))
			{
				m_drawcursor^=1;
				m_cursorflash-=TICKSPERSEC/3;
				Dirty();
			}
		}

		key=kGUI::GetKey();
		switch(key)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			if(m_edit)
			{
				unsigned long long offset;
				unsigned char byte;
				unsigned char nib;

				used=true;
				offset=(m_cursory*16)+m_cursorx;
				m_dh->Seek(offset);
				m_dh->Read(&byte,(unsigned long)1);

				if(key>='0' && key<='9')
					nib=(unsigned char)(key-'0');
				else
					nib=(unsigned char)(key-'a')+10;

				if(!m_nibble)
				{
					/* high nibble */
					byte=nib<<4|(byte&15);
				}
				else
				{
					/* low nibble */
					byte=(byte&0xf0)|nib;
				}
				m_dh->Close();

#if 0
				m_dh->OpenWrite("rb+",0);
				m_dh->Seek(offset);
				m_dh->Write(&byte,1L);
				m_dh->Close();
#endif

				m_dh->Open();
			}
		break;
		case GUIKEY_LEFT:
			if(m_edit)
			{
				used=true;
				m_nibble^=1;
				if(!m_nibble)
					break;
			}
			if(m_cursorx)
			{
				used=true;
				--m_cursorx;
			}
			else if(m_cursory)
			{
				used=true;
				--m_cursory;
				m_cursorx=15;
			}
		break;
		case GUIKEY_RIGHT:
			used=true;
			if(m_edit)
			{
				m_nibble^=1;
				if(m_nibble)
					break;
			}
			if((m_cursorx+1)<16)
				++m_cursorx;
			else
			{
				m_cursorx=0;
				++m_cursory;
			}
		break;
		case GUIKEY_UP:
			if(m_cursory)
			{
				used=true;
				--m_cursory;
			}
		break;
		case GUIKEY_DOWN:
			used=true;
			++m_cursory;
		break;
		case GUIKEY_PGUP:
			if(m_cursory<m_numrows)
			{
				used=true;
				m_cursory=0;
			}
			else
			{
				used=true;
				m_cursory-=m_numrows;
			}
		break;
		case GUIKEY_PGDOWN:
			m_cursory+=m_numrows;
			used=true;
		break;
		case GUIKEY_HOME:
			m_cursorx=0;
			m_cursory=0;
			m_scrolly=0;
			used=true;
		break;
		case GUIKEY_END:
			used=true;
			m_cursorx=(m_size-1)&15;
			m_cursory=(m_size-1)>>4;
			m_scrolly=m_cursory-m_numrows;
			if(m_scrolly<0)
				m_scrolly=0;
		break;
		}

		if(kGUI::GetMouseClickLeft())
		{
			kGUICorners c;
			int row,col;

			GetCorners(&c);
			c.ty+=2;
			c.lx+=m_cw*10;

			row=(kGUI::GetMouseY()-c.ty)/m_lh;
			if(row>=0 && row<=m_numrows)
			{
				col=(kGUI::GetMouseX()-c.lx)/(m_cw*3);
				if(col>=0 && col<16)
				{
					m_cursory=m_scrolly+row;
					m_cursorx=col;
					used=true;
				}
			}
		}
		wd=kGUI::GetMouseWheelDelta();
		if(wd)
		{
			kGUI::ClearMouseWheelDelta();
			m_cursory-=wd;
			if(m_cursory<0)
				m_cursory=0;
			used=true;
		}

		if(used)
		{
			if((m_cursory*16)+m_cursorx>=m_size)
			{
				m_cursorx=(m_size-1)&15;
				m_cursory=(m_size-1)>>4;
			}
			while(m_cursory<m_scrolly)
				--m_scrolly;
			while((m_scrolly+m_numrows)<m_cursory)
				++m_scrolly;

			Dirty();
		}
	}

	return(used);
}
