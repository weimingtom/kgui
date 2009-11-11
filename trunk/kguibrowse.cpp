/**********************************************************************************/
/* kGUI - kguibrowse.cpp                                                          */
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

/*! @file kguibrowse.cpp 
   @brief This is a browser object that you can have in your own application             
   It uses the kGUIHTMLObj for rendering and uses the download class to download 
   files from the internet. By default it doesn't use the movie player but there  
   is a movie player plugin class that can be 'attached' so it will also play     
   movies. The reason it is seperate is that the movie player is very large so    
   this way you can decide to include it or not. */

//using namespace kguibrowselocal;

#include "kgui.h"
#include "kguibrowse.h"
#include "_text.h"

#include <npapi.h>

#define SMALLCAPTIONFONT 1
#define SMALLCAPTIONFONTSIZE 9

class ViewSource : public kGUIWindowObj
{
public:
	ViewSource(int w,int h,kGUIString *title,kGUIString *source);
	/* called when window size changes */
	void DirtyandCalcChildZone(void);
private:
	CALLBACKGLUEPTR(ViewSource,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	kGUIInputBoxObj m_source;
};

class CSSBlockRowObj : public kGUITableRowObj
{
public:
	CSSBlockRowObj(unsigned int index);
	~CSSBlockRowObj() {}
	int GetNumObjects(void) {return 3;}
	kGUIObj **GetObjectList(void) {return m_objectlist;}
	void SetBlocked(bool b) {m_block.SetSelected(b);} 
	bool GetBlocked(void) {return m_block.GetSelected();}
	void SetTrace(bool t) {m_trace.SetSelected(t);} 
	bool GetTrace(void) {return m_trace.GetSelected();}
private:
	kGUIObj *m_objectlist[3];
	kGUITickBoxObj m_block;
	kGUITickBoxObj m_trace;
	kGUIInputBoxObj m_name;
};

class ViewSettings : public kGUIWindowObj
{
public:
	ViewSettings(kGUIBrowseObj *b,int w,int h);
	~ViewSettings() {m_csstable.DeleteChildren(true);}
	/* called when window size changes */
	void DirtyandCalcChildZone(void);
private:
	CALLBACKGLUEPTR(ViewSettings,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(ViewSettings,ButtonsEvent,kGUIEvent)
	void ButtonsEvent(kGUIEvent *event);

	kGUIBrowseObj *m_b;
	kGUIScrollContainerObj m_scroll;
	kGUIControlBoxObj m_controls;

	/* browser settings */
	/* number of days to save visited links for */
	kGUITextObj m_visiteddayscaption;
	kGUITextObj m_visiteddayscaption2;
	kGUIInputBoxObj m_visiteddays;

	/* settings for download cache mode to use */
	kGUIComboBoxObj m_cachemode;
	kGUITextObj m_cachesizecaption;
	kGUITextObj m_cachesizecaption2;
	kGUIInputBoxObj m_cachesize;

	/* settings for save tabs mode to use */
	kGUITextObj m_savemodecaption;
	kGUIComboBoxObj m_savemode;

	/* draw options */

	/* load images */
	kGUITextObj m_loadimagescaption;
	kGUITickBoxObj m_loadimages;

	/* use CSS */
	kGUITextObj m_usecsscaption;
	kGUITickBoxObj m_usecss;

	/* use user CSS */
	kGUITextObj m_useusercsscaption;
	kGUITickBoxObj m_useusercss;

	/* user CSS */
	kGUITextObj m_usercsscaption;
	kGUIInputBoxObj m_usercss;


	/* draw frame on boxes */
	kGUITextObj m_drawboxcaption;
	kGUITickBoxObj m_drawbox;

	/* draw areas */
	kGUITextObj m_drawareascaption;
	kGUITickBoxObj m_drawareas;
	kGUIComboBoxObj m_drawareascolor;

	/* screen media to use */
	kGUITextObj m_screenmediacaption;
	kGUIComboBoxObj m_screenmedia;

	/* print media to use */
	kGUITextObj m_printmediacaption;
	kGUIComboBoxObj m_printmedia;

	/* color simulator to use */
	kGUITextObj m_colormodecaption;
	kGUIComboBoxObj m_colormode;

	kGUIButtonObj m_clear;
	kGUIButtonObj m_set;
	kGUIButtonObj m_toggle;

	kGUITableObj m_csstable;
};

void HistoryRecord::Set(const char *url,const char *post,const char *referer,const char *source,const char *header)
{
	/* title defaults to URL and is then replaced if there is a valid title in the html page */
	m_title.SetString(url);
	m_url.SetString(url);
	m_post.SetString(post);
	m_referer.SetString(referer);
	m_source.SetString(source);
	m_header.SetString(header);
}

enum
{
MAINMENU_NEWTAB,
MAINMENU_SAVEPAGE,
MAINMENU_VIEWPAGESOURCE,
MAINMENU_VIEWCORRECTEDPAGESOURCE,
MAINMENU_VIEWPOSTDATA,
MAINMENU_VIEWHEADER,
MAINMENU_VIEWSCRIPTSOURCE,
MAINMENU_VIEWCSS,
MAINMENU_VIEWCORRECTEDCSS,
MAINMENU_VIEWERRORS,
MAINMENU_VIEWMEDIA,
MAINMENU_TRACELAYOUT,
MAINMENU_TRACEDRAW,
MAINMENU_VIEWCOOKIES,
MAINMENU_SETTINGS,
MAINMENU_NUM};

enum
{
BOOKMARK_EDITBOOKMARK=MAINMENU_NUM,
BOOKMARK_ADDBOOKMARK,
BOOKMARK_BOOKMARKS
};

static const char *mainmenutxt[]={
	"New Tab",
	"Save Page As",
	"View Page Source",
	"View Corrected Page Source",
	"View Post Data",
	"View Header",
	"View Script Source",
	"View CSS Source",
	"View Corrected CSS Source",
	"View Page Errors",
	"View Page Media",
	"Trace Layout",
	"Trace Draw",
	"View Cookies",
	"Settings"};

/* colorblind simulator modes */

typedef struct
{
	const char *name;
	double rr;
	double rg;
	double rb;
	double gr;
	double gg;
	double gb;
	double br;
	double bg;
	double bb;
}COLOR_MATRIX;

static const COLOR_MATRIX colormatrix[]={
	{"Normal",			1.0f,	0.0f,	0.0f,	0.0f,	1.0f,	0.0f,	0.0f,	0.0f,	1.0f},
	{"Protanopia",		0.567f,	0.433f,	0.0f,	0.558f,	0.442f,	0.0f,	0.0f,	0.242f,	0.758f},
	{"Protanomaly",		0.817f,	0.183f,	0.0f,	0.333f,	0.667f,	0.0f,	0.0f,	0.125f,	0.875f},
	{"Deuteranopia",	0.625f,	0.375f,	0.0f,	0.7f,	0.3f,	0.0f,	0.0f,	0.3f,	0.7f},
	{"Deuteranomaly",	0.8f,	0.2f,	0.0f,	0.258f,	0.742f,	0.0f,	0.0f,	0.142f,	0.858f},
	{"Tritanopia",		0.95f,	0.05f,	0.0f,	0.0f,	0.433f,	0.567f,	0.0f,	0.475f,	0.525f},
	{"Tritanomaly",		0.967f,	0.033f,	0.0f,	0.0f,	0.733f,	0.267f,	0.0f,	0.183f,	0.817f},
	{"Achromatopsia",	0.299f,	0.587f,	0.114f,	0.299f,	0.587f,	0.114f,	0.299f,	0.587f,	0.114f},
	{"Achromatomaly",	0.618f,	0.320f,	0.062f,	0.163f,	0.775f,	0.062f,	0.163f,	0.320f,	0.516f}};

void kGUITabBrowseIcon::Dirty(void)
{
	m_tabobj->DirtyTab(m_tabnum);
}

void kGUIBrowseTabObj::DrawTab(int tabindex,kGUIText *text,int x,int y)
{
	kGUICorners c;

	if(m_icons.GetEntryPtr(tabindex)->GetIsValid())
		m_icons.GetEntryPtr(tabindex)->Draw(x,y-2);	

	/* if text is really long then we need to clip it */
	kGUI::PushClip();
	c.lx=x;
	c.rx=x+BROWSETABWIDTH;
	c.ty=y;
	c.by=y+text->GetLineHeight();
	kGUI::ShrinkClip(&c);
	text->Draw(x+20,y,0,0);
	kGUI::PopClip();
}


TabRecord::TabRecord()
{
	m_curdl=0;
	m_histindex=0;
	m_histend=0;
	m_iconvalid=false;
	m_reposition=false;
	m_reparse=false;
}

HistoryRecord *TabRecord::NextPage(void)
{
	int i;
	HistoryRecord *pi;

	if(m_histindex==MAXPAGES)
	{
		/* scroll pages down and throw away oldest */
		for(i=0;i<(MAXPAGES-1);++i)
			m_history[i].Copy(&m_history[i+1]);

		/* use last available page */
		pi=&m_history[MAXPAGES-1];
	}
	else
		pi=&m_history[m_histindex++];

	m_histend=m_histindex;
	return(pi);
}

kGUIBrowseObj::kGUIBrowseObj(kGUIBrowseSettings *settings,int w,int h)
{
	kGUIString s;

	m_numdlactive=0;
	m_dlactive.Init(4,4);
//	m_dlpool.SetBlockSize(8);

	m_printpage.SetSettings(settings);
	m_printpage.SetItemCache(settings->GetItemCache());
	m_printpage.SetVisitedCache(settings->GetVisitedCache());
	m_printpage.SetAuthHandler(&m_ah);

	m_settings=settings;
	m_tabrecords.Init(4,1);
	m_iconvalid=false;

	SetNumGroups(1);
	SetZone(0,0,w,h);
	m_backimage.SetFilename("_back.gif");
	m_forwardimage.SetFilename("_forward.gif");
	m_reloadimage.SetFilename("_reload.gif");
	m_reloadimage2.SetFilename("_reload2.gif");
	m_lockedimage.SetFilename("_locked.gif");
	m_unlockedimage.SetFilename("_unlocked.gif");

	m_mainmenu.SetFontSize(20);
	m_mainmenu.SetFontID(1);
	m_mainmenu.SetNumEntries(2);
	m_mainmenu.GetTitle(0)->SetString("Menu");
	m_mainmenu.GetTitle(1)->SetString("Bookmarks");
	m_mainmenu.SetEntry(0,&m_menu);
	m_mainmenu.SetEntry(1,&m_bookmarksmenu);
	m_mainmenu.SetEventHandler(this,CALLBACKNAME(DoMainMenu));

	m_browsecontrols.AddObject(&m_mainmenu);

	m_menu.SetFontSize(14);
	m_menu.SetIconWidth(22);
	m_menu.Init(MAINMENU_NUM,mainmenutxt);

	m_bookmarksmenu.SetFontSize(16);

	m_gomenu.SetFontSize(14);
	m_gomenu.SetEventHandler(this,CALLBACKNAME(DoGotoMenu));

	m_backcaption.SetPos(0,0);
	m_backcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_backcaption.SetFontID(SMALLCAPTIONFONT);
	m_backcaption.SetString("Back");

	m_back.SetPos(0,15);
	m_back.SetHint("Go back to previous page.");	//todo add to translated text
	m_back.SetImage(&m_backimage);
	m_back.SetSize(m_backimage.GetImageWidth()+6,m_backimage.GetImageHeight()+6);
	m_back.SetEventHandler(this,CALLBACKNAME(GoBack));
	m_browsecontrols.AddObjects(2,&m_backcaption,&m_back);

	m_forwardcaption.SetPos(0,0);
	m_forwardcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_forwardcaption.SetFontID(SMALLCAPTIONFONT);
	m_forwardcaption.SetString("Forward");

	m_forward.SetPos(0,15);
	m_forward.SetHint("Go forward to next page.");	//todo add to translated text
	m_forward.SetImage(&m_forwardimage);
	m_forward.SetSize(m_forwardimage.GetImageWidth()+6,m_forwardimage.GetImageHeight()+6);
	m_forward.SetEventHandler(this,CALLBACKNAME(GoForward));
//	m_forward.SetRightClicked(this,CALLBACKNAME(GoForwardMenu));
	m_browsecontrols.AddObjects(2,&m_forwardcaption,&m_forward);

	m_refreshcaption.SetPos(0,0);
	m_refreshcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_refreshcaption.SetFontID(SMALLCAPTIONFONT);
	m_refreshcaption.SetString("Reload");

	m_refresh.SetImage(&m_reloadimage);
	m_refresh.SetSize(m_reloadimage.GetImageWidth()+6,m_reloadimage.GetImageHeight()+6);
	m_refresh.SetPos(0,15);
	m_refresh.SetEventHandler(this,CALLBACKNAME(Refresh));
	m_browsecontrols.AddObjects(2,&m_refreshcaption,&m_refresh);

	m_refresh2caption.SetPos(0,0);
	m_refresh2caption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_refresh2caption.SetFontID(SMALLCAPTIONFONT);
	m_refresh2caption.SetString("Reload All");

	m_refresh2.SetImage(&m_reloadimage2);
	m_refresh2.SetSize(m_reloadimage2.GetImageWidth()+6,m_reloadimage2.GetImageHeight()+6);
	m_refresh2.SetPos(0,15);
	m_refresh2.SetEventHandler(this,CALLBACKNAME(RefreshAll));
	m_browsecontrols.AddObjects(2,&m_refresh2caption,&m_refresh2);

	m_printcaption.SetPos(0,0);
	m_printcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_printcaption.SetFontID(SMALLCAPTIONFONT);
	m_printcaption.SetString("Print");

	m_print.SetPos(0,15);
	m_print.SetSize(65,22);
	m_print.SetHint("Print");
	m_print.SetString("Print");
	m_print.SetEventHandler(this,CALLBACKNAME(Print));
	m_browsecontrols.AddObjects(2,&m_printcaption,&m_print);

	/* the lock showing unsecure / secure mode */
	m_lock.SetPos(0,15);
	m_lock.SetImage(&m_unlockedimage);
	m_lock.SetSize(m_unlockedimage.GetImageWidth(),m_unlockedimage.GetImageHeight());
	//m_lock.SetHint("Print");
	m_browsecontrols.AddObjects(1,&m_lock);

	m_urlcaption.SetPos(0,0);
	m_urlcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_urlcaption.SetFontID(SMALLCAPTIONFONT);
	m_urlcaption.SetString("URL");

	s.SetString("http://");
	m_url.SetString(&s);
	m_url.SetPos(0,15);
	m_url.SetSize(750,22);
	m_url.SetEventHandler(this,CALLBACKNAME(UrlChanged));
	m_browsecontrols.AddObjects(2,&m_urlcaption,&m_url);

	m_searchcaption.SetPos(0,0);
	m_searchcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_searchcaption.SetFontID(SMALLCAPTIONFONT);
	m_searchcaption.SetString("SEARCH");

	m_search.SetPos(0,15);
	m_search.SetSize(150,22);
	m_search.SetEventHandler(this,CALLBACKNAME(SearchChanged));
	m_browsecontrols.AddObjects(2,&m_searchcaption,&m_search);
	m_browsecontrols.NextLine();

//	m_referercaption.SetPos(0,0);
//	m_referercaption.SetFontSize(SMALLCAPTIONFONTSIZE);
//	m_referercaption.SetFontID(SMALLCAPTIONFONT);
//	m_referercaption.SetString("Referrer");

//	m_referer.SetPos(0,15);
//	m_referer.SetSize(800,20);

	m_busyimage.SetPos(0,0);
	m_busyimage.SetFilename("_busy.gif");
	m_busyimage.SetSize(m_busyimage.GetImageWidth(),m_busyimage.GetImageHeight());
	m_browsecontrols.AddObjects(1,&m_busyimage);

	m_statuscaption.SetPos(0,0);
	m_statuscaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_statuscaption.SetFontID(SMALLCAPTIONFONT);
	m_statuscaption.SetString("Status");

	m_status.SetPos(0,15);
	m_status.SetSize(200,20);
	m_browsecontrols.AddObjects(2,&m_statuscaption,&m_status);

	m_linkcaption.SetPos(0,0);
	m_linkcaption.SetFontSize(SMALLCAPTIONFONTSIZE);
	m_linkcaption.SetFontID(SMALLCAPTIONFONT);
	m_linkcaption.SetString("Link");

	m_linkurl.SetPos(0,15);
	m_linkurl.SetSize(m_browsecontrols.GetZoneW()-m_browsecontrols.GetCurrentX(),20);
	m_browsecontrols.AddObjects(2,&m_linkcaption,&m_linkurl);

	AddObject(&m_browsecontrols);

	m_tabs.SetPos(0,m_browsecontrols.GetZoneH());
	h-=m_browsecontrols.GetZoneH();
	m_tabs.SetSize(w,h);

	m_curtab=0;
	m_numtabs=1;
	m_tabs.SetAllowClose(true);
	m_tabs.SetNumTabs(1);
	m_tabs.SetTabName(0,"(untitled)");
	m_tabs.SetEventHandler(this,CALLBACKNAME(TabChanged));
	AddObject(&m_tabs);

	/* apply settings to the default page */
	InitTabRecord(m_tabrecords.GetEntryPtr(0));
	m_curscreenpage=m_tabrecords.GetEntryPtr(0)->GetScreenPage();
	m_tabs.AddObject(m_curscreenpage);

	UpdateButtons();
	kGUI::AddUpdateTask(this,CALLBACKNAME(Update));
}

void kGUIBrowseObj::CopyTabURL(unsigned int n,kGUIString *url)
{
	HistoryRecord *p=m_tabrecords.GetEntryPtr(n)->GetCurHist();

	if(p)
		url->SetString(p->GetURL());
	else
		url->Clear();
}

void kGUIBrowseObj::InitTabRecord(TabRecord *tr)
{
	HTMLPageObj *hp;

	hp=tr->GetScreenPage();
	hp->SetSettings(m_settings);
	hp->SetItemCache(m_settings->GetItemCache());
	hp->SetVisitedCache(m_settings->GetVisitedCache());
	hp->SetAuthHandler(&m_ah);
	hp->SetPlugins(&m_plugins);

	hp->SetDrawLinkUnder(&m_linkurl);			
	hp->SetPos(0,0);
	hp->SetSize(m_tabs.GetChildZoneW(),m_tabs.GetChildZoneH()-m_tabs.GetTabRowHeight());

	hp->SetClickCallback(this,CALLBACKNAME(Click));
	hp->SetIconCallback(this,CALLBACKNAME(SetIcon));
}

void kGUIBrowseObj::RePosition(bool rp)
{
	unsigned int i;
	TabRecord *tabrecord;
	HTMLPageObj *screenpage;

	/* resize the tab object then resize all the children */
	m_tabs.SetSize(GetChildZoneW(),GetChildZoneH()-m_tabs.GetZoneY());

	for(i=0;i<m_numtabs;++i)
	{
		tabrecord=m_tabrecords.GetEntryPtr(i);
		screenpage=&tabrecord->m_screenpage;

		screenpage->SetSize(m_tabs.GetChildZoneW(),m_tabs.GetChildZoneH()-m_tabs.GetTabRowHeight());
		/* resize the current page now, other pages will be resized when TabChanged is trigerred */
		if(i==m_curtab)
			screenpage->RePosition(rp);
		else
		{
			tabrecord->m_reposition=true;
			if(rp)	/* don't clear if already set */
				tabrecord->m_reparse=rp;
		}
	}
}

void kGUIBrowseObj::SetIcon(kGUIHTMLPageObj *page,DataHandle *dh)
{
	unsigned int t;
	TabRecord *tr;

	/* find the tab since it might not be the current one anymore */
	for(t=0;t<m_numtabs;++t)
	{
		tr=m_tabrecords.GetEntryPtr(t);
		if(&tr->m_screenpage==page)
		{
			/* got it! */
			tr->SetIcon(true,dh);
			m_tabs.SetIcon(t,dh);

			/* set the icon on the URL input box */
			if(t==m_curtab)
			{
				m_icon.Copy(dh);
				m_iconvalid=m_url.SetIcon(dh);

				if(m_iconvalid==false)
				{
					//make it an empty datahandle if image is not valid
					m_icon.OpenWrite("wb");
					m_icon.Close();
				}
			}
			return;
		}
	}
	assert(false,"Should Never Get Here!");
}

void kGUIBrowseObj::NewTab(void)
{
	unsigned int i;
	int curtab=m_tabs.GetCurrentTab();
	const char *sdir;

	/* save prev URL string, set new URL string */
	m_tabrecords.GetEntryPtr(curtab)->m_urlstring.SetString(&m_url);
	m_url.Clear();
	/* save prev icon, clear new ICON */
	m_tabrecords.GetEntryPtr(curtab)->SetIcon(m_iconvalid,&m_icon);
	m_iconvalid=false;
	m_url.SetIcon(0);	/* hide old URL icon until new one can be loaded */

	/* add new tab */
	++m_numtabs;
	m_tabs.SetNumTabs(m_numtabs);
	m_tabs.SetTabName(m_numtabs-1,"(Untitled)");

	sdir=GetSaveDirectory();
	m_curscreenpage=m_tabrecords.GetEntryPtr(m_numtabs-1)->GetScreenPage();

	//set default save directory to the one from the previous current tab
	m_curscreenpage->SetSaveDirectory(sdir);
	InitTabRecord(m_tabrecords.GetEntryPtr(m_numtabs-1));

	/* when we change the number of tabs, the tab code re-inits the child object in each tab */
	/* so we need to re-attach all the previous tab pages */
	for(i=0;i<m_numtabs;++i)
	{
		m_tabs.SetCurrentTab(i);
		m_tabs.AddObject(m_tabrecords.GetEntryPtr(i)->GetScreenPage());
	}
	m_tabs.SetCurrentTab(m_numtabs-1);
	m_curtab=m_numtabs-1;
//	m_tabs.AddObject(m_curscreenpage);

	/* update the forward/backward etc. buttons */
	UpdateButtons();
}

void kGUIBrowseObj::CloseTab(void)
{
	unsigned int i;
	TabRecord *tr;
	HistoryRecord *hr;

	/* if this page already has a pending load then flag is as to be ignored */
	StopLoad();

	/* delete this tabrecord entry */
	tr=m_tabrecords.GetEntryPtr(m_curtab);
	m_tabrecords.DeleteEntry(m_curtab,true);
	--m_numtabs;

	/* if no tabs then open a single untitled one */
	if(!m_numtabs)
	{
		m_numtabs=1;
		m_tabs.SetNumTabs(m_numtabs);
		m_tabs.SetTabName(m_numtabs-1,"(Untitled)");
		m_curtab=0;
		InitTabRecord(m_tabrecords.GetEntryPtr(0));
		m_curscreenpage=m_tabrecords.GetEntryPtr(0)->GetScreenPage();
		m_tabs.AddObject(m_curscreenpage);
	}
	else
	{
		if(m_curtab==m_numtabs)
			--m_curtab;

		/* when we change the number of tabs, the tab code re-inits the child object in each tab */
		/* so we need to re-attach all the previous tab pages */
		m_tabs.SetNumTabs(m_numtabs);
		for(i=0;i<m_numtabs;++i)
		{
			tr=m_tabrecords.GetEntryPtr(i);
			m_tabs.SetCurrentTab(i);
			hr=tr->GetCurHist();
			if(hr)
				m_tabs.SetTabName(i,hr->GetTitle());
			else
				m_tabs.SetTabName(i,"(untitled)");
			m_tabs.SetIcon(i,&tr->m_icon);
			m_tabs.AddObject(m_tabrecords.GetEntryPtr(i)->GetScreenPage());
		}
		m_tabs.SetCurrentTab(m_curtab);

		tr=m_tabrecords.GetEntryPtr(m_curtab);
		m_url.SetString(&tr->m_urlstring);
		m_curscreenpage=&tr->m_screenpage;
	}

	/* update the forward/backward etc. buttons */
	UpdateButtons();
}

void kGUIBrowseObj::DoMainMenu(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ENTER:
	{
		/* enable the necessary menus and populate the bookmarks menu */
		unsigned int i;
		unsigned int numbookmarks;
		kGUIImageObj *icon;
		HistoryRecord *hist=GetCurHist();

		m_menu.SetEntryEnable(MAINMENU_SAVEPAGE,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWPAGESOURCE,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWCORRECTEDPAGESOURCE,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWPOSTDATA,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWHEADER,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWSCRIPTSOURCE,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWCSS,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWCORRECTEDCSS,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWERRORS,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_VIEWMEDIA,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_TRACELAYOUT,hist!=0);
		m_menu.SetEntryEnable(MAINMENU_TRACEDRAW,hist!=0);

		if(m_settings)
		{
			numbookmarks=m_settings->GetNumBookmarks();
			
			m_bookmarksmenu.SetIconWidth(22);
			m_bookmarksmenu.Init(numbookmarks+3);
			/* todo add to translated text file */
			m_bookmarksmenu.SetEntry(0,"Bookmark Page",BOOKMARK_ADDBOOKMARK);
			m_bookmarksmenu.SetEntry(1,"Edit Bookmarks",BOOKMARK_EDITBOOKMARK);
			m_bookmarksmenu.GetEntry(2)->SetIsBar(true);

			for(i=0;i<numbookmarks;++i)
			{
				m_bookmarksmenu.SetEntry(i+3,m_settings->GetBookmark(i)->GetTitle(),BOOKMARK_BOOKMARKS+i);
				icon=m_bookmarksmenu.GetEntry(i+3)->GetIconObj();
				icon->Copy(m_settings->GetBookmark(i)->GetIcon());
				icon->SetSize(16,16);
				icon->ScaleTo(16,16);
			}
		}
	}
	break;
	case EVENT_SELECTED:
		switch(event->m_value[0].i)
		{
		case MAINMENU_NEWTAB:
			NewTab();
		break;
		case MAINMENU_SAVEPAGE:
		{
			HistoryRecord *hist=GetCurHist();
			kGUIFileReq *req;
			kGUIString base;
			kGUIString root;
			kGUIString name;
			kGUIString savedir;
			char *place;

			assert(hist!=0,"Error, Menu should not have allowed this!");

			/* isolate the name from the end of the URL */	
			name.SetString(hist->GetURL());
			place=strstr(name.GetString(),"?");
			if(place)
			{
				/* trim filename at the '?' */
				name.Clip((unsigned int)(place-name.GetString()));
			}

			kGUI::ExtractURL(&name,&base,&root);
			if(root.GetLen())
			{
				if(root.GetLen()<name.GetLen())
					name.Delete(0,root.GetLen());
				else
					name.SetString("page.html");
			}

			savedir.SetString(GetSaveDirectory());
			kGUI::MakeFilename(&savedir,&name,&m_savefn);

			req=new kGUIFileReq(FILEREQ_SAVE,m_savefn.GetString(),0,this,CALLBACKNAME(SaveAs));
		}
		break;
		case MAINMENU_VIEWPAGESOURCE:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Page Source '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,hist->GetSource());
		}
		break;
		case MAINMENU_VIEWCORRECTEDPAGESOURCE:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString correctedsource;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");
			title.Sprintf("Corrected Page Source '%s'",hist->GetTitle()->GetString());

			m_curscreenpage->GetCorrectedSource(&correctedsource);
			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,&correctedsource);
		}
		break;
		case MAINMENU_VIEWPOSTDATA:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");
			title.Sprintf("Post Data '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,hist->GetPost());
		}
		break;
		case MAINMENU_VIEWHEADER:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");
			title.Sprintf("Header '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,hist->GetHeader());
		}
		break;
		case MAINMENU_VIEWSCRIPTSOURCE:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Page Scripts '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,m_curscreenpage->GetScripts());
		}
		break;
		case MAINMENU_VIEWCSS:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString source;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Page CSS '%s'",hist->GetTitle()->GetString());

			m_curscreenpage->GetCSS(&source);
			source.Replace("}","}\n");

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,&source);
		}
		break;
		case MAINMENU_VIEWCORRECTEDCSS:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString source;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Corrected Page CSS '%s'",hist->GetTitle()->GetString());

			m_curscreenpage->GetCorrectedCSS(&source);
			source.Replace("}","}\n");

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,&source);
		}
		break;
		case MAINMENU_VIEWERRORS:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Page Errors '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,m_curscreenpage->GetErrors());
		}
		break;
		case MAINMENU_VIEWMEDIA:
		{
			ViewSource *vs;
			kGUIString title;
			HistoryRecord *hist=GetCurHist();

			assert(hist!=0,"Error, Menu should not have allowed this!");

			title.Sprintf("Page Media '%s'",hist->GetTitle()->GetString());

			vs=new ViewSource(	(int)(GetZoneW()*0.75),
								(int)(GetZoneH()*0.75),
								&title,m_curscreenpage->GetMedia());
		}
		break;
		case MAINMENU_VIEWCOOKIES:
		{
			ViewCookieWindow *vcw;

			if(m_settings)
				vcw=new ViewCookieWindow(m_settings);
		}
		break;
		case MAINMENU_TRACELAYOUT:
			m_curscreenpage->TraceLayout();
		break;
		case MAINMENU_TRACEDRAW:
			m_curscreenpage->TraceDraw();
		break;
		case MAINMENU_SETTINGS:
		{
			ViewSettings *vs;

			/* view / edit settings for browser */
			vs=new ViewSettings(this,
					(int)(GetZoneW()*0.75),
					(int)(GetZoneH()*0.75));
		}
		break;
		case BOOKMARK_ADDBOOKMARK:
		{
			HistoryRecord *p=m_tabrecords.GetEntryPtr(m_curtab)->GetCurHist();
			
			if(p->GetURL()->GetLen())
			{
				m_settings->AddBookmark(p->GetTitle(),p->GetURL(),&m_icon);
				SettingsChanged();
			}
		}
		break;
		case BOOKMARK_EDITBOOKMARK:
		{
			EditBookmarkWindow *ebw;

			ebw=new EditBookmarkWindow(m_settings);
		}
		break;
		case -1:	/* menu aborted */
		break;
		default:
		{
			/* goto bookmark */
			kGUIBookmark *bookmark;
			HistoryRecord *hist;

			SaveCurrent();

			bookmark=m_settings->GetBookmark(event->m_value[0].i-BOOKMARK_BOOKMARKS);

			hist=NextPage();
			m_url.SetString(bookmark->GetURL());
			hist->Set(bookmark->GetURL()->GetString(),0,0,0,0);
			hist->SetScrollY(0);

			Load();
		}
		break;
		}
	break;
	}
}

