/**********************************************************************************/
/* kGUI - kguifont.cpp                                                            */
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

/*! @file kguifont.cpp 
    @brief This is the font rendering code, it uses the FreeType library.                 
 It has a build-in caching system so commonly used fonts/size/char cominations  
 are not generated over and over but refer to the cached bitmaps */

//hmmm, this probably should not be set for LINUX or MACINTOSH modes
#define _WIN32_WINNT 0x0500

#include "kgui.h"
#include "kguifont.h"

kGUIFace::~kGUIFace()
{
	int size;
	int c;
	int *cchar;

	for(size=1;size<MAXQUICKSIZE;++size)
	{
		for(c=0;c<MAXCCACHE;++c)
		{
			cchar=m_quickcache[size][c];
			if(cchar)
				delete []cchar;
		}
	}
}


kGUIFont kguifont;

FT_Library kGUIFont::m_library;
unsigned int kGUIFont::m_numfonts;

#if defined(WIN32) || defined(MINGW)
DWORD kGUIFont::m_numreg;
#elif defined(LINUX) || defined(MACINTOSH)
#else
#error
#endif

kGUIFace kGUIFont::m_faces[MAXFONTS]; /* the font faces */
FT_Error kGUIFont::m_error;          /* error returned by FreeType? */
kGUIString kGUI::m_ftversion;	/* freetype version */
kGUIFace *kGUIFont::m_lastface=0;
unsigned int kGUIFont::m_lastsize=0;
ClassArray<kGUIFontFileInfo>kGUIFont::m_ffilist;
unsigned int kGUIFont::m_ffinumentries=0;

/* true = ok, false = error */
bool kGUI::InitFontEngine(void)
{
	int a,b,c;
	kGUIFont::m_numfonts=0;
#if defined(WIN32) || defined(MINGW)
	kGUIFont::m_numreg=0;
#elif defined(LINUX) || defined(MACINTOSH)
#else
#error
#endif
	kGUIFont::m_error = FT_Init_FreeType( &kGUIFont::m_library );

	FT_Library_Version( kGUIFont::m_library,&a,&b,&c);
	m_ftversion.Sprintf("%d.%d.%d",a,b,c);

	kGUIFont::m_ffilist.Init(32,16);
	kGUIFont::m_ffinumentries=0;

	if(kGUIFont::m_error)
		return(false);
	return(true);
}

/* scan dir and make an xref table between filenames and fonr name/types */

void kGUI::AddFontDir(const char *path)
{
	unsigned int i,n;
	kGUIDir dir;
	unsigned long fontfilesize;
	unsigned char *mem;
	kGUIFontFileInfo *ffi;
	FT_Face ftface;

	dir.LoadDir(path,false,true,"ttf");
	n=dir.GetNumFiles();
	for(i=0;i<n;++i)
	{
		mem=kGUI::LoadFile(dir.GetFilename(i),&fontfilesize);
		if(mem)
		{
			if(FT_New_Memory_Face( kGUIFont::GetLibrary(),
									mem,			/* first byte in memory */
									fontfilesize,	/* size in bytes */
									0,				/* face_index */
									&ftface )==0)
			{
				/* make a name for this font */
				ffi=kGUIFont::m_ffilist.GetEntryPtr(kGUIFont::m_ffinumentries++);
				
				ffi->SetFilename(dir.GetFilename(i));
				ffi->SetFacename(ftface->family_name);
				ffi->SetStyle(ftface->style_name);
				kGUI::Trace("Font fn='%s',face='%s',style='%s'\n",ffi->GetFilename()->GetString(),ffi->GetFacename()->GetString(),ffi->GetStyle()->GetString());
			}
			delete []mem;
		}
	}
}

unsigned int kGUI::GetNumFonts(void)
{
	return kGUIFont::m_numfonts;
}

void kGUI::GetFontName(unsigned int index,kGUIString *s)
{
	s->SetString(kGUIFont::m_faces[index].GetName());
}

void kGUI::CloseFontEngine(void)
{
	unsigned int i;

	for(i=0;i<kGUIFont::m_numfonts;++i)
		kGUIFont::GetFace(i)->Purge();
	FT_Done_FreeType(kGUIFont::m_library);
}

int kGUI::LoadFont(const char *filename,bool bold)
{
	kGUIFace *fslot;

	fslot=kGUIFont::m_faces+kGUIFont::m_numfonts;
	if(fslot->LoadFont(filename,bold)<0)
		return(-1);

	/* success */
	kGUIFont::m_numfonts++;
	return(kGUIFont::m_numfonts-1);
}

void kGUI::SelectFont(kGUIFace *face,unsigned int size)
{
	if(face==kGUIFont::m_lastface && size==kGUIFont::m_lastsize)
		return;

	kGUIFont::m_lastface=face;
	kGUIFont::m_lastsize=size;

	FT_Set_Char_Size( face->m_ftface, /* handle to face object */
		0, /* char_width in 1/64th of points */
		size*64, /* char_height in 1/64th of points */
		0, /* horizontal device resolution */  
		0 ); /* vertical device resolution */

	/* todo: add assert for error code returned */
}

void kGUIFace::Purge(void)
{
	FT_Done_Face( m_ftface);
	if(m_memfile)
		delete []m_memfile;
}

