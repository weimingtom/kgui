#ifndef __KGUIBROWSE__
#define __KGUIBROWSE__

#include "kguicookies.h"
#include "kguixml.h"
#include "kguihtml.h"
#include "kguimovie.h"

/* number of pages that you can go back */
#define MAXPAGES 100

/*! @internal @class PageInfo 
	@brief info for a page in the browser 
    @ingroup kGUIBrowseObj */
class PageInfo
{
public:
	PageInfo() {m_input=new Hash();m_input->Init(8,0);}
	~PageInfo() {delete m_input;}
	void Set(const char *url,const char *post,const char *referer,const char *source,const char *header);
	void SetSource(kGUIString *source) {m_source.SetString(source);}
	void SetType(kGUIString *type) {m_type.SetString(type);}
	void SetTitle(kGUIString *title) {m_title.SetString(title);}
	void SetURL(kGUIString *url) {m_url.SetString(url);}
	void SetHeader(kGUIString *header) {m_header.SetString(header);}
	void Copy(PageInfo *copy) {m_scrolly=copy->m_scrolly;m_title.SetString(&copy->m_title);m_url.SetString(&copy->m_url);m_type.SetString(&copy->m_type);m_referer.SetString(&copy->m_referer);m_post.SetString(&copy->m_post);m_source.SetString(&copy->m_source);m_header.SetString(&copy->m_header);if(m_input)delete m_input;m_input=copy->m_input;copy->m_input=new Hash();copy->m_input->Init(8,0);};

	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
	kGUIString *GetPost(void) {return &m_post;}
	kGUIString *GetReferer(void) {return &m_referer;}
	kGUIString *GetSource(void) {return &m_source;}
	kGUIString *GetType(void) {return &m_type;}
	kGUIString *GetHeader(void) {return &m_header;}
	Hash *GetInput(void) {return m_input;}
	void SetScrollY(int y) {m_scrolly=y;}
	int GetScrollY(void) {return m_scrolly;}
private:
	int m_scrolly;
	kGUIString m_title;
	kGUIString m_url;
	kGUIString m_post;
	kGUIString m_referer;
	kGUIString m_source;
	kGUIString m_type;
	kGUIString m_header;	/* we need this incase any stuff is linked in the header */
	
	Hash *m_input;
};

/* this is seperate so a program can load/save it into it's own config file and then */
/* attach it to each browse object when they are created */

class kGUIBookmark
{
public:
	void SetTitle(kGUIString *t) {m_title.SetString(t);}
	void SetURL(kGUIString *t) {m_url.SetString(t);}
	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
private:
	kGUIString m_url;
	kGUIString m_title;
};

class kGUIBrowseSettings : public kGUIHTMLSettings
{
public:
	kGUIBrowseSettings() {m_visitedcache=0;m_itemcache=0;m_visiteddays=30;m_cachemode=CACHEMODE_SAVE;m_cachesize=50;m_numbookmarks=0;m_bookmarks.Init(64,32);m_screenmedia.SetString("string");m_printmedia.SetString("print");}
	void SetVisitedCache(kGUIHTMLVisitedCache *visitedcache) {m_visitedcache=visitedcache;}
	kGUIHTMLVisitedCache *GetVisitedCache(void) {return m_visitedcache;}
	void SetItemCache(kGUIHTMLItemCache *itemcache) {m_itemcache=itemcache;}
	kGUIHTMLItemCache *GetItemCache(void) {return m_itemcache;}

	void SetVisitedDays(unsigned int vd) {m_visiteddays=vd;m_visitedcache->SetNumDays(vd);}
	unsigned int GetVisitedDays(void) {return m_visiteddays;}

	void SetCacheMode(unsigned int cm) {m_cachemode=cm;m_itemcache->SetMode(cm);}
	unsigned int GetCacheMode(void) {return m_cachemode;}

