/**********************************************************************************/
/* kGUI - kguiimage.cpp                                                           */
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

/*! @file kguiimage.cpp 
    @brief This is class that decompresses images. All images are decompressed to a       
    standard RGBA format and that way the render code doesn't have to know about   
    all the different formats. This code uses libjpg, libpng for decoding.
    The image class also handles caching of uncompressed images to reduce memory.  
    For exmaple: if you have a table and it has 15,000 unique images, one per row  
    then the system will not uncompress each one, only the ones visible as the     
    user scrolls through the table will be decompressed and as they scroll off the 
     screen then they can be flushed to free up memory. */

#include "kgui.h"

#define XMD_H
extern "C" {
#undef FAR
#include "jpeglib.h"
#include "png.h"
}

int jpegversion=JPEG_LIB_VERSION;
const char pngversion[]={PNG_LIBPNG_VER_STRING};

LinkEnds<class kGUIImage,class kGUIImage> kGUIImage::m_loadedends;
LinkEnds<class kGUIImage,class kGUIImage> kGUIImage::m_lockedandloadedends;
LinkEnds<class kGUIImage,class kGUIImage> kGUIImage::m_unloadedends;
int kGUIImage::m_numloaded;
int kGUIImage::m_maxloaded;

void kGUIImage::InitCache(int maximages)
{
	m_numloaded=0;
	m_maxloaded=maximages;
}

kGUIImage::kGUIImage()
{
	m_bad=false;
	m_memimage=false;
	m_allocmemimage=false;
	m_locked=false;
	m_imagetype=GUISHAPE_UNDEFINED;
	m_imagewidth=0;
	m_imageheight=0;
	m_bpp=0;
	m_stepx=1.0f;
	m_stepy=1.0f;
	m_numframes=0;
	m_imagedata.Init(1,4);
	m_delays.Init(1,4);
}

unsigned short kGUIImage::ReadU16(const char *fp)
{
	unsigned short v1=fp[0]&255;
	unsigned short v2=fp[1]&255;
	return(v1|(v2<<8));
}

short kGUIImage::Read16(const char *fp)
{
	short val16=ReadU16(fp);
	return(val16);
}

unsigned int kGUIImage::ReadU24(const char *fp)
{
	unsigned int v1=fp[0]&255;
	unsigned int v2=fp[1]&255;
	unsigned int v3=fp[2]&255;
	return(v1|(v2<<8)|(v3<<16));
}

int kGUIImage::Read24(const char *fp)
{
	int val24=ReadU24(fp);
	if ( val24 > 0x7FFFFF )
		val24-=0xFFFFFF;
	return(val24);
}

unsigned int kGUIImage::ReadU32(const char *fp)
{
	unsigned int v1=fp[0]&255;
	unsigned int v2=fp[1]&255;
	unsigned int v3=fp[2]&255;
	unsigned int v4=fp[3]&255;
	return(v1|(v2<<8)|(v3<<16)|(v4<<24));
}

int kGUIImage::Read32(const char *fp)
{
	int val32=ReadU32(fp);
	if ( val32 > 0x7FffFFFF )
		val32-=0xFFFFFFFF;
	return(val32);
}

void kGUIImage::Purge(void)
{
	unsigned int i;

	if(m_numframes)
	{
		if(m_memimage==false)
		{
			/* first, unlink me from my spot in the loaded list */
			if(ValidLinks())
				Unlink();
			/* now add me to the top of the unloaded list */
			Link(m_unloadedends.GetHead());

			--m_numloaded;

			for(i=0;i<m_numframes;++i)
				delete []m_imagedata.GetEntry(i);
			m_numframes=0;
		}
		else
		{
			if(m_allocmemimage==true)
			{
				for(i=0;i<m_numframes;++i)
					delete []m_imagedata.GetEntry(i);
			}
			m_numframes=0;
		}
	}

	/* set these to zero to signal that the image is no longer valid */
	m_imagewidth=0;
	m_imageheight=0;

	if(ValidLinks())
		Unlink();
}

kGUIImage::~kGUIImage()
{
	Purge();
}

void kGUIImage::SetMemImage(unsigned int frame,int format,int width,int height,unsigned int bpp,const unsigned char *data)
{
	if(!frame)
		Purge();	/* purge previous image if there was one */
	else
	{
		/* purge this frame if it was already allocated */
		if(m_memimage==true && m_allocmemimage==true && m_numframes>frame)
			delete []m_imagedata.GetEntry(frame);
	}

	m_memimage=true;
	m_allocmemimage=false;
	m_imagetype=format;
	m_imagewidth=width;
	m_imageheight=height;
	m_bpp=bpp;
	m_numframes=frame+1;
	m_imagedata.SetEntry(frame,(unsigned char *)data);
	m_locked=true;
	Link(m_lockedandloadedends.GetHead());
	ImageChanged();
}

void kGUIImage::SetMemImageCopy(unsigned int frame,int format,int width,int height,unsigned int bpp,const unsigned char *data)
{
	unsigned char *copydata;

	if(!frame)
		Purge();	/* purge all previous image frames */
	else
	{
		/* purge this frame if it was already allocated */
		if(m_memimage==true && m_allocmemimage==true && m_numframes>frame)
			delete m_imagedata.GetEntry(frame);
	}

	m_locked=true;
	m_allocmemimage=true;
	m_memimage=true;
	m_imagetype=format;
	m_imagewidth=width;
	m_imageheight=height;
	m_bpp=bpp;

	copydata=new unsigned char [width*height*bpp];
	if(data)
		memcpy(copydata,data,width*height*bpp);
	else
		memset(copydata,0,width*height*bpp);
	m_numframes=frame+1;
	m_imagedata.SetEntry(frame,copydata);
	Link(m_lockedandloadedends.GetHead());
	ImageChanged();
}

void kGUIImage::SetRaw(unsigned char *raw)
{
	Purge();

	m_memimage=true;
	m_allocmemimage=false;
	m_imagetype=GUISHAPE_RAW;
	m_imagewidth=raw[0];
	m_imageheight=raw[1];
	m_numframes=1;
	m_imagedata.SetEntry(0,raw+2);
	m_bpp=3;	/* rgb 1 byte of each color */
	Link(m_lockedandloadedends.GetHead());
	ImageChanged();
}

/* this gets called automatically when the attached DataHandle is modified */
void kGUIImage::HandleChanged(void)
{
	Purge();	/* purge previous image if there was one */

	Link(m_unloadedends.GetHead());

	m_memimage=false;
	m_allocmemimage=false;
	m_imagewidth=0;
	m_imageheight=0;

	LoadImage(true);	/* don't load image, just set it's size */
	ImageChanged();
}

template<int imagetype>
inline static void ReadSubPixel(SUBPIXEL_DEF *sub)
{
	const unsigned char *sli;
	const unsigned char *li;
	double yweight,xweight,ph,pw;
	double weight;
	double r=0.0f;
	double g=0.0f;
	double b=0.0f;
	double a=0.0f;

	sli=sub->limage;		/* save pointer to each raster line */

	ph=sub->pixelheight;
	yweight=min(1.0f-sub->yfrac,sub->pixelheight);
	do
	{
		pw=sub->pixelwidth;
		xweight=min(1.0f-sub->xfrac,sub->pixelwidth);
		li=sli;
		do
		{
			if(xweight==1.0f && yweight==1.0f)
			{
				switch(imagetype)
				{
				case GUISHAPE_RAW:
				case GUISHAPE_JPG:
					r+=li[0];
					g+=li[1];
					b+=li[2];
					li+=3;
				break;
				case GUISHAPE_GIF:
				case GUISHAPE_PNG:
					r+=li[0];
					g+=li[1];
					b+=li[2];
					a+=li[2];
					li+=4;
				break;
				case GUISHAPE_SURFACE:
				{
					int x,xr,xg,xb;

					x=*((kGUIColor *)li);
					DrawColorToRGB(x,xr,xg,xb);
					r+=xr;
					g+=xg;
					b+=xb;
					li+=sizeof(kGUIColor);
				}
				break;
				}
			}
			else
			{
				weight=xweight*yweight;
				switch(imagetype)
				{
				case GUISHAPE_RAW:
				case GUISHAPE_JPG:
					r+=li[0]*weight;
					g+=li[1]*weight;
					b+=li[2]*weight;
					li+=3;
				break;
				case GUISHAPE_GIF:
				case GUISHAPE_PNG:
					r+=li[0]*weight;
					g+=li[1]*weight;
					b+=li[2]*weight;
					a+=li[2]*weight;
					li+=4;
				break;
				case GUISHAPE_SURFACE:
				{
					int x,xr,xg,xb;

					x=*((kGUIColor *)li);
					DrawColorToRGB(x,xr,xg,xb);
					r+=xr*weight;
					g+=xg*weight;
					b+=xb*weight;
					li+=sizeof(kGUIColor);
				}
				break;
				}
			}
			pw-=xweight;
			xweight=min(1.0f,pw);
		}while(pw>0.001f);

		ph-=yweight;
		yweight=min(1.0f,ph);
		sli+=sub->rowadd;	
	}while(ph>0.0001f);

	sub->rgba[0]=(int)(r*sub->pixelscale);
	sub->rgba[1]=(int)(g*sub->pixelscale);
	sub->rgba[2]=(int)(b*sub->pixelscale);
	sub->rgba[3]=(int)(a*sub->pixelscale);
}

static void ReadSubPixel(int imagetype,SUBPIXEL_DEF *sub)
{
	switch(imagetype)
	{
	case GUISHAPE_RAW:
		ReadSubPixel<GUISHAPE_RAW>(sub);
	break;
	case GUISHAPE_JPG:
		ReadSubPixel<GUISHAPE_JPG>(sub);
	break;
	case GUISHAPE_GIF:
		ReadSubPixel<GUISHAPE_GIF>(sub);
	break;
	case GUISHAPE_PNG:
		ReadSubPixel<GUISHAPE_PNG>(sub);
	break;
	case GUISHAPE_SURFACE:
		ReadSubPixel<GUISHAPE_SURFACE>(sub);
	break;
	}
}

void kGUIImage::AsyncLoadPixels(void)
{
//	m_asyncactive=true;
	m_thread.Start(this,CALLBACKNAME(DoAsyncLoad));
//	kGUI::Trace("Start thread!\n");
}

void kGUIImage::DoAsyncLoad(void)
{
	LoadImage(false);
//	kGUI::Trace("End of DoAsyncLoad\n");
	if(m_thread.GetActive())
		m_thread.Close(false);
//	m_asyncactive=false;
//	kGUI::Trace("End thread!\n");
}