void kGUIFace::CalcHeight(unsigned int size)
{
	unsigned int c;
	static char largechars[]={"QWpqjy"};
	int glyph_index,above,below,maxabove,maxbelow;

	assert(size<=MAXFONTSIZE,"Size to large!");

	/* -1,-1 = not calculated yet */
	if(m_pixabove[size]!=-1 || m_pixbelow[size]!=-1)
		return;

	maxabove=0;
	maxbelow=0;
	kGUI::SelectFont(this,size);

	for(c=0;c<sizeof(largechars);++c)
	{
		glyph_index = FT_Get_Char_Index( m_ftface, largechars[c] );
		if(glyph_index>0)
		{
			if(FT_Load_Glyph(m_ftface, glyph_index, FT_LOAD_DEFAULT)==0)
			{
				if(FT_Render_Glyph( m_ftface->glyph, ft_render_mode_normal )==0)
				{
					above=m_ftface->glyph->bitmap_top;
					below=m_ftface->glyph->bitmap.rows-above;

					if(above>maxabove)
						maxabove=above;
					if(below>maxbelow)
						maxbelow=below;
				}
			}
		}
	}
	m_pixabove[size]=maxabove;
	m_pixbelow[size]=maxbelow;
}

int kGUIFace::LoadFont(const char *filename,bool bold)
{
	int size,glyph_index;
	int advance;
	unsigned int c;
	unsigned long fontfilesize;

	m_haskerning=false;
	m_bold=bold;
	/* handle bigfile based fonts */
	m_memfile=kGUI::LoadFile(filename,&fontfilesize);
	assert(m_memfile!=0,"Couldn't find font!");

#if defined(WIN32) || defined(MINGW)
	//windows only, this is so that when printing, reports can use these fonts too!
	AddFontMemResourceEx(m_memfile, fontfilesize,0,&kGUIFont::m_numreg);
#elif defined(LINUX) || defined(MACINTOSH)
#else
#error
#endif

	if(FT_New_Memory_Face( kGUIFont::GetLibrary(),
							m_memfile,	/* first byte in memory */
							fontfilesize,	/* size in bytes */
							0,				/* face_index */
							GetFacePtr() ))
		return(-1);

	/* make a name for this font */
	if(!strcmp(m_ftface->style_name, "Regular"))
		m_name.Sprintf("%s", m_ftface->family_name);
    else
        m_name.Sprintf("%s %s",m_ftface->family_name, m_ftface->style_name);

	/* calculate pixel heights for different sizes of this font */
	for(size=1;size<=MAXFONTSIZE;++size)
	{
		kGUI::SelectFont(this,size);
		m_pixabove[size]=-1;
		m_pixbelow[size]=-1;

		/* pre-calculate character widths */
		if(size<=MAXQUICKSIZE)
		{
			for(c=0;c<MAXCCACHE;++c)
			{
				advance=0;
				glyph_index = FT_Get_Char_Index( m_ftface, c );
				if(glyph_index>0)
				{
					if(FT_Load_Glyph(m_ftface, glyph_index, FT_LOAD_DEFAULT)==0)
					{
						if(bold)
						{
							if(!FT_Render_Glyph( m_ftface->glyph, ft_render_mode_normal ))
								FT_GlyphSlot_Embolden(m_ftface->glyph);
						}
						advance=m_ftface->glyph->advance.x >> 6;
					}
				}
				m_quickwidths[size][c]=advance;
			}
		}
	}
	m_haskerning = FT_HAS_KERNING( m_ftface )!=0; 

	return(0);	/* ok */
}

/* called only if size was not available from the cache */
int kGUIFace::GetCharWidth(unsigned int ch)
{
	int cwidth=0;
	int glyph_index;

	glyph_index = FT_Get_Char_Index( m_ftface, ch );
	if(glyph_index>0)
	{
		if(!FT_Load_Glyph(m_ftface, glyph_index, FT_LOAD_DEFAULT))
			cwidth=m_ftface->glyph->advance.x >> 6;
	}
	return(cwidth);
}

/* tell me the number of characters that can fit in the width supplied */

