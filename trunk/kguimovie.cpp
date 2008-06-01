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

/*********************************************************************************/
/*                                                                               */
/* This is the interface to the ffmpeg movie player                              */
/*                                                                               */
/*********************************************************************************/

//thanks for dranger.com for the online tutorial that I used as the basis for this code.

//todo: audio

#include "kgui.h"
#include "kguimovie.h"

/* a simple wrapper class for loading movies using ffmpeg */

extern "C"
{
	/****** FFMPEG ******/

	/* windows needs 2 include files that are not standard */
	/* these are in wiffmpeg so any windows projects needs to */
	/* set this as an include directory */

#include "avcodec.h"
#include "avformat.h"
#include "swscale.h"
}

#define INT64_C(val) val##LL

#ifndef STRINGIFY
#define STRINGIFY(x)	STRINGIFY1(x)
#define STRINGIFY1(x)	#x
#endif

//return version number, used in credit screens
const char *kGUI::GetFFMpegVersion(void)
{
	static char ffmpegversion[]={ "avcodec=" STRINGIFY(LIBAVCODEC_VERSION) ", avformat=" STRINGIFY(LIBAVFORMAT_VERSION) };

	return ffmpegversion;
}

class kGUIMovieLocal
{
public:
	kGUIMovieLocal() {};
	static int kg_open(URLContext *h, const char *filename, int flags);
	static int kg_read(URLContext *h, unsigned char *buf, int size);
	static int kg_write(URLContext *h, unsigned char *buf, int size);
	static offset_t kg_seek(URLContext *h, offset_t pos, int whence);
	static int kg_close(URLContext *h);
	static URLProtocol kg_protocol;

