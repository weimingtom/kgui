/*********************************************************************************/
/* Events - kGUI sample program showing event handling                           */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/* 18-May-2008                                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

#define APPNAME "Events"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

#define NUMLINES 25

class EventSample
{
public:
	EventSample();
	~EventSample();
private:
	CALLBACKGLUEPTR(EventSample,ButtonEvent,kGUIEvent);		/* make a static connection to the callback */
	void ButtonEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(EventSample,InputEvent,kGUIEvent);		/* make a static connection to the callback */
	void InputEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(EventSample,WindowEvent,kGUIEvent);		/* make a static connection to the callback */
	void WindowEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(EventSample,MenuEvent,kGUIEvent);		/* make a static connection to the callback */
	void MenuEvent(kGUIEvent *event);

	void UpdateText(void);

	kGUIMenuObj m_menu;
	kGUIMenuColObj m_filemenu;
	kGUIMenuColObj m_helpmenu;
	kGUIMenuColObj m_loadsubmenu;
	kGUIButtonObj m_button;
	kGUITextObj m_text;
	kGUIInputBoxObj m_input;
	kGUITextObj m_text2;
	kGUIScrollContainerObj m_scontainer;
	kGUITextObj m_lines[NUMLINES];

	unsigned int m_numwindows;
	volatile unsigned int m_numopenwindows;
	Array<kGUIWindowObj *>m_openwindows;
};

EventSample *g_eventsample;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	g_eventsample=new EventSample();
}

void AppClose(void)
{
	delete g_eventsample;
}

enum
{
FE_LOAD,
FE_LOADGPX,
FE_LOADAS,
FE_SAVE,
FE_HELP,
FE_CREDITS
};


EventSample::EventSample()
{
	int i;

	kGUIWindowObj *background;

	m_numwindows=0;
	m_numopenwindows=0;
	m_openwindows.Init(16,4);

	/* get pointer to the background window object */
	background=kGUI::GetBackground();
 	background->SetTitle("EventSample");

	m_filemenu.SetFontSize(25);
	m_filemenu.SetNumEntries(2);
	m_filemenu.SetEntry(0,"Load",FE_LOAD);
	m_filemenu.SetEntry(1,"Save",FE_SAVE);
	m_filemenu.GetEntry(0)->SetSubMenu(&m_loadsubmenu);

	m_loadsubmenu.SetFontSize(25);
	m_loadsubmenu.SetNumEntries(2);
	m_loadsubmenu.SetEntry(0,"Load GPX",FE_LOADGPX);
	m_loadsubmenu.SetEntry(1,"Load As",FE_LOADAS);

	m_helpmenu.SetFontSize(25);
	m_helpmenu.SetNumEntries(2);
	m_helpmenu.SetEntry(0,"Help",FE_HELP);
	m_helpmenu.SetEntry(1,"Credits",FE_CREDITS);

	/* add a menu */
	m_menu.SetPos(0,0);
	m_menu.SetNumEntries(2);
	m_menu.GetTitle(0)->SetString("File");
	m_menu.GetTitle(0)->SetFontSize(25);
	m_menu.GetTitle(1)->SetString("Help");
	m_menu.GetTitle(1)->SetFontSize(25);

	m_menu.SetEntry(0,&m_filemenu);
	m_menu.SetEntry(1,&m_helpmenu);

	/* add the menu to the background window */
	m_menu.SetEventHandler(this,CALLBACKNAME(MenuEvent));
	background->AddObject(&m_menu);

	/* add a button */
	m_button.SetPos(0,45);						/* x,y */
	m_button.SetSize(300,50);					/* width, height */
	m_button.SetFontSize(30);					/* in points */
	m_button.SetColor(DrawColor(255,0,255));	/* purple */
	m_button.SetString("Open Window");			/* set button text */

	/* add the button to the background window */
	background->AddObject(&m_button);

	/* add static text */
	m_text.SetPos(350,45);						/* x,y */
	m_text.SetFontSize(30);						/* in points */
	m_text.SetColor(DrawColor(128,128,0));		/* brown */
	UpdateText();

	/* add the text to the background window */
	background->AddObject(&m_text);

	/* add a button */
	m_input.SetPos(25,100);						/* x,y */
	m_input.SetSize(300,50);					/* width, height */
	m_input.SetFontSize(30);					/* in points */
	m_input.SetColor(DrawColor(64,0,255));		/* blue */

	/* add the input box to the background window */
	background->AddObject(&m_input);

	/* add static text */
	m_text2.SetPos(25,175);						/* x,y */
	m_text2.SetFontSize(30);					/* in points */
	m_text2.SetColor(DrawColor(0,128,128));		/* dark cyan */

	/* add the text to the background window */
	background->AddObject(&m_text2);

	/* add lines to the scroll container */
	for(i=0;i<NUMLINES;++i)
	{
		m_lines[i].Sprintf("Line number %d..asdjkkajshdjkashdjkahdsjhka",i);
		m_lines[i].SetPos(0,i*30);
		m_scontainer.AddObject(&m_lines[i]);
	}

	m_scontainer.SetPos(25,225);
	m_scontainer.SetSize(500,300);

	m_scontainer.SetMaxWidth(1000);
	m_scontainer.SetMaxHeight(1000);

	/* add the scroll container to the background window */
	background->AddObject(&m_scontainer);

	m_button.SetEventHandler(this,CALLBACKNAME(ButtonEvent));
	m_input.SetEventHandler(this,CALLBACKNAME(InputEvent));

	kGUI::ShowWindow();
}