void kGUIBrowseObj::SaveAs(kGUIFileReq *req,int closebutton)
{
	if(closebutton==MSGBOX_OK)
	{
		m_savefn.SetString(req->GetFilename());
		if(kGUI::FileExists(m_savefn.GetString())==true)
		{
			/* replace, are you sure? */
			kGUIMsgBoxReq *box;
			box=new kGUIMsgBoxReq(MSGBOX_YES|MSGBOX_NO,this,CALLBACKNAME(AskOverwrite),true,"File '%s' Exists, Overwrite?!",m_savefn.GetString());
		}
		else
			DoSaveAs();
	}
}

void kGUIBrowseObj::AskOverwrite(int closebutton)
{
	if(closebutton==MSGBOX_YES)
		DoSaveAs();
}

void kGUIBrowseObj::DoSaveAs(void)
{
	DataHandle dh;
	kGUIString correctedsource;

	m_curscreenpage->GetCorrectedSourceEmbed(&correctedsource);

	dh.SetFilename(m_savefn.GetString());
	dh.OpenWrite("wb");
	dh.Write(correctedsource.GetString(),correctedsource.GetLen());
	dh.Close();
}

kGUIBrowseObj::~kGUIBrowseObj()
{
	unsigned int i;
	DownloadPageRecord *dle;

	/* set abort flag for all remaining downloads */
	for(i=0;i<m_numdlactive;++i)
	{
		dle=m_dlactive.GetEntry(i);
		dle->m_dl.Abort();
	}

	/* wait for all remaining downloads to abort */
	while(m_numdlactive)
	{
		i=0;
		while(i<m_numdlactive)
		{
			dle=m_dlactive.GetEntry(i);

			/* is this still active ? */
			if(dle->m_dl.GetAsyncActive()==false)
			{
				delete dle;
				m_dlactive.DeleteEntry(i,1);
				--m_numdlactive;
			}
			else
				++i;
		}
	}

	kGUI::DelUpdateTask(this,CALLBACKNAME(Update));
}

