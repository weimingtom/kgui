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

#include "kgui.h"
#include "kguibrowse.h"

#define SMALLCAPTIONFONT 1
#define SMALLCAPTIONSIZE 9

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

class ViewSettings : public kGUIWindowObj
{
public:
	ViewSettings(kGUIBrowseObj *b,int w,int h);
	/* called when window size changes */
	void DirtyandCalcChildZone(void);
private:
	CALLBACKGLUEPTR(ViewSettings,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);

	kGUIBrowseObj *m_b;
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

	/* draw options */

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

	/* draw areas */
	kGUITextObj m_screenmediacaption;
	kGUIComboBoxObj m_screenmedia;

	/* draw areas */
	kGUITextObj m_printmediacaption;
	kGUIComboBoxObj m_printmedia;
};

void PageInfo::Set(const char *url,const char *post,const char *referer,const char *source,const char *header)
{
	/* title defaults to URL and is then replaced if there is a valud title in the html page */
	m_title.SetString(url);
	m_url.SetString(url);
	m_post.SetString(post);
	m_referer.SetString(referer);
	m_source.SetString(source);
	m_header.SetString(header);
}

enum
{
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

static const char *mainmenutxt[]={
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

kGUIBrowseObj::kGUIBrowseObj(kGUIBrowseSettings *settings,int w,int h)
{
	m_screenpage.SetSettings(settings);
	m_screenpage.SetItemCache(settings->GetItemCache());
	m_screenpage.SetVisitedCache(settings->GetVisitedCache());
	m_screenpage.SetAuthHandler(&m_ah);

	m_printpage.SetSettings(settings);
	m_printpage.SetItemCache(settings->GetItemCache());
	m_printpage.SetVisitedCache(settings->GetVisitedCache());
	m_printpage.SetAuthHandler(&m_ah);

	m_settings=settings;
	m_pageindex=0;
	m_pageend=0;
	m_numplugins=0;
	m_plugins.Init(4,4);
	m_iconvalid=false;

	SetNumGroups(1);
	SetZone(0,0,w,h);
	m_backimage.SetFilename("_back.gif");
	m_forwardimage.SetFilename("_forward.gif");
	m_reloadimage.SetFilename("_reload.gif");
	m_reloadimage2.SetFilename("_reload2.gif");
	m_lockedimage.SetFilename("_locked.gif");
	m_unlockedimage.SetFilename("_unlocked.gif");

	m_menulabel.SetFontSize(20);
	m_menulabel.SetFontID(1);
	m_menulabel.SetString("Menu");
	m_menulabel.SetEventHandler(this,CALLBACKNAME(ShowMainMenu));
	m_browsecontrols.AddObject(&m_menulabel);

	m_menu.SetFontSize(14);
	m_menu.Init(MAINMENU_NUM,mainmenutxt);
	m_menu.SetEventHandler(this,CALLBACKNAME(DoMainMenu));

	m_bookmarkslabel.SetFontSize(20);
	m_bookmarkslabel.SetFontID(1);
	m_bookmarkslabel.SetString("Bookmarks");
	m_bookmarkslabel.SetEventHandler(this,CALLBACKNAME(ShowBookmarks));
	m_browsecontrols.AddObject(&m_bookmarkslabel);

	m_bookmarksmenu.SetFontSize(14);
	m_bookmarksmenu.SetEventHandler(this,CALLBACKNAME(DoBookmarks));

	m_gomenu.SetFontSize(14);
	m_gomenu.SetEventHandler(this,CALLBACKNAME(DoGotoMenu));

	m_backcaption.SetPos(0,0);
	m_backcaption.SetFontSize(SMALLCAPTIONSIZE);
	m_backcaption.SetFontID(SMALLCAPTIONFONT);
	m_backcaption.SetString("Back");

	m_back.SetPos(0,15);
	m_back.SetHint("Zoom In on the map.");
	m_back.SetImage(&m_backimage);
	m_back.SetSize(m_backimage.GetImageWidth()+6,m_backimage.GetImageHeight()+6);
	m_back.SetEventHandler(this,CALLBACKNAME(GoBack));
//	m_back.SetRightClicked(this,CALLBACKNAME(GoBackMenu));
	m_browsecontrols.AddObjects(2,&m_backcaption,&m_back);

	m_forwardcaption.SetPos(0,0);
	m_forwardcaption.SetFontSize(SMALLCAPTIONSIZE);
	m_forwardcaption.SetFontID(SMALLCAPTIONFONT);
	m_forwardcaption.SetString("Forward");

	m_forward.SetPos(0,15);
	m_forward.SetHint("Zoom Out on the map.");
	m_forward.SetImage(&m_forwardimage);
	m_forward.SetSize(m_forwardimage.GetImageWidth()+6,m_forwardimage.GetImageHeight()+6);
	m_forward.SetEventHandler(this,CALLBACKNAME(GoForward));
//	m_forward.SetRightClicked(this,CALLBACKNAME(GoForwardMenu));
	m_browsecontrols.AddObjects(2,&m_forwardcaption,&m_forward);

	m_refreshcaption.SetPos(0,0);
	m_refreshcaption.SetFontSize(SMALLCAPTIONSIZE);
	m_refreshcaption.SetFontID(SMALLCAPTIONFONT);
	m_refreshcaption.SetString("Reload");

	m_refresh.SetImage(&m_reloadimage);
	m_refresh.SetSize(m_reloadimage.GetImageWidth()+6,m_reloadimage.GetImageHeight()+6);
	m_refresh.SetPos(0,15);
	m_refresh.SetEventHandler(this,CALLBACKNAME(Refresh));
	m_browsecontrols.AddObjects(2,&m_refreshcaption,&m_refresh);

	m_refresh2caption.SetPos(0,0);
	m_refresh2caption.SetFontSize(SMALLCAPTIONSIZE);
	m_refresh2caption.SetFontID(SMALLCAPTIONFONT);
	m_refresh2caption.SetString("Reload All");

	m_refresh2.SetImage(&m_reloadimage2);
	m_refresh2.SetSize(m_reloadimage2.GetImageWidth()+6,m_reloadimage2.GetImageHeight()+6);
	m_refresh2.SetPos(0,15);
	m_refresh2.SetEventHandler(this,CALLBACKNAME(RefreshAll));
	m_browsecontrols.AddObjects(2,&m_refresh2caption,&m_refresh2);

	m_printcaption.SetPos(0,0);
	m_printcaption.SetFontSize(SMALLCAPTIONSIZE);
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
	m_urlcaption.SetFontSize(SMALLCAPTIONSIZE);
	m_urlcaption.SetFontID(SMALLCAPTIONFONT);
	m_urlcaption.SetString("URL");

	m_url.SetString("http://");
	m_url.SetPos(0,15);
	m_url.SetSize(750,22);
	m_url.SetEventHandler(this,CALLBACKNAME(UrlChanged));
	m_browsecontrols.AddObjects(2,&m_urlcaption,&m_url);
	m_browsecontrols.NextLine();

	m_referercaption.SetPos(0,0);
	m_referercaption.SetFontSize(SMALLCAPTIONSIZE);
	m_referercaption.SetFontID(SMALLCAPTIONFONT);
	m_referercaption.SetString("Referrer");

	m_referer.SetPos(0,15);
	m_referer.SetSize(800,20);

	m_busyimage.SetPos(0,0);
	m_busyimage.SetFilename("_busy.gif");
	m_busyimage.SetSize(m_busyimage.GetImageWidth(),m_busyimage.GetImageHeight());
	m_browsecontrols.AddObjects(1,&m_busyimage);

	m_statuscaption.SetPos(0,0);
	m_statuscaption.SetFontSize(SMALLCAPTIONSIZE);
	m_statuscaption.SetFontID(SMALLCAPTIONFONT);
	m_statuscaption.SetString("Status");

	m_status.SetPos(0,15);
	m_status.SetSize(200,20);
	m_browsecontrols.AddObjects(2,&m_statuscaption,&m_status);

	m_linkcaption.SetPos(0,0);
	m_linkcaption.SetFontSize(SMALLCAPTIONSIZE);
	m_linkcaption.SetFontID(SMALLCAPTIONFONT);
	m_linkcaption.SetString("Link");

	m_linkurl.SetPos(0,15);
	m_linkurl.SetSize(400,20);
	m_browsecontrols.AddObjects(2,&m_linkcaption,&m_linkurl);

	/* tell html renderer where to put the link under the mouse */
	m_screenpage.SetDrawLinkUnder(&m_linkurl);			

	AddObject(&m_browsecontrols);

	m_screenpage.SetPos(0,m_browsecontrols.GetZoneH());
	h-=m_browsecontrols.GetZoneH();
	m_screenpage.SetSize(w,h);
	m_screenpage.SetClickCallback(this,CALLBACKNAME(Click));
//	m_screenpage.SetIconCallback(&m_url,CALLBACKCLASSNAME(kGUIOffsetInputBoxObj,SetIcon));
	m_screenpage.SetIconCallback(this,CALLBACKNAME(SetIcon));
	AddObject(&m_screenpage);

	UpdateButtons();
}

void kGUIBrowseObj::SetIcon(DataHandle *dh)
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

void kGUIBrowseObj::ShowMainMenu(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_LEFTCLICK)
		m_menu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

void kGUIBrowseObj::DoMainMenu(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		switch(m_menu.GetSelection())
		{
		case MAINMENU_VIEWPAGESOURCE:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Page Source '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&m_source);
		}
		break;
		case MAINMENU_VIEWCORRECTEDPAGESOURCE:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString correctedsource;

			title.Sprintf("Corrected Page Source '%s'",m_screenpage.GetTitle()->GetString());

			m_screenpage.GetCorrectedSource(&correctedsource);
			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&correctedsource);
		}
		break;
		case MAINMENU_VIEWPOSTDATA:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Post Data '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&m_post);
		}
		break;
		case MAINMENU_VIEWHEADER:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Header '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&m_header);
		}
		break;
		case MAINMENU_VIEWSCRIPTSOURCE:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Page Scripts '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,m_screenpage.GetScripts());
		}
		break;
		case MAINMENU_VIEWCSS:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString source;

			title.Sprintf("Page CSS '%s'",m_screenpage.GetTitle()->GetString());

			m_screenpage.GetCSS(&source);
			source.Replace("}","}\n");

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&source);
		}
		break;
		case MAINMENU_VIEWCORRECTEDCSS:
		{
			ViewSource *vs;
			kGUIString title;
			kGUIString source;

			title.Sprintf("Corrected Page CSS '%s'",m_screenpage.GetTitle()->GetString());

			m_screenpage.GetCorrectedCSS(&source);
			source.Replace("}","}\n");

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,&source);
		}
		break;
		case MAINMENU_VIEWERRORS:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Page Errors '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,m_screenpage.GetErrors());
		}
		break;
		case MAINMENU_VIEWMEDIA:
		{
			ViewSource *vs;
			kGUIString title;

			title.Sprintf("Page Media '%s'",m_screenpage.GetTitle()->GetString());

			vs=new ViewSource(	(int)(kGUI::GetScreenWidth()*0.75),
								(int)(kGUI::GetScreenHeight()*0.75),
								&title,m_screenpage.GetMedia());
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
			m_screenpage.TraceLayout();
		break;
		case MAINMENU_TRACEDRAW:
			m_screenpage.TraceDraw();
		break;
		case MAINMENU_SETTINGS:
		{
			ViewSettings *vs;

			/* view / edit settings for browser */
			vs=new ViewSettings(this,
					(int)(kGUI::GetScreenWidth()*0.75),
					(int)(kGUI::GetScreenHeight()*0.75));
		}
		break;
		}
	}
}

