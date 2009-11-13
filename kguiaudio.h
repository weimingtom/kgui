#ifndef __KGUIAUDIO__
#define __KGUIAUDIO__

/**********************************************************************************/
/* kGUI - kguiaudio.h                                                                  */
/*                                                                                */
/* Programmed by Kevin Pickell, started September 2005                            */
/*                                                                                */
/* http://code.google.com/p/kgui/	                                              */
/*                                                                                */
/*    kGUI is free software; you can redistribute it and/or modify                */
/*    it under the terms of the GNU Lesser General Public License as published by */
/*    the Free Software Foundation; version 2.                                    */
/*                                                                                */
/*    kGUI is distributed in the hope that it will be useful,                     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/*    GNU General Public License for more details.                                */
/*                                                                                */
/*    http://www.gnu.org/licenses/lgpl.txt                                        */
/*                                                                                */
/*    You should have received a copy of the GNU General Public License           */
/*    along with kGUI; if not, write to the Free Software                         */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */
/*                                                                                */
/**********************************************************************************/

#if defined (WIN32) || defined(MINGW)
#include <mmsystem.h>
#elif defined(LINUX)
#include <alsa/asoundlib.h>
#undef assert
#define USETHREAD 1
#endif

class kGUIAudioManager;

enum
{
AUDIOFORMAT_WAVE,
AUDIOFORMAT_UNKNOWN
};

class kGUIAudioFormat : public DataHandle
{
public:
	kGUIAudioFormat();
	void Load(void);
	unsigned int GetSoundFormat(void) {return m_soundformat;}
	unsigned int GetNumSoundChannels(void)  {return m_numsoundchannels;}
	unsigned int GetSoundRate(void)  {return m_soundrate;}
	unsigned int GetSoundLength(void)  {return m_soundlength;}
	const unsigned char *GetSoundData(void)  {return m_soundbuffer.GetArrayPtr();}
private:
	unsigned int m_soundformat;
	unsigned int m_numsoundchannels;
	unsigned int m_soundrate;
	unsigned int m_soundlength;
	Array<unsigned char>m_soundbuffer;
};

class AudioBuffer
{
public:
	AudioBuffer() {m_copy=false;m_buffer=0;m_len=0;m_playindex=0;}
	void Set(unsigned int len,const unsigned char *buffer,bool copy);
	void Append(unsigned int len,const unsigned char *buffer);
	unsigned int GetLen(void) {return m_len;}
	const unsigned char *GetBuffer(void) {return (m_copy==true?m_copybuffer.GetArrayPtr():m_buffer);}
	unsigned int GetPlayIndex(void) {return m_playindex;}
	void SetPlayIndex(unsigned int index) {m_playindex=index;}
#if defined (WIN32) || defined(MINGW)
	void Play(HWAVEOUT handle);
	void Release(HWAVEOUT handle);
#endif
private:
#if defined (WIN32) || defined(MINGW)
    WAVEHDR m_header;
#endif
	int m_len;
	bool m_copy;
	Array<unsigned char>m_copybuffer;
	const unsigned char *m_buffer;
	unsigned int m_playindex;
};

class kGUIAudio
{
public:
	kGUIAudio();
	~kGUIAudio();
	void SetManager(kGUIAudioManager *manager) {m_manager=manager;}
	void Init(void);
	void Play(int rate,int channels,const unsigned char *sample,unsigned long samplesize,bool copy,bool loop=false);
	void PlayBuffer(void);
	void Update(void);
	void SetPlayDone(void) {++m_playdone;}
	void SetPause(bool p);
	bool GetPause(void) {return m_paused;}
	void Stop(bool needmutex=true);
	void PlayDone(void);
	void SetRelease(bool r) {m_release=r;}
	bool GetPlaying(void) {return m_playing;}
	unsigned int GetCounter(void) {return m_counter;}
	void SetCounter(unsigned int counter) {m_counter=counter;}
	
	unsigned int GetRate(void) {return m_rate;}
	void SetRate(unsigned int rate) {m_rate=rate;}

	unsigned int GetChannels(void) {return m_channels;}
	void SetChannels(unsigned int channels) {m_channels=channels;}
private:
#if defined (WIN32) || defined(MINGW)
	HWAVEOUT m_hWaveOut;	/* device handle */
    WAVEFORMATEX m_wfx;		/* look this up in your documentation */
#elif defined(LINUX)
	snd_pcm_t *m_handle;
	snd_pcm_hw_params_t *m_params;
	snd_async_handler_t *m_callback;
	int m_samplesize;
	int m_lastavail,m_laststate;
#endif
	kGUIMutex m_mutex;
	kGUIAudioManager *m_manager;
	unsigned int m_channels;
	unsigned int m_rate;
	unsigned int m_counter;
	bool m_loop;
	bool m_playing;
	bool m_paused;
	unsigned int m_playdone;
	bool m_playidle;
	bool m_release;
	int m_playbuffer;
	int m_numbuffers;
	Array<AudioBuffer *>m_buffers;
};

class kGUIAudioHandle
{
public:
	kGUIAudioHandle() {m_obj=0;m_counter=0;}
	bool GetIsValid(void);
	bool GetIsPlaying(void);
	kGUIAudio *GetAudio(void) {return (m_obj);}
	kGUIAudio *m_obj;
	unsigned int m_counter;
};

class kGUIAudioManager
{
public:
	kGUIAudioManager();
	~kGUIAudioManager();
	void Init(void);

	/* these are used to hold on to an audio channel until we release it */
	kGUIAudio *GetAudio(void) {kGUIAudio *a;m_mutex.Lock();a=m_audiopool.PoolGet();a->SetManager(this);a->SetRelease(false);m_activelist.SetEntry(m_numactive++,a);m_mutex.UnLock();return(a);}
	void ReleaseAudio(kGUIAudio *a,bool lock=true) {if(lock)m_mutex.Lock();a->SetRelease(false);a->Stop();m_audiopool.PoolRelease(a);m_activelist.Delete(a);--m_numactive;if(lock)m_mutex.UnLock();}

	/* these are used to hold on to an audio channel until we release it */
	AudioBuffer *GetBuffer(void) {AudioBuffer *a;m_mutex.Lock();a=m_audiobufferpool.PoolGet();m_mutex.UnLock();return(a);}
	void ReleaseBuffer(AudioBuffer *a,bool needlock=true) {if(needlock)m_mutex.Lock();m_audiobufferpool.PoolRelease(a);if(needlock)m_mutex.UnLock();}

	/* this will release it when it has finished playing */
	kGUIAudioHandle Play(int rate,int channels,const unsigned char *sample,unsigned long samplesize,bool copy,bool loop=false);
	void Stop(kGUIAudioHandle ah);
private:
	void UpdateThread(void);
	CALLBACKGLUE(kGUIAudioManager,UpdateThread);
	unsigned int m_counter;
	kGUIMutex m_mutex;
	ClassPool<kGUIAudio>m_audiopool;
	ClassPool<AudioBuffer>m_audiobufferpool;
	unsigned int m_numactive;
	Array<kGUIAudio *>m_activelist;
	bool m_close;
	kGUIThread m_thread;
};


#endif