/* return pointer to next page buffer, if full, then scroll buffers back */

HistoryRecord *kGUIBrowseObj::NextPage(void)
{
	HistoryRecord *pi;

	pi=m_tabrecords.GetEntryPtr(m_curtab)->NextPage();
	UpdateButtons();
	return(pi);
}

void kGUIBrowseObj::SetSource(kGUIString *url,kGUIString *source,kGUIString *type,kGUIString *header)
{
	HistoryRecord *hist=NextPage();

	hist->SetScrollY(0);

	if(source)
	{
		hist->Set(url->GetString(),0,0,source->GetString(),header?header->GetString():0);
		Goto(m_curtab);
	}
	else
	{
		hist->Set(url->GetString(),0,0,0,0);
		hist->SetScrollY(0);
		hist->SetType(type);
		hist->SetHeader(header);
		hist->SetReferer(url);

		m_url.SetString(url);

		Load();
	}
}

void kGUIBrowseObj::UpdateButtons(void)
{
	TabRecord *tr=m_tabrecords.GetEntryPtr(m_curtab);
	HistoryRecord *hist=GetCurHist();

	m_back.SetEnabled(tr->m_histindex>1);
	m_refresh.SetEnabled(tr->m_histindex>0);
	m_refresh2.SetEnabled(tr->m_histindex>0);
	m_print.SetEnabled(tr->m_histindex>0);
	m_forward.SetEnabled(tr->m_histindex<tr->m_histend);
	
	if(hist==0)
		m_lock.SetImage(&m_unlockedimage);
	else if(hist->GetSecure()==true)
		m_lock.SetImage(&m_lockedimage);
	else
		m_lock.SetImage(&m_unlockedimage);

	/* update the busy anim icon based on the current tabs download status */
	m_busyimage.SetAnimate(tr->m_curdl!=0);
	if(tr->m_curdl==0)
	{
		m_status.SetString("Done");
		m_curscreenpage->SetStatusLine(&m_status);	/* allow writing again... */
	}
	else
	{
		m_curscreenpage->SetStatusLine(0);	/* stop page from overwriting */
		m_status.SetString("Loading");
	}
	m_pagechangedcallback.Call();
}

void kGUIBrowseObj::UpdateButtons2(void)
{
	TabRecord *tr=m_tabrecords.GetEntryPtr(m_curtab);

	if(tr->m_curdl)
	{
		unsigned int kb;
		kGUIString cnum;

		kb=tr->m_curdl->m_dl.GetReadBytes()/1024;
		if(kb)
		{
			cnum.SetFormattedInt(kb);
			m_status.Sprintf("Loading %sK",cnum.GetString());
		}
	}
}