/* you can have a unique event handler for each object, or you can have one to handle many objects */
void EventSample::ButtonEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
	{
		kGUIWindowObj *window;

		/* add a new window */
		window=new kGUIWindowObj();

		/* save all open windows in a list */
		m_openwindows.SetEntry(m_numopenwindows++,window);

		/* set the windows position and size */
		window->SetPos(200+(m_numwindows*10),200+(m_numwindows*10));
		window->SetSize(200,200);
		window->GetTitle()->Sprintf("Window #%d",++m_numwindows);

		/* attach our event handler to the window */
		window->SetEventHandler(this,CALLBACKNAME(WindowEvent));

		/* add the window to the system so it is rendered and handled for events */
		kGUI::AddWindow(window);

		/* update the #windows text on the main page */
		UpdateText();
	}
	break;
	}
}

void EventSample::InputEvent(kGUIEvent *event)
{
	/* if desired you can change this to an event that only gets triggered */
	/* when they press return in an input box instead of as they type */
	switch(event->GetEvent())
	{
	case EVENT_MOVED:
		m_text2.Sprintf("You typed: %s",m_input.GetString());
	break;
	}
}

void EventSample::MenuEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_SELECTED:
	{
		kGUIMsgBoxReq *msg;

		msg=new kGUIMsgBoxReq(MSGBOX_OK,true,"Menu Entry selected id=%d\n",event->m_value[0].i);
	}
	break;
	}
}


void EventSample::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
	{
		kGUIWindowObj *window;

		/* the event structure has a generic kGUIObj * */
		/* so cast it to a window pointer */
		window=static_cast<kGUIWindowObj*>(event->GetObj());
		delete window;

		/* remove from list of open windows */
		m_openwindows.Delete(window);
		--m_numopenwindows;

		UpdateText();
	}
	break;
	}
}

void EventSample::UpdateText(void)
{
	m_text.Sprintf("Number of Windows opened is %d",m_numopenwindows);
}

EventSample::~EventSample()
{
	/* close any windows left open */

	/* since the delete will call the attached window close event, it will remove it from the */
	/* list as it is deleted and m_numopenwindows will decrement automatically as they are closed */
	while(m_numopenwindows)
		m_openwindows.GetEntry(m_numopenwindows-1)->Close();
}
