/**********************************************************************************/
/* kGUI - kguiaudio.cpp                                                           */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
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

#include "kgui.h"
#include "kguiaudio.h"

void AudioBuffer::Set(unsigned int len,const unsigned char *buffer,bool copy)
{
	m_len=len;
	m_copy=copy;
	if(copy==false)
		m_buffer=buffer;
	else
	{
		m_copybuffer.Alloc(len);
		memcpy(m_copybuffer.GetArrayPtr(),buffer,len);
	}
}

void AudioBuffer::Append(unsigned int len,const unsigned char *buffer)
{
	unsigned int newlen;

	assert(m_copy==true,"Can only append to a copy buffer!");
	newlen=m_len+len;
	m_copybuffer.Alloc(newlen,true);
	memcpy(m_copybuffer.GetArrayPtr()+m_len,buffer,len);
	m_len=newlen;
}

#if defined (WIN32) || defined(MINGW)
void AudioBuffer::Play(HWAVEOUT handle)
{
	ZeroMemory(&m_header, sizeof(m_header));
	m_header.dwBufferLength = GetLen()/sizeof(short);
	m_header.lpData = (LPSTR)GetBuffer();

	waveOutPrepareHeader(handle, &m_header, sizeof(m_header));
	waveOutWrite(handle, &m_header, sizeof(m_header));
}

void AudioBuffer::Release(HWAVEOUT handle)
{
	waveOutUnprepareHeader(handle, &m_header, sizeof(m_header));
}
#endif

kGUIAudioManager::kGUIAudioManager()
{
	m_counter=0;
	m_audiopool.SetBlockSize(16);
	m_audiobufferpool.SetBlockSize(32);

	m_close=false;
	m_numactive=0;
	m_activelist.Init(16,16);
#if USETHREAD
	m_thread.Start(this,CALLBACKNAME(UpdateThread));
#endif
}

void kGUIAudioManager::Init(void)
{
}

#if USETHREAD
void kGUIAudioManager::UpdateThread(void)
{
	while(m_close==false)
	{
		unsigned int i;
		kGUIAudio *a;

		kGUI::Sleep(10);

		m_mutex.Lock();
//		kGUI::Trace("UpdateThread\n");

		for(i=0;i<m_numactive;++i)
		{
			a=m_activelist.GetEntry(i);
			a->Update();
		}
		m_mutex.UnLock();
	}
	m_thread.Close(false);
}
#endif

kGUIAudioHandle kGUIAudioManager::Play(int rate,int channels,const unsigned char *sample,unsigned long samplesize,bool copy,bool loop)
{
	kGUIAudioHandle ah;
	kGUIAudio *a;

	a=GetAudio();
	a->SetManager(this);
	a->SetCounter(m_counter);

	a->Play(rate,channels,sample,samplesize,copy,loop);

	/* todo: add to active list */
	ah.m_obj=a;
	ah.m_counter=m_counter++;
	return(ah);
}

/* since audio objects are re-used when finished we use a handle to make */
/* sure it is still a valid pointer */
void kGUIAudioManager::Stop(kGUIAudioHandle ah)
{
	kGUIAudio *a;

	a=ah.m_obj;
	/* is this still valid? */
	if(a->GetCounter()==ah.m_counter)
		a->Stop();
}


kGUIAudioManager::~kGUIAudioManager()
{
	kGUIAudio *a;

	m_close=true;
#if USETHREAD
	while(m_thread.GetActive())
		kGUI::Sleep(1);
#endif

	m_mutex.Lock();
	while(m_numactive)
	{
		a=m_activelist.GetEntry(0);
		ReleaseAudio(a,false);
	}
	m_mutex.UnLock();
}

/******************************************************/

kGUIAudio::kGUIAudio()
{
	m_playing=false;
	m_playdone=0;
	m_playidle=false;
	m_release=false;
#if defined (WIN32) || defined(MINGW)
	m_hWaveOut=0;
#elif defined(LINUX)
	m_handle=0;
	m_params=0;
	m_lastavail=0;
	m_laststate=0;
#endif
	m_numbuffers=0;
	m_playbuffer=0;
	m_buffers.Init(4,4);
}

#if defined (WIN32) || defined(MINGW)

union dtoa_def
{
	DWORD dw;
	kGUIAudio *ap;
};

