/**********************************************************************************/
/* kGUI - kguistring.cpp                                                          */
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

/*! @file kguistring.cpp
    @brief This is the string handling class. It handles all allocation for holding a     
 string. strings can have different encodings. There are also a sprintf and     
 asprintf (append sprintf) members that handle allocation as strings grow       
                                                                                
 There is also s stringsplit class that splits strings based on a               
 'split string' typically a comma or a tab character, but it can also be a      
 multi character string too. */

#include "kgui.h"

void kGUIString::Alloc(unsigned int l,bool preserve)
{
	unsigned int oldlen;

	if(l<=m_maxlen)
		return;		/* already big enough */

	oldlen=m_maxlen;
	m_maxlen=l;
	if(m_string==0 && preserve==true)
		preserve=false;		/* nothing to preserve yet silly */

	if(!preserve)
	{
		if(m_string && m_string!=m_defstring)
			delete []m_string;
		m_string=new char[l+1];
		m_string[0]=0;
	}
	else
	{
		char *save;

		save=m_string;
		m_string=new char[l+1];
		memcpy(m_string,save,oldlen+1);

		if(save!=m_defstring)
			delete []save;
	}
}

void kGUIString::Clip(unsigned int len)
{
	if(m_len>len)
	{
		m_len=len;
		m_string[len]=0;
		StringChanged();
	}
}

void kGUIString::Clip(const char *s)
{
	const char *sp;
	unsigned int newlen;

	sp=strstr(m_string,s);
	if(sp)
	{
		newlen=(unsigned int)(sp-m_string);
		if(m_len>newlen)
		{
			m_len=newlen;
			m_string[newlen]=0;
			StringChanged();
		}
	}
}


/* expand encoded characters into possible 32 bit characters */
unsigned int kGUIString::GetChar(unsigned int pos,unsigned int *numbytes)
{
	unsigned int c=0;
	unsigned int i=1;

	if(pos<m_len)
	{
		switch(m_encoding)
		{
		case ENCODING_8BIT:
			i=1;
			c=m_string[pos]&255;
		break;
		case ENCODING_UTF8:
			// http://en.wikipedia.org/wiki/UTF-8
			c=m_string[pos]&255;
			if(c<128)
				i=1;
			else
			{
				if(c<0xdf)
				{
					if((pos+1)>=m_len)
					{
						/* error, return '?'! */
						c='?';
					}
					else
					{
						unsigned int c1=c&31;
						unsigned int c2=(m_string[pos+1]&255)&63;

						i=2;
						c=(c1<<6)|c2;
					}
				}
				else if(c<0xef)
				{
					if((pos+2)>=m_len)
					{
						/* error, return '?'! */
						c='?';
					}
					else
					{
						unsigned int c1=c&15;
						unsigned int c2=(m_string[pos+1]&255)&63;
						unsigned int c3=(m_string[pos+2]&255)&63;

						i=3;
						c=(c1<<12)|(c2<<6)|c3;
					}
				}
				else if(c<0xff)
				{
					if((pos+3)>=m_len)
					{
						/* error, return '?'! */
						c='?';
					}
					else
					{
						unsigned int c1=c&7;
						unsigned int c2=(m_string[pos+1]&255)&63;
						unsigned int c3=(m_string[pos+2]&255)&63;
						unsigned int c4=(m_string[pos+3]&255)&63;

						i=4;
						c=(c1<<18)|(c2<<12)|(c3<<6)|c4;
					}
				}
			}
		break;
		default:
			assert(false,"Unsupported format!");
		break;
		}
	}
	*(numbytes)=i;
	return(c);
}

//go back 1 character, return number of bytes back
unsigned int kGUIString::GoBack(unsigned int pos)
{
	unsigned int c;
	unsigned int i;

	i=1;
	switch(m_encoding)
	{
	case ENCODING_8BIT:
	break;
	case ENCODING_UTF8:
		--pos;
		c=m_string[pos];
		if(c>=128)
		{
			while((c&0xc0)==0x80)
			{
				--pos;
				c=m_string[pos];
				++i;
			}
		}
	break;
	}
	return(i);
}

