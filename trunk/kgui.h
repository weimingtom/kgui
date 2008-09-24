#ifndef __KGUI__
#define __KGUI__

/**********************************************************************************/
/* kGUI - kgui.h                                                                  */
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

/**********************************************************************************/
/*                                                                                */
/* kGUI - A generic "windows" style gui, and framework, designed to be platform   */
/*        independent.                                                            */
/*                                                                                */
/* pronunciation: "kay gooey"                                                     */
/*                                                                                */
/* Other libraries needed:  FreeType, Jpeg, PNG, zlib, mySQL,ffmpeg               */
/*                          on Linux/Mac - cups (for printing)                    */
/*                                                                                */
/*                                                                                */
/**********************************************************************************/

/*! @file kgui.h
    @brief This is the main include file for the kGUI system. It should be the
	first include file that your source files include, and there should be no need
	to include stdio.h or other system include files as this one should already include
	all of the ones you need for most cross platform code. */

#define NOLINKS 1
#define SOFTSCROLL 1
#define WINBPP 32

#if defined(LINUX)
/****************** Linux ************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define _vsnprintf vsnprintf
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define DIRCHAR "/"

#elif defined(MINGW)

/*******************************************/
/**************** Windows MinGW ************/
/*******************************************/

#define _CRT_SECURE_NO_WARNINGS
#define __MSVCRT_VERSION__ 0x0601

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <windef.h>
#include <wingdi.h>
#include <shellapi.h>
#include <winuser.h>
#include <winbase.h>
#include <math.h>

#define DIRCHAR "\\"
#undef WIN32

#elif defined(MACINTOSH)
/****************** Macintosh ************/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* the mac has a DataHandle so rename ours to avoid a conflict */
#define DataHandle kDataHandle

#define _vsnprintf vsnprintf
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define DIRCHAR "/"

#elif defined(WIN32)
/*******************************************/
/****************** Windows MFC ************/
/*******************************************/

#define DIRCHAR "\\"

#define _CRT_SECURE_NO_WARNINGS
#ifndef WINVER
#define WINVER 0x0501
#endif

#include <afxwin.h>
#define new DEBUG_NEW

#include <math.h>

/* stop windows from complaining "The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name" */
#define stricmp _stricmp
#define strnicmp _strnicmp
#define hypot _hypot

#else

/* unknown or undefined platform */
#error

#endif

#undef assert

#ifndef PI
#define PI 3.141592653589793f
#endif

#ifndef min
#define min(x,y) ((x)>(y)?(y):(x))
#endif

#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#endif

extern void fatalerror(const char *message);

inline static void assert(bool result,const char *string) {if(result==false)fatalerror(string);}
/* assert with sprintf */

#define MAXESTRING 8192

inline static void passert(bool result,const char *fmt,...) {\
if(result==false)\
{\
	char estring[MAXESTRING];\
	va_list args;\

	va_start(args, fmt);\
	_vsnprintf(estring, MAXESTRING, fmt, args);\
	va_end(args);\
	fatalerror(estring);\
}\
}

#include "hash.h"

#define TICKSPERSEC 1000
#define MAXTICK TICKSPERSEC
/* delay with no input before object hint is displayed */
#define SHOWHINTTICK (MAXTICK)
/* maximum number of nested clip areas */
#define MAXCLIPS 1024
/* maximum number of dirty areas */
#define MAXDIRTY 16
/* maximum depth of active object stack */
#define MAXACTIVE 64
/* maximum number of fonts loaded */
#define MAXFONTS 32


/* maximum delay between clicks to be called a double click */
#define DBLCLICKTIME (MAXTICK/2)

#define CALLBACKGLUE(classname , func) static void CB_ ## func(void *obj) {static_cast< classname *>(obj)->func();}
#define CALLBACKGLUEPTR(classname , func, type) static void CB_ ## func(void *obj,type *name) {static_cast< classname *>(obj)->func(name);}
#define CALLBACKGLUEPTRPTR(classname , func, type,type2) static void CB_ ## func(void *obj,type *name,type2 *name2) {static_cast< classname *>(obj)->func(name,name2);}
#define CALLBACKGLUEPTRPTRPTR(classname , func, type,type2,type3) static void CB_ ## func(void *obj,type *name,type2 *name2,type3 *name3) {static_cast< classname *>(obj)->func(name,name2,name3);}
#define CALLBACKGLUEVAL(classname , func, type) static void CB_ ## func(void *obj,type val) {static_cast< classname *>(obj)->func(val);}
#define CALLBACKGLUEVALVAL(classname , func, type, type2) static void CB_ ## func(void *obj,type val1,type2 val2) {static_cast< classname *>(obj)->func(val1,val2);}
#define CALLBACKGLUEPTRVAL(classname , func, type,type2) static void CB_ ## func(void *obj,type *name,type2 name2) {static_cast< classname *>(obj)->func(name,name2);}

/* hmm, can I reduce the callback code so I use a union or templated instead of duplicate code? */

#define CALLBACKNAME(func) CB_ ## func
#define CALLBACKCLASSNAME(classname,func) classname::CB_ ## func

/*! @class kGUIMutex
    @brief simple mutex class, it doesn't allow multiple entry from the same thread either */
class kGUIMutex
{
public:
	kGUIMutex();
	~kGUIMutex();
	/*! waits forever until it can get the mutex */
	void Lock(void);
	/*! tries to get the mutex
	    @return true=got the mutex, false=didn't get it */
	bool TryLock(void);
	/*! release the mutex */
	void UnLock(void);
private:
	/*! don't allow double entry in the same thread  !*/
	bool m_locked;
#if defined(WIN32) || defined(MINGW)
	HANDLE m_mutex;
#elif defined(LINUX) || defined(MACINTOSH)
	pthread_mutexattr_t m_attr;
	pthread_mutex_t m_mutex;
#else
#error
#endif
};

/*! Linked list template !*/
template <class T>
class Links
{
public:
	Links() {m_prev=0;m_next=0;}
	inline T *GetNext() {return m_next;}
	inline T *GetPrev() {return m_prev;}
	inline void SetNext(T *obj) {m_next=obj;}
	inline void SetPrev(T *obj) {m_prev=obj;}
	inline void Unlink(void) {T *lp=m_prev;T *ln=m_next;lp->SetNext(ln);SetNext(0);SetPrev(0);ln->SetPrev(lp);}
	inline void Link(T *head) {T *lp=head;T *ln=lp->GetNext();lp->SetNext(static_cast<T *>(this));SetPrev(lp);SetNext(ln);ln->SetPrev(static_cast<T *>(this));}
	inline bool ValidLinks(void) {if(m_prev==0 || m_next==0) return(false); return(true);}
private:
	T *m_prev;
	T *m_next;
};

template <class T,class A>
class LinkEnds
{
public:
	LinkEnds() {m_head=new A;m_tail=new A;Reset();}
	~LinkEnds() {delete m_head;delete m_tail;}
	inline void Reset(void) {m_head->SetNext(m_tail);m_tail->SetPrev(m_head);}
	inline T *GetHead() {return m_head;}
	inline T *GetFirst() {return m_head->GetNext();}
	inline T *GetLast() {return m_tail->GetPrev();}
	inline T *GetTail() {return m_tail;}
	void Delete(void);
	void Sort(void *codeobj,bool (*code)(void *o,T *obj1,T *obj2),bool rev=false);
private:
	T *m_head;
	T *m_tail;
};

/* delete all items in the list */

template <class T,class A>
void LinkEnds<T,A>::Delete(void)
{
	T *item;
	T *nextitem;
	
	item=GetFirst();
	while(item!=GetTail())
	{
		nextitem=item->GetNext();

		delete item;
		item=nextitem;
	}
	Reset();
}

template <class T,class A>
void LinkEnds<T,A>::Sort(void *codeobj,bool (*code)(void *o,T *obj1,T *obj2),bool rev)
{
	int i,j,besti;
	T **gobjptrs;
	T *gobj;
	T *nextgobj;
	int num;

	num=0;
	gobj=GetFirst();
	while(gobj!=GetTail())
	{
		++num;
		gobj=gobj->GetNext();
	}
	
	if(num<2)
		return;			/* nothing to sort */

	/* build an array of pointers */
	gobjptrs=new T *[num];

	/* fill the array */
	i=0;
	gobj=GetFirst();
	while(gobj!=GetTail())
	{
		gobjptrs[i++]=gobj;
		nextgobj=gobj->GetNext();
		gobj->Unlink();
		gobj=nextgobj;
	}

	/* now sort them */
	for(i=0;i<(num-1);++i)
	{
		besti=i;
		for(j=i+1;j<num;++j)
		{
			/* swapem if true */
			if((code)(codeobj,gobjptrs[besti],gobjptrs[j])==true)
				besti=j;
		}
		if(besti!=i)
		{
			gobj=gobjptrs[i];
			gobjptrs[i]=gobjptrs[besti];
			gobjptrs[besti]=gobj;
		}
	}

	/* now rebuld the linked list */
	for(i=0;i<num;++i)
	{
		gobj=gobjptrs[i];
		gobj->Link(GetHead());
	}

	delete []gobjptrs;
}

/*! @class Array
	@brief An Array handling template class for structures and primitives (not for classes)
    @ingroup Arrays */
template <class T>
class Array
{
public:
	Array() {m_grow=false;m_numentries=0;m_array=0;m_growsize=1;}
	~Array() {if(m_array) delete []m_array;m_array=0;}
	/*! Set the Initial size and Grow size when referencing past end !*/ 
	inline void Init(int startsize,int growsize) {Alloc(startsize);SetGrowSize(growsize);SetGrow(true);}
	/*! Enable / Disable the ability for the Array to grow !*/ 
	inline void SetGrow(bool g) {m_grow=g;}
	/*! If growing is enabled then this is the amount to grow when referenced past the end of the Array !*/ 
	inline void SetGrowSize(int g) {m_growsize=g;}
	inline bool GetGrow(void) {return m_grow;}
	inline int GetGrowSize(void) {return m_growsize;}
	/*! return the current number of allocated entries in the Array */
	inline unsigned int GetNumEntries(void) {return m_numentries;}
	inline void Alloc(unsigned int num, bool preserve=true);
	inline void SetEntry(unsigned int num, T entry);
	inline void DeleteEntry(unsigned int num);
	inline void DeleteEntry(unsigned int num,unsigned int numentries);
	inline void InsertEntry(unsigned int listsize,unsigned int num,unsigned int numentries);
	inline void Delete(T entry);
	inline T GetEntry(unsigned int num);
	inline T *GetEntryPtr(unsigned int num);
	void Sort(unsigned int num,int (*code)(const void *o1,const void *o2));
	inline T *GetArrayPtr(void) {return m_array;}
private:
	unsigned int m_numentries;
	bool m_grow;
	int m_growsize;	/* number of entries to add when list is full */
	T *m_array;
};

template <class T>
void Array<T>::Alloc(unsigned int num,bool preserve)
{
	unsigned int j;
	T *newarray;

	if(num<=m_numentries)
		return;		/* already enough space */

	/* allocate a new list */
	newarray=new T[num];
	/* initlz to zero */
	memset(newarray,0,sizeof(T)*num);

	/* if an old list exists, then copy from old list to the new one */
	if(m_array)
	{
		if(preserve==true)
		{
			j=m_numentries;
			if(j>num)
				j=num;

			memcpy(newarray,m_array,sizeof(T)*j);
		}
		delete []m_array;
	}
	m_array=newarray;
	m_numentries=num;
};

template <class T>
void Array<T>::SetEntry(unsigned int num,T entry)
{
	while(num>=m_numentries && m_grow==true)
		Alloc(m_growsize>0?num+m_growsize:num+max(1,(m_numentries>>-m_growsize)),true);

	assert((num>=0) && (num<m_numentries),"Array::SetEntry Out of bounds on array!");
	m_array[num]=entry;
};

template <class T>
void Array<T>::DeleteEntry(unsigned int num)
{
	int movesize;

	assert((num>=0) && (num<m_numentries),"Array::DeleteEntry(num) Out of bounds on array!");
	/* if deleting the last entry, then movesize=0 */
	movesize=sizeof(T)*((m_numentries-1)-num);
	if(movesize)
		memmove(m_array+num,m_array+num+1,movesize);
	/* set the last entry to zero */
	memset(m_array+m_numentries-1,0,sizeof(T));
}

template <class T>
void Array<T>::DeleteEntry(unsigned int num,unsigned int numentries)
{
	int movesize;

	assert((num>=0) && (num<m_numentries),"Array::DeleteEntry(num,numentries) Out of bounds on array!");
	/* if deleting the last entry, then movesize=0 */
	movesize=sizeof(T)*((m_numentries-numentries)-num);
	if(movesize)
		memmove(m_array+num,m_array+num+numentries,movesize);
	/* set the last entries to zero */
	memset(m_array+m_numentries-numentries,0,sizeof(T)*numentries);
}

template <class T>
void Array<T>::InsertEntry(unsigned int listsize,unsigned int num,unsigned int numentries)
{
	int movesize;
	
	while((listsize+numentries)>=m_numentries && m_grow==true)
		Alloc(m_growsize>0?listsize+m_growsize:listsize+(max(1,m_numentries>>-m_growsize)),true);

	assert((num>=0) && (num<m_numentries),"Array::InsertEntry(listsize,num,numentries) Out of bounds on array!");
	assert((listsize+numentries)<=(m_numentries),"Array::InsertEntry(listsize,num,numentries) Out of bounds on array!");

	movesize=(listsize-num)*sizeof(T);
	if(movesize)
		memmove(m_array+num+numentries,m_array+num,movesize);
	/* clear the inserted entries to zero */
	memset(m_array+num,0,sizeof(T)*numentries);
}

template <class T>
void Array<T>::Delete(T entry)
{
	unsigned int i;

	for(i=0;i<m_numentries;++i)
	{
		if(m_array[i]==entry)
		{
			DeleteEntry(i);
			return;
		}
	}
	assert(false,"Entry not found!");
}

template <class T>
T Array<T>::GetEntry(unsigned int num)
{
	passert((num>=0) && (num<m_numentries),"Array::GetEntry(%d) (numentries=%d) Out of bounds on array!",num,m_numentries);
	return(m_array[num]);
};

template <class T>
T *Array<T>::GetEntryPtr(unsigned int num)
{
	while(num>=m_numentries && m_grow==true)
		Alloc(m_growsize>0?num+m_growsize:num+(max(1,m_numentries>>-m_growsize)),true);

	assert((num>=0) && (num<m_numentries),"Array::GetEntryPtr(num) Out of bounds on array!");
	return(m_array+num);
};

template <class T>
void Array<T>::Sort(unsigned int num,int (*code)(const void *o1,const void *o2))
{
	qsort(m_array,num,sizeof(T),code);
};

/*! @class SmallArray
	@brief An Array handling template class for structures and primitives (not for classes), used for holding a small number of items
    @ingroup Arrays */
template <class T>
class SmallArray
{
public:
	SmallArray() {m_grow=false;m_numentries=0;m_array=0;m_growsize=1;}
	~SmallArray() {if(m_array) delete []m_array;m_array=0;}
	inline void Init(int startsize,int growsize) {Alloc(startsize);SetGrowSize(growsize);SetGrow(true);}
	inline void SetGrow(bool g) {m_grow=g;}
	inline void SetGrowSize(int g) {m_growsize=g;}
	inline bool GetGrow(void) {return m_grow;}
	inline int GetGrowSize(void) {return m_growsize;}
	inline unsigned int GetNumEntries(void) {return m_numentries;}
	inline void Alloc(unsigned int num, bool preserve=true);
	inline void SetEntry(unsigned int num, T entry);
	inline void DeleteEntry(unsigned int num);
	inline void DeleteEntry(unsigned int num,unsigned int numentries);
	inline void InsertEntry(unsigned int listsize,unsigned int num,unsigned int numentries);
	inline void Delete(T entry);
	inline T GetEntry(unsigned int num);
	inline T *GetEntryPtr(unsigned int num);
	void Sort(unsigned int num,int (*code)(const void *o1,const void *o2));
	inline T *GetArrayPtr(void) {return m_array;}
private:
	bool m_grow:1;
	unsigned int m_numentries:15;
	int m_growsize:15;			/* number of entries to add when list is full */
	T *m_array;
};

template <class T>
void SmallArray<T>::Alloc(unsigned int num,bool preserve)
{
	unsigned int j;
	T *newarray;

	if(num<=m_numentries)
		return;		/* already enough space */

	/* allocate a new list */
	newarray=new T[num];
	/* initlz to zero */
	memset(newarray,0,sizeof(T)*num);

	/* if an old list exists, then copy from old list to the new one */
	if(m_array)
	{
		if(preserve==true)
		{
			j=m_numentries;
			if(j>num)
				j=num;

			memcpy(newarray,m_array,sizeof(T)*j);
		}
		delete []m_array;
	}
	m_array=newarray;
	m_numentries=num;
};

template <class T>
void SmallArray<T>::SetEntry(unsigned int num,T entry)
{
	while(num>=m_numentries && m_grow==true)
		Alloc(m_growsize>0?num+m_growsize:num+(max(1,m_numentries>>-m_growsize)),true);

	assert((num>=0) && (num<m_numentries),"SmallArray::SetEntry(num,entry) Out of bounds on array!");
	m_array[num]=entry;
};

template <class T>
void SmallArray<T>::DeleteEntry(unsigned int num)
{
	int movesize;

	assert((num>=0) && (num<m_numentries),"SmallArray::DeleteEntry(num) Out of bounds on array!");
	/* if deleting the last entry, then movesize=0 */
	movesize=sizeof(T)*((m_numentries-1)-num);
	if(movesize)
		memmove(m_array+num,m_array+num+1,movesize);
	/* set the last entry to zero */
	memset(m_array+m_numentries-1,0,sizeof(T));
}

template <class T>
void SmallArray<T>::DeleteEntry(unsigned int num,unsigned int numentries)
{
	int movesize;

	assert((num>=0) && (num<m_numentries),"SmallArray::DeleteEntry(num,numentries) Out of bounds on array!");
	/* if deleting the last entry, then movesize=0 */
	movesize=sizeof(T)*((m_numentries-numentries)-num);
	if(movesize)
		memmove(m_array+num,m_array+num+numentries,movesize);
	/* set the last entries to zero */
	memset(m_array+m_numentries-numentries,0,sizeof(T)*numentries);
}

template <class T>
void SmallArray<T>::InsertEntry(unsigned int listsize,unsigned int num,unsigned int numentries)
{
	int movesize;
	
	while((listsize+numentries)>=m_numentries && m_grow==true)
		Alloc(m_growsize>0?listsize+m_growsize:listsize+(max(1,m_numentries>>-m_growsize)),true);

	assert((num>=0) && (num<m_numentries),"SmallArray::InsertEntrt(listsize,num,numentries) Out of bounds on array!");
	assert((listsize+numentries)<=(m_numentries),"SmallArray::InsertEntrt(listsize,num,numentries) Out of bounds on array!");

	movesize=(listsize-num)*sizeof(T);
	if(movesize)
		memmove(m_array+num+numentries,m_array+num,movesize);
	/* clear the inserted entries to zero */
	memset(m_array+num,0,sizeof(T)*numentries);
}

template <class T>
void SmallArray<T>::Delete(T entry)
{
	unsigned int i;

	for(i=0;i<m_numentries;++i)
	{
		if(m_array[i]==entry)
		{
			DeleteEntry(i);
			return;
		}
	}
	assert(false,"Entry not found!");
}

template <class T>
T SmallArray<T>::GetEntry(unsigned int num)
{
	assert((num>=0) && (num<m_numentries),"SmallArray::GetEntry(num) Out of bounds on array!");
	return(m_array[num]);
};

template <class T>
T *SmallArray<T>::GetEntryPtr(unsigned int num)
{
	assert((num>=0) && (num<m_numentries),"SmallArray::GetEntryPtr(num) Out of bounds on array!");
	return(m_array+num);
};

template <class T>
void SmallArray<T>::Sort(unsigned int num,int (*code)(const void *o1,const void *o2))
{
	qsort(m_array,num,sizeof(T),code);
};

