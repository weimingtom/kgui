#ifndef __KGUIBROWSE__
#define __KGUIBROWSE__

#include <new>
#include "kguicookies.h"
#include "kguixml.h"
#include "kguihtml.h"

/* number of pages that you can go back */
#define MAXPAGES 100

//local only classes
namespace kguibrowselocal
{

class HTMLPageObj : public kGUIHTMLPageObj
{
public:
	HTMLPageObj() {m_colormode=0;}
	void SetColorMode(int mode) {m_colormode=mode;}
	int GetColorMode(void) {return m_colormode;}
	/* override this so we can ( if enabled ) use our color blind simulator */
	void Draw(void);
private:
	int m_colormode;
};

/*! @internal @class HistoryRecord 
	@brief info for a page in the browser 
    @ingroup kGUIBrowseObj */
class HistoryRecord
{
public:
	HistoryRecord() {m_secure=false;m_input=new Hash();m_input->Init(8,0);}
	~HistoryRecord() {delete m_input;}
	void Set(const char *url,const char *post,const char *referer,const char *source,const char *header);
	void SetSource(kGUIString *source) {m_source.SetString(source);}
	void SetType(kGUIString *type) {m_type.SetString(type);}
	void SetTitle(kGUIString *title) {m_title.SetString(title);}
	void SetURL(kGUIString *url) {m_url.SetString(url);}
	void SetHeader(kGUIString *header) {m_header.SetString(header);}
	void SetPost(kGUIString *post) {post==0?m_post.Clear():m_post.SetString(post);}
	void SetReferer(kGUIString *referer) {referer==0?m_referer.Clear():m_referer.SetString(referer);}
	void SetSecure(bool s) {m_secure=s;}
	void Copy(HistoryRecord *copy) {m_scrolly=copy->m_scrolly;m_title.SetString(&copy->m_title);m_url.SetString(&copy->m_url);m_type.SetString(&copy->m_type);m_referer.SetString(&copy->m_referer);m_post.SetString(&copy->m_post);m_source.SetString(&copy->m_source);m_header.SetString(&copy->m_header);if(m_input)delete m_input;m_input=copy->m_input;copy->m_input=new Hash();copy->m_input->Init(8,0);m_secure=copy->m_secure;};

	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
	kGUIString *GetPost(void) {return &m_post;}
	kGUIString *GetReferer(void) {return &m_referer;}
	kGUIString *GetSource(void) {return &m_source;}
	kGUIString *GetType(void) {return &m_type;}
	kGUIString *GetHeader(void) {return &m_header;}
	bool GetSecure(void) {return m_secure;}
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
	bool m_secure:1;	
	Hash *m_input;
};

#if 0
/* used to report info about a page, for example */
/* page wants to set a cookie, allow or cancel */
/* page wants to forward to a new url, allow or cancel */
class TabAlertObj: public kGUIObj
{
public:
	TabAlertObj() {SetZoneH(m_text.GetFontHeight()+4);}
	~TabAlertObj() {}
	void Refresh(kGUIString *newurl) {m_url.SetString(newurl);m_text.SetString(newurl);}
	void Draw(void);
	bool UpdateInput(void) {return false;}
private:
	kGUIText m_text;
	kGUIString m_url;
};
#endif

/* object needed for each tab */
class TabRecord
{
public:
	TabRecord();
	HistoryRecord *NextPage(void);
	HistoryRecord *GetHist(int index) {return m_history+index;}
	HistoryRecord *GetCurHist(void) {return m_histindex==0?0:m_history+m_histindex-1;}
	HTMLPageObj *GetScreenPage(void) {return &m_screenpage;}
	void SetIcon(bool valid,DataHandle *dh) {m_iconvalid=valid;m_icon.Copy(dh);}
	void GetIcon(bool *valid,DataHandle *dh) {*(valid)=m_iconvalid;dh->Copy(&m_icon);}
	void ReSize(int neww,int newh);
#if 0
	TabAlertObj *AddAlert(void);
	void DelAlert(TabAlertObj *dela);
#endif
	int m_histindex;	
	int m_histend;
	HistoryRecord m_history[MAXPAGES];
	kGUIString m_urlstring;	/* current string in the URL title bar */
	HTMLPageObj m_screenpage;
	bool m_iconvalid:1;
	bool m_reposition:1;
	bool m_reparse:1;
	DataHandle m_icon;
	class DownloadPageRecord *m_curdl;	/* if not null then download page is active */
#if 0
	unsigned int m_numalerts;
	ClassArray<TabAlertObj>m_alerts;
#endif
};

class DownloadPageRecord
{
public:
	TabRecord *m_tabrecord;
	DataHandle m_dh;
	kGUIDownloadEntry m_dl;
};

}	//end of namespace