static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance,DWORD dwParam1, DWORD dwParam2)
{
	kGUIAudio *a;
	dtoa_def dtoa;

	/* use a union to change DWORD to a pointer to avoid compile errors */
	dtoa.dw=dwInstance;
	a=dtoa.ap;

	if(a->GetPlaying())
	{
		a->SetPlayDone();
#if UPDATETHREAD
#else
		a->Update();
#endif
	}
}
#endif

void kGUIAudio::Init(void)
{
	m_rate=44100;
	m_channels=1;
}

void kGUIAudio::Play(int rate,int channels,const unsigned char *sample,unsigned long samplesize,bool copy,bool loop)
{
	AudioBuffer *ab;

//	kGUI::Trace("Play:Lock (%s)\n",m_mutex.GetIsLocked()==true?"locked":"unlocked");
	
	m_mutex.Lock();

	if(m_playing==false)
	{
//		kGUI::Trace("Play:%08x, Not Playing, reset packets!\n",this);
		m_playbuffer=0;
		m_numbuffers=0;
	}
	m_rate=rate;
	m_channels=channels;
	m_loop=loop;

//	kGUI::Trace("Play:%08x, Adding Packet, rate=%d,channels=%d,size=%d\n",this,rate,channels,samplesize);

	/* can we append this packet to the last pending packet? */
	if(copy && m_numbuffers>m_playbuffer)
	{
		ab=m_buffers.GetEntry(m_numbuffers-1);
		ab->Append(samplesize,sample);
	}
	else
	{
		/* buffer sample data */
		ab=m_manager->GetBuffer();
		m_buffers.SetEntry(m_numbuffers,ab);
		ab->Set(samplesize,sample,copy);
		++m_numbuffers;
	}

	PlayBuffer();
	m_mutex.UnLock();
}