void kGUIImage::LoadImage(bool justsize)
{
	kGUIString *fn=GetFilename();
	long filetime;
	bool loaded;

	if(GetDataType()==DATATYPE_UNDEFINED)
		return;

	filetime=GetTime();
	if((justsize==true) && fn->GetLen())	/* just getting the size? */
	{
		kGUIImageSizeCache *ip;

		ip=kGUI::GetImageSizefromCache(fn->GetString());
		if(ip)
		{
			if(ip->time==filetime)
			{
				m_imagewidth=ip->width;
				m_imageheight=ip->height;
				return;
			}
		}
	}

	/* name.xxx */
	/* or name.xxx;version */

//	if(justsize==false && fn)
//		kGUI::Trace("Load Image Tick=%d, name='%s'\n",kGUI::GetFrame(),fn);

	loaded=false;
	if(strstri(fn->GetString(),".gif"))
		loaded=LoadGIFImage(justsize);
	else if(strstri(fn->GetString(),".jpg") || strstri(fn->GetString(),".jpeg"))
		loaded=LoadJPGImage(justsize);
	else if(strstri(fn->GetString(),".png"))
		loaded=LoadPNGImage(justsize);
	else if(strstri(fn->GetString(),".ico"))
		loaded=LoadWINICOImage(justsize);
	else if(strstri(fn->GetString(),".bmp"))
		loaded=LoadBMPImage(justsize);
	
	/* this typically get's triggered if the file is named xxx.jpg but is actually a gif file */
	if(loaded==false)
	{
		/* try looking into data to see what it is? */
		if(Open()==true)
		{
			unsigned char testread[5];

			if(Read(&testread,(unsigned long)4)==4)
			{
				testread[4]=0;
				if(!strcmp((char *)testread,"GIF8"))
				{
					Close();
					LoadGIFImage(justsize);
				}
				else if(testread[0]==0xff && testread[1]==0xd8)
				{
					Close();
					LoadJPGImage(justsize);
				}
				else if(testread[0]==0x89 && testread[1]==0x50 && testread[2]==0x4e && testread[3]==0x47)
				{
					Close();
					LoadPNGImage(justsize);
				}
				else if(testread[0]==0x00 && testread[1]==0x00 && testread[2]==0x01 && testread[3]==0x00)
				{
					Close();
					LoadWINICOImage(justsize);
				}
				else if(testread[0]==0x42 && testread[1]==0x4d)
				{
					Close();
					LoadBMPImage(justsize);
				}
				else
					Close();	/* unknown image format */
			}
			else
				Close();	/* file isn't even 4 bytes long */
		}
	}
	if(m_numframes)
		++m_numloaded;

	/* add this to the cache? */
	if(justsize==true && m_numframes)	/* just getting the size? */
	{
		kGUIImageSizeCache info;

		info.time=filetime;
		info.width=m_imagewidth;
		info.height=m_imageheight;
		kGUI::SetImageSizeToCache(fn->GetString(),&info);
	}
}

unsigned char dhgetc(DataHandle *dh)
{
	unsigned char c;

	dh->Read(&c,(unsigned long)1);
	return(c);
}

int getshort(DataHandle *dh)
{
	int c;
	c=dhgetc(dh);
	c=c+(dhgetc(dh)<<8);
	return(c);
}

void getstring(DataHandle *dh, char *s, int count)
{
	while(count)
	{
		*(s++)=dhgetc(dh);
		--count;
	}
	*(s)=0;
}

class GifNode
{
public:
	int prev;
	unsigned char color;
};

class GifLine
{
public:
	unsigned char *start;
	unsigned char *end;
};

#define MAXB 16384

#define TRACEGIF 0

bool kGUIImage::LoadGIFImage(bool justsize)
{
	unsigned char type[6+1];
	unsigned char temp_buffer[MAXB+1];
	GifNode node[MAXB+1];

	unsigned char gpalr[256];
	unsigned char gpalg[256];
	unsigned char gpalb[256];

	unsigned char palr[256];
	unsigned char palg[256];
	unsigned char palb[256];
	unsigned char pala[256];
	unsigned char resolution;
	unsigned char bgcolor=0;
	unsigned char aspect;
	unsigned char flags;
	bool interlaced;
	int colors;
	int version;

	unsigned int fw,fh;
	int ch,t,r;
	int code_size,curr_code_size,data_count;
	unsigned int holding;
	int bitptr;
	int code,old_code,next_code,start_table_size;
	int clear_code,eof_code,mask;
	int x,y;
	unsigned int lx,ty;
	int pi;					/* palette index */
	unsigned char *picture;
	int disposal=0;
	GifLine *giflines=0;
	GifLine *gl;
	static unsigned int intstep[4]={8,8,4,2};
	static unsigned int intstart[4]={0,4,2,1};
	int totaldelay100=0;
	int lastdelaytps=0;
	int totaldelaytps,deltatps;

	int lastframe=-1;
	bool transp=false;
	Array<unsigned char>xbuf;

	if(Open()==false)
		return(false);
	
	Read(&type,(unsigned long)6);
	type[6]=0;
	if (strcmp((char *)type,"GIF89a")==0)
		version=89;
	else if (strcmp((char *)type,"GIF87a")==0)
		version=87;
	else
	{
		Close();
		return(false);	/* not a gif image at all! */
	}

	m_imagetype=GUISHAPE_GIF;
	m_imagewidth=getshort(this);
	m_imageheight=getshort(this);
	m_bpp=4;

	if(justsize==true)
	{
		Close();
		return(true);
	}

	/* load header */

#if TRACEGIF
	kGUI::Trace("Loading picture '%s'\n",GetFilename());
#endif

	resolution=dhgetc(this);
	bgcolor=dhgetc(this);
	aspect=dhgetc(this);

	colors=1<<((resolution&7)+1);

	if (resolution&128)
	{
		assert(colors<=256,"Palette Overflow!");
		for (t=0; t<colors; t++)
		{
			gpalr[t]=dhgetc(this);
			gpalg[t]=dhgetc(this);
			gpalb[t]=dhgetc(this);
		}
	}

	/* load extension blocks */

	while(1)
	{
nextframe:;
		ch=dhgetc(this);
		if (ch=='!')
		{
			int xb=dhgetc(this);
			int xblen;
			unsigned char *xbptr;

			while(1)
			{
				xblen=dhgetc(this);
				if (!xblen)
					break;
				xbuf.Alloc(xblen);
				xbptr=xbuf.GetArrayPtr();
				for (t=0; t<xblen; t++)
					xbptr[t]=(unsigned char)dhgetc(this);

				if(xb==0xf9)
				{
					/* turn off last transparent color? */
					if(transp==true)
					{
						transp=false;
						pala[bgcolor]=255;
#if TRACEGIF
						kGUI::Trace("Turning off transparent color #%d\n",bgcolor);
#endif
					}

					if(xbptr[0]&1)
					{
						transp=true;
						bgcolor=xbptr[3];		/* not sure if this is correct */
						pala[bgcolor]=0;
#if TRACEGIF
						kGUI::Trace("Turning on transparent color #%d\n",bgcolor);
#endif
					}
					disposal=(xbptr[0]>>2)&3;
					totaldelay100+=(xbptr[2]<<8)|xbptr[1];
				}
			}
		}
		else
		{
			Seek(GetOffset()-1);
			break;
		}
	}

	/* load image descriptor */

	t=dhgetc(this);

	if (t!=',')
		goto gifdone;

	lx=getshort(this);
	ty=getshort(this);

	/* get frame size, can be smaller for animated gifs */
	fw=getshort(this);
	fh=getshort(this);

	/* we can expand if this is the first frame but not after */
	if(fw>m_imagewidth && !m_numframes)
		m_imagewidth=fw;
	if(fh>m_imageheight && !m_numframes)
		m_imageheight=fh;

	assert(fw<=m_imagewidth,"Error, too wide");
	assert(fh<=m_imageheight,"Error, too tall");

	flags=dhgetc(this);
#if TRACEGIF
	kGUI::Trace("Frame %d, x=%d,y=%d,w=%d,h=%d, flags=%02x\n",m_numframes,lx,ty,fw,fh,flags);
#endif

	picture=(unsigned char *)new char[(m_imagewidth*m_imageheight*4)];
	assert(picture!=0,"Error allocating space for image!");
	m_imagedata.SetEntry(m_numframes,picture);

	/* convert delay from 1/100ths of a second to TICKSPERSEC */
	totaldelaytps=(totaldelay100*TICKSPERSEC)/100;
	deltatps=totaldelaytps-lastdelaytps;
	if(deltatps<1)
		deltatps=1;
	lastdelaytps+=deltatps;

	m_delays.SetEntry(m_numframes,(unsigned int)deltatps);


	if(lastframe<0)
	{
		memset(picture,0,(m_imagewidth*m_imageheight*4));
	}
	else
		memcpy(picture,m_imagedata.GetEntry(lastframe),(m_imagewidth*m_imageheight*4));

	/* I could assert here too, but it's better to just continue and show garbage */
	if((lx+fw)>m_imagewidth)
		lx=m_imagewidth-fw;
	if((ty+fh)>m_imageheight)
		ty=m_imageheight-fh;

	picture+=(lx*4);
	picture+=(ty*m_imagewidth*4);

	/* update the status for initializing the next frame ( animated gifs only ) */
	switch(disposal)
	{
	case 0: /* leave this frame up */
	case 1:	/* leave this frame up */
		lastframe=m_numframes;
	break;
	case 2:	/* clear to black */
		lastframe=-1;	
	break;
	case 3:	/* leave previously set last frame as is */
	break;
	}

	++m_numframes;

	/* build an array of start/end pointers so decompress code */
	/* will work with both interlaced and non-interlaces pics */
	
	/* we also put a null pointer at the end to detect attemped overwrites */

	if (flags&64)
	{
		unsigned int pass,y,ystep;

		interlaced=true;
		giflines=new GifLine[fh+1];

		gl=giflines;
		for(pass=0;pass<4;++pass)
		{
			y=intstart[pass];
			ystep=intstep[pass];
			while(y<fh)
			{
				gl->start=picture+(y*m_imagewidth*4);
				gl->end=gl->start+(fw*4);
				++gl;
				y+=ystep;
			}
		}
		gl->start=0;
		gl->end=0;
	}
	else if(fw!=m_imagewidth || fh!=m_imageheight)
	{
		unsigned int y;

		/* since picture is smaller than full size use edge table */
		interlaced=false;
		giflines=new GifLine[fh+1];

		gl=giflines;
		for(y=0;y<fh;++y)
		{
			gl->start=picture+(y*m_imagewidth*4);
			gl->end=gl->start+(fw*4);
			++gl;
		}
		gl->start=0;
		gl->end=0;
	}
	else
	{
		interlaced=false;
		giflines=new GifLine[2];
		giflines[0].start=picture;
		giflines[0].end=picture+(m_imagewidth*m_imageheight*4);
		giflines[1].start=0;
		giflines[1].end=0;
	}

	/* start writing here! */
	picture=giflines[0].start;

	gl=giflines;	/* point to start of line list */

	/* local palette? */
	if (flags&128)
	{
		x=1<<((flags&7)+1);
		assert(x<=256,"Palette Overflow!");
#if TRACEGIF
		kGUI::Trace("Loading palette #%d colors\n",x);
#endif
		for (t=0; t<x; t++)
		{
			palr[t]=dhgetc(this);
			palg[t]=dhgetc(this);
			palb[t]=dhgetc(this);
			pala[t]=255;
		}
	}
	else
	{
		/* no local palette, use global palette */
#if TRACEGIF
		kGUI::Trace("No local palette, using global palette\n");
#endif
		for (t=0; t<256; t++)
		{
			palr[t]=gpalr[t];
			palg[t]=gpalg[t];
			palb[t]=gpalb[t];
			pala[t]=255;
		}
	}
	if(transp==true)
	{
#if TRACEGIF
		kGUI::Trace("Setting bgcolor #%d to transparent\n",bgcolor);
#endif
		pala[bgcolor]=0;
	}

	code_size=dhgetc(this);
	start_table_size=(1<<code_size);
	assert((unsigned int)start_table_size<(sizeof(node)/sizeof(GifNode)),"Overflow");
	for (t=0; t<start_table_size; t++)
	{
		node[t].prev=-1;
		node[t].color=t; 
	}

	clear_code=t;
	eof_code=t+1;
	next_code=t+2;

	code_size++;

	x=0;
	y=0;
	holding=0;
	bitptr=0;
	curr_code_size=code_size;
	old_code=-1;

	mask=(1<<curr_code_size)-1;
	data_count=0;

	while(1)
	{
		while (bitptr<curr_code_size)
		{
			if (data_count==0)
			{
				if ((data_count=dhgetc(this))==EOF)
					break;
				if (data_count==0)
					break;
			}
			t=dhgetc(this);
			holding=holding+(t<<bitptr);
			data_count--;
			bitptr=bitptr+8;
		}

		if (t==EOF || data_count==EOF)
		{
			break;
		}
		code=holding&mask;

		holding=holding>>curr_code_size;
		bitptr=bitptr-curr_code_size;
	
		if (code==clear_code)
		{
			curr_code_size=code_size;
			next_code=start_table_size+2;
			old_code=-1;
			mask=(1<<curr_code_size)-1;
			continue;
		}
		else if (old_code==-1)
		{
			pi=node[code].color;

			if(m_numframes>1 && pala[pi]!=255)
				picture+=4;
			else
			{
				*(picture++)=palr[pi];
				*(picture++)=palg[pi];
				*(picture++)=palb[pi];
				*(picture++)=pala[pi];
			}
			if(picture==gl->end)
			{
				++gl;
				picture=gl->start;
				if(!picture)
					goto endframe;
			}
		}
		else if (code==eof_code)
			break;
		else
		{
			if (code<next_code)
			{
				t=0;
				r=code;
				while(1)
				{
					/* corrupt file? */
					if(t==sizeof(temp_buffer))
						goto gifdone;

					temp_buffer[t++]=node[r].color;
					if (node[r].prev==-1)
						break;
					r=node[r].prev;
				}

				for (r=t-1; r>=0; r--)
				{
					pi=temp_buffer[r];

					if(m_numframes>1 && pala[pi]!=255)
						picture+=4;
					else
					{
						*(picture++)=palr[pi];
						*(picture++)=palg[pi];
						*(picture++)=palb[pi];
						*(picture++)=pala[pi];
					}
					if(picture==gl->end)
					{
						++gl;
						picture=gl->start;
						if(!picture)
							goto endframe;
					}
				}
				assert((unsigned int)next_code<(sizeof(node)/sizeof(GifNode)),"Overflow");
				node[next_code].color=temp_buffer[t-1];
				node[next_code].prev=old_code;

				if (next_code==mask && mask!=4095)
				{
					curr_code_size++;
					mask=(1<<curr_code_size)-1;
				}
				next_code++;
			}
			else
			{
				t=0;
				r=old_code;
				while(1)
				{
					assert((unsigned int)t<sizeof(temp_buffer),"Overflow");
					temp_buffer[t++]=node[r].color;
					if (node[r].prev==-1)
						break;
					r=node[r].prev;
				}
				assert((unsigned int)next_code<(sizeof(node)/sizeof(GifNode)),"Overflow");
				node[next_code].color=temp_buffer[t-1];
				node[next_code].prev=old_code;

				if (next_code==mask && mask!=4095)
				{
					curr_code_size++;
					mask=(1<<curr_code_size)-1;
				}
				next_code++;
				for (r=t-1; r>=0; r--)
				{
					pi=temp_buffer[r];

					if(m_numframes>1 && pala[pi]!=255)
						picture+=4;
					else
					{
						*(picture++)=palr[pi];
						*(picture++)=palg[pi];
						*(picture++)=palb[pi];
						*(picture++)=pala[pi];
					}
					if(picture==gl->end)
					{
						++gl;
						picture=gl->start;
						if(!picture)
							goto endframe;
					}
				}
				pi=temp_buffer[t-1];

				if(m_numframes>1 && pala[pi]!=255)
					picture+=4;
				else
				{
					*(picture++)=palr[pi];
					*(picture++)=palg[pi];
					*(picture++)=palb[pi];
					*(picture++)=pala[pi];
				}
				if(picture==gl->end)
				{
					++gl;
					picture=gl->start;
					if(!picture)
						goto endframe;
				}
			}
		}
		old_code=code;
	}

endframe:;
	delete []giflines;
	giflines=0;
	t=dhgetc(this);	/* eat and throw away? */

	do
	{
		if(GetOffset()==GetSize())
			break;
		t=dhgetc(this);
		switch(t)
		{
		case 0x3b:
			goto gifdone;
		break;
		case '!':
		case ',':
			Seek(GetOffset()-1);
			goto nextframe;
		break;
		}
	}while(1);

gifdone:;
	if(giflines)
		delete []giflines;
	Close();
	if(!m_numframes)
		return(false);
	else
		return(true);
}

