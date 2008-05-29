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
	CALLBACKGLUEPTR(ReportSample,InputEvent,kGUIEvent);		/* make a static connection to the callback */
	void InputEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(ReportSample,WindowEvent,kGUIEvent);		/* make a static connection to the callback */
	void WindowEvent(kGUIEvent *event);

	void UpdateText(void);

	/* combo box showing lists of printers */
	kGUIComboBoxObj m_printerlist;

	kGUIButtonObj m_previewbutton;
	kGUIButtonObj m_printbutton;
};

ReportSample *g_eventsample;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	g_eventsample=new ReportSample();
}

void AppClose(void)
{
	delete g_eventsample;
}

ReportSample::ReportSample()
{
	unsigned int i,numprinters;
	kGUIWindowObj *background;

	background=kGUI::GetBackground();
 	background->SetTitle("Report Sample");

	numprinters=kGUI::GetNumPrinters();
	m_printerlist.SetNumEntries(kGUI::GetNumPrinters());
	for(i=0;i<kGUI::GetNumPrinters();++i)
		m_printerlist.SetEntry(i,kGUI::GetPrinterObj(i)->GetName(),i);

	/* set default to 'current' printer */

	/* combo box for selecting the printer to use */
	i=min(350,m_printerlist.GetWidest());
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
	m_printbutton.SetPos(25,100);						/* x,y */
	m_printbutton.SetSize(300,50);					/* width, height */
	m_printbutton.SetFontSize(30);					/* in points */
	m_printbutton.SetColor(DrawColor(255,0,255));		/* purple */
	m_printbutton.SetString("Print Preview");			/* set button text */

	/* add the button to the background window */
	background->AddObject(&m_printbutton);

//	m_button.SetEventHandler(this,CALLBACKNAME(ButtonEvent));
//	m_input.SetEventHandler(this,CALLBACKNAME(InputEvent));
}

/* you can have a unique event handler for each object, or you can have one to handle many objects */
void ReportSample::ButtonEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
	break;
	}
}

ReportSample::~ReportSample()
{
}