void kGUIString::ChangeEncoding(unsigned int e)
{
	if(m_encoding!=e)
	{
		unsigned int index;
		unsigned int c;
		unsigned int nb;

		kGUIString newstring;

		/* rebuild string with new encoding */

		newstring.SetEncoding(e);
		index=0;
		while(index<GetLen())
		{
			c=GetChar(index,&nb);
			index+=nb;

			/* append 'c' to new string */
			switch(e)
			{
			case ENCODING_8BIT:
				newstring.Append((char)c);
			break;
			case ENCODING_UTF8:
				if(c<0x80)
					newstring.Append((char)c);
				else if(c<0x800)
				{
					newstring.Append((char)((0xc0)|(c>>6)));
					newstring.Append((char)((0x80)|(c&0x3f)));
				}
				else if(c<0xd800)
				{
					newstring.Append((char)((0xe0)|(c>>12)));
					newstring.Append((char)((0x80)|((c>>6)&0x3f)));
					newstring.Append((char)((0x80)|(c&0x3f)));
				}
				else
				{
					newstring.Append((char)((0xf0)|(c>>18)));
					newstring.Append((char)((0x80)|((c>>12)&0x3f)));
					newstring.Append((char)((0x80)|((c>>6)&0x3f)));
					newstring.Append((char)((0x80)|(c&0x3f)));
				}
			break;
			}
		}
		/* replace with new string */
		SetString(&newstring);
	}
}

/* convert from a cursor position to a character index, this is necessary since */
/* some encoding like UTF-8 can have 1-4 bytes represent a character */

unsigned int kGUIString::CursorToIndex(unsigned int cursor)
{
	unsigned int index=0;
	unsigned int c=0;
	unsigned int nb;

	while(c<cursor)
	{
		GetChar(c,&nb);
		c+=nb;
		++index;
	}
	return(index);
}

/* convert from a character index to a cursor position, this is necessary since */
/* some encoding like UTF-8 can have 1-4 bytes represent a character */

unsigned int kGUIString::IndexToCursor(unsigned int index)
{
	unsigned int c=0;
	unsigned int nb;

	while(index)
	{
		GetChar(c,&nb);
		c+=nb;
		--index;
	}
	return(c);
}

void kGUIString::SetChar(unsigned int pos, char c)
{
	assert(pos<m_len,"Trying to set past end of string");
	
	if(m_string[pos]==c)
		return;

	m_string[pos]=c;
	if(!c)
		m_len=pos;
	StringChanged();
}

void kGUIString::SetString(const char *t)
{
	unsigned int tl;

	if(m_string==t)
		return;					/* setting to myself? */

	if(!t)
		tl=0;
	else
		tl=(unsigned int)strlen(t);

	if(tl>m_maxlen)				/* will it fit? */
		Alloc(tl,false);

	if(t)
	{
		if(!strcmp(m_string,t))
			return;		/* same, don't trigger changed */
		strcpy(m_string,t);
	}
	else
	{
		if(!m_string[0])
			return;		/* same, don't trigger changed */
		m_string[0]=0;
	}
	m_len=tl;
	
	StringChanged();
}

void kGUIString::SetString(const char *t,unsigned int numchars)
{
	if(numchars>m_maxlen)			/* will it fit? */
		Alloc(numchars,false);

	if(m_len==numchars)
	{
		/* add one to make sure the null terminator is the same in both too */
		if(!memcmp(m_string,t,numchars+1))
			return;		/* same, don't trigger changed */
	}

	/* caution: may contain nulls, so we will use memcpy instead of strncpy */
	memcpy(m_string,t,numchars);
	m_string[numchars]=0;
	m_len=numchars;
	StringChanged();
}

void kGUIString::SetString16(const char *t)
{
	unsigned int tl;
	char c;

	/* count number of 16 bit characters */
	tl=0;
	while(t[tl<<1])
		++tl;

	if(tl>m_maxlen)				/* will it fit? */
		Alloc(tl,false);

	tl=0;
	do
	{
		c=t[tl<<1];
		m_string[tl]=c;
		++tl;
	}while(c);
	m_len=tl-1;
	
	StringChanged();
}

void kGUIString::Delete(unsigned int index,unsigned int num)
{
	assert(m_string!=0,"Cannot move null string");
	assert(((index+num)<=m_len),"Cannot delete negative range");

	/* +1 is to copy the null along too */
	memmove(m_string+index,m_string+index+num,m_len-(index+num)+1);
	m_len-=num;
	StringChanged();
}

