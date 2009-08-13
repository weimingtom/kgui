/*********************************************************************************/
/* kGUI - kguimovie.cpp                                                            */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                 */
/*                                                                               */
/*    kGUI is free software; you can redistribute it and/or modify               */
/*    it under the terms of the GNU Lesser General Public License as published by*/
/*    the Free Software Foundation; version 2.                                   */
/*                                                                               */
/*    kGUI is distributed in the hope that it will be useful,                    */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/*    GNU General Public License for more details.                               */
/*                                                                               */
/*    http://www.gnu.org/licenses/lgpl.txt                                       */
/*                                                                               */
/*    You should have received a copy of the GNU General Public License          */
/*    along with kGUI; if not, write to the Free Software                        */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

/*! @file kguimovie.cpp
    @brief This is the class wrapper interface to the ffmpeg movie player.
      special thanks to dranger.com for the online tutorial that I used as the basis for this code. */

/*! @todo Handle audio for Movie Player */

#include "kgui.h"
#include "kguimovie.h"

/* a simple wrapper class for loading movies using ffmpeg */

extern "C"
{
	/****** FFMPEG ******/

	/* windows needs 2 include files that are not standard */
	/* these are in wiffmpeg so any windows projects needs to */
	/* set this as an include directory */

#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
}

#define INT64_C(val) val##LL

#ifndef STRINGIFY
#define STRINGIFY(x)	STRINGIFY1(x)
#define STRINGIFY1(x)	#x
#endif

//return version number, used in credit screens
const char *kGUIMovie::GetVersion(void)
{
	static char ffmpegversion[]={ "avcodec=" STRINGIFY(LIBAVCODEC_VERSION) ", avformat=" STRINGIFY(LIBAVFORMAT_VERSION)", avutil=" STRINGIFY(LIBAVUTIL_VERSION) };

	return ffmpegversion;
}

class kGUIMovieLocal
{
public:
	kGUIMovieLocal() {};
	static int kg_open(URLContext *h, const char *filename, int flags);
	static int kg_read(URLContext *h, unsigned char *buf, int size);
	static int kg_write(URLContext *h, unsigned char *buf, int size);
	static int64_t kg_seek(URLContext *h, int64_t pos, int whence);
	static int kg_close(URLContext *h);
    static int kg_read_pause(URLContext *h, int pause);
    static int64_t kg_read_seek(URLContext *h,int stream_index, int64_t timestamp, int flags);
	static int kg_get_file_handle(URLContext *h);

	static URLProtocol kg_protocol;

	AVFormatContext *m_pFormatCtx;
	ByteIOContext m_pb;
	AVCodecContext *m_pVCodecCtx;	/* video codec context */
	AVCodecContext *m_pACodecCtx;	/* audio codec context or null if movie has no audio */
	AVCodec *m_pVCodec;				/* video codec */
	AVCodec *m_pACodec;				/* audio codec */
	AVFrame *m_pFrame;
	struct SwsContext *m_img_convert_ctx;
	AVPacket m_packet;
};

int kGUIMovieLocal::kg_open(URLContext *h, const char *filename, int flags)
{
	union
	{
	DataHandle *dh;
	unsigned int a;
	}v;

	/* filename is in format of "kg:xxx" where xxx is a pointer to a datahandle */
	v.a=atoi(filename+3);

	h->priv_data=v.dh;

//	kGUI::Trace("kg_open %x\n",dh);
	v.dh->Open();
	return 0;
}

int kGUIMovieLocal::kg_read(URLContext *h, unsigned char *buf, int size)
{
    DataHandle *dh = (DataHandle *)h->priv_data;

	return(dh->Read(buf,(unsigned long)size));
}

int kGUIMovieLocal::kg_write(URLContext *h, unsigned char *buf, int size)
{
    return -1;
}

int64_t kGUIMovieLocal::kg_seek(URLContext *h, int64_t pos, int whence)
{
    DataHandle *dh = (DataHandle *)h->priv_data;

	switch(whence)
	{
	case SEEK_CUR:
		pos+=dh->GetOffset();
	break;
	case SEEK_END:
		pos+=dh->GetSize();
	break;
	case SEEK_SET:
	break;
	default:
		return(-1);	/* huh? */
	break;
	}
	dh->Seek((unsigned long)pos);
	return(pos);
}

