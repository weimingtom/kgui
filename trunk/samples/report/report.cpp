/*********************************************************************************/
/* Events - kGUI sample program showing report printing                          */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 18-May-2008                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

#define APPNAME "Report Sample"

#if defined(WIN32) || defined(MINGW)
#include "resource.h"
#endif

#include "kguisys.cpp"

/* print a report, add user items to the report print preview page */

class ReportSample
{
public:
	ReportSample();
	~ReportSample();
private:
	CALLBACKGLUEPTR(ReportSample,ButtonEvent,kGUIEvent);		/* make a static connection to the callback */
	void ButtonEvent(kGUIEvent *event);

	/* combo box showing lists of printers */
	kGUIComboBoxObj m_printerlist;

	kGUIButtonObj m_previewbutton;
	kGUIButtonObj m_printbutton;
};

/* my report */

#include "kguireport.h"
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
		t->SetSize(t->GetWidth()+6,t->GetLineHeight()+6);
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

ReportSample *g_eventsample;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
	kGUI::SetDefFontSize(15);
	kGUI::SetDefReportFontSize(20);

	g_eventsample=new ReportSample();
}

void AppClose(void)
{
	delete g_eventsample;
}

ReportSample::ReportSample()
{
	unsigned int i,numprinters;
	int def;
	kGUIWindowObj *background;

	background=kGUI::GetBackground();
 	background->SetTitle("Report Sample");

	numprinters=kGUI::GetNumPrinters();
	m_printerlist.SetNumEntries(kGUI::GetNumPrinters());
	for(i=0;i<kGUI::GetNumPrinters();++i)
		m_printerlist.SetEntry(i,kGUI::GetPrinterObj(i)->GetName(),i);

	/* set default to 'current' printer */
	def=kGUI::GetDefPrinterNum();
	if(def>=0)
		m_printerlist.SetSelection(def);

	/* combo box for selecting the printer to use */
	i=MIN(350,m_printerlist.GetWidest());
	m_printerlist.SetPos(10,10);
	m_printerlist.SetSize(i,20);
	background->AddObject(&m_printerlist);

	/* add a button */
	m_previewbutton.SetPos(25,100);						/* x,y */
	m_previewbutton.SetSize(300,50);					/* width, height */
	m_previewbutton.SetFontSize(30);					/* in points */
	m_previewbutton.SetColor(DrawColor(255,0,255));		/* purple */
	m_previewbutton.SetString("Print Preview");			/* set button text */

	/* add the button to the background window */
	background->AddObject(&m_previewbutton);

	/* add a button */
	m_printbutton.SetPos(350,100);						/* x,y */
	m_printbutton.SetSize(300,50);					/* width, height */
	m_printbutton.SetFontSize(30);					/* in points */
	m_printbutton.SetColor(DrawColor(0,255,255));		/* purple */
	m_printbutton.SetString("Print");				/* set button text */

	/* add the button to the background window */
	background->AddObject(&m_printbutton);

	m_previewbutton.SetEventHandler(this,CALLBACKNAME(ButtonEvent));
	m_printbutton.SetEventHandler(this,CALLBACKNAME(ButtonEvent));
	kGUI::ShowWindow();
}

/* you can have a unique event handler for each object, or you can have one to handle many objects */
void ReportSample::ButtonEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
	{
		MyReport *rep;

		rep=new MyReport(m_printerlist.GetSelection());
		if(event->GetObj()==&m_previewbutton)
			rep->Preview();
		else
			rep->Print();
	}
	break;
	}
}

ReportSample::~ReportSample()
{
}