void kGUIBrowseObj::Goto(unsigned int tabnum)
{
	HistoryRecord *hist=m_tabrecords.GetEntryPtr(tabnum)->GetCurHist();
	HTMLPageObj *page=m_tabrecords.GetEntryPtr(tabnum)->GetScreenPage();
	const char *cp;
	kGUIString llname;

	//todo remove most of this stuff
	if(tabnum==m_curtab)
		m_url.SetString(hist->GetURL());

	kGUI::SetMouseCursor(MOUSECURSOR_BUSY);

	/* is there a local link appended to the URL? */
	cp=strstr(m_url.GetString(),"#");
	if(cp)
		llname.SetString(cp+1);

	page->SetTarget(&llname);
	page->SetMedia(GetSettings()->GetScreenMedia());
	page->SetSource(hist->GetURL(),hist->GetSource(),hist->GetType(),hist->GetHeader());
	page->LoadInput(hist->GetInput());			/* overwrite any form inputs with previous input */
	m_debug.SetString(page->GetDebug());

	/* only if window currently has vertical scrollbars */
	/* is there a local link appended to the URL? */
	if(llname.GetLen() && page->GetHasVertScrollBars()==true)
	{
		kGUIObj *topobj;
		int currenty;

		topobj=page->LocateLocalLink(&llname);
		if(topobj)
		{
			kGUICorners c1;
			kGUICorners c2;

			/* scroll down to the top object */
			topobj->GetCorners(&c2);
			page->GetCorners(&c1);

			currenty=page->GetScrollY();
			page->SetScrollY(currenty+(c2.ty-c1.ty));
		}
	}
	else
		page->SetScrollY(hist->GetScrollY());

	if(tabnum==m_curtab)
		page->Dirty();

	/* save title of page so if user right clicks on go forward or goback buttons then ir shows title instead of URL */
	if(page->GetTitle()->GetLen())
	{
		hist->SetTitle(page->GetTitle());

		/* set the tab title too */
		m_tabs.SetTabName(tabnum,hist->GetTitle());
	}
	/* tell window to change title */
	m_pagechangedcallback.Call();

	kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
}

void kGUIBrowseObj::SaveCurrent(void)
{
	HistoryRecord *hist;

	/* save old scroll position and save user input on the page */
	hist=m_tabrecords.GetEntryPtr(m_curtab)->GetCurHist();
	if(hist)
	{
		hist->SetScrollY(m_curscreenpage->GetScrollY());
		if(hist->GetInput())
			m_curscreenpage->SaveInput(hist->GetInput());
	}
}

void kGUIBrowseObj::Click(kGUIHTMLClickInfo *info)
{
	HistoryRecord *hist;

	SaveCurrent();
	if(info->m_newtab)
		NewTab();

	m_url.SetString(info->m_url);

	hist=NextPage();
	hist->SetURL(info->m_url);
	hist->SetPost(info->m_post);
	hist->SetReferer(info->m_referrer);
	hist->SetScrollY(0);
	hist->GetInput()->Reset();

	Load();
}

void kGUIBrowseObj::Update(void)
{
	unsigned int i;
	DownloadPageRecord *dle;

	/* scan the download pool for active items that are now done */

	i=0;
	while(i<m_numdlactive)
	{
		dle=m_dlactive.GetEntry(i);

		/* is this still active ? */
		if(dle->m_dl.GetAsyncActive()==true)
			++i;
		else
		{
			if(dle->m_tabrecord)
				PageLoaded(dle,dle->m_dl.GetStatus());
			else
				delete dle;

			m_dlactive.DeleteEntry(i,1);
			--m_numdlactive;
		}
	}
	UpdateButtons2();
}

void kGUIBrowseObj::StopLoad(void)
{
	TabRecord *tabrecord=m_tabrecords.GetEntryPtr(m_curtab);
	DownloadPageRecord *dle;

	/* if this page already has a pending load then flag is as to be ignored */
	dle=tabrecord->m_curdl;
	if(dle)
	{
		tabrecord->m_curdl=0;
		dle->m_tabrecord=0;
		dle->m_dl.Abort();
	}
}

void kGUIBrowseObj::Load(void)
{
	kGUIString url;
	const char *cp;
	DownloadPageRecord *dle;
	TabRecord *tabrecord=m_tabrecords.GetEntryPtr(m_curtab);
	HistoryRecord *hist=GetCurHist();

	m_iconvalid=false;
	m_url.SetIcon(0);	/* hide old URL icon until new one can be loaded */

	/* if this page already has a pending load then flag is as to be ignored */
	StopLoad();

	/* is there a partial page offet? ie: "page.html#aaaaa" */ 
	url.SetString(hist->GetURL());
	cp=strstr(url.GetString(),"#");
	if(cp)
		url.Clip((int)(cp-url.GetString()));

//	dle=m_dlpool.PoolGet();
	dle=new DownloadPageRecord();
	dle->m_tabnum=m_curtab;
	dle->m_tabrecord=m_tabrecords.GetEntryPtr(m_curtab);
	if(!strcmpin(url.GetString(),"file://",7))
	{
		int load;

		dle->m_dh.SetFilename(url.GetString()+7);
		dle->m_dl.SetRedirectURL(0);

		if(dle->m_dh.Open()==true)
		{
			load=DOWNLOAD_OK;
			dle->m_dh.Close();
		}
		else
			load=DOWNLOAD_ERROR;
		PageLoaded(dle,load);
	}
	else
	{
		m_curscreenpage->SetStatusLine(0);	/* stop page from overwriting */
		m_status.SetString("Loading");
		m_busyimage.SetAnimate(true);

		/* save in the active list */
		m_dlactive.SetEntry(m_numdlactive++,dle);

		/* todo: have browser setting for not sending this along */
		if(hist)
		{
			dle->m_dl.SetReferer(hist->GetReferer());
			dle->m_dl.SetPostData(hist->GetPost());
		}
		else
		{
			dle->m_dl.SetReferer((const char *)0);
			dle->m_dl.SetPostData(0);
		}
		dle->m_dh.SetMemory();
		dle->m_dl.SetAuthHandler(&m_ah);
		dle->m_dl.AsyncDownLoad(&dle->m_dh,&url);
		tabrecord->m_curdl=dle;
	}
}

/* show popup listing all previous pages we can go back to */

void kGUIBrowseObj::GoBackMenu(void)
{
	int i;
	int j;
	TabRecord *ctp=m_tabrecords.GetEntryPtr(m_curtab);
	HistoryRecord *p;

	if(ctp->m_histindex<2)
		return;	/* nowhere to go forward to */

	m_gomenu.SetNumEntries(ctp->m_histindex-1);
	j=ctp->m_histindex-2;
	for(i=1;i<ctp->m_histindex;++i)
	{
		p=ctp->GetHist(j);
		m_gomenu.SetEntry(i-1,p->GetTitle(),j);
		--j;
	}
	m_gomenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

/* show popup listing all pages we can go forward to */

void kGUIBrowseObj::GoForwardMenu(void)
{
	int i;
	unsigned int j;
	unsigned int num;
	HistoryRecord *p;
	TabRecord *ctp=m_tabrecords.GetEntryPtr(m_curtab);

	num=ctp->m_histend-ctp->m_histindex;
	if(!num)
		return;	/* nowhere to go forward to */

	m_gomenu.SetNumEntries(num);
	j=0;
	for(i=ctp->m_histindex;i<ctp->m_histend;++i)
	{
		p=ctp->GetHist(i);
		m_gomenu.SetEntry(j++,p->GetTitle(),i);
	}
	m_gomenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

void kGUIBrowseObj::DoGotoMenu(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		int sel;

		sel=m_gomenu.GetSelection();

		if(sel>=0)
		{
		/* todo: cancel any pending loads */
			StopLoad();
			m_tabrecords.GetEntryPtr(m_curtab)->m_histindex=sel+1;
			UpdateButtons();
			Goto(m_curtab);
		}
	}
}

void kGUIBrowseObj::GoForward(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_RIGHTCLICK:
		GoForwardMenu();
	break;
	case EVENT_PRESSED:
	{
		TabRecord *tabrecord=m_tabrecords.GetEntryPtr(m_curtab);
		SaveCurrent();

		/* todo: cancel any pending loads */

		if(tabrecord->m_histindex<tabrecord->m_histend)
		{
			StopLoad();
			++tabrecord->m_histindex;
			UpdateButtons();
			Goto(m_curtab);
		}
	}
	break;
	}
}

void kGUIBrowseObj::GoBack(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_RIGHTCLICK:
		GoBackMenu();
	break;
	case EVENT_PRESSED:
	{
		TabRecord *ctp=m_tabrecords.GetEntryPtr(m_curtab);
		SaveCurrent();

		if(ctp->m_histindex>1)
		{
			StopLoad();
			--ctp->m_histindex;
			UpdateButtons();
			Goto(m_curtab);
		}
	}
	break;
	}
}

void kGUIBrowseObj::Print(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		kGUIString llname;
		const char *cp;
		HistoryRecord *hist=GetCurHist();

		assert(hist!=0,"Error!");
		/* is there a local link appended to the URL? */
		cp=strstr(m_url.GetString(),"#");
		if(cp)
			llname.SetString(cp+1);

		m_printpage.SetTarget(&llname);
		m_printpage.SetMedia(GetSettings()->GetPrintMedia());
		m_printpage.SetSource(hist->GetURL(),hist->GetSource(),hist->GetType(),hist->GetHeader());

		/* todo: copy current input values on forms from screenpage to printpage */

		m_printpage.Preview();
	}
}

void kGUIBrowseObj::UrlChanged(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ENTER:
		if(m_url.GetLen())
			m_url.SelectAll();
	break;
	case EVENT_PRESSRETURN:
	{
		HistoryRecord *oldhist=GetCurHist();
		HistoryRecord *newhist;
		const char *referer;

		SaveCurrent();
		newhist=NextPage();

		/* if URL does not have http prefix then insert it if not a local file on hd */
		if(!strcmpin(m_url.GetString(),"file://",5))
		{
			/* ok, it's a local file not an online webpage */
		}
		else if(strcmpin(m_url.GetString(),"http",4))
		{
			if(kGUI::FileExists(m_url.GetString()))
				m_url.Insert(0,"file://");
			else
				m_url.Insert(0,"http://");
		}
		if(oldhist)
			referer=oldhist->GetReferer()->GetString();
		else
			referer=0;
		newhist->Set(m_url.GetString(),0,referer,0,0);
		newhist->SetScrollY(0);

		Load();
	}
	break;
	}
}

void kGUIBrowseObj::SearchChanged(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ENTER:
		if(m_search.GetLen())
			m_search.SelectAll();
	break;
	case EVENT_PRESSRETURN:
	{
		HistoryRecord *newhist;
		kGUIString searchurl;
		kGUIString encsearch;

		SaveCurrent();
		newhist=NextPage();

		kGUIHTMLFormObj::Encode(&m_search,&encsearch);
		searchurl.Sprintf("http://www.google.ca/search?hl=en&q=%s&btnG=Google+Search&meta=",encsearch.GetString());

		newhist->Set(searchurl.GetString(),0,0,0,0);
		newhist->SetScrollY(0);

		Load();
	}
	break;
	}
}


void kGUIBrowseObj::TabChanged(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_MOVED:
	{
		int oldtab=event->m_value[0].i;
		int newtab=m_tabs.GetCurrentTab();
		TabRecord *oldtabrecord;
		TabRecord *newtabrecord;

		oldtabrecord=m_tabrecords.GetEntryPtr(oldtab);
		newtabrecord=m_tabrecords.GetEntryPtr(newtab);

		/* this is not the actual page URL but the currently edited one */
		oldtabrecord->m_urlstring.SetString(&m_url);
		m_url.SetString(&newtabrecord->m_urlstring);

		//this is to be depreciated!
		m_curscreenpage=&newtabrecord->m_screenpage;
		m_curtab=newtab;

		/* if the size doesn't match then set it and force a layout */
		if(newtabrecord->m_reposition)
		{
			m_curscreenpage->RePosition(newtabrecord->m_reparse);
			newtabrecord->m_reposition=false;
			newtabrecord->m_reparse=false;
		}
		/* update the forward/backward etc. buttons */
		UpdateButtons();
	}
	break;
	case EVENT_CLOSE:
		/* close the current tab */
		CloseTab();
	break;
	}
}