const unsigned int kGUIText::CalcFitWidth(unsigned int sstart,unsigned int slen,const unsigned int maxwidth,unsigned int *pixwidth)
{
	int n;
	unsigned int size;
	unsigned int width,cwidth;
	kGUIFace *face=0;
	unsigned int *qw=0;
	unsigned int ch;	/* current character */
	unsigned int nb;	/* number of bytes for current character */

	n=0;
	width=0;
	if(slen)
	{
		if(GetUseRichInfo()==false)
		{
			face=kGUIFont::GetFace(GetFontID());
			size=GetFontSize();
			qw=face->GetQuickWidths(size);
			kGUI::SelectFont(face,size);

			while(slen>0)
			{
				ch=GetChar(sstart,&nb);
				assert(ch!=0,"reading past end of string!");
				sstart+=nb;
				slen-=nb;

				if(ch==10)
					break;
				if(ch=='\t')
				{
					cwidth=GetTabWidth(width);
					if(!cwidth)
					{
						if(qw && ' '<MAXCCACHE)
							cwidth=qw[' '];
						else
							cwidth=face->GetCharWidth(' ');
					}
				}
				else
				{
					if(qw && ch<MAXCCACHE)
						cwidth=qw[ch];
					else
						cwidth=face->GetCharWidth(ch);
				}
				if((width+cwidth)>maxwidth)
					break;
				n+=nb;
				width+=cwidth;
			}
		}
		else
		{
			RICHINFO_DEF *ri;
			unsigned int lastfontid=0;

			size=0;
			while(slen>0)
			{
				ri=GetRichInfoPtr(sstart);
				if((ri->fontid!=lastfontid) || (ri->fontsize!=size))
				{
					size=ri->fontsize;
					lastfontid=ri->fontid;
					face=kGUIFont::GetFace(lastfontid);
					kGUI::SelectFont(face,size);
					qw=face->GetQuickWidths(size);
				}

				ch=GetChar(sstart,&nb);
				assert(ch!=0,"reading past end of string!");
				sstart+=nb;
				slen-=nb;

				if(ch==10)
					break;
				if(ch=='\t')
				{
					cwidth=GetTabWidth(width);
					if(!cwidth)
					{
						if(qw && ' '<MAXCCACHE)
							cwidth=qw[' '];
						else
							cwidth=face->GetCharWidth(' ');
					}
				}
				else
				{
					if(qw && ch<MAXCCACHE)
						cwidth=qw[ch];
					else
						cwidth=face->GetCharWidth(ch);
				}
				if((width+cwidth)>maxwidth)
					break;
				n+=nb;
				width+=cwidth;
			}
		}
	}
	if(pixwidth)
		pixwidth[0]=width;
	return(n);
}

/* calc the pixel width and maximum pixel height for a given section */
void kGUIText::GetSubSize(int sstart,int slen,unsigned int *pixwidth,unsigned int *pixheight)
{
	unsigned int width;
	unsigned int height;
	unsigned int maxheight;
	kGUIFace *face=0;
	unsigned int size;
	unsigned int *qw=0;
	unsigned int ch;	/* current character */
	unsigned int nb;	/* number of bytes for current character */

	/* optimize: two loops one for rich and one for plain */
	width=0;
	if(GetUseRichInfo()==false)
	{
		/* plain mode, single font and size for whole string */
		maxheight=GetLineHeight();
		if(slen)
		{
			face=kGUIFont::GetFace(GetFontID());
			size=GetFontSize();
			kGUI::SelectFont(face,size);
			qw=face->GetQuickWidths(size);

			width=0;
			while(slen>0)
			{
				ch=GetChar(sstart,&nb);
				assert(ch!=0,"reading past end of string!");
				sstart+=nb;
				slen-=nb;

				if(ch=='\t')
				{
					int tw;

					tw=GetTabWidth(width);
					if(!tw)
					{
						/* use space width if no defined tabs */
						if(qw && ' '<MAXCCACHE)
							width+=qw[' '];
						else
							width+=face->GetCharWidth(' ');
					}
					width+=tw;
				}
				else
				{
					if(qw && ch<MAXCCACHE)
						width+=qw[ch];
					else
						width+=face->GetCharWidth(ch);
					width+=m_letterspacing;
				}
			}
		}
	}
	else
	{
		/* rich string, can have diff font and size on each character */
		maxheight=0;
		if(slen)
		{
			RICHINFO_DEF *ri;
			unsigned int lastfontid=0;

			size=0;
			while(slen>0)
			{
				ri=GetRichInfoPtr(sstart);
				if((ri->fontid!=lastfontid) || (ri->fontsize!=size))
				{
					size=ri->fontsize;
					lastfontid=ri->fontid;
					face=kGUIFont::GetFace(lastfontid);
					kGUI::SelectFont(face,size);
					qw=face->GetQuickWidths(size);
					height=face->GetPixHeight(size);
					if(height>maxheight)
						maxheight=height;
				}
				ch=GetChar(sstart,&nb);
				assert(ch!=0,"reading past end of string!");
				sstart+=nb;
				slen-=nb;

				if(ch=='\t')
				{
					int tw;

					tw=GetTabWidth(width);
					if(!tw)
					{
						/* use space width if no defined tabs */
						if(qw && ' '<MAXCCACHE)
							width+=qw[' '];
						else
							width+=face->GetCharWidth(' ');
					}
					width+=tw;
				}
				else
				{
					if(qw && ch<MAXCCACHE)
						width+=qw[ch];
					else
						width+=face->GetCharWidth(ch);
					width+=m_letterspacing;
				}
			}
		}
	}
	if(pixwidth)
		pixwidth[0]=width;
	if(pixheight)
		pixheight[0]=maxheight;
}



/* allocate the rich list and fill it with the default info */
void kGUIText::InitRichInfo(void)
{
	int i;
	int num=GetLen();
	RICHINFO_DEF tc;

	m_userichinfo=true;
	m_richinfo.Alloc(num);
	tc.fontid=GetFontID();
	tc.fontsize=GetFontSize();
	tc.bgcolor=m_bgcolor;
	tc.fcolor=GetColor();

	for(i=0;i<num;++i)
		m_richinfo.SetEntry(i,tc);
	m_richinfosize=num;
	StringChanged();
}