void kGUIBrowseObj::ShowBookmarks(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_LEFTCLICK)
	{
		/* populate the bookmarks menu */
		unsigned int i;
		unsigned int numbookmarks;
		kGUIImageObj *icon;

		if(m_settings)
		{
			numbookmarks=m_settings->GetNumBookmarks();
			
			m_bookmarksmenu.SetIconWidth(16);
			m_bookmarksmenu.Init(numbookmarks+2);
			m_bookmarksmenu.SetEntry(0,"Bookmark Page",0);
			m_bookmarksmenu.SetEntry(1,"Edit Bookmarks",1);
			for(i=0;i<numbookmarks;++i)
			{
				m_bookmarksmenu.SetEntry(i+2,m_settings->GetBookmark(i)->GetTitle(),i+2);
				icon=m_bookmarksmenu.GetEntry(i+2)->GetIconObj();
				icon->Copy(m_settings->GetBookmark(i)->GetIcon());
				icon->SetSize(16,16);
				icon->ScaleTo(16,16);
			}
			m_bookmarksmenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
		}
	}
}

void kGUIBrowseObj::DoBookmarks(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		int sel;

		sel=m_bookmarksmenu.GetSelection();
		switch(sel)
		{
		case -1:
			/* menu aborted */
		break;
		case 0:	/* bookmark current page */
		{
			PageInfo *p;
			
			p=m_pages+m_pageindex-1;
			if(p->GetURL()->GetLen())
				m_settings->AddBookmark(p->GetTitle(),p->GetURL(),&m_icon);
		}
		break;
		case 1:	/* edit bookmarks */
		{
			EditBookmarkWindow *ebw;

			ebw=new EditBookmarkWindow(m_settings);
		}
		break;
		default:
		{
			/* goto bookmark */
			kGUIBookmark *bookmark;
			PageInfo *p;

			SaveCurrent();

			bookmark=m_settings->GetBookmark(sel-2);

			p=NextPage();
			m_url.SetString(bookmark->GetURL());
			p->Set(bookmark->GetURL()->GetString(),0,0,0,0);
			p->SetScrollY(0);

			Load();
		}
		break;
		}
	}
}