/*! @class ClassArray
	@brief An Array handling template class for an array of classes
    @ingroup Arrays */
template <class T>
class ClassArray
{
public:
	ClassArray() {}
	~ClassArray();
	inline void Init(int startsize,int growsize) {m_pointers.Init(startsize,growsize);}
	inline void SetGrow(bool g) {m_pointers.SetGrow(g);}
	inline void SetGrowSize(unsigned int g) {m_pointers.SetGrowSize(g);}
	inline unsigned int GetNumEntries(void) {return m_pointers.GetNumEntries();}
	inline T *GetEntryPtr(unsigned int num);
	inline void DeleteEntry(unsigned int num);
	inline void Purge(void);
private:
	Array<T *>m_pointers;
};

template <class T>
void ClassArray<T>::Purge()
{
	unsigned int i;
	T *obj;

	for(i=0;i<m_pointers.GetNumEntries();++i)
	{
		obj=m_pointers.GetEntry(i);
		if(obj)
		{
			delete obj;
			m_pointers.SetEntry(i,0);
		}
	}
}

template <class T>
ClassArray<T>::~ClassArray()
{
	Purge();
}

template <class T>
T *ClassArray<T>::GetEntryPtr(unsigned int num)
{
	T *obj;

	if(num>=m_pointers.GetNumEntries() && m_pointers.GetGrow()==true)
		m_pointers.Alloc(num+m_pointers.GetGrowSize(),true);

	obj=m_pointers.GetEntry(num);
	if(!obj)
	{
		obj=new T();
		m_pointers.SetEntry(num,obj);
	}
	return(obj);
};

template <class T>
void ClassArray<T>::DeleteEntry(unsigned int num)
{
	T *obj;

	assert((num>=0) && (num<m_pointers.GetNumEntries()),"ClassArray::DeleteEntry(num) Out of bounds on array!");

	/* no entries to move */
	if(num==m_pointers.GetNumEntries()-1)
		return;

	/* save entry we are deleting */
	obj=m_pointers.GetEntry(num);
	/* move entries down */
	m_pointers.DeleteEntry(num);
	/* put entry at end of list */
	m_pointers.SetEntry(m_pointers.GetNumEntries()-1,obj);
}

/*! enum Keyboard codes */
enum
{
GUIKEY_UP=0x0100,
GUIKEY_DOWN,
GUIKEY_PGUP,
GUIKEY_PGDOWN,
GUIKEY_LEFT,
GUIKEY_RIGHT,
GUIKEY_BACKSPACE,
GUIKEY_DELETE,
GUIKEY_HOME,
GUIKEY_END,
GUIKEY_ESC,
GUIKEY_INSERT,
GUIKEY_TAB,
GUIKEY_SHIFTTAB,
GUIKEY_SELECTALL,
GUIKEY_F1,
GUIKEY_F2,
GUIKEY_F3,
GUIKEY_F4,
GUIKEY_F5,
GUIKEY_F6,
GUIKEY_F7,
GUIKEY_F8,
GUIKEY_F9,
GUIKEY_F10,
GUIKEY_F11,
GUIKEY_F12,
GUIKEY_COPY,
GUIKEY_PASTE,
GUIKEY_CUT,
GUIKEY_UNDO,
GUIKEY_RETURN,
GUIKEY_CTRL_PLUS,
GUIKEY_CTRL_MINUS
};

#if WINBPP==16
#define DrawColor(r,g,b) (((b&0xf8)>>3)|((g&0xf8)<<2)|((r&0xf8)<<7))
#define DrawColorToRGB(dc,r,g,b) {r=((dc>>7)&0xf8)+4;g=((dc>>2)&0xf8)+4;b=((dc<<3)&0xf8)+4;}
typedef unsigned short kGUIColor;
#else
#define DrawColor(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define DrawColorToRGB(dc,r,g,b) {r=((dc>>16)&0xff);g=((dc>>8)&0xff);b=(dc&0xff);}
typedef unsigned int kGUIColor;
#endif

typedef struct
{
	double x,y;
}kGUIDPoint2;

typedef struct
{
	int x,y;
}kGUIPoint2;

/* this is what is generated at runtime by adding the zone above to */
/* an objects parent's position */

/*! @struct kGUICorners
    @brief screen corners generated from kGUIObj zones */
typedef struct
{
	/*! left screen x */
	int lx;	
	/*! top screen y */
	int ty;
	/*! right screen x */
	int rx;
	/*! bottom screen y */
	int by;
}kGUICorners;

/*! @struct kGUIDCorners
    @brief screen corners generated from kGUIObj zones (uses doubles not ints) */
typedef struct
{
	/*! left screen x */
	double lx;	
	/*! top screen y */
	double ty;
	/*! right screen x */
	double rx;
	/*! bottom screen y */
	double by;
}kGUIDCorners;

/*! @class kGUIDelay
    @brief a simple delay class, can count up or down. Used internally for flashing
	cursor counter. */
class kGUIDelay
{
public:
	kGUIDelay() {m_left=0;}
	/*! Reset the delay back to zero */
	void Reset(void) {m_left=0;}
	/*! @return the current delay counter value */
	int Get(void) {return m_left;}
	/*! add the elapsed ticks to the delay counter */
	void Update(void);
	/*! subtract the elapsed ticks from the delay counter, if less than zero then reset to delaytime
	    @param delaytime reset counter to this when decremented below zero
		@return true=was reset to delaytime, false=still counting down */
	bool Update(int delaytime);
private:
	/*! current counter value */
	int m_left;
};

/*! @class kGUICallBack
    @brief a callback class that takes no parameters */
class kGUICallBack
{
public:
	kGUICallBack() {m_obj=0;m_func=0;}
	/*! set the callback, NULL = disable 
	    @param o object pointer
		@param f function pointer */
	void Set(void *o,void (*f)(void *));
	/*! call the callback if it is valid */
	inline void Call(void) {if(m_func) m_func(m_obj);}
	/*! call the callback if it is valid and clear it before calling so it can't be called again */
	inline void CallOnce(void) {if(m_func){void (*func)(void *);func=m_func;m_func=0;func(m_obj);}}
	/*! test if callback is valid
	    @return true=is valid, false= not valid */
	inline bool IsValid(void) {return (m_func!=0);}
	/*! compare callback 
	    @param o object pointer
		@param f function pointer
	    @return true=is a match, false= not a match */
	bool Is(void *o,void (*f)(void *)) {if((o==m_obj) && (m_func==f))return true;return false;}
private:
	/*! object pointer */
	void *m_obj;
	/*! function pointer */
	void (*m_func)(void *);
};

/*! @class kGUICallBackInt
    @brief a callback class that takes 1 integer as a parameter*/
class kGUICallBackInt
{
public:
	kGUICallBackInt() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,int));
	inline void Call(int i) {if(m_func) m_func(m_obj,i);}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,int);
};

/*! @class kGUICallBackIntInt
    @brief a callback class that takes 2 integers as a parameters */
class kGUICallBackIntInt
{
public:
	kGUICallBackIntInt() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,int,int));
	inline void Call(int i,int j) {if(m_func) m_func(m_obj,i,j);}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,int,int);
};

/*! a callback template class that takes 1 class pointer as a parameter */
template <class T>
class kGUICallBackPtr
{
public:
	kGUICallBackPtr() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,T *));
	inline void Call(T *i) {if(m_func) m_func(m_obj,i);}
	/*! call the callback if it is valid and clear it before calling so it can't be called again */
	inline void CallOnce(T *i) {if(m_func){void (*func)(void *,T *i);func=m_func;m_func=0;func(m_obj,i);}}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,T *);
};

/*! a callback template class that takes 1 class pointer as a parameter and an integer */
template <class T>
class kGUICallBackPtrInt
{
public:
	kGUICallBackPtrInt() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,T *,int));
	inline void Call(T *i, int j) {if(m_func) m_func(m_obj,i,j);}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,T *,int j);
};

/*! a callback template class that takes 2 class pointers as a parameters */
template <class T,class U>
class kGUICallBackPtrPtr
{
public:
	kGUICallBackPtrPtr() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,T *,U *));
	inline void Call(T *i, U *j) {if(m_func) m_func(m_obj,i,j);}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,T *,U *j);
};

/*! a callback template class that takes 3 class pointers as a parameters */
template <class T,class U,class V>
class kGUICallBackPtrPtrPtr
{
public:
	kGUICallBackPtrPtrPtr() {m_obj=0;m_func=0;}
	void Set(void *o,void (*f)(void *,T *,U *,V *));
	inline void Call(T *i, U *j, V *k) {if(m_func) m_func(m_obj,i,j,k);}
	inline bool IsValid(void) {return (m_func!=0);}
private:
	void *m_obj;
	void (*m_func)(void *,T *,U *j,V *k);
};

/* this needs to be down here as assert is not defined until down here */
template <class T>
void kGUICallBackPtr<T>::Set(void *o,void (*f)(void *,T *))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

template <class T>
void kGUICallBackPtrInt<T>::Set(void *o,void (*f)(void *,T *,int ))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

template <class T,class U>
void kGUICallBackPtrPtr<T,U>::Set(void *o,void (*f)(void *,T *,U *))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

template <class T,class U,class V>
void kGUICallBackPtrPtrPtr<T,U,V>::Set(void *o,void (*f)(void *,T *,U *,V *))
{
	if(o)
		assert(m_obj==0,"Callback already set!");
	m_obj=o;
	m_func=f;
}

class kGUI;

#define MAXFONTSIZE 512
/*! @class kGUIFontInfo
    @brief this contains font id, color and style info for drawing text */
class kGUIFontInfo
{
public:
	kGUIFontInfo();
	virtual ~kGUIFontInfo() {}
	/*! set the fontid
	    @param id font id */
	inline void SetFontID(int id) {m_fontid=id;FontChanged();}
	/*! set the font size
	    @param s font size in points */
	inline void SetFontSize(int s) {m_size=min(s,MAXFONTSIZE);FontChanged();}
	/*! return the font size
	    @return font size in points */
	inline const int GetFontSize(void) {return m_size;}
	/*! return the fontid
	    @return fontid */
	inline const int GetFontID(void) {return m_fontid;}
	/*! get the height of one line of text
	    @return font height in pixels */
	unsigned int GetFontHeight(void);
	/*! set the text color
	    @param c color */
	inline void SetColor(kGUIColor c) {m_color=c;FontChanged();}
	/*! return the text color
	    @return color */
	inline kGUIColor GetColor(void) {return m_color;}
	/*! copy the font info from another instance */
	inline void SetFontInfo(const kGUIFontInfo *fi,bool callchanged=true) {m_fontid=fi->m_fontid;m_size=fi->m_size;m_color=fi->m_color;FontChanged();}
private:
	/*! called if any values have changed */
	virtual void FontChanged(void) {}
	/*! fontid */
	int m_fontid;
	/*! font point size */
	int m_size;
	/*! text color */
	kGUIColor m_color;
};

#include "kguistring.h"
#include "datahandle.h"
#include "big.h"

/*! @internal @class kGUIInputLineInfo
    @brief This is a line break class used for static text and input boxes */
class kGUIInputLineInfo
{
public:
	kGUIInputLineInfo() {ty=0;by=0;startindex=0;endindex=0;pixwidth=0;pixheight=0;hardbreak=false;}
	int ty,by;					/* added to handle varying height rows */
	unsigned int startindex;	/* index into string */
	unsigned int endindex;		/* index into string */
	unsigned int pixwidth;		/* pix width */
	unsigned int pixheight;		/* pix width */
	bool hardbreak;				/* true=lines ends with c/r, false=line continues... */
};

enum
{
FT_LEFT,
FT_RIGHT,
FT_CENTER
};

enum
{
FT_TOP,
FT_MIDDLE,
FT_BOTTOM
};

/* a simple class that contains both string and draw/font information */

typedef struct
{
	unsigned int fontid;
	unsigned int fontsize;
	kGUIColor fcolor;
	kGUIColor bgcolor;
}RICHINFO_DEF;

/*! @class kGUIText
    @brief This is the text class. It it is a string with attached font info,
	line break info and it can also be in "rich text" mode */
class kGUIText : public kGUIFontInfo, public kGUIString
{
public:
	kGUIText() {m_alpha=1.0f;m_usebg=false;m_bgcolor=DrawColor(255,255,255);m_halign=FT_LEFT;m_valign=FT_TOP;m_llnum=0;m_lltotalheight=0;m_richinfosize=0;m_rstart=0;m_rend=0;m_underline=false;m_userichinfo=false;m_fixedtabs=false;m_tablist.Init(0,4);m_letterspacing=0;}
	~kGUIText();
	/* get pixel width using font and size */
	void GetSubSize(int sstart,int slen,unsigned int *pixwidth,unsigned int *pixheight);
	inline const unsigned int GetWidth(void) {unsigned int w;GetSubSize(0,GetLen(),&w,0);return w;}
	const unsigned int GetWidthSub(int sstart,int slen) {unsigned int w;GetSubSize(sstart,slen,&w,0);return w;}
	const unsigned int GetHeight(void) {return GetFontHeight();}
	const unsigned int GetAscHeight(void);	/* todo: move to fontinfo */
	const unsigned int GetDescHeight(void);	/* todo: move to fontinfo */
	void Draw(int x,int y,int w,int h);
	void Draw(int x,int y,int w,int h,kGUIColor color);
	void DrawSection(int sstart,int slen,int sx,int x,int y,int rowheight);
	void DrawSection(int sstart,int slen,int sx,int x,int y,int rowheight,kGUIColor color);
	void DrawChar(char * src, int x,int y,int w,int h,kGUIColor color);
	const unsigned int CalcFitWidth(unsigned int sstart,unsigned int slen,const unsigned int maxwidth,unsigned int *pixwidth=0);
	int CalcLineList(int w);
	int GetNumLines(void) {return m_llnum;}
	int GetLineNum(unsigned int ypos);
	int GetLineNumPix(int y);
	int GetTabWidth(int localx);

	/* these are sub-pixel anti-aliased versions that use the kGUISubPixelCollector class */
	void DrawRot(double x,double y,double angle,kGUIColor color,double alpha=1.0f) {DrawSectionRot(0,GetLen(),x,y,angle,color,alpha);}
	void DrawSectionRot(int sstart,int slen,double x,double y,double angle,kGUIColor color,double alpha=1.0f);
	void DrawChar(char * src, double x,double y,int w,int h,kGUIColor color,double apha);

	kGUIInputLineInfo *GetLineInfo(int line) {return m_linelist.GetEntry(line);}
	int CalcHeight(int width);	/* return height for a given width */
	void SetBGColor(kGUIColor c) {m_usebg=true;m_bgcolor=c;}
	void SetUseBGColor(bool b) {m_usebg=b;}
	bool GetUseBGColor(void) {return m_usebg;}
	kGUIColor GetBGColor(void) {return m_bgcolor;}

	void SetUnderline(bool u) {m_underline=u;}
	bool GetUnderline(void) {return m_underline;}

	void SetAlpha(double a) {m_alpha=a;}
	double GetAlpha(void) {return m_alpha;}

	void SetHAlign(unsigned int f) {m_halign=f;}	/* left, center, right */
	unsigned int GetHAlign(void) {return m_halign;}	/* left, center, right */
	void SetVAlign(unsigned int f) {m_valign=f;}	/* top,middle,bottom */
	unsigned int GetVAlign(void) {return m_valign;}	/* top,middle,bottom */

	/* optional: array of color info on a character by character basis */
	void SetUseRichInfo(bool b) {m_userichinfo=b;}
	bool GetUseRichInfo(void) {return m_userichinfo;}
	void InitRichInfo(void);
	void PurgeRichInfo(void) {m_richinfosize=0;}
	RICHINFO_DEF *GetRichInfoPtr(unsigned int i) {return m_richinfo.GetEntryPtr(i);}
	void SetRichFontID(unsigned int si,unsigned int ei,unsigned int fontid);
	void SetRichFontSize(unsigned int si,unsigned int ei,unsigned int fontsize);
	void SetRichFColor(unsigned int si,unsigned int ei,kGUIColor color);
	void SetRichBGColor(unsigned int si,unsigned int ei,kGUIColor color);

	void InsertRichInfo(int index,int num);
	void DeleteRichInfo(int index,int num);
	void SetRevRange(int start,int end) {m_rstart=start;m_rend=end;}

	void SetLetterSpacing(int s) {m_letterspacing=s;}

	void SetFixedTabs(bool ft) {m_fixedtabs=ft;}
	void SetTab(unsigned int index,int x) {m_tablist.SetEntry(index,x);}
private:
	Array<kGUIInputLineInfo *>m_linelist;
	SmallArray<int>m_tablist;
	int m_llnum;
	unsigned int m_lltotalheight;
	kGUIColor m_bgcolor;
	double m_alpha;
	/* reverse area section */
	int m_rstart;
	int m_rend;

	unsigned int m_halign:2;
	unsigned int m_valign:2;
	bool m_usebg:1;
	bool m_fixedtabs:1;
	/* optional color list info */
	bool m_underline:1;
	bool m_userichinfo:1;
	int m_letterspacing:16;
	int m_richinfosize;
	Array<RICHINFO_DEF> m_richinfo;
};

/* @class kGUIZone
   @brief this class is used to store a relative position and object size ( in pixels ) */
class kGUIZone
{
public:
	kGUIZone() {m_x=0;m_y=0;m_w=0;m_h=0;}
	virtual ~kGUIZone() {}

	/* set functions */
	inline void SetZoneX(int x) {if(m_x!=x){ZonePreChanged();m_x=x;ZoneChanged();}}
	inline void SetZoneY(int y) {if(m_y!=y){ZonePreChanged();m_y=y;ZoneChanged();}}
	inline void SetZoneW(int w) {if(m_w!=w){ZonePreChanged();m_w=w;ZoneChanged();}}
	inline void SetZoneH(int h) {if(m_h!=h){ZonePreChanged();m_h=h;ZoneChanged();}}
	inline void SetPos(int x,int y) {if((m_x!=x) || (m_y!=y)){ZonePreChanged();m_x=x;m_y=y;ZoneChanged();}}
	inline void SetSize(int w,int h) {if((m_w!=w) || (m_h!=h)){ZonePreChanged();m_w=w;m_h=h;ZoneChanged();}}
	inline void SetZone(int x,int y,int w,int h) {if((m_x!=x) || (m_y!=y) || (m_w!=w) || (m_h!=h)){ZonePreChanged();m_x=x;m_y=y;m_w=w;m_h=h;ZoneChanged();}}
	inline void SetZone(kGUIZone *z) {if((m_x!=z->m_x) || (m_y!=z->m_y) || (m_w!=z->m_w) || (m_h!=z->m_h)){ZonePreChanged();m_x=z->m_x;m_y=z->m_y;m_w=z->m_w;m_h=z->m_h;ZoneChanged();}}
	/* movezone is the same as SetZone but without doing the callbacks */
	/* it is used for the table object to set the rows and cells positions */
	inline void MoveZoneX(int x) {m_x=x;}
	inline void MoveZoneY(int y) {m_y=y;}
	inline void MoveZoneW(int w) {m_w=w;}
	inline void MoveZoneH(int h) {m_h=h;}
	inline void MovePos(int x,int y) {m_x=x;m_y=y;}
	inline void MoveSize(int w,int h) {m_w=w;m_h=h;}
	inline void MoveZone(int x,int y,int w,int h) {m_x=x;m_y=y;m_w=w;m_h=h;}
	inline void MoveZone(kGUIZone *z) {m_x=z->m_x;m_y=z->m_y;m_w=z->m_w;m_h=z->m_h;}

	/* get functions */
	inline int GetZoneX(void) {return m_x;}
	inline int GetZoneY(void) {return m_y;}
	inline int GetZoneW(void) {return m_w;}
	inline int GetZoneH(void) {return m_h;}
	inline int GetZoneRX(void) {return m_x+m_w;}
	inline int GetZoneBY(void) {return m_y+m_h;}
	/* make a copy */
	inline void CopyZone(kGUIZone *gzone) {gzone->m_x=m_x;gzone->m_y=m_y;gzone->m_w=m_w;gzone->m_h=m_h;}