void kGUIString::Insert(unsigned int index,const char *itext)
{
	unsigned int ilen=(unsigned int)strlen(itext);

	assert(m_string!=0,"Cannot move null string");
	assert(index<=m_len,"Cannot insert past end of string");

	if((m_len+ilen)>m_maxlen)		/* will it fit? */
		Alloc(m_len+ilen+MAX(m_len>>2,32),true);	

	m_len+=ilen;
	memmove(m_string+index+ilen,m_string+index,((m_len+1)-ilen)-index);
	memcpy(m_string+index,itext,ilen);

	StringChanged();
}

void kGUIString::Append(unsigned int c)
{
	/* append 'c' to new string */
	switch(m_encoding)
	{
	case ENCODING_8BIT:
		Append((char)c);
	break;
	case ENCODING_UTF8:
		if(c<0x80)
			Append((char)c);
		else if(c<0x800)
		{
			Append((char)((0xc0)|(c>>6)));
			Append((char)((0x80)|(c&0x3f)));
		}
		else if(c<0xd800)
		{
			Append((char)((0xe0)|(c>>12)));
			Append((char)((0x80)|((c>>6)&0x3f)));
			Append((char)((0x80)|(c&0x3f)));
		}
		else
		{
			Append((char)((0xf0)|(c>>18)));
			Append((char)((0x80)|((c>>12)&0x3f)));
			Append((char)((0x80)|((c>>6)&0x3f)));
			Append((char)((0x80)|(c&0x3f)));
		}
	break;
	}
}

void kGUIString::Append(char c)
{
	char as[2];

	as[0]=c;
	as[1]=0;
	Append(as);
}

void kGUIString::Append(const char *atext)
{
	assert(atext!=0,"Null string!");
	Append(atext,(unsigned int)strlen(atext));
}

void kGUIString::Append(const char *atext,unsigned int alen)
{
	assert(atext!=0,"Null string!");
	if((m_len+alen)>m_maxlen)		/* will it fit? */
		Alloc(m_len+alen+MAX(m_len>>2,32),true);	

	/* caution: may contain nulls */
	memcpy(m_string+m_len,atext,alen);
	m_len+=alen;
	m_string[m_len]=0;
	StringChanged();
}

/* remove all characters from the string that are not in the validchars list */

void kGUIString::Clean(const char *validchars)
{
	unsigned int i;
	char cc[2];

	i=0;
	cc[1]=0;
	while(i<m_len)
	{
		cc[0]=m_string[i];
		if(!strstr(validchars,cc))
			Delete(i,1);	/* remove character from string */
		else
			++i;
	}
}

/* copy a substring of the text to a dest buffer */

void kGUIString::CopyString(char *dest,unsigned int start,unsigned int len)
{
	assert(m_string!=0,"Cannot copy null string");
	memcpy(dest,m_string+start,len);
	dest[len]=0;
}

void kGUIString::Proper(bool leaveup)
{
	unsigned int i;
	bool wasletter=false;

	for(i=0;i<m_len;++i)
	{
		if(m_string[i]>='a' && m_string[i]<='z')
		{
			if(wasletter==false)	
				m_string[i]^=('a'-'A');	/* make upper case */
			wasletter=true;
		}
		else if(m_string[i]>='A' && m_string[i]<='Z')
		{
			if(wasletter==true && leaveup==false)
				m_string[i]^=('a'-'A');	/* make lower case */
			wasletter=true;
		}
		else
			wasletter=false;
	}
	StringChanged();
}

void kGUIString::Upper(void)
{
	unsigned int i;

	for(i=0;i<m_len;++i)
	{
		if(m_string[i]>='a' && m_string[i]<='z')
			m_string[i]^=('a'-'A');	/* make upper case */
	}
	StringChanged();
}

void kGUIString::Lower(void)
{
	unsigned int i;

	for(i=0;i<m_len;++i)
	{
		if(m_string[i]>='A' && m_string[i]<='Z')
			m_string[i]^=('a'-'A');	/* make lower case */
	}
	StringChanged();
}

