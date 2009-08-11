/*********************************************************************************/
/* Space - kGUI sample program                                                  */
/*                                                                               */
/* ( Black Widow ) web spidering program                                         */
/*                                                                               */
/* Programmed by Kevin Pickell                                                   */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguimatrix.h"
#include "kguiaudio.h"

#define APPNAME "Space"

#if defined(WIN32) || defined(MINGW)
/* this is for the ICON in windows */
#include "resource.h"
#endif

/* this includes the main loop for the selected OS, like Windows, Linux, Mac etc */
#include "kguisys.cpp"

#define GameVector3 kGUIVector3
#define GameMatrix33 kGUIMatrix33
#define GameColor kGUIColor 
#define GameAudioHandle kGUIAudioHandle

#include "game.h"
#include "game.cpp"

/* this is generated, it is a bigfile that contains all the files in the 'big' directory */
#include "data.cpp"

enum
{
MODE_TITLE,
MODE_PLAYING,
MODE_OVER};

class Game : public GameBase, public kGUIObj
{
public:
	Game();
	~Game();
	void Draw(void);
	bool UpdateInput(void) {return(true);}
	void DrawRect(double lx,double ty,double rx,double by,kGUIColor color) {kGUI::DrawRect((int)(m_lx+m_cx+lx),(int)(m_ty+m_cy+ty),(int)(m_lx+m_cx+rx),(int)(m_ty+m_cy+by),color);}
	void DrawModel(double x,double y,double h,kGUIColor color,GameModel *model,double scale,double fat);
	bool IsPressed(unsigned int num,INPUTID inputid);

	void PauseRequest(unsigned int n) {}
	void GameOver(void) {m_mode=MODE_OVER;}
	double Rand(double low,double high) {double range;double r;range=(high-low);r=(rand()&4095)/4095.0f;r=(range*r)+low;return(r);}
	unsigned int Rand(unsigned int n) {return (rand()%n);}
	void Update(void);
	CALLBACKGLUE(Game,Update);

	void PlaySound(unsigned int sound);
	GameAudioHandle StartSound(unsigned int sound);
	void StopSound(GameAudioHandle handle);

	void Event(kGUIEvent *event);
	CALLBACKGLUEPTR(Game,Event,kGUIEvent);
private:
	int m_mode;
	int m_lx;
	int m_ty;
	int m_et;
	int m_frame;

	/* sounds */
	kGUIAudioManager m_audiomanager;
	kGUIAudioFormat m_sound[SOUND_NUMSOUNDS];
};

/* this table needs to match the enum order in game.h */
static const char *soundnames[SOUND_NUMSOUNDS]={
	"beat1.wav",
	"beat2.wav",
	"thrust.wav",
	"shot.wav",
	"explosion.wav"};

Game::Game()
{
	unsigned int i;

	m_mode=MODE_TITLE;
	m_et=0;
	m_frame=0;

	m_audiomanager.Init();

	/* load all audio samples */
	for(i=0;i<SOUND_NUMSOUNDS;++i)
	{
		m_sound[i].SetFilename(soundnames[i]);
		m_sound[i].Load();
		assert(m_sound[i].GetSoundFormat()!=AUDIOFORMAT_UNKNOWN,"Unknown Sound Format!");
	}
}

Game::~Game()
{
}

void Game::Update(void)
{
	Dirty();
	m_et+=kGUI::GetET();
	while(m_et>=(TICKSPERSEC/60))
	{
		m_et-=(TICKSPERSEC/60);
		++m_frame;
		GameBase::Update();

		switch(m_mode)
		{
		case MODE_TITLE:
			if(IsPressed(0,GAMEINPUT_THRUST))
			{
				SetNumPlayers(1);
				Start();
				m_mode=MODE_PLAYING;
			}
		break;
		case MODE_OVER:
			if(IsPressed(0,GAMEINPUT_THRUST))
			{
				StartTitle();
				m_mode=MODE_TITLE;
			}
		break;
		}
	}
}


void Game::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	m_lx=c.lx;
	m_ty=c.ty;
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(0,0,0));
	GameBase::Draw(c.rx-c.lx,c.by-c.ty);
	switch(m_mode)
	{
	case MODE_TITLE:
		DrawStringC("SPACE",5.0f,0,-150.0f,0xffffff00);
		DrawStringC("PRESS 'A' TO START",3.0f,0.0f,50.0f,0xffffff00);
	break;
	case MODE_OVER:
		DrawStringC("GAME OVER",5.0f,0,-50.0f,0xffffff00);
	break;
	default:
	break;
	}

}

/* handle resize events on window so we resize the game window too */
void Game::Event(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_SIZECHANGED:
	{
		kGUIWindowObj *background;
		int w,h;

		/* get pointer to the background window object */
		background=kGUI::GetBackground();
		w=background->GetChildZoneW();
		h=background->GetChildZoneH();

		SetSize(w,h);
	}
	break;
	}
}

bool Game::IsPressed(unsigned int num,INPUTID inputid)
{
	if(!num)
	{
		switch(inputid)
		{
		case GAMEINPUT_LEFT:
			return(kGUI::GetKeyState(GUIKEY_LEFT));
		break;
		case GAMEINPUT_RIGHT:
			return(kGUI::GetKeyState(GUIKEY_RIGHT));
		break;
		case GAMEINPUT_THRUST:
			return(kGUI::GetKeyState('A'));
		break;
		case GAMEINPUT_FIRE:
			return(kGUI::GetKeyState('S'));
		break;
		case GAMEINPUT_PAUSE:
			return(kGUI::GetKeyState('P'));
		break;
		}
	}
	else if(num==1)
	{
		switch(inputid)
		{
		case GAMEINPUT_LEFT:
			return(kGUI::GetKeyState('Z'));
		break;
		case GAMEINPUT_RIGHT:
			return(kGUI::GetKeyState('X'));
		break;
		case GAMEINPUT_THRUST:
			return(kGUI::GetKeyState('A'));
		break;
		case GAMEINPUT_FIRE:
			return(kGUI::GetKeyState('S'));
		break;
		case GAMEINPUT_PAUSE:
			return(kGUI::GetKeyState('P'));
		break;
		}
	}
	return(false);
}