void kGUIBrowseObj::PageLoaded(DownloadPageRecord *dle,int result)
{
	TabRecord *tabrecord=dle->m_tabrecord;
	HistoryRecord *hist=tabrecord->GetCurHist();

	tabrecord->m_curdl=0;	/* load finished! */
	if(tabrecord==m_tabrecords.GetEntryPtr(m_curtab))
	{
		m_busyimage.SetAnimate(false);
		m_status.SetString("Done");
		m_curscreenpage->SetStatusLine(&m_status);	/* allow writing again... */
	}

	if(result==DOWNLOAD_OK && dle->m_dh.GetSize())
	{
		dle->m_dh.Open();
		dle->m_dh.Read(hist->GetSource(),dle->m_dh.GetSize());
		dle->m_dh.Close();

		hist->SetType(dle->m_dl.GetEncoding());
		hist->SetHeader(dle->m_dl.GetHeader());
		hist->SetSecure(dle->m_dl.GetSecure());

		/* if this was redirected then get the new redirected URL */
		if(dle->m_dl.GetRedirectURL()->GetLen())
		{
			hist->SetURL(dle->m_dl.GetRedirectURL());
			m_url.SetString(dle->m_dl.GetRedirectURL());
		}
		Goto(dle->m_tabnum);
	}
	else
	{
		if(dle->m_dl.GetReturnCode()==401)
		{
			/* bring up authentication input box */
			AuthenticateWindow *aw;
			kGUIString aa;
			kGUIString domain;
			kGUIString path;
			kGUIString realm;
			const char *cp;

			//WWW-Authenticate: Basic realm="File Download Authorization"
			dle->m_dl.ExtractFromHeader("WWW-Authenticate:",&aa);
			cp=strstri(aa.GetString(),"realm=");
			if(cp)
			{
				realm.SetString(cp+6);
				realm.Trim(TRIM_SPACE|TRIM_TAB|TRIM_CR|TRIM_QUOTES);

				kGUI::ExtractURL(&m_url,&domain,&path);
				domain.Replace("http://","");
				aw=new AuthenticateWindow(this,dle,&realm,&domain);
				return;
			}
		}
		else
		{
			ShowError(dle);
		}
	}
	UpdateButtons();
	delete dle;
}

void kGUIBrowseObj::ShowError(DownloadPageRecord *dle)
{
	HistoryRecord *hist=dle->m_tabrecord->GetCurHist();

	/* did we get an error string from the server? */
	if(dle->m_dl.GetErrorPage()->GetLen())
	{
		hist->SetSource(dle->m_dl.GetErrorPage());
		hist->SetType(dle->m_dl.GetEncoding());
		hist->SetHeader(dle->m_dl.GetHeader());
	}
	else
	{
		/* put up an error page */
		hist->GetSource()->Sprintf("<HTML><HEAD><TITLE>Error %d loading page '%s'</TITLE></HEAD><BODY><H1>Error %d loading page '%s'</H1><BR></BODY></HTML>",dle->m_dl.GetReturnCode(),dle->m_dl.GetURL()->GetString(),dle->m_dl.GetReturnCode(),dle->m_dl.GetURL()->GetString());
		hist->GetType()->SetString("text/html");
		hist->GetHeader()->Clear();
	}
	//todo: hmmm, if this is not the current tab then defer layout till later
	Goto(dle->m_tabnum);
}

void kGUIBrowseObj::Authenticate(kGUIString *domrealm,kGUIString *name,kGUIString *password)
{
	unsigned int i;
	unsigned int c;
	unsigned int bits;
	kGUIString np;
	kGUIString enc;
	kGUIBitStream bs;
	static unsigned char b64[]={"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

	np.SetString(name);
	np.Append(":");
	np.Append(password);
	
	/* generate base64 string from name and password */
	bs.SetReverseOut();
	bs.SetReverseIn();
	bs.Set(np.GetString());
	bits=np.GetLen()*8;
	for(i=0;i<bits;i+=6)
	{
		c=bs.ReadU(6);
		enc.Append(b64[c]);
	}

	/* add this to the authentication handler */
	m_ah.Add(domrealm,&enc);

	Load();
}

/* Reload Page */
void kGUIBrowseObj::Refresh(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		Load();
}

/* Reload page and all art and linked files */
void kGUIBrowseObj::RefreshAll(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_curscreenpage->FlushCurrentMedia();
		Load();
	}
}

/*******************************************/
/* temp window for viewing page source etc */
/*******************************************/

ViewSource::ViewSource(int w,int h,kGUIString *title,kGUIString *source)
{
	m_source.SetFontSize(13);
	AddObject(&m_source);
	
	SetSize(w,h);
	Center();

	kGUI::AddWindow(this);
	SetTitle(title);
	m_source.SetString(source);
	SetEventHandler(this,CALLBACKNAME(WindowEvent));
}

void ViewSource::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

/* called when the window size changes */	
void ViewSource::DirtyandCalcChildZone(void)
{
	kGUIContainerObj::DirtyandCalcChildZone();
	m_source.SetZone(0,0,GetChildZoneW(),GetChildZoneH());
}

/************************************************************************************/

#define VSFONTSIZE 20

static const char *medialist[]={
	"screen",
	"aural",
	"braille",
	"embossed",
	"handheld",
	"print",
	"projection",
	"tty",
	"tv"};

ViewSettings::ViewSettings(kGUIBrowseObj *b,int w,int h)
{
	unsigned int i;
	SetSize(w,h);
	Center();

	m_b=b;
	m_controls.SetPos(0,0);
	m_controls.SetZoneW(GetChildZoneW());
	m_controls.SetMaxHeight(10000);

	m_visiteddayscaption.SetFontID(1);
	m_visiteddayscaption.SetFontSize(VSFONTSIZE);
	m_visiteddayscaption.SetString("Save visited links for");

	m_visiteddays.SetFontSize(VSFONTSIZE);
	m_visiteddays.SetInputType(GUIINPUTTYPE_INT);
	m_visiteddays.SetSize(50,m_visiteddays.GetLineHeight()+6);

	m_visiteddayscaption2.SetFontID(1);
	m_visiteddayscaption2.SetFontSize(VSFONTSIZE);
	m_visiteddayscaption2.SetString("Days");

	m_controls.AddObject(&m_visiteddayscaption);
	m_controls.AddObject(&m_visiteddays);
	m_controls.AddObject(&m_visiteddayscaption2);
	m_controls.NextLine();

	/* caching mode to use */
	
	m_cachemode.SetNumEntries(3);
	m_cachemode.SetFontSize(VSFONTSIZE);
	/* todo, add to translated text file */
	m_cachemode.SetEntry(0,"Cache downloaded files",0);
	m_cachemode.SetEntry(1,"Cache downloaded files, delete at end of each session",1);
	m_cachemode.SetEntry(2,"Don't cache downloaded files",2);
	m_cachemode.SetSize(m_cachemode.GetWidest(),m_visiteddayscaption2.GetLineHeight()+6);

	m_controls.AddObject(&m_cachemode);
	m_controls.NextLine();

	m_cachesizecaption.SetFontID(1);
	m_cachesizecaption.SetFontSize(VSFONTSIZE);
	m_cachesizecaption.SetString("Use up to");

	m_cachesize.SetFontSize(VSFONTSIZE);
	m_cachesize.SetInputType(GUIINPUTTYPE_INT);
	m_cachesize.SetSize(75,m_cachesize.GetLineHeight()+6);

	m_cachesizecaption2.SetFontID(1);
	m_cachesizecaption2.SetFontSize(VSFONTSIZE);
	m_cachesizecaption2.SetString("MB for downloaded file cache");

	m_controls.AddObject(&m_cachesizecaption);
	m_controls.AddObject(&m_cachesize);
	m_controls.AddObject(&m_cachesizecaption2);
	m_controls.NextLine();

	m_savemodecaption.SetFontID(1);
	m_savemodecaption.SetFontSize(VSFONTSIZE);
	m_savemodecaption.SetString("Save Tabs when quitting:");

	m_savemode.SetNumEntries(3);
	m_savemode.SetFontSize(VSFONTSIZE);
	/* todo, add to translated text file */
	m_savemode.SetEntry(0,"No",SAVEMODE_NO);
	m_savemode.SetEntry(1,"Yes always",SAVEMODE_YES);
	m_savemode.SetEntry(2,"Yes only if more than one",SAVEMODE_YESMORE1);
	m_savemode.SetSize(m_savemode.GetWidest(),m_savemodecaption.GetLineHeight()+6);

	m_controls.AddObject(&m_savemodecaption);
	m_controls.AddObject(&m_savemode);
	m_controls.NextLine();


	m_loadimagescaption.SetFontID(1);
	m_loadimagescaption.SetFontSize(VSFONTSIZE);
	m_loadimagescaption.SetString("Load Images");
	m_controls.AddObject(&m_loadimagescaption);

	m_loadimages.SetSelected(true);
	m_controls.AddObject(&m_loadimages);
	m_controls.NextLine();

	m_usecsscaption.SetFontID(1);
	m_usecsscaption.SetFontSize(VSFONTSIZE);
	m_usecsscaption.SetString("Use CSS");
	m_controls.AddObject(&m_usecsscaption);

	m_usecss.SetSelected(true);
	m_controls.AddObject(&m_usecss);
	m_controls.NextLine();

	m_useusercsscaption.SetFontID(1);
	m_useusercsscaption.SetFontSize(VSFONTSIZE);
	m_useusercsscaption.SetString("Use User CSS");
	m_controls.AddObject(&m_useusercsscaption);

	m_useusercss.SetSelected(true);
	m_controls.AddObject(&m_useusercss);
	m_controls.NextLine();

	m_usercsscaption.SetFontID(1);
	m_usercsscaption.SetFontSize(VSFONTSIZE);
	m_usercsscaption.SetString("User CSS");
	m_controls.AddObject(&m_usercsscaption);
	m_controls.NextLine();

	m_usercss.SetAllowEnter(true);
	m_usercss.SetAllowTab(true);
	m_usercss.SetFontSize(VSFONTSIZE);
	m_usercss.SetSize(m_controls.GetChildZoneW()-(m_controls.GetBorderGap()<<1),150);
	m_controls.AddObject(&m_usercss);
	m_controls.NextLine();

	m_drawboxcaption.SetFontID(1);
	m_drawboxcaption.SetFontSize(VSFONTSIZE);
	m_drawboxcaption.SetString("Draw Frame on Boxes");
	m_controls.AddObject(&m_drawboxcaption);

	m_controls.AddObject(&m_drawbox);
	m_controls.NextLine();

	m_drawareascaption.SetFontID(1);
	m_drawareascaption.SetFontSize(VSFONTSIZE);
	m_drawareascaption.SetString("Draw Map Areas");
	m_controls.AddObject(&m_drawareascaption);

	m_controls.AddObject(&m_drawareas);
	m_controls.NextLine();

	/* media */
	m_screenmedia.SetNumEntries(sizeof(medialist)/sizeof(const char *));
	m_printmedia.SetNumEntries(sizeof(medialist)/sizeof(const char *));
	for(i=0;i<sizeof(medialist)/sizeof(const char *);++i)
	{
		m_screenmedia.SetEntry(i,medialist[i],medialist[i]);
		m_printmedia.SetEntry(i,medialist[i],medialist[i]);
	}

	m_screenmediacaption.SetFontID(1);
	m_screenmediacaption.SetFontSize(VSFONTSIZE);
	m_screenmediacaption.SetString("Screen Media");
	m_controls.AddObject(&m_screenmediacaption);

	m_screenmedia.SetSize(m_screenmedia.GetWidest(),m_screenmediacaption.GetLineHeight()+6);
	m_controls.AddObject(&m_screenmedia);
	m_controls.NextLine();

	m_printmediacaption.SetFontID(1);
	m_printmediacaption.SetFontSize(VSFONTSIZE);
	m_printmediacaption.SetString("Print Media");
	m_controls.AddObject(&m_printmediacaption);

	m_printmedia.SetSize(m_printmedia.GetWidest(),m_printmediacaption.GetLineHeight()+6);
	m_controls.AddObject(&m_printmedia);
	m_controls.NextLine();

	/* colorblind simulator values */
	m_colormode.SetNumEntries(sizeof(colormatrix)/sizeof(COLOR_MATRIX));
	for(i=0;i<sizeof(colormatrix)/sizeof(COLOR_MATRIX);++i)
		m_colormode.SetEntry(i,colormatrix[i].name,i);

	m_colormodecaption.SetFontID(1);
	m_colormodecaption.SetFontSize(VSFONTSIZE);
	m_colormodecaption.SetString("ColorBlind Simulation Mode");
	m_controls.AddObject(&m_colormodecaption);

	m_colormode.SetSize(m_colormode.GetWidest(),m_colormodecaption.GetLineHeight()+6);
	m_controls.AddObject(&m_colormode);
	m_controls.NextLine();

	m_clear.SetFontID(1);
	m_clear.SetFontSize(VSFONTSIZE);
	m_clear.SetString("Clear All");
	m_clear.SetSize(m_clear.GetWidth()+6,m_clear.GetLineHeight()+6);
	m_clear.SetEventHandler(this,CALLBACKNAME(ButtonsEvent));
	m_controls.AddObject(&m_clear);

	m_set.SetFontID(1);
	m_set.SetFontSize(VSFONTSIZE);
	m_set.SetString("Set All");
	m_set.SetSize(m_set.GetWidth()+6,m_set.GetLineHeight()+6);
	m_set.SetEventHandler(this,CALLBACKNAME(ButtonsEvent));
	m_controls.AddObject(&m_set);

	m_toggle.SetFontID(1);
	m_toggle.SetFontSize(VSFONTSIZE);
	m_toggle.SetString("Toggle All");
	m_toggle.SetEventHandler(this,CALLBACKNAME(ButtonsEvent));
	m_toggle.SetSize(m_toggle.GetWidth()+6,m_toggle.GetLineHeight()+6);
	m_controls.AddObject(&m_toggle);
	m_controls.NextLine();

	/* css enable/disable table */

	m_csstable.SetNumCols(3);
	m_csstable.SetColTitle(0,"Disable");
	m_csstable.SetColTitle(1,"Trace");
	m_csstable.SetColTitle(2,"Name");
	m_csstable.SetColWidth(0,50);
	m_csstable.SetColWidth(1,50);
	m_csstable.SetColWidth(2,200);
	m_csstable.SetAllowAddNewRow(false);
	m_csstable.SetAllowDelete(false);

	/* populate the table */

	for(i=0;i<kGUIHTMLPageObj::GetNumCSSAttributes();++i)
	{
		CSSBlockRowObj *row;

		row=new CSSBlockRowObj(i);
		row->SetBlocked(b->GetSettings()->GetCSSBlock(i));
		row->SetTrace(b->GetSettings()->GetCSSTrace(i));
		m_csstable.AddRow(row);
	}

	/* make table big enough to hold all entries */
	m_csstable.SetSize(m_csstable.CalcTableWidth(),m_csstable.CalcTableHeight());
	m_controls.AddObject(&m_csstable);
	m_controls.NextLine();

	/* set values to current settings */
	m_visiteddays.SetInt(b->GetSettings()->GetVisitedDays());
	m_cachemode.SetSelection(b->GetSettings()->GetCacheMode());
	m_cachesize.SetInt(b->GetSettings()->GetCacheSize());
	m_savemode.SetSelection(b->GetSettings()->GetSaveMode());
	m_loadimages.SetSelected(b->GetSettings()->GetLoadImages());
	m_usercss.SetString(b->GetSettings()->GetUserCSS());
	m_usecss.SetSelected(b->GetSettings()->GetUseCSS());
	m_useusercss.SetSelected(b->GetSettings()->GetUseUserCSS());
	m_drawbox.SetSelected(b->GetSettings()->GetDrawBoxes());
	m_drawareas.SetSelected(b->GetSettings()->GetDrawAreas());
	m_screenmedia.SetSelectionz(b->GetSettings()->GetScreenMedia()->GetString());
	m_printmedia.SetSelectionz(b->GetSettings()->GetPrintMedia()->GetString());
	m_colormode.SetSelection(b->GetColorMode());

	m_scroll.SetPos(0,0);
	m_scroll.SetSize(GetChildZoneW(),GetChildZoneH());
	m_scroll.AddObject(&m_controls);
	m_scroll.SetMaxWidth(m_controls.GetZoneW());
	m_scroll.SetMaxHeight(m_controls.GetZoneH());
	AddObject(&m_scroll);

	kGUI::AddWindow(this);
	GetTitle()->SetString("Browser Settings");
	SetEventHandler(this,CALLBACKNAME(WindowEvent));
}