kGUIBrowseObj::~kGUIBrowseObj()
{
}

/* return pointer to next page buffer, if full, then scroll buffers back */

PageInfo *kGUIBrowseObj::NextPage(void)
{
	int i;
	PageInfo *pi;

	if(m_pageindex==MAXPAGES)
	{
		/* scroll pages down and throw away oldest */
		for(i=0;i<(MAXPAGES-1);++i)
			m_pages[i].Copy(&m_pages[i+1]);

		/* use last available page */
		pi=&m_pages[MAXPAGES-1];
	}
	else
		pi=&m_pages[m_pageindex++];

	m_pageend=m_pageindex;
	UpdateButtons();
	return(pi);
}

void kGUIBrowseObj::SetSource(kGUIString *url,kGUIString *source,kGUIString *type,kGUIString *header)
{
	PageInfo *p=NextPage();

	p->SetScrollY(0);

	if(source)
	{
		p->Set(url->GetString(),0,0,source->GetString(),header?header->GetString():0);
		Goto();
	}
	else
	{
		p->Set(url->GetString(),0,0,0,0);
		p->SetScrollY(0);

		m_type.SetString(type);
		m_header.SetString(header);
		m_url.SetString(url);
		m_referer.SetString(url);

		Load();
	}
}

void kGUIBrowseObj::UpdateButtons(void)
{
	m_back.SetEnabled(m_pageindex>1);
	m_refresh.SetEnabled(m_pageindex>0);
	m_refresh2.SetEnabled(m_pageindex>0);
	m_print.SetEnabled(m_pageindex>0);
	m_forward.SetEnabled(m_pageindex<m_pageend);
}