using namespace kguibrowselocal;

/* this is seperate so a program can load/save it into it's own config file and then */
/* attach it to each browse object when they are created */

class kGUIBookmark
{
public:
	void SetTitle(kGUIString *t) {m_title.SetString(t);}
	void SetURL(kGUIString *t) {m_url.SetString(t);}
	void SetIcon(DataHandle *from) {m_icon.Copy(from);}
	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
	kGUIImage *GetIcon(void) {return &m_icon;}
private:
	kGUIString m_url;
	kGUIString m_title;
	kGUIImage m_icon;
};

//save tabs when quitting?
enum
{
SAVEMODE_NO,
SAVEMODE_YES,
SAVEMODE_YESMORE1};

class kGUIBrowseSettings : public kGUIHTMLSettings
{
public:
	kGUIBrowseSettings() {m_visitedcache=0;m_itemcache=0;m_visiteddays=30;m_cachemode=CACHEMODE_SAVE;m_cachesize=50;m_numbookmarks=0;m_bookmarks.Init(64,32);m_screenmedia.SetString("screen");m_printmedia.SetString("print");m_savemode=SAVEMODE_YESMORE1;m_numtabs=0;m_tabs.Init(4,4);}
	void SetVisitedCache(kGUIHTMLVisitedCache *visitedcache) {m_visitedcache=visitedcache;}
	kGUIHTMLVisitedCache *GetVisitedCache(void) {return m_visitedcache;}
	void SetItemCache(kGUIHTMLItemCache *itemcache) {m_itemcache=itemcache;}
	kGUIHTMLItemCache *GetItemCache(void) {return m_itemcache;}

	void SetVisitedDays(unsigned int vd) {m_visiteddays=vd;m_visitedcache->SetNumDays(vd);}
	unsigned int GetVisitedDays(void) {return m_visiteddays;}

	//mode for savetabs when quiting
	void SetSaveMode(unsigned int cm) {m_savemode=cm;}
	unsigned int GetSaveMode(void) {return m_savemode;}
	void LoadTabs(class kGUIBrowseObj *b);
	void SaveTabs(class kGUIBrowseObj *b);

	void SetCacheMode(unsigned int cm) {m_cachemode=cm;m_itemcache->SetMode(cm);}
	unsigned int GetCacheMode(void) {return m_cachemode;}

	void SetCacheSize(unsigned int cs) {m_cachesize=cs;m_itemcache->SetMaxSize(cs);}
	unsigned int GetCacheSize(void) {return m_cachesize;}

	unsigned int GetNumBookmarks(void) {return m_numbookmarks;}
	void SetNumBookmarks(unsigned int n) {m_numbookmarks=n;}
	kGUIBookmark *GetBookmark(unsigned int index) {return m_bookmarks.GetEntryPtr(index);}
	void AddBookmark(kGUIString *title,kGUIString *url,DataHandle *icon);
	void UpdateBookmark(unsigned int index,kGUIString *title,kGUIString *url,DataHandle *icon);

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
	unsigned int m_savemode;
	kGUIString m_screenmedia;
	kGUIString m_printmedia;
	ClassArray<kGUIBookmark>m_bookmarks;
	unsigned int m_numtabs;
	ClassArray<kGUIString>m_tabs;
};

class kGUIBrowseIcon
{
public:
	kGUIBrowseIcon() {m_valid=false;m_animateeventactive=false;}
	virtual ~kGUIBrowseIcon() {if(m_animateeventactive)kGUI::DelUpdateTask(this,CALLBACKNAME(Animate));}
	bool GetIsValid(void) {return m_valid;}
	bool SetIcon(DataHandle *dh);
	void Draw(int x,int y) {m_icon.Draw(m_currentframe,x,y);}
	virtual void Dirty(void)=0;
private:
	CALLBACKGLUE(kGUIBrowseIcon,Animate);
	void Animate(void);
	unsigned int m_currentframe;
	unsigned int m_animdelay;
	bool m_animateeventactive:1;
	bool m_valid:1;
	kGUIImage m_icon;
};