int kGUIMovieLocal::kg_close(URLContext *h)
{
    DataHandle *dh = (DataHandle *)h->priv_data;

//	kGUI::Trace("kg_close %x\n",dh);
	dh->Close();
	return 0;
}

int kGUIMovieLocal::kg_read_pause(URLContext *h, int pause)
{
	return(-1);
}

int64_t kGUIMovieLocal::kg_read_seek(URLContext *h,int stream_index, int64_t timestamp, int flags)
{
	return(-1);
}

int kGUIMovieLocal::kg_get_file_handle(URLContext *h)
{
	return(-1);
}

/* this is the list of functions to call when a filename using ffmpeg */
/* starts with the prefix listed, IE: a file "kg:fred.mpg" will use these */
/* load routines to handle it. */

URLProtocol kGUIMovieLocal::kg_protocol = {
    "kg",
	kGUIMovieLocal::kg_open,
    kGUIMovieLocal::kg_read,
    kGUIMovieLocal::kg_write,
    kGUIMovieLocal::kg_seek,
    kGUIMovieLocal::kg_close,
	0,	/* next */
    kGUIMovieLocal::kg_read_pause,
    kGUIMovieLocal::kg_read_seek,
	kGUIMovieLocal::kg_get_file_handle
};

bool kGUIMovie::m_initglobals=false;

void kGUIMovie::InitGlobals(void)
{
	m_initglobals=true;
	// Register all formats and codecs
	av_register_all();
	// register myloadfile protocol
	register_protocol(&kGUIMovieLocal::kg_protocol);
}

void kGUIMovie::PurgeGlobals(void)
{
	m_initglobals=false;
}

union ptoi_def
{
	unsigned char *ucp;
	unsigned int u32;
};

kGUIMovie::kGUIMovie()
{
	unsigned int alignoff;
	ptoi_def ptoi;

	m_local=new kGUIMovieLocal();
	m_width=0;
	m_height=0;
	m_loop=false;

	m_isvalid=false;
	m_playing=false;
	m_image=0;
	m_quality=MOVIEQUALITY_LOW;
	m_playaudio=true;

	m_numframesready=0;
	m_local->m_pFrame=0;

	m_local->m_img_convert_ctx=0;
	m_local->m_pVCodecCtx=0;
	m_local->m_pACodecCtx=0;
	m_local->m_pFormatCtx=0;
	m_time=0;
	m_ptime=0;
	m_audiomanager.Init();
	m_audio=m_audiomanager.GetAudio();
    m_abraw = new unsigned char[((AVCODEC_MAX_AUDIO_FRAME_SIZE+16)*3)/2];
	/* align to 16 byte boundary */
	m_abalign=m_abraw;

	/* use a union to convert a pointer to a integer without warnings */
	ptoi.ucp=m_abalign;
	alignoff=(ptoi.u32)&15;
	if(alignoff)
		m_abalign+=(16-alignoff);
}

kGUIMovie::~kGUIMovie()
{
	CloseMovie();
	delete m_local;

	m_audiomanager.ReleaseAudio(m_audio);
	delete m_abraw;
}