void kGUIText::InsertRichInfo(int index,int num)
{
	int i;
	RICHINFO_DEF rid;

	assert(index<=m_richinfosize,"Cannot insert past end of string");

	/* if at beginning or end of string then use "current" info */
	if(index==m_richinfosize || !index)
	{
		rid.fontid=GetFontID();
		rid.fontsize=GetFontSize();
		rid.bgcolor=m_bgcolor;
		rid.fcolor=GetColor();
	}
	else
		rid=*(GetRichInfoPtr(index-1));

	m_richinfo.Alloc(m_richinfosize+num);
	m_richinfo.InsertEntry(m_richinfosize,index,num);

	for(i=0;i<num;++i)
		m_richinfo.SetEntry(index+i,rid);

	m_richinfosize+=num;
	StringChanged();
}

void kGUIText::DeleteRichInfo(int index,int num,bool callchanged)
{
	assert(((index+num)<=m_richinfosize),"Cannot delete negative range");
	m_richinfo.DeleteEntry(index,num);
	m_richinfosize-=num;
	if(callchanged)
		StringChanged();
}

void kGUIText::SetRichFontID(unsigned int si,unsigned int ei,unsigned int fontid)
{
	unsigned int i;
	unsigned int l=GetLen();
	RICHINFO_DEF *ri;

	for(i=si;i<ei;++i)
	{
		if(i==l)
			SetFontID(fontid);
		else
		{
			ri=GetRichInfoPtr(i);
			ri->fontid=fontid;
		}
	}
	StringChanged();
}

void kGUIText::SetRichFontSize(unsigned int si,unsigned int ei,unsigned int fontsize)
{
	unsigned int i;
	unsigned int l=GetLen();
	RICHINFO_DEF *ri;

	for(i=si;i<ei;++i)
	{
		if(i==l)
			SetFontSize(fontsize);
		else
		{
			ri=GetRichInfoPtr(i);
			ri->fontsize=fontsize;
		}
	}
	StringChanged();
}

void kGUIText::SetRichFColor(unsigned int si,unsigned int ei,kGUIColor color)
{
	unsigned int i;
	unsigned int l=GetLen();
	RICHINFO_DEF *ri;

	for(i=si;i<ei;++i)
	{
		if(i==l)
			SetColor(color);
		else
		{
			ri=GetRichInfoPtr(i);
			ri->fcolor=color;
		}
	}
	StringChanged();
}

void kGUIText::SetRichBGColor(unsigned int si,unsigned int ei,kGUIColor color)
{
	unsigned int i;
	unsigned int l=GetLen();
	RICHINFO_DEF *ri;

	for(i=si;i<ei;++i)
	{
		if(i==l)
			SetBGColor(color);
		else
		{
			ri=GetRichInfoPtr(i);
			ri->bgcolor=color;
		}
	}
	StringChanged();
}

kGUIText::~kGUIText()
{
	unsigned int i;
	kGUIInputLineInfo *lbptr;

	for(i=0;i<m_linelist.GetNumEntries();++i)
	{
		lbptr=m_linelist.GetEntry(i);
		if(lbptr)
			delete lbptr;
	}
}


/* calculate the number of lines and break positions */

int kGUIText::CalcLineList(int w)
{
	unsigned int line;
	unsigned int sindex;
	unsigned int totalchars;
	unsigned int numfit;
	unsigned int pixwidth;
	unsigned int maxw;

	const unsigned char *t;
	const unsigned char *cc;
	const unsigned char *ce;
	kGUIInputLineInfo *lbptr;

	if(m_linelist.GetNumEntries()==0)
	{
		m_linelist.Init(1,-1);
		m_linelist.SetEntry(0,new kGUIInputLineInfo);
	}

	maxw=0;
	line=0;
	sindex=0;
	t=(const unsigned char *)GetString();
	totalchars=GetLen();
	if((!t) || (!totalchars))
	{
		lbptr=m_linelist.GetEntry(0);
		lbptr->startindex=sindex;
		lbptr->endindex=sindex;
		lbptr->pixwidth=0;
		lbptr->hardbreak=true;
		lbptr->pixheight=GetLineHeight();
		m_lltotalheight=lbptr->pixheight+2;
		lbptr->ty=0;
		lbptr->by=m_lltotalheight-1;
		m_llnum=1;
		return(0);
	}

	m_lltotalheight=0;
	do{
		numfit=CalcFitWidth(sindex,totalchars-sindex,w,&pixwidth);
		if(pixwidth>maxw)
			maxw=pixwidth;

		/* go back and break on a space, unless there are no spaces */
		if(!numfit)
		{
			if(t[sindex]!=10)				/* supposed to be a blank line? */
				numfit=totalchars-sindex;	/* rest of line */
		}
		else
		{
			cc=t+sindex;
			ce=cc+numfit;
			if(ce[0]!=10 && ce[0])
			{
				while((ce[-1]!=' ') && (ce>cc))
					--ce;

				if(ce!=cc)			/* only if a space was found */
					numfit=(int)(ce-cc);
			}
		}

		/* alloc more space? */
		if(line>=m_linelist.GetNumEntries())
			m_linelist.Alloc(line<<3);

		lbptr=m_linelist.GetEntry(line);
		if(!lbptr)
		{
			lbptr=new kGUIInputLineInfo;
			m_linelist.SetEntry(line,lbptr);
		}
		lbptr->startindex=sindex;		/* line starts at this character */
		lbptr->endindex=sindex+numfit;	/* line stops at this character */

		/* calc size of Substring */
		lbptr->ty=m_lltotalheight;
		GetSubSize(sindex,numfit,&lbptr->pixwidth,&lbptr->pixheight);
		if(!numfit)
		{
			/* how tall is a blank line, use height of last character in previous line */
			if(!sindex)
				lbptr->pixheight=GetLineHeight();
			else
				GetSubSize(sindex-1,1,0,&lbptr->pixheight);
		}
		m_lltotalheight+=lbptr->pixheight+2;
		lbptr->by=m_lltotalheight-1;

		if(t[sindex+numfit]==0 || t[sindex+numfit]==10)
			lbptr->hardbreak=true;
		else
			lbptr->hardbreak=false;

		++line;
		sindex+=numfit;
		if(t[sindex]==10)
			++sindex;
		else if(!numfit)
			break;
	}while(sindex<totalchars);
	m_llnum=line;
	return(maxw);
}

