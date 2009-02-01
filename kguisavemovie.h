#ifndef __KGUISAVEMOVIE__
#define __KGUISAVEMOVIE__

#include "kgui.h"
//#include "kguiaudio.h"

extern "C"
{
struct AVFormatContext;
struct AVStream;
struct AVFrame;
}

class kGUISaveMovie
{
public:
	kGUISaveMovie();
	~kGUISaveMovie();
	static const char *GetVersion(void);

	void SaveMovie(int fps,int w,int h,const char *filename);
	virtual kGUIImage *RenderFrame(int framenum)=0;
//	virtual void UpdateAudio(void)=0;
private:
	AVStream *add_video_stream(AVFormatContext *oc, int codec_id);
	void open_video(AVFormatContext *oc, AVStream *st);
	AVFrame *alloc_picture(int pix_fmt, int width, int height);
	bool write_video_frame(AVFormatContext *oc, AVStream *st);
	void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);
	void close_video(AVFormatContext *oc, AVStream *st);

	AVFrame *m_picture;
	AVFrame *m_tmp_picture;
	unsigned char *m_video_outbuf;
	int m_frame_count, m_video_outbuf_size;
	int m_fps;
	int m_w,m_h;
};

#endif