/***************error handler******************/

struct my_error_mgr 
{
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/***************input handler******************/

typedef struct {
struct jpeg_source_mgr pub; /* public fields */
unsigned char buffer[65536];
DataHandle *jdh;				/* pointer to current datahandle */
} my_source_mgr;

typedef my_source_mgr *my_src_ptr;

METHODDEF(void) init_source (j_decompress_ptr cinfo)
{
	my_source_mgr *src = (my_source_mgr *) cinfo->src;

	src->pub.next_input_byte=src->buffer;
	src->pub.bytes_in_buffer=src->jdh->Read(src->buffer,(unsigned long)sizeof(src->buffer));
}

METHODDEF(boolean) fill_input_buffer (j_decompress_ptr cinfo)
{
	my_source_mgr *src = (my_source_mgr *) cinfo->src;

	src->pub.next_input_byte=src->buffer;
	src->pub.bytes_in_buffer=src->jdh->Read(src->buffer,(unsigned long)sizeof(src->buffer));
	
	/* hit eof?, fill buffer with end data! */
	if(!src->pub.bytes_in_buffer)
	{
		/* Create a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		src->pub.bytes_in_buffer = 2;
	}
	return (TRUE);
}

METHODDEF(void) skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_source_mgr *src = (my_source_mgr *) cinfo->src;

	if (num_bytes > 0)
	{
		while (num_bytes > (long) src->pub.bytes_in_buffer)
		{
			num_bytes -= (long) src->pub.bytes_in_buffer;
			fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void) term_source (j_decompress_ptr cinfo)
{
/* no work necessary here */
}

bool kGUIImage::LoadJPGImage(bool justsize)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	unsigned char *pic;
	unsigned char *readptr;
	unsigned int line;
	my_source_mgr src;
	
	src.jdh=(DataHandle *)this;		/* save global for memory system to use */
	if(Open()==false)
		return(false);

	cinfo.err = jpeg_std_error(&jerr.pub);

	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		Close();
		return(false);
	}	

	jpeg_create_decompress(&cinfo);

	cinfo.src=(jpeg_source_mgr *)&src;

	src.pub.init_source = init_source;
	src.pub.fill_input_buffer = fill_input_buffer;
	src.pub.skip_input_data = skip_input_data;
	src.pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src.pub.term_source = term_source;
	src.pub.bytes_in_buffer = 0;

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	m_imagewidth = cinfo.output_width;
	m_imageheight = cinfo.output_height;
	m_bpp=3;
	if(justsize==false)
	{
		pic = new unsigned char[(m_imageheight*m_imagewidth*3)];
		m_numframes=1;
		m_imagedata.SetEntry(0,pic);
		readptr=pic;
		line=0;
		while (cinfo.output_scanline < (unsigned int) m_imageheight)
		{
			assert(cinfo.output_scanline==line,"Internal Error!");
			++line;

			/* is this greyscale? */
			if(cinfo.out_color_components == 1)
			{
				int w;
				unsigned char *gp;

				gp=readptr+(m_imagewidth<<1);
				jpeg_read_scanlines(&cinfo, &gp, 1);

				/* expand greyscale to rgb */
				w=m_imagewidth;
				while(w)
				{
					*(readptr++)=*(gp);
					*(readptr++)=*(gp);
					*(readptr++)=*(gp++);
					--w;
				}
			}
			else
			{
				jpeg_read_scanlines(&cinfo, &readptr, 1);
				readptr+=m_imagewidth*3;
			}
		}
		m_imagetype=GUISHAPE_JPG;
		jpeg_finish_decompress(&cinfo);
	}
	jpeg_destroy_decompress(&cinfo);
	Close();
	return(true);
}

/*
 * Sample routine for JPEG compression.  We assume that the target file name
 * and a compression quality factor are passed in.
 */

bool kGUIImage::SaveJPGImage(const char *filename,int quality)
{
	/* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * outfile;		/* target file */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
  /* Step 1: allocate and initialize JPEG compression object */
  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
	unsigned char *linebuffer=0;
	unsigned char *lb;
	unsigned char *sb;
	unsigned int x;
	unsigned char a;
	unsigned char *data=m_imagedata.GetEntry(0);

  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);
  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */
  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = fopen(filename, "wb")) == NULL)
	return(false);
 
  jpeg_stdio_dest(&cinfo, outfile);
  /* Step 3: set parameters for compression */
  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = m_imagewidth; 	/* image width and height, in pixels */
  cinfo.image_height = m_imageheight;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
  /* Step 4: Start compressor */
  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);
  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */
  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = m_imagewidth * m_bpp;	/* JSAMPLEs per row in image_buffer */
  linebuffer=new unsigned char[m_imagewidth * 3];
  while (cinfo.next_scanline < cinfo.image_height)
  {
		/* convert scan line from its internal format to rgb */
	    sb=&data[cinfo.next_scanline * row_stride];
		lb=linebuffer;
		for(x=0;x<m_imagewidth;++x)
		{
			ReadPixel(sb,lb,lb+1,lb+2,&a);
			lb+=3;
			sb+=m_bpp;
		}
	  
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
	  row_pointer[0] = linebuffer;
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  /* Step 6: Finish compression */
  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose(outfile);
  /* Step 7: release JPEG compression object */
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);
  /* And we're done! */
  delete []linebuffer;
  return(true);
}

//DataHandle *pnghandle;

void read_function(png_structp read, png_bytep data, png_size_t length)
 {
	 DataHandle *pnghandle;

	 pnghandle=(DataHandle *)png_get_io_ptr(read);
	 pnghandle->Read(data,(unsigned long)length);
//     static int buffer_location = 0;
//     memcpy(data, 1, length, buffer + location);
//     buffer_location += length;
}

bool kGUIImage::LoadPNGImage(bool justsize)
{
	png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
	unsigned char *readptr;
	DataHandle *pnghandle;

	/* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */

	if(Open()==false)
		return(false);

	pnghandle=this;				/* save pointer to DataHandle for feeding data */

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,NULL, NULL);

   if (png_ptr == NULL)
   {
      Close();
      return(false);
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      Close();
      return(false);
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      Close();
      /* If we get here, we had a problem reading the file */
      return(false);
   }

   /* Set up the input control if you are using standard C streams */
	png_set_read_fn(png_ptr, (void *)pnghandle, read_function);

   /*
    * If you have enough memory to read in the entire image at once,
    * and you need to specify only transforms that can be controlled
    * with one of the PNG_TRANSFORM_* bits (this presently excludes
    * dithering, filling, setting background, and doing gamma
    * adjustment), then you can read the entire image (including
    * pixels) into the info structure with this call:
    */