int kGUIText::GetLineNum(unsigned int charindex)
{
	int startindex,endindex,midindex;
	unsigned int endchar;
	kGUIInputLineInfo *lbptr;

	if(m_llnum<2)
		return(0);
	else if(charindex>=GetLen())
		return(m_llnum-1);

	/* binary search */
	startindex=0;
	endindex=m_llnum-1;

	/* make sure split list is accurate! */
	lbptr=GetLineInfo(endindex);
	assert(lbptr->endindex==(GetLen()-1) || lbptr->endindex==(GetLen()),"Split list is NOT up to date!");

	do
	{
		midindex=(startindex+endindex)>>1;
		lbptr=GetLineInfo(midindex);
		endchar=lbptr->endindex;
		if(lbptr->hardbreak==false)
			--endchar;

		if(charindex<lbptr->startindex)
		{
			if(endindex==midindex)
				--endindex;
			else
				endindex=midindex;
		}
		else if(charindex>endchar)
		{
			if(startindex==midindex)
				++startindex;
			else
				startindex=midindex;
		}
		else 
			return(midindex);
	}while(1);
}

int kGUIText::GetLineNumPix(int ypos)
{
	int startindex,endindex,midindex;
	kGUIInputLineInfo *li;

	if(m_llnum<2)
		return(0);

	if(ypos<0)
		return(0);

	if(ypos>=(int)m_lltotalheight)
		return(m_llnum-1);

	/* binary search */
	startindex=0;
	endindex=m_llnum-1;
	do
	{
		midindex=(startindex+endindex)>>1;
		li=GetLineInfo(midindex);
		if(ypos<li->ty)
		{
			if(endindex==midindex)
				--endindex;
			else
				endindex=midindex;
		}
		else if(ypos>li->by)
		{
			if(startindex==midindex)
				++startindex;
			else
				startindex=midindex;
		}
		else 
			return(midindex);
	}while(1);
}

unsigned int kGUIFontInfo::GetFontHeight(void)
{
	kGUIFace *face=kGUIFont::GetFace(GetFontID());

	return(face->GetPixHeight(GetFontSize()));
}

/* get ascender height ( pixels above the baseline ) */
const unsigned int kGUIText::GetAscHeight(void)
{
	kGUIFace *face=kGUIFont::GetFace(GetFontID());

	return(face->GetPixAscHeight(GetFontSize()));
}

/* get descender height ( pixels below the baseline ) */
const unsigned int kGUIText::GetDescHeight(void)
{
	kGUIFace *face=kGUIFont::GetFace(GetFontID());

	return(face->GetPixDescHeight(GetFontSize()));
}


/* return the pixel height for this text using the current
   font and for the given width */
int kGUIText::CalcHeight(int width)
{
	CalcLineList(width);
	return(m_lltotalheight);
}

void kGUIText::DrawChar( char * src, int x,int y,int w,int h,kGUIColor color)
{
	int lx,rx,ty,by;
	int cx,cy;
	const kGUICorners *cc;
	unsigned char s;
	int srcskip;
	int destskip;
	kGUIColor *sp;
	int fr,fg,fb;
	int br,bg,bb;
	int drawr,drawg,drawb;

	if(!w || !h)
		return;

	if(kGUI::OffClip(x,y,x+w,y+h)==true)
		return;

	DrawColorToRGB(color,fr,fg,fb);
	srcskip=0;
	destskip=0;
	lx=x;
	ty=y;
	rx=lx+w;
	by=ty+h;
	cc=kGUI::GetClipCorners();
	if(lx<cc->lx)
	{
		src+=cc->lx-lx;	/* move over */
		lx=cc->lx;
	}
	if(rx>cc->rx)
		rx=cc->rx;
	if(ty<cc->ty)
	{
		src+=(cc->ty-ty)*w;	/* move down */
		ty=cc->ty;
	}
	if(by>cc->by)
		by=cc->by;

	sp=kGUI::GetSurfacePtr(lx,ty);
	destskip=kGUI::GetSurfaceWidth()-(rx-lx);
	srcskip=w-(rx-lx);

	if(m_alpha==1.0f)
	{
		for(cy=ty;cy<by;++cy)
		{
			for(cx=lx;cx<rx;++cx)
			{
				s=*(src++);
				if(s)
				{
					DrawColorToRGB(*(sp),br,bg,bb);

					drawr=br+(((fr-br)*s)/256);
					drawg=bg+(((fg-bg)*s)/256);
					drawb=bb+(((fb-bb)*s)/256);
					*(sp++)=DrawColor(drawr,drawg,drawb);
				}
				else
					++sp;
			}
			sp+=destskip;
			src+=srcskip;
		}
	}
	else
	{
		for(cy=ty;cy<by;++cy)
		{
			for(cx=lx;cx<rx;++cx)
			{
				s=(unsigned char)((*((unsigned char *)src++))*m_alpha);
				if(s)
				{
					DrawColorToRGB(*(sp),br,bg,bb);
					drawr=br+(((fr-br)*s)/256);
					drawg=bg+(((fg-bg)*s)/256);
					drawb=bb+(((fb-bb)*s)/256);
					*(sp++)=DrawColor(drawr,drawg,drawb);
				}
				else
					++sp;
			}
			sp+=destskip;
			src+=srcskip;
		}
	}
	if(m_underline==true)
		kGUI::DrawLine(x,y+h,x+w,y+h,color);
}

