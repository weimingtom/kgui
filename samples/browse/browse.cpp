/*********************************************************************************/
/* Browse - kGUI sample program showing how to use the browser object            */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 25-Jun-2008                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguibrowse.h"
#include "kguicookies.h"

#define APPNAME "Browse"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

#define MOVIEPLAYER 1
#define USESSL 1
#define DEFMAXIMAGES 1000

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

#if USESSL
/* we use MatrixSSL */
#include "kguimatrixssl.h"
#include "kguimatrixssl.cpp"
#endif

#if MOVIEPLAYER
/* movie player */
#include "kguimovie.h"

/* needed to attach the movie player to the web browser */
#include "kguimovieplugin.h"
#endif

/* all items in the big directory are put into a 'big' file and then it is converted */
/* to inline data that is included directly into _data.cpp */

class Browse
{
public:
	Browse();
	~Browse();
private:
	CALLBACKGLUEPTR(Browse,WindowEvent,kGUIEvent);
	void WindowEvent(kGUIEvent *event);
	CALLBACKGLUE(Browse,PageChanged);
	void PageChanged(void);
	CALLBACKGLUE(Browse,SettingsChanged);
	void SettingsChanged(void);
	kGUIBrowseObj *m_browse;
	kGUIBrowseSettings m_settings;
	kGUIHTMLItemCache m_itemcache;
	kGUIHTMLVisitedCache m_visitedcache;
#if MOVIEPLAYER
	kGUIHTMLMoviePluginObj m_movieplugin;
#endif
};

Browse *g_Browse;

void AppInit(void)
{
	g_Browse=new Browse();
}

void AppClose(void)
{
	delete g_Browse;
}

Browse::Browse()
{
	kGUIWindowObj *bg;
	kGUIXML prefs;
	BigFile *bf;

	/* browser assumed first font is regular, 2nd font is bold */
	kGUI::LoadFont("font.ttf",false);	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf",true);	/* use default font inside kgui for bold */

	kGUI::SetDefFontSize(13);
	kGUI::SetDefReportFontSize(13);

   	/* init character encoding tables */
	kGUIXMLCODES::Init();

	/* Init Cookie Handler */
	kGUI::SetCookieJar(new kGUICookieJar());

#if USESSL
	/* Init SSL Handler */
	/* NOTE: This SSL Handler is GPL'd not LGPL'ed */

	bf->Extract("CAcertSrv.pem","CAcertSrv.pem");
	kGUI::SetSSLManager(new kGUIMatrixSSLManager("CAcertSrv.pem"));
#endif

	/* connect the caches to the settings */
	if(kGUI::IsDir("cache")==false)
		kGUI::MakeDirectory("cache");
	m_itemcache.SetDirectory("cache");
	m_settings.SetItemCache(&m_itemcache);
	m_settings.SetVisitedCache(&m_visitedcache);

#if MOVIEPLAYER
	kGUIMovie::InitGlobals();
#endif
	bg=kGUI::GetBackground();
 	bg->SetTitle("Browse");
	m_browse=new kGUIBrowseObj(&m_settings,bg->GetChildZoneW(),bg->GetChildZoneH());
	m_browse->SetPos(0,0);
	bg->AddObject(m_browse);

	/* load settings, including browser settins */
	prefs.SetFilename("browse.xml");
	if(prefs.Load()==true)
	{
		kGUIXMLItem *xroot;

		xroot=prefs.GetRootItem()->Locate("browse");
		if(xroot)
		{
			int wx,wy,ww,wh;

			wx=xroot->Locate("windowx")->GetValueInt();
			wy=xroot->Locate("windowy")->GetValueInt();
			ww=xroot->Locate("windoww")->GetValueInt();
			wh=xroot->Locate("windowh")->GetValueInt();

			kGUI::SetWindowPos(wx,wy);
			bg->SetSize(ww,wh);
			m_browse->SetSize(bg->GetChildZoneW(),bg->GetChildZoneH());

			m_settings.Load(xroot);
		}
	}

#if MOVIEPLAYER
	kGUIMovie::InitGlobals();
	m_browse->AddPlugin(&m_movieplugin);
#endif

	/* attach event handler to look for window resize events */
	bg->SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_browse->SetPageChangedCallback(this,CALLBACKNAME(PageChanged));
	m_browse->SetSettingsChangedCallback(this,CALLBACKNAME(SettingsChanged));

	//tell browser that we changed it's size
	m_browse->RePosition(false);

	//load default pages from config if necessary
	m_settings.LoadTabs(m_browse);

	kGUI::ShowWindow();
}

/* settings have changed so re-save the config xml file */
void Browse::SettingsChanged(void)
{

}

void Browse::PageChanged(void)
{
	kGUIString *ts;

	/* Update the windows title to match the website page title */
	ts=kGUI::GetBackground()->GetTitle();
	ts->SetString("Browse: ");
	ts->Append(m_browse->GetTitle());
}

void Browse::WindowEvent(kGUIEvent *event)
{
	/* track window re-size events */
	switch(event->GetEvent())
	{
	case EVENT_SIZECHANGED:
	{
		kGUIWindowObj *bg;

		bg=kGUI::GetBackground();

		/* update the browse window size */
		m_browse->SetSize(bg->GetChildZoneW(),bg->GetChildZoneH());
		m_browse->RePosition(false);
	}
	break;
	}
}

Browse::~Browse()
{
	kGUIXML prefs;
	kGUIXMLItem *xroot;
	int wx,wy,ww,wh;

	m_settings.SaveTabs(m_browse);

	delete m_browse;

	/* generate the XML file for our saved settings */
	prefs.SetEncoding(ENCODING_UTF8);
	xroot=prefs.GetRootItem()->AddChild("browse");

	/* special case for base window */
	kGUI::GetWindowPos(&wx,&wy,&ww,&wh);

	xroot->AddChild("windowx",wx);
	xroot->AddChild("windowy",wy);
	xroot->AddChild("windoww",ww);
	xroot->AddChild("windowh",wh);
	/* save the settings from the browse object */
	m_settings.Save(xroot);

	/* save the xml file */
	prefs.SetFilename("browse.xml");
	prefs.Save();

	delete kGUI::GetCookieJar();
#if USESSL
	delete kGUI::GetSSLManager();
#endif
	kGUIXMLCODES::Purge();
}