void kGUIMovie::OpenMovie(void)
{
	unsigned int i;
	kGUIString fn;

	assert(kGUIMovie::m_initglobals==true,"Error, you need to call kGUIMovie::InitGlobals() first!\n");
	CloseMovie();
	
	/* since there is no way for us to bind "this" object to the openfile code */
	/* I put a hack in where I pass the object pointer as the filename and then */
	/* the open code casts it to a datahandle object and saves it for future calls */

//	if(!strcmp("004.wmv",GetFilename()))
//		kGUI::Trace("Loading bad movie?\n");

	m_rawtime=0;
	m_time=0;
	m_ptime=0;

	m_numframesready=0;
	m_local->m_packet.size=0;

	/* filename is actually a pointer to a datahandle */
	fn.Sprintf("kg:%d",(DataHandle *)this);

	// Open video file
	if(av_open_input_file(&m_local->m_pFormatCtx, fn.GetString() /*,GetFilename()*/, NULL, 0, NULL)!=0)
	{
		CloseMovie();
		//	  printf("error: av_open_input_file\n");

		return; // Couldn't open file
	}  

	// Retrieve stream information
	if(av_find_stream_info(m_local->m_pFormatCtx)<0)
	{
	//	  printf("error: av_find_stream_info\n");
		CloseMovie();
		return; // Couldn't find stream information
	}

	// Find the first video stream
	m_videoStream=-1;
	for(i=0; i<m_local->m_pFormatCtx->nb_streams; i++)
	{
		if(m_local->m_pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
		{
			m_videoStream=(int)i;
			break;
		}
	}
	if(m_videoStream==-1)
	{
		CloseMovie();
		return; // Didn't find a video stream
	}

	m_audioStream=-1;
	m_local->m_pACodecCtx=0;
	if(m_playaudio)
	{
		for(i=0; i<m_local->m_pFormatCtx->nb_streams; i++)
		{
			if(m_local->m_pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
			{
				m_audioStream=(int)i;
				m_local->m_pACodecCtx=m_local->m_pFormatCtx->streams[m_audioStream]->codec;
				break;
			}
		}
	}

	// Get a pointer to the codec context for the video stream
	m_local->m_pVCodecCtx=m_local->m_pFormatCtx->streams[m_videoStream]->codec;
	m_vstreamtimebase=av_q2d(m_local->m_pFormatCtx->streams[m_videoStream]->time_base);
	m_vcodectimebase=av_q2d(m_local->m_pVCodecCtx->time_base);

	// Find the decoder for the video stream
	m_local->m_pVCodec=avcodec_find_decoder(m_local->m_pVCodecCtx->codec_id);
	if(m_local->m_pVCodec==NULL)
	{
	//    fprintf(stderr, "Unsupported codec!\n");
		m_local->m_pVCodecCtx=0;	/* couldn't open so no need to close */
		m_local->m_pACodecCtx=0;	/* couldn't open so no need to close */
		CloseMovie();
		return; // Codec not found
	}
	// Open video codec
	if(avcodec_open(m_local->m_pVCodecCtx, m_local->m_pVCodec)<0)
	{
		m_local->m_pVCodecCtx=0;	/* couldn't open so no need to close */
		m_local->m_pACodecCtx=0;	/* couldn't open so no need to close */
		CloseMovie();
		return; // Could not open codec
	}

	//open the Audio codec if movie contained audio
	if(m_local->m_pACodecCtx)
	{
		m_local->m_pACodec = avcodec_find_decoder(m_local->m_pACodecCtx->codec_id);
		if(m_local->m_pACodec)
			avcodec_open(m_local->m_pACodecCtx, m_local->m_pACodec);
		else
			m_local->m_pACodecCtx=0;
	}

	// Allocate video frame
	m_local->m_pFrame=avcodec_alloc_frame();

	m_width=m_local->m_pVCodecCtx->width;
	m_height=m_local->m_pVCodecCtx->height;
	m_format=m_local->m_pVCodecCtx->pix_fmt;

	if(m_local->m_pFormatCtx->start_time!=(long long)AV_NOPTS_VALUE)
		m_starttime=(int)(((double)m_local->m_pFormatCtx->start_time/1000000.0f)*TICKSPERSEC);
	else
		m_starttime=0;

	m_duration=(int)(((double)m_local->m_pFormatCtx->duration/1000000.0f)*TICKSPERSEC);
	m_isvalid=true;

	/* since the sourcec image size changed we need to re-do this! */
	if(m_image)
		SetOutputImage(m_image,m_quality);
}

#if 1
#define OUTFORMAT PIX_FMT_RGB32
#define OUTBPP 4
#else
#define OUTFORMAT PIX_FMT_RGB24
#define OUTBPP 3
#endif

/* better quality = runs slower */
void kGUIMovie::SetOutputImage(kGUIImage *image,unsigned int quality)
{
	unsigned int qtable[]={
		SWS_POINT,SWS_FAST_BILINEAR,SWS_BILINEAR,SWS_BICUBLIN,SWS_BICUBIC,SWS_SPLINE,SWS_SINC};

	assert(quality<(sizeof(qtable)/sizeof(unsigned int)),"Quality value out of range!\n");
	m_quality=quality;
	m_image=image;
	if(!image)
		return;

	m_outwidth=image->GetImageWidth();
	m_outheight=image->GetImageHeight();
	assert(m_outwidth>0 && m_outheight>0 && image->GetImageType()==GUISHAPE_SURFACE,"Error, destination image is not correct!");

	if(m_isvalid)
	{
		if(m_local->m_img_convert_ctx)
		{
			av_free(m_local->m_img_convert_ctx);
			m_local->m_img_convert_ctx=0;
		}
//		kGUI::Trace("kGUIMovie::SetOutputImage inw=%d,inh=%d,format=%d,outw=%d,outh=%d\n",m_width,m_height,m_format,m_outwidth,m_outheight);
		m_local->m_img_convert_ctx = sws_getContext(
			m_width, m_height, (PixelFormat)m_format, 
            m_outwidth, m_outheight, OUTFORMAT,
			qtable[quality], NULL, NULL, NULL);
	}
}

void kGUIMovie::UpdateBuffers(void)
{
	int frameFinished;
	int alen;

	do
	{
		if(m_numframesready)
			return;

		if(m_local->m_packet.size==0)
		{
			if(av_read_frame(m_local->m_pFormatCtx, &m_local->m_packet)<0)
				return;
		}

		// Is this a packet from the video stream?
		if(m_local->m_packet.stream_index==m_videoStream)
		{
			// Decode video frame
			frameFinished=0;
			alen=avcodec_decode_video(m_local->m_pVCodecCtx, m_local->m_pFrame, &frameFinished, 
			   m_local->m_packet.data, m_local->m_packet.size);

			//did an error occur??
			if(alen<0)
			{
				av_free_packet(&m_local->m_packet);
				m_local->m_packet.size=0;
				return;
			}

			if(frameFinished>0)
			{
				static const AVRational VIDEO_TIMEBASE = {1, TICKSPERSEC};
				long long vc;

			    /* update the video clock */
				if(m_local->m_packet.pts && m_local->m_packet.pts!=(long long)AV_NOPTS_VALUE)
					m_rawtime = m_local->m_packet.pts;
				else if(m_local->m_packet.dts && m_local->m_packet.dts!=(long long)AV_NOPTS_VALUE)
					m_rawtime=m_local->m_packet.dts;

				vc = av_rescale_q(m_rawtime, m_local->m_pFormatCtx->streams[m_videoStream]->time_base, VIDEO_TIMEBASE);
				m_rawtime+=m_local->m_packet.duration;

				m_bufptime=(int)(vc-m_starttime);

				++m_numframesready;

				av_free_packet(&m_local->m_packet);
				m_local->m_packet.size=0;
			}
			else
			{
				av_free_packet(&m_local->m_packet);
				m_local->m_packet.size=0;
			}
		}
		else if((m_local->m_packet.stream_index==m_audioStream) && m_local->m_pACodecCtx && m_playaudio)
        {
			int len;
			unsigned char *ptr;

			ptr = m_local->m_packet.data;
			len = m_local->m_packet.size;
			while(len>0)
			{
				frameFinished=AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(short);
				alen=avcodec_decode_audio2(m_local->m_pACodecCtx, (int16_t *)m_abalign, &frameFinished, ptr, len);

				if(alen<0)
				{
					av_free_packet(&m_local->m_packet);
					m_local->m_packet.size=0;
					return;	/* error */
				}

				len-=alen;
				ptr+=alen;

				// Did we get a audio frame?
				if(frameFinished>=0)
				{
					/* play audio packet */
					m_audio->Play(m_local->m_pACodecCtx->sample_rate,m_local->m_pACodecCtx->channels,(const unsigned char *)m_abalign,frameFinished*2,true);
				}
			}
			av_free_packet(&m_local->m_packet);
			m_local->m_packet.size=0;
		}
		else
		{
			av_free_packet(&m_local->m_packet);
			m_local->m_packet.size=0;
		}
	}while(1);
}

bool kGUIMovie::ShowFrame(void)
{
	unsigned char *data[4];
	int linesize[4];

	if(m_numframesready==0)
	{
		/* if no pending frame then clear the image to all black */
//		memset(m_image->GetImageDataPtr(0),0, m_outheight*m_outwidth*OUTBPP);
		return(false);
	}

	//skip if we are really lagging
	if(m_time<=m_bufptime)
	{
		/* copy the frame to the image */ 
		data[0]=m_image->GetImageDataPtr(0);
		data[1]=data[0]+1;
		data[2]=data[0]+2;
		data[3]=data[0]+3;
		linesize[0]=linesize[1]=linesize[2]=linesize[3]=m_image->GetImageWidth()*4;

		//scale directly to the image buffer
		sws_scale(m_local->m_img_convert_ctx, m_local->m_pFrame->data, 
			m_local->m_pFrame->linesize, 0,m_height, 
			data, linesize);
		m_image->ImageChanged();
	}

	m_ptime=m_bufptime;
	--m_numframesready;

	return(true);
}

void kGUIMovie::Event(void)
{
	/* is event is triggered when a movie is playing */
	m_time+=kGUI::GetET();

	UpdateBuffers();

	while(m_time>=m_ptime)
	{
		assert(m_image!=0,"Output Image not defined!");

		if(ShowFrame()==false)
		{
			if(m_loop==true)
				ReStart();
			else
			{
				/* end of movie, set time to end time and stop */
				SetPlaying(false);
				m_done=true;
				m_time=m_duration-1;
			}
			break;
		}
		else
			UpdateBuffers();
	}
}

void kGUIMovie::SetPlaying(bool p)
{
	if(m_playing==p)
		return;

	m_done=false;
	m_playing=p;
	if(!m_playing)
	{
		kGUI::DelEvent(this,CALLBACKNAME(Event));
		/* stop audio too */
		m_audio->Stop();
	}
	else
		kGUI::AddEvent(this,CALLBACKNAME(Event));
}

void kGUIMovie::Seek(int place)
{
	long long timestamp;

	//Timestamp is in million per second and was in TICKSPERSEC
	timestamp=(long long)(place*(1000000LL/TICKSPERSEC));
	if(place<m_time)
	{
		m_audio->Stop();

		//flush any accumulated data from packets since we did a seek
		avcodec_flush_buffers(m_local->m_pVCodecCtx);
		if(m_local->m_pACodecCtx)
			avcodec_flush_buffers(m_local->m_pACodecCtx);

		//backwards just means go back to the last keyframe ( I think )
		av_seek_frame(m_local->m_pFormatCtx,-1,timestamp, AVSEEK_FLAG_BACKWARD);

		/* flush any pending frames */
		m_numframesready=0;

		m_done=false;
		m_time=place;
		m_ptime=place;
		m_rawtime=place;	/* not correct */

		m_local->m_packet.size=0;

		assert(m_image!=0,"Image needs to be set!\n");
		memset(m_image->GetImageDataPtr(0),0, m_outheight*m_outwidth*OUTBPP);

		/* since no frame is ready it will get and display the frame and que up the next one too */
		LoadNextFrame();
	}

	/* since it seeked to the nearest keyframe lets go forward till place */
	while(m_ptime<place)
	{
		if(LoadNextFrame()==false)
			break;	/* end of movie, abort */
	}
	m_time=place;
}

void kGUIMovie::CloseMovie(void)
{
	m_isvalid=false;

	SetPlaying(false);

	// Close the audio codec
	if(m_local->m_pACodecCtx)
	{
		avcodec_close(m_local->m_pACodecCtx);
		m_local->m_pACodecCtx=0;
	}

	// Close the video codec
	if(m_local->m_pVCodecCtx)
	{
		avcodec_close(m_local->m_pVCodecCtx);
		m_local->m_pVCodecCtx=0;
	}

	// Free the YUV frame
	if(m_local->m_pFrame)
	{
		av_free(m_local->m_pFrame);
		m_local->m_pFrame=0;
	}

	if(m_local->m_img_convert_ctx)
	{
		av_free(m_local->m_img_convert_ctx);
		m_local->m_img_convert_ctx=0;
	}

	// Close the video file
	if(m_local->m_pFormatCtx)
	{
		av_close_input_file(m_local->m_pFormatCtx);
		m_local->m_pFormatCtx=0;
	}
}