void kGUIText::Draw(int x,int y,int w,int h)
{
	/* this is the only draw call that looks at the valign and halign */
	kGUIInputLineInfo *lbptr;
	int i;
	FT_HALIGN halign=GetHAlign();
	int cx=0;
	int cy;
	int numl;
	int th;

	if(m_llnum<2)
	{
		numl=1;
		th=GetLineHeight()+2;
	}
	else
	{
		numl=m_llnum;
		th=m_lltotalheight;
	}

	cy=y;
	switch(GetVAlign())
	{
	case FT_TOP:
	break;
	case FT_MIDDLE:
		cy+=(h-th)/2;
	break;
	case FT_BOTTOM:
		cy+=h-th;
	break;
	}

	if(m_llnum<2)
	{
		switch(halign)
		{
		case FT_LEFT:
			cx=x;
		break;
		case FT_CENTER:
			cx=x+((w-GetWidth())/2);
		break;
		case FT_RIGHT:
			cx=x+(w-GetWidth());
		break;
		}

		DrawSection(0,GetLen(),x,cx,cy,GetLineHeight());
	}
	else
	{
		for(i=0;i<m_llnum;++i)
		{
			lbptr=m_linelist.GetEntry(i);
			switch(halign)
			{
			case FT_LEFT:
				cx=x;
			break;
			case FT_CENTER:
				cx=x+((w-lbptr->pixwidth)/2);
			break;
			case FT_RIGHT:
				cx=x+(w-lbptr->pixwidth);
			break;
			}
			DrawSection(lbptr->startindex,lbptr->endindex-lbptr->startindex,x,cx,cy,lbptr->pixheight);
			cy+=lbptr->pixheight+2;
		}
	}
}

void kGUIText::Draw(int x,int y,int w,int h,kGUIColor color)
{
	/* this is the only draw call that looks at the valign and halign */
	kGUIInputLineInfo *lbptr;
	int i;
	FT_HALIGN halign=GetHAlign();
	int cx=0;
	int cy;
	int lineheight=GetLineHeight()+2;
	int numl;

	if(m_llnum<2)
		numl=1;
	else
		numl=m_llnum;

	cy=y;
	switch(GetVAlign())
	{
	case FT_TOP:
	break;
	case FT_MIDDLE:
		cy+=(h-((lineheight*numl)-2))/2;
	break;
	case FT_BOTTOM:
		cy+=h-((lineheight*numl)-2);
	break;
	}

	if(m_llnum<2)
	{
		switch(halign)
		{
		case FT_LEFT:
			cx=x;
		break;
		case FT_CENTER:
			cx=x+((w-(int)GetWidth())/2);
		break;
		case FT_RIGHT:
			cx=x+(w-(int)GetWidth());
		break;
		}

		DrawSection(0,GetLen(),x,cx,cy,GetLineHeight(),color);
	}
	else
	{
		for(i=0;i<m_llnum;++i)
		{
			lbptr=m_linelist.GetEntry(i);
			switch(halign)
			{
			case FT_LEFT:
				cx=x;
			break;
			case FT_CENTER:
				cx=x+((w-lbptr->pixwidth)/2);
			break;
			case FT_RIGHT:
				cx=x+(w-lbptr->pixwidth);
			break;
			}
			DrawSection(lbptr->startindex,lbptr->endindex-lbptr->startindex,x,cx,cy,lbptr->pixheight,color);
			cy+=lineheight;
		}
	}
}

/* 0= use space, no user tabs defined */
int kGUIText::GetTabWidth(int localx)
{
	unsigned int tabindex;

	if(!m_tablist.GetNumEntries())
		return(0);

	/* are they fixed width or unique widths? */
	if(m_fixedtabs==true)
	{
		int tx,tw;

		tx=tw=m_tablist.GetEntry(0);
		while(localx>=tx)
			tx+=tw;
		return(tx-localx);
	}

	tabindex=0;
	while(tabindex<m_tablist.GetNumEntries())
	{
		if(m_tablist.GetEntry(tabindex)>localx)
		{
			return(m_tablist.GetEntry(tabindex)-localx);
		}
		++tabindex;
	}
	return(0);	/* past end of tab list */
}

