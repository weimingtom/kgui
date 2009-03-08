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

kGUIAudioManager::kGUIAudioManager()
{
	m_counter=0;
	m_audiopool.SetBlockSize(16);
	m_audiobufferpool.SetBlockSize(32);

	m_close=false;
	m_numactive=0;
	m_activelist.Init(16,16);
	m_thread.Start(this,CALLBACKNAME(UpdateThread));
}

void kGUIAudioManager::Init(void)
{
}

void kGUIAudioManager::UpdateThread(void)
{
	while(m_close==false)
	{
		unsigned int i;
		kGUIAudio *a;

		kGUI::Sleep(1);

		m_mutex.Lock();
		for(i=0;i<m_numactive;++i)
		{
			a=m_activelist.GetEntry(i);
			a->Update();
		}
		m_mutex.UnLock();
	}
	m_thread.Close(false);
}

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
	while(m_thread.GetActive())
		kGUI::Sleep(1);

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
#endif
	m_numbuffers=0;
	m_playbuffer=0;
	m_buffers.Init(4,4);
}

#if defined (WIN32) || defined(MINGW)
static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance,DWORD dwParam1, DWORD dwParam2)
{
	kGUIAudio *a;

	a =(kGUIAudio *)dwInstance;
	if(a->GetPlaying())
		a->SetPlayDone();
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
	
	while(m_mutex.TryLock()==false)
		kGUI::Sleep(1);

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
#if 0
	if(copy && m_numbuffers>m_playbuffer)
	{
		ab=m_buffers.GetEntry(m_numbuffers-1);
		ab->Append(samplesize,sample);
	}
	else
	{
#endif
		/* buffer sample data */
		ab=m_manager->GetBuffer();
		m_buffers.SetEntry(m_numbuffers,ab);
		ab->Set(samplesize,sample,copy);
		++m_numbuffers;
//	}

#if 1
	PlayBuffer();
#else
	if(m_playing==false)
		PlayBuffer();
	else if(m_playing==true && m_playidle==true)
	{
		m_playidle=false;
		PlayBuffer();
	}
#endif
	m_mutex.UnLock();
//	kGUI::Trace("Play:UnLock\n");

	//Update();
}

void kGUIAudio::PlayBuffer(void)
{
#if defined (WIN32) || defined(MINGW)
	AudioBuffer *ab;

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
		if(waveOutOpen(&m_hWaveOut,WAVE_MAPPER,&m_wfx,(DWORD_PTR)waveOutProc,(DWORD_PTR)this,CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
		{
			assert(false, "unable to open WAVE_MAPPER device");
		}
	}
	m_playing=true;

	ab=m_buffers.GetEntry(m_playbuffer);
//	kGUI::Trace("Playing Buffer %08x #%d,len=%d\n",this,m_playbuffer,ab->GetLen());
	++m_playbuffer;

	ZeroMemory(&ab->m_header, sizeof(WAVEHDR));
	ab->m_header.dwBufferLength = ab->GetLen()/sizeof(short);
	ab->m_header.lpData = (LPSTR)ab->GetBuffer();

	waveOutPrepareHeader(m_hWaveOut, &ab->m_header, sizeof(WAVEHDR));
	waveOutWrite(m_hWaveOut, &ab->m_header, sizeof(WAVEHDR));
#endif
}

void kGUIAudio::Update(void)
{
	if(m_playing && m_playdone>0)
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
					m_manager->ReleaseBuffer(ab,false);
					m_buffers.DeleteEntry(0,1);
					--m_playbuffer;
					--m_numbuffers;
					--m_playdone;
				}
			}
			else
				m_playdone=0;
			if(m_playbuffer<m_numbuffers)
			{
				//kGUI::Trace("Done:PlayBuffer\n");
				//PlayBuffer();
			}
			else
			{
				if(m_loop)
				{
					/* back to the first buffer! */
					m_playbuffer=0;

					//todo: send all packets again?
					PlayBuffer();
					//kGUI::Trace("Done:PlayBuffer(loop)\n");
				}
				else
				{
//					kGUI::Trace("Done:Stop %08x\n",this);
					if(m_release)
						Stop(false);
					else
						m_playidle=true;
				}
			}
			//kGUI::Trace("Done:PreUnlock\n");
			m_mutex.UnLock();
		}
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