//   png_read_png(png_ptr, info_ptr, png_transforms, png_voidp_NULL);

	m_imagetype=GUISHAPE_PNG;
	m_bpp=4;
	if(justsize==true)
	{
		png_read_info(png_ptr, info_ptr);

		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
			&interlace_type, int_p_NULL, int_p_NULL);
		m_imagewidth = width;
		m_imageheight = height;
	}
	else
	{
		kGUIBitStream bs;
		int bpp;
		png_color *greypal=0;

		bs.SetReverseIn();	/* only used for 1-7 bit depth */
		bs.SetReverseOut();	/* only used for 1-7 bit depth */
		png_read_png(png_ptr, info_ptr, 0, png_voidp_NULL);

		bpp=info_ptr->pixel_depth;
		m_imagewidth = info_ptr->width;
		m_imageheight = info_ptr->height;
		color_type = info_ptr->color_type;

		switch(color_type)
		{
		case PNG_COLOR_TYPE_GRAY:
		{
			/* generate a grey-scale palette */
			unsigned int e,ne;
			png_color *ge;

			ne=1<<bpp;

			greypal=new png_color[ne];
			ge=greypal;
			for(e=0;e<ne;++e)
			{
				ge->red=ge->green=ge->blue=(unsigned int)((e*255.0f)/(ne-1));
				++ge;
			}
			info_ptr->palette=greypal;
		}
		break;
		case PNG_COLOR_MASK_ALPHA:
		{
			/* generate a grey-scale palette */
			unsigned int e,ne;
			png_color *ge;

			ne=1<<(bpp>>1);

			greypal=new png_color[ne];
			ge=greypal;
			for(e=0;e<ne;++e)
			{
				ge->red=ge->green=ge->blue=(unsigned int)((e*255.0f)/(ne-1));
				++ge;
			}
			info_ptr->palette=greypal;
		}

		break;
		}

		/* At this point you have read the entire image */

		readptr = new unsigned char[(m_imageheight*m_imagewidth*4)];
		assert(readptr!=0,"Error: unable to allocate space for PNG image!");
		m_numframes=1;
		m_imagedata.SetEntry(0,readptr);

		/* put the data here */
		for(unsigned int y=0;y<m_imageheight;++y)
		{
			const unsigned char *rowdata=info_ptr->row_pointers[y];
			switch(bpp)
			{
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			{
				unsigned int pixel;

				assert(info_ptr->palette!=0,"No palette!");

				bs.Set(rowdata);
                for(unsigned int x=0;x<m_imagewidth;++x)
				{
					pixel=bs.ReadU(bpp);
					readptr[0]=info_ptr->palette[pixel].red;
					readptr[1]=info_ptr->palette[pixel].green;
					readptr[2]=info_ptr->palette[pixel].blue;

					if(info_ptr->num_trans)
					{
						if(color_type==PNG_COLOR_TYPE_GRAY && pixel==info_ptr->trans_values.gray)
							readptr[3]=0;
						else if(info_ptr->trans && pixel<info_ptr->num_trans)
							readptr[3]=info_ptr->trans[pixel];
						else
							readptr[3]=255;
					}
					else
						readptr[3]=255;
					readptr+=4;
				}
			}
			break;
			case 8:
				for(unsigned int x=0;x<m_imagewidth;++x)
				{
					unsigned int pvalue=*(rowdata++);

					if(info_ptr->palette)
					{
						readptr[0]=info_ptr->palette[pvalue].red;
						readptr[1]=info_ptr->palette[pvalue].green;
						readptr[2]=info_ptr->palette[pvalue].blue;
						if(info_ptr->trans  && pvalue<info_ptr->num_trans)
							readptr[3]=info_ptr->trans[pvalue];
						else
							readptr[3]=255;
					}
					else
					{
						readptr[0]=pvalue;
						readptr[1]=pvalue;
						readptr[2]=pvalue;
						readptr[3]=255;
					}
					readptr+=4;
				}
			break;
			case 16:
				if(color_type==PNG_COLOR_MASK_ALPHA)
				{
					const unsigned char *rowp=rowdata;
					for(unsigned int x=0;x<m_imagewidth;++x)
					{
						int index=rowp[0];
						int alpha=rowp[1];

						*(readptr++)=info_ptr->palette[index].red;
						*(readptr++)=info_ptr->palette[index].green;
						*(readptr++)=info_ptr->palette[index].blue;
						*(readptr++)=alpha;
						rowp+=2;
					}
				}
				else
				{
					const unsigned short *rowp=(const unsigned short *)rowdata;
					unsigned short rgb;

					for(unsigned int x=0;x<m_imagewidth;++x)
					{
						if(info_ptr->palette)
						{
							int index=(((const unsigned char *)rowp)[0]<<8)|((const unsigned char *)rowp)[1];

							*(readptr++)=info_ptr->palette[index].red;
							*(readptr++)=info_ptr->palette[index].green;
							*(readptr++)=info_ptr->palette[index].blue;

							if(color_type==PNG_COLOR_TYPE_GRAY)
							{
								if(index==info_ptr->trans_values.gray)
									*(readptr++)=0;
								else
									*(readptr++)=255;
							}
							else if(info_ptr->num_trans  && info_ptr->trans_values.index==index)
								*(readptr++)=0;
							else if(info_ptr->trans && index<info_ptr->num_trans)
								*(readptr++)=info_ptr->trans[index];
							else
								*(readptr++)=255;
						}
						else
						{
							rgb=*(rowp);
						
							/* expand 4 bits RGB into 8 bits */
							*(readptr++)=(int)(((rgb>>8)&15)*(255.0f/15));
							*(readptr++)=(int)(((rgb>>4)&15)*(255.0f/15));
							*(readptr++)=(int)(((rgb)&15)*(255.0f/15));
							*(readptr++)=255;
						}
						++rowp;
					}
				}
			break;
			case 24:
			{
				const unsigned char *rowp=rowdata;
				for(unsigned int x=0;x<m_imagewidth;++x)
				{
					if(info_ptr->num_trans  && info_ptr->trans_values.red==rowp[0]
											&& info_ptr->trans_values.green==rowp[1]
											&& info_ptr->trans_values.blue==rowp[2])
					{
						*(readptr++)=*(rowp++);
						*(readptr++)=*(rowp++);
						*(readptr++)=*(rowp++);
						*(readptr++)=0;
					}
					else
					{
						*(readptr++)=*(rowp++);
						*(readptr++)=*(rowp++);
						*(readptr++)=*(rowp++);
						*(readptr++)=255;
					}
				}
			}
			break;
			case 32:
				if(color_type==PNG_COLOR_MASK_ALPHA)
				{
					const unsigned char *rowp=rowdata;
					for(unsigned int x=0;x<m_imagewidth;++x)
					{
						int index=((rowp)[0]<<8)|(rowp)[1];
						int alpha=((rowp)[2]<<8)|(rowp)[3];

						*(readptr++)=info_ptr->palette[index].red;
						*(readptr++)=info_ptr->palette[index].green;
						*(readptr++)=info_ptr->palette[index].blue;
						*(readptr++)=alpha>>8;
						rowp+=4;
					}
				}
				else
				{
					memcpy(readptr,rowdata,m_imagewidth*4);
					readptr+=m_imagewidth*4;
					//rowdata+=m_imagewidth*4;
				}
			break;
			case 48:
			{
				const unsigned char *rowp=rowdata;
				for(unsigned int x=0;x<m_imagewidth;++x)
				{
					const unsigned short *sp=(const unsigned short *)rowp;

					if(info_ptr->num_trans  && info_ptr->trans_values.red==sp[0]
											&& info_ptr->trans_values.green==sp[1]
											&& info_ptr->trans_values.blue==sp[2])
					{
						*(readptr++)=rowp[0];
						*(readptr++)=rowp[2];
						*(readptr++)=rowp[4];
						*(readptr++)=0;
					}
					else
					{
						*(readptr++)=rowp[0];
						*(readptr++)=rowp[2];
						*(readptr++)=rowp[4];
						*(readptr++)=255;
					}
					rowp+=6;
				}
			}
			break;
			case 64:
			{
				const unsigned char *rowp=rowdata;
				for(unsigned int x=0;x<m_imagewidth;++x)
				{
					*(readptr++)=rowp[0];
					*(readptr++)=rowp[2];
					*(readptr++)=rowp[4];
					*(readptr++)=rowp[6];
					rowp+=8;
				}
			}
			break;
			default:
				assert(false,"uncoded PNG format");
			break;
			}
		}
		if(greypal)
		{
			info_ptr->palette=0;
			delete []greypal;
		}
   }
   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   /* close the file */
   Close();

   /* LOADED! that's it */
	return(true);
}

typedef struct
{
	unsigned short reserved;
	unsigned short type;
	unsigned short count;	/* used */
}WINICO_Header;

typedef struct
{
	unsigned char width;	/* used */
	unsigned char height;	/* used */
	unsigned char color_count;
	unsigned char reserved;
	unsigned short planes;
	unsigned short bits;
	unsigned int size;
	unsigned int offset;	/* used */
}WINICO_Header1;

typedef struct
{
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bits;	/* used */
	unsigned int compression;
	unsigned int imagesize;
	unsigned int xpixperm;
	unsigned int ypixperm;
	unsigned int colors;
	unsigned int impcolors;
}WINICO_Header2;