	AVFormatContext *m_pFormatCtx;
	ByteIOContext m_pb;
	AVCodecContext *m_pVCodecCtx;	/* video codec context */
	AVCodecContext *m_pACodecCtx;	/* audio codec context or null if movie has no audio */
	AVCodec *m_pVCodec;				/* video codec */
	AVCodec *m_pACodec;				/* audio codec */
	AVFrame *m_pFrame; 
#if USERGBBUFFER
	AVFrame *m_pFrameRGB;
	int m_numBytes;
	uint8_t *m_buffer;
#endif
	struct SwsContext *m_img_convert_ctx;
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

offset_t kGUIMovieLocal::kg_seek(URLContext *h, offset_t pos, int whence)
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

kGUIMovie::kGUIMovie()
{
	m_local=new kGUIMovieLocal();
	m_width=0;
	m_height=0;

	m_isvalid=false;
	m_playing=false;
	m_image=0;
	m_local->m_pFrame=0;
#if USERGBBUFFER
	m_pFrameRGB=0;
	m_buffer=0;
#endif
	m_local->m_img_convert_ctx=0;
	m_local->m_pVCodecCtx=0;
	m_local->m_pACodecCtx=0;
	m_local->m_pFormatCtx=0;
	m_frameready=0;
	m_time=0;
	m_ptime=0;
}

kGUIMovie::~kGUIMovie()
{
	CloseMovie();
	delete m_local;
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

	m_videoclock=0.0f;
	m_time=0;
	m_ptime=0;
	m_frameready=false;

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
	for(i=0; i<m_local->m_pFormatCtx->nb_streams; i++)
	{
		if(m_local->m_pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
		{
			m_audioStream=(int)i;
			m_local->m_pACodecCtx=m_local->m_pFormatCtx->streams[m_audioStream]->codec;
			break;
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

	if(m_local->m_pFormatCtx->start_time!=(int)AV_NOPTS_VALUE)
		m_starttime=(int)(((double)m_local->m_pFormatCtx->start_time/1000000.0f)*TICKSPERSEC);
	else
		m_starttime=0;

	m_duration=(int)(((double)m_local->m_pFormatCtx->duration/1000000.0f)*TICKSPERSEC);
	m_isvalid=true;

	/* since the sourcec image size changed we need to re-do this! */
	if(m_image)
		SetOutputImage(m_image);
}

#if 1
#define OUTFORMAT PIX_FMT_RGB32
#define OUTBPP 4
#else
#define OUTFORMAT PIX_FMT_RGB24
#define OUTBPP 3
#endif

void kGUIMovie::SetOutputImage(kGUIImage *image)
{
	m_image=image;
	if(!image)
		return;
	m_outwidth=image->GetImageWidth();
	m_outheight=image->GetImageHeight();
	assert(m_outwidth>0 && m_outheight>0 && image->GetImageType()==GUISHAPE_SURFACE,"Error, destination image is not correct!");

#if USERGBBUFFER
	if(m_pFrameRGB)
	{
		av_free(m_pFrameRGB);
		m_pFrameRGB=0;
	}
	if(m_buffer)
	{
		av_free(m_buffer);
		m_buffer=0;
	}
	// Allocate an AVFrame structure
	m_pFrameRGB=avcodec_alloc_frame();
	if(m_pFrameRGB==NULL)
		return;

	// Determine required buffer size and allocate buffer
	m_numBytes=avpicture_get_size(OUTFORMAT, m_outwidth,
					m_outheight);
	m_buffer=(uint8_t *)av_malloc(m_numBytes*sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)m_pFrameRGB, m_buffer, OUTFORMAT,
			m_outwidth, m_outheight);
#endif

	if(m_local->m_img_convert_ctx)
	{
		av_free(m_local->m_img_convert_ctx);
		m_local->m_img_convert_ctx=0;
	}
	m_local->m_img_convert_ctx = sws_getContext(
			m_width, m_height, m_format, 
            m_outwidth, m_outheight, OUTFORMAT,
			SWS_BICUBIC, NULL, NULL, NULL);
}

bool kGUIMovie::LoadNextFrame(void)
{
	int frameFinished=0;
	bool gotpts=false;
	double frame_delay;
	AVPacket packet;
	bool again;

	assert(m_image!=0,"Output Image not defined!");

	if(m_frameready==true)
	{
		/* show last loaded frame */
		ShowFrame();
		again=false;
	}
	else
	{
		/* if no pending frame then clear the image to all black */
		memset(m_image->GetImageDataPtr(0),0, m_outheight*m_outwidth*OUTBPP);
		again=true;
	}

	while(av_read_frame(m_local->m_pFormatCtx, &packet)>=0)
	{
		// Is this a packet from the video stream?
		if(packet.stream_index==m_videoStream)
		{
			// Decode video frame
			if(gotpts==false)
			{
				gotpts=true;		//only grab the pts from the first video packet 
				if(packet.pts && packet.pts!=(int)AV_NOPTS_VALUE)
					m_videoclock=(double)packet.pts*m_vstreamtimebase;
				else if(packet.dts && packet.dts!=(int)AV_NOPTS_VALUE)
					m_videoclock=(double)packet.dts*m_vstreamtimebase;
			    /* update the video clock */

				/* if we are repeating a frame, adjust clock accordingly */
				frame_delay = m_vcodectimebase;
				if(m_local->m_pFrame->repeat_pict)
					frame_delay += m_local->m_pFrame->repeat_pict * (frame_delay * 0.5);
				m_videoclock += frame_delay;
				m_ptime=(int)(m_videoclock*TICKSPERSEC)-m_starttime;
			}
			avcodec_decode_video(m_local->m_pVCodecCtx, m_local->m_pFrame, &frameFinished, 
			   packet.data, packet.size);
      
			// Did we get a video frame?
			if(frameFinished)
			{
			    av_free_packet(&packet);
				/* frame is loaded and ready to be presented when it is time */
				m_frameready=true;
				
				/* if there was no pending frame to show then get the time for the next one now. */
				if(again)
					return(LoadNextFrame());
				return(true);
			}
		}
    }
    
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
	return(false);
}

void kGUIMovie::ShowFrame(void)
{
	/* copy the frame to the image */ 

#if USERGBBUFFER
	unsigned char *dest;
	unsigned char *src;
	unsigned int ls;

	sws_scale(m_local->m_img_convert_ctx, m_local->m_pFrame->data, 
			m_local->m_pFrame->linesize, 0,m_height, 
			m_pFrameRGB->data, m_pFrameRGB->linesize);

	ls=m_pFrameRGB->linesize[0];
	dest=m_image->GetImageDataPtr(0);
	
	/* sometimes the lizesize is larger than outwidth*OUTBPP and in that case we need to copy */
	/* a raster line at a time, if it is equal then we can copy the whole frame at once */
	if((m_outwidth*OUTBPP)==ls)
	{
		src=m_pFrameRGB->data[0];
		memcpy(dest,src, m_outheight*m_outwidth*OUTBPP);
	}
	else
	{
		for(unsigned y=0; y<m_outheight; y++)
		{
			src=m_pFrameRGB->data[0]+y*ls;
			memcpy(dest,src, m_outwidth*OUTBPP);
			dest+=m_outwidth*OUTBPP;
		}
	}
#else
	{
		unsigned char *data[4];
		int linesize[4];

		data[0]=m_image->GetImageDataPtr(0);
		data[1]=data[0]+1;
		data[2]=data[0]+2;
		data[3]=data[0]+3;
		linesize[0]=linesize[1]=linesize[2]=linesize[3]=m_image->GetImageWidth()*4;

		//scale directly to the image buffer
		sws_scale(m_local->m_img_convert_ctx, m_local->m_pFrame->data, 
			m_local->m_pFrame->linesize, 0,m_height, 
			data, linesize);
	}
#endif
	m_frameready=false;
	m_image->ImageChanged();
}

void kGUIMovie::Event(void)
{
	/* is event is triggered when a movie is playing */
	m_time+=kGUI::GetET();
	while(m_time>=m_ptime)
	{
		if(LoadNextFrame()==false)
		{
			if(m_loop==true)
				Seek(0);	/* start again */
			else
			{
				/* end of movie, set time to end time and stop */
				SetPlaying(false);
				m_done=true;
				m_time=m_duration-1;
			}
			break;
		}
	}
}

void kGUIMovie::SetPlaying(bool p)
{
	if(m_playing==p)
		return;

	m_done=false;
	m_playing=p;
	if(!m_playing)
		kGUI::DelEvent(this,CALLBACKNAME(Event));
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
		//backwards just means go back to the last keyframe ( I think )
		av_seek_frame(m_local->m_pFormatCtx,-1,timestamp, AVSEEK_FLAG_BACKWARD);

		//flush any accumulated data from packets since we did a seek
		avcodec_flush_buffers(m_local->m_pVCodecCtx);
		if(m_local->m_pACodecCtx)
			avcodec_flush_buffers(m_local->m_pACodecCtx);
	
		/* flush last frame if there was one */
		m_done=false;
		m_frameready=false;
		m_time=place;
		m_ptime=place;

		/* since no frame is ready it will get and display the frame and que up the next one too */
		LoadNextFrame();
	}

	/* since it seeked to the nearest keyframe lets go forward till place */
	while(m_ptime<place)
	{
		if(LoadNextFrame()==false)
			break;	/* end of movie, abort */
	}
//	m_ptime=place;
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

#if USERGBBUFFER
	if(m_pFrameRGB)
	{
		av_free(m_pFrameRGB);
		m_pFrameRGB=0;
	}
	if(m_buffer)
	{
		av_free(m_buffer);
		m_buffer=0;
	}
#endif

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