	/* these are overrideable functions so you can detect when the zone is changed and do any */
	/* appropriate action, like redrawing the screen, recalculating line-breaks etc. */
	virtual void ZonePreChanged(void) {}
	virtual void ZoneChanged(void) {}
private:
	int m_x,m_y,m_w,m_h;
};

enum
{
EVENT_UNDEFINED,
EVENT_FOCUS,
EVENT_LOSTFOCUS,
EVENT_PRESSED,
EVENT_LEFTCLICK,
EVENT_RIGHTCLICK,
EVENT_LEFTDOUBLECLICK,
EVENT_RIGHTDOUBLECLICK,
EVENT_AFTERUPDATE,
EVENT_NOCHANGE,				/* called instead of after update if data is the same */
EVENT_ENTER,				/* item recieved focus */
EVENT_MOVED,				/* moved cursor in input box, or table, or new tab clicked for tabobj */
EVENT_SELECTED,				/* menu selection was selected, also generated by table code for combo/menu */
EVENT_CANCELLED,			/* menu selection was aborted */
EVENT_CLOSE,				/* window was closed */
EVENT_SIZECHANGED,			/* object size was changed */

/* events trigerred by tables */
EVENT_ADDROW,
EVENT_DELETEROW,
EVENT_ROW_LEFTCLICK,
EVENT_ROW_LEFTDOUBLECLICK,
EVENT_ROW_RIGHTCLICK,
EVENT_ROW_RIGHTDOUBLECLICK,
EVENT_COL_LEFTCLICK,
EVENT_COL_RIGHTCLICK,
EVENT_MOUSEOFF,				/* used for menus */

EVENT_PRESSRETURN,			/* trigerred by the input box after a change or nochange event if the user pressed return */

EVENT_NUMEVENTS
};

/*! @class kGUIEvent
    @brief This is the callback event object class, it contains the callbacks info on what
	the event was and associated values */
class kGUIEvent
{
public:
	kGUIEvent() {m_obj=0;m_event=EVENT_UNDEFINED;m_geteventcalled=false;}
	void SetObj(class kGUIObj *obj) {m_obj=obj;}
	void SetEvent(unsigned int event) {m_geteventcalled=false;m_event=event;}
	
	class kGUIObj *GetObj(void) {return m_obj;}
	unsigned int GetEvent(void) {m_geteventcalled=true;return m_event;}
	bool WasGetEventCalled(void) {return m_geteventcalled;}

	union
	{
		int i;
		unsigned int ui;
		char c;
		void *vp;
		double d;
	}m_value[4];

private:
	class kGUIObj *m_obj;
	unsigned int m_event:16;
	bool m_geteventcalled:1;
};

/* @class kGUIScroll
   @brief Sine dampened scrolling class, moves from current x,y to dest x,y */
class kGUIScroll
{
public:
	kGUIScroll() {m_attached=false;m_cx=0;m_cy=0;m_dx=0;m_dy=0;}
	~kGUIScroll();
	void Init(int x,int y) {m_cx=x;m_cy=y;m_dx=x;m_dy=y;Attach();}
	void SetCurrent(int x,int y) {m_cx=x;m_cy=y;Attach();}
	void SetCurrentX(int x) {m_cx=x;Attach();}
	void SetCurrentY(int y) {m_cy=y;Attach();}
	void SetDest(int x,int y) {m_dx=x;m_dy=y;Attach();}
	void SetDestX(int x) {m_dx=x;Attach();}
	void SetDestY(int y) {m_dy=y;Attach();}

	/* goto NOW, don't scroll */
	void Goto(int x,int y) {m_cx=m_dx=x;m_cy=m_dy=y;Attach();}
	void GotoX(int x) {m_cx=m_dx=x;Attach();}
	void GotoY(int y) {m_cy=m_dy=y;Attach();}

	int GetCurrentX(void) {return m_cx;}
	int GetCurrentY(void) {return m_cy;}
	int GetDestX(void) {return m_dx;}
	int GetDestY(void) {return m_dy;}

	/* set the event handler */
	void SetEventHandler(void *codeobj,void (*code)(void *,kGUIEvent *));

	/* call the objects attached event handler */
	void CallEvent(unsigned int event) {kGUIEvent eobj;eobj.SetObj(0);eobj.SetEvent(event);m_eventcallback.Call(&eobj);}
	void CallEvent(unsigned int event,kGUIEvent *eobj) {eobj->SetObj(0);eobj->SetEvent(event);m_eventcallback.Call(eobj);}
private:
	CALLBACKGLUE(kGUIScroll,Update);
	void Attach(void);	/* attach and detach to event queue */
	void Update(void);
	bool m_attached;	/* attached to update event callback */
	int m_cx;	/* current position */
	int m_cy;
	int m_dx;	/* current destination */
	int m_dy;
	kGUICallBackPtr<kGUIEvent> m_eventcallback;
};

/* this is the base class for all objects in the GUI system */

/* in order to reduce the number of virtual functions ( to reduce the memory footprint ) */
/* I've added a control function instead that handles many functions */

enum
{
KGCONTROL_GETISCONTAINER,
KGCONTROL_GETSKIPTAB,
KGCONTROL_GETENABLED
};

typedef struct
{
	union
	{
		bool m_bool;
	};
}KGCONTROL_DEF;

/*! @defgroup kGUIObjects kGUIObjects */ 

/*! @class kGUIObj
	@brief This is the virtual base class for all gui objects 
    @ingroup kGUIObjects */
class kGUIObj : public kGUIZone
{
public:
	kGUIObj() {m_parent=0;}
	virtual ~kGUIObj();
	/*! This is the virtual draw function */
	virtual void Draw(void)=0;
	/*! This is the virtual update input function */
	virtual bool UpdateInput(void)=0;
	/*! This is the generic virtual function used to get info about the object */
	virtual void Control(unsigned int command,KGCONTROL_DEF *data);

	/* this is called on objects by the table draw code to reposition */
	/* cells without the callbacks and dirty calls that the regular one has. */

	/*! this is called before an objects zone position changes */
	void ZonePreChanged(void) {Dirty();}
	/*! this is called after an objects zone position changes */
	void ZoneChanged(void) {Dirty();}

	/*! calculate 4 corner positions by traversing the parent links as objects zone positions are relative to each of it's parents */
	void GetCorners(kGUICorners *c);
	/*! static callback handler for the Dirty function */
	CALLBACKGLUE(kGUIObj,Dirty)
	/*! flag the position occupied by the object as dirty so it is re-drawn */
	void Dirty(void);
	/*! flag the position occupied by the corners passed as dirty so it is re-drawn */
	void Dirty(const kGUICorners *c);

	/*! assign this object a parent
	    @param p its parent container object */
	inline void SetParent(class kGUIContainerObj *p) {m_parent=p;}
	/*! return this objects parent container object
	    @return return it's parent container object */
	inline class kGUIContainerObj *GetParent(void) {return m_parent;}

	/*! return if this object is a container or a not
	    @return true = is a container, false = not a container */
	bool IsContainer(void) {KGCONTROL_DEF c;Control(KGCONTROL_GETISCONTAINER,&c);return c.m_bool;}
	/*! return if this object should be skipped when pressing tab
	    @return true = skip it, false = don't skip */
	bool SkipTab(void) {KGCONTROL_DEF c;Control(KGCONTROL_GETSKIPTAB,&c);return c.m_bool;}
	/*! return if this object is enabled or disabled
	    @return true = is enabled, false = is disabled */
	bool GetEnabled(void) {KGCONTROL_DEF c;Control(KGCONTROL_GETENABLED,&c);return c.m_bool;}

	/*! set this object as the currently active gui object, this also sets each of 
	   it's parents as active as well */
	void SetCurrent(void);
	/*! return if this is the current active object 
	   @return true = I am the current object, false = not the current object */
	bool ImCurrent(void);

	/*! set the event handler callback for this object
	    @param codeobj pointer to the callback object
		@param code pointer to the callback function */
	void SetEventHandler(void *codeobj,void (*code)(void *,kGUIEvent *));

	/*! call the objects attached event handler and pass the event 
	    @param event event index */
	void CallEvent(unsigned int event) {kGUIEvent eobj;eobj.SetObj(this);eobj.SetEvent(event);m_eventcallback.Call(&eobj);}
	/*! call the objects attached event handler and pass the event structure supplied 
	    @param event event index
	    @param eobj event object */
	void CallEvent(unsigned int event,kGUIEvent *eobj) {eobj->SetObj(this);eobj->SetEvent(event);m_eventcallback.Call(eobj);}
private:
	/*! pointer to the objects parent container object */
	class kGUIContainerObj *m_parent;
	/*! event handler callback for this object */
	kGUICallBackPtr<kGUIEvent> m_eventcallback;
};

/*! @class kGUITextObj
	@brief this is the static text gui object
    @ingroup kGUIObjects */
class kGUITextObj : public kGUIObj, public kGUIText
{
public:
	kGUITextObj() {m_xoff=3;m_yoff=3;}

	void Control(unsigned int command,KGCONTROL_DEF *data);
	void Draw(void);
	bool UpdateInput(void);
	void ShrinktoFit(void);	/* decrement font size until text fits (only if it is too long already) */

	/* override the string and text changed callbacks */
	void StringChanged(void) {Changed();}
	void FontChanged(void) {Changed();}
	void SetOffsets(int xoff,int yoff) {m_xoff=xoff;m_yoff=yoff;Changed();}
private:
	void Changed(void);
	unsigned int m_xoff:16;
	unsigned int m_yoff:16;
};

/*! @class kGUIRectObj
	@brief this is the rectangle gui object
    @ingroup kGUIObjects */
class kGUIRectObj : public kGUIObj
{
public:
	kGUIRectObj() {m_color=DrawColor(0,0,0);}
	void Draw(void);
	bool UpdateInput(void) {return true;}
	void SetColor(kGUIColor c) {m_color=c;}
	kGUIColor GetColor(void) {return (m_color);}
private:
	kGUIColor m_color;
};

enum
{
GUISHAPE_RAW,
GUISHAPE_GIF,
GUISHAPE_JPG,
GUISHAPE_PNG,
GUISHAPE_SURFACE,
GUISHAPE_UNDEFINED
};

typedef struct
{
	const unsigned char *limage;
	int rowadd;
	double xfrac;
	double yfrac;
	double pixelwidth;
	double pixelheight;
	double pixelscale;
	int rgba[4];
}SUBPIXEL_DEF;

#include "kguithread.h"

/*! @class kGUIImage
    @brief This is the image class. It currently handles: Jpeg, Gif, Animated Gif,
	PNG, ICO and BMP formats */
class kGUIImage  : public Links<class kGUIImage>, public DataHandle
{
public:
	static void InitCache(int maxloaded);
	kGUIImage();
	virtual ~kGUIImage();
	virtual void ImageChanged(void) {}
	virtual void Purge(void);
	virtual void HandleChanged(void);

	/* point to a memory based image, used for drawing a display image */
	void SetMemImage(unsigned int frame,int format,int width,int height,unsigned int bpp,const unsigned char *data);
	void SetMemImageCopy(unsigned int frame,int format,int width,int height,unsigned int bpp,const unsigned char *data);

	double GetScaledImageWidth(void) {return ((double)GetImageWidth())*GetScaleX();} 
	double GetScaledImageHeight(void) {return ((double)GetImageHeight())*GetScaleY();} 

	void SetRaw(unsigned char *raw);
	bool Draw(int frame,int x1,int y1);
	bool DrawAlpha(int frame,int x1,int y1,double alpha);
	bool TileDraw(int frame,int x1,int y1,int x2,int y2);
	void DrawLineRect(int frame,int lx,int ty,int rx,int by,bool horiz);
	void ScaleTo(int w,int h) {SetScale(m_imagewidth==0?1.0f:(double)w/(double)m_imagewidth,m_imageheight==0?1.0f:(double)h/(double)m_imageheight);}
	void SetScale(double xscale,double yscale) {m_stepx=1.0f/xscale;m_stepy=1.0f/yscale;}
	double GetScaleX(void) {return 1.0f/m_stepx;}
	double GetScaleY(void) {return 1.0f/m_stepy;}
	int GetImageHeight(void) {return m_imageheight;}
	int GetImageWidth(void) {return m_imagewidth;}
	void ReadPixel(const unsigned char *ptr,unsigned char *r,unsigned char *g,unsigned char *b,unsigned char *a);

	void SetImageHeight(int h) {m_imageheight=h;}
	void SetImageWidth(int w) {m_imagewidth=w;}
	void SetLocked(bool l);
		
	inline bool IsValid(void) {if((!m_imagewidth) || (!m_imageheight))return(false);return(true);}
	void CopyImage(kGUIImage *image);
	void GreyImage(int frame);
	bool SaveJPGImage(const char *filename,int quality);
	bool LoadPixels(void);

	unsigned int GetNumFrames(void) {return m_numframes;}
	void SetDelay(int frame,unsigned int delay) {m_delays.SetEntry(frame,delay);}
	unsigned int GetDelay(int frame) {return m_delays.GetEntry(frame);}
	void AsyncLoadPixels(void);
	bool GetAsyncActive(void) {return m_thread.GetActive();/*m_asyncactive;*/}

	void SetBad(bool b) {m_bad=b;}
	bool GetBad(void) {return m_bad;}

	unsigned int GetImageType(void) {return m_imagetype;}
	unsigned char *GetImageDataPtr(unsigned int frame) {return m_imagedata.GetEntry(frame);}
private:
	CALLBACKGLUE(kGUIImage,DoAsyncLoad)
	/* these are used for the MAC PowerPC reading in WINICO format, endian issues */
	unsigned short ReadU16(const char *fp);
	unsigned int ReadU24(const char *fp);
	unsigned int ReadU32(const char *fp);
	int Read32(const char *fp);
	int Read24(const char *fp);
	short Read16(const char *fp);

	void DoAsyncLoad(void);
	void LoadImage(bool justsize=false);	/* justsize=just tell me the size, don't load */
	bool LoadJPGImage(bool justsize);
	bool LoadGIFImage(bool justsize);
	bool LoadPNGImage(bool justsize);
	bool LoadWINICOImage(bool justsize);
	bool LoadBMPImage(bool justsize);
	static LinkEnds<class kGUIImage,class kGUIImage> m_loadedends;
	static LinkEnds<class kGUIImage,class kGUIImage> m_lockedandloadedends;
	static LinkEnds<class kGUIImage,class kGUIImage> m_unloadedends;
	static int m_numloaded;
	static int m_maxloaded;

	bool m_locked:1;
	bool m_memimage:1;
	bool m_allocmemimage:1;
	unsigned int m_imagetype:3;
	unsigned int m_bpp:3;

	kGUIThread m_thread;		/* only used for async loading */
	bool m_bad:1;

	double m_stepx,m_stepy;

	unsigned int m_imagewidth;
	unsigned int m_imageheight;

	unsigned int m_numframes;
	Array<unsigned char *>m_imagedata;	
	Array<unsigned int>m_delays;			
};

#define BUTTONTEXTEDGE 3

/*! @class kGUIButtonObj
	@brief this is the button gui object
    @ingroup kGUIObjects */
class kGUIButtonObj : public kGUIObj, public kGUIText
{
public:
	kGUIButtonObj();
	~kGUIButtonObj();

	void Control(unsigned int command,KGCONTROL_DEF *data);
	void Draw(void);
	bool UpdateInput(void);
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}

	inline void SetSize(int w,int h) {kGUIObj::SetSize(w,h);Changed();}
	void SetImage(kGUIImage *image) {m_image=image;MakeDisabledImage(image);Dirty();}
	void SetFrame(bool f) {m_frame=f;}
	virtual void SetEnabled(bool e) {if(m_enabled!=e) {m_enabled=e;Dirty();}}
	void Enable(void) {SetEnabled(true);}
	void Disable(void) {SetEnabled(false);}
	void SetShowCurrent(bool c) {m_showcurrent=c;}

	/* make button just big enough to hold text or image */
	void Contain(void);

	/* override the string and text changed callbacks */
	void StringChanged(void) {Changed();}
	void FontChanged(void) {Changed();}
private:
	void Changed(void) {CalcLineList(GetZoneW()-(BUTTONTEXTEDGE<<1));Dirty();}
	void MakeDisabledImage(kGUIImage *image);
	kGUIString *m_hint;
	kGUIImage *m_image;
	kGUIImage *m_disabledimage;
	bool m_pushover:1;
	bool m_enabled:1;
	bool m_pressed:1;
	bool m_frame:1;			/* draw button frame? */
	bool m_showcurrent:1;	/* show it is current by drawing a checkered outline on it? */
};

/*! @class kGUITickBoxObj
	@brief this is the tickbox (or checkbox) gui object
    @ingroup kGUIObjects */
class kGUITickBoxObj : public kGUIObj
{
public:
	kGUITickBoxObj();
	~kGUITickBoxObj();
	void SetSelected(bool s) {m_selected=s;Dirty();}
	void SetScale(bool s) {m_scale=s;}
	void Draw(void);
	void Click(void);	/* called by user code when clicking on an accompanying pabel */
	bool UpdateInput(void);
	bool GetSelected(void) {return m_selected;}
	void SetLocked(bool l) {m_locked=l;}
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
private:
	void CallAfterUpdate(void);
	bool m_selected:1;
	bool m_locked:1;
	bool m_scale:1;
	kGUIString *m_hint;
};

/*! @class kGUIRadioObj
	@brief this is the radio gui object
    @ingroup kGUIObjects */
class kGUIRadioObj : public kGUIObj
{
public:
	kGUIRadioObj();
	~kGUIRadioObj();
	void SetSelected(bool s) {m_selected=s;Dirty();}
	void SetScale(bool s) {m_scale=s;}
	void Draw(void);
	bool UpdateInput(void);
	bool GetSelected(void) {return m_selected;}
	void SetLocked(bool l) {m_locked=l;}
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
private:
	void CallAfterUpdate(void);
	kGUIString *m_hint;
	bool m_selected:1;
	bool m_locked:1;
	bool m_scale:1;
};


/*! @class kGUIDividerObj
	@brief this is the moveable divider gui object, it is used for making the items above it and below it
    (or left and right of it) larger or smaller by dragging it back and forth
	@ingroup kGUIObjects */
class kGUIDividerObj : public kGUIObj
{
public:
	void Draw(void);
	bool UpdateInput(void);
private:
};

enum
{
SCROLLMODE_CLICK,
SCROLLMODE_ADJUST
};

/*! @class kGUIScrollBarObj
    @brief this is the scrollbar object, it can be either vertical or horizontal
	@ingroup kGUIObjects */
class kGUIScrollBarObj : public kGUIObj
{
public:
	kGUIScrollBarObj();
	void Draw();
	inline bool IsActive(void) {return m_active;}
	inline void SetHorz(void) {m_isvert=false;}
	inline void SetVert(void) {m_isvert=true;}
	bool UpdateInput();
	void SetShowEnds(bool e) {m_showends=e;}
	void SetFixedThumb(int w) {m_fixed=w;}
	inline void SetValues(int above,int shown,int below) {m_numabove=above;m_numshown=shown;m_numbelow=below;}
	void SetClickSize(int e) {m_endclick=e;}
private:
	void CalcBarPos(const kGUICorners *c,kGUICorners *sl);	/* recalcs barlength, top & bot offsets */
	int m_fixed:16;
	bool m_isvert:1;
	bool m_active:1;
	bool m_showends:1;
	unsigned int m_mode:1;	/* ??? */
	int m_endclick;		/* move value to return when user clicks on end up/down buttons, defaults to 1 */
	/* this is designed this way to work properly with tables that have differing */
	/* row heights, so numshown can vary up and down as the table is scrolling by */
	int m_numabove;
	int m_numshown;
	int m_numbelow;
	/* these are the calculated variables used for drawing and input */
	int m_barlength;
	int m_topoffset;
	int m_botoffset;
};