/* windows icon format for favicon.ico */
bool kGUIImage::LoadWINICOImage(bool justsize)
{
	unsigned int i,n,numcolors=0,x,y,bpp,apixel,xpixel,arowpad;
	WINICO_Header header;
	Array<WINICO_Header1> header1;
	WINICO_Header2 header2;
	Array<unsigned char> palr;
	Array<unsigned char> palg;
	Array<unsigned char> palb;
	WINICO_Header1 *h1p;
	unsigned char b,g,r,a;
	unsigned char *writeptr;
	unsigned char *wp;
	kGUIBitStream aabs;
	unsigned long apackedbits,apackedbytes;
	Array<unsigned char> abitbuffer;
	unsigned int best,bestw,besth;
	unsigned long off;
	kGUIBitStream xbs;
	unsigned long xpackedbits,xpackedbytes;
	Array<unsigned char> xbitbuffer;

	if(Open()==false)
		return(false);

	Read(&header,(unsigned long)sizeof(WINICO_Header));

	if(!(ReadU16((const char *)&header.reserved)==0 && ReadU16((const char *)&header.type)==1))
	{
		Close();
		return(false);
	}

	n=ReadU16((const char *)&header.count);
	header1.Alloc(n);

	/* all header1s are first */
	best=0;
	bestw=0;
	besth=0;
	for(i=0;i<n;++i)
	{
		Read(header1.GetEntryPtr(i),(unsigned long)sizeof(WINICO_Header1));

		/* we will load the largest one */
		h1p=header1.GetEntryPtr(i);
		if(h1p->width>=best && h1p->height>=besth)
		{
			best=i;
			bestw=h1p->width;
			besth=h1p->height;
		}
	}
	/* each header1 has an offset to it's header2 and then the bitmap data follows the header2 */
	h1p=header1.GetEntryPtr(best);

	off=ReadU32((const char *)&h1p->offset);
	if(off>GetSize())
	{
		Close();
		return(false);
	}

	Seek(off);
	Read(&header2,(unsigned long)sizeof(WINICO_Header2));

	m_imagewidth=h1p->width;
	m_imageheight=h1p->height;

	bpp=ReadU16((const char *)&header2.bits);

	/* does this icon have a palette or is it in 32 bit? */
	if(bpp==32)
	{
		writeptr = new unsigned char[(m_imageheight*m_imagewidth*4)];
		assert(writeptr!=0,"Error: unable to allocate space for WIN ICON image!");
		m_numframes=1;
		m_imagedata.SetEntry(0,writeptr);

		/* load the bitmap using 32bit RGBA data */
		for(y=0;y<m_imageheight;++y)
		{
			wp=writeptr+(((m_imageheight-1)-y)*(m_imagewidth*4));
			for(x=0;x<m_imagewidth;++x)
			{
				Read(&b,(unsigned long)1);
				Read(&g,(unsigned long)1);
				Read(&r,(unsigned long)1);
				Read(&a,(unsigned long)1);

				wp[0]=r;
				wp[1]=g;
				wp[2]=b;
				wp[3]=a;
				wp+=4;
			}
		}
	}
	else
	{
		if(bpp<=8)
		{
			numcolors=h1p->color_count;
			if(!numcolors)
				numcolors=256;

			/* load the palette */
			palr.Alloc(numcolors);
			palg.Alloc(numcolors);
			palb.Alloc(numcolors);

			for(i=0;i<numcolors;++i)
			{
				Read(&b,(unsigned long)1);
				Read(&g,(unsigned long)1);
				Read(&r,(unsigned long)1);
				Read(&a,(unsigned long)1);
				palr.SetEntry(i,r);
				palg.SetEntry(i,g);
				palb.SetEntry(i,b);
			}
		}

		/* xor data is bitpacked so calc # bytes */

		xpackedbits=bpp*m_imagewidth*m_imageheight;
		xpackedbytes=xpackedbits>>3;
		if(xpackedbits&7)
			++xpackedbytes;
		xbitbuffer.Alloc(xpackedbytes);
		Read(xbitbuffer.GetArrayPtr(),xpackedbytes);
		xbs.SetReverseIn();
		xbs.Set(xbitbuffer.GetArrayPtr());

		/* and bitmap data is 1 bpp */
		/* each row is padded out to the next 4 bytes */

		apackedbits=1*m_imagewidth;
		if(apackedbits&31)
			arowpad=(32-apackedbits&31);
		else
			arowpad=0;

		apackedbytes=(m_imageheight*(apackedbits+arowpad))>>3;

		abitbuffer.Alloc(apackedbytes);
		Read(abitbuffer.GetArrayPtr(),apackedbytes);
		aabs.SetReverseIn();
		aabs.Set(abitbuffer.GetArrayPtr());

		writeptr = new unsigned char[(m_imageheight*m_imagewidth*4)];
		assert(writeptr!=0,"Error: unable to allocate space for WIN ICON image!");
		m_numframes=1;
		m_imagedata.SetEntry(0,writeptr);

		/* load the bitmap using the palette */
		for(y=0;y<m_imageheight;++y)
		{
			wp=writeptr+(((m_imageheight-1)-y)*(m_imagewidth*4));
			for(x=0;x<m_imagewidth;++x)
			{
				apixel=aabs.ReadU(1);
				xpixel=xbs.XReadU(bpp);
				
				/* and AND palette is only 1 bit color values should be 0x80 or 0x00 */
				if(apixel==0)
				{
					switch(bpp)
					{
					case 32:
						a=(xpixel>>24)&255;
						b=(xpixel>>16)&255;
						g=(xpixel>>8)&255;
						r=(xpixel)&255;
					break;
					case 24:
						a=255;
						b=(xpixel>>16)&255;
						g=(xpixel>>8)&255;
						r=(xpixel)&255;
					break;
					case 16:
						a=255;
						/* todo, make more 0-31 goto 0-255 */
						b=((xpixel>>10)&0x1f)<<3;
						g=((xpixel>>5)&0x1f)<<3;
						r=((xpixel)&0x1f)<<3;
					break;
					default:
						if(xpixel<numcolors)
						{
							r=palr.GetEntry(xpixel);
							g=palg.GetEntry(xpixel);
							b=palb.GetEntry(xpixel);
						}
						else
						{
							/* error! */
							r=g=b=0;
						}
						a=255;
					break;
					}
				}
				else
				{
					a=0;	/* transparent */
					r=0;
					g=0;
					b=0;
				}

				wp[0]=r;
				wp[1]=g;
				wp[2]=b;
				wp[3]=a;
				wp+=4;
			}
			/* throw away pad bits */
			for(x=0;x<arowpad;++x)
				aabs.ReadU(1);
		}
	}

	/* once read in it is the same format as PNG */
	m_imagetype=GUISHAPE_PNG;
	m_bpp=4;
	Close();
	return(true);
}

/*! @internal @struct BMP_Header
    @brief Internal struct used by the kGUIImage class
	@ingroup kGUIImage */
typedef struct
{
	unsigned char type[2];
	unsigned char size[4];
	unsigned char reserved1[2];
	unsigned char reserved2[2];
	unsigned char offset[4];
	unsigned char headersize[4];	/* should be 40 bytes? */
	unsigned char width[4];
	unsigned char height[4];
	unsigned char nplanes[2];		/* should be 1 */
	unsigned char bpp[2];			/* 1, 4, 8, 16, 24 and 32 */
	unsigned char compression[4];	/* 0 to 5, see enums */
	unsigned char imagesize[4];
	unsigned char hppm[4];			/* horiz: pixels per meter */
	unsigned char vppm[4];			/* vertical: pixels per meter */
	unsigned char numpalettecolors[4];
}BMP_Header;

enum
{
BMP_RGB,
BMP_RLE8,
BMP_RLE4,
BMP_BITFIELDS,
BMP_JPEG,
BMP_PNG
};

/* BMP */
bool kGUIImage::LoadBMPImage(bool justsize)
{
	unsigned int i;
	unsigned int bpp;
	unsigned int compression;
	unsigned char *writeptr;
	unsigned int offset;
	unsigned int x,y;
	unsigned char *wp;
	unsigned int numpalettecolors;
	unsigned char palr[256];
	unsigned char palg[256];
	unsigned char palb[256];
	unsigned int nindex;
	unsigned int index;
	unsigned int index2=0;	/* only set to 0 to stop the compiler from complaining */
	unsigned int imagewidthpad;
	kGUIBitStream aabs;
	Array<unsigned char> abitbuffer;
	unsigned long apackedbytes;

	BMP_Header header;

	if(Open()==false)
		return(false);

	Read(&header,(unsigned long)sizeof(BMP_Header));

	if(!(header.type[0]==0x42 && header.type[1]==0x4d))
	{
		Close();
		return(false);
	}

	/* it is a BMP, so let's load it */

	m_imagewidth=ReadU32((const char *)&header.width);
	m_imageheight=ReadU32((const char *)&header.height);

	bpp=ReadU16((const char *)&header.bpp);
	compression=ReadU32((const char *)&header.compression);
	offset=ReadU32((const char *)&header.offset);
	numpalettecolors=ReadU32((const char *)&header.numpalettecolors);

	/* widths are padded to next 4 bytes */
	imagewidthpad=(m_imagewidth*bpp)>>3;
	while(imagewidthpad&3)
		++imagewidthpad;
	imagewidthpad-=(m_imagewidth*bpp)>>3;

	if(bpp==4 && !numpalettecolors)
		numpalettecolors=16;
	else if(bpp==8 && !numpalettecolors)
		numpalettecolors=256;

	/* load palette if applicable */
	if(numpalettecolors)
	{
		assert(numpalettecolors<=256,"Too many colors in the palette!");

		offset=ReadU32((const char *)&header.headersize)+14;
		Seek(offset);
		for(i=0;i<numpalettecolors;++i)
		{
			Read(palb+i,(unsigned long)1);
			Read(palg+i,(unsigned long)1);
			Read(palr+i,(unsigned long)1);
			Read(&x,(unsigned long)1);	/* throw away */
		}
	}

	offset=ReadU32((const char *)&header.offset);
	Seek(offset);
	apackedbytes=(m_imageheight*(m_imagewidth+imagewidthpad)*bpp)>>3;
	abitbuffer.Alloc(apackedbytes);
	Read(abitbuffer.GetArrayPtr(),apackedbytes);
	aabs.Set(abitbuffer.GetArrayPtr());

	switch(compression)
	{
	case BMP_RGB:
		writeptr = new unsigned char[(m_imageheight*m_imagewidth*4)];
		assert(writeptr!=0,"Error: unable to allocate space for BMP image!");
		m_numframes=1;
		m_imagedata.SetEntry(0,writeptr);

		for(y=0;y<m_imageheight;++y)
		{
			wp=writeptr+(((m_imageheight-1)-y)*(m_imagewidth*4));
			nindex=0;
			for(x=0;x<m_imagewidth;++x)
			{
				/* read RGB, then put 'A' */
				switch(bpp)
				{
				case 4:
					if(!nindex)
					{
						index2=aabs.ReadU(4);
						index=aabs.ReadU(4);
					}
					else
						index=index2;
					nindex^=1;

					wp[0]=palr[index];
					wp[1]=palg[index];
					wp[2]=palb[index];
					wp[3]=0xff;
				break;
				case 8:
					index=aabs.ReadU(8);
					wp[0]=palr[index];
					wp[1]=palg[index];
					wp[2]=palb[index];
					wp[3]=0xff;
				break;
				case 16:
					/* only x555 */
					index=aabs.ReadU(16);
					/* todo, scale up fractionally so 0x1f = 0xff not 0xf8 */
					wp[0]=((index>>10)&0x1f)<<3;
					wp[1]=((index>>5)&0x1f)<<3;
					wp[2]=((index)&0x1f)<<3;
					wp[3]=0xff;
				break;
				case 24:
					wp[2]=aabs.ReadU(8);
					wp[1]=aabs.ReadU(8);
					wp[0]=aabs.ReadU(8);
					wp[3]=0xff;
				break;
				case 32:
					wp[3]=aabs.ReadU(8);
					wp[2]=aabs.ReadU(8);
					wp[0]=aabs.ReadU(8);
					wp[1]=aabs.ReadU(8);
				break;
				}
				wp+=4;
			}

			/* throw away pad bytes */
			if(imagewidthpad)
				aabs.ReadU(imagewidthpad<<3);
		}

	break;
	case BMP_RLE8:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	case BMP_RLE4:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	case BMP_BITFIELDS:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	case BMP_JPEG:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	case BMP_PNG:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	default:
		/* unknown / unsupported format */
		Close();
		return(false);
	break;
	}

	/* once read in it is the same format as PNG */
	m_imagetype=GUISHAPE_PNG;
	m_bpp=4;
	Close();
	return(true);
}

bool kGUIImage::LoadPixels(void)
{
	if(!m_numframes)
	{
		if(m_numloaded==m_maxloaded)
		{
			kGUIImage *pobj;

			/* purge image at the bottom of the loaded list */
			pobj=m_loadedends.GetTail()->GetPrev();
			pobj->Purge();
		}
		LoadImage(false);
		if(!m_numframes)
			return(false);		/* must have had a load error */
	}
	return(true);
}

/* this is an unscaled tile draw, repeat the image over and over to cover the area */
bool kGUIImage::TileDraw(int frame,int x1,int y1,int x2,int y2)
{
	bool drawn=false;
	kGUICorners c;
	SUBPIXEL_DEF sub;

	assert(m_stepx==1.0f && m_stepy==1.0f,"This is an unscaled tiledraw only!");

	c.lx=x1;
	c.rx=x2;
	c.ty=y1;
	c.by=y2;
	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	if(kGUI::ValidClip())
	{
		if(LoadPixels())
		{
			/* move me to the top of the used list please */
			if(m_memimage==false && m_locked==false)
			{
				if(m_loadedends.GetHead()->GetNext()!=this)
				{
					/* first, unlink me from my spot in the loaded list */
					if(ValidLinks())
						Unlink();

					/* now add me to the top of the loaded list */
					Link(m_loadedends.GetHead());
				}
			}

			/* if this image is only 1x1 then use drawrect, it is much faster! */
			if(m_imagewidth==1 && m_imageheight==1)
			{
				sub.limage=m_imagedata.GetEntry(frame);
				sub.rowadd=m_bpp;

				sub.pixelwidth=1.0f;
				sub.pixelheight=1.0f;
				sub.pixelscale=1.0f;

				sub.xfrac=0.0f;
				sub.yfrac=0.0f;
				ReadSubPixel(m_imagetype,&sub);

				kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(sub.rgba[0],sub.rgba[1],sub.rgba[2]),1.0f);
			}
			else
			{
				int x,y;

				while((x1+(int)m_imagewidth)<=c.lx)
					x1+=m_imagewidth;

				while((x2-(int)m_imagewidth)>=c.rx)
					x2-=m_imagewidth;

				while((y1+(int)m_imageheight)<=c.ty)
					y1+=m_imageheight;

				while((y2-(int)m_imageheight)>=c.by)
					y2-=m_imageheight;

				/* i'm sure I can do this faster once I get around to optimizing it */
				for(y=y1;y<y2;y+=m_imageheight)
				{
					for(x=x1;x<x2;x+=m_imagewidth)
					{
						Draw(frame,x,y);
					}
				}
			}
			drawn=true;
		}
	}
	kGUI::PopClip();
	return(drawn);
}