void kGUIString::Trim(int what)
{
	int num;
	char *place;

	/* remove leading spaces, tabs, or lf, or c/r */
	num=0;
	place=m_string;
	while(((place[0]==' ') && (what&TRIM_SPACE)) || ((place[0]==9) && (what&TRIM_TAB)) || ((place[0]==10) && (what&TRIM_CR)) || ((place[0]==13) && (what&TRIM_CR)))
	{
		++num;
		++place;
	}
	if(num)
	{
		memmove(m_string,m_string+num,(m_len+1)-num);
		m_len-=num;
	}

	/* remove trailing spaces, tabs, or lf, or c/r */

	if(m_len)
	{
		place=m_string+m_len-1;
		while(((place[0]==' ') && (what&TRIM_SPACE)) || ((place[0]==9) && (what&TRIM_TAB)) || ((place[0]==10) && (what&TRIM_CR)) || ((place[0]==13) && (what&TRIM_CR)) || ((place[0]==0) && (what&TRIM_NULL)))	
		{
			place[0]=0;
			--place;
			--m_len;
			if(!m_len)
				break;
		}
	}
	if(what&TRIM_QUOTES)
		RemoveQuotes();
	StringChanged();
}

//int start=0,int casemode=0,int maxchanges=-1
int kGUIString::Replace(const char *from,const char *to,unsigned int start,int casemode,int maxchanges)
{
	char *place;
	unsigned int newlen;
	int lfrom;
	int lto;
	int ldelta;
	int numchanges;

	lfrom=(unsigned int)strlen(from);
	if(m_len<(unsigned int)lfrom)
		return(0);			/* quick return, can't be in here! */
	if(to)
		lto=(unsigned int)strlen(to);
	else
		lto=0;
	ldelta=lto-lfrom;

	numchanges=0;

	/* if the strings are the same size then the dest string will always be the same size */
	if(!ldelta)
	{
		place=m_string+start;
		do{
			if(!casemode)
				place=strstr(place,from);
			else
				place=strstri(place,from);
			if(place)
			{
				/* copy to over from */
				memcpy(place,to,lto);
				place+=lto;	/* skip replacment */
				if(++numchanges==maxchanges)
					break;
			}
		}while(place);
	}
	else if(ldelta>0)
	{
		/* calculate the new updated length after the changes */
		newlen=m_len;
		assert(start<m_len,"Offset too large!");
		place=m_string+start;
		do{
			if(!casemode)
				place=strstr(place,from);
			else
				place=strstri(place,from);

			if(place)
			{
				newlen+=ldelta;
				place+=lfrom;	/* skip original */
				if(++numchanges==maxchanges)
					break;
			}
		}while(place);

		if(!numchanges)
			return(0);

		/* is there enough space for the new string? if not, make more space */
		if(newlen>m_maxlen)
			Alloc(newlen+8,true);	

		numchanges=0;
		place=m_string+start;
		do{
			if(!casemode)
				place=strstr(place,from);
			else
				place=strstri(place,from);
			if(place)
			{
				/* insert space memmove will copy reverse for overlapping areas */
				memmove(place+lfrom+ldelta,place+lfrom,strlen(place+lfrom)+1);
				/* copy to over from */
				memcpy(place,to,lto);
				if(++numchanges==maxchanges)
					break;
				place+=lto;	/* skip replacment */
			}
		}while(place);
		m_len=newlen;
		assert(m_len==(unsigned int)strlen(m_string),"length error!");
	}
	else
	{
		/* since dest string cannot be larger we can do this in one pass */
		newlen=m_len;
		assert(start<m_len,"Offset too large!");
		place=m_string+start;
		do{
			if(!casemode)
				place=strstr(place,from);
			else
				place=strstri(place,from);
			if(place)
			{
				/* collapse space memmove will copy reverse for overlapping areas */
				memmove(place+lfrom+ldelta,place+lfrom,strlen(place+lfrom)+1);
				/* copy to over from */
				memcpy(place,to,lto);
				newlen+=ldelta;
				if(++numchanges==maxchanges)
					break;
				place+=lto;	/* skip replacment */
			}
		}while(place);
		m_len=newlen;
		assert(m_len==(unsigned int)strlen(m_string),"length error!");
	}
	StringChanged();
	return(numchanges);
}


