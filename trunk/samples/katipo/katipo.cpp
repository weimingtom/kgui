/*********************************************************************************/
/* Katipo - kGUI sample program                                                  */
/*                                                                               */
/* ( Black Widow ) web spidering program                                         */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguidl.h"
#include "kguireq.h"
#include "kguihtml.h"
#include "kguibrowse.h"
#include "kguixml.h"

#define APPNAME "Mandelbrot"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

enum
{
COL_STATUS,
COL_URL,
COL_REFERER,
COL_NUMCOLS};

class OutputTableRow : public kGUITableRowObj
{
public:
	OutputTableRow() {m_objectlist[COL_STATUS]=&m_status;m_objectlist[COL_URL]=&m_url;m_objectlist[COL_REFERER]=&m_referer;SetRowHeight(25);}
	int GetNumObjects(void) {return COL_NUMCOLS;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 
	void SetStatus(const char *s) {m_status.SetString(s);}
	void SetURL(const char *s) {m_url.SetString(s);}
	void SetReferer(const char *s) {m_referer.SetString(s);}
	kGUIString *GetURL(void) {return &m_url;}
//	kGUIString *GetStatus(void) {return &m_status;}
private:
	kGUIObj *m_objectlist[COL_NUMCOLS];
	kGUIInputBoxObj m_status;
	kGUIInputBoxObj m_url;
	kGUIInputBoxObj m_referer;
};

class StopTableRow : public kGUITableRowObj
{
public:
	StopTableRow() {m_objectlist[0]=&m_url;SetRowHeight(25);}
	int GetNumObjects(void) {return 1;}
	kGUIObj **GetObjectList(void) {return m_objectlist;} 
	void SetURL(const char *s) {m_url.SetString(s);}
	kGUIString *GetURL(void) {return &m_url;}
private:
	kGUIObj *m_objectlist[1];
	kGUIInputBoxObj m_url;
};

enum
{
STATUS_WAIT,
STATUS_LOADING
};

class Katipo
{
public:
	Katipo();
	~Katipo();
private:
	void Event(kGUIEvent *event);
	CALLBACKGLUEPTR(Katipo,Event,kGUIEvent)
	void StartScan(void);
	void Scan(void);
	void ScanHTML(kGUIHTMLObj *obj);
	CALLBACKGLUE(Katipo,Scan)
	void StopScan(void);
	void Load(kGUIFileReq *req,int pressed);
	void Save(kGUIFileReq *req,int pressed);
	CALLBACKGLUEPTRVAL(Katipo,Load,kGUIFileReq,int);
	CALLBACKGLUEPTRVAL(Katipo,Save,kGUIFileReq,int);

	//	kGUIBrowseObj *m_browse;
	kGUIBrowseSettings m_browsesettings;

	kGUIControlBoxObj m_layout;		/* layout control for other objects */

	kGUIMenuObj m_menu;
	kGUIMenuColObj m_filemenu;

	kGUITextObj m_urlcaption;
	kGUIInputBoxObj m_url;			/* filename to save movie to */

	kGUITextObj m_stopcaption;
	kGUITableObj m_stoptable;

	kGUIButtonObj m_startscan;
	kGUIButtonObj m_stopscan;

	kGUITableObj m_table;

	bool m_scanning;
	unsigned int m_scanrowindex;
	OutputTableRow *m_scanrow;
	int m_scanstatus;
	int m_scandelay;

	Hash m_scanhash;
	DataHandle m_scandh;
	kGUIDownloadEntry m_scandl;
	kGUIString m_scanstring;
	kGUIHTMLSettings m_htmlsettings;
	kGUIHTMLPageObj m_html;

	kGUIString m_base;
	kGUIString m_root;

	kGUITextObj m_help;
};

Katipo *g_Katipo;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
	kGUI::LoadFont("fontb.ttf");	/* use default bold font */
	kGUI::SetDefFontSize(20);
	kGUI::SetDefReportFontSize(20);

	kGUIXMLCODES::Init();

	g_Katipo=new Katipo();
}