/*! @class kGUIImageObj
    @brief This is an image gui object, it can be static or animated and can also be a different size
    than the attached image if desired.
	@ingroup kGUIObjects */
class kGUIImageObj : public kGUIObj, public kGUIImage
{
public:
	kGUIImageObj();
	~kGUIImageObj();
	void ImageChanged(void) {m_currentframe=0;m_animdelay=0;Dirty();}
	void SetScale(double xscale,double yscale) {kGUIImage::SetScale(xscale,yscale);UpdateScrollbars();}
	void SetXOffset(int x) {m_leftoff=x;}
	void SetYOffset(int y) {m_topoff=y;}
	int GetXOffset(void) {return m_leftoff;}
	int GetYOffset(void) {return m_topoff;}
	double CalcScaleToFit(int w,int h);
	void ShrinkToFit(void);		/* only will make smaller, not bigger */
	void ExpandToFit(void);		/* only will make bigger, not smaller */
	void ScaleToFit(void);		/* will make either bigger or smaller */
	void CenterImage(void);
	void Draw(void);
	void TileDraw(void);
	bool UpdateInput(void);
	void SetShowScrollBars(bool s);
	void SetAnimate(bool a) {m_animdelay=0;m_animate=a;Dirty();}
	void SetCurrentFrame(unsigned int f) {if(f==m_currentframe)return;LoadPixels();assert(f<GetNumFrames(),"Error!");m_currentframe=f;Dirty();}
	unsigned int GetCurrentFrame(void) {return  m_currentframe;}
	void SetAlpha(double a) {m_alpha=a;}

	void SetDrawModeScale(void);
	void SetDrawModeCrop(void);
	void UpdateScrollbars(void);
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
private:
	CALLBACKGLUEPTR(kGUIImageObj,ScrollMoveRow,kGUIEvent)
	CALLBACKGLUEPTR(kGUIImageObj,ScrollMoveCol,kGUIEvent)
	void ScrollMoveRow(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveRow(event->m_value[0].i);}
	void ScrollMoveCol(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveCol(event->m_value[0].i);}
	CALLBACKGLUE(kGUIImageObj,Animate)
	void Animate(void);						/* animate callback */

	void MoveCol(int row);	/* move cursor col */
	void MoveRow(int row);	/* move cursor row */

	double m_alpha;
	int m_leftoff;
	int m_topoff;
	bool m_animate:1;
	bool m_animateeventactive:1;
	bool m_showscrollbars:1;
	unsigned int m_animdelay;
	unsigned int m_currentframe;
	kGUIScrollBarObj *m_hscrollbar;
	kGUIScrollBarObj *m_vscrollbar;
	kGUIString *m_hint;
};

/*! @class kGUIImageRefObj
    @brief This is an image reference gui object, it can be static or animated and can also be a different size
    than the referenced image if desired, it contains a reference to an image so many of these can share a common image.
	useful for tables or webpages where the same image can be drawn many times over and over
	@ingroup kGUIObjects */
class kGUIImageRefObj : public kGUIObj
{
public:
	kGUIImageRefObj() {m_currentframe=0;m_animate=false;m_animateeventactive=false;m_animdelay=0;m_image=0;m_scalex=1.0f;m_scaley=1.0f;m_alpha=1.0f;m_leftoff=0;m_topoff=0;}
	~kGUIImageRefObj();
	kGUIImage *GetImage(void) {return m_image;}
	void SetImage(kGUIImage *image) {if(m_image==image) return;m_image=image;m_currentframe=0;m_animdelay=0;Dirty();}
	void SetScale(double xscale,double yscale) {m_scalex=xscale;m_scaley=yscale;Dirty();}
	double GetScaleX(void) {return m_scalex;}
	double GetScaleY(void) {return m_scaley;}
	void SetAlpha(double a) {m_alpha=a;Dirty();}
	void SetXOffset(int l) {m_leftoff=l;}
	void SetYOffset(int t) {m_topoff=t;}
	int GetXOffset(void) {return m_leftoff;}
	int GetYOffset(void) {return m_topoff;}

	unsigned int GetNumFrames(void) {return m_image->GetNumFrames();}
	void SetAnimate(bool a) {m_animate=a;Dirty();}
	void SetCurrentFrame(unsigned int f) {m_currentframe=f;Dirty();}
	unsigned int GetCurrentFrame(void) {return  m_currentframe;}

	void SetBad(bool b) {m_image->SetBad(b);}
	bool GetBad(void) {return m_image->GetBad();}
	void Draw(void);
	void TileDraw(void);
	bool UpdateInput(void);
private:
	CALLBACKGLUE(kGUIImageRefObj,Animate)
	void Animate(void);		/* animate callback */
	kGUIImage *m_image;
	bool m_animate:1;
	bool m_animateeventactive:1;
	unsigned int m_animdelay;
	unsigned int m_currentframe;
	double m_alpha;
	double m_scalex,m_scaley;
	int m_leftoff;
	int m_topoff;
};

enum
{
GUIINPUTTYPE_STRING,
GUIINPUTTYPE_INT,
GUIINPUTTYPE_DOUBLE
};

/*! @class kGUIInputBoxObj
	@brief this is an inputbox gui object, it can be a single line or multiple lines with scrollbars.
    it can contain plain text with a single font, color and point size, or it can contain rich
	text where each character can be a different font/color/size. It can be in password mode where
	charcters are shown as asterisks.
	@ingroup kGUIObjects */
class kGUIInputBoxObj : public kGUIObj, public kGUIText
{
public:
	kGUIInputBoxObj();
	~kGUIInputBoxObj();

	void Control(unsigned int command,KGCONTROL_DEF *data);
	void Draw(void);
	void Activate(void);
	void DeActivate(void);
	bool UpdateInput(void);
	void SetMaxLen(int m) {m_maxlen=m;}
	unsigned int GetCursorPos(void) {return m_hcursor;}
	void GetCursorRange(unsigned int *si,unsigned int *ei);
	void Select(int start,int end) {m_hstart=start;m_hcursor=end;m_hrange=true;PutCursorOnScreen();Dirty();}

	void SetPassword(bool pw) {m_password=pw;}

	/* normally these keys exit the box, but if enabled they can actually */
	/* be entered into the string and focus stays on the box unless clicked off */
	void SetAllowEnter(bool b) {m_allowenter=b;}
	void SetAllowTab(bool b) {m_allowtab=b;}
	void SetAllowCursorExit(bool b) {m_allowcursorexit=b;}
	void SetInt(int v);
	void SetLeaveSelection(bool l) {m_leaveselection=l;}
	void SetAllowUndo(bool u) {m_allowundo=u;}
	void SetLeaveScroll(bool l) {m_leavescroll=l;}

	void SetInt(const char *v);
	void SetDouble(const char *f,const char *v);

	void SetInt(const char *f,int v);
	void SetDouble(const char *f,double v);

	const char *GetValueString(void);
	void SetWrap(bool w) {m_wrap=w;}
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}

	void SetLocked(bool l) {m_locked=l;}

	/* override the string and text changed callbacks */
	void StringChanged(void) {Changed();}
	void FontChanged(void) {Changed();}
	/* this can be manually called by users code after changing the string to trigger the */
	/* parent objects callback being called too */
	void CallAfterUpdate(void);

	void SetShowCommas(bool c) {m_showcommas=c;}

	void SetInputType(unsigned int it) {m_inputtype=it;}

	void SelectAll(void) {m_hstart=0;m_hcursor=GetLen();m_hrange=true;Dirty();}

	/* only 8 bits to max 255 pixels, if more is needed then change bitsize on m_xoff */
	void SetXOffset(unsigned int xoff) {m_xoff=xoff;}

	bool MoveCursorRow(int delta);
private:
	CALLBACKGLUEPTR(kGUIInputBoxObj,ScrollMoveRow,kGUIEvent)
	CALLBACKGLUEPTR(kGUIInputBoxObj,ScrollMoveCol,kGUIEvent)
	CALLBACKGLUEPTR(kGUIInputBoxObj,ScrollEvent,kGUIEvent)
	void ScrollMoveRow(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveRow(event->m_value[0].i);}
	void ScrollMoveCol(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveCol(event->m_value[0].i);}
	void ScrollEvent(kGUIEvent *event) {if(event->GetEvent()==EVENT_MOVED)Dirty();}
	void Changed(void);
	void PutCursorOnScreen(void);
	void MoveRow(int delta);
	void MoveCol(int delta);
	void CalcLines(bool full);
	void DeleteSelection(void);
	void CallDoubleClick(void) {CallEvent(EVENT_LEFTDOUBLECLICK);}
	void CallRightClick(void) {CallEvent(EVENT_RIGHTCLICK);}
	bool CheckInput(int key);
	void CalcViewLines(void);

	bool m_allowenter:1;
	bool m_allowtab:1;
	bool m_allowcursorexit:1;
	bool m_leaveselection:1;
	bool m_leavescroll:1;
	bool m_usevs:1;
	bool m_usehs:1;
	bool m_recalclines:1;
	bool m_hrange:1;
	bool m_locked:1;
	bool m_wrap:1;
	bool m_allowundo:1;
	bool m_password:1;
	unsigned int m_valuetype:2;
	unsigned int m_inputtype:2;
	bool m_showcommas:1;
	unsigned int m_xoff:8;			/* allow text to be offset from edge so we can draw out own stuff on the left */

	int m_maxlen;
	unsigned int m_maxw;	/* longest line in pixels */
	int m_numviewlines;		/* number of lines in input box */
	int m_numfullviewlines;	/* number of full height lines in input box */
	/* hilight characters between hstart and hcursor if hrange=true */
	int m_topoff;	/* lines */
	int m_leftoff;	/* pixels */
	unsigned int m_hcursor;
	unsigned int m_hstart;
	kGUIDelay m_hdelay;			/* delay to slow down mouse scrolling */
	kGUIString *m_hint;
	kGUIString *m_undotext;
	kGUIString *m_valuetext;		/* value of text with different precision than displayed */

	short m_ow,m_oh;		/* old width/height */

	/* only valid when box is activated */
	kGUIScrollBarObj *m_vscrollbar;
	kGUIScrollBarObj *m_hscrollbar;
	kGUIScroll *m_scroll;
};

/*! @class kGUIScrollTextObj
	@brief this is an scrolltext gui object. It is essentially static text with left and right
    scroll buttons on the ends and when the buttons are pressed the attached callback
	can change the text contents.
	@ingroup kGUIObjects */
class kGUIScrollTextObj : public kGUITextObj
{
public:
	kGUIScrollTextObj();
	~kGUIScrollTextObj() {}
	void Draw(void);
	bool UpdateInput(void);
private:
};

class kGUITableObj;
class kGUIComboTableRowObj;

/* combo pulldown box */

/* when using a table for the popup area, it must select */
/* no row buttons, no col buttons, no col scroller and */
/* "selectreturn" mode where a click or a enter will close the */
/* table and return "active" status to it's parent, the combo box */

enum
{
COMBOTYPE_NUM,
COMBOTYPE_STRING
};

/*! @class kGUIComboBoxObj
	@brief this is an combobox gui object. 
	@ingroup kGUIObjects */
class kGUIComboBoxObj : public kGUIObj, public kGUIFontInfo
{
public:
	kGUIComboBoxObj();
	~kGUIComboBoxObj();

	void SetNumEntries(unsigned int n);
	unsigned int GetNumEntries(void) {return m_numentries;}
	void SetAllowTyping(bool t) {m_allowtyping=t;}
	void SetEntry(unsigned int index,const char *entryname,int entryval);
	void SetEntry(unsigned int index,const char *entryname,const char *entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,int entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval);
	void RenameEntry(const char *oldname,const char *newname);
	void SetLocked(bool l) {m_locked=l;Dirty();}
	int GetWidest(void);
	void Draw(void);
	bool UpdateInput(void);
	kGUIText *GetEntryTextPtr(unsigned int index);
	void SetSelection(int s);
	bool SetSelectionz(int s);
	void SetSelection(const char *string);			/* uses the viewable name */
	bool SetSelectionz(const char *string);			/* uses the viewable name */
	void SetSelectionString(const char *string);	/* uses the hidden string name */
	bool SetSelectionStringz(const char *string);	/* uses the hidden string name */
	unsigned int GetType(void) {return m_type;}
	int GetSelection(void);
	kGUIString *GetSelectionStringObj(void);
	const char *GetSelectionString(void) {return GetSelectionStringObj()->GetString();}
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetColorMode(unsigned int width);
	void SetColorBox(unsigned int index,kGUIColor c);
private:
	CALLBACKGLUEPTR(kGUIComboBoxObj,SelectionDone,kGUIEvent)
	void SelectionDone(kGUIEvent *e);
	void DrawPopUp(void);
	unsigned int m_colorcolwidth;

	bool m_allowtyping:1;
	bool m_locked:1;
	bool m_colormode:1;
	bool m_popped:1;		/* selector list is currently shown */
	unsigned int m_editmode:2;
	unsigned int m_type:1;
	unsigned int m_numentries;
	unsigned int m_undoselection;
	unsigned int m_selection;
	kGUITableObj *m_poptable;			/* use a table for the popup selector */
	kGUIString *m_hint;
	kGUIString *m_typedstring;			/* user typed string is stored here */
	kGUIComboTableRowObj **m_poptableentries;
};

/*! @internal @class kGUISharedComboEntries
    @brief The shared combo entries are designed to be used in tables where
    many rows all share the same combo and can therefore reduce the
    memory footprint by sharing the combo entries and data */
class kGUISharedComboEntries : public kGUIFontInfo
{
public:
	kGUISharedComboEntries();
	virtual ~kGUISharedComboEntries();

	void SetNumEntries(unsigned int n);
	unsigned int GetNumEntries(void) {return m_numentries;}

	void SetEntry(unsigned int index,const char *entryname,int entryval);
	void SetEntry(unsigned int index,const char *entryname,const char *entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,int entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval);
	void RenameEntry(const char *oldname,const char *newname);
	int GetWidest(void);
	kGUIText *GetEntryTextPtr(unsigned int index);

	void SetType(unsigned int type) {m_type=type;}
	unsigned int GetType(void) {return m_type;}

	void SetAllowTyping(bool t) {m_allowtyping=t;}
	bool GetAllowTyping(void) {return m_allowtyping;}

	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	kGUIString *GetHint(void) {return m_hint;}

	void SetColorMode(unsigned int width);
	bool GetColorMode(void) {return m_colormode;}
	unsigned int GetColorWidth(void) {return m_colorcolwidth;}

	void SetColorBox(unsigned int index,kGUIColor c);
	kGUIComboTableRowObj *GetRow(unsigned int index) {return m_poptableentries[index];}

	void SetTable(kGUITableObj *t) {m_poptable=t;}
	kGUITableObj *GetTable(void) {return m_poptable;}
	void CloseTable(void);

	bool IsValid(void) {return (m_poptableentries!=0 && m_numentries);}
private:
	unsigned int m_colorcolwidth;

	bool m_allowtyping:1;
	bool m_colormode:1;
	bool m_popped:1;		/* selector list is currently shown */
	unsigned int m_type:1;

	unsigned int m_numentries;
	unsigned int m_undoselection;
	unsigned int m_selection;
	kGUITableObj *m_poptable;			/* use a table for the popup selector */
	kGUIString *m_hint;
	kGUIComboTableRowObj **m_poptableentries;
};

/*! @class kGUISharedComboboxObj
	@brief this is an shared combobox gui object. It references externally shared entries. It is useful
    for a combo box that is replicated in a table over and over and therefore takes a lot
	less memory than having it's own entries.
	@ingroup kGUIObjects */
class kGUISharedComboboxObj : public kGUIObj
{
public:
	kGUISharedComboboxObj();
	~kGUISharedComboboxObj() {}

	void SetSharedEntries(kGUISharedComboEntries *shared) {m_shared=shared;}
	void SetLocked(bool l) {m_locked=l;Dirty();}
	void Draw(void);
	bool UpdateInput(void);
	void SetSelection(int s);
	bool SetSelectionz(int s);
	void SetSelection(const char *string);
	bool SetSelectionz(const char *string);
	int GetSelection(void);
	kGUIString *GetSelectionStringObj(void);
	const char *GetSelectionString(void) {return GetSelectionStringObj()->GetString();}
private:
	CALLBACKGLUEPTR(kGUISharedComboboxObj,SelectionDone,kGUIEvent)
	void SelectionDone(kGUIEvent *e);
	void DrawPopUp(void);
	bool m_locked:1;
	bool m_popped:1;		/* selector list is currently shown */
	unsigned int m_editmode:2;
	unsigned int m_undoselection;
	unsigned int m_selection;

	kGUIString *m_typedstring;			/* user typed string is stored here */
	kGUISharedComboEntries *m_shared;
};


class kGUIMenuEntryObj;

/* when using a table for the popup area, it must select */
/* no row buttons, no col buttons, no col scroller, no row scroller and */
/* "selectreturn" mode where a click or a enter will close the */
/* table and return "active" status to it's parent, the menucol box */

/*! @class kGUIMenuColObj
	@brief this is a popup menu gui object. Since it is a popup menu it only has one list
	and is not heiarchical.
	@ingroup kGUIObjects */
class kGUIMenuColObj : public kGUIObj
{
public:
	kGUIMenuColObj();
	~kGUIMenuColObj();

	void Init(int num);
	void Init(int num, const char **strings);
	void Init(int num, const char **strings, int *nums);
	void Init(int num, kGUIString *strings, int *nums);
	void Activate(int x,int y);	/* screen position to pop up at */
	void ReActivate(void);
	void SetIconWidth(int w) {m_iconwidth=w;}
	void SetNumEntries(int n);
	void SetEntry(int index,kGUIString *entryname,int entryval=-1);
	void SetEntry(int index,const char *entryname,int entryval=-1);
	void SetEntryEnable(int entryval,bool e,bool updatecolor=true);
	kGUIMenuEntryObj *GetEntry(int index) {return m_poptableentries.GetEntryPtr(index);}
	kGUIMenuEntryObj *GetCurrentEntry(void);
	int GetWidest(void);
	void Draw(void) {};
	bool UpdateInput(void);
	void SetFontID(int id) {m_fontinfo.SetFontID(id);Resize();}
	void SetFontSize(int s) {m_fontinfo.SetFontSize(s);Resize();}
	void SetFontColor(kGUIColor c) {m_fontinfo.SetColor(c);}
	void SetBGColor(int index,kGUIColor bg);
	void SetSelection(int s) {m_selection=s;Dirty();}
	void Close(void);
	void SetDrawPopRow(bool d) {m_drawpoprow=d;}

	int GetSelection(void);
	const char *GetSelectionString(void);
	bool IsActive(void) {return m_isactive;}
	void UpdateInput2(void);
private:
	CALLBACKGLUEPTR(kGUIMenuColObj,TableEvent,kGUIEvent)
	void TableEvent(kGUIEvent *event);
	void Resize(void);
	void DrawPopUp(void);
	bool m_isactive:1;
	bool m_drawpoprow:1;
	bool m_hassubmenu:1;
	int m_numentries;
	int m_selection;
	int m_iconwidth;
	kGUIFontInfo m_fontinfo;
	int m_popx,m_popy,m_popw,m_poph;	/* popup position */
	kGUITableObj *m_poptable;		/* use a table for the popup selector */
	ClassArray<kGUIMenuEntryObj>m_poptableentries;
};

/*! @class kGUIContainerObj
	@brief this object is the root object for an object that can contain child objects like a window or tab panel
	@ingroup kGUIObjects */
class kGUIContainerObj : public kGUIObj
{
public:
	kGUIContainerObj();
	~kGUIContainerObj();

	void ZonePreChanged(void) {Dirty();}
	void ZoneChanged(void) {DirtyandCalcChildZone();}
	void SkinChanged(void);

	void Control(unsigned int command,KGCONTROL_DEF *data);
	virtual int CurrentGroup(void) {return 0;}
	virtual void Close(void) {}

	void SetTop(bool t);
	inline bool GetTop(void) {return m_staytop;}
	bool Tab(int dir);
	void AddObject(kGUIObj *obj);
	void InsertObject(kGUIObj *obj,int index);
	void DelObject(kGUIObj *obj);