int kGUIString::ASprintf(const char *fmt,...)
{
	va_list args;
	int oldlen=GetLen();

    va_start(args, fmt);
	AVSprintf(fmt,args);
    va_end(args);
	return(GetLen()-oldlen);	/* return added length */
}

int kGUIString::Sprintf(const char *fmt,...)
{
	va_list args;

    va_start(args, fmt);
	Clear();
	AVSprintf(fmt,args);
    va_end(args);
	return(GetLen());
}

#if 0
#define COMPARESPRINT 1

/* appends to current string contents */
void kGUIString::AVSprintf(const char *fmt,va_list args)
{
	char fstring[65536];

#if COMPARESPRINT
	//compare results using NEW to current
	kGUIString test;
	const char *p1;
	const char *p2;

	test.NewAVSprintf(fmt,args);
	_vsnprintf(fstring, sizeof(fstring), fmt, args);
	p1=test.GetString();
	p2=fstring;
	do
	{
		if(p1[0]!=p2[0])
			assert(false,"String mismatch!");
		else if(!p1[0])
			break;
		++p1;
		++p2;
	}while(1);

	Append(fstring);
#else

	/* this one can be clipped if the buffer is not large enough!, you have been warned */
	_vsnprintf(fstring, sizeof(fstring), fmt, args);
	Append(fstring);
#endif
}
#endif

#define SPRFLAG_ZEROPAD 1

/* appends to current string contents */
void kGUIString::AVSprintf(const char *fmt,va_list args)
{
	char c;
	char nstring[64];
	char dstring[64];
	char *np;
	int nc;
	const char *sfmt;

	/* this is by no means a complete implementation, just one that will not overflow */

	nc=0;
	sfmt=fmt;
	while(fmt[0])
	{
		c=*(fmt++);
		if(c!='%')
			++nc;
		else
		{
			if(nc)
				Append(sfmt,nc);
			np=nstring;
			*(np++)='%';
more:;
			c=*(fmt++);
			switch(c)
			{
			case 0:
				return;	/* end of string! */
			break;
			case ' ':
			case '+':
			case '-':
			case '.':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'l':
				*(np++)=c;
				goto more;
			break;
			case '%':
				Append('%');
			break;
			case 'c':
			{
				int val;

				val = va_arg(args, int);
				Append((char)val);		/* what if string is in UTF-8 mode? */
			}
			break;
			case 's':
			{
				char *cp;

			    cp = va_arg(args, char *);
				assert(cp!=0,"Null string!");
				Append(cp);
			}
			break;
			case 'S':
			{
				kGUIString *so;

			    so = va_arg(args, kGUIString *);
				Append(so);
			}
			break;
			case 'x':
			case 'd':
			{
				int val;

				val = va_arg(args, int);
				*(np++)=c;
				*(np)=0;
				sprintf(dstring,nstring,val);
				Append(dstring);
			}
			break;
			case 'f':
			{
				double val;

				val = va_arg(args, double);
				*(np++)=c;
				*(np)=0;
				sprintf(dstring,nstring,val);
				Append(dstring);
			}
			break;
			default:
				assert(false,"Unsupported!");
			break;
			}
			nc=0;
			sfmt=fmt;
		}
	}
	if(nc)
		Append(sfmt,nc);
}

/* put brackets around negative numbers and insert commas for every thousand */
void kGUIString::SetFormattedInt(int value)
{
	bool isneg;
	int cpos;

	/* -50  = (50) */
	/* 55 = 55 */
	/* 1234 = 1,234 */
	/* 1234567 = 1,234,567 */

	if(value>=0)
		isneg=false;
	else
	{
		value=-value;
		isneg=true;
	}
	Sprintf("%d",value);
	cpos=GetLen()-3;
	while(cpos>0)
	{
		Insert(cpos,",");
		cpos-=3;
	}
	if(isneg)
	{
		Insert(0,"(");
		Append(")");
	}
}

/* trim trailing zeros past the decimal place for numbers, for example */
/* 123.45600 becomes 123.456 */
/* 99.00 becomes 99 */