void ViewSettings::ButtonsEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
		if(event->GetObj()==&m_clear)
		{
			for(unsigned int i=0;i<HTMLATT_UNKNOWN;++i)
			{
				CSSBlockRowObj *row;

				row=static_cast<CSSBlockRowObj *>(m_csstable.GetRow(i));
				row->SetBlocked(false);
			}
		}
		else if(event->GetObj()==&m_set)
		{
			for(unsigned int i=0;i<HTMLATT_UNKNOWN;++i)
			{
				CSSBlockRowObj *row;

				row=static_cast<CSSBlockRowObj *>(m_csstable.GetRow(i));
				row->SetBlocked(true);
			}
		}
		else if(event->GetObj()==&m_toggle)
		{
			for(unsigned int i=0;i<HTMLATT_UNKNOWN;++i)
			{
				CSSBlockRowObj *row;

				row=static_cast<CSSBlockRowObj *>(m_csstable.GetRow(i));
				row->SetBlocked(!row->GetBlocked());
			}
		}
	break;
	}
}

void ViewSettings::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		/* apply the settings back */
		m_b->GetSettings()->SetVisitedDays(m_visiteddays.GetInt());
		m_b->GetSettings()->SetCacheMode(m_cachemode.GetSelection());
		m_b->GetSettings()->SetCacheSize(m_cachesize.GetInt());
		m_b->GetSettings()->SetSaveMode(m_savemode.GetSelection());
		m_b->GetSettings()->SetLoadImages(m_loadimages.GetSelected());
		m_b->GetSettings()->SetUserCSS(&m_usercss);
		m_b->GetSettings()->SetUseCSS(m_usecss.GetSelected());
		m_b->GetSettings()->SetUseUserCSS(m_useusercss.GetSelected());
		m_b->GetSettings()->SetDrawBoxes(m_drawbox.GetSelected());
		m_b->GetSettings()->SetDrawAreas(m_drawareas.GetSelected());
		m_b->GetSettings()->SetScreenMedia(m_screenmedia.GetSelectionStringObj());
		m_b->GetSettings()->SetPrintMedia(m_printmedia.GetSelectionStringObj());
		m_b->SetColorMode(m_colormode.GetSelection());

		/* copy the enable/disable settings from the cssenable table */
		for(unsigned int i=0;i<HTMLATT_UNKNOWN;++i)
		{
			CSSBlockRowObj *row;

			row=static_cast<CSSBlockRowObj *>(m_csstable.GetRow(i));
			m_b->GetSettings()->SetCSSBlock(i,row->GetBlocked());
			m_b->GetSettings()->SetCSSTrace(i,row->GetTrace());
		}

		/* trigger redraw, and flush rule cache since user rules may have been added */
		m_b->FlushRuleCache();
		m_b->SettingsChanged();
		m_b->RePosition(true);
	delete this;
	break;
	case EVENT_SIZECHANGED:
		m_scroll.SetSize(GetChildZoneW(),GetChildZoneH());
	break;
	}
}

/* called when the window size changes */	
void ViewSettings::DirtyandCalcChildZone(void)
{
	kGUIContainerObj::DirtyandCalcChildZone();
}

CSSBlockRowObj::CSSBlockRowObj(unsigned int index)
{
	m_block.SetSelected(false);
	m_trace.SetSelected(false);
	m_name.SetString(kGUIHTMLPageObj::GetCSSAttributeName(index));
	m_objectlist[0]=&m_block;
	m_objectlist[1]=&m_trace;
	m_objectlist[2]=&m_name;
	m_name.SetLocked(true);
	SetRowHeight(20);
}

/* add URL to existing bookmarks, if already there then update title and return, else add it */

void kGUIBrowseSettings::AddBookmark(kGUIString *title,kGUIString *url,DataHandle *icon)
{
	unsigned int i;
	kGUIBookmark *bookmark;

	for(i=0;i<m_numbookmarks;++i)
	{
		bookmark=m_bookmarks.GetEntryPtr(i);
		if(!strcmp(bookmark->GetURL()->GetString(),url->GetString()))
		{
			/* already exists so just update the title and return */
			bookmark->SetTitle(title);
			bookmark->SetIcon(icon);
			return;
		}
	}
	/* not there so we now add it */
	bookmark=m_bookmarks.GetEntryPtr(m_numbookmarks++);
	bookmark->SetTitle(title);
	bookmark->SetURL(url);
	bookmark->SetIcon(icon);
}

/* this is used by the bookmark edit window */
void kGUIBrowseSettings::UpdateBookmark(unsigned int index,kGUIString *title,kGUIString *url,DataHandle *icon)
{
	kGUIBookmark *bookmark;

	bookmark=m_bookmarks.GetEntryPtr(index);
	bookmark->SetTitle(title);
	bookmark->SetURL(url);
	bookmark->SetIcon(icon);
}

/* load browse settings from the XML file */

void kGUIBrowseSettings::Load(kGUIXMLItem *root)
{
	kGUIXMLItem *group;
	kGUIXMLItem *tabs;
	kGUIXMLItem *bookmarks;
	kGUIXMLItem *bookmark;
	kGUIBookmark *be;
	kGUIXMLItem *item;
	unsigned int i;
	kGUIXMLItem *cookiegroup;
	kGUICookieJar *jar;

	group=root->Locate("browsersettings");
	if(!group)
		return;

	SetVisitedDays(group->Locate("visiteddays")->GetValueInt());
	SetCacheMode(group->Locate("cachemode")->GetValueInt());
	SetCacheSize(group->Locate("cachesize")->GetValueInt());
	
	item=group->Locate("screenmedia");
	if(item)
		SetScreenMedia(item->GetValue());

	item=group->Locate("printmedia");
	if(item)
		SetPrintMedia(item->GetValue());

	item=group->Locate("savemode");
	if(item)
		SetSaveMode(item->GetValue()->GetInt());

	m_numtabs=0;
	tabs=group->Locate("tabs");
	if(tabs)
	{
		for(m_numtabs=0;m_numtabs<tabs->GetNumChildren();++m_numtabs)
		{
			item=tabs->GetChild(m_numtabs);
			m_tabs.GetEntryPtr(m_numtabs)->SetString(item->GetValue());
		}
	}

	kGUIHTMLSettings::Load(group);
	if(m_itemcache)
		m_itemcache->Load(group);
	if(m_visitedcache)
		m_visitedcache->Load(group);

	cookiegroup=group->Locate("cookies");
	jar=kGUI::GetCookieJar();
	if(cookiegroup)
	{
		if(jar)
			jar->Load(cookiegroup);
	}

	/* load bookmarks */
	bookmarks=group->Locate("bookmarks");
	if(bookmarks)
	{
		m_numbookmarks=bookmarks->GetNumChildren();
		for(i=0;i<m_numbookmarks;++i)
		{
			be=GetBookmark(i);
			bookmark=bookmarks->GetChild(i);
			be->SetTitle(bookmark->Locate("title")->GetValue());
			be->SetURL(bookmark->Locate("url")->GetValue());
			if(bookmark->Locate("icon"))
			{
				unsigned int j;
				unsigned int binarylen;
				kGUIString *b;
				Array<unsigned char>base64icon;
				Array<unsigned char>binaryicon;
				DataHandle icon;

				/* convert from base64 encoded to binary */
				icon.SetMemory();
				b=bookmark->Locate("icon")->GetValue();
				
				base64icon.Init(b->GetLen(),1);
				for(j=0;j<b->GetLen();++j)
					base64icon.SetEntry(j,b->GetChar(j));

				binarylen=kGUI::Base64Decode(b->GetLen(),&base64icon,&binaryicon);

				icon.SetMemory();
				icon.OpenWrite("wb",binarylen);
				icon.Write(binaryicon.GetArrayPtr(),(unsigned long)binarylen);
				icon.Close();

				be->SetIcon(&icon);
			}
		}
	}
}