void AppClose(void)
{
	delete g_Katipo;
	kGUIXMLCODES::Purge();
}

enum
{
MENU_LOAD,
MENU_SAVE,
MENU_QUIT
};

Katipo::Katipo()
{
	kGUIWindowObj *background;
	unsigned int bw,bh;
	unsigned int w,h;
	int bold=1;

	/* get pointer to the background window object */
	background=kGUI::GetBackground();
 	background->SetTitle("Katipo");

	bw=background->GetChildZoneW();
	bh=background->GetChildZoneH();

	m_menu.SetFontID(1);
	m_menu.SetNumEntries(1);
	m_menu.GetTitle(0)->SetString("File");
	m_menu.SetEntry(0,&m_filemenu);
	m_menu.SetEventHandler(this,CALLBACKNAME(Event));
	m_layout.AddObject(&m_menu);
	m_layout.NextLine();

	m_filemenu.SetIconWidth(22);

	m_filemenu.SetNumEntries(3);
	m_filemenu.SetEntry(0,"Load",MENU_LOAD);
	m_filemenu.SetEntry(1,"Save",MENU_SAVE);
	m_filemenu.SetEntry(2,"Quit",MENU_QUIT);


	/* set caption strings */
	m_urlcaption.SetString("Url");
	m_stopcaption.SetString("Stop");
	/* calc length of longest one */
	w=m_urlcaption.GetWidth();
	w=MAX(w,m_stopcaption.GetWidth());
	w+=24;

	h=m_urlcaption.GetLineHeight()+8;
	
	m_urlcaption.SetFontID(bold);
	m_urlcaption.SetPos(0,0);
	m_url.SetPos(w,0);
	m_url.SetSize(bw-w-10,h);
	m_layout.AddObjects(2,&m_urlcaption,&m_url);
	m_layout.NextLine();

	m_stopcaption.SetFontID(bold);
	m_stopcaption.SetPos(0,0);
	m_stoptable.SetPos(w,0);
	m_stoptable.SetSize(bw-w-10,h*5);
	m_stoptable.SetNumCols(1);
	m_stoptable.GetColHeaderTextPtr(0)->SetString("Stop URL Root");
	m_stoptable.SetEventHandler(this,CALLBACKNAME(Event));
	m_stoptable.SetColWidth(0,550);
	m_stoptable.SetAllowAddNewRow(true);

	m_layout.AddObjects(2,&m_stopcaption,&m_stoptable);
	m_layout.NextLine();

	m_startscan.SetString("Start Scan");
	m_startscan.SetFontID(bold);
	m_startscan.SetSize(m_startscan.GetWidth()+16,h);
	m_startscan.SetEventHandler(this,CALLBACKNAME(Event));
	m_layout.AddObject(&m_startscan);

	m_stopscan.SetString("Stop Scan");
	m_stopscan.SetFontID(bold);
	m_stopscan.SetSize(m_stopscan.GetWidth()+16,h);
	m_stopscan.SetEnabled(false);
	m_stopscan.SetEventHandler(this,CALLBACKNAME(Event));

	m_layout.AddObject(&m_stopscan);
	m_layout.NextLine();

	/* add table */
	m_table.SetNumCols(COL_NUMCOLS);
	m_table.GetColHeaderTextPtr(COL_STATUS)->SetString("Status");
	m_table.GetColHeaderTextPtr(COL_URL)->SetString("URL");
	m_table.GetColHeaderTextPtr(COL_REFERER)->SetString("Referer");
	m_table.SetColWidth(COL_STATUS,150);
	m_table.SetColWidth(COL_URL,600);
	m_table.SetColWidth(COL_REFERER,200);
	m_table.SetSize(bw,bh-m_layout.GetZoneH());
	m_table.SetEventHandler(this,CALLBACKNAME(Event));
	m_layout.AddObject(&m_table);

//	m_startpage.SetEventHandler(this,CALLBACKNAME(Event));
	m_layout.SetPos(0,0);
	background->AddObject(&m_layout);

#if 0
	m_browse=new kGUIBrowseObj(&m_browsesettings,w,h/2);
	m_browse->SetPos(0,30);
	background->AddObject(m_browse);
#endif

	/* hash table for URLs in table already */
	m_scanhash.Init(16,0);

	/* save downloaded data to memory */
	m_scandh.SetMemory();
	/* set default settings for the HTML parser */
	m_html.SetSettings(&m_htmlsettings);

	kGUI::ShowWindow();
	kGUI::AddEvent(this,CALLBACKNAME(Scan));
}