	void SetCacheSize(unsigned int cs) {m_cachesize=cs;m_itemcache->SetMaxSize(cs);}
	unsigned int GetCacheSize(void) {return m_cachesize;}

	unsigned int GetNumBookmarks(void) {return m_numbookmarks;}
	void SetNumBookmarks(unsigned int n) {m_numbookmarks=n;}
	kGUIBookmark *GetBookmark(unsigned int index) {return m_bookmarks.GetEntryPtr(index);}
	void AddBookmark(kGUIString *title,kGUIString *url);
	void UpdateBookmark(unsigned int index,kGUIString *title,kGUIString *url);

	void SetScreenMedia(kGUIString *m) {m_screenmedia.SetString(m);}
	kGUIString *GetScreenMedia(void) {return &m_screenmedia;}

	void SetPrintMedia(kGUIString *m) {m_printmedia.SetString(m);}
	kGUIString *GetPrintMedia(void) {return &m_printmedia;}

	void Load(kGUIXMLItem *root);
	void Save(kGUIXMLItem *root);
private:
	kGUIHTMLItemCache *m_itemcache;
	kGUIHTMLVisitedCache *m_visitedcache;

	unsigned int m_visiteddays;
	unsigned int m_cachemode;
	unsigned int m_cachesize;	/* in MB */
	unsigned int m_numbookmarks;
	kGUIString m_screenmedia;
	kGUIString m_printmedia;
	ClassArray<kGUIBookmark>m_bookmarks;
};

/* used to input the URL but draws the input text shifted to the right to allow space */
/* for the favicon shape */
class kGUIOffsetInputBoxObj : public kGUIInputBoxObj
{
public:
	kGUIOffsetInputBoxObj();
	void Draw(void);
	void SetIcon(DataHandle *dh);
	CALLBACKGLUEPTR(kGUIOffsetInputBoxObj,SetIcon,DataHandle);
private:
	CALLBACKGLUE(kGUIOffsetInputBoxObj,Animate);
	void Animate(void);
	unsigned int m_currentframe;
	unsigned int m_animdelay;
	bool m_animateeventactive:1;
	bool m_iconvalid:1;
	kGUIImage m_icon;
};

class kGUIBrowseObj : public kGUIContainerObj
{
public:
	kGUIBrowseObj(kGUIBrowseSettings *settings,int w,int h);
	~kGUIBrowseObj();

	/* use the whole thing! */
	void CalcChildZone(void) {SetChildZone(0,0,GetZoneW(),GetZoneH());}

	void Draw(void) {DrawC(0);}
	bool UpdateInput(void) {return UpdateInputC(0);}

	/* show URL and buttons? etc */
	void SetShowHeader(bool h) {m_showheader=h;}
	bool GetShowHeader(bool h) {return m_showheader;}

	/* default page to show, url="", source=passed source */
	void SetPageSource(kGUIString *s);

	void SetSaveDirectory(const char *dir) {m_page.SetSaveDirectory(dir);}
	const char *GetSaveDirectory(void) {return m_page.GetSaveDirectory();}

	void SetItemCache(kGUIHTMLItemCache *c) {m_page.SetItemCache(c);}
	void SetVisitedCache(kGUIHTMLVisitedCache *v) {m_page.SetVisitedCache(v);}

	kGUIString *GetTitle(void) {return m_page.GetTitle();}
	void SetSource(kGUIString *url,kGUIString *source,kGUIString *type,kGUIString *header);
	PageInfo *NextPage(void);
	void RePosition(bool rp) {m_page.SetSize(GetChildZoneW(),GetChildZoneH()-m_page.GetZoneY());m_page.RePosition(rp);}

	void SetPageChangedCallback(void *codeobj,void (*code)(void *)) {m_pagechangedcallback.Set(codeobj,code);}

	/* default printer to use */
	void SetPID(int pid) {m_page.SetPID(pid);}
	int GetPID(void) {return m_page.GetPID();}