void kGUIBrowseObj::Goto(void)
{
	PageInfo *p=m_pages+m_pageindex-1;
	const char *cp;
	kGUIString llname;

	m_url.SetString(p->GetURL());
	m_referer.SetString(p->GetReferer());
	m_post.SetString(p->GetPost());
	m_source.SetString(p->GetSource());
	m_type.SetString(p->GetType());
	m_header.SetString(p->GetHeader());
	if(p->GetSecure()==true)
		m_lock.SetImage(&m_lockedimage);
	else
		m_lock.SetImage(&m_unlockedimage);

	kGUI::SetMouseCursor(MOUSECURSOR_BUSY);

	/* is there a local link appended to the URL? */
	cp=strstr(m_url.GetString(),"#");
	if(cp)
		llname.SetString(cp+1);

	m_screenpage.SetTarget(&llname);
	m_screenpage.SetMedia(GetSettings()->GetScreenMedia());
	m_screenpage.SetSource(&m_url,&m_source,&m_type,&m_header);
	m_screenpage.LoadInput(p->GetInput());			/* overwrite any form inputs with previous input */
	m_debug.SetString(m_screenpage.GetDebug());

	/* only if window currently has vertical scrollbars */
	/* is there a local link appended to the URL? */
	if(llname.GetLen() && m_screenpage.GetHasVertScrollBars()==true)
	{
		kGUIObj *topobj;
		int currenty;

		topobj=m_screenpage.LocateLocalLink(&llname);
		if(topobj)
		{
			kGUICorners c1;
			kGUICorners c2;

			/* scroll down to the top object */
			topobj->GetCorners(&c2);
			m_screenpage.GetCorners(&c1);

			currenty=m_screenpage.GetScrollY();
			m_screenpage.SetScrollY(currenty+(c2.ty-c1.ty));
		}
	}
	else
		m_screenpage.SetScrollY(p->GetScrollY());
	m_screenpage.Dirty();

	/* save title of page so if user right clicks on go forward or goback buttons then ir shows title instead of URL */
	if(m_screenpage.GetTitle()->GetLen())
		p->SetTitle(m_screenpage.GetTitle());

	/* tell window to change title */
	m_pagechangedcallback.Call();

	kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
}

