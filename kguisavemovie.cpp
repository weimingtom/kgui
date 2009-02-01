
/*********************************************************************************/
/* kGUI - kguisavemovie.cpp                                                      */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                                */
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

/*! @file kguisavemovie.cpp
    @brief This is the class wrapper interface to the ffmpeg movie player for saving a movie. */

#include "kgui.h"
#include "kguisavemovie.h"

extern "C"
{
#include <avformat.h>
#include <swscale.h>
}

kGUISaveMovie::kGUISaveMovie()
{
   /* initialize libavcodec, and register all codecs and formats */
    av_register_all();
}

#ifndef STRINGIFY
#define STRINGIFY(x)	STRINGIFY1(x)
#define STRINGIFY1(x)	#x
#endif

//return version number, used in credit screens
const char *kGUISaveMovie::GetVersion(void)
{
	static char ffmpegversion[]={ "avcodec=" STRINGIFY(LIBAVCODEC_VERSION) ", avformat=" STRINGIFY(LIBAVFORMAT_VERSION)", avutil=" STRINGIFY(LIBAVUTIL_VERSION) };

	return ffmpegversion;
}

void kGUISaveMovie::SaveMovie(int fps,int w,int h,const char *filename)
{
    AVOutputFormat *fmt;
    AVFormatContext *oc;
//    AVStream *audio_st;
	AVStream *video_st;
//    double audio_pts;
	double video_pts;
    unsigned int i;

	m_fps=fps;
	m_w=w;
	m_h=h;
    /* auto detect the output format from the name. default is mpeg. */
    fmt = guess_format(NULL, filename, NULL);
    assert(fmt!=0,"Cannot deduce format!");
	
    /* allocate the output media context */
    oc = av_alloc_format_context();
    assert(oc!=0,"Memory Error!");
    oc->oformat = fmt;
    sprintf(oc->filename, "%s", filename);

    /* add the audio and video streams using the default format codecs
       and initialize the codecs */
    video_st = NULL;
//    audio_st = NULL;
    if (fmt->video_codec != CODEC_ID_NONE)
	{
        video_st = add_video_stream(oc, fmt->video_codec);
    }
#if 0
	if (fmt->audio_codec != CODEC_ID_NONE)
	{
        audio_st = add_audio_stream(oc, fmt->audio_codec);
    }
#endif

    /* set the output parameters (must be done even if no parameters). */
    if (av_set_parameters(oc, NULL) < 0)
	{
		assert(false, "Invalid output format parameters\n");
    }

    dump_format(oc, 0, filename, 1);

    /* now that all the parameters are set, we can open the audio and
       video codecs and allocate the necessary encode buffers */
    if (video_st)
        open_video(oc, video_st);
//    if (audio_st)
//      open_audio(oc, audio_st);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
	{
        if (url_fopen(&oc->pb, filename, URL_WRONLY) < 0)
		{
            fprintf(stderr, "Could not open '%s'\n", filename);
            exit(1);
        }
    }

    /* write the stream header, if any */
    av_write_header(oc);

	m_frame_count=0;
	for(;;)
	{
        /* compute current audio and video time */
#if 0
		if (audio_st)
            audio_pts = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
        else
            audio_pts = 0.0;
#endif

        if (video_st)
            video_pts = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
        else
            video_pts = 0.0;

//        if ((!audio_st || audio_pts >= STREAM_DURATION) &&
  //          (!video_st || video_pts >= STREAM_DURATION))
    //        break;

        /* write interleaved audio and video frames */
//        if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
//            write_audio_frame(oc, audio_st);
//        } else {
            if(write_video_frame(oc, video_st)==false)
				break;
//        }
    }

    /* close each codec */
    if (video_st)
        close_video(oc, video_st);
//    if (audio_st)
//        close_audio(oc, audio_st);

    /* write the trailer, if any */
    av_write_trailer(oc);

    /* free the streams */
    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(oc->pb);
    }

    /* free the stream */
    av_free(oc);
}

AVStream *kGUISaveMovie::add_video_stream(AVFormatContext *oc, int codec_id)
{
    AVCodecContext *c;
    AVStream *st;

    st = av_new_stream(oc, 0);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        exit(1);
    }

    c = st->codec;
    c->codec_id = (CodecID)codec_id;
    c->codec_type = CODEC_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = m_w;
    c->height = m_h;
    c->time_base.den = m_fps;
    c->time_base.num = 1;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = PIX_FMT_YUV420P;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO)
	{
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO){
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        c->mb_decision=2;
    }
    // some formats want stream headers to be separate
    if(!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov") || !strcmp(oc->oformat->name, "3gp"))
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

void kGUISaveMovie::open_video(AVFormatContext *oc, AVStream *st)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0)
	{
        assert(false, "could not open codec!\n");
    }

    m_video_outbuf = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE))
	{
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        m_video_outbuf_size = m_w*m_h*5;
        m_video_outbuf = (uint8_t *)av_malloc(m_video_outbuf_size);
    }

    /* allocate the encoded raw picture */
	m_picture = alloc_picture(c->pix_fmt, c->width, c->height);
	assert(m_picture!=0,"Could not allocate picture!");

    /* if the output format is not YUV420P, then a temporary YUV420P
       picture is needed too. It is then converted to the required
       output format */
    m_tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_YUV420P)
	{
        m_tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
		assert(m_tmp_picture!=0,"Could not allocate picture!");
    }
}