	/* attach plugins */
	void AddPlugin(kGUIHTMLPluginObj *obj) {m_plugins.SetEntry(m_numplugins++,obj);m_page.AddPlugin(obj);}

	kGUIBrowseSettings *GetSettings(void) {return m_settings;}
	void FlushRuleCache(void) {m_page.FlushRuleCache();}
	void ShowError(void);
	void CancelAuthenticate();
	void Authenticate(kGUIString *domrealm,kGUIString *name,kGUIString *password);
private:
	CALLBACKGLUEPTR(kGUIBrowseObj,UrlChanged,kGUIEvent);
	CALLBACKGLUEVAL(kGUIBrowseObj,PageLoaded,int);
	CALLBACKGLUEPTR(kGUIBrowseObj,Refresh,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,RefreshAll,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,GoForward,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,GoBack,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,Print,kGUIEvent);
	CALLBACKGLUEPTRPTRPTR(kGUIBrowseObj,Click,kGUIString,kGUIString,kGUIString);
	CALLBACKGLUEPTR(kGUIBrowseObj,ShowMainMenu,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,ShowBookmarks,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,DoMainMenu,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,DoBookmarks,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,DoGotoMenu,kGUIEvent);
	void UpdateButtons(void);
	void Goto(void);
	void Load(void);
	void ShowMainMenu(kGUIEvent *event);
	void DoMainMenu(kGUIEvent *event);
	void ShowBookmarks(kGUIEvent *event);
	void DoBookmarks(kGUIEvent *event);
	void DoGotoMenu(kGUIEvent *event);
	void UrlChanged(kGUIEvent *event);
	void PageLoaded(int result);
	void GoForward(kGUIEvent *event);
	void GoBack(kGUIEvent *event);
	void GoForwardMenu(void);
	void GoBackMenu(void);
	void Print(kGUIEvent *event);
	void Refresh(kGUIEvent *event);
	void RefreshAll(kGUIEvent *event);
	void SaveCurrent(void);
	void Click(kGUIString *url,kGUIString *referrer,kGUIString *post);

	kGUIBrowseSettings *m_settings;

	bool m_showheader;
	kGUIControlBoxObj m_browsecontrols;

	kGUITextObj m_menulabel;
	kGUIMenuColObj m_menu;
	kGUIMenuColObj m_gomenu;

	kGUITextObj m_bookmarkslabel;
	kGUIMenuColObj m_bookmarksmenu;
	
	kGUIImage m_backimage;
	kGUIImage m_forwardimage;
	kGUIImage m_reloadimage;
	kGUIImage m_reloadimage2;
	kGUITextObj m_backcaption;
	kGUITextObj m_forwardcaption;
	kGUIButtonObj m_back;
	kGUIButtonObj m_forward;

	kGUITextObj m_refreshcaption;
	kGUIButtonObj m_refresh;

	kGUITextObj m_refresh2caption;
	kGUIButtonObj m_refresh2;

	kGUITextObj m_printcaption;
	kGUIButtonObj m_print;

	kGUITextObj m_urlcaption;
	kGUIOffsetInputBoxObj m_url;

	kGUIImageObj m_busyimage;

	kGUITextObj m_statuscaption;
	kGUIInputBoxObj m_status;

	kGUITextObj m_referercaption;
	kGUIInputBoxObj m_referer;

	kGUIString m_type;
	kGUIString m_header;
	kGUIString m_post;
	kGUITextObj m_linkcaption;
	kGUITextObj m_linkurl;

	kGUIHTMLPageObj m_page;
	kGUIInputBoxObj m_source;

	kGUIInputBoxObj m_debug;

	DataHandle m_dh;
	kGUIDownloadEntry m_dl;
	kGUIDownloadAuthenticateRealms m_ah;

	unsigned int m_numplugins;
	Array<kGUIHTMLPluginObj *>m_plugins;

