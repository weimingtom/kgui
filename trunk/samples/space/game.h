#ifndef __GAME__
#define __GAME__

#define MAXSPEED 10.f
#define MAXSHOTS 5
#define SHOTLIFE 60
#define MAXROCKS 100
#define SHOTDELAY 10
#define NUMROCKSIZES 3
#define NUMROCKMODELS 4
#define MAXFONTCHARS 256
#define MAXPARTICLES 100

#ifndef PI
#define PI 3.141592654f
#endif

enum
{
SOUND_BEAT1,
SOUND_BEAT2,
SOUND_THRUST,
SOUND_SHOT,
SOUND_EXPLODE,
SOUND_NUMSOUNDS};

enum
{
OBJTYPE_SHIP,
OBJTYPE_SHOT,
OBJTYPE_UFOSHOT,
OBJTYPE_ROCK,
OBJTYPE_UFO};

class GameModel
{
public:
	int m_num;
	GameVector3 *m_points;
};

/* this is seperated out so we can have a generic class for collision detection between objects */
class GameObject
{
public:
	void SetModel(GameModel *model);
	virtual void Collide(GameObject *o)=0;
	class GameBase *m_game;
	unsigned int m_type;
	bool m_active;
	double m_x;
	double m_y;
	double m_heading;
	int m_collpoints;
	double m_minradius;
	double m_maxradius;
	GameModel *m_model;
};

class Rock : public GameObject
{
public:
	void Collide(GameObject *o);
	int m_size;
	double m_headingspeed;
	double m_speedx;
	double m_speedy;
};

class Shot : public GameObject
{
public:
	void Collide(GameObject *o);
	int m_life;
	class Ship *m_ship;	/* so we know who to assign the score to */
	double m_speedx;
	double m_speedy;
};

class Ufo : public GameObject
{
public:
	void Collide(GameObject *o);
	double m_heading;
	double m_curheading;
	int m_segmentlen;
	int m_appeardelay;
	bool m_appearside;
	int m_shotdelay;
	Shot m_shot[MAXSHOTS];
};

class Ship : public GameObject
{
public:
	void Collide(GameObject *o);
	int m_index;
	unsigned int m_state;
	int m_statedelay;
	int m_shotdelay;
	unsigned int m_life;
	double m_speedx;
	double m_speedy;
	int m_showthrust;
	bool m_thrustsound;
	unsigned int m_score;
	bool m_draw;
	GameColor m_color;
	GameAudioHandle m_thrusthandle;
	Shot m_shot[MAXSHOTS];
};

class GameParticle
{
public:
	GameParticle *m_prev;
	GameParticle *m_next;
	unsigned int m_life;
	double m_x,m_y;
	double m_speedx,m_speedy;
	GameColor m_color;
};


#define MAXPLAYERS 4

/* +1 is for the UFO */
#define TOTALOBJECTS ((MAXPLAYERS*(MAXSHOTS+1))+MAXROCKS+(1+MAXSHOTS))

class GameBase
{
public:
	GameBase();
	virtual ~GameBase();

	enum INPUTID
	{
	GAMEINPUT_LEFT,
	GAMEINPUT_RIGHT,
	GAMEINPUT_THRUST,
	GAMEINPUT_FIRE,
	GAMEINPUT_PAUSE};

	enum
	{
	STATE_APPEAR,
	STATE_PLAY,
	STATE_DIE,
	STATE_DEAD};

	/* kGUIObj functions go here */ 
	void Draw(unsigned int w,unsigned int h);
	void Update(void);
	void CheckWrap(double *px,double *py);

	/* derived class functions go here */
	virtual void DrawRect(double lx,double ty,double rx,double by,GameColor color)=0;
	virtual void DrawModel(double x,double y,double h,GameColor color,GameModel *model,double scale,double fat)=0;
	virtual bool IsPressed(unsigned int num,INPUTID inputid)=0;
	virtual double Rand(double low,double high)=0;
	virtual unsigned int Rand(unsigned int maxvalue)=0;
	virtual void PauseRequest(unsigned int num)=0;
	virtual void GameOver(void)=0;

	/* class functions */	
	void Init(int w,int h);
	unsigned int GetMaxPlayers(void) {return MAXPLAYERS;}
	unsigned int GetNumPlayers(void) {return m_numplayers;}
	void SetNumPlayers(unsigned int np) {m_numplayers=np;}
	void Start(void);
	void StartTitle(void);
	void StartLevel(void);
	void GenerateRock(Rock *rock);
	void Stop(void) {m_playing=false;}
	unsigned int GetScore(unsigned int num) {return m_ship[num].m_score;}
	void RockHit(Rock *rock);
	void Explode(GameObject *obj,GameColor color);
	double StringPix(const char *string,double scale);
	void DrawString(const char *string,double scale,double x,double y,GameColor color);
	void DrawStringC(const char *string,double scale,double x,double y,GameColor color);
	bool IsInside(GameObject *o1,GameObject *o2);
	GameParticle *AddParticle(double x,double y,double heading,double speed);

	void SetPaused(bool p) {m_paused=p;}
	bool GetPaused(void) {return m_paused;}

	bool GetPlaying(void) {return m_playing;}

	/* allow game to have different number of lives */
	void SetNumLives(unsigned int n) {m_lives=n;}
	unsigned int GetNumLives(void) {return m_lives;}

	void SetScreenCenter(double x,double y) {m_cx=x,m_cy=y;}

	/* sounds */
	virtual void PlaySound(unsigned int sound)=0;
	virtual GameAudioHandle StartSound(unsigned int sound)=0;
	virtual void StopSound(GameAudioHandle handle)=0;

	/* used for frontend lobby to show colors for players */
	GameColor GetColor(unsigned int n) {return m_shipcolors[n];}
	GameModel *GetShipModel(void);
protected:
	static GameColor m_shipcolors[MAXPLAYERS];

	bool m_title;		/* show rocks only for background of title screen */
	bool m_playing;
	bool m_paused;
	bool m_gameover;
	int m_level;
	unsigned int m_lives;
	unsigned int m_numplayers;
	unsigned int m_numplayersleft;
	unsigned int m_numrocks;
	Ship m_ship[MAXPLAYERS];
	Rock m_rock[MAXROCKS];
	Ufo m_ufo;
	GameObject *m_objects[TOTALOBJECTS];
	GameModel *m_fontmodel[MAXFONTCHARS];
	GameParticle m_freephead;
	GameParticle m_freeptail;
	GameParticle m_usedphead;
	GameParticle m_usedptail;
	GameParticle m_particle[MAXPARTICLES];
	int m_w;
	int m_h;
	double m_cx;
	double m_cy;
	bool m_thrustsound;
	int m_beat;
	int m_beatcount;
	int m_beatdelay;
};

#endif