void kGUIBrowseObj::SaveCurrent(void)
{
	PageInfo *p;
	kGUIString input;

	/* save old scroll position and save user input on the page */
	p=m_pages+(m_pageindex-1);
	p->SetScrollY(m_screenpage.GetScrollY());
	if(p->GetInput())
		m_screenpage.SaveInput(p->GetInput());
}

void kGUIBrowseObj::Click(kGUIString *url,kGUIString *referrer,kGUIString *post)
{
	PageInfo *p;
	SaveCurrent();

	/* NOTE: referrer and post can both be null! */

	if(referrer)
		m_referer.SetString(referrer);
	else
		m_referer.SetString(m_url.GetString());

	m_url.SetString(url);
	if(post)
		m_post.SetString(post);
	else
		m_post.Clear();

	p=NextPage();
	p->Set(m_url.GetString(),m_post.GetString(),m_referer.GetString(),0,0);
	p->SetScrollY(0);
	p->GetInput()->Reset();

	Load();
}

void kGUIBrowseObj::Load(void)
{
	kGUIString url;
	const char *cp;

	m_iconvalid=false;
	m_url.SetIcon(0);	/* hide old URL icon until new one can be loaded */
	if(m_dl.GetAsyncActive()==true)
	{
		m_dl.Abort();
		m_dl.WaitFinished();
	}

	/* is there a partial page offet? ie: "page.html#aaaaa" */ 
	url.SetString(&m_url);
	cp=strstr(url.GetString(),"#");
	if(cp)
		url.Clip((int)(cp-url.GetString()));

	if(!strcmpin(url.GetString(),"file://",7))
	{
		int load;

		m_dh.SetFilename(url.GetString()+7);
		m_dl.SetRedirectURL(0);

		if(m_dh.Open()==true)
		{
			load=DOWNLOAD_OK;
			m_dh.Close();
		}
		else
			load=DOWNLOAD_ERROR;
		PageLoaded(load);
	}
	else
	{
		m_screenpage.SetStatusLine(0);	/* stop page from overwriting */
		m_status.SetString("Loading");
		m_busyimage.SetAnimate(true);

		/* todo: have browser setting for not sending this along */
		m_dl.SetReferer(m_referer.GetString());
//		m_dl.SetReferer("");
		m_dl.SetPostData(&m_post);
		m_dh.SetMemory();
		m_dl.SetAuthHandler(&m_ah);
		m_dl.AsyncDownLoad(&m_dh,&url,this,CALLBACKNAME(PageLoaded));
	}
}