	void SetNumGroups(int num);
	void DrawC(int num);
	bool UpdateInputC(int num);

	void SetAllowOverlappingChildren(bool o) {m_allowoverlappingchildren=o;}
	void SetContainChildren(bool c) {m_containchildren=c;}

	/* these are in CurrentGroup */
	unsigned int GetNumChildren(void) {if(!m_numgroups) return(0);return m_numchildren.GetEntry(CurrentGroup());}
	kGUIObj *GetChild(unsigned int num) {return m_children[CurrentGroup()].GetEntry(num);}
	void SetChild(unsigned int num,kGUIObj *obj) {m_children[CurrentGroup()].SetEntry(num,obj);}

	/* these have group number specified */
	unsigned int GetNumChildren(int group) {return m_numchildren.GetEntry(group);}
	kGUIObj *GetChild(int group,unsigned int num) {return m_children[group].GetEntry(num);}
	void DelChild(int group,unsigned int num) {int nc=m_numchildren.GetEntry(group);return m_children[group].DeleteEntry(num);m_numchildren.SetEntry(group,--nc);}
	
	/* this is area of the object where the children objects go */
	void SetChildZone(int x,int y,int w,int h) {m_childzone.SetZone(x,y,w,h);}
	inline void CopyChildZone(kGUIZone *gzone) {m_childzone.CopyZone(gzone);}
	inline void GetChildCorners(kGUICorners *c) {GetCorners(c);c->lx+=m_childzone.GetZoneX();c->ty+=m_childzone.GetZoneY();c->rx=c->lx+m_childzone.GetZoneW();c->by=c->ty+m_childzone.GetZoneH();}
	inline int GetChildZoneX(void) {return m_childzone.GetZoneX();}
	inline int GetChildZoneY(void) {return m_childzone.GetZoneY();}
	inline int GetChildZoneW(void) {return m_childzone.GetZoneW();}
	inline int GetChildZoneH(void) {return m_childzone.GetZoneH();}
	
	/* this needs to specifically be called if children have been allocated */
	virtual void DeleteChildren(bool purge=true);	/* false will remove references to all children but not purge them */

	void SortObjects(int group,int (*code)(const void *o1,const void *o2));

	virtual kGUIObj *GetCurrentChild(void);
	virtual int GetCurrentChildNum(void);
	virtual void SetCurrentChild(kGUIObj *cobj);
	virtual void SetCurrentChild(int num);	/* -1 is no current child */

	/* scroll values for child area */
	void SetChildScroll(int sx,int sy) {m_scrollx=sx;m_scrolly=sy;}
	void SetChildScrollX(int sx) {m_scrollx=sx;}
	void SetChildScrollY(int sy) {m_scrolly=sy;}
	int GetChildScrollX(void) {return m_scrollx;}
	int GetChildScrollY(void) {return m_scrolly;}

	/* override the zonechanged callbacks */
	virtual void DirtyandCalcChildZone(void) {Dirty();CalcChildZone();}
private:
	virtual void CalcChildZone(void)=0;
	unsigned int m_numgroups;
	int m_scrollx,m_scrolly;
	kGUIZone m_childzone;		/* area in container for children */
	
	Array<int>m_current;		/* use tab to switch between objects */
	Array<unsigned int>m_numchildren;
	Array<class kGUIObj *>*m_children;
	bool m_staytop:1;						/* if true, then window stays on top till closed */
	bool m_allowoverlappingchildren:1;		/* needed for overlapping rows due to rowspan in tables */
	bool m_containchildren:1;		/* allow children to go beyond parent */
};

/*! @class kGUIRootObj
	@brief this is root gui object. All other gui objects are attached to it.
	@ingroup kGUIObjects */
class kGUIRootObj : public kGUIContainerObj
{
public:
	kGUIRootObj();
	~kGUIRootObj() {}
	void Draw(void);
	void Close(void);
	bool UpdateInput(void);
private:
	void CalcChildZone(void);
};

/*! @class kGUIRootObj
	@brief this is a container object with scrollbars, essentially it can handle showing a larger area
	than the view area and handle scrolling around automatically. */

class kGUIScrollContainerObj : public kGUIContainerObj
{
public:
	kGUIScrollContainerObj();
	~kGUIScrollContainerObj() {}
	void Draw(void);
	bool UpdateInput(void);
	void SetShowHoriz(bool h) {m_usehs=h;Dirty();}
	void SetShowVert(bool v) {m_usevs=v;Dirty();}
	void SetMaxWidth(int w) {m_maxwidth=w;UpdateScrollBars();Dirty();}
	void SetMaxHeight(int h) {m_maxheight=h;UpdateScrollBars();Dirty();}
	void Expand(void);	/* set maxwidth and maxheight to fit all children */
private:
	CALLBACKGLUEPTR(kGUIScrollContainerObj,ScrollMoveRow,kGUIEvent)
	CALLBACKGLUEPTR(kGUIScrollContainerObj,ScrollMoveCol,kGUIEvent)
	CALLBACKGLUEPTR(kGUIScrollContainerObj,ScrollEvent,kGUIEvent)
	void ScrollMoveRow(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveRow(event->m_value[0].i);}
	void ScrollMoveCol(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveCol(event->m_value[0].i);}
	void ScrollEvent(kGUIEvent *event) {if(event->GetEvent()==EVENT_MOVED)Scrolled();}

	void MoveRow(int delta);
	void MoveCol(int delta);
	void Scrolled(void);
	void CalcChildZone(void);
	void UpdateScrollBars(void);

	int m_maxwidth;
	int m_maxheight;
	bool m_usehs;
	bool m_usevs;
	kGUIScroll m_scroll;		/* damped source / dest position handler */
	kGUIScrollBarObj m_hscrollbar;
	kGUIScrollBarObj m_vscrollbar;
};

/* override the base text object so we can detect changes */
class kGUIMenuColTitleObj : public kGUIText
{
public:
	void SetMenu(class kGUIMenuObj *m) {m_m=m;}

	/* if the user changes the title then trigger a window redraw */
	void StringChanged(void);
	void FontChanged(void);
private:
	class kGUIMenuObj *m_m;
};

/*! @class kGUIMenuObj
	@brief this is a menu gui object.
	@ingroup kGUIObjects */

class kGUIMenuObj : public kGUIContainerObj, public kGUIFontInfo
{
public:
	kGUIMenuObj();
	~kGUIMenuObj();
	void SetNumEntries(unsigned int n);
	kGUIText *GetTitle(unsigned int col) {return m_title.GetEntryPtr(col);}
	void SetEntry(unsigned int index,kGUIMenuColObj *col) {m_entry.SetEntry(index,col);}
	void Resize(void);
	CALLBACKGLUE(kGUIMenuObj,Track);
private:
	CALLBACKGLUEPTR(kGUIMenuObj,MenuEvent,kGUIEvent)
	void MenuEvent(kGUIEvent *event);
	void CalcChildZone(void) {SetChildZone(0,0,GetZoneW(),GetZoneH());}
	void OpenMenu(kGUIMenuColObj *menu,int x,int y);
	void CloseMenu(void);
	void Track(void);
	void Draw(void);
	bool UpdateInput(void);
	int m_numentries;
	int m_depth;
	int m_colhover;
	bool m_track;
	ClassArray<kGUIMenuColTitleObj>m_title;
	Array<unsigned int>m_titlex;
	Array<unsigned int>m_titlew;
	Array<kGUIMenuColObj *>m_entry;
	Array<kGUIMenuColObj *>m_activeentry;
};

/*! @class kGUITabObj
	@brief this is tab gui object. It can have multiple rows of tabs and each tab can correspond to
    it's own child area or share child areas with other tabs using a cross reference list between
	tabs and child areas. In typical use you would have n tabs and n child areas or n tabs and 1 child area.
	@ingroup kGUIObjects */
class kGUITabObj : public kGUIContainerObj
{
public:
	kGUITabObj();
	~kGUITabObj();
	void Draw(void);
	bool UpdateInput(void);
	void SetCurrentTab(int n) {if(m_curtab!=n){m_curtab=n;Dirty();}}
	bool SetCurrentTabNamez(const char *name);	/* true=found, false=not found */
	virtual int CurrentGroup(void) {return m_numtabs?m_tabgroups.GetEntry(m_curtab):0;}
	int GetCurrentTab(void) {return (m_curtab);}
	void SetNumTabs(int numtabs,int numgroups);
	void SetNumTabs(int numtabs) {SetNumTabs(numtabs,numtabs);}
	int GetNumTabs(void) {return m_numtabs;}
	int GetNumGroups(void) {return m_numgroups;}
	void SetTabName(int tabindex,const char *name) {m_tabnames.GetEntryPtr(tabindex)->SetString(name);UpdateTabs();}
	void SetTabName(int tabindex,kGUIString *name) {m_tabnames.GetEntryPtr(tabindex)->SetString(name);UpdateTabs();}
	const char *GetTabName(int index) {return m_tabnames.GetEntryPtr(index)->GetString();}
	kGUIText *GetTabTextPtr(int index) {return m_tabnames.GetEntryPtr(index);}
	void SetTabGroup(int tabindex,int groupindex) {m_tabgroups.SetEntry(tabindex,groupindex);}
	int GetTabGroup(int tabindex) {return m_tabgroups.GetEntry(tabindex);}
	
	void SetLocked(bool l) {m_locked=l;}
	void SetStartTabX(int x) {m_starttabx=x;}
	int GetTabRowHeight(void);
	CALLBACKGLUE(kGUITabObj,Track);
private:
	void CalcChildZone(void);
	void UpdateTabs(void);
	void Track(void);
	void DirtyTab(int tab);

	bool m_track:1;
	bool m_locked:1;
	int m_numtabs;
	int m_numgroups;
	int m_curtab;
	int m_overtab;
	int m_numtabrows;
	int m_starttabx;
	ClassArray<kGUIText>m_tabnames;
	Array<int>m_tabgroups;
	Array<int>m_tabx;
	Array<int>m_taby;
};

/* This is the base class for table row objects, the user code must */
/* inherit from this class for it's objects */

class kGUITableRowObj : public kGUIContainerObj
{
public:
	kGUITableRowObj() {m_rowheight=0;m_selected=false;m_currentcell=0;}
	~kGUITableRowObj() {}
	virtual int GetNumObjects(void)=0;
	virtual kGUIObj **GetObjectList(void)=0;
	void Draw(void);
	bool UpdateInput(void);
	inline unsigned int GetRowHeight(void) {return m_rowheight;}
	void SetRowHeight(const unsigned int h);
	/* if one of the cell pointers have changed to a different object */
	void CellChanged(void);
	inline void SetSelected(bool s) {m_selected=s;Dirty();}
	inline bool GetSelected(void) {return m_selected;}

	/* overide the base container class for these */
	kGUIObj *GetCurrentChild(void) {return m_currentcell;}
	void SetCurrentChild(kGUIObj *cobj);
	void SetCurrentChild(int num);	/* -1 is no current child */
private:
	bool m_selected:1;
	unsigned int m_rowheight:31;
	kGUIObj *m_currentcell;
	virtual void CalcChildZone(void);
};

class kGUIComboTableRowObj : public kGUITableRowObj
{
public:
	kGUIComboTableRowObj(unsigned int n) {m_numobjs=n;m_objptrs[0]=&m_text;m_objptrs[1]=&m_box;m_istextvalue=false;}
	inline int GetNumObjects(void) {return m_numobjs;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
	inline void SetString(const char *t) {m_text.SetString(t);}
	inline void SetString(kGUIString *t) {m_text.SetString(t);}
	inline kGUIFontInfo *GetFontInfoPtr(void) {return &m_text;} 
	inline void SetFontInfo(const kGUIFontInfo *fi) {m_text.SetFontInfo(fi);}
	inline const char *GetString(void) {return m_text.GetString();}
	inline kGUIText *GetText(void) {return &m_text;}
	inline void SetValue(int v) {m_istextvalue=false;m_value=v;}
	inline void SetValue(const char *v) {m_istextvalue=true;m_textvalue.SetString(v);}
	inline void SetValue(kGUIString *v) {m_istextvalue=true;m_textvalue.SetString(v);}
	inline bool GetIsTextValue(void) {return m_istextvalue;}
	inline int GetValue(void) {return m_value;}
	inline kGUIString *GetTextValue(void) {return &m_textvalue;}
	inline int GetHeight(void) {return m_text.GetHeight();}
	inline int GetWidth(void) {return m_text.GetWidth();}
	inline void SetBox(int w,kGUIColor c) {m_box.SetColor(c);m_box.SetSize(w,20);}
	inline kGUIColor GetBoxColor(void) {return m_box.GetColor();}
private:
	bool m_istextvalue:1;
	unsigned int m_numobjs:8;
	kGUIObj *m_objptrs[2];
	kGUITextObj m_text;
	kGUIRectObj m_box;
	int m_value;
	kGUITextObj m_textvalue;
};

class kGUIMenuEntryObj : public kGUITableRowObj
{
public:
	kGUIMenuEntryObj() {m_isbar=false;m_objptrs[0]=&m_icon;m_objptrs[1]=&m_text;m_objptrs[2]=&m_subshape;SetRowHeight(10);m_submenu=0;}
	void Draw(void);
	inline int GetNumObjects(void) {return 3;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
	inline void SetString(const char *t) {m_text.SetString(t);}
	inline void SetString(kGUIString *t) {m_text.SetString(t);}
	inline void SetFontInfo(const kGUIFontInfo *fi) {m_text.SetFontInfo(fi);}
	inline const char *GetString(void) {return m_text.GetString();}
	inline void SetTextColor(kGUIColor c) {m_text.SetColor(c);}
	inline void SetBGColor(kGUIColor bg) {m_text.SetBGColor(bg);}
	inline kGUIText *GetText(void) {return &m_text;}
	inline void SetValue(int v) {m_value=v;}
	inline int GetValue(void) {return m_value;}
	inline int GetHeight(void) {return m_text.GetHeight();}
	inline int GetWidth(void) {return m_text.GetWidth();}
	inline kGUIImageObj *GetIconObj(void) {return &m_icon;}
	void SetSubMenu(kGUIMenuColObj *submenu) {m_submenu=submenu;}
	inline kGUIMenuColObj *GetSubMenu(void) {return m_submenu;}
	bool GetIsBar(void) {return m_isbar;}
	void SetIsBar(bool b) {m_isbar=b;}
private:
	kGUIObj *m_objptrs[3];
	kGUIImageObj m_icon;
	kGUITextObj m_text;
	kGUIImageRefObj m_subshape;
	int m_value;
	bool m_isbar;
	kGUIMenuColObj *m_submenu;
};

enum
{
TABLEEDIT_NONE,
TABLEEDIT_COLWIDTH,
TABLEEDIT_ROWHEIGHT
};

/* override the base text object so we can detect changes */
class kGUITableColTitleObj : public kGUIText
{
public:
	void SetTable(class kGUITableObj *t) {m_t=t;}

	/* if the user changes the title then trigger a window redraw */
	void StringChanged(void);
	void FontChanged(void);
private:
	class kGUITableObj *m_t;
};

/*! @class kGUITableObj
	@brief this is table gui object. The table is a scrollable grid of child cells,
    each cell can contain any kGUIObj class object. A table could theoretically contain
	another table inside of a cell. The table can have horizontal or verical scroll bars
	if desired, it also can handle re-ordering of the columns and re-sorting of the rows.
	This table class is used to render popup menus and combo boxes as well.
	@ingroup kGUIObjects */
class kGUITableObj : public kGUIContainerObj
{
public:
	kGUITableObj();
	~kGUITableObj() {}
	void SetSize(int w,int h) {kGUIContainerObj::SetSize(w,h);SizeDirty();}

	void AddObject(kGUIObj *obj);
	void Draw(void);
	void Activate(void);
	bool UpdateInput(void);
	void TableDirty(void);	/* records have been deleted */
	void UpdateIfDirty(void) {if(m_positionsdirty==true)ReCalcPositions();}
	void DeleteChildren(bool purge=true);	
	void DeleteRow(kGUITableRowObj *obj,bool purge=true);
	bool DeleteRowz(kGUITableRowObj *obj,bool purge=true);
	void SetNumCols(unsigned int n);
	void SetColWidth(int n,int w) {m_colwidths.SetEntry(n,w);m_sizechanged=true;m_positionsdirty=true;}
	void SetColTitle(int n,kGUIString *s) {m_coltitles.GetEntryPtr(n)->SetString(s);m_positionsdirty=true;}
	void SetColTitle(int n,const char *t) {m_coltitles.GetEntryPtr(n)->SetString(t);m_positionsdirty=true;}
	void SetColHint(int n,const char *t) {m_colhints.GetEntryPtr(n)->SetString(t);}
	void SetColHint(int n,kGUIString *t) {m_colhints.GetEntryPtr(n)->SetString(t);}
	kGUIText *GetColHeaderTextPtr(int n) {return m_coltitles.GetEntryPtr(n);}
	int GetNumCols(void) {return m_numcols;}
	int GetNumRows(void) {return GetNumChildren();}
	int GetColWidth(int n) {return m_colwidths.GetEntry(n);}
	unsigned int GetColOrder(int n) {return m_colorder.GetEntry(n);}
	bool GetColShow(int n) {return m_showcols.GetEntry(n);}
	int GetColTitleIndex(const char *name);
	const char *GetColTitle(int n) {return m_coltitles.GetEntryPtr(n)->GetString();}

	void SetColOrder(unsigned int n,unsigned int x) {m_colorder.SetEntry(n,x);m_sizechanged=true;m_positionsdirty=true;Dirty();}
	void SetColShow(int n,bool s) {m_showcols.SetEntry(n,s);if(m_showcols.GetEntry(m_colorder.GetEntry(m_cursorcol))==false)MoveCol(-1);m_sizechanged=true;m_positionsdirty=true;Dirty();}
	void AddRow(kGUITableRowObj *obj,int place=0);
	int CalcTableWidth(void);
	int CalcTableHeight(void);
	void SetPositionsDirty(void) {m_positionsdirty=true;}
	void CellChanged(void) {m_positionsdirty=true;m_sizechanged=true;}
	inline void SizeDirty(void) {CalcChildZone();m_sizechanged=true;m_positionsdirty=true;}
	inline void SetSelectMode(void) {m_selectmode=true;}	/* used for combo box */
	inline bool GetSelectMode(void) {return m_selectmode;}
	inline void SetListMode(void) {m_listmode=true;};		/* used for list box */
	inline bool GetListMode(void) {return m_listmode;}
	inline void NoColScrollbar(void) {m_showcolscrollbar=false;CalcChildZone();}
	inline void NoRowScrollbar(void) {m_showrowscrollbar=false;CalcChildZone();}
	inline void NoColHeaders(void) {m_showcolheaders=false;CalcChildZone();}
	inline void NoRowHeaders(void) {m_showrowheaders=false;CalcChildZone();}
	inline void SetPopRowHeaders(bool d) {m_poprowheaders=d;CalcChildZone();}
	inline void LockCol(void) {m_lockcol=true;}
	inline void LockRow(void) {m_lockrow=true;}
	inline void SetAllowDelete(bool d) {m_allowdelete=d;}
	inline unsigned int GetNumberRows(void) {return GetNumChildren();}
	inline unsigned int GetCursorRow(void) {return m_cursorrow;}
	inline unsigned int GetCursorCol(void) {return m_cursorcol;}
	inline int GetSelected(void) {return m_selected;}
	inline void ClearOver(void) {m_wasover=false;m_releasecount=0;}
	inline void GotoRow(unsigned int r,bool clearsel=true) {MoveRow((int)(r-m_cursorrow),clearsel);}
	void CallAfterUpdate(void);
	void SetAllowAdjustRowHeights(bool allow) {m_allowadjrowheights=allow;}
	void SetAllowAdjustColWidths(bool allow) {m_allowadjcolwidths=allow;}