void kGUIString::TTZ(void)
{
	unsigned int oldlen=m_len;

	if(strstr(m_string,".")==0)
		return;					/* no decimal place, done! */
	
	/* eat trailing zeros */
	while(m_string[m_len-1]=='0')
		--m_len;

	/* is there now a trailing period? */
	if(m_string[m_len-1]=='.')
		--m_len;
	
	if(m_len!=oldlen)
	{
		m_string[m_len]=0;	/* put null in correct spot, done! */

		StringChanged();
	}
}

/* remove quotes and return true if there was any found */
bool kGUIString::RemoveQuotes(void)
{
	char c1;
	char c2;

	if(m_len>=2)
	{
		c1=m_string[0];
		c2=m_string[m_len-1];
		if(c1==c2)
		{
			if(c1=='\"' || c1=='\'')
			{
				Clip(m_len-1);	/* remove last char */
				Delete(0,1);	/* remove first char */
				return(true);
			}
		}
	}
	return(false);
}

/* http://en.wikipedia.org/wiki/Byte-order_mark */
//UTF-8 	EF BB BF
//UTF-16 (BE) 	FE FF
//UTF-16 (LE) 	FF FE
//UTF-32 (BE) 	00 00 FE FF
//UTF-32 (LE) 	FF FE 00 00
//SCSU 	0E FE FF
//UTF-7 	2B 2F 76, and one of the following bytes: [ 38 | 39 | 2B | 2F ]
//UTF-EBCDIC 	DD 73 66 73
//BOCU-1 	FB EE 28

typedef struct
{
	unsigned int encoding;
	unsigned int size;
	unsigned int data[4];
}ENCHEADER_DEF;

static ENCHEADER_DEF enclist[]={
	{ENCODING_UTF32BE,4,{0x00,0x00,0xfe,0xff}},
	{ENCODING_UTF32LE,4,{0xff,0xfe,0x00,0x00}},
	{ENCODING_UTF8,3,{0xef,0xbb,0xbf,0x00}},
	{ENCODING_UTF16BE,2,{0xfe,0xff}},
	{ENCODING_UTF16LE,2,{0xff,0xfe}}};

//SCSU 	0E FE FF
//UTF-7 	2B 2F 76, and one of the following bytes: [ 38 | 39 | 2B | 2F ]
//UTF-EBCDIC 	DD 73 66 73
//BOCU-1 	FB EE 28

unsigned int kGUIString::CheckBOM(unsigned int len,const unsigned char *header,int *bomsize)
{
	unsigned int i;
	unsigned int j;
	ENCHEADER_DEF *e;
	bool match;

	e=enclist;
	for(i=0;i<sizeof(enclist)/sizeof(ENCHEADER_DEF);++i)
	{
		/* if string is shorter thab BOM then it can't match! */
		if(len>=e->size)
		{
			match=true;
			for(j=0;j<(e->size) && (match==true);++j)
			{
				if(header[j]!=e->data[j])
					match=false;
			}
			if(match)
			{
				*(bomsize)=e->size;
				return(e->encoding);
			}
		}
		++e;
	}
	*(bomsize)=0;
	return(ENCODING_8BIT);
}

void kGUIString::CheckBOM(void)
{
	if(GetLen()>3)
	{
		unsigned char header[4];
		int bomsize=0;
		unsigned int encoding;

		header[0]=GetUChar(0);
		header[1]=GetUChar(1);
		header[2]=GetUChar(2);
		header[3]=GetUChar(3);
		encoding=CheckBOM(4,header,&bomsize);
		if(bomsize)
		{
			Delete(0,bomsize);
			SetEncoding(encoding);
		}
	}
}

unsigned int kGUIString::GetEncoding(const char *s)
{
	if(!stricmp(s,"UTF-8"))
		return(ENCODING_UTF8);
	else if(!stricmp(s,"ISO-8859-1"))
		return(ENCODING_8BIT);

	/* unknown/unhandled format, return 8bit */
	return(ENCODING_8BIT);
}