void kGUIText::DrawSection(int sstart,int slen,int sx,int x,int y,int rowheight,kGUIColor color)
{
	kGUIFace *face;
	int glyph_index;
	int font_height,font_above,font_below;
	const kGUICorners *clip;
	FT_Face ftface;
	int size;
	bool cachesize;
	int *cacheptr;
	unsigned int ch;	/* current character */
	unsigned int nb;	/* number of bytes for current character */
	int ry;
	bool isbold;

	size=GetFontSize();
	if(!size)
		return;

	if(size<MAXQUICKSIZE)
		cachesize=true;	
	else
		cachesize=false;

	face=kGUIFont::GetFace(GetFontID());
	isbold=face->GetBold();
	ftface=face->GetFace();
	font_height=face->GetPixHeight(size);
	ry=y+rowheight-font_height;
	font_above = face->GetPixAscHeight(size);
	font_below = face->GetPixDescHeight(size);

	kGUI::SelectFont(face,size);

	clip=kGUI::GetClipCorners();
	if(y>clip->by)
		return;

	while(slen>0)
	{
		/* get character in current string encoding mode */
		ch=GetChar(sstart,&nb);
		if(!ch)
			return;
		sstart+=nb;
		slen-=nb;

		/* is this character in the cache? */
		if( cachesize==true && (ch<MAXCCACHE))
			cacheptr=face->m_quickcache[size][ch];
		else
			cacheptr=0;
		if(cacheptr)
		{
			DrawChar( (char *)(cacheptr+5),
						x + cacheptr[2],
						y+font_above-cacheptr[3],
						cacheptr[0], cacheptr[1],
						color);

			x+=cacheptr[4];
			x+=m_letterspacing;
			if(x>clip->rx)
				return;
		}
		else
		{
			if(ch=='\t')
			{
				int tw;

				tw=GetTabWidth(x-sx);
				if(!tw)
				{
					if( cachesize==true && (' '<MAXCCACHE))
					{
						cacheptr=face->m_quickcache[size][(int)' '];
						tw=cacheptr[4];
					}
					else
						tw=face->GetCharWidth(' ');
				}
				x+=tw;
				x+=m_letterspacing;
				if(x>clip->rx)
					return;
			}
			else
			{
				glyph_index = FT_Get_Char_Index( ftface, ch );
				if(glyph_index>0)
				{
					if(!FT_Load_Glyph(ftface, glyph_index, FT_LOAD_DEFAULT))
					{
						if(!FT_Render_Glyph( ftface->glyph, ft_render_mode_normal ))
						{
							if(isbold)
								FT_GlyphSlot_Embolden(ftface->glyph);

							/* cache this character? */
							if( cachesize==true && (ch<MAXCCACHE))
							{
								int bsize=ftface->glyph->bitmap.width*ftface->glyph->bitmap.rows;

								cacheptr = new int[5+((bsize)/sizeof(int))+1];
								face->m_quickcache[size][ch]=cacheptr;
								cacheptr[0]=ftface->glyph->bitmap.width;
								cacheptr[1]=ftface->glyph->bitmap.rows;
								cacheptr[2]=ftface->glyph->bitmap_left;
								cacheptr[3]=ftface->glyph->bitmap_top;
								cacheptr[4]=ftface->glyph->advance.x >> 6;
								memcpy(cacheptr+5,ftface->glyph->bitmap.buffer,bsize);
							}

							/* draw to screen using writepixel */
							DrawChar( (char *)ftface->glyph->bitmap.buffer,
										x + ftface->glyph->bitmap_left,
										y+font_above-ftface->glyph->bitmap_top,
										ftface->glyph->bitmap.width, ftface->glyph->bitmap.rows,
										color);

							x+=ftface->glyph->advance.x >> 6;
							x+=m_letterspacing;
							if(x>clip->rx)
								return;
						}
					}
				}
			}
		}
	}
}

