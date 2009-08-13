#ifndef __KGUIMOVIE__
#define __KGUIMOVIE__

#include "kgui.h"
#include "kguiaudio.h"

enum
{
MOVIEQUALITY_LOW,
MOVIEQUALITY_LOW2,
MOVIEQUALITY_LOW3,
MOVIEQUALITY_MED,
MOVIEQUALITY_MED2,
MOVIEQUALITY_MED3,
MOVIEQUALITY_HIGH};

class kGUIMovie: public DataHandle
{
public:
	kGUIMovie();
	~kGUIMovie();
	/* these global functions need to be called once at program startup and shutdown */
	static void InitGlobals(void);
	static void PurgeGlobals(void);
	static const char *GetVersion(void);

	void SetOutputImage(kGUIImage *image,unsigned int quality=MOVIEQUALITY_MED);
	void SetPlayAudio(bool pa) {m_playaudio=pa;}
	void OpenMovie(void);
	bool GetIsValid(void) {return m_isvalid;}
	unsigned int GetMovieWidth(void) {return m_width;}
	unsigned int GetMovieHeight(void) {return m_height;}

	/* load and buffer frames */
	void UpdateBuffers(void);
	/* show the cached frame */
	bool ShowFrame(void);
	void CloseMovie(void);

	void SetPlaying(bool p);
	bool GetPlaying(void) {return m_playing;}
	void SetLoop(bool l) {m_loop=l;}
	bool GetLoop(void) {return m_loop;}
	bool GetDone(void) {return m_done;}	/* hit end and stopped */
	int GetDuration(void) {return m_duration;}
	int GetTime(void) {return m_time;}
	void Seek(int place);
	unsigned int GetQuality(void) {return m_quality;}
	bool LoadNextFrame(void) {bool rc;UpdateBuffers();rc=ShowFrame();m_time=m_ptime;return(rc);}
	void ReStart(void) {Seek(0);}
private:
	static bool m_initglobals;
	CALLBACKGLUE(kGUIMovie,Event)
	void Event(void);

	kGUIImage *m_image;
	unsigned int m_quality;
	kGUIAudioManager m_audiomanager;
	kGUIAudio *m_audio;

	class kGUIMovieLocal *m_local;

	unsigned int m_numframesready;

	bool m_isvalid:1;
	bool m_playing:1;
	bool m_done:1;
	bool m_loop:1;
	bool m_playaudio:1;
	int m_time;			/* current tick time in TICKSPERSEC */
	int m_ptime;		/* presentation time to show next frame in TICKSPERSEC */
	int m_bufptime;
	unsigned int m_width,m_height;
	unsigned int m_outwidth,m_outheight;
	int m_format;

	int m_videoStream;
	int m_audioStream;
	unsigned char *m_abraw;
	unsigned char *m_abalign;

	int m_frameFinished;
	int m_starttime;
	int m_duration;
	double m_vstreamtimebase;
	double m_vcodectimebase;
	long long m_rawtime;
};

// a renderable movie object with attached controls
class kGUIMovieObj : public kGUIContainerObj, public kGUIMovie
{
public:
	kGUIMovieObj();
	~kGUIMovieObj();
	void OpenMovie(void) {kGUIMovie::OpenMovie();m_moviecontrols.SetMovie(this);}
	void SetMovieSize(int w,int h) {SetSize(MIN(kGUI::GetScreenWidth(),w),MIN(kGUI::GetScreenHeight(),h+m_moviecontrols.GetZoneH()));}
	void Draw(void) {DrawC(0);}
	bool UpdateInput(void) {return UpdateInputC(0);}

	/* this is added to allow parent code to add events to it light right click on image */
	kGUIImageObj *GetImageObj(void) {return &m_image;}
private:
	void CalcChildZone(void);
	kGUIMovieControlObj m_moviecontrols;
	kGUIImageObj m_image;
};

#endif