void Katipo::Event(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_SELECTED:
		switch(event->m_value[0].i)
		{
		case MENU_LOAD:
		{
			kGUIFileReq *fr;

			fr=new kGUIFileReq(FILEREQ_OPEN,0,".xml",this,CALLBACKNAME(Load));
		}
		break;
		case MENU_SAVE:
		{
			kGUIFileReq *fr;

			fr=new kGUIFileReq(FILEREQ_SAVE,"scan.xml",".xml",this,CALLBACKNAME(Save));
		}
		break;
		case MENU_QUIT:
			kGUI::CloseApp();
		break;
		}
	break;
	case EVENT_ADDROW:
		if(event->GetObj()==&m_table)
		{
			OutputTableRow *row;

			row=new OutputTableRow();
			m_table.AddRow(row);
		}
		else if(event->GetObj()==&m_stoptable)
		{
			StopTableRow *row;

			row=new StopTableRow();
			m_stoptable.AddRow(row);
		}
	break;
	case EVENT_AFTERUPDATE:
	break;
	case EVENT_PRESSED:
		if(event->GetObj()==&m_startscan)
			StartScan();
		else if(event->GetObj()==&m_stopscan)
			StopScan();
	break;
	}
}

void Katipo::StartScan(void)
{
	OutputTableRow *row;

	kGUI::ExtractURL(&m_url,&m_base,&m_root);

	m_table.DeleteChildren();
	row=new OutputTableRow();
	row->SetURL(m_url.GetString());
	m_table.AddRow(row);

	/* clear the hash table */
	m_scanhash.Reset();
	m_scanhash.Add(m_url.GetString(),0);

	m_startscan.SetEnabled(false);
	m_stopscan.SetEnabled(true);
	m_scanrowindex=0;
	m_scanstatus=STATUS_WAIT;
	m_scandelay=0;
	m_scanning=true;
}

/* scan event, called every frame */
void Katipo::Scan(void)
{
	if(!m_scanning)
		return;			/* not scanning yet */

	switch(m_scanstatus)
	{
	case STATUS_WAIT:
		m_scandelay+=kGUI::GetET();
		if(m_scandelay>=TICKSPERSEC)
		{
			/* start loading next entry */
			m_scanrow=static_cast<OutputTableRow *>(m_table.GetChild(m_scanrowindex));
			m_scanrow->SetStatus("Loading...");
			m_scanstatus=STATUS_LOADING;

			/* todo: referer */
			m_scandl.AsyncDownLoad(&m_scandh,m_scanrow->GetURL(),0,0);
		}
	break;
	case STATUS_LOADING:
		if(m_scandl.GetAsyncActive()==false)
		{
			/* set URL for item just downloaded */
			
			if(m_scandl.GetStatus()==DOWNLOAD_OK && m_scandh.GetSize())
			{
				kGUIString d;

				m_scandh.Open();
				m_scandh.Read(&m_scanstring,m_scandh.GetSize());
				m_scandh.Close();

				m_html.SetSource(m_scandl.GetURL(),&m_scanstring,m_scandl.GetEncoding(),m_scandl.GetHeader());
				
				/* parse the DOM tree and collect local links */
				ScanHTML(m_html.GetRootObj());

				d.Sprintf("OK - %d\n",m_scandl.GetReturnCode());
				m_scanrow->SetStatus(d.GetString());
			}
			else
				m_scanrow->SetStatus("Error!");

			if(++m_scanrowindex==m_table.GetNumberRows())
				StopScan();
			else
			{
				m_scandelay=0;
				m_scanstatus=STATUS_WAIT;
			}
		}
	break;
	}

}