/* load tabs into browser */
void kGUIBrowseSettings::LoadTabs(kGUIBrowseObj *b)
{
	unsigned int i;
	kGUIString *url;

	for(i=0;i<m_numtabs;++i)
	{
		url=m_tabs.GetEntryPtr(i);
		if(i)
			b->NewTab();
		b->SetSource(url,0,0,0);
	}
}

/* save tabs if enabled */
void kGUIBrowseSettings::SaveTabs(kGUIBrowseObj *b)
{
	if(m_savemode==SAVEMODE_YES || (m_savemode==SAVEMODE_YESMORE1 && b->GetNumTabs()>1))
	{
		unsigned int i;
		kGUIString url;

		m_numtabs=b->GetNumTabs();
		for(i=0;i<m_numtabs;++i)
		{
			b->CopyTabURL(i,&url);
			if(url.GetLen())
				m_tabs.GetEntryPtr(i)->SetString(&url);
		}
	}
	else
		m_numtabs=0;
}

/* save browse settings to the XML file */

void kGUIBrowseSettings::Save(kGUIXMLItem *root)
{
	kGUIXMLItem *group;
	unsigned int i;
	kGUIBookmark *be;
	kGUIXMLItem *bookmarks;
	kGUIXMLItem *bookmark;
	kGUIXMLItem *cookiegroup;
	kGUICookieJar *jar;
	kGUIXMLItem *tabs;

	group=new kGUIXMLItem();
	group->SetName("browsersettings");
	root->AddChild(group);

	group->AddChild("visiteddays",m_visiteddays);
	group->AddChild("cachemode",m_cachemode);
	group->AddChild("cachesize",m_cachesize);
	group->AddChild("screenmedia",&m_screenmedia);
	group->AddChild("printmedia",&m_printmedia);
	group->AddChild("savemode",m_savemode);

	/* save tabs (if any have been added) */
	if(m_numtabs)
	{
		tabs=new kGUIXMLItem();
		tabs->SetName("tabs");
		group->AddChild(tabs);
		for(i=0;i<m_numtabs;++i)
			tabs->AddChild("tab",m_tabs.GetEntryPtr(i));
	}

	kGUIHTMLSettings::Save(group);
	
	if(m_visitedcache)
		m_visitedcache->Save(group);
	if(m_itemcache)
		m_itemcache->Save(group);

	jar=kGUI::GetCookieJar();
	if(jar)
	{
		cookiegroup=group->AddChild("cookies");
		jar->Save(cookiegroup);
	}

	/* save bookmarks */
	bookmarks=group->AddChild("bookmarks");
	for(i=0;i<m_numbookmarks;++i)
	{
		be=GetBookmark(i);
		bookmark=bookmarks->AddChild("bookmark");
		bookmark->AddParm("title",be->GetTitle());
		bookmark->AddParm("url",be->GetURL());

		if(be->GetIcon()->GetLoadableSize())
		{
			unsigned int j;
			unsigned int binarylen;
			unsigned int base64len;
			DataHandle *dh;
			unsigned char c;
			kGUIString bs;
			Array<unsigned char>binaryicon;
			Array<unsigned char>base64icon;
			
			dh=be->GetIcon();
			binarylen=dh->GetLoadableSize();

			/* fill the binary array */
			dh->Open();
			binaryicon.Init(binarylen,1);
			for(j=0;j<binarylen;++j)
			{
				dh->Read(&c,(unsigned long)1L);
				binaryicon.SetEntry(j,c);
			}
			dh->Close();
			
			base64len=kGUI::Base64Encode(binarylen,&binaryicon,&base64icon);
			bs.SetString((const char *)base64icon.GetArrayPtr(),base64len);

			bookmark->AddParm("icon",&bs);
		}
	}
}

/* this is a copy of the input box class with the ability to draw the "favicon" */
/* in the left of the box */

kGUIOffsetInputBoxObj::kGUIOffsetInputBoxObj()
{
	/* leave room for 16x16 icon */
	m_currentframe=0;
	m_animdelay=0;
	m_animateeventactive=false;
	m_iconvalid=false;
	SetOffsets();
}

void kGUIOffsetInputBoxObj::SetString(kGUIString *s)
{
	/* extract out the domain */
	kGUIInputBoxObj::SetString(s);

	if(strncmp(s->GetString(),"http://",7)==0)
		m_domain.SetString(s->GetString()+7);
	else if(strncmp(s->GetString(),"https://",8)==0)
		m_domain.SetString(s->GetString()+8);
	else
		m_domain.Clear();

	m_domain.Clip("/");
	m_domain.Clip("?");
	m_domain.Clip(":");
	SetOffsets();
}

void kGUIOffsetInputBoxObj::SetOffsets(void)
{
	unsigned int iw;
	unsigned int dw;

	dw=m_domain.GetWidth();
	if(m_iconvalid)
	{
		if(dw)
			m_iconx=2;
		else
			m_iconx=0;
		iw=16+2;
	}
	else
	{
		m_iconx=0;
		iw=0;
	}
	m_domainx=m_iconx+iw;
	if(dw)
		dw+=2;
	
	SetXOffset(m_domainx+dw);
}

bool kGUIOffsetInputBoxObj::SetIcon(DataHandle *dh)
{
	unsigned int i;
	kGUIImage full;
	kGUIDrawSurface tempds;
	kGUIDrawSurface *ds;
	double scale,scale2;

	m_iconvalid=false;
	if(dh)
	{
		full.CopyHandle(dh);
		full.LoadPixels();
		if(full.IsValid())
		{
			tempds.Init(16,16);
			/* draw thumbnail */
			kGUI::PushClip();
			ds=kGUI::GetCurrentSurface();
			kGUI::SetCurrentSurface(&tempds);
			kGUI::ResetClip();	/* set clip to full surface on stack */

			scale=16/(double)full.GetImageWidth();
			scale2=16/(double)full.GetImageHeight();
			if(scale>scale2)
				scale=scale2;
			full.SetScale(scale,scale);

			for(i=0;i<full.GetNumFrames();++i)
			{
				/* clear the window to white since the icon can have transparency */
				tempds.Clear(DrawColor(255,255,255));
				full.Draw(i,(16-(int)(scale*full.GetImageWidth()))>>1,(16-(int)(scale*full.GetImageHeight()))>>1);
				m_icon.SetMemImageCopy(i,tempds.GetFormat(),tempds.GetWidth(),tempds.GetHeight(),tempds.GetBPP(),(unsigned char *)tempds.GetSurfacePtr(0,0));
				m_icon.SetDelay(i,full.GetDelay(i));
			}
			kGUI::SetCurrentSurface(ds);
			kGUI::PopClip();

			m_currentframe=0;
			m_iconvalid=true;

			/* add animate event? */
			if(m_icon.GetNumFrames()>1 && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				m_animdelay=0;
				kGUI::AddUpdateTask(this,CALLBACKNAME(Animate));
			}
		}
	}
	SetOffsets();
	Dirty();
	return(m_iconvalid);
}

void kGUIOffsetInputBoxObj::Animate(void)
{
	if( (m_icon.GetNumFrames()<1))
	{
		m_animateeventactive=false;
		kGUI::DelUpdateTask(this,CALLBACKNAME(Animate));
		return;
	}

	m_animdelay+=kGUI::GetET();
	if(m_animdelay>=m_icon.GetDelay(m_currentframe))
	{
		m_animdelay=0;
		if(++m_currentframe>=m_icon.GetNumFrames())
			m_currentframe=0;
		Dirty();
	}
}

void kGUIOffsetInputBoxObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUIInputBoxObj::Draw();

		/* draw blue background */
		if(GetXOffset())
			kGUI::DrawRect(c.lx,c.ty,c.lx+GetXOffset(),c.by,DrawColor(91,153,223));

		/* draw the icon */
		if(m_iconvalid)
			m_icon.Draw(m_currentframe,c.lx+m_iconx,c.ty+2+1);

		m_domain.Draw(c.lx+m_domainx,c.ty+5,0,0,DrawColor(255,255,255));
	}
	kGUI::PopClip();
}

void kGUIDomainTextObj::Draw(void)
{
	unsigned int dw;
	kGUICorners c;

	if(strcmp(m_last.GetString(),GetString()))
	{
		m_last.SetString(GetString());
		/* changed */
		if(strncmp(GetString(),"http://",7)==0)
			m_domain.SetString(GetString()+7);
		else if(strncmp(GetString(),"https://",8)==0)
			m_domain.SetString(GetString()+8);
		else
			m_domain.Clear();

		m_domain.Clip("/");
		m_domain.Clip("?");
		m_domain.Clip(":");
	}

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		dw=m_domain.GetWidth();
		if(dw)
		{
			kGUI::DrawRect(c.lx,c.ty,c.lx+dw+4,c.by,DrawColor(91,153,223));
			m_domain.Draw(c.lx+2,c.ty+5,0,0,DrawColor(255,255,255));
			c.lx+=dw+4;
		}
		kGUIText::Draw(c.lx,c.ty+5,0,0);
	}
	kGUI::PopClip();
}

bool kGUIBrowseIcon::SetIcon(DataHandle *dh)
{
	unsigned int i;
	kGUIImage full;
	kGUIDrawSurface tempds;
	kGUIDrawSurface *ds;
	double scale,scale2;

//	m_valid=false;
	if(dh)
	{
		full.CopyHandle(dh);
		full.LoadPixels();
		if(full.IsValid())
		{
			tempds.Init(16,16);
			/* draw thumbnail */
			kGUI::PushClip();
			ds=kGUI::GetCurrentSurface();
			kGUI::SetCurrentSurface(&tempds);
			kGUI::ResetClip();	/* set clip to full surface on stack */

			scale=16/(double)full.GetImageWidth();
			scale2=16/(double)full.GetImageHeight();
			if(scale>scale2)
				scale=scale2;
			full.SetScale(scale,scale);

			for(i=0;i<full.GetNumFrames();++i)
			{
				/* clear the window to white since the icon can have transparency */
				tempds.Clear(DrawColor(255,255,255));
				full.Draw(i,(16-(int)(scale*full.GetImageWidth()))>>1,(16-(int)(scale*full.GetImageHeight()))>>1);
				m_icon.SetMemImageCopy(i,tempds.GetFormat(),tempds.GetWidth(),tempds.GetHeight(),tempds.GetBPP(),(unsigned char *)tempds.GetSurfacePtr(0,0));
				m_icon.SetDelay(i,full.GetDelay(i));
			}
			kGUI::SetCurrentSurface(ds);
			kGUI::PopClip();

			m_currentframe=0;
			m_valid=true;

			/* add animate event? */
			if(m_icon.GetNumFrames()>1 && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				m_animdelay=0;
				kGUI::AddUpdateTask(this,CALLBACKNAME(Animate));
			}
		}
	}
	return(m_valid);
}