void Game::DrawModel(double x,double y,double h,kGUIColor color,GameModel *model,double scale,double fat)
{
	unsigned int i;
	bool kick;
	kGUIMatrix33 rmat;
	kGUIMatrix33 smat;
	kGUIMatrix33 mat;
	kGUIVector3 p1;
	kGUIVector3 p2;
	kGUIVector3 lp;
	unsigned int numpoints=model->m_num;
	kGUIVector3 *lines=model->m_points;

	if(scale==1.0f)
		mat.SetRotZ(h);
	else
	{
		smat.SetScale(scale);
		rmat.SetRotZ(h);
		rmat.Mult(&smat,&mat);
	}
	mat.SetTX(m_cx+x+m_lx);
	mat.SetTY(m_cy+y+m_ty);
	/* project the first point */
	mat.Transform(lines,&p1);
	lp=p1;
	kick=lines->m_z==0.0f;
	++lines;
	for(i=1;i<numpoints;++i)
	{
		mat.Transform(lines,&p2);
		if(kick)
		{
//			if(scale==1.0f)
				kGUI::DrawLine(p1.m_x,p1.m_y,p2.m_x,p2.m_y,color);
//			else
//				kGUI::DrawFatLine(p1.m_x,p1.m_y,p2.m_x,p2.m_y,color,scale/2.0f);
		}
		kick=lines->m_z==0.0f;
		p1=p2;
		++lines;
	}
	if(kick)
	{
//		if(scale==1.0f)
			kGUI::DrawLine(p2.m_x,p2.m_y,lp.m_x,lp.m_y,color);
//		else
//			kGUI::DrawFatLine(p2.m_x,p2.m_y,lp.m_x,lp.m_y,color,scale/2.0f);
	}

#if 0
	lp.m_x=-10.0f;
	lp.m_y=0.0f;
	mat.Transform(&lp,&p1);
	lp.m_x=10.0f;
	lp.m_y=0.0f;
	mat.Transform(&lp,&p2);
	kGUI::DrawLine(p1.m_x,p1.m_y,p2.m_x,p2.m_y,DrawColor(255,0,0));
	lp.m_x=0.0f;
	lp.m_y=-10.0f;
	mat.Transform(&lp,&p1);
	lp.m_x=0.0f;
	lp.m_y=10.0f;
	mat.Transform(&lp,&p2);
	kGUI::DrawLine(p1.m_x,p1.m_y,p2.m_x,p2.m_y,DrawColor(255,0,0));
#endif
}

void Game::PlaySound(unsigned int sound)
{
	kGUIAudioFormat *a=&m_sound[sound];

	m_audiomanager.Play(a->GetSoundRate(),a->GetNumSoundChannels(),a->GetSoundData(),a->GetSoundLength(),false,false);
}

GameAudioHandle Game::StartSound(unsigned int sound)
{
	kGUIAudioFormat *a=&m_sound[sound];

	return(m_audiomanager.Play(a->GetSoundRate(),a->GetNumSoundChannels(),a->GetSoundData(),a->GetSoundLength(),false,true));
}

void Game::StopSound(GameAudioHandle handle)
{
	m_audiomanager.Stop(handle);
}

class Space
{
public:
	Space();
	~Space();
private:
	Game m_game;
};

Space *g_Space;
BigFile *g_bf;

void AppInit(void)
{
	kGUI::LoadFont("font.ttf",false);	/* use default font inside kgui for regulsr */
	kGUI::LoadFont("font.ttf",true);	/* use default font inside kgui for bold */
//	kGUI::LoadFont("font.ttf");	/* use default font inside kgui */
//	kGUI::LoadFont("fontb.ttf");	/* use default bold font */
	kGUI::SetDefFontSize(11);
	kGUI::SetDefReportFontSize(11);

	g_bf=new BigFile();
	//code was changed from using local bigfile to having the data included right into the code
#ifdef USE_EXTERNAL_DATA
	g_bf->SetFilename("data.big");
#else
	g_bf->SetMemory(bin_data,sizeof(bin_data));
#endif
	g_bf->Load();
	DataHandle::AddBig(g_bf);

	g_Space=new Space();
}

void AppClose(void)
{
	delete g_Space;
}

Space::Space()
{
	kGUIWindowObj *background;
	int w,h;

	/* get pointer to the background window object */
	background=kGUI::GetBackground();
 	background->SetTitle("Space");

	w=background->GetChildZoneW();
	h=background->GetChildZoneH();
	m_game.SetPos(0,0);
	m_game.SetSize(w,h);
	m_game.SetNumPlayers(2);
	m_game.Init(w,h);
//	m_game.Start();
	m_game.StartTitle();
	background->SetEventHandler(&m_game,CALLBACKCLASSNAME(Game,Event));
	kGUI::AddEvent(&m_game,CALLBACKCLASSNAME(Game,Update));
	background->AddObject(&m_game);

	kGUI::ShowWindow();
}

Space::~Space()
{
	kGUI::DelEvent(&m_game,CALLBACKCLASSNAME(Game,Update));
}
