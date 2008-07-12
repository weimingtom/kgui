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

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

/* we use MatrixSSL */
#include "kguimatrixssl.h"
#include "kguimatrixssl.cpp"

#define MOVIEPLAYER 1

#if MOVIEPLAYER
/* movie player */
#include "kguimovie.h"

/* needed to attach the movie player to the web browser */
#include "kguimovieplugin.h"
#endif

/* all items in the big directory are put into a 'big' file and then it is converted */
/* to inline data that is included directly into _data.cpp */

#include "_brdata.cpp"

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

	/* add bigfile to the list of system bigfiles */
	bf=new BigFile();
	bf->SetMemory(bin__brdata,sizeof(bin__brdata));
	DataHandle::AddBig(bf);

	/* browser assumed first font is regular, 2nd font is bold */
	kGUI::LoadFont("arial.ttf");
	kGUI::LoadFont("arialbd.ttf");

	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	/* init character encoding tables */
	kGUIXMLCODES::Init();

	/* Init Cookie Handler */
	kGUI::SetCookieJar(new kGUICookieJar());

	/* Init SSL Handler */
	/* NOTE: This SSL Handler is GPL'd not LGPL'ed */
	kGUI::SetSSLManager(new kGUIMatrixSSLManager("CAcertSrv.pem"));

	/* connect the caches to the settings */
	if(kGUI::IsDir("cache")==false)
		kGUI::MakeDirectory("cache");
	m_itemcache.SetDirectory("cache");
	m_settings.SetItemCache(&m_itemcache);
	m_settings.SetVisitedCache(&m_visitedcache);

	/* load settings, including browser settins */
	if(prefs.Load("browse.xml")==true)
	{
		kGUIXMLItem *xroot;
		
		xroot=prefs.GetRootItem()->Locate("browse");
		if(xroot)
			m_settings.Load(xroot);
	}

	bg=kGUI::GetBackground();
 	bg->SetTitle("Browse");

	m_browse=new kGUIBrowseObj(&m_settings,bg->GetChildZoneW(),bg->GetChildZoneH());
	m_browse->SetPos(0,0);
#if MOVIEPLAYER
	kGUIMovie::InitGlobals();
	m_browse->AddPlugin(&m_movieplugin);
#endif
	bg->AddObject(m_browse);

	/* attach event handler to look for window resize events */
	bg->SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_browse->SetPageChangedCallback(this,CALLBACKNAME(PageChanged));
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
	if(event->GetEvent()==EVENT_SIZECHANGED)
	{
		kGUIWindowObj *bg;

		bg=kGUI::GetBackground();

		/* update the browse window size */
		m_browse->SetSize(bg->GetChildZoneW(),bg->GetChildZoneH());
		m_browse->RePosition(true);
	}
}

Browse::~Browse()
{
	kGUIXML prefs;
	kGUIXMLItem *xroot;

	delete m_browse;

	/* generate the XML file for our saved settings */
	prefs.SetEncoding(ENCODING_UTF8);
	xroot=prefs.GetRootItem()->AddChild("browse");
	m_settings.Save(xroot);

	/* save the xml file */
	prefs.Save("browse.xml");

	delete kGUI::GetCookieJar();
	delete kGUI::GetSSLManager();

	kGUIXMLCODES::Purge();
}
