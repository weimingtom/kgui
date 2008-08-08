/*********************************************************************************/
/* CSVEdit - kGUI sample program showing:                                        */
/*                                                                               */
/* Tables                                                                        */
/* Load/Save requestor                                                           */
/* Printing reports                                                              */
/* Load/Save XML preferences                                                     */
/* Sample Font added to internal bigfile                                         */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 02-Jun-2008                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguicsv.h"
#include "kguireport.h"
#include "kguireq.h"

#define APPNAME "CSVEdit Sample"

#if defined(WIN32) || defined(MINGW)
#include "resource.h"
#endif

#include "kguisys.cpp"

class CSVTableRow : public kGUITableRowObj
{
public:
	CSVTableRow();
	int GetNumObjects(void) {return m_numcells;}
	kGUIObj **GetObjectList(void) {return m_objectlist.GetArrayPtr();} 
	void SetCell(int col,kGUIString *s) {m_cells.GetEntryPtr(col)->SetString(s);}
	kGUIString *GetCell(int col) {return m_cells.GetEntryPtr(col);}
private:
	int m_numcells;
	Array<kGUIObj *>m_objectlist;
	ClassArray<kGUIInputBoxObj>m_cells;
};

class CSVEditSample
{
public:
	CSVEditSample();
	~CSVEditSample();
	int GetNumCols(void) {return m_table.GetNumCols();}
	int GetNumRows(void) {return m_table.GetNumRows();}
	kGUIString *GetCellPtr(int row,int col) {return static_cast<kGUIInputBoxObj *>(m_table.GetRow(row)->GetObjectList()[col]);}
private:
	CALLBACKGLUEPTR(CSVEditSample,ButtonEvent,kGUIEvent);		/* make a static connection to the callback */
	void ButtonEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(CSVEditSample,OpenMenuEvent,kGUIEvent);		/* make a static connection to the callback */
	void OpenMenuEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(CSVEditSample,MenuEvent,kGUIEvent);			/* make a static connection to the callback */
	void MenuEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(CSVEditSample,ColMenuEvent,kGUIEvent);			/* make a static connection to the callback */
	void ColMenuEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(CSVEditSample,TableEvent,kGUIEvent);		/* make a static connection to the callback */
	void TableEvent(kGUIEvent *event);
	CALLBACKGLUEPTRVAL(CSVEditSample,LoadCSV,kGUIFileReq,int);			/* make a static connection to the callback */
	void LoadCSV(kGUIFileReq *req,int status);
	CALLBACKGLUEPTRVAL(CSVEditSample,SaveCSV,kGUIFileReq,int);			/* make a static connection to the callback */
	void SaveCSV(kGUIFileReq *req,int status);
	static int SortEntry(const void *o1,const void *o2);

	kGUITextObj m_menucaption;
	kGUIMenuColObj m_popmenu;
	kGUIMenuColObj m_colmenu;

	unsigned int m_numsortcols;
	Array<int>m_sortcols;
	Array<bool>m_sortrevs;	/* reverse flag */

	/* todo, had header flag */
	/* todo: font size for use in the table */

	kGUITableObj m_table;
};

/* my report */

class MyReport : public kGUIReport
{
public:
	MyReport(int pid);
	~MyReport();
private:
	int GetPPI(void) {return 125;}					/* pixels per inch */
	double GetPageWidth(void) {return -1.0f;}		/* inches */
	double GetPageHeight(void) {return -1.0f;}		/* inches */
	double GetLeftMargin(void) {return 0.25f;}		/* inches */
	double GetRightMargin(void) {return 0.25f;}		/* inches */
	double GetTopMargin(void)	{return 0.25f;}		/* inches */
	double GetBottomMargin(void) {return 0.25f;}	/* inches */
	void Setup(void);
	void Setup(int page);
	const char *GetName(void) {return "My Report Title";}
	/********************************/

	/* put uset controls that will appear in the print preview page here */
	kGUITextObj m_tnumlines;
	kGUIInputBoxObj m_inumlines;
	kGUITextObj m_tfontsize;
	kGUIInputBoxObj m_ifontsize;

	/* this will be duplicated at the top of EACH page */
	kGUIReportTextObj m_pagenofn;
	kGUIReportTextObj m_pagetitle;
	kGUIReportTextObj m_pagedatetime;

	/* this is int the body */
	ClassArray<kGUIReportTextObj>m_lines;

	kGUIReportRowHeaderObj m_rowheader;
};

CSVEditSample *g_csv;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui for bold */
	kGUI::SetDefFontSize(15);
	kGUI::SetDefReportFontSize(20);

	new CSVEditSample();
}