	void CalculateColWidth(int col);
	void CalculateColWidths(void) {unsigned int xx;for(xx=0;xx<m_numcols;++xx)CalculateColWidth(xx);}
	CALLBACKGLUEVAL(kGUITableObj,MoveRow,int )
	CALLBACKGLUEVAL(kGUITableObj,MoveCol,int)

	unsigned int CalcRowHeight(unsigned int n);

	kGUITableRowObj *GetRow(unsigned int r) {return static_cast<kGUITableRowObj *>(GetChild(r));}
	kGUITableRowObj *GetCursorRowObj(void) {return GetRow(m_cursorrow);}
	int GetRowHeight(unsigned int n) {return GetRow(n)->GetRowHeight();}
	void SetRowHeight(unsigned int n,int h) {GetRow(n)->SetRowHeight(h);}

	int GetRowSelectorWidth(void);

	/* easy way to save user adjusted column widths and hide/show into a app config xml file */
	void LoadConfig(class kGUIXMLItem *root);
	void SaveConfig(class kGUIXMLItem *root,const char *name);

	/* sort function that calls user code to determine sort order */
	void Sort(int (*code)(const void *o1,const void *o2));

	/* callback events */
	void SetAllowAddNewRow(bool allownew) {m_allownew=allownew;}

	/* only used for select mode ( menus and combo boxes etc.) */
	void SetEntryEnable(unsigned int index,bool b) {m_available.SetEntry(index,b);}
	/* if entry is not in the table then return true */
	bool GetEntryEnable(unsigned int index) {if(m_available.GetNumEntries()>index) return m_available.GetEntry(index);return true;}
	/* used by LoadTable function in database code */
	kGUITableRowObj *AddNewRow(void);
	void MoveRow(int delta,bool clearsel=true);
	void MoveCol(int delta,bool counthidden=false);
	void ScrollRow(int delta,bool updatecursor=true);
	void SwapRow(int delta);		/* 1 or -1 are the only valid delta values */
	void SelectRow(int line,bool add=false);
	void UnSelectRows(void);
	void SetAllowMultiple(bool a) {m_allowmultiple=a;}
	void CalcChildZone(void);
private:
	CALLBACKGLUEPTR(kGUITableObj,ScrollMoveRow,kGUIEvent)
	CALLBACKGLUEPTR(kGUITableObj,ScrollMoveCol,kGUIEvent)
	void ScrollMoveRow(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE){if(GetListMode()==true)ScrollRow(event->m_value[0].i,false);else MoveRow(event->m_value[0].i);}}
	void ScrollMoveCol(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)MoveCol(event->m_value[0].i);}

	CALLBACKGLUEPTR(kGUITableObj,Scrolled,kGUIEvent)
	CALLBACKGLUEVAL(kGUITableObj,DelSelRowsDone,int)
	void CallSelectedEvent(void);
	void DelSelRowsDone(int closebutton);
	void CursorMoved(void);
	void ReCalcPositions(void);
	void Scrolled(kGUIEvent *event);
	void CalcDrawBounds(void);
	void UpdateCurrentObj(void);
	kGUIObj *GetCell(unsigned int row,unsigned int col);

	unsigned int m_numcols;
	unsigned int m_numrows;
	int m_lastselectedrow;
	int m_colheaderheight;

	Array<int>m_colwidths;
	Array<unsigned int>m_colorder;
	Array<bool>m_showcols;
	ClassArray<kGUITableColTitleObj>m_coltitles;
	ClassArray<kGUIString>m_colhints;

	/* these are recalculated whenever the column order is changed, resized or hidden/shown */
	Array<int>m_cxs;
	Array<int>m_cwidths;

	int m_drawaddy;
	int m_editmode;
	int m_editcol;
	int m_editrow;
	
	bool m_sizechanged:1;		/* row added/deleted or columns resized/changed */
	bool m_positionsdirty:1;
	bool m_drawaddbutton:1;
	bool m_wasover:1;			/* initally false, then true once mouse is over table */
	bool m_allowdelete:1;		/* can users delete entries? */
	bool m_lockcol:1;
	bool m_lockrow:1;			/* don't allow cursor to move? */
	bool m_allownew:1;
	bool m_showrowscrollbar:1;
	bool m_showcolscrollbar:1;
	bool m_showcolheaders:1;
	bool m_showrowheaders:1;
	bool m_poprowheaders:1;
	bool m_selectmode:1;		/* if true, clicking on a row selects it and exits */
	bool m_listmode:1;			/* if true, clicking on a row selects the whole row */
	bool m_allowadjrowheights:1;
	bool m_allowadjcolwidths:1;
	bool m_viewadd:1;			/* is the add new record button on the screen or off the bottom? */
	bool m_allowmultiple:1;		/* allow more than one line to be selected */
	bool m_wasoff:1;			/* mouse was off of table last update */

	int m_releasecount;

	/* this is the current cursor position */
	unsigned int m_cursorrow;
	unsigned int m_cursorcol;

	/* these are the bounds of the visable area */
	unsigned int m_toprow;
	unsigned int m_botrow;
	unsigned int m_leftcol;
	unsigned int m_rightcol;
	unsigned int m_lastfullrow;	/* the last row fully on screen */
	unsigned int m_lastfullcol;	/* the last column fully on screen */
	kGUIScrollBarObj m_rowscrollbar;
	kGUIScrollBarObj m_colscrollbar;

	unsigned int m_drawtoprow;
	unsigned int m_drawbotrow;
	unsigned int m_drawleftcol;
	unsigned int m_drawrightcol;

	Array<bool>m_available;	/* this array is only used for select mode tables (combo/menu etc) */
	int m_selected;	/* row selected, -1=aborted selection */
	kGUIScroll m_scroll;	/* table pixel scroll values */
};

/*! @class kGUIListboxObj
	@brief this is listbox gui object. The listbox is just like a combobox except 
    that it is a fixed size and doesn't pop up. Also it can optionally allow
	selection of multiple entries if desired.
	@ingroup kGUIObjects */
class kGUIListboxObj : public kGUITableObj, public kGUIFontInfo
{
public:
	kGUIListboxObj();
	~kGUIListboxObj();

	void SetNumEntries(unsigned int n);
	unsigned int GetNumEntries(void) {return m_numentries;}
	unsigned int CalcHeight(unsigned int numrows);
	void SetEntry(unsigned int index,const char *entryname,int entryval);
	void SetEntry(unsigned int index,const char *entryname,const char *entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,int entryval);
	void SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval);
	void RenameEntry(const char *oldname,const char *newname);
	void SetLocked(bool l) {m_locked=l;Dirty();}
	int GetWidest(void);

	kGUIText *GetEntryTextPtr(unsigned int index);
	void SetSelection(int s,bool add=false);
	bool SetSelectionz(int s,bool add=false);
	void SetSelection(const char *string,bool add=false);			/* uses the view name */
	void SetSelectionString(const char *string,bool add=false);		/* uses the hidden string name */
	bool SetSelectionStringz(const char *string,bool add=false);					/* uses the view name */
	bool SetSelectionz(const char *string,bool add=false);			/* uses the hidden string name */
	unsigned int GetType(void) {return m_type;}
	unsigned int GetSelections(Array<unsigned int>*list);

	kGUIString *GetSelectionStringObj(unsigned int entry);
	void SetHint(const char *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetHint(kGUIString *string) {if(!m_hint)m_hint=new kGUIString();m_hint->SetString(string);}
	void SetColorMode(unsigned int width);
	void SetColorBox(unsigned int index,kGUIColor c);

private:
	void DrawPopUp(void);
	unsigned int m_colorcolwidth;

	bool m_locked:1;
	bool m_colormode:1;
	unsigned int m_editmode:2;
	unsigned int m_type:1;

	unsigned int m_numentries;
	unsigned int m_undoselection;
	unsigned int m_selection;
	kGUIString *m_hint;
	kGUIComboTableRowObj **m_tableentries;
};

enum 
{
WINDOWMODE_ADJPOS,
WINDOWMODE_ADJWIDTH,
WINDOWMODE_ADJHEIGHT,
WINDOWMODE_ADJSIZE,
WINDOWMODE_NONE
};

enum
{
WINDOWBUTTON_NONE=0,
WINDOWBUTTON_CLOSE=1,
WINDOWBUTTON_MINIMIZE=2,
WINDOWBUTTON_FULL=4
};

class kGUIWindowTitle : public kGUIText
{
public:
	void SetWindow(class kGUIWindowObj *w) {m_w=w;}

	/* if the user changes the title then trigger a window redraw */
	void StringChanged(void);
	void FontChanged(void);
private:
	class kGUIWindowObj *m_w;
};

/*! @class kGUIWindowObj
	@brief this is window gui object. 
	@ingroup kGUIObjects */
class kGUIWindowObj : public kGUIContainerObj
{
public:
	kGUIWindowObj();
	void SetPos(int x,int y);
	void SetSize(int w,int h);
	void SetInsideSize(int w,int h);
	void Draw(void);

	bool UpdateInput(void);
	
	/* get pointer to title so toy can change the text and or color etc */
	kGUIText *GetTitle() {return &m_title;}

	/* center the window */
	void Center(void);

	/* todo: depreciate these */
	void SetTitle(const char *t) {m_title.SetString(t);Dirty();}
	void SetTitle(kGUIString *t) {m_title.SetString(t);Dirty();}

	/* turn on or off the window frame */
	void SetFrame(bool f) {m_frame=f;DirtyandCalcChildZone();}

	void ExpandToFit(void);
	void Close(void);
	void Shrink(void);
	void SetBackground(bool b) {m_background=b;}
	bool GetBackground(void) {return m_background;}
	
	/* callback events */
	bool GetIsMinimized(void) {return m_minimized;}
	void SetAllowButtons(int allow) {m_allow=allow;}
private:
	void CalcChildZone(void);
	kGUIWindowTitle m_title;
	bool m_frame:1;
	bool m_minimized:1;
	bool m_full:1;
	bool m_background:1;			/* main background window */
	unsigned int m_activemode:3;	/* only valid when window is active */
	unsigned int m_over:4;			/* mouse hovering over one of the top bar buttons */
	unsigned int m_allow:4;			/* buttons in top corner */
	kGUIZone m_savezone;			/* saved window zone when minimized or full */	
};

/*! @class kGUIControlBoxObj 
	@brief this is control box gui object. The controlbox object is used to automatically
    position child objects within it for easy screen layout.
	@ingroup kGUIObjects 
	@todo  Allow re-layout if control area has been made smaller or larger */
class kGUIControlBoxObj : public kGUIContainerObj
{
public:
	kGUIControlBoxObj();
	void Reset();
	void AllocSpace(int w,int h,int *x,int *y);
	void AddObject(kGUIObj *obj);
	void AddObjects(unsigned int num,...);
	void AddFixedObject(kGUIObj *obj);
	void SetBorderGap(int g) {m_bordergap=g;DirtyandCalcChildZone();}
	void SetObjectGap(int g) {m_objectgap=g;}
	void SetMaxWidth(int mw) {m_maxwidth=mw;}
	void SetMaxHeight(int mh) {m_maxheight=mh;}
	void SetBGColor(kGUIColor bgcol) {m_bgcolor=bgcol;Dirty();}
	void SetDrawBG(bool d) {m_drawbg=d;Dirty();}
	void SetDrawFrame(bool f) {m_drawframe=f;Dirty();}
	void Draw(void);
	bool UpdateInput(void);
	void NextLine(void);
	int GetCurrentX(void) {return m_currentx;}
	int GetCurrentY(void) {return m_currenty;}
	int GetNextY(void) {return m_currenty+m_objectgap+m_objectgap+m_bordergap;}
	int GetBorderGap(void) {return m_bordergap;}
	void SetRedo(bool r) {m_redo=r;Dirty();}
private:
	void CalcChildZone(void);
	bool m_drawframe:1;
	bool m_redo:1;			/* allow objects to be added even if connected */
	bool m_drawbg:1;
	kGUIColor m_bgcolor;
	int m_currentx;
	int m_currenty;
	int m_tallest;		/* reset for each row */
	int m_bordergap;	/* gap area outside of group */
	int m_objectgap;	/* gap between controls */
	int m_maxwidth;
	int m_maxheight;

	/* make objects float around fixed ones */
	int m_numfixed;
	Array<kGUICorners>m_fixedobjects;
};

/* connect this to a movie object to automagically control it */

class kGUIMovie;

/*! @class kGUIMovieControlObj
	@brief this is movie control gui object. This is essentially a movie object
    along with the associated controls for playing the movie.
	@ingroup kGUIObjects */
class kGUIMovieControlObj : public kGUIContainerObj
{
public:
	kGUIMovieControlObj();
	~kGUIMovieControlObj();
	void SetMovie(kGUIMovie *movie);
	void SetLoop(bool l) {m_loop.SetSelected(true);}
	bool GetLoop(void) {return m_loop.GetSelected();}
	void CallAfterUpdate(void);
	void Draw(void);
	bool UpdateInput(void);
private:
	void CalcChildZone(void);
	CALLBACKGLUE(kGUIMovieControlObj,Event);
	CALLBACKGLUEPTR(kGUIMovieControlObj,Move,kGUIEvent);
	CALLBACKGLUEPTR(kGUIMovieControlObj,PressPlayPause,kGUIEvent);
	CALLBACKGLUEPTR(kGUIMovieControlObj,LoopChanged,kGUIEvent);

	void Event(void);
	void Move(kGUIEvent *event);
	void PressPlayPause(kGUIEvent *event);
	void LoopChanged(kGUIEvent *event);
	void Position(void);		/* reposition buttons etc */
	void UpdateButton(void);	/* change button from play to pause */
	bool m_eventactive:1;
	bool m_lastplaying:1;

	kGUIMovie *m_movie;

	kGUIImage m_iplay;
	kGUIImage m_ipause;
	kGUIButtonObj m_playpause;
	kGUIScrollBarObj m_slider;
	kGUITickBoxObj m_loop;
	int m_lasttime;
	int m_lastduration;
};

typedef struct
{
	int width,height;
	long time;
}kGUIImageSizeCache;

/* used for both screen and printing */
/* data format is int per pixel */

class kGUIDrawSurface
{
public:
	kGUIDrawSurface() {m_width=0;m_height=0;m_pixels=0;m_xoff=0;m_yoff=0;m_printjob=0;}
	~kGUIDrawSurface() {if(m_pixels) delete []m_pixels;}
	inline void Init(int w,int h) {if(m_pixels) delete []m_pixels;m_width=w;m_height=h;m_pixels=new kGUIColor[w*h];m_xoff=0;m_yoff=0;Clear(0);}
	inline void SetOffsets(int xoff,int yoff) {m_xoff=xoff;m_yoff=yoff;}
	void Clear(kGUIColor c);
	inline int GetWidth(void) {return m_width;}
	inline int GetHeight(void) {return m_height;}
	inline int GetOffsetX(void) {return m_xoff;}
	inline int GetOffsetY(void) {return m_yoff;}
	inline int GetBPP(void) {return sizeof(kGUIColor);}
	inline int GetFormat(void) {return GUISHAPE_SURFACE;}
	
	/* todo: optimize?, make pointer table for each line instead of using multiply */
	inline kGUIColor *GetSurfacePtrABS(int x,int y) {return &(m_pixels[(((y))*m_width)+(x)]);}
	inline kGUIColor *GetSurfacePtr(int x,int y) {return &(m_pixels[(((y)+m_yoff)*m_width)+(x)+m_xoff]);}

	/* clipped version, return zero if point is off screen */
	inline kGUIColor *GetSurfacePtrC(kGUICorners *c,int x,int y) {if((x+m_xoff)<c->lx)return(0);if((y+m_yoff)<c->ty)return(0);if((x+m_xoff)>=c->rx)return(0);if((y+m_yoff)>=c->by)return(0);return &(m_pixels[(((y)+m_yoff)*m_width)+(x)+m_xoff]);}
	void UnRotateSurface(kGUIDrawSurface *ss);
	void UnRotateSurface(kGUIDrawSurface *ss,int lx,int rx,int ty,int by);

	class kGUIPrintJob *GetPrintJob(void) {return m_printjob;}
	void SetPrintJob(class kGUIPrintJob *printjob) {m_printjob=printjob;}
private:
	int m_width;
	int m_height;
	int m_xoff,m_yoff;	/* mainly used for printer reports */
	kGUIColor *m_pixels;
	/* if printing then this is valid, if not, it is null */
	class kGUIPrintJob *m_printjob;
};

class kGUIPrinter
{
public:
	kGUIPrinter() {m_valid=true;m_gotinfo=false;}
	~kGUIPrinter() {}
	void SetName(const char *n) {m_name.SetString(n);}
	const char *GetName(void) {return m_name.GetString();}
	void GetInfo(int *ppw,int *pph,int *ppih,int *ppiv);
	bool GetValid(void) {return m_valid;}
	/* this is used if no printers are defined so this sets a default printer for viewing but not printing */
	void SetDefaultPageSize(void) {m_valid=false;m_gotinfo=true;m_pagewidth=8*100;m_pageheight=11*100;m_ppihoriz=100;m_ppivert=100;}
private:
	kGUIString m_name;

	bool m_valid:1;
	bool m_gotinfo:1;
	int m_pagewidth;
	int m_pageheight;
	int m_ppihoriz;
	int m_ppivert;
};

class kGUIDate
{
public:
	kGUIDate();						/* defaults to 0000-00-00 */
	void SetToday(void);
	void Copy(kGUIDate *d2) {m_day=d2->m_day;m_month=d2->m_month;m_year=d2->m_year;m_hour=d2->m_hour;m_minute=d2->m_minute;m_second=d2->m_second;}
	bool GetFileDate(const char *fn);	/* get modified date from file */
	void Set(const char *datestring);	/* mysql format yyyy-mm-dd */
	bool Setz(const char *datestring);	/* mysql format yyyy-mm-dd return true=ok, false=error*/
	void Set(int d,int m,int y);
	void SetDay(int d) {m_day=d;}
	void SetMonth(int m) {m_month=m;}
	void SetYear(int y) {m_year=y;}
	void AddSeconds(int numseconds);
	void AddDays(int numdays);
	int GetDay(void) {return m_day;}
	int GetDayofWeek(void);		/* return 0=6 for weekday */
	int GetMonth(void) {return m_month;}
	int GetYear(void) {return m_year;}
	long GetQuick(void) {return (m_day|(m_month<<5)|(m_year<<(5+4))); }
	void SetQuick(long q) {m_day=q&31;m_month=(q>>5)&15;m_year=q>>(5+4);}
	void ShortDate(kGUIString *s);
	void ShortDateTime(kGUIString *s,bool use24=true);
	void LongDate(kGUIString *s);
	void MedDate(kGUIString *s);
	void Time(kGUIString *s,bool use24=false);
	void SetHour(int h) {m_hour=h;}
	void SetMinute(int m) {m_minute=m;}
	void SetSecond(int s) {m_second=s;}
	int GetHour(void) {return m_hour;}
	int GetMinute(void) {return m_minute;}
	int GetSecond(void) {return m_second;}

	int GetDiffSeconds(kGUIDate *d2);
	int GetDiffMinutes(kGUIDate *d2);
	int GetDiffHours(kGUIDate *d2);
	int GetDiffDays(kGUIDate *d2);
	
	/* convert from GMT to localtime and vice versa */
	void GMTtoLocal(void);
	void LocaltoGMT(void);

	/* static date functions */
	static const char *GetWeekDay3(int d);	/* 0=Sun, 1=Mon .... 6=Sat */
	static const char *GetWeekDay(int d);	/* 0=Sunday, 1=Monday .... 6=Saturday */
	static const char *GetMonthName(int m);	/* 1-12 */
private:
	void CalcGMTDelta(void);
	static int m_gmtdelta;					/* delta in seconds between GMT and localtime */
	int m_day;
	int m_month;
	int m_year;

	int m_hour;
	int m_minute;
	int m_second;
};

#define MSGBOX_ABORT -1

#define MSGBOX_NO 1
#define MSGBOX_YES 2
#define MSGBOX_CANCEL 4
#define MSGBOX_DONE 8
#define MSGBOX_OK 16