/* show popup listing all previous pages we can go back to */

void kGUIBrowseObj::GoBackMenu(void)
{
	int i;
	int j;
	PageInfo *p;

	if(m_pageindex<2)
		return;	/* nowhere to go forward to */

	m_gomenu.SetNumEntries(m_pageindex-1);
	j=m_pageindex-2;
	for(i=1;i<m_pageindex;++i)
	{
		p=m_pages+j;
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
	PageInfo *p;

	num=m_pageend-m_pageindex;
	if(!num)
		return;	/* nowhere to go forward to */

	m_gomenu.SetNumEntries(num);
	j=0;
	for(i=m_pageindex;i<m_pageend;++i)
	{
		p=m_pages+i;
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
			m_pageindex=sel+1;
			UpdateButtons();
			Goto();
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
		SaveCurrent();

		if(m_pageindex<m_pageend)
		{
			++m_pageindex;
			UpdateButtons();
			Goto();
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
		SaveCurrent();

		if(m_pageindex>1)
		{
			--m_pageindex;
			UpdateButtons();
			Goto();
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

		/* is there a local link appended to the URL? */
		cp=strstr(m_url.GetString(),"#");
		if(cp)
			llname.SetString(cp+1);

		m_printpage.SetTarget(&llname);
		m_printpage.SetMedia(GetSettings()->GetPrintMedia());
		m_printpage.SetSource(&m_url,&m_source,&m_type,&m_header);

		/* todo: copy current input values on forms from screenpage to printpage */

		m_printpage.Preview();
	}
}

void kGUIBrowseObj::UrlChanged(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ENTER:
		m_url.SelectAll();
	break;
	case EVENT_PRESSRETURN:
	{
		PageInfo *p;

		SaveCurrent();
		p=NextPage();

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
		p->Set(m_url.GetString(),0,m_referer.GetString(),0,0);
		p->SetScrollY(0);

		Load();
	}
	break;
	}
}

void kGUIBrowseObj::PageLoaded(int result)
{
	PageInfo *p=m_pages+(m_pageindex-1);

	m_busyimage.SetAnimate(false);
	m_status.SetString("Done");
	m_screenpage.SetStatusLine(&m_status);	/* allow writing again... */

	if(result==DOWNLOAD_OK && m_dh.GetSize())
	{
		m_dh.Open();
		m_dh.Read(&m_source,m_dh.GetSize());
		m_dh.Close();

		p->SetSource(&m_source);
		p->SetType(m_dl.GetEncoding());
		p->SetHeader(m_dl.GetHeader());
		p->SetSecure(m_dl.GetSecure());

		/* if this was redirected then get the new redirected URL */
		if(m_dl.GetRedirectURL()->GetLen())
		{
			p->SetURL(m_dl.GetRedirectURL());
			m_url.SetString(m_dl.GetRedirectURL());
		}
		Goto();
	}
	else
	{
		if(m_dl.GetReturnCode()==401)
		{
			/* bring up authentication input box */
			AuthenticateWindow *aw;
			kGUIString aa;
			kGUIString domain;
			kGUIString path;
			kGUIString realm;
			const char *cp;

			//WWW-Authenticate: Basic realm="File Download Authorization"
			m_dl.ExtractFromHeader("WWW-Authenticate:",&aa);
			cp=strstri(aa.GetString(),"realm=");
			if(cp)
			{
				realm.SetString(cp+6);
				realm.Trim(TRIM_SPACE|TRIM_TAB|TRIM_CR|TRIM_QUOTES);

				kGUI::ExtractURL(&m_url,&domain,&path);
				domain.Replace("http://","");
				aw=new AuthenticateWindow(this,&realm,&domain);
			}
		}
		else
			ShowError();
	}
}

void kGUIBrowseObj::ShowError(void)
{
	PageInfo *p=m_pages+(m_pageindex-1);

	/* did we get an error string from the server? */
	if(m_dl.GetErrorPage()->GetLen())
	{
		m_source.SetString(m_dl.GetErrorPage());
		p->SetType(m_dl.GetEncoding());
		p->SetHeader(m_dl.GetHeader());
	}
	else
	{
		kGUIString type;
		kGUIString header;

		/* put up an error page */
		m_source.Sprintf("<HTML><HEAD><TITLE>Error %d loading page '%s'</TITLE></HEAD><BODY><H1>Error %d loading page '%s'</H1><BR></BODY></HTML>",m_dl.GetReturnCode(),m_url.GetString(),m_dl.GetReturnCode(),m_url.GetString());
		type.SetString("text/html");

		p->SetType(&type);
		p->SetHeader(&header);
	}
	p->SetSource(&m_source);
	Goto();
}


void kGUIBrowseObj::CancelAuthenticate()
{
	ShowError();
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
		m_screenpage.FlushCurrentMedia();
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

	m_visiteddayscaption.SetFontID(1);
	m_visiteddayscaption.SetFontSize(VSFONTSIZE);
	m_visiteddayscaption.SetString("Save visited links for");

	m_visiteddays.SetFontSize(VSFONTSIZE);
	m_visiteddays.SetInputType(GUIINPUTTYPE_INT);
	m_visiteddays.SetSize(50,m_visiteddays.GetHeight()+6);

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
	m_cachemode.SetEntry(0,"Cache downloaded files",0);
	m_cachemode.SetEntry(1,"Cache downloaded files, delete at end of each session",1);
	m_cachemode.SetEntry(2,"Don't cache downloaded files",2);
	m_cachemode.SetSize(m_cachemode.GetWidest(),m_visiteddayscaption2.GetHeight()+6);

	m_controls.AddObject(&m_cachemode);
	m_controls.NextLine();

	m_cachesizecaption.SetFontID(1);
	m_cachesizecaption.SetFontSize(VSFONTSIZE);
	m_cachesizecaption.SetString("Use up to");

	m_cachesize.SetFontSize(VSFONTSIZE);
	m_cachesize.SetInputType(GUIINPUTTYPE_INT);
	m_cachesize.SetSize(75,m_cachesize.GetHeight()+6);

	m_cachesizecaption2.SetFontID(1);
	m_cachesizecaption2.SetFontSize(VSFONTSIZE);
	m_cachesizecaption2.SetString("MB for downloaded file cache");

	m_controls.AddObject(&m_cachesizecaption);
	m_controls.AddObject(&m_cachesize);
	m_controls.AddObject(&m_cachesizecaption2);
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

	m_screenmedia.SetSize(m_screenmedia.GetWidest(),m_screenmediacaption.GetHeight()+6);
	m_controls.AddObject(&m_screenmedia);
	m_controls.NextLine();

	m_printmediacaption.SetFontID(1);
	m_printmediacaption.SetFontSize(VSFONTSIZE);
	m_printmediacaption.SetString("Print Media");
	m_controls.AddObject(&m_printmediacaption);

	m_printmedia.SetSize(m_printmedia.GetWidest(),m_printmediacaption.GetHeight()+6);
	m_controls.AddObject(&m_printmedia);
	m_controls.NextLine();

	/* set values to current settings */
	m_visiteddays.SetInt(b->GetSettings()->GetVisitedDays());
	m_cachemode.SetSelection(b->GetSettings()->GetCacheMode());
	m_cachesize.SetInt(b->GetSettings()->GetCacheSize());
	m_usercss.SetString(b->GetSettings()->GetUserCSS());
	m_usecss.SetSelected(b->GetSettings()->GetUseCSS());
	m_useusercss.SetSelected(b->GetSettings()->GetUseUserCSS());
	m_drawbox.SetSelected(b->GetSettings()->GetDrawBoxes());
	m_drawareas.SetSelected(b->GetSettings()->GetDrawAreas());
	m_screenmedia.SetSelectionz(b->GetSettings()->GetScreenMedia()->GetString());
	m_printmedia.SetSelectionz(b->GetSettings()->GetPrintMedia()->GetString());

	AddObject(&m_controls);
	kGUI::AddWindow(this);
	GetTitle()->SetString("Browser Settings");
	SetEventHandler(this,CALLBACKNAME(WindowEvent));
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
		m_b->GetSettings()->SetUserCSS(&m_usercss);
		m_b->GetSettings()->SetUseCSS(m_usecss.GetSelected());
		m_b->GetSettings()->SetUseUserCSS(m_useusercss.GetSelected());
		m_b->GetSettings()->SetDrawBoxes(m_drawbox.GetSelected());
		m_b->GetSettings()->SetDrawAreas(m_drawareas.GetSelected());
		m_b->GetSettings()->SetScreenMedia(m_screenmedia.GetSelectionStringObj());
		m_b->GetSettings()->SetPrintMedia(m_printmedia.GetSelectionStringObj());

		/* trigger redraw, and flush rule cache since user rules may have been added */
		m_b->FlushRuleCache();
		m_b->RePosition(true);
		delete this;
	break;
	}
}

/* called when the window size changes */	
void ViewSettings::DirtyandCalcChildZone(void)
{
	kGUIContainerObj::DirtyandCalcChildZone();
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

	group=new kGUIXMLItem();
	group->SetName("browsersettings");
	root->AddChild(group);

	group->AddChild("visiteddays",m_visiteddays);
	group->AddChild("cachemode",m_cachemode);
	group->AddChild("cachesize",m_cachesize);
	group->AddChild("screenmedia",&m_screenmedia);
	group->AddChild("printmedia",&m_printmedia);

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
		/* draw the icon */
		if(m_iconvalid)
			m_icon.Draw(m_currentframe,c.lx+2+1,c.ty+2+1);
	}
	kGUI::PopClip();
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
				kGUI::AddEvent(this,CALLBACKNAME(Animate));
			}
		}
	}
	if(m_iconvalid)
		SetXOffset(16+2);
	else
		SetXOffset(0);
	Dirty();
	return(m_iconvalid);
}