AVFrame *kGUISaveMovie::alloc_picture(int pix_fmt, int width, int height)
{
    AVFrame *picture;
    uint8_t *picture_buf;
    int size;

    picture = avcodec_alloc_frame();
    if (!picture)
        return NULL;
    size = avpicture_get_size(pix_fmt, width, height);
    picture_buf = (uint8_t *)av_malloc(size);
    if (!picture_buf) {
        av_free(picture);
        return NULL;
    }
    avpicture_fill((AVPicture *)picture, picture_buf,
                   pix_fmt, width, height);
    return picture;
}

/* prepare a dummy image */
void kGUISaveMovie::fill_yuv_image(AVFrame *pict, int frame_index, int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
        }
    }

    /* Cb and Cr */
    for(y=0;y<height/2;y++) {
        for(x=0;x<width/2;x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

bool kGUISaveMovie::write_video_frame(AVFormatContext *oc, AVStream *st)
{
    int out_size, ret;
    AVCodecContext *c;
    static struct SwsContext *img_convert_ctx;
	kGUIImage *img;

//	printf("writing frame #%d\n",m_frame_count);
    c = st->codec;

	img=RenderFrame(m_frame_count);
	if(img)
	{
		/* generated picture is put into m_picture */
	}

//    if (m_frame_count >= STREAM_NB_FRAMES)
    if (!img)
	{
        /* no more frame to compress. The codec has a latency of a few
           frames if using B frames, so we get the last frames by
           passing the same picture again */
    }
	else
	{
		unsigned char *data[4];
		int linesize[4];

		/* convert image to format needed by the codec */
        if (img_convert_ctx == NULL)
		{
			img_convert_ctx = sws_getContext(img->GetImageWidth(), img->GetImageHeight(),
                                                 PIX_FMT_RGB32,
                                                 c->width, c->height,
                                                 c->pix_fmt,
                                                 SWS_BICUBIC, NULL, NULL, NULL);
                assert (img_convert_ctx != NULL,"Cannot initialize the conversion context");
		}

		data[0]=img->GetImageDataPtr(0);
		data[1]=data[0]+1;
		data[2]=data[0]+2;
		data[3]=data[0]+3;
		linesize[0]=linesize[1]=linesize[2]=linesize[3]=img->GetImageWidth()*4;

//            fill_yuv_image(m_tmp_picture, m_frame_count, c->width, c->height);
		sws_scale(img_convert_ctx, data, linesize,
                    0, c->height, m_picture->data, m_picture->linesize);
	}
//	else
//	{
//		fill_yuv_image(m_picture, m_frame_count, c->width, c->height);
//	}

    if (oc->oformat->flags & AVFMT_RAWPICTURE)
	{
        /* raw video case. The API will change slightly in the near
           future for that */
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.flags |= PKT_FLAG_KEY;
        pkt.stream_index= st->index;
        pkt.data= (uint8_t *)m_picture;
        pkt.size= sizeof(AVPicture);

        ret = av_write_frame(oc, &pkt);
    }
	else
	{
        /* encode the image */
        out_size = avcodec_encode_video(c, m_video_outbuf, m_video_outbuf_size, m_picture);
        /* if zero size, it means the image was buffered */
        if (out_size > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);

            pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
            if(c->coded_frame->key_frame)
                pkt.flags |= PKT_FLAG_KEY;
            pkt.stream_index= st->index;
            pkt.data= m_video_outbuf;
            pkt.size= out_size;

            /* write the compressed frame in the media file */
            ret = av_write_frame(oc, &pkt);
        } else {
            ret = 0;
        }
    }
    if (ret != 0) {
        fprintf(stderr, "Error while writing video frame\n");
        exit(1);
    }
    m_frame_count++;
	return(img!=0);
}

void kGUISaveMovie::close_video(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    av_free(m_picture->data[0]);
    av_free(m_picture);
    if (m_tmp_picture) {
        av_free(m_tmp_picture->data[0]);
        av_free(m_tmp_picture);
    }
    av_free(m_video_outbuf);
}

kGUISaveMovie::~kGUISaveMovie()
{

}