	kGUICallBack m_pagechangedcallback;	/* used to tell parent that the window title changed */
	/************************************************************/
	int m_pageindex;	
	int m_pageend;
	PageInfo m_pages[MAXPAGES];
	int m_pid;
};

/*! @internal @class AuthenticateWindow 
	@brief Internal class used by the kGUIBrowseObj class.
	Popup window for entering users name & password for webpage authentication. 
    @ingroup kGUIBrowseObj */
class AuthenticateWindow
{
public:
	AuthenticateWindow(kGUIBrowseObj *browse,kGUIString *realm,kGUIString *domain);
private:
	CALLBACKGLUEPTR(AuthenticateWindow,Event,kGUIEvent);
	void Event(kGUIEvent *event);

	kGUIString m_domrealm;

	kGUIWindowObj m_window;
	kGUIControlBoxObj m_controls;
	kGUITextObj m_title;
	kGUITextObj m_namecaption;
	kGUITextObj m_passwordcaption;
	kGUIInputBoxObj m_name;
	kGUIInputBoxObj m_password;
	kGUIButtonObj m_ok;
	kGUIButtonObj m_cancel;
	kGUIBrowseObj *m_browse;
};

class EditBookmarkWindow
{
public:
	EditBookmarkWindow(kGUIBrowseSettings *settings);
	~EditBookmarkWindow();
private:
	CALLBACKGLUEPTR(EditBookmarkWindow,WindowEvent,kGUIEvent);
	CALLBACKGLUEPTR(EditBookmarkWindow,TableEvent,kGUIEvent);
	CALLBACKGLUEPTR(EditBookmarkWindow,Up,kGUIEvent);
	CALLBACKGLUEPTR(EditBookmarkWindow,Down,kGUIEvent);

	void WindowEvent(kGUIEvent *event);
	void TableEvent(kGUIEvent *event);
	void Up(kGUIEvent *event);
	void Down(kGUIEvent *event);

	void UpdateButtons(void);
	void TableChanged(void);

	kGUIBrowseSettings *m_settings;

	kGUIWindowObj m_window;
	kGUIControlBoxObj m_controls;
	kGUIButtonObj m_up;
	kGUIButtonObj m_down;
	kGUITableObj m_table;
};

class EditBookmarkRow : public kGUITableRowObj
{
public:
	EditBookmarkRow(kGUIString *title,kGUIString *url) {m_title.SetString(title);m_url.SetString(url);m_objectlist[0]=&m_title;}
	int GetNumObjects(void) {return 1;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 

	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
private:
	kGUIObj *m_objectlist[1];

	kGUITextObj m_title;
	kGUIString m_url;
};

class ViewCookieWindow
{
public:
	ViewCookieWindow(kGUIBrowseSettings *settings);
	~ViewCookieWindow();
private:
	CALLBACKGLUEPTR(ViewCookieWindow,WindowEvent,kGUIEvent);
	CALLBACKGLUEPTR(ViewCookieWindow,TableEvent,kGUIEvent);
	CALLBACKGLUEPTR(ViewCookieWindow,Block,kGUIEvent);

	void WindowEvent(kGUIEvent *event);
	void TableEvent(kGUIEvent *event);
	void Block(kGUIEvent *event);

	void UpdateButtons(void);
	void TableChanged(void);

	kGUIBrowseSettings *m_settings;

	kGUIWindowObj m_window;
	kGUIControlBoxObj m_controls;
	kGUIButtonObj m_block;
	kGUITableObj m_table;
};

class ViewCookieRow : public kGUITableRowObj
{
public:
	ViewCookieRow(kGUICookie *cookie);
	int GetNumObjects(void) {return 5;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 
	kGUICookie *GetCookie(void) {return m_cookie;}
private:
	kGUIObj *m_objectlist[5];

	kGUICookie *m_cookie;
	kGUITextObj m_domain;
	kGUITextObj m_name;
	kGUITextObj m_value;
	kGUITextObj m_path;
	kGUITextObj m_expiry;
};


#endif
