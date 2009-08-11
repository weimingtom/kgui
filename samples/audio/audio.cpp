/*********************************************************************************/
/* Audio - kGUI sample program                                                  */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguiaudio.h"

#define APPNAME "Audio"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

typedef struct
{
	const char *name;
	const char *filename;
}SAMPLELIST_DEF;

SAMPLELIST_DEF sl[]={
	{"chord","chord.wav"},
	{"notify","notify.wav"},
	{"tada","tada.wav"},
	{"ding","ding.wav"}};

#define NUMSAMPLES ((sizeof(sl))/(sizeof(sl[0])))

class Audio
{
public:
	Audio();
	~Audio();
private:
	void ButtonEvent(kGUIEvent *event);
	CALLBACKGLUEPTR(Audio,ButtonEvent,kGUIEvent);
	kGUIAudioManager m_audiomanager;
	kGUITextObj m_text[NUMSAMPLES];
	kGUIButtonObj m_play[NUMSAMPLES];
	kGUIButtonObj m_playloop[NUMSAMPLES];
	kGUIButtonObj m_stop[NUMSAMPLES];
	kGUIAudio *m_audio[NUMSAMPLES];
	const unsigned char *m_sample[NUMSAMPLES];
	unsigned long m_samplesize[NUMSAMPLES];
};

Audio *g_Audio;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf",false);	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf",true);	/* use default font inside kgui for bold */
//	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
//	kGUI::LoadFont("fontb.ttf");	/* use default bold font */
	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	g_Audio=new Audio();
}

void AppClose(void)
{
	delete g_Audio;
}

Audio::Audio()
{
	unsigned int i;
	int y;
	kGUIWindowObj *background;

	/* get pointer to the background window object */
	background=kGUI::GetBackground();
 	background->SetTitle("Audio");

	m_audiomanager.Init();

	y=0;
	for(i=0;i<NUMSAMPLES;++i)
	{
		m_text[i].SetPos(0,y);
		m_text[i].SetString(sl[i].name);
		m_text[i].SetSize(150,30);
		background->AddObject(&m_text[i]);

		m_play[i].SetPos(150,y);
		m_play[i].SetString("Play");
		m_play[i].SetSize(150,30);
		m_play[i].SetEventHandler(this,CALLBACKNAME(ButtonEvent));
		background->AddObject(&m_play[i]);

		m_playloop[i].SetPos(310,y);
		m_playloop[i].SetString("Loop Play");
		m_playloop[i].SetSize(150,30);
		m_playloop[i].SetEventHandler(this,CALLBACKNAME(ButtonEvent));
		background->AddObject(&m_playloop[i]);

		m_stop[i].SetPos(460,y);
		m_stop[i].SetString("Stop");
		m_stop[i].SetSize(150,30);
		m_stop[i].SetEventHandler(this,CALLBACKNAME(ButtonEvent));
		background->AddObject(&m_stop[i]);

		m_sample[i]=kGUI::LoadFile(sl[i].filename,&m_samplesize[i]);
		m_audio[i]=m_audiomanager.GetAudio();
		y+=35;
	}
	kGUI::ShowWindow();
}

void Audio::ButtonEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_PRESSED:
		for(unsigned int i=0;i<NUMSAMPLES;++i)
		{
			if(event->GetObj()==&m_play[i])
				m_audio[i]->Play(44000,1,m_sample[i],m_samplesize[i],false,false);
			else if(event->GetObj()==&m_playloop[i])
				m_audio[i]->Play(44000,1,m_sample[i],m_samplesize[i],false,true);
			else if(event->GetObj()==&m_stop[i])
				m_audio[i]->Stop();
		}
	break;
	}
}

Audio::~Audio()
{
	unsigned int i;

	for(i=0;i<NUMSAMPLES;++i)
	{
		m_audiomanager.ReleaseAudio(m_audio[i]);
		delete m_sample[i];
	}
}