/* used to input the URL but draws the input text shifted to the right to allow space */
/* for the favicon shape */
class kGUIOffsetInputBoxObj : public kGUIInputBoxObj
{
public:
	kGUIOffsetInputBoxObj();
	void Draw(void);
	bool SetIcon(DataHandle *dh);
	CALLBACKGLUEPTR(kGUIOffsetInputBoxObj,SetIcon,DataHandle);
private:
	CALLBACKGLUE(kGUIOffsetInputBoxObj,Animate);
	void Changed(void);
	void Animate(void);
	void SetOffsets();
	unsigned int m_currentframe;
	unsigned int m_animdelay;
	bool m_animateeventactive:1;
	bool m_iconvalid:1;
	bool m_inchange:1;
	kGUIImage m_icon;
};

/* used to draw the link under the cursor, hilights the domain in blue */
class kGUIDomainTextObj : public kGUITextObj
{
public:
	kGUIDomainTextObj();
	void Draw(void);
private:
	void Changed(void);
	bool m_inchange;
};

/* we will make our own tab class since we want the tabs to have icons and close button on them */
#define BROWSETABWIDTH 175

class kGUITabBrowseIcon : public kGUIBrowseIcon
{
public:
	void SetTab(class kGUIBrowseTabObj *tabobj,unsigned int tabnum) {m_tabobj=tabobj;m_tabnum=tabnum;}
	void Dirty(void);
private:
	unsigned int m_tabnum;
	class kGUIBrowseTabObj *m_tabobj;
};

class kGUIBrowseTabObj : public kGUITabObj
{
public:
	kGUIBrowseTabObj() {m_icons.Init(4,4);}
	void SetNumTabs(int n) {kGUITabObj::SetNumTabs(n);SetHideTabs(n<2);}
	unsigned int GetTabWidth(int index) {return BROWSETABWIDTH;}
	void SetIcon(int tabindex,DataHandle *dh) {m_icons.GetEntryPtr(tabindex)->SetIcon(dh);m_icons.GetEntryPtr(tabindex)->SetTab(this,tabindex);DirtyTab(tabindex);}
	void DrawTab(int tabindex,kGUIText *text,int x,int y);
private:
	ClassArray<kGUITabBrowseIcon>m_icons;
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

	void SetSaveDirectory(const char *dir) {m_curscreenpage->SetSaveDirectory(dir);}
	const char *GetSaveDirectory(void) {return m_curscreenpage->GetSaveDirectory();}

	unsigned int GetTabNum(TabRecord *r);

	HistoryRecord *GetCurHist(void) {return m_tabrecords.GetEntryPtr(m_curtab)->GetCurHist();}

	unsigned int GetNumTabs(void) {return m_numtabs;}
	void CopyTabURL(unsigned int n,kGUIString *url);

	kGUIString *GetTitle(void) {return m_curscreenpage->GetTitle();}
	void SetSource(kGUIString *url,kGUIString *source,kGUIString *type,kGUIString *header);
	HistoryRecord *NextPage(unsigned int tabnum);
	void RePosition(bool rp);

	void SetPageChangedCallback(void *codeobj,void (*code)(void *)) {m_pagechangedcallback.Set(codeobj,code);}
	void SetSettingsChangedCallback(void *codeobj,void (*code)(void *)) {m_settingschangedcallback.Set(codeobj,code);}

	/* default printer to use */
	void SetPID(int pid) {m_printpage.SetPID(pid);}
	int GetPID(void) {return m_printpage.GetPID();}

	/* attach plugins */
	void AddPlugin(kGUIHTMLPluginObj *obj) {m_plugins.AddPlugin(obj);}

	/* set the color mode */
	void SetColorMode(int mode) {m_curscreenpage->SetColorMode(mode);}
	int GetColorMode(void) {return m_curscreenpage->GetColorMode();}

	kGUIBrowseSettings *GetSettings(void) {return m_settings;}
	void FlushRuleCache(void) {m_curscreenpage->FlushRuleCache();m_printpage.FlushRuleCache();}
	void ShowError(DownloadPageRecord *dle);
	void Authenticate(kGUIString *domrealm,kGUIString *name,kGUIString *password);

	void NewTab(void);
	void SettingsChanged(void) {m_settingschangedcallback.Call();}
private:
	void SaveAs(kGUIFileReq *req,int closebutton);
	void AskOverwrite(int closebutton);
	void DoSaveAs(void);

	CALLBACKGLUEPTRVAL(kGUIBrowseObj,SaveAs,kGUIFileReq,int)
	CALLBACKGLUEVAL(kGUIBrowseObj,AskOverwrite,int)