void AppClose(void)
{
	delete g_csv;
}

enum
{
MENU_LOADCSV,
MENU_SAVECSV,
MENU_PRINT,
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

CSVEditSample::CSVEditSample()
{
	int i;
	kGUIWindowObj *background;

	g_csv=this;
	background=kGUI::GetBackground();
 	background->SetTitle("CSVEdit");

	/* this is static text that when clicked on opens the popup menu */
	m_menucaption.SetFontSize(20);
	m_menucaption.SetString("Menu");
	m_menucaption.SetEventHandler(this,CALLBACKNAME(OpenMenuEvent));
	background->AddObject(&m_menucaption);

	/* let's populate the popup menu */
	m_popmenu.SetNumEntries(MENU_NUMENTRIES);
	m_popmenu.SetEntry(MENU_LOADCSV,"Load CSV");
	m_popmenu.SetEntry(MENU_SAVECSV,"Save CSV");
	m_popmenu.SetEntry(MENU_PRINT,"Print");
	m_popmenu.SetEntry(MENU_EXIT,"Exit");
	m_popmenu.SetEventHandler(this,CALLBACKNAME(MenuEvent));

	/* the menu when right clicking on a column */
	m_colmenu.SetNumEntries(COLMENU_NUMENTRIES);
	m_colmenu.SetEntry(COLMENU_INSERTCOLBEFORE,"Insert Column Before");
	m_colmenu.SetEntry(COLMENU_INSERTCOLAFTER,"Insert Column After");
	m_colmenu.SetEntry(COLMENU_DELETECOL,"Delete Column");
	m_colmenu.SetEntry(COLMENU_SORTASC,"Sort Ascending");
	m_colmenu.SetEntry(COLMENU_SORTDESC,"Sort Descending");
	m_colmenu.SetEntry(COLMENU_SORTASC2,"Secondaty Sort Ascending");
	m_colmenu.SetEntry(COLMENU_SORTDESC2,"Secondaty Sort Descending");
	m_colmenu.SetEventHandler(this,CALLBACKNAME(ColMenuEvent));

	/* default to 4 columns */
	m_table.SetNumCols(4);
	for(i=0;i<4;++i)
	{
		m_table.GetColHeaderTextPtr(i)->Sprintf("Col #%d",i);
		m_table.SetColWidth(i,150);
	}
	m_table.SetAllowAddNewRow(true);	/* allow user to add new row */
	m_table.SetPos(0,30);
	m_table.SetSize(background->GetChildZoneW(),background->GetChildZoneH()-30);
	m_table.SetEventHandler(this,CALLBACKNAME(TableEvent));
	background->AddObject(&m_table);

	m_sortcols.Init(3,1);
	m_sortrevs.Init(3,1);
	m_numsortcols=0;

	/* add 20 rows to the table, added by table event handler */
	for(i=0;i<20;++i)
		m_table.AddNewRow();
	kGUI::ShowWindow();
}

void CSVEditSample::TableEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ADDROW:
		CSVTableRow *row;

		row=new CSVTableRow();
		m_table.AddRow(row);
	break;
	case EVENT_COL_RIGHTCLICK:
		m_colmenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
	break;
	}
}

void CSVEditSample::OpenMenuEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_LEFTCLICK)
		m_popmenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

void CSVEditSample::MenuEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		switch(event->m_value[0].i)
		{
		case MENU_LOADCSV:
		{
			kGUIFileReq *loadreq;

			loadreq=new kGUIFileReq(FILEREQ_OPEN,"",".csv",this,CALLBACKNAME(LoadCSV));
		}
		break;
		case MENU_SAVECSV:
		{
			kGUIFileReq *savereq;

			savereq=new kGUIFileReq(FILEREQ_SAVE,"",".csv",this,CALLBACKNAME(SaveCSV));
		}
		break;
		case MENU_PRINT:
		break;
		case MENU_EXIT:
			kGUI::CloseApp();
		break;
		}
	}
}