class kGUIMsgBoxReq
{
public:
	kGUIMsgBoxReq(int buttons,void *codeobj,void (*code)(void *,int result),bool spr,const char *message,...);
	kGUIMsgBoxReq(int buttons,bool spr,const char *message,...);
	kGUIText *GetTitle(void) {return m_window.GetTitle();}
private:
	CALLBACKGLUEPTR(kGUIMsgBoxReq,WindowEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIMsgBoxReq,PressNo,kGUIEvent)
	CALLBACKGLUEPTR(kGUIMsgBoxReq,PressYes,kGUIEvent)
	CALLBACKGLUEPTR(kGUIMsgBoxReq,PressCancel,kGUIEvent)
	CALLBACKGLUEPTR(kGUIMsgBoxReq,PressOK,kGUIEvent)
	CALLBACKGLUEPTR(kGUIMsgBoxReq,PressDone,kGUIEvent)
	void Init(const char *message,int buttons);
	void WindowEvent(kGUIEvent *event);
	void PressNo(kGUIEvent *event);
	void PressYes(kGUIEvent *event);
	void PressCancel(kGUIEvent *event);
	void PressOK(kGUIEvent *event);
	void PressDone(kGUIEvent *event);

	kGUIWindowObj m_window;
	kGUIInputBoxObj m_text;
	kGUIButtonObj m_no;
	kGUIButtonObj m_yes;
	kGUIButtonObj m_cancel;
	kGUIButtonObj m_ok;
	kGUIButtonObj m_done;

	bool m_docall;
	kGUICallBackInt m_donecallback;
	int m_result;
};

class kGUIInputBoxReq
{
public:
	kGUIInputBoxReq(void *codeobj,void (*code)(void *,kGUIString *result,int closebutton),const char *message,...);
	kGUIText *GetTitle(void) {return m_window.GetTitle();}
	void SetDefault(const char *def) {m_input.SetString(def);}
private:
	CALLBACKGLUEPTR(kGUIInputBoxReq,WindowEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIInputBoxReq,PressCancel,kGUIEvent)
	CALLBACKGLUEPTR(kGUIInputBoxReq,PressOK,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	void PressCancel(kGUIEvent *event);
	void PressOK(kGUIEvent *event);

	kGUIWindowObj m_window;
	kGUITextObj m_text;
	kGUIInputBoxObj m_input;
	kGUIButtonObj m_cancel;
	kGUIButtonObj m_ok;
	int m_closebutton;
	kGUICallBackPtrInt<kGUIString> m_donecallback;
};

class kGUIDir
{
public:
	kGUIDir() {m_numfiles=0;m_numdirs=0;m_filenames.Init(64,64);m_dirnames.Init(16,16);}
	~kGUIDir();
	void Purge(void);
	void LoadDrives(void);
	void LoadDir(const char *path,bool recursive,bool fullnames,const char *ext=0);
	void LoadDir(BigFile *bf,const char *ext=0);
	int GetNumFiles(void) {return m_numfiles;}
	const char *GetFilename(int index) {return m_filenames.GetEntryPtr(index)->GetString();}
	int GetNumDirs(void) {return m_numdirs;}
	const char *GetDirname(int index) {return m_dirnames.GetEntryPtr(index)->GetString();}
private:
	void LoadDir2(const char *path,bool recursive,bool fullnames,kGUIStringSplit *exts);
	int m_numfiles;
	ClassArray <kGUIString>m_filenames;
	int m_numdirs;
	ClassArray <kGUIString>m_dirnames;
};

extern int jpegversion;			/* defined in kguiimage.cpp */
extern const char pngversion[];	/* defined in kguiimage.cpp */

struct random_bucket
{
};

#define POOLWORDS 128    /* Power of 2 - note that this is 32-bit words */
#define POOLBITS (POOLWORDS*32)

class kGUIRandom
{
public:
	kGUIRandom() {InitSeed();}
	void InitSeed(void);
	void AddEntropy(const unsigned long input);
	void AddEntropy(const char *buf, int nbytes);
	void ExtractEntropy(char * buf,  int nbytes);
private:
	void MD5Transform(unsigned long buf[4],unsigned long const in[16]);

	unsigned int m_add_ptr;
	unsigned int m_entropy_count;
	int m_input_rotate;
	unsigned long m_pool[POOLWORDS];
};



enum
{
MOUSECURSOR_DEFAULT,
MOUSECURSOR_BUSY,
MOUSECURSOR_ADJUSTHORIZ,
MOUSECURSOR_ADJUSTVERT,
MOUSECURSOR_ADJUSTSIZE,
MOUSECURSOR_LINK};

class kGUISkin
{
public:
	virtual ~kGUISkin(){}
	virtual void SetWindowIcon(const char *fn)=0;
	virtual void GetWindowEdges(kGUICorners *c)=0;
	virtual void GetWindowButtonPositions(int allow,kGUICorners *c,kGUICorners *wclose,kGUICorners *wfull,kGUICorners *wminimize)=0;
	virtual void GetMinimizedWindowSize(int *w,int *h)=0;
	virtual void DrawWindow(kGUIWindowObj *obj,kGUICorners *c,int allow,int over)=0;
	virtual void DrawWindowNoFrame(kGUIWindowObj *obj,kGUICorners *c)=0;
	virtual void DrawBusy(kGUICorners *c)=0;
	virtual void DrawBusy2(kGUICorners *c,int offset)=0;

	virtual void GetTabSize(int *expand,int *left,int *right,int *height)=0;
	virtual void DrawTab(kGUIText *obj,int x,int y,bool current,bool over)=0;

	virtual void GetScrollbarSize(bool isvert,int *lt,int *br,int *wh)=0;
	virtual void DrawScrollbar(bool isvert,kGUICorners *c,kGUICorners *sc,bool showends)=0;
	virtual int GetScrollbarWidth(void)=0;
	virtual int GetScrollbarHeight(void)=0;

	virtual int GetMenuRowHeaderWidth(void)=0;
	virtual int GetTableRowHeaderWidth(void)=0;
	
	virtual void DrawTableRowHeader(kGUICorners *c,bool selected,bool cursor,bool add)=0;
	virtual void DrawSubMenuMarker(kGUICorners *c)=0;
	virtual void DrawMenuRowHeader(kGUICorners *c)=0;

	virtual void GetTickboxSize(int *w,int *h)=0;
	virtual void DrawTickbox(kGUICorners *c,bool scale,bool selected,bool current)=0;

	virtual void GetRadioSize(int *w,int *h)=0;
	virtual void DrawRadio(kGUICorners *c,bool scale,bool selected,bool current)=0;

	virtual int GetScrollHorizButtonWidths(void)=0;
	virtual void DrawScrollHoriz(kGUICorners *c)=0;

	virtual int GetComboArrowWidth(void)=0;
	virtual void DrawComboArrow(kGUICorners *c)=0;
};

typedef struct
{
	unsigned int c;
	int lx,ty,rx,by;
	kGUIColor fgcol;
}PRINTFONTCHAR_DEF;

char lc(char c);
char uc(char c);
char *strstri(const char *lword,const char *sword);
unsigned char strcmpin(const char *lword,const char *sword,int n);

/* supported languages for translated gui text, add new language support to SetLanguage code */
/* text is in kguiloc.txt and files _text.h and _text.cpp are generated by kguilocstr */

enum
{
KGUILANG_ENGLISH,
KGUILANG_FRENCH,
KGUILANG_GERMAN,
KGUILANG_SWEDISH,
KGUILANG_CHINESE,
KGUILANG_JAPANESE,
KGUILANG_NUMLANGUAGES
};

class kGUISystem
{
public:
	kGUISystem() {}
	virtual ~kGUISystem() {}
	virtual void FileShow(const char *filename)=0;
	virtual void ShellExec(const char *program,const char *parms,const char *dir)=0;
	virtual bool MakeDirectory(const char *name)=0;
	virtual void Copy(kGUIString *s)=0;
	virtual void Paste(kGUIString *s)=0;
	virtual void Sleep(int ms)=0;
	virtual long FileTime(const char *fn)=0;
	virtual void HideWindow(void)=0;
	virtual void ShowWindow(void)=0;
	virtual void ReDraw(void)=0;
	virtual void ChangeMouse(void)=0;
	virtual void ShowMouse(bool show)=0;
	virtual void GetWindowPos(int *x,int *y,int *w,int *h)=0;
	virtual void SetWindowPos(int x,int y)=0;
	virtual void SetWindowSize(int w,int h)=0;
	virtual void AdjustMouse(int dx,int dy)=0;
	virtual void Minimize(void)=0;
	virtual bool IsDir(const char *fn)=0;

	virtual bool NeedRotatedSurfaceForPrinting(void)=0;
	virtual unsigned int GetNumPrinters(void)=0;
	virtual void GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv)=0;
	virtual int GetDefPrinterNum(void)=0;
	virtual kGUIPrinter *GetPrinterObj(int pid)=0;

	virtual class kGUIPrintJob *AllocPrintJob(void)=0;
private:
};

class kGUIPrintJob
{
public:
	virtual ~kGUIPrintJob() {}
	virtual bool Start(kGUIPrinter *p,const char *jobname,int numpages,int numcopies,double ppi,double lm,double rm,double tm,double bm,kGUIDrawSurface *surface,bool landscape)=0;
	virtual void StartPage(void)=0;
	virtual void PrintSurface(kGUIDrawSurface *surface)=0;
	virtual void DrawRect(int lx,int rx,int ty,int by,kGUIColor color)=0;
	virtual void DrawTextList(int pointsize,const char *fontname,int num, Array<PRINTFONTCHAR_DEF> *list)=0;
	virtual void DrawImage(int lx,int rx,int ty,int by)=0;
	virtual void EndPage(void)=0;
	virtual bool End(void)=0;
protected:
	kGUIPrinter *m_p;
	bool m_error;
	/* pixels per inch on printer */
	int m_ppiw,m_ppih;
	/* margins in pixels */
	int m_lmp;
	int m_rmp;
	int m_tmp;
	int m_bmp;
	/* draw area in pixels */
	int m_dw;
	int m_dh;
	/* surface size */
	int m_sw;
	int m_sh;
	/* scale factor to convert from surface pixels to printer pixels */
	double m_scalew;
	double m_scaleh;
	/* landscape mode */
	bool m_landscape;
};

/* used for draw functions that use sub pixel ( anti-alias ) rendering */
typedef struct SUBLINEPIX_DEF
{
	SUBLINEPIX_DEF *next;
	double weight;
	double leftx,width;
}SUBLINEPIX_DEF;

typedef struct
{
	SUBLINEPIX_DEF *chunk;
	double leftx,rightx;
}SUBLINE_DEF;

#include "heap.h"

class kGUISubPixelCollector
{
public:
	kGUISubPixelCollector();
	void SetGamma(double g);
	void SetBounds(double y1,double y2);
	void SetColor(kGUIColor c,double alpha);
	void AddRect(double x,double y,double w,double h,double weight);
	void Draw(void);
private:
	void AddChunk(int y,double lx,double rx,double weight);
	double m_gamma256[256+1];
	kGUIColor m_color;
	double m_red;
	double m_green;
	double m_blue;
	double m_alpha;
	int m_topy;
	int m_bottomy;
	Array<SUBLINE_DEF>m_lines;
	Heap m_chunks;
	int m_chunkindex;
};

typedef struct
{
	unsigned int numlangs;
	unsigned int numstrings;
	const unsigned char **strings;
}LOCSTRING_DEF;

class kGUILocStrings
{
public:
	kGUILocStrings();
	~kGUILocStrings();
	void Init(LOCSTRING_DEF *loc);
	void SetLanguage(unsigned int lang) {m_lang=lang;m_langoff=lang*m_numstrings;}
	unsigned int GetLanguage(void) {return m_lang;}
	kGUIString *GetString(unsigned int word) {return &m_strings[m_langoff+word];}
	kGUIString *GetString(unsigned int lang,unsigned int word) {return &m_strings[(lang*m_numstrings)+word];}
	unsigned int GetNumLanguages(void) {return m_numlangs;}
private:
	unsigned int m_lang;
	unsigned int m_langoff;
	unsigned int m_numlangs;
	unsigned int m_numstrings;
	kGUIString *m_strings;
};

/* used by the polygon draw code */
typedef struct
{		/* a polygon edge */
    double x;	/* x coordinate of edge's intersection with current scanline */
	double dx;	/* change in x with respect to y */
	int i;	/* edge number: edge i goes from pt[i] to pt[i+1] */
} Edge;

class kGUICookieJar;
class kGUISSLManager;

class kGUI
{
public:
	kGUI();
	static void Trace(const char *fmt,...);	/* print to start/error log */
	static void SetTrace(bool t) {m_trace=t;}
	static bool GetTrace(void) {return m_trace;}
	static bool Init(kGUISystem *sys,int width,int height,int fullwidth,int fullheight,int maximages=200);
	static void Close(void);
	static void Tick(int et);
	static int GetET(void) {return m_et;}
	static int GetFrame(void) {return m_frame;}
	/* Window stuff */
	static void AddWindow(kGUIObj *window);
	static void DelWindow(kGUIObj *window);
	static bool AmITheTopWindow(kGUIObj *window);
	static bool StayTopWindow(void);
	static void TopWindow(kGUIObj *window);			/* bring window to top */

	static kGUIWindowObj *GetBackground(void) {return m_backgroundobj;}

	/* used for mouse control on a window/slider etc */
	static void ClearActiveStack(void) {m_activeobj=0;m_activeindex=0;}
	static void PushActiveObj(kGUIObj *obj);
	static void PopActiveObj(void);
	static kGUIObj *GetActiveObj(void) {return m_activeobj;}
	static void RestoreActiveObj(kGUIObj *obj) {m_activeobj=obj;}
	static kGUIObj *GetCurrentObj(void);

	/* keyboard */
	static void KeyPressed(int k) {m_keys.SetEntry(m_numkeys++,k);}
	static void SetKeyShift(bool s) {m_keyshift=s;}
	static bool GetKeyShift(void) {return m_keyshift;}
	static void SetKeyControl(bool c) {m_keycontrol=c;}
	static bool GetKeyControl(void) {return m_keycontrol;}
	static int GetKey(void) {return m_numkeys?m_keys.GetEntry(0):0;}
	static void ClearKey(void) {m_keys.SetEntry(0,0);}
	static void EatKey(void) {if(m_numkeys){m_keys.DeleteEntry(0,1);--m_numkeys;}}
	static void SetForceUsed(bool f) {m_forceused=f;}

	static void RestoreScreenSurface(void) {m_currentsurface=&m_screensurface;}

	static int GetSurfaceWidth(void) { return (m_currentsurface->GetWidth());}
	static int GetSurfaceHeight(void) { return (m_currentsurface->GetHeight());}
	static kGUIPrintJob *GetPrintJob(void) {return m_currentsurface->GetPrintJob();}

	/* draw Functions */
	static bool GetFastDraw(void) {return m_fastdraw;}
	static void SetFastDraw(bool f) {m_fastdraw=f;}
	static kGUIColor *Draw(kGUICorners *c);	/* renders screen and returns pointer to bitmap screen */
	static inline kGUIColor *GetSurfacePtr(int x,int y) {return m_currentsurface->GetSurfacePtr(x,y);}
	static inline kGUIColor *GetSurfacePtrC(int x,int y) {return m_currentsurface->GetSurfacePtrC(&m_clipcorners,x,y);}
	static inline kGUIColor *GetSurfacePtrABS(int x,int y) {return m_currentsurface->GetSurfacePtrABS(x,y);}

	static void HSVToRGB(double h,double s,double v, unsigned char *pr, unsigned char *pg, unsigned char *pb);
	static void RGBToHSV(unsigned char cr, unsigned char cg, unsigned char cb,double *ph,double *ps,double *pv);
	static void DrawRect(int x1,int y1,int x2,int y2,kGUIColor color);
	static void DrawRect(int x1,int y1,int x2,int y2,kGUIColor color,double alpha);
	static void DrawCircle(int x,int y,int r,kGUIColor color,double alpha);
	static void DrawCircleOutline(int x,int y,int r,int thickness,kGUIColor color,double alpha);
	static void DrawDotRect(int x1,int y1,int x2,int y2,kGUIColor color1,kGUIColor color2);
	static void DrawRectBevel(int lx,int ty,int rx,int by,bool pressed);
	static void DrawRectBevelIn(int lx,int ty,int rx,int by);
	static void DrawRectBevelIn(int lx,int ty,int rx,int by,kGUIColor c);
	static void DrawButtonFrame(int lx,int ty,int rx,int by,bool pressed);
	static void DrawCurve(int x,int y,int r,int t,double sa,double ea,double sv,double ev);
	static void DrawRectFrame(int lx,int ty,int rx,int by,kGUIColor icolor,kGUIColor ocolor,double alpha=1.0f);
	static void DrawLineRect(int lx,int ty,int rx,int by,const unsigned char *image);
	static bool DrawLine(int x1,int y1,int x2,int y2,kGUIColor c);
	static void DrawPixel(int x,int y,kGUIColor c);
	static bool DrawLine(int x1,int y1,int x2,int y2,kGUIColor c,double alpha);
	static void DrawCurrentFrame(int lx,int ty,int rx,int by);

	/* mouse functions */
	static void SetMouse(int x,int y,int wheeldelta,bool left,bool right);
	inline static int GetMouseX(void) {return m_mousex;}
	inline static int GetMouseY(void) {return m_mousey;}
	inline static void ClearMouseWheelDelta(void) {m_mousewheeldelta=0;}
	/* this is only called when the main window is moved */
	inline static void AdjustMouse(int dx,int dy) {m_mousex+=dx;m_mousey+=dy;m_sys->AdjustMouse(dx,dy);}
	inline static void SetNoMouse(bool nomouse) {m_nomouse=nomouse;}
	inline static int GetMouseWheelDelta(void) {return m_nomouse?0:m_mousewheeldelta;}
	inline static int GetMouseDX(void) {return m_nomouse?0:m_dmousex;}
	inline static int GetMouseDY(void) {return m_nomouse?0:m_dmousey;}
	inline static bool GetMouseLeft(void) {return m_nomouse?false:m_mouseleft;}
	inline static bool GetMouseRight(void) {return m_nomouse?false:m_mouseright;}
	inline static bool GetMouseClick(void) {return (GetMouseClickLeft()||GetMouseClickRight());}
	inline static bool GetMouseClickLeft(void) {return (m_nomouse?false:m_xmouseleft&m_mouseleft);}
	inline static bool GetMouseClickRight(void) {return (m_nomouse?false:m_xmouseright&m_mouseright);}
	inline static bool GetMouseReleaseLeft(void) {return (m_nomouse?false:m_xmouseleft&(!m_mouseleft));}
	inline static bool GetMouseReleaseRight(void) {return (m_nomouse?false:m_xmouseright&(!m_mouseright));}
	inline static bool GetMouseDoubleClickLeft(void) {return (m_nomouse?false:m_mouseleftdoubleclick);}
	inline static bool GetMouseDoubleClickRight(void) {return (m_nomouse?false:m_mouseleftdoubleclick);}
	inline static bool GetMouseDoubleClick(void) {return (GetMouseDoubleClickLeft() || GetMouseDoubleClickRight());}

	/* Update objects because of input */
	static void UpdateInput(void);

	/* Screen/Dirty update list functions */

	static void Dirty(const kGUICorners *c);
	static void ResetClip(void);
	static void SetClip(void);
	static void PushClip(void);
	static void PopClip(void);
	static void ShrinkClip(int lx,int ty,int rx,int by);
	inline static void ShrinkClip(const kGUICorners *c) {ShrinkClip(c->lx,c->ty,c->rx,c->by);}
	inline static bool ValidArea(const kGUICorners *c) {return((c->lx<c->rx) && (c->ty<c->by));}
	inline static bool ValidClip(void) {return(ValidArea(&m_clipcorners));}
	static bool OffClip(int lx,int ty,int rx,int by);
	inline static const kGUICorners *GetClipCorners(void) {return &m_clipcorners;}
	/* Zone functions */
	static bool Overlap(int x,int y,const kGUICorners *c1);
	static bool Overlap(const kGUICorners *c1,const kGUICorners *c2);
	static bool Inside(const kGUICorners *c1,const kGUICorners *c2);
	inline static bool MouseOver(const kGUICorners *c) {return(Overlap(kGUI::GetMouseX(),kGUI::GetMouseY(),c));}