void kGUIAudio::PlayBuffer(void)
{
	AudioBuffer *ab;
#if defined (WIN32) || defined(MINGW)
	int wrc;

	if(m_playing==false)
	{
		/*
		* first we need to set up the WAVEFORMATEX structure. 
		* the structure describes the format of the audio.
		*/
		m_wfx.nSamplesPerSec = m_rate; /* sample rate */
		m_wfx.wBitsPerSample = 16; /* sample size */
		m_wfx.nChannels = m_channels; /* channels*/
		/*
		* WAVEFORMATEX also has other fields which need filling.
		* as long as the three fields above are filled this should
		* work for any PCM (pulse code modulation) format.
		*/
		m_wfx.cbSize = 0; /* size of _extra_ info */
		m_wfx.wFormatTag = WAVE_FORMAT_PCM;
		m_wfx.nBlockAlign = (m_wfx.wBitsPerSample >> 3) * m_wfx.nChannels;
		m_wfx.nAvgBytesPerSec = m_wfx.nBlockAlign * m_wfx.nSamplesPerSec;
		/*
		* try to open the default wave device. WAVE_MAPPER is
		* a constant defined in mmsystem.h, it always points to the
		* default wave device on the system (some people have 2 or
		* more sound cards).
		*/
		wrc=waveOutOpen(&m_hWaveOut,WAVE_MAPPER,&m_wfx,(DWORD_PTR)waveOutProc,(DWORD_PTR)this,CALLBACK_FUNCTION);
		//assert(wrc==MMSYSERR_NOERROR, "unable to open WAVE_MAPPER device");
		if(wrc!=MMSYSERR_NOERROR)
			return;		//not sure how to handle this, maybe re-try in an update function??
	}
	m_playing=true;

	ab=m_buffers.GetEntry(m_playbuffer);
//	kGUI::Trace("Playing Buffer %08x #%d,len=%d\n",this,m_playbuffer,ab->GetLen());
	++m_playbuffer;
	ab->Play(m_hWaveOut);
#elif defined(LINUX)
	unsigned int playindex;
	int ret;
	unsigned int playsize;

	if(m_playing==false)
	{
		kGUI::Trace("Playing Buffer this=%08x, rate=%d, channels=%d\n",this,m_rate,m_channels);

		if(snd_pcm_open (&m_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)<0)
		{
			kGUI::Trace("snd_pcm_open error\n");
			return;
		}

		if (snd_pcm_hw_params_malloc (&m_params) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_malloc error\n");
			return;
		}
				 
		if (snd_pcm_hw_params_any (m_handle, m_params) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_any error\n");
			return;
		}

		if (snd_pcm_hw_params_set_access (m_handle, m_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_set_access error\n");
			return;
		}
	
		if (snd_pcm_hw_params_set_format (m_handle, m_params, SND_PCM_FORMAT_S16_LE) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_set_format error\n");
			return;
		}
	
		if (snd_pcm_hw_params_set_rate_near (m_handle, m_params, &m_rate, 0) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_set_rate_near error\n");
			return;
		}
	
		if (snd_pcm_hw_params_set_channels (m_handle, m_params, m_channels) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params_set_channels error\n");
			return;
		}
	
		if (snd_pcm_hw_params (m_handle, m_params) < 0)
		{
			kGUI::Trace("snd_pcm_hw_params error\n");
			return;
		}
	
		snd_pcm_hw_params_free (m_params);
		m_params=0;
		m_samplesize=sizeof(short)*m_channels;
	}

	//get maximum amount that we can play in bytes!
	ab=m_buffers.GetEntry(m_playbuffer);
	playindex=ab->GetPlayIndex();
	playsize=ab->GetLen()-playindex;

//	kGUI::Trace("Playing Buffer this=%08x, ab=%08x, #%d,offset=%d,len=%d of %d\n",this,ab,m_playbuffer,playindex,playsize,ab->GetLen());
	
	snd_pcm_prepare (m_handle);
	ret=snd_pcm_writei (m_handle, ab->GetBuffer()+playindex, playsize/m_samplesize);
	if(ret<0)
	{
		snd_pcm_recover(m_handle,ret,1);
	}
	else
	{
		playsize=ret*m_samplesize;
//		kGUI::Trace("Playing Buffer actual playsize=%d\n",playsize);
		if(m_playing==false)
		{
			m_playing=true;
			snd_pcm_start(m_handle);
		}
		ab->SetPlayIndex(playindex+playsize);
		if((playindex+playsize)==ab->GetLen())
		{
			/* finished with this buffer! */
			ab->SetPlayIndex(0);
			if(m_loop==false)
			{
				/* since we are not looping we can just free it up */
				ab=m_buffers.GetEntry(0);
				m_manager->ReleaseBuffer(ab,false);
				m_buffers.DeleteEntry(0,1);
				--m_numbuffers;
			}
			else
			{
				++m_playbuffer;	/* move on to next buffer */
				if(m_playbuffer==m_numbuffers)
					m_playbuffer=0;
			}
		}
	}
#endif
}

void kGUIAudio::Update(void)
{
	if(m_playing)
	{
#ifdef LINUX
		m_mutex.Lock();
//		kGUI::Trace("Update - Playbuffer=%d,NumBuffers=%d\n",m_playbuffer,m_numbuffers);
		if(m_playbuffer<m_numbuffers)
		{
			/* try feeding more! */
//			kGUI::Trace("Update - Calling PlayBuffer\n");
			PlayBuffer();
		}
		else if(m_playbuffer==m_numbuffers && m_playdone==0)
		{
			//since we can't get any callback to tell us when this is done
			//we will just check this way!
			int avail,state;

			avail=snd_pcm_avail_update(m_handle);
		 	state=snd_pcm_state(m_handle);
			if(avail!=m_lastavail || state!=m_laststate)
			{
//				kGUI::Trace("kGUIAudio::Update avail=%d,state=%d\n",avail,state);
				m_lastavail=avail;
				m_laststate=state;
			}
			if(state!=SND_PCM_STATE_RUNNING)
			{
				kGUI::Trace("kGUIAudio::Update not running detected\n");
				if(m_loop==false)
				{
					if(m_release)
						Stop(false);
					else
						m_playidle=true;
				}
			}
		}
		m_mutex.UnLock();
#elif defined (WIN32) || defined(MINGW)
		if(m_playdone>0)
		{
			if(m_mutex.TryLock())
			{
				/* release the play buffer if we are not looping */
				if(m_loop==false)
				{
					AudioBuffer *ab;

					while(m_playdone>0)
					{
						ab=m_buffers.GetEntry(0);
						ab->Release(m_hWaveOut);
						m_manager->ReleaseBuffer(ab,false);
						m_buffers.DeleteEntry(0,1);
						--m_playbuffer;
						assert(m_numbuffers!=0,"Underflow error!");
						--m_numbuffers;
						--m_playdone;
					}
				}
				else
					m_playdone=0;
				if(m_playbuffer==m_numbuffers)
				{
					if(m_loop)
					{
						/* back to the first buffer! */
						m_playbuffer=0;
						PlayBuffer();
					}
					else
					{
						if(m_release)
							Stop(false);
						else
							m_playidle=true;
					}
				}
				m_mutex.UnLock();
			}
		}
#endif
	}
}

void kGUIAudio::Stop(bool needmutex)
{
	if(m_playing)
	{
		if(needmutex)
		{
//			kGUI::Trace("Stop:Lock\n");
			m_mutex.Lock();
		}
		m_playing=false;
#if defined (WIN32) || defined(MINGW)
		waveOutReset(m_hWaveOut);
		waveOutClose(m_hWaveOut);
#elif defined(LINUX)
		snd_pcm_drop (m_handle);
		snd_pcm_close (m_handle);
		m_handle=0;
#endif
		if(m_release)
			m_manager->ReleaseAudio(this);
		if(needmutex)
		{
			m_mutex.UnLock();
//			kGUI::Trace("Stop:UnLock\n");
		}
	}
}

kGUIAudio::~kGUIAudio()
{
#if defined (WIN32) || defined(MINGW)
	if(m_playing)
		Stop();
#endif
}

/*****************************************************************/

bool kGUIAudioHandle::GetIsValid(void)
{
	if(m_obj==0)
		return(false);
	return(m_obj->GetCounter()==m_counter);
}

bool kGUIAudioHandle::GetIsPlaying(void)
{
	if(GetIsValid()==false)
		return(false);
	return(m_obj->GetPlaying());
}

/*****************************************************************/

kGUIAudioFormat::kGUIAudioFormat()
{
	m_soundformat=AUDIOFORMAT_UNKNOWN;
	m_numsoundchannels=0;
	m_soundrate=0;
	m_soundlength=0;
}

void kGUIAudioFormat::Load(void)
{
	unsigned long fs;
	unsigned char header[12];
	unsigned int bps;
	unsigned int format;
	unsigned int length;

	if(Open())
	{
		fs=GetLoadableSize();

		Read((void *)&header,(unsigned long)sizeof(header));
		if( header[0]=='R' && header[1]=='I' && header[2]=='F' && header[3]=='F' &&
			header[8]=='W' && header[9]=='A' && header[10]=='V' && header[11]=='E')
		{

			fs-=sizeof(header);
			Read((void *)&header,(unsigned long)sizeof(header));
			if( header[0]=='f' && header[1]=='m' && header[2]=='t' && header[3]==' ')
			{
				format=header[8]|(header[9]<<8);
				assert(format==1,"Only Supports Uncompressed PCM");
				m_numsoundchannels=header[10]|(header[11]<<8);

				fs-=sizeof(header);
				Read((void *)&header,(unsigned long)sizeof(header));
		
				m_soundrate=header[0]|(header[1]<<8)|(header[2]<<16)|(header[3]<<24);
				bps=header[10]|(header[11]<<8);

				do
				{
					fs-=8;
					Read((void *)&header,(unsigned long)8);
				
					if( header[0]=='d' && header[1]=='a' && header[2]=='t' && header[3]=='a')
					{
						break;
					}
					length=header[4]|(header[5]<<8)|(header[6]<<16)|(header[7]<<24);
					fs-=length;
					if(fs<=0)
					{
						/* never did find data!! */
						Close();
						return;
					}
					Seek(GetOffset()+length);
				}while(1);

				m_soundformat=AUDIOFORMAT_WAVE;

				switch(bps)
				{
				case 8:
				{
					unsigned int i;
					unsigned char *sb;

					/* expand to 16 bits */
					fs<<=1;
					m_soundlength=fs;
					m_soundbuffer.Alloc(fs);
					sb=m_soundbuffer.GetArrayPtr();
					for(i=0;i<fs;i+=2)
					{
						Read((void *)sb,(unsigned long)1);
						sb[1]=sb[0];
						sb+=2;
					}
				}
				break;
				case 16:
					m_soundlength=fs;
					m_soundbuffer.Alloc(fs);
					Read(m_soundbuffer.GetArrayPtr(),fs);
				break;
				default:
					assert(false,"Unsupported format!");
				break;
				}
			}
		}
		Close();
	}
}