bool kGUIImage::Draw(int frame,int x1,int y1)
{
	int x,y,x2,y2,r,g,b,dw,sw,a;
	int wbpp;
	kGUIColor color;
	const unsigned char *image;
	const unsigned char *limage;
	kGUIColor *sp;
	int skip;
	double yfrac,xfrac;
	SUBPIXEL_DEF sub;
	const kGUICorners *cc;	/* clip corners */
	bool fastdraw=kGUI::GetFastDraw();

	if(LoadPixels()==false)
		return(false);

	/* move me to the top of the list please */
	if(m_memimage==false && m_locked==false)
	{
		if(m_loadedends.GetHead()->GetNext()!=this)
		{
			/* first, unlink me from my spot in the loaded list */
			if(ValidLinks())
				Unlink();

			/* now add me to the top of the loaded list */
			Link(m_loadedends.GetHead());
		}
	}

	cc=kGUI::GetClipCorners();

	image=m_imagedata.GetEntry(frame);
	wbpp=m_bpp*m_imagewidth;

	xfrac=0;
	yfrac=0;

	if(m_stepx==1.0f)
		x2=x1+m_imagewidth;
	else
		x2=x1+(int)(m_imagewidth/m_stepx);

	if(m_stepy==1.0f)
		y2=y1+m_imageheight;
	else
		y2=y1+(int)(m_imageheight/m_stepy);

	if(kGUI::OffClip(x1,y1,x2,y2)==true)
		return(false);

	if(x1<cc->lx)
	{
		if(m_stepx==1.0f)
			image+=m_bpp*(cc->lx-x1);
		else
		{
			xfrac+=m_stepx*(cc->lx-x1);
			while(xfrac>=1.0f)
			{
				image+=m_bpp;
				xfrac-=1.0f;
			}
		}
		x1=cc->lx;
	}
	if(x2>cc->rx)
		x2=cc->rx;

	if(y1<cc->ty)
	{
		if(m_stepy==1.0f)
			image+=wbpp*(cc->ty-y1);
		else
		{
			yfrac+=m_stepy*(cc->ty-y1);
			while(yfrac>=1.0f)
			{
				image+=wbpp;
				yfrac-=1.0f;
			}
		}
		y1=cc->ty;
	}
	if(y2>cc->by)
		y2=cc->by;

	if((y2<=y1) || (x2<=x1))	/* nothing to draw! */
		return(false);

	/* todo: iff offsets then clip against edges of draw surface */

	sp=kGUI::GetSurfacePtr(x1,y1);
	sw=kGUI::GetSurfaceWidth();
	skip=sw-(x2-x1);
	dw=x2-x1;

	if((m_stepx==1.0f) && (m_stepy==1.0f))
	{
		switch(m_imagetype)
		{
		case GUISHAPE_SURFACE:
		{
			int dwbytes=dw*sizeof(kGUIColor);

			for(y=y1;y<y2;++y)
			{
				memcpy(sp,image,dwbytes);
				image+=wbpp;
				sp+=sw;
			}
		}
		break;
		case GUISHAPE_RAW:
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					color=DrawColor(image[0],image[1],image[2]);
					image+=3;
					if(color)
						*(sp)=color;
					++sp;
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		case GUISHAPE_JPG:
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					*(sp++)=DrawColor(image[0],image[1],image[2]);
					image+=3;
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		case GUISHAPE_GIF:
		case GUISHAPE_PNG:
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					r=image[0];
					g=image[1];
					b=image[2];
					a=image[3];
					image+=4;
					if(a==255)
						*(sp++)=DrawColor(r,g,b);
					else if(a>0)
					{
						int olda;
						int oldr,oldg,oldb;
						int newr,newg,newb;

						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		}
	}
	else	/* xstep or ystep is not 1.0f */
	{
		double startxfrac=xfrac;

		sub.rowadd=wbpp;
		sub.pixelwidth=m_stepx;
		sub.pixelheight=m_stepy;
		sub.pixelscale=1.0f/(m_stepx*m_stepy);

		switch(m_imagetype)
		{
		case GUISHAPE_RAW:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				for(x=x1;x<x2;++x)
				{
					color=DrawColor(image[0],image[1],image[2]);
					if(color)
						*(sp)=color;
					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
					++sp;
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_JPG:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_JPG>(&sub);
						*(sp++)=DrawColor(sub.rgba[0],sub.rgba[1],sub.rgba[2]);
					}
					else
						*(sp++)=DrawColor(image[0],image[1],image[2]);
					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_SURFACE:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_SURFACE>(&sub);
						*(sp++)=DrawColor(sub.rgba[0],sub.rgba[1],sub.rgba[2]);
					}
					else
						*(sp++)=*((kGUIColor *)image);

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_GIF:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_GIF>(&sub);
						r=sub.rgba[0];
						g=sub.rgba[1];
						b=sub.rgba[2];
						a=image[3];
					}
					else
					{
						r=image[0];
						g=image[1];
						b=image[2];
						a=image[3];
					}

					if(a==255)
						*(sp++)=DrawColor(r,g,b);
					else if(a>0)
					{
						int olda;
						int oldr,oldg,oldb;
						int newr,newg,newb;

						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_PNG:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_PNG>(&sub);
						r=sub.rgba[0];
						g=sub.rgba[1];
						b=sub.rgba[2];
						a=image[3];
					}
					else
					{
						r=image[0];
						g=image[1];
						b=image[2];
						a=image[3];
					}

					if(a==255)
						*(sp++)=DrawColor(r,g,b);
					else if(a>0)
					{
						int olda;
						int oldr,oldg,oldb;
						int newr,newg,newb;

						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		}
	}

	if(kGUI::GetPrintJob())
	{
		int offx,offy;

		offx=kGUI::GetCurrentSurface()->GetOffsetX();
		offy=kGUI::GetCurrentSurface()->GetOffsetY();

		kGUI::GetPrintJob()->DrawImage(x1+offx,x2+offx,y1+offy,y2+offy);
	}

	return(true);
}

/* this is exactly the same as the code above but uses alpha */