void CSVEditSample::ColMenuEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		switch(event->m_value[0].i)
		{
		case COLMENU_INSERTCOLBEFORE:
		break;
		case COLMENU_INSERTCOLAFTER:
		break;
		case COLMENU_DELETECOL:
		break;
		case COLMENU_SORTASC:
			m_numsortcols=1;
			m_sortcols.SetEntry(0,m_table.GetColOrder(m_table.GetCursorCol()));
			m_sortrevs.SetEntry(0,false);
			m_table.Sort(SortEntry);
		break;
		case COLMENU_SORTDESC:
			m_numsortcols=1;
			m_sortcols.SetEntry(0,m_table.GetColOrder(m_table.GetCursorCol()));
			m_sortrevs.SetEntry(0,true);
			m_table.Sort(SortEntry);
		break;
		case COLMENU_SORTASC2:
			m_sortcols.SetEntry(m_numsortcols,m_table.GetColOrder(m_table.GetCursorCol()));
			m_sortrevs.SetEntry(m_numsortcols,false);
			++m_numsortcols;
			m_table.Sort(SortEntry);
		break;
		case COLMENU_SORTDESC2:
			m_sortcols.SetEntry(m_numsortcols,m_table.GetColOrder(m_table.GetCursorCol()));
			m_sortrevs.SetEntry(m_numsortcols,true);
			++m_numsortcols;
			m_table.Sort(SortEntry);
		break;
		}
	}
}

int CSVEditSample::SortEntry(const void *o1,const void *o2)
{
	unsigned int s;
	int sc;
	int res;
	bool sr;
	CSVTableRow *r1=*(static_cast<CSVTableRow **>((void *)o1));
	CSVTableRow *r2=*(static_cast<CSVTableRow **>((void *)o2));

	for(s=0;s<g_csv->m_numsortcols;++s)
	{
		sc=g_csv->m_sortcols.GetEntry(s);
		sr=g_csv->m_sortrevs.GetEntry(s);

		res=strcmp(r1->GetCell(sc)->GetString(),r2->GetCell(sc)->GetString());
		if(res)
		{
			if(sr==false)
				return(res);
			return(-res);
		}

	}
	/* tie */
	return(0);
}


/* you can have a unique event handler for each object, or you can have one to handle many objects */
void CSVEditSample::ButtonEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
	{
		MyReport *rep;

		rep=new MyReport(kGUI::GetDefPrinterNum());
		rep->Preview();
	}
	break;
	}
}

void CSVEditSample::LoadCSV(kGUIFileReq *req,int status)
{
	if(status==MSGBOX_OK)
	{
		kGUICSV csvfile;
		unsigned int r,numrows;
		unsigned int c,numcols;

		csvfile.SetFilename(req->GetFilename());
		if(csvfile.Load()==true)
		{
			numrows=csvfile.GetNumRows();
			numcols=csvfile.GetNumCols();

			m_table.DeleteChildren();
			m_table.SetNumCols(numcols);

			for(r=0;r<numrows;++r)
			{
				m_table.AddNewRow();
				for(c=0;c<numcols;++c)
					csvfile.GetField(r,c,GetCellPtr(r,c));
			}
		}
	}
}

void CSVEditSample::SaveCSV(kGUIFileReq *req,int status)
{
	if(status==MSGBOX_OK)
	{
		int row,col;
		int numrows;
		int numcols;
		kGUIString *cell;
		bool stop;
		kGUICSV csvfile;

		numrows=GetNumRows();
		numcols=GetNumCols();

		/* decrement numcols if empty columns found */
		stop=false;
		while(numcols>1 && stop==false)
		{
			row=0;
			while(row<numrows && stop==false)
			{
				cell=GetCellPtr(row,numcols-1);
				if(cell->GetLen())
					stop=true;
				++row;
			}
			if(stop==false)
				--numcols;
		}
		/* decrement numrows if empty rows found */
		stop=false;
		while(numrows>1 && stop==false)
		{
			col=0;
			while(col<numcols && stop==false)
			{
				cell=GetCellPtr(numrows-1,col);
				if(cell->GetLen())
					stop=true;
				++col;
			}
			if(stop==false)
				--numrows;
		}
		
		/* ok we got a numrows and numcols to save */
		csvfile.SetFilename(req->GetFilename());
		csvfile.Init(numrows,numcols);
		for(row=0;row<numrows;++row)
		{
			for(col=0;col<numcols;++col)
				csvfile.SetField(row,col,GetCellPtr(row,col));
		}
		csvfile.Save();
	}
}

CSVEditSample::~CSVEditSample()
{
	m_table.DeleteChildren();
}

/*********************************************************************************/

CSVTableRow::CSVTableRow()
{
	int i;

	/* get number of columns from the table */
	m_numcells=g_csv->GetNumCols();
	m_objectlist.Init(m_numcells,1);
	m_cells.Init(m_numcells,1);

	for(i=0;i<m_numcells;++i)
		m_objectlist.SetEntry(i,m_cells.GetEntryPtr(i));
	SetRowHeight(20);
}

/*********************************************************************************/