void kGUIText::DrawSection(int sstart,int slen,int sx,int x,int y,int rowheight)
{
	int glyph_index;
	int font_height,font_above,font_below;
	const kGUICorners *clip;
	unsigned int size;
	bool cachesize;
	int *cacheptr;
	kGUIColor c;
	int tw;
	bool usebg;
	int nc=0;	/* num chars drawn */
	Array<PRINTFONTCHAR_DEF>pfclist;
	PRINTFONTCHAR_DEF pfc;
	int offx=0,offy=0;
	bool printjobactive;
	unsigned int ch;	/* current character */
	unsigned int nb;	/* number of bytes for current character */
	kGUIColor fcolor;
	kGUIColor bgcolor;
	unsigned int fontid;
	kGUIFace *face;
	FT_Face ftface;
	int ry;
	bool isbold;

	size=GetFontSize();
	if(!size)
		return;

	fontid=GetFontID();
	face=kGUIFont::GetFace(fontid);
	isbold=face->GetBold();
	ftface=face->GetFace();

	usebg=GetUseRichInfo();

	if(size<MAXQUICKSIZE)
		cachesize=true;
	else
		cachesize=false;

	font_height=face->GetPixHeight(size);
	ry=y+rowheight-font_height;
	font_above = face->GetPixAscHeight(size);
	font_below = face->GetPixDescHeight(size);
	kGUI::SelectFont(face,size);

	clip=kGUI::GetClipCorners();
	if(GetUseRichInfo()==false)
	{
		if(ry>clip->by)
			return;
	}

	if(kGUI::GetPrintJob())
	{
		pfclist.Alloc(slen);
		printjobactive=true;
		offx=kGUI::GetCurrentSurface()->GetOffsetX();
		offy=kGUI::GetCurrentSurface()->GetOffsetY();
	}
	else
		printjobactive=false;

	while(slen>0)
	{
		/* get character in current string encoding mode */
		ch=GetChar(sstart,&nb);
		if(!ch)
			return;

		if(GetUseRichInfo()==true)
		{
			RICHINFO_DEF *ri;

			ri=GetRichInfoPtr(sstart);
			fcolor=ri->fcolor;
			bgcolor=ri->bgcolor;
			if(fontid!=ri->fontid || size!=ri->fontsize)
			{
				fontid=ri->fontid;
				size=ri->fontsize;

				face=kGUIFont::GetFace(fontid);
				ftface=face->GetFace();
				font_height=face->GetPixHeight(size);
				ry=y+rowheight-font_height;
				font_above = face->GetPixAscHeight(size);
				font_below = face->GetPixDescHeight(size);
				kGUI::SelectFont(face,size);
				if(size<MAXQUICKSIZE)
					cachesize=true;
				else
					cachesize=0;
			}
		}
		else
		{
			fcolor=GetColor();
			bgcolor=GetBGColor();
			usebg=m_usebg;
		}

		/* is this character in the reverse area? */
		if(sstart>=m_rstart && sstart<m_rend)
		{
			/* yes, swap the foreground and background colors */
			c=fcolor;
			fcolor=bgcolor;
			bgcolor=c;
			usebg=true;
		}
		sstart+=nb;
		slen-=nb;

		/* is this character in the cache? */
		if( cachesize==true && (ch<MAXCCACHE))
			cacheptr=face->m_quickcache[size][ch];
		else
			cacheptr=0;

		/* save position, char and color info */
		if(printjobactive)
		{
			if(ch!='\t')
			{
				pfc.c=ch;
				pfc.fgcol=fcolor;
				pfc.lx=x+offx;
				pfc.ty=ry+offy;
				pfc.rx=clip->rx+offx;
				pfc.by=clip->by+offy;
				pfclist.SetEntry(nc++,pfc);
			}
		}

		if(cacheptr)
		{
			if(usebg==true)
				kGUI::DrawRect(x,y,x+cacheptr[4],y+font_height,bgcolor);
			DrawChar( (char *)(cacheptr+5),
						x + cacheptr[2],
						ry+font_above-cacheptr[3],
						cacheptr[0], cacheptr[1],
						fcolor);

			x+=cacheptr[4];
			x+=m_letterspacing;
			if(x>clip->rx)
				goto done;
		}
		else
		{
			if(ch=='\t')
			{
				int tw;

				tw=GetTabWidth(x-sx);
				if(!tw)
				{
					if( cachesize==true && (' '<MAXCCACHE))
					{
						cacheptr=face->m_quickcache[size][(int)' '];
						if(cacheptr)
							tw=cacheptr[4];
						else
							tw=face->GetCharWidth(' ');
					}
					else
						tw=face->GetCharWidth(' ');
				}
				if(usebg==true)
					kGUI::DrawRect(x,y,x+tw,y+font_height,bgcolor);
				x+=tw;
				x+=m_letterspacing;
				if(x>clip->rx)
					return;
			}
			else
			{
				glyph_index = FT_Get_Char_Index( ftface, ch );
				if(glyph_index>0)
				{
					if(!FT_Load_Glyph(ftface, glyph_index, FT_LOAD_DEFAULT))
					{
						if(!FT_Render_Glyph( ftface->glyph, ft_render_mode_normal ))
						{
							if(isbold)
								FT_GlyphSlot_Embolden(ftface->glyph);

							/* cache this character? */
							if( cachesize==true && (ch<MAXCCACHE))
							{
								int bsize=ftface->glyph->bitmap.width*ftface->glyph->bitmap.rows;

								cacheptr = new int[5+((bsize)/sizeof(int))+1];
								face->m_quickcache[size][ch]=cacheptr;
								cacheptr[0]=ftface->glyph->bitmap.width;
								cacheptr[1]=ftface->glyph->bitmap.rows;
								cacheptr[2]=ftface->glyph->bitmap_left;
								cacheptr[3]=ftface->glyph->bitmap_top;
								cacheptr[4]=ftface->glyph->advance.x >> 6;
								memcpy(cacheptr+5,ftface->glyph->bitmap.buffer,bsize);
							}

							tw=ftface->glyph->advance.x >> 6;
							if(usebg==true)
								kGUI::DrawRect(x,y,x+tw,y+font_height,bgcolor);
							DrawChar( (char *)ftface->glyph->bitmap.buffer,
										x + ftface->glyph->bitmap_left,
										ry+font_above-ftface->glyph->bitmap_top,
										ftface->glyph->bitmap.width, ftface->glyph->bitmap.rows,
										fcolor);

							x+=tw;
							x+=m_letterspacing;
							if(x>clip->rx)
								goto done;
						}
					}
				}
			}
		}
	}
done:;

	/* send to printer? */
	if(kGUI::GetPrintJob() && nc)
		kGUI::GetPrintJob()->DrawTextList(size,face->GetName(),nc,&pfclist);
}