void kGUIOffsetInputBoxObj::Animate(void)
{
	if( (m_icon.GetNumFrames()<1))
	{
		m_animateeventactive=false;
		kGUI::DelEvent(this,CALLBACKNAME(Animate));
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

AuthenticateWindow::AuthenticateWindow(kGUIBrowseObj *browse,kGUIString *realm,kGUIString *domain)
{
	m_browse=browse;

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
			delete this;
	break;
	case EVENT_PRESSED:
		if(event->GetObj()==&m_ok)
		{
			m_browse->Authenticate(&m_domrealm,&m_name,&m_password);
			delete this;
		}
		else if(event->GetObj()==&m_cancel)
		{
			m_browse->CancelAuthenticate();
			delete this;
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

	m_up.SetSize(100,30);
	m_up.SetString("Up");
	m_up.SetEventHandler(this,CALLBACKNAME(Up));
	m_controls.AddObject(&m_up);

	m_down.SetSize(100,30);
	m_down.SetString("Down");
	m_down.SetEventHandler(this,CALLBACKNAME(Down));
	m_controls.AddObject(&m_down);

//	m_import.SetSize(100,30);
//	m_import.SetString(gs(TXT_IMPORTBUTTON));
//	m_import.SetEventHandler(this,CALLBACKNAME(Import));
//	m_controls.AddObject(&m_import);

//	m_export.SetSize(100,30);
//	m_export.SetString(gs(TXT_EXPORTBUTTON));
//	m_export.SetEventHandler(this,CALLBACKNAME(Export));
//	m_controls.AddObject(&m_export);

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

	m_table.SetSize(min(m_table.CalcTableWidth(),m_window.GetChildZoneW()),min(500,kGUI::GetScreenHeight()));
	m_controls.AddObject(&m_table);
	UpdateButtons();

	m_window.AddObject(&m_controls);

	m_window.Shrink();
	m_window.Center();
	kGUI::AddWindow(&m_window);
}

EditBookmarkWindow::~EditBookmarkWindow()
{
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

	m_table.SetSize(min(m_table.CalcTableWidth(),m_window.GetChildZoneW()),min(500,kGUI::GetScreenHeight()));
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