MyReport::MyReport(int pid)
{
	//printer to use
	SetPID(pid);

	/* add custom controls to the print preview panel */
	m_tnumlines.SetString("Number of Lines:");
	AddUserControl(&m_tnumlines);
	m_inumlines.SetString("25");
	m_inumlines.SetSize(100,m_tnumlines.GetZoneH());
	AddUserControl(&m_inumlines);

	m_tfontsize.SetString("Starting Font Size:");
	AddUserControl(&m_tfontsize);
	m_ifontsize.SetString("50");
	m_ifontsize.SetSize(100,m_tfontsize.GetZoneH());
	AddUserControl(&m_ifontsize);

	m_lines.Init(16,16);
}

void MyReport::Setup(void)
{
	int ppw,pph;
	int i,fs,y,numlines,fontsize;
	kGUIDate date;
	kGUIString timestring;
	kGUIReportTextObj *t;

	numlines=m_inumlines.GetInt();
	fontsize=m_ifontsize.GetInt();

	/* get page size in pixels */
	GetPageSizePixels(&ppw,&pph);

	m_pagenofn.SetPos(0,0);
	/* text will be overwritten during the Setup code for each page */
	m_pagenofn.SetString("Page x of x");
	AddObjToSection(REPORTSECTION_PAGEHEADER,&m_pagenofn);

	m_pagetitle.SetPos(0,0);
	m_pagetitle.SetString("My Page Title!");
	m_pagetitle.SetZoneW(ppw);
	m_pagetitle.SetHAlign(FT_CENTER);
	AddObjToSection(REPORTSECTION_PAGEHEADER,&m_pagetitle);

	/* generate a string for date/time */
	date.SetToday();
	date.LongDate(&m_pagedatetime);
	date.Time(&timestring);
	m_pagedatetime.ASprintf(" %s",timestring.GetString());

	m_pagedatetime.SetPos(0,0);
	m_pagedatetime.SetZoneW(ppw);
	m_pagedatetime.SetHAlign(FT_RIGHT);
	AddObjToSection(REPORTSECTION_PAGEHEADER,&m_pagedatetime);

	fs=fontsize;
	y=0;
	for(i=0;i<numlines;++i)
	{
		t=m_lines.GetEntryPtr(i);	/* get a report text object */

		t->SetPos(0,y);

		t->SetFontSize(fs);	
		t->SetString("Hello World!");
		t->SetSize(t->GetWidth()+6,t->GetHeight()+6);
		y+=t->GetZoneH();

		/* attach to the page */
		AddObjToSection(REPORTSECTION_BODY,t);

		++fs;
	}

#if 0
	int i,ne,x,y,w,xcol;
	GPXRow *row;
	GridLine *line;
	int oldsize;

	oldsize=kGUI::GetDefReportFontSize();
	kGUI::SetDefReportFontSize(gpx->GetTableFontSize());
//	SetBitmapMode(true);	/* send to printer as a bitmap */

	/* count number of visible columns */
	ne=0;
	for(i=0;i<m_table->GetNumCols();++i)
	{
		if(m_table->GetColShow(i)==true)
			++ne;
	}	

	m_rowheader.SetNumColumns(m_table->GetNumCols());
	x=0;
	y=0;
	for(i=0;i<m_table->GetNumCols();++i)
	{
		xcol=m_table->GetColOrder(i);
		if(m_table->GetColShow(xcol)==true)
		{
			w=m_table->GetColWidth(xcol)+12;

			m_rowheader.SetColX(y,x);
			m_rowheader.SetColWidth(y,w);
			m_rowheader.SetColName(y,wpcolnames[xcol]);
			x+=w;
			++y;
		}
	}
	m_rowheader.SetZoneH(20);
	AddObjToSection(REPORTSECTION_PAGEHEADER,&m_rowheader,false);

	ne=m_table->GetNumChildren();
	y=0;
	for(i=0;i<ne;++i)
	{
		row=static_cast<GPXRow *>(m_table->GetChild(i));
		line=new GridLine(&m_rowheader,m_table,row,(i&1)==1);
		line->SetZoneY(y);
		AddObjToSection(REPORTSECTION_BODY,line,true);
		y+=line->GetZoneH();
	}
	kGUI::SetDefReportFontSize(oldsize);
#endif
}

/* this is called before printing each page */
void MyReport::Setup(int page)
{
	m_pagenofn.Sprintf("Page %d of %d",page,GetNumPages());
}

MyReport::~MyReport()
{
	//save selected printer for using as the default next time
//	gpx->m_MyReport.SetString(kGUI::GetPrinterObj(GetPID())->GetName());
}