bool kGUIImage::DrawAlpha(int frame,int x1,int y1,double alpha)
{
	int x,y,x2,y2,r,g,b,dw,sw,a;
	int wbpp;
	kGUIColor color;
	const unsigned char *image;
	const unsigned char *limage;
	kGUIColor *sp;
	int skip;
	double yfrac,xfrac;
	SUBPIXEL_DEF sub;
	const kGUICorners *cc;	/* clip corners */
	bool fastdraw=kGUI::GetFastDraw();
	int olda;
	int oldr,oldg,oldb;
	int newr,newg,newb;

	if(LoadPixels()==false)
		return(false);

	/* move me to the top of the list please */
	if(m_memimage==false && m_locked==false)
	{
		if(m_loadedends.GetHead()->GetNext()!=this)
		{
			/* first, unlink me from my spot in the loaded list */
			if(ValidLinks())
				Unlink();

			/* now add me to the top of the loaded list */
			Link(m_loadedends.GetHead());
		}
	}

	cc=kGUI::GetClipCorners();

	image=m_imagedata.GetEntry(frame);
	wbpp=m_bpp*m_imagewidth;

	xfrac=0;
	yfrac=0;

	if(m_stepx==1.0f)
		x2=x1+m_imagewidth;
	else
		x2=x1+(int)(m_imagewidth/m_stepx);

	if(m_stepy==1.0f)
		y2=y1+m_imageheight;
	else
		y2=y1+(int)(m_imageheight/m_stepy);

	if(kGUI::OffClip(x1,y1,x2,y2)==true)
		return(false);

	if(x1<cc->lx)
	{
		if(m_stepx==1.0f)
			image+=m_bpp*(cc->lx-x1);
		else
		{
			xfrac+=m_stepx*(cc->lx-x1);
			while(xfrac>=1.0f)
			{
				image+=m_bpp;
				xfrac-=1.0f;
			}
		}
		x1=cc->lx;
	}
	if(x2>cc->rx)
		x2=cc->rx;

	if(y1<cc->ty)
	{
		if(m_stepy==1.0f)
			image+=wbpp*(cc->ty-y1);
		else
		{
			yfrac+=m_stepy*(cc->ty-y1);
			while(yfrac>=1.0f)
			{
				image+=wbpp;
				yfrac-=1.0f;
			}
		}
		y1=cc->ty;
	}
	if(y2>cc->by)
		y2=cc->by;

	if((y2<=y1) || (x2<=x1))	/* nothing to draw! */
		return(false);

	a=(int)(alpha*255);
	if(!a)
		return(false);
	olda=255-a;

	/* todo: iff offsets then clip against edges of draw surface */

	sp=kGUI::GetSurfacePtr(x1,y1);
	sw=kGUI::GetSurfaceWidth();
	skip=sw-(x2-x1);
	dw=x2-x1;

	if((m_stepx==1.0f) && (m_stepy==1.0f))
	{
		switch(m_imagetype)
		{
		case GUISHAPE_SURFACE:
		{
			int dwbytes=dw*sizeof(kGUIColor);

			/* todo, handle alphablend */
			for(y=y1;y<y2;++y)
			{
				memcpy(sp,image,dwbytes);
				image+=wbpp;
				sp+=sw;
			}
		}
		break;
		case GUISHAPE_RAW:
			a=(int)(alpha*255);
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					r=image[0];
					g=image[1];
					b=image[2];
					color=DrawColor(r,g,b);
					image+=3;

					if(color)
					{
						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		case GUISHAPE_JPG:
			a=(int)(alpha*255);
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					r=image[0];
					g=image[1];
					b=image[2];
					image+=3;
					
					DrawColorToRGB(*(sp),oldr,oldg,oldb);
					newr=((r*a)+(oldr*olda))>>8;
					newg=((g*a)+(oldg*olda))>>8;
					newb=((b*a)+(oldb*olda))>>8;
					*(sp++)=DrawColor(newr,newg,newb);
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		case GUISHAPE_GIF:
		case GUISHAPE_PNG:
			for(y=y1;y<y2;++y)
			{
				limage=image;
				for(x=x1;x<x2;++x)
				{
					r=image[0];
					g=image[1];
					b=image[2];
					a=(int)(image[3]*alpha);
					image+=4;
					if(a>0)
					{
						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;
				}
				sp+=skip;
				image=limage+wbpp;
			}
		break;
		}
	}
	else	/* xstep or ystep is not 1.0f */
	{
		double startxfrac=xfrac;

		sub.rowadd=wbpp;
		sub.pixelwidth=m_stepx;
		sub.pixelheight=m_stepy;
		sub.pixelscale=1.0f/(m_stepx*m_stepy);

		switch(m_imagetype)
		{
		case GUISHAPE_RAW:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				for(x=x1;x<x2;++x)
				{
					color=DrawColor(image[0],image[1],image[2]);
					if(color)
						*(sp)=color;
					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
					++sp;
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_JPG:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_JPG>(&sub);
						*(sp++)=DrawColor(sub.rgba[0],sub.rgba[1],sub.rgba[2]);
					}
					else
						*(sp++)=DrawColor(image[0],image[1],image[2]);
					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_SURFACE:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_SURFACE>(&sub);
						*(sp++)=DrawColor(sub.rgba[0],sub.rgba[1],sub.rgba[2]);
					}
					else
						*(sp++)=*((kGUIColor *)image);

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_GIF:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_GIF>(&sub);
						r=sub.rgba[0];
						g=sub.rgba[1];
						b=sub.rgba[2];
						a=image[3];
					}
					else
					{
						r=image[0];
						g=image[1];
						b=image[2];
						a=image[3];
					}

					if(a==255)
						*(sp++)=DrawColor(r,g,b);
					else if(a>0)
					{
						int olda;
						int oldr,oldg,oldb;
						int newr,newg,newb;

						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		case GUISHAPE_PNG:
			for(y=y1;y<y2;++y)
			{
				limage=image;		/* save pointer to beginning of line */
				xfrac=startxfrac;	/* set xfrac to starting xfrac each time */

				sub.yfrac=yfrac;
				for(x=x1;x<x2;++x)
				{
					if(fastdraw==false)
					{
						sub.limage=image;
						sub.xfrac=xfrac;
						ReadSubPixel<GUISHAPE_PNG>(&sub);
						r=sub.rgba[0];
						g=sub.rgba[1];
						b=sub.rgba[2];
						a=image[3];
					}
					else
					{
						r=image[0];
						g=image[1];
						b=image[2];
						a=image[3];
					}

					if(a==255)
						*(sp++)=DrawColor(r,g,b);
					else if(a>0)
					{
						int olda;
						int oldr,oldg,oldb;
						int newr,newg,newb;

						DrawColorToRGB(*(sp),oldr,oldg,oldb);
						olda=255-a;
						newr=((r*a)+(oldr*olda))>>8;
						newg=((g*a)+(oldg*olda))>>8;
						newb=((b*a)+(oldb*olda))>>8;
						*(sp++)=DrawColor(newr,newg,newb);
					}
					else
						++sp;

					xfrac+=m_stepx;
					while(xfrac>=1.0f)
					{
						image+=m_bpp;
						xfrac-=1.0f;
					}
				}
				sp+=skip;

				yfrac+=m_stepy;
				while(yfrac>=1.0f)
				{
					limage+=wbpp;
					yfrac-=1.0f;
				}

				image=limage;
			}
		break;
		}
	}

	if(kGUI::GetPrintJob())
	{
		int offx,offy;

		offx=kGUI::GetCurrentSurface()->GetOffsetX();
		offy=kGUI::GetCurrentSurface()->GetOffsetY();

		kGUI::GetPrintJob()->DrawImage(x1+offx,x2+offx,y1+offy,y2+offy);
	}

	return(true);
}

void kGUIImage::DrawLineRect(int frame,int lx,int ty,int rx,int by,bool horiz)
{
	int x,y;
	int r,g,b;
	int numpixels;
	double step;
	SUBPIXEL_DEF sub;

	if(kGUI::OffClip(lx,ty,rx,by)==true)
		return;

	if(LoadPixels()==false)
		return;

	/* move me to the top of the list please */
	if(m_memimage==false)
	{
		if(m_loadedends.GetHead()->GetNext()!=this)
		{
			/* first, unlink me from my spot in the loaded list */
			if(ValidLinks())
				Unlink();

			/* now add me to the top of the loaded list */
			Link(m_loadedends.GetHead());
		}
	}

	sub.limage=m_imagedata.GetEntry(frame);
	sub.rowadd=m_bpp;

	numpixels=m_imagewidth*m_imageheight;	/* number of source pixels */

	/* do horizontal lines or vertical ones */
	if(horiz==true)
	{
		step=(double)numpixels/(double)(by-ty);
		sub.pixelwidth=1.0f;
		sub.pixelheight=step;
		sub.pixelscale=1.0f/(step);

		sub.xfrac=0.0f;
		sub.yfrac=0.0f;
		for(y=ty;y<by;++y)
		{
			ReadSubPixel(m_imagetype,&sub);
			r=sub.rgba[0];
			g=sub.rgba[1];
			b=sub.rgba[2];
			kGUI::DrawRect(lx,y,rx,y+1,DrawColor(r,g,b));
			sub.yfrac+=step;
			while(sub.yfrac>=1.0f)
			{
				sub.limage+=m_bpp;
				sub.yfrac-=1.0f;
			}
		}
	}
	else
	{
		step=(double)numpixels/(double)(rx-lx);
		sub.pixelwidth=step;
		sub.pixelheight=1.0f;
		sub.pixelscale=1.0f/(step);

		sub.xfrac=0.0f;
		sub.yfrac=0.0f;
		for(x=lx;x<rx;++x)
		{
			ReadSubPixel(m_imagetype,&sub);
			r=sub.rgba[0];
			g=sub.rgba[1];
			b=sub.rgba[2];
			kGUI::DrawRect(x,ty,x+1,by,DrawColor(r,g,b));
			sub.xfrac+=step;
			while(sub.xfrac>=1.0f)
			{
				sub.limage+=m_bpp;
				sub.xfrac-=1.0f;
			}
		}
	}
}

void kGUIImage::CopyImage(kGUIImage *image)
{
	int pixdatasize;
	unsigned char *copydata;

	if(image->LoadPixels()==false)
		return;

	/* free any previous image data if applicable */
	Purge();

	m_imagetype=image->m_imagetype;
	m_imagewidth=image->m_imagewidth;
	m_imageheight=image->m_imageheight;
	m_bpp=image->m_bpp;
	m_memimage=true;
	m_allocmemimage=true;
	/* allocate space for R,G,B x pixels */
	pixdatasize=m_imagewidth*m_imageheight*m_bpp;
	copydata=new unsigned char[pixdatasize];
	assert(copydata!=0,"Unable to allocate memory for image!");
	m_numframes=1;
	m_imagedata.SetEntry(0,copydata);
	memcpy(copydata,image->m_imagedata.GetEntry(0),pixdatasize);
}

/* 172,168,153 */
void kGUIImage::GreyImage(int frame)
{
	unsigned int x,y;
	unsigned char r=0,g=0,b=0;
	double h,s,v;
	unsigned char *imagedata;

	imagedata=m_imagedata.GetEntry(frame);
	for(y=0;y<m_imageheight;++y)
	{
		for(x=0;x<m_imagewidth;++x)
		{
			switch(m_imagetype)
			{
			case GUISHAPE_GIF:
			case GUISHAPE_PNG:
			case GUISHAPE_RAW:
			case GUISHAPE_JPG:
				r=imagedata[0];
				g=imagedata[1];
				b=imagedata[2];
			break;
			case GUISHAPE_SURFACE:
				x=*((kGUIColor *)imagedata);
				DrawColorToRGB(x,r,g,b);
			break;
			}
			kGUI::RGBToHSV(r,g,b,&h,&s,&v);
			if(v<(128.0f/256.0f))
			{
				r=172;
				g=168;
				b=153;
			}
			else
			{
				r=g=b=(int)(v*255.0f);
			}

//			if(v<(128.0f/256.0f))
			{
//				v=168.0f/256.0f;
//				kGUI::HSVToRGB(h,s,v,&r,&g,&b);
//				r=172;
//				g=168;
//				b=153;
				switch(m_imagetype)
				{
				case GUISHAPE_GIF:
				case GUISHAPE_PNG:
				case GUISHAPE_RAW:
				case GUISHAPE_JPG:
					imagedata[0]=r;
					imagedata[1]=g;
					imagedata[2]=b;
				break;
				case GUISHAPE_SURFACE:
					*((kGUIColor *)imagedata)=DrawColor(r,g,b);
				break;
				}
			}
			imagedata+=m_bpp;
		}
	}
}

void kGUIImage::ReadPixel(const unsigned char *ptr,unsigned char *r,unsigned char *g,unsigned char *b,unsigned char *a)
{
	switch(m_imagetype)
	{
	case GUISHAPE_RAW:
	case GUISHAPE_JPG:
		r[0]=ptr[0];
		g[0]=ptr[1];
		b[0]=ptr[2];
		a[0]=255;
	case GUISHAPE_GIF:
	case GUISHAPE_PNG:
		r[0]=ptr[0];
		g[0]=ptr[1];
		b[0]=ptr[2];
		a[0]=ptr[3];
	break;
	case GUISHAPE_SURFACE:
	{
		kGUIColor x;
		int xr,xg,xb;

		x=*((kGUIColor *)ptr);
		DrawColorToRGB(x,xr,xg,xb);
		r[0]=(unsigned char)xr;
		g[0]=(unsigned char)xg;
		b[0]=(unsigned char)xb;
		a[0]=255;
	}
	break;
	}
}

/*********************** imageobj ****************************/

kGUIImageObj::kGUIImageObj()
{
	m_hint=0;
	m_showscrollbars=false;
	m_leftoff=0;
	m_topoff=0;
	m_vscrollbar=0;
	m_hscrollbar=0;
	
	m_alpha=1.0f;
	m_currentframe=0;
	m_animate=false;
	m_animateeventactive=false;
}

kGUIImageObj::~kGUIImageObj()
{
	if(m_animateeventactive)
	{
		m_animateeventactive=false;
		kGUI::DelEvent(this,CALLBACKNAME(Animate));
	}

	/* these are allocated in pairs so we only need to check one for null */
	if(m_vscrollbar)
	{
		delete m_vscrollbar;
		delete m_hscrollbar;
	}

	if(m_hint)
		delete m_hint;
}

void kGUIImageObj::SetShowScrollBars(bool s)
{
	m_leftoff=0;
	m_topoff=0;
	m_showscrollbars=s;
	if(s)
	{
		/* these are allocated in pairs so we only need to check one for null */
		if(!m_vscrollbar)
		{
			m_vscrollbar=new kGUIScrollBarObj();
			m_vscrollbar->SetVert();
			m_vscrollbar->SetEventHandler(this,& CALLBACKNAME(ScrollMoveRow));

			m_hscrollbar=new kGUIScrollBarObj();
			m_hscrollbar->SetHorz();
			m_hscrollbar->SetEventHandler(this,& CALLBACKNAME(ScrollMoveCol));
		}
	}
	UpdateScrollbars();
}

void kGUIImageObj::Animate(void)
{
	kGUICorners sc;
	kGUICorners c;

	/* if no longer on screen then disable the event */
	/* or if the image no longer has more than 1 frame */
	sc.lx=0;
	sc.rx=kGUI::GetScreenWidth();
	sc.ty=0;
	sc.by=kGUI::GetScreenHeight();
	GetCorners(&c);

	if( (kGUI::Overlap(&sc,&c)==false) || (GetNumFrames()<1) || m_animate==false)
	{
		m_animateeventactive=false;
		kGUI::DelEvent(this,CALLBACKNAME(Animate));
		return;
	}

	m_animdelay+=kGUI::GetET();
	if(m_animdelay>=GetDelay(m_currentframe))
	{
		m_animdelay=0;
		if(++m_currentframe>=GetNumFrames())
			m_currentframe=0;
		Dirty();
	}
}

double kGUIImageObj::CalcScaleToFit(int w,int h)
{
	float ws,hs,s;
	float imagew=(float)w;
	float imageh=(float)h;
	float objh=(float)(GetZoneH());
	float objw=(float)(GetZoneW());

	ws=objw/imagew;
	hs=objh/imageh;
	if(ws>hs)
		s=hs;
	else
		s=ws;
	return(s);
}

void kGUIImageObj::ShrinkToFit(void)
{
	double s=min(CalcScaleToFit(GetImageWidth(),GetImageHeight()),1.0f);
	SetScale(s,s);
}

void kGUIImageObj::ExpandToFit(void)
{
	double s=max(CalcScaleToFit(GetImageWidth(),GetImageHeight()),1.0f);
	SetScale(s,s);
}

/* expand or shrink attached image to fit in the imageobj area */
void kGUIImageObj::ScaleToFit(void)
{
	double s=CalcScaleToFit(GetImageWidth(),GetImageHeight());
	SetScale(s,s);
}

void kGUIImageObj::CenterImage(void)
{
	int objh=GetZoneH();
	int objw=GetZoneW();
	int imageh=(int)GetScaledImageHeight();
	int imagew=(int)GetScaledImageWidth();

	if(imagew>objw)
		m_leftoff=0;
	else
		m_leftoff=-((objw-imagew)>>1);
	if(imageh>objh)
		m_topoff=0;
	else
		m_topoff=-((objh-imageh)>>1);
}

void kGUIImageObj::MoveRow(int move)
{
	int viewh=GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight();
	int maxh=(int)(max(0,GetScaledImageHeight()-viewh));

	m_topoff+=move;
	if(m_topoff<0)
		m_topoff=0;
	else if(m_topoff>maxh)
		m_topoff=maxh;
	UpdateScrollbars();
}

void kGUIImageObj::MoveCol(int move)
{
	int vieww=GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth();
	int maxw=(int)(max(0,GetScaledImageWidth()-vieww));

	m_leftoff+=move;
	if(m_leftoff<0)
		m_leftoff=0;
	else if(m_leftoff>maxw)
		m_leftoff=maxw;
	UpdateScrollbars();
}

void kGUIImageObj::UpdateScrollbars(void)
{
	int vieww=GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth();
	int viewh=GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight();
	int imagew=(int)GetScaledImageWidth();
	int imageh=(int)GetScaledImageHeight();

	Dirty();

	if(imagew<vieww)
		m_leftoff=0;
	else if(m_leftoff>(imagew-vieww))
		m_leftoff=imagew-vieww;		/* if the scale changed then these can be too large */

	if(imageh<viewh)
		m_leftoff=0;
	else if(m_topoff>(imageh-viewh))
		m_topoff=imageh-viewh;		/* if the scale changed then these can be too large */

	if(m_showscrollbars==false)
		return;

	m_hscrollbar->SetValues(m_leftoff,vieww,imagew-vieww-m_leftoff);
	m_vscrollbar->SetValues(m_topoff,viewh,imageh-viewh-m_topoff);
}

bool kGUIImageObj::UpdateInput(void)
{
	int move;
	bool over;
	kGUICorners c;
	int ilx,irx,ity,iby;

	GetCorners(&c);
	if(kGUI::WantHint()==true && m_hint)
		kGUI::SetHintString(kGUI::GetMouseX(),kGUI::GetMouseY()-15,m_hint->GetString());

	/* is image inside in a smaller area? if so then we only control that smaller area let other items exist in our edges */
	ilx=c.lx-m_leftoff;
	ity=c.ty-m_topoff;
	irx=ilx+(int)GetScaledImageWidth();
	iby=ity+(int)GetScaledImageHeight();

	if(ilx>c.lx)
		c.lx=ilx;
	if(irx<c.rx)
		c.rx=irx;
	if(ity>c.ty)
		c.ty=ity;
	if(iby<c.by)
		c.by=iby;

//	if(kGUIImage::Draw(m_currentframe,c.lx-m_leftoff,c.ty-m_topoff)==false)
//			kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));

	over=kGUI::MouseOver(&c);
	if(!over)
	{
		if(m_showscrollbars==true)
		{
			m_hscrollbar->GetCorners(&c);
			over=kGUI::MouseOver(&c);
			if(!over)
			{
				m_vscrollbar->GetCorners(&c);
				over=kGUI::MouseOver(&c);
			}
		}
	}

	if((over==false && kGUI::GetMouseClick()==true) || kGUI::GetKey())
	{
		if(this==kGUI::GetActiveObj())
			kGUI::PopActiveObj();
		return(false);
	}

	if(this!=kGUI::GetActiveObj() && kGUI::GetMouseClick()==true)
		kGUI::PushActiveObj(this);

	if(this==kGUI::GetActiveObj())
	{
		if(m_showscrollbars==true)
		{
			if(m_hscrollbar->IsActive()==true)
				return(m_hscrollbar->UpdateInput());

			if(m_vscrollbar->IsActive()==true)
				return(m_vscrollbar->UpdateInput());

			m_hscrollbar->GetCorners(&c);
			if(kGUI::MouseOver(&c))
				return(m_hscrollbar->UpdateInput());

			m_vscrollbar->GetCorners(&c);
			if(kGUI::MouseOver(&c))
				return(m_vscrollbar->UpdateInput());

			move=kGUI::GetMouseWheelDelta();
			kGUI::ClearMouseWheelDelta();
			if(move)
			{
				MoveRow(-move<<4);
				return(true);
			}
			if(kGUI::GetMouseLeft()==true)
			{
				if(kGUI::GetMouseDX() || kGUI::GetMouseDY())
				{
					MoveCol(-kGUI::GetMouseDX());
					MoveRow(-kGUI::GetMouseDY());
					return(true);
				}
			}
		}
		if(kGUI::GetMouseDoubleClickLeft())
		{
			CallEvent(EVENT_LEFTDOUBLECLICK);
			return(true);
		}
		else if(kGUI::GetMouseClickLeft())
		{
			CallEvent(EVENT_LEFTCLICK);
			return(true);
		}
		else if(kGUI::GetMouseClickRight())
		{
			CallEvent(EVENT_RIGHTCLICK);
			return(true);
		}
	}

	return(false);
}

kGUIImageRefObj::~kGUIImageRefObj()
{
	if(m_animateeventactive)
		kGUI::DelEvent(this,CALLBACKNAME(Animate));
}

bool kGUIImageRefObj::UpdateInput(void)
{
	if(kGUI::GetMouseDoubleClickLeft())
		CallEvent(EVENT_LEFTDOUBLECLICK);
	else if(kGUI::GetMouseClickLeft())
		CallEvent(EVENT_LEFTCLICK);

	if(kGUI::GetMouseDoubleClickRight())
		CallEvent(EVENT_RIGHTDOUBLECLICK);
	else if(kGUI::GetMouseClickRight())
		CallEvent(EVENT_RIGHTCLICK);
	return(true);
}

void kGUIImageRefObj::Animate(void)
{
	kGUICorners sc;
	kGUICorners c;

	/* if no longer on screen then disable the event */
	/* or if the image no longer has more than 1 frame */
	sc.lx=0;
	sc.rx=kGUI::GetScreenWidth();
	sc.ty=0;
	sc.by=kGUI::GetScreenHeight();
	GetCorners(&c);

	if( (kGUI::Overlap(&sc,&c)==false) || (GetNumFrames()<2))
	{
		m_animateeventactive=false;
		kGUI::DelEvent(this,CALLBACKNAME(Animate));
		return;
	}

	m_animdelay+=kGUI::GetET();
	if(m_animdelay>=m_image->GetDelay(m_currentframe))
	{
		m_animdelay=0;
		if(++m_currentframe>=GetNumFrames())
			m_currentframe=0;
		Dirty();
	}
}

void kGUIImageRefObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip() && m_image)
	{
		if(m_image->IsValid())
		{
			if(m_currentframe>GetNumFrames())
				m_currentframe=0;

			/* add animate event? */
			if(GetNumFrames()>1 && m_animate==true && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				kGUI::AddEvent(this,CALLBACKNAME(Animate));
			}

			m_image->SetScale(GetScaleX(),GetScaleY());
			if(m_alpha==1.0f)
			{
				if(m_image->Draw(m_currentframe,c.lx,c.ty)==false)
					kGUI::DrawRect(c.lx+m_leftoff,c.ty+m_topoff,c.rx,c.by,DrawColor(255,255,255));
			}
			else
			{
				m_image->DrawAlpha(m_currentframe,c.lx+m_leftoff,c.ty+m_topoff,m_alpha);
			}
		}
	}
	kGUI::PopClip();
}