	/* misc functions */
	static void FindMinimizeSpot(kGUICorners *corners);
	static bool WantHint(void) {return m_askhint;}
	static void SetHintString(int x,int y,const char *hint);
	static void DrawHint(void);

	/* this flag is set when calling the draw functions on objects */
	/* that pop up areas over other items */
	/* in this case the objects draw function is called twice, once with */
	/* ispop set to false, then again with it set to true */
	static void Check(kGUIObj *obj,kGUICorners *c);

	static const char *GetVersion(void) {return "Build:" __DATE__;}
	static int GetJpegVersion(void) {return jpegversion;}
	static const char *GetPngVersion(void) {return pngversion;}
	static const char *GetFFMpegVersion(void);

	/* built in list of colors, used by browser and art programs */
	static unsigned int GetNumColors(void);
	static const char *GetColorName(int index);
	static kGUIColor GetColor(int index);

	/* font functions */
	static bool InitFontEngine(void);
	static void AddFontDir(const char *path);
	static const char *GetFTVersion(void) {return m_ftversion.GetString();}
	static void CloseFontEngine(void);
	static unsigned int GetNumFonts(void);
	static void GetFontName(unsigned int index,kGUIString *s);

	/* return handle number of font 0=error */
	static int LoadFont(const char *filename);
	static void SelectFont(class kGUIFace *face,unsigned int size);
	static void SetDefFontID(int s) {m_deffontid=s;}
	static int GetDefFontID(void) {return m_deffontid;}
	static void SetDefFontSize(int s) {m_deffontsize=s;}
	static int GetDefFontSize(void) {return m_deffontsize;}

	static void SetDefReportFontID(int s) {m_defreportfontid=s;}
	static int GetDefReportFontID(void) {return m_defreportfontid;}
	static void SetDefReportFontSize(int s) {m_defreportfontsize=s;}
	static int GetDefReportFontSize(void) {return m_defreportfontsize;}

	/* cursor flag functions */
	static bool GetDrawCursor(void) {return m_drawcursor;}
	static bool GetDrawCursorChanged(void) {return m_drawcursorchanged;}

	static void SetImageSizeCacheFilename(char *fn);
	static kGUIImageSizeCache *GetImageSizefromCache(const char *fn) {return (kGUIImageSizeCache *)(m_imagesizehash.Find(fn));}
	static void SetImageSizeToCache(const char *fn,kGUIImageSizeCache *data) {m_imagesizehash.Add(fn,data);}
	static void LoadImageSizeCache(void);
	static void SaveImageSizeCache(void);
	static void CallAAParents(kGUIObj *obj);
	static void SetCurrentSurface(kGUIDrawSurface *s) {m_currentsurface=s;}
	static kGUIDrawSurface *GetCurrentSurface(void) {return m_currentsurface;}
	
	static void RotatePoints(int nvert,kGUIPoint2 *inpoints,kGUIPoint2 *outpoints,double angle);
	static void TranslatePoints(int nvert,kGUIPoint2 *inpoints,kGUIPoint2 *outpoints,kGUIPoint2 *xlate);
	static double CalcAngle(double dx,double dy);
	static int FastHypot(int dx,int dy);

	/* integer coords draws */

	static void DrawPoly(int nvert,kGUIPoint2 *point,kGUIColor c);
	static void DrawPoly(int nvert,kGUIPoint2 *point,kGUIColor c,double alpha);
	static bool ReadPoly(int nvert,kGUIPoint2 *point,kGUIColor c);
	static void DrawPolyLine(int nvert,kGUIPoint2 *point,kGUIColor c);
	static void DrawFatLine(int x1,int y1,int x2,int y2,kGUIColor c,double radius,double alpha=1.0f);
	static void DrawFatPolyLine(unsigned int nvert,kGUIPoint2 *point,kGUIColor c,double radius,double alpha=1.0f);
	static Array<int>m_polysortint;
	static Array<Edge>m_polysortedge;

	/* double coord draws, anti-aliased edges */

	static bool DrawLine(double x1,double y1,double x2,double y2,kGUIColor c,double alpha=1.0f);
	static void DrawPoly(int nvert,kGUIDPoint2 *point,kGUIColor c,double alpha=1.0f);
	static void DrawPolyLine(int nvert,kGUIDPoint2 *point,kGUIColor c);
	static void DrawFatLine(double x1,double y1,double x2,double y2,kGUIColor c,double radius,double alpha=1.0f);
	static void DrawFatPolyLine(unsigned int nvert,kGUIDPoint2 *point,kGUIColor c,double radius,double alpha=1.0f);

	static bool PointInsidePoly(double px,double py,int nvert,kGUIPoint2 *point);
	static bool PointInsidePoly(double px,double py,int nvert,kGUIDPoint2 *point);


	/* filehandling and bigfile entry points */
	static void FileDelete(const char *filename);
	static bool FileExists(const char *filename);
	static void FileRename(const char *from,const char *to);
	static long long FileSize(const char *filename);
	static long FileCRC(const char *filename);
	static void FileCopy(const char *fromname,const char *toname);

	/* base64 encode/decode */
	static unsigned int Base64Decode(unsigned int insize,Array<unsigned char>*in,Array<unsigned char>*out);
	static unsigned int Base64Encode(unsigned int insize,Array<unsigned char>*in,Array<unsigned char>*out);

	static unsigned char *LoadFile(const char *filename,unsigned long *filesize=0);

	static void SetInputCallback(void *codeobj,void (*code)(void *)) {m_inputcallback.Set(codeobj,code);}

	/* URL manipulation */
	static void ExtractURL(kGUIString *url,kGUIString *parentbase,kGUIString *parentroot);
	static void MakeURL(kGUIString *parent,kGUIString *in,kGUIString *out);
	static void MakeURL(kGUIString *parentbase,kGUIString *parentroot,kGUIString *in,kGUIString *out);

	/* shut down the program */
	static void CloseApp(void) {m_closeapp=true;}
	static bool IsAppClosed(void) {return m_closeapp;}

	static void SetTempMouseCursor(int mid) {m_mousecursor=mid;m_tempmouse=3;ChangeMouse();}
	static void SetMouseCursor(int mid) {m_mousecursor=mid;m_tempmouse=-1;ChangeMouse();}
	static int GetMouseCursor(void) {return m_mousecursor;}
	static int GetTempMouse(void) {return m_tempmouse;}
	static void SetTempMouse(int t) {m_tempmouse=t;}

	/* events */
	/*! Add event to update event list called once per UpdateInput
	    @param codeobj pointer to object
		@param code pointer to function */
	static void AddEvent(void *codeobj,void (*code)(void *));
	/*! Remove event from update event list
	    @param codeobj pointer to object
		@param code pointer to function */
	static void DelEvent(void *codeobj,void (*code)(void *));

	/*! Set the size of the screen
	    @param w width in pixels
		@param h height in pixels */
	static void SetScreenSize(int w,int h) {m_screenwidth=w;m_screenheight=h;}
	/*! @return screen width in pixels */
	static int GetScreenWidth(void) {return m_screenwidth;}
	/*! @return screen height in pixels */
	static int GetScreenHeight(void) {return m_screenheight;}

	/*! @return full screen width in pixels */
	static int GetFullScreenWidth(void) {return m_fullscreenwidth;}
	/*! @return full screen width in pixels */
	static int GetFullScreenHeight(void) {return m_fullscreenheight;}

	/*! set the skin render object
		@param skin pointer to the skin class object */
	static void SetSkin(kGUISkin *skin) {m_skin=skin;}
	/*! get the current skin render object
		@return pointer to the skin class object */
	static kGUISkin *GetSkin(void) {return m_skin;}
	/*! this should be called after chaning the skin object to trigger a screen re-layout and re-draw */
	static void SkinChanged(void);

	/*! get random number from the system random number pool
	    @param buf pointer to buffer to store the random numbers
	    @param nbytes number of bytes to get */
	static void GetRandom(char * buf,  int nbytes) {m_random->ExtractEntropy(buf,nbytes);}

	/*! set the global cookie handling object, this is used by the download class,
	    @param jar pointer to the jar class object */
	static void SetCookieJar(kGUICookieJar *jar) {m_cookiejar=jar;}
	/*! get the global cookie handling object, if null, then none is active
	    @return pointer to the current cookiejar class object */
	static kGUICookieJar *GetCookieJar(void) {return m_cookiejar;}

	/*! set the global ssl manager object, this is used by the download class,
	    @param ssl pointer to the sslmanager class object */
	static void SetSSLManager(kGUISSLManager *sslmanager) {m_sslmanager=sslmanager;}
	/*! get the global ssl manager object, if null, then none is active
	    @return pointer to the current sslmanager class object */
	static kGUISSLManager *GetSSLManager(void) {return m_sslmanager;}

	/*! This needs to be called by threads before they can get exclusive access any of the static kgui functions */
	static void GetAccess(void);
	/*! This needs to be called by threads before they can get exclusive access any of the static kgui functions, and only if they got true as a result */
	static bool TryAccess(void);
	/*! This is called to release exclusive access to the static kgui functions. */
	static void ReleaseAccess(void);

	/*! Set the system class handler, this is for machine specific calls that are different for each operating system
	    @param sys pointer to the system class object */
	static void SetSystem(kGUISystem *sys) {m_sys=sys;}
	
	static bool MakeDirectory(const char *name) {return m_sys->MakeDirectory(name);}
	/* open file in default program like browser, or video player etc */
	static void FileShow(const char *filename) {m_sys->FileShow(filename);}

	static void MoveWindowPos(int dx,int dy);

	static kGUISystem *GetSystem(void) {return m_sys;}

	static void HideWindow(void) {m_sys->HideWindow();}
	static void ShowWindow(void) {m_sys->ShowWindow();}
	/* force a screen redraw */
	static void ReDraw(void) {m_allowdraw=true;m_sys->ReDraw();m_allowdraw=false;}
	static void Copy(kGUIString *s) {m_sys->Copy(s);}
	static void Paste(kGUIString *s) {m_sys->Paste(s);}
	static void GetWindowPos(int *x,int *y,int *w,int *h) {m_sys->GetWindowPos(x,y,w,h);}
	static void SetWindowPos(int x,int y) {m_sys->SetWindowPos(x,y);}
	static void SetWindowSize(int w,int h) {m_sys->SetWindowSize(w,h);}
	static void Minimize(void) {m_sys->Minimize();}
	static void ChangeMouse(void) {m_sys->ChangeMouse();}
	static void ShowMouse(bool show) {m_sys->ShowMouse(show);}
	static long SysFileTime(const char *fn) {return m_sys->FileTime(fn);}
	static long FileTime(const char *fn);
	static void ShellExec(const char *program,const char *parms,const char *dir=0) {m_sys->ShellExec(program,parms,dir);}
	static void Sleep(int ms) {m_sys->Sleep(ms);}
	static bool IsDir(const char *fn) {return m_sys->IsDir(fn);}

	/* printer interface */
	static bool NeedRotatedSurfaceForPrinting(void) {return m_sys->NeedRotatedSurfaceForPrinting();}
	static unsigned int GetNumPrinters(void) {return m_sys->GetNumPrinters();}
	static void GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv) {m_sys->GetPrinterInfo(name,pw,ph,ppih,ppiv);}
	static int GetDefPrinterNum(void) {return m_sys->GetDefPrinterNum();}
	static kGUIPrinter *GetPrinterObj(int pid) {return m_sys->GetPrinterObj(pid);}

	static int LocatePrinter(const char *name,bool ordefault);

	/*! split a filename into it's path and name components
	    @param longfn - input - full filename for example c:/aaa/bbb/ccc.jpg 
		@param path - output - path component for example c:/aaa/bbb/ 
		@param shortfn - output - name component for example ccc.jpg */
	static void SplitFilename(kGUIString *longfn,kGUIString *path,kGUIString *shortfn);
	/*! split a filename and extract it's path
	    @param longfn - input - full filename for example c:/aaa/bbb/ccc.jpg 
		@param path - output - path component for example c:/aaa/bbb/ */
	static void ExtractPath(kGUIString *longfn,kGUIString *path);
	/*! split a filename and extract it's short name
	    @param longfn - input - full filename for example c:/aaa/bbb/ccc.jpg 
		@param shortfn - output - name component for example ccc.jpg */
	static void ExtractFilename(kGUIString *longfn,kGUIString *shortfn);
	/*! make a full filename by appending the path and short filename, with an appropriate seperator between them
	    @param path - input the path example c:/a
		@param shortfn - input the short filename b.jpg
		@param longfn - output the newly generated filename c:/a/b.gif */
	static void MakeFilename(kGUIString *path,kGUIString *shortfn,kGUIString *longfn);

	/*! return pointer to the root render object */
	static kGUIContainerObj *GetRootObject(void) {return m_rootobj;}

	/* set default strings for supported language */
	static void SetLanguage(unsigned int lang) {m_locstrings.SetLanguage(lang);}
	static unsigned int GetLanguage(void) {return m_locstrings.GetLanguage();}
	static kGUIString *GetString(unsigned int word) {return m_locstrings.GetString(word);}
	static kGUIString *GetString(unsigned int lang,unsigned int word) {return m_locstrings.GetString(lang,word);}

	/* set the panic function, call this if the program encounters a fatal error */
	static void SetPanic(void *o,void (*f)(void *,kGUIString *s)) {m_panic.Set(o,f);}
	static void Panic(kGUIString *s) {m_panic.CallOnce(s);}

	static kGUIObj *m_forcecurrentobj;
	/* current clip corners */
	static kGUICorners m_clipcorners;
	static kGUIDCorners m_clipcornersd;
	static kGUISubPixelCollector m_subpixcollector;
private:
	static kGUISystem *m_sys;
	static kGUICallBackPtr<kGUIString> m_panic;
	static kGUIDrawSurface *m_currentsurface;
	static kGUIDrawSurface m_screensurface;

	/* all objects are linked to this base root object */
	static kGUIRootObj *m_rootobj;
	static kGUIWindowObj *m_backgroundobj;	/* one background object */

	/* strings container */
	static kGUILocStrings m_locstrings;

	/* current active object */
	static int m_activeindex;
	static kGUIObj *m_activeobjstack[MAXACTIVE];
	static kGUIObj *m_activeobj;
	/* pointer to popup draw fuction for current active object */

	/* object the cursor is over */
	static int m_mousecursor;
	static int m_tempmouse;	/* change back to default after 1 frame */

	static kGUIMutex m_busymutex;
	static bool m_allowdraw;
	static bool m_forceused;

	/* current mouse position and button status*/
	static bool m_nomouse;	/* if true, all mouse calls return false */
	static int m_mousex;
	static int m_mousey;
	static int m_mousewheeldelta;
	static bool m_mouseleft;
	static bool m_mouseright;
	static bool m_xmouseleft;
	static bool m_xmouseright;
	static bool m_mouseleftdoubleclick;
	static bool m_mouserightdoubleclick;

	/* tick button was last released on */
	static kGUIDelay m_mouserightrelesetick;
	static kGUIDelay m_mouseleftrelesetick;

	static bool m_inputidle;
	static int m_hinttick;
	static bool m_askhint;
	static bool m_sethint;
	static bool m_closeapp;

	static kGUIText m_hinttext;
	static kGUICorners m_hintcorners;	/* valid only when hint on screen */

	/*  mouse position and button status*/
	static int m_lmousex;
	static int m_lmousey;
	static bool m_lmouseleft;
	static bool m_lmouseright;

	static int m_dmousex;	/* delta */
	static int m_dmousey;

	/* dirty zone array variables */
	static int m_dirtyindex;
	static kGUICorners m_dirtycorners[MAXDIRTY];
	/* clip stack */
	static int m_clipindex;

	static int m_numevents;
	static Array<kGUICallBack *>m_events;

	static kGUICorners m_clipstack[MAXCLIPS];
	static int m_et;
	static int m_frame;
	static kGUIDelay m_flash;

	static int m_numkeys;		/* number of keys in input buffer */
	static Array<int>m_keys;	/* array of keys in the input buffer */
	static bool m_keyshift;	/* true=shift key pressed */
	static bool m_keycontrol;	/* true=control key pressed */

	static bool m_fastdraw;
	static bool m_drawcursorchanged;	/* just changed flag, used to "dirty" things with cursors */
	static bool m_drawcursor;
	static int m_deffontid;
	static int m_deffontsize;
	static int m_defreportfontid;
	static int m_defreportfontsize;
	static kGUIString m_ftversion;	/* freetype version */
	static int m_fullscreenwidth,m_fullscreenheight;
	static int m_screenwidth,m_screenheight;

	static char *m_imagesizefilename;
	static Hash m_imagesizehash;
	static Array<kGUIPoint2>m_fatpoints;
	static Array<kGUIDPoint2>m_dfatpoints;

	static bool m_trace;
	static kGUICallBack m_inputcallback;
	static kGUISkin *m_skin;			/* pointer to current gui skin */
	static kGUIRandom *m_random;		/* default random number generator */
	static kGUICookieJar *m_cookiejar;
	static kGUISSLManager *m_sslmanager;
};

class kGUIBitStream
{
public:
	kGUIBitStream() {m_revi=false;m_revo=false;}
	void SetReverseIn(void) {m_revi=true;}		/* set this before calling set */
	void SetReverseOut(void) {m_revo=true;}			/* set this before calling set */
	void Set(const void *buf);
	unsigned int ReadU(int nbits);	/* unsigned */
	int ReadS(int nbits);	/* signed */
	int NumRead(void);	/* number of bits read since last set commend */

	/* these read and then order the bits in the reverse sequence */
	unsigned int XReadU(int nbits);	/* unsigned */
	int XReadS(int nbits);	/* signed */
private:
	const unsigned char *m_start;
	const unsigned char *m_buf;
	int m_bit;
	int m_bits;	/* counts down frm 8 to 0 */
	bool m_revi:1;
	bool m_revo:1;
};

/****************************************************************************/

/* used for reading / writing data between two threads */

template <class T>
class kGUICommStack
{
public:
	kGUICommStack() {m_readhead=0;m_writetail=0;m_numrecords=0;}
	void Init(int numrecords) {m_readhead=0;m_writetail=0;m_array.Alloc(numrecords);m_numrecords=numrecords;}
	bool GetIsEmpty(void) {return (m_readhead==m_writetail);}
	bool Read(T *obj) {if(m_readhead==m_writetail)return(false);*obj=m_array.GetEntry(m_readhead);if(++m_readhead==m_numrecords)m_readhead=0;return(true);}
	bool Write(T *obj) {int wi=m_writetail+1;if(wi==m_numrecords)wi=0;if(wi==m_readhead) return(false);m_array.SetEntry(m_writetail,*obj);m_writetail=wi;return(true);}
private:
	int m_numrecords;
	int m_readhead;
	int m_writetail;
	Array<T>m_array;
};

/* bar true = size changes to show progress, bar = false means animates as size stays the same */
class kGUIBusyRectObj : public kGUIObj
{
public:
	kGUIBusyRectObj() {m_offset=0;m_isbar=true;}
	void SetIsBar(bool isbar) {m_isbar=isbar;}
	void Animate(void) {m_offset+=kGUI::GetET();Dirty();}
	void Draw(void);
	bool UpdateInput(void) {return true;}
private:
	int m_offset;	/* used for animating bar only */
	bool m_isbar;
};


class kGUIBusy
{
public:
	kGUIBusy(int w);
	~kGUIBusy();
	kGUIText *GetTitle(void) {return m_window.GetTitle();}
	void SetCur(int v);
	void SetMax(int v);
private:
	kGUIWindowObj m_window;
	kGUIBusyRectObj m_busyrect;
	int m_w;
	int m_lastw;
	int m_max;
};

/****************************************************************************/

#endif