void Katipo::ScanHTML(kGUIHTMLObj *obj)
{
	unsigned int i;
	kGUIString *s;
	kGUIString url;
	kGUIString referer;
	kGUIString base;
	kGUIString root;

	/* call children recursively */
	for(i=0;i<obj->GetNumStyleChildren();++i)
	{
		ScanHTML(obj->GetStyleChild(i));
	}

	/* todo, handle css?? */
	switch(obj->GetID())
	{
	case HTMLTAG_A:
		s=obj->GetURL();
		if(s)
			url.SetString(s);
		s=obj->GetReferrer();
		if(s)
			referer.SetString(s);
	break;
	case HTMLTAG_IMG:
		/* handle images only if requested */
	break;
	}

	if(url.GetLen())
	{
		int b;
		int off;
		StopTableRow *srow;

		/* clip off #xxx from end of URL so www.web.com/page.html#aaa = www.web.com/page.html */
		off=url.Str("#");
		if(off>=0)
			url.Clip(off);

		/* make sure the domain matches */
		kGUI::ExtractURL(&url,&base,&root);

		/* compare root against block list */
		for(b=0;b<m_stoptable.GetNumRows();++b)
		{
			srow=static_cast<StopTableRow *>(m_stoptable.GetChild(b));
			if(!strncmp(url.GetString(),srow->GetURL()->GetString(),srow->GetURL()->GetLen()))
				return;
		}

		if(!strcmp(base.GetString(),m_base.GetString()))
		{
			if(m_scanhash.Find(url.GetString())==0)
			{
				OutputTableRow *row;

				/* add to table */
				m_scanhash.Add(url.GetString(),0);
				row=new OutputTableRow();
				row->SetURL(url.GetString());
				row->SetReferer(referer.GetString());
				m_table.AddRow(row);
			}
		}
	}
}

void Katipo::StopScan(void)
{
	m_startscan.SetEnabled(true);
	m_stopscan.SetEnabled(false);
	m_scanning=false;
}

void Katipo::Load(kGUIFileReq *req,int pressed)
{

	if(pressed==MSGBOX_OK)
	{
		kGUIXML xml;
		if(xml.Load(req->GetFilename())==true)
		{
			unsigned int i;
			kGUIXMLItem *xroot;
			kGUIXMLItem *xgroup;
			kGUIXMLItem *xitem;
			StopTableRow *srow;

			xroot=xml.GetRootItem()->Locate("katipo");
			if(xroot)
			{
				xitem=xroot->Locate("url");
				if(xitem)
					m_url.SetString(xitem->GetValue());

				m_stoptable.DeleteChildren();
				xgroup=xroot->Locate("stoplist");
				if(xgroup)
				{
					for(i=0;i<xgroup->GetNumChildren();++i)
					{
						xitem=xgroup->GetChild(i);

						srow=new StopTableRow();
						srow->SetURL(xitem->GetValueString());
						m_stoptable.AddRow(srow);
					}
				}
			}
		}
		/* print, error opening file */
	}
}

void Katipo::Save(kGUIFileReq *req,int pressed)
{
	if(pressed==MSGBOX_OK)
	{
		int i;
		kGUIXML xml;
		kGUIXMLItem *xroot;
		kGUIXMLItem *xgroup;
		StopTableRow *srow;

		/* generate the XML file for our saved settings */
		xml.SetEncoding(ENCODING_UTF8);
		xroot=xml.GetRootItem()->AddChild("katipo");

		xroot->AddChild("url",m_url.GetString());

		xgroup=xroot->AddChild("stoplist");
		for(i=0;i<m_stoptable.GetNumRows();++i)
		{
			srow=static_cast<StopTableRow *>(m_stoptable.GetChild(i));
			xgroup->AddChild("entry",srow->GetURL());
		}

		/* save the xml file */
		xml.Save(req->GetFilename());
	}
}

Katipo::~Katipo()
{
	StopScan();
	kGUI::DelEvent(this,CALLBACKNAME(Scan));
	m_stoptable.DeleteChildren();
	m_table.DeleteChildren();
//	delete m_browse;
}