void kGUIBrowseIcon::Animate(void)
{
	if( (m_icon.GetNumFrames()<1))
	{
		m_animateeventactive=false;
		kGUI::DelUpdateTask(this,CALLBACKNAME(Animate));
		return;
	}

	m_animdelay+=kGUI::GetET();
	if(m_animdelay>=m_icon.GetDelay(m_currentframe))
	{
		m_animdelay=0;
		if(++m_currentframe>=m_icon.GetNumFrames())
			m_currentframe=0;
		Dirty();
	}
}


/*****************************************************************************/

AuthenticateWindow::AuthenticateWindow(kGUIBrowseObj *browse,DownloadPageRecord *dle,kGUIString *realm,kGUIString *domain)
{
	m_browse=browse;
	m_dle=dle;
	m_domrealm.Sprintf("%s:%s",domain->GetString(),realm->GetString());

	m_window.SetTitle("Authentication Required");

	/* set to max size first */
	m_window.SetTop(true);
	m_window.SetSize(kGUI::GetScreenWidth(),kGUI::GetScreenHeight());
	m_window.SetEventHandler(this,CALLBACKNAME(Event));

	m_controls.SetPos(0,0);
	m_controls.SetMaxWidth(m_window.GetChildZoneW());

	m_title.Sprintf("Enter username and password for '%s' at %s",realm->GetString(),domain->GetString());
	m_controls.AddObject(&m_title);
	m_controls.NextLine();

	m_namecaption.Sprintf("User Name:");
	m_controls.AddObject(&m_namecaption);
	m_controls.NextLine();

	m_name.SetSize(300,20);
	m_controls.AddObject(&m_name);
	m_controls.NextLine();

	m_passwordcaption.Sprintf("Password:");
	m_controls.AddObject(&m_passwordcaption);
	m_controls.NextLine();

	m_password.SetSize(300,20);
	m_controls.AddObject(&m_password);
	m_controls.NextLine();
	m_password.SetPassword(true);	/* draw asterisks for password characters */

	m_ok.SetSize(100,30);
	m_ok.SetString("OK");
	m_ok.SetEventHandler(this,CALLBACKNAME(Event));
	m_controls.AddObject(&m_ok);

	m_cancel.SetSize(100,30);
	m_cancel.SetString("Cancel");
	m_cancel.SetEventHandler(this,CALLBACKNAME(Event));
	m_controls.AddObject(&m_cancel);

	m_window.AddObject(&m_controls);

	m_window.Shrink();
	m_window.Center();
	kGUI::AddWindow(&m_window);

	/* put the flashing cursor in here */
	m_name.SetCurrent();
}

void AuthenticateWindow::Event(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		if(event->GetObj()==&m_window)
		{
			delete m_dle;
			delete this;
		}
	break;
	case EVENT_PRESSED:
		if(event->GetObj()==&m_ok)
		{
			m_browse->Authenticate(&m_domrealm,&m_name,&m_password);
			m_window.Close();
		}
		else if(event->GetObj()==&m_cancel)
		{
			m_browse->ShowError(m_dle);
			m_window.Close();
		}
	break;	
	}
}

/*****************************************************************************/

EditBookmarkWindow::EditBookmarkWindow(kGUIBrowseSettings *settings)
{
	unsigned int i;
	EditBookmarkRow *row;

	m_settings=settings;
	m_window.SetTitle("Edit Bookmarks");

	/* set to max size first */
	m_window.SetTop(true);
	m_window.SetSize(kGUI::GetScreenWidth(),kGUI::GetScreenHeight());
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));

	m_controls.SetPos(0,0);
	m_controls.SetMaxWidth(m_window.GetChildZoneW());

	m_up.SetString(kGUI::GetString(KGUISTRING_UP));
	m_up.Contain();
	m_up.SetEventHandler(this,CALLBACKNAME(Up));
	m_controls.AddObject(&m_up);

	m_down.SetString(kGUI::GetString(KGUISTRING_DOWN));
	m_down.Contain();
	m_down.SetEventHandler(this,CALLBACKNAME(Down));
	m_controls.AddObject(&m_down);

	m_controls.NextLine();

	m_table.SetAllowDelete(true);
	m_table.SetAllowAddNewRow(false);
	m_table.SetEventHandler(this,& CALLBACKNAME(TableEvent));

	m_table.SetNumCols(2);
	m_table.SetColTitle(1,"Bookmark Title");
	m_table.SetColWidth(0,16);
	m_table.SetColWidth(1,(int)(kGUI::GetScreenWidth()*0.75f));

	/* populate table with the bookmarks */
	for(i=0;i<settings->GetNumBookmarks();++i)
	{
		row=new EditBookmarkRow(m_settings->GetBookmark(i)->GetTitle(),
								m_settings->GetBookmark(i)->GetURL(),
								m_settings->GetBookmark(i)->GetIcon());

		m_table.AddRow(row);
	}

	m_table.SetSize(MIN(m_table.CalcTableWidth(),m_window.GetChildZoneW()),MIN(500,kGUI::GetScreenHeight()));
	m_controls.AddObject(&m_table);
	UpdateButtons();

	m_window.AddObject(&m_controls);

	m_window.Shrink();
	m_window.Center();
	kGUI::AddWindow(&m_window);
}

EditBookmarkWindow::~EditBookmarkWindow()
{
	//todo call browser 'SettingsChanged'
	m_table.DeleteChildren(true);
}

void EditBookmarkWindow::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

void EditBookmarkWindow::TableEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_MOVED:
		UpdateButtons();
	break;
	case EVENT_AFTERUPDATE:
		TableChanged();
	break;
	}
}

/* table has changed so update bookmarks to match */
void EditBookmarkWindow::TableChanged(void)
{
	unsigned int i,n;
	EditBookmarkRow *row;

	n=m_table.GetNumberRows();

	/* update bookmarks to match table */
	m_settings->SetNumBookmarks(n);
	for(i=0;i<n;++i)
	{
		row=static_cast<EditBookmarkRow *>(m_table.GetChild(i));
		m_settings->UpdateBookmark(i,row->GetTitle(),row->GetURL(),row->GetIcon());
	}
}

void EditBookmarkWindow::Up(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		unsigned int line;

		line=m_table.GetCursorRow();
		if(line<m_table.GetNumChildren(0) && line>0)
		{
			m_table.SwapRow(-1);
			m_table.MoveRow(-1);
			TableChanged();
			UpdateButtons();
		}
	}
}

void EditBookmarkWindow::Down(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		unsigned int line;

		line=m_table.GetCursorRow();
		if((int)line<((int)m_table.GetNumChildren(0)-1))
		{
			m_table.SwapRow(1);
			m_table.MoveRow(1);
			TableChanged();
			UpdateButtons();
		}
	}
}

void EditBookmarkWindow::UpdateButtons(void)
{
	m_up.SetEnabled(m_table.GetCursorRow()>0);
	m_down.SetEnabled((m_table.GetCursorRow()+1)<m_table.GetNumChildren());
}

/*****************************************************************************/

ViewCookieWindow::ViewCookieWindow(kGUIBrowseSettings *settings)
{
	unsigned int i,n;
	ViewCookieRow *row;
	kGUICookieJar *jar;
	Array<kGUICookie *>cookielist;

	jar=kGUI::GetCookieJar();
	m_settings=settings;
	m_window.SetTitle("View Cookies");

	/* set to max size first */
	m_window.SetTop(true);
	m_window.SetSize(kGUI::GetScreenWidth(),kGUI::GetScreenHeight());
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));

	m_controls.SetPos(0,0);
	m_controls.SetMaxWidth(m_window.GetChildZoneW());

	m_block.SetSize(100,30);
	m_block.SetString("Block");
	m_block.SetEventHandler(this,CALLBACKNAME(Block));
	m_controls.AddObject(&m_block);

	m_controls.NextLine();

	m_table.SetAllowDelete(true);
	m_table.SetAllowAddNewRow(false);
	m_table.SetEventHandler(this,& CALLBACKNAME(TableEvent));

	m_table.SetNumCols(5);
	m_table.SetColTitle(0,"Domain");
	m_table.SetColWidth(0,200);
	m_table.SetColTitle(1,"Name");
	m_table.SetColWidth(1,100);
	m_table.SetColTitle(2,"Value");
	m_table.SetColWidth(2,200);
	m_table.SetColTitle(3,"Path");
	m_table.SetColWidth(3,200);
	m_table.SetColTitle(4,"Expiry");
	m_table.SetColWidth(4,200);

	/* populate table with the bookmarks */

	n=jar->GetCookieList(&cookielist);
	for(i=0;i<n;++i)
	{
		row=new ViewCookieRow(cookielist.GetEntry(i));
		m_table.AddRow(row);
	}

	m_table.SetSize(MIN(m_table.CalcTableWidth(),m_window.GetChildZoneW()),MIN(500,kGUI::GetScreenHeight()));
	m_controls.AddObject(&m_table);
	UpdateButtons();

	m_window.AddObject(&m_controls);

	m_window.Shrink();
	m_window.Center();
	kGUI::AddWindow(&m_window);
}

ViewCookieWindow::~ViewCookieWindow()
{
	m_table.DeleteChildren(true);
}

void ViewCookieWindow::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

void ViewCookieWindow::TableEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_MOVED:
		UpdateButtons();
	break;
	case EVENT_DELETEROW:
	{
		ViewCookieRow *row;
		kGUICookie *cookie;

		/* row number being deleted is passed in event -> value[0].i */
		row=static_cast<ViewCookieRow *>(m_table.GetChild(event->m_value[0].i));
		cookie=row->GetCookie();
		cookie->SetRemove(true);		/* flag cookie for removal */
	}
	break;
	case EVENT_AFTERUPDATE:
		/* handle delete */
	break;
	}
}

void ViewCookieWindow::Block(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
	}
}

void ViewCookieWindow::UpdateButtons(void)
{
	/* allow block if table is not empty */
	m_block.SetEnabled(m_table.GetCursorRow()<m_table.GetNumChildren());
}

ViewCookieRow::ViewCookieRow(kGUICookie *cookie)
{
	m_cookie=cookie;

	m_objectlist[0]=&m_domain;
	m_objectlist[1]=&m_name;
	m_objectlist[2]=&m_value;
	m_objectlist[3]=&m_path;
	m_objectlist[4]=&m_expiry;

	m_domain.SetString(cookie->GetDomain());
	m_name.SetString(cookie->GetName());
	m_value.SetString(cookie->GetValue());
	m_path.SetString(cookie->GetPath());
	/* todo, add expiry etc, tickbox for permanent or nor */
	
	cookie->GetExpiry()->LongDate(&m_expiry);
}

void HTMLPageObj::Draw(void)
{
	kGUIHTMLPageObj::Draw();

	/* 0 = normal mode, 1 or greater is colorblind filter */
	if(m_colormode>0)
	{
		kGUICorners c;
		const COLOR_MATRIX *cm;

		GetCorners(&c);
		kGUI::PushClip();
		kGUI::ShrinkClip(&c);

		/* is there anywhere to draw? */
		if(kGUI::ValidClip())
		{
			const kGUICorners *cc;
			int w,h,n;
			kGUIColor *sp;
			int skip;
			kGUIColor color;
			double r,g,b;
			double newr,newg,newb;

			cm=&colormatrix[m_colormode];
			cc=kGUI::GetClipCorners();
			w=cc->rx-cc->lx;
			h=cc->by-cc->ty;

			/* convert colors */
			sp=kGUI::GetSurfacePtr(cc->lx,cc->ty);
			skip=kGUI::GetSurfaceWidth()-w;
			while(h)
			{
				n=w;
				while(n--)
				{
					color=*(sp);
					DrawColorToRGB(color,r,g,b);

					newr=(r*cm->rr)+(g*cm->rg)+(b*cm->rb);
					newg=(r*cm->gr)+(g*cm->gg)+(b*cm->gb);
					newb=(r*cm->br)+(g*cm->bg)+(b*cm->bb);

					*(sp++)=DrawColor((int)newr,(int)newg,(int)newb);
				}
				sp+=skip;
				--h;
			}
		}
		kGUI::PopClip();
	}
}