void kGUIImageRefObj::TileDraw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip() && m_image)
	{
		if(m_image->IsValid())
		{
			if(m_currentframe>GetNumFrames())
				m_currentframe=0;

			/* add animate event? */
			if(GetNumFrames()>1 && m_animate==true && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				kGUI::AddEvent(this,CALLBACKNAME(Animate));
			}

			m_image->SetScale(GetScaleX(),GetScaleY());
			if(m_image->TileDraw(m_currentframe,c.lx,c.ty,c.rx,c.by)==false)
				kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
		}
	}
	kGUI::PopClip();
}

void kGUIImageObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(m_showscrollbars==true)
		{
			kGUIZone sz;

			sz.SetZone(c.rx-kGUI::GetSkin()->GetScrollbarWidth(),c.ty,kGUI::GetSkin()->GetScrollbarWidth(),c.by-c.ty);
			m_vscrollbar->MoveZone(&sz);
			m_vscrollbar->Draw();

			sz.SetZone(c.lx,c.by-kGUI::GetSkin()->GetScrollbarHeight(),(c.rx-c.lx)-kGUI::GetSkin()->GetScrollbarWidth(),kGUI::GetSkin()->GetScrollbarHeight());
			m_hscrollbar->MoveZone(&sz);
			m_hscrollbar->Draw();
			c.rx-=kGUI::GetSkin()->GetScrollbarWidth();
			c.by-=kGUI::GetSkin()->GetScrollbarHeight();
			kGUI::ShrinkClip(&c);
		}

		if(IsValid())	/* is this a valid image (not corrupt?) */
		{
			if(m_currentframe>GetNumFrames())
				m_currentframe=0;

			/* add animate event? */
			if(GetNumFrames()>1 && m_animate==true && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				kGUI::AddEvent(this,CALLBACKNAME(Animate));
			}

			if(m_alpha==1.0f)
			{
				if(kGUIImage::Draw(m_currentframe,c.lx-m_leftoff,c.ty-m_topoff)==false)
					kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
			}
			else
			{
				if(kGUIImage::DrawAlpha(m_currentframe,c.lx-m_leftoff,c.ty-m_topoff,m_alpha)==false)
					kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
			}
		}
	}
	kGUI::PopClip();
}

void kGUIImageObj::TileDraw(void)
{
	kGUICorners c;

	GetCorners(&c);

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(m_showscrollbars==true)
		{
			kGUIZone sz;

			sz.SetZone(c.rx-kGUI::GetSkin()->GetScrollbarWidth(),c.ty,kGUI::GetSkin()->GetScrollbarWidth(),c.by-c.ty);
			m_vscrollbar->MoveZone(&sz);
			m_vscrollbar->Draw();

			sz.SetZone(c.lx,c.by-kGUI::GetSkin()->GetScrollbarHeight(),(c.rx-c.lx)-kGUI::GetSkin()->GetScrollbarWidth(),kGUI::GetSkin()->GetScrollbarHeight());
			m_hscrollbar->MoveZone(&sz);
			m_hscrollbar->Draw();
			c.rx-=kGUI::GetSkin()->GetScrollbarWidth();
			c.by-=kGUI::GetSkin()->GetScrollbarHeight();
			kGUI::ShrinkClip(&c);
		}

		if(IsValid())	/* is this a valid image (not corrupt?) */
		{
			if(m_currentframe>GetNumFrames())
				m_currentframe=0;

			/* add animate event? */
			if(GetNumFrames()>1 && m_animate==true && m_animateeventactive==false)
			{
				m_animateeventactive=true;
				kGUI::AddEvent(this,CALLBACKNAME(Animate));
			}

			if(kGUIImage::TileDraw(m_currentframe,c.lx-m_leftoff,c.ty-m_topoff,c.rx-m_leftoff,c.by-m_topoff)==false)
				kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
		}
	}
	kGUI::PopClip();
}