/* return character index */
int kGUIString::Str(const char *ss,bool matchcase,bool matchword,unsigned int offset)
{
	const char *cp;
	char c;

	assert(offset<=m_len,"Searching off of end of string!");
	while(1)
	{
		if(matchcase)
			cp=strstr(m_string+offset,ss);
		else
			cp=strstri(m_string+offset,ss);

		if(cp && matchword)
		{
			bool bad=false;
			unsigned int eoffset;

			offset=(unsigned int)(cp-m_string);
			/* if not a "word" then check again after this position */
			if(offset>0)
			{
				c=GetChar(offset-1);
				if( (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='0'))
					bad=true;
			}
			if(bad==false)
			{
				eoffset=offset+(unsigned int)strlen(ss);
				if(eoffset<GetLen())
				{
					c=GetChar(eoffset);
					if( (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='0'))
						bad=true;
				}
			}
			if(bad==false)
				break;
			offset+=(unsigned int)strlen(ss);
			if(offset==GetLen())
				return(-1);
		}
		else
			break;
	}
	if(!cp)
		return(-1);
	return((int)(cp-m_string));
}

int kGUIString::Str(kGUIString *ss,bool matchcase,bool matchword,unsigned int offset)
{
	assert(offset<=m_len,"Searching off of end of string!");
	if(GetEncoding()==ss->GetEncoding())
		return(Str(ss->GetString(),matchcase,matchword,offset));

	/* different encoding, must match encoding first */
	assert(false,"Todo!");
	return(-1);
}

/*******************************************************************************/

/* helper class for splitting strings */

kGUIStringSplit::kGUIStringSplit()
{
	m_removequotes=true;	/* default to remove quotes */
	m_trim=true;			/* default is to trim resulting words */
	m_ignoreempty=true;		/* default is to ingnore empty words */
	m_numwords=0;
	m_list.Init(16,4);
}

// ' aaa bbb    cccc  ddddd  '
// it also will group words enclosed in double quotes

unsigned int kGUIStringSplit::Split(kGUIString *s,const char *splitstring,int casemode,bool usequotes)
{
	int pass;
	unsigned int si;
	unsigned int ei;
	unsigned int srclen=s->GetLen();
	unsigned int sslen=(unsigned int)strlen(splitstring);
	unsigned char match;
	int skip;
	kGUIString *w;
	unsigned int encoding=s->GetEncoding();

	for(pass=0;pass<2;++pass)
	{
		if(pass==1)
		{
			if(!m_numwords)
				return(m_numwords);
		}
		m_numwords=0;
		si=0;
		while(si<srclen)
		{
			/* if multiple words enclosed in double quotes then don't look for split character within? */
			if(s->m_string[si]=='"' && usequotes==true)
			{
				const char *ec;

				ec=strstr(s->m_string+si+1,"\"");
				if(ec)
				{
					++si;	/* skip first quote */
					ei=(int)(ec-s->m_string);
					if(pass)
					{
						w=m_list.GetEntryPtr(m_numwords);
						w->SetEncoding(encoding);
						if(m_removequotes)
							w->SetString(s->m_string+si,ei-si);
						else
							w->SetString(s->m_string+si-1,(ei-si)+2);
						if(m_trim)
							w->Trim();
					}
					++m_numwords;
					++ei;	/* skip last quote */
					skip=0;
					goto nextword;
				}
			}
			skip=0;
			ei=si;
			while(ei<srclen)
			{
				if(casemode==0)
					match=strncmp(s->m_string+ei,splitstring,sslen);
				else
					match=strnicmp(s->m_string+ei,splitstring,sslen);
				if(!match)
				{
					skip=sslen;
					/* do we have a delimiter at the end of the string?? */
					if(m_ignoreempty==false)
					{
						if(ei+sslen==srclen)
						{
							++m_numwords;
							if(pass)
							{
								/* add empty word to end */
								w=m_list.GetEntryPtr(m_numwords-1);
								w->SetEncoding(encoding);
								w->Clear();
							}
						}
					}
					break;
				}
				++ei;
			}
			/* word goes from si to ei */
			++m_numwords;
			if(pass)
			{
				w=m_list.GetEntryPtr(m_numwords-1);
				w->SetEncoding(encoding);
				w->SetString(s->m_string+si,ei-si);
				if(m_trim)
					w->Trim();
				if(m_ignoreempty && !w->GetLen())
					--m_numwords;
			}
nextword:;
			si=ei+skip;
		}
	}
	return(m_numwords);
}

kGUIString *kGUIStringSplit::GetWord(unsigned int index)
{
	assert(index<m_numwords,"Trying to get a word past end of list!");
	return(m_list.GetEntryPtr(index));
}