	CALLBACKGLUEPTR(kGUIBrowseObj,UrlChanged,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,SearchChanged,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,Refresh,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,RefreshAll,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,GoForward,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,GoBack,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,Print,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,Click,kGUIHTMLClickInfo);
	CALLBACKGLUEPTR(kGUIBrowseObj,DoMainMenu,kGUIEvent);
	CALLBACKGLUEPTR(kGUIBrowseObj,DoGotoMenu,kGUIEvent);
	CALLBACKGLUEPTRPTR(kGUIBrowseObj,SetIcon,kGUIHTMLPageObj,DataHandle);
	CALLBACKGLUEPTR(kGUIBrowseObj,TabChanged,kGUIEvent);
	CALLBACKGLUE(kGUIBrowseObj,Update);
	void UpdateButtons(void);
	void UpdateButtons2(void);
	void CloseTab(void);
	void InitTabRecord(TabRecord *tr);
	void Goto(unsigned int tabnum);
	void Load(unsigned int tabnum);
	void StopLoad(unsigned int tabnum);
	void SetIcon(kGUIHTMLPageObj *page,DataHandle *dh);
	void DoMainMenu(kGUIEvent *event);
	void DoGotoMenu(kGUIEvent *event);
	void UrlChanged(kGUIEvent *event);
	void SearchChanged(kGUIEvent *event);
	void TabChanged(kGUIEvent *event);
	void PageLoaded(DownloadPageRecord *dle,int result);
	void GoForward(kGUIEvent *event);
	void GoBack(kGUIEvent *event);
	void GoForwardMenu(void);
	void GoBackMenu(void);
	void Print(kGUIEvent *event);
	void Refresh(kGUIEvent *event);
	void RefreshAll(kGUIEvent *event);
	void SaveCurrent(unsigned int tabnum);
	void Click(kGUIHTMLClickInfo *info);
	void Update(void);

	kGUIBrowseSettings *m_settings;

	bool m_showheader;
	kGUIControlBoxObj m_browsecontrols;

	kGUIMenuObj m_mainmenu;
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

	kGUIImage m_lockedimage;
	kGUIImage m_unlockedimage;
	kGUIImageRefObj m_lock;

	kGUITextObj m_urlcaption;
	kGUIOffsetInputBoxObj m_url;

	kGUITextObj m_searchcaption;
	kGUIOffsetInputBoxObj m_search;

	kGUIImageObj m_busyimage;

	kGUITextObj m_statuscaption;
	kGUITextObj m_status;

	kGUITextObj m_linkcaption;
	kGUIDomainTextObj m_linkurl;

	//we made out own class so we can insert the color blind simulator
	unsigned int m_curtab;
	unsigned int m_numtabs;
	kGUIBrowseTabObj m_tabs;
	HTMLPageObj *m_curscreenpage;
	kGUIHTMLPageObj m_printpage;

	kGUIInputBoxObj m_debug;

	volatile unsigned int m_numdlactive;
	Array<DownloadPageRecord *>m_dlactive;

	kGUIDownloadAuthenticateRealms m_ah;

	kGUICallBack m_pagechangedcallback;	/* used to tell parent that the window title changed */
	kGUICallBack m_settingschangedcallback;	/* used to tell parent that settings have changed so it can re-save config */
	/************************************************************/
	kGUIHTMLPluginGroupObj m_plugins;
	ClassArray<TabRecord>m_tabrecords;
	int m_pid;
	bool m_iconvalid;
	DataHandle m_icon;
	kGUIString m_savefn;
};

/*! @internal @class AuthenticateWindow 
	@brief Internal class used by the kGUIBrowseObj class.
	Popup window for entering users name & password for webpage authentication. 
    @ingroup kGUIBrowseObj */
class AuthenticateWindow
{
public:
	AuthenticateWindow(kGUIBrowseObj *browse,DownloadPageRecord *dle,kGUIString *realm,kGUIString *domain);
private:
	CALLBACKGLUEPTR(AuthenticateWindow,Event,kGUIEvent);
	void Event(kGUIEvent *event);

	DownloadPageRecord *m_dle;
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
	EditBookmarkRow(kGUIString *title,kGUIString *url,kGUIImage *icon) {m_title.SetString(title);m_url.SetString(url);m_objectlist[0]=&m_icon;m_objectlist[1]=&m_title;m_icon.Copy(icon);m_icon.ScaleTo(16,16);}
	int GetNumObjects(void) {return 2;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 

	kGUIString *GetTitle(void) {return &m_title;}
	kGUIString *GetURL(void) {return &m_url;}
	kGUIImage *GetIcon(void) {return &m_icon;}
private:
	kGUIObj *m_objectlist[2];

	kGUIImageObj m_icon;
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
