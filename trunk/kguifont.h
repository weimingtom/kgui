#ifndef __KGUIFONT__
#define __KGUIFONT__

/**********************************************************************************/
/* kGUI - kguifont.h                                                              */
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

/* this is an internal include file for kguifont.cpp and kguifont2.cpp ONLY */

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define MAXQUICKSIZE 24

#define MAXCCACHE 128

class kGUIFace
{
	friend class kGUI;
	friend class kGUIText;
public:
	~kGUIFace();
	int LoadFont(const char *filename);
	void Purge(void);
	inline const FT_Face GetFace(void) {return m_ftface;}
	inline FT_Face *GetFacePtr(void) {return &m_ftface;}
	int GetCharWidth(unsigned int ch);
	inline int GetPixHeight(int size) {CalcHeight(size);return m_pixabove[size]+m_pixbelow[size];}
	inline int GetPixAscHeight(int size) {CalcHeight(size);return m_pixabove[size];}
	inline int GetPixDescHeight(int size) {CalcHeight(size);return m_pixbelow[size];}
	inline unsigned int *GetQuickWidths(int size) {if((size)>=MAXQUICKSIZE) return 0;return m_quickwidths[size];}
	unsigned char *m_memfile;
	const char *GetName(void) {return m_name.GetString();}
	void CalcHeight(unsigned int size);
private:
	FT_Face m_ftface;
	kGUIString m_name;
	int m_pixabove[MAXFONTSIZE];
	int m_pixbelow[MAXFONTSIZE];
	unsigned int m_quickwidths[MAXQUICKSIZE][MAXCCACHE];
	int *m_quickcache[MAXQUICKSIZE][MAXCCACHE];
};

class kGUIFontFileInfo
{
public:
	void SetFilename(const char *s) {m_filename.SetString(s);} 
	void SetFacename(const char *s) {m_facename.SetString(s);} 
	void SetStyle(const char *s) {m_style.SetString(s);} 

	kGUIString *GetFilename(void) {return &m_filename;}
	kGUIString *GetFacename(void) {return &m_facename;}
	kGUIString *GetStyle(void) {return &m_style;}
private:
	kGUIString m_filename;
	kGUIString m_facename;
	kGUIString m_style;
};

class kGUIFont
{
	friend class kGUI;
public:
	static kGUIFace *GetFace(const unsigned int n)	{/*assert(n<m_numfonts,"Error Bad Font Reference");*/return &m_faces[n];}
	static inline FT_Library GetLibrary(void) {return m_library;}

#if defined(WIN32) || defined(MINGW) 
	static DWORD m_numreg;			/* number of fonts registered to system (pc only) */
#elif defined(LINUX) || defined(MACINTOSH)
#else
#error
#endif

private:
	static FT_Library m_library;    /* the FreeType library */
	static unsigned int m_numfonts;			/* number of fonts allocated */
	static kGUIFace m_faces[MAXFONTS]; /* the font faces */
	static FT_Error m_error;        /* error returned by FreeType? */
	static kGUIFace *m_lastface;
	static unsigned int m_lastsize;
	static ClassArray<kGUIFontFileInfo>m_ffilist;
	static unsigned int m_ffinumentries;
};

#endif
