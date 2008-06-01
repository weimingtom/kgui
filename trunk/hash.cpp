/**********************************************************************************/
/* kGUI - hash.cpp                                                                */
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

/* used for array only */
#include "kgui.h"

#include "hash.h"
#include "heap.h"

Hash::Hash()
{
	m_casesensitive=false;
	Init();
}

void Hash::Init(void)
{
	m_heap=new Heap();
	m_table=0;
	m_tablebits=0;
	m_tablemask=0;
	m_fixedlen=0;
	m_datalen=0;
	m_num=0;
	m_head.m_prev=0;
	m_head.m_next=&m_tail;
	m_tail.m_prev=&m_head;
	m_tail.m_next=0;
}

void Hash::Init(int bits,int dl)
{
	int i,tableentries;
	
	Purge();
	Init();

	m_tablebits=bits;
	tableentries=1<<m_tablebits;
	m_table=new HashEntry *[tableentries];
	for(i=0;i<tableentries;++i)
		m_table[i]=0;
	m_tablemask=tableentries-1;
	m_datalen=dl;
}

void Hash::Reset(void)
{
	Init(m_tablebits,m_datalen);
}

Hash::~Hash()
{
	Purge();
}

void Hash::Purge(void)
{
	delete m_heap;
	m_heap=0;
	if(m_table)
	{
		delete []m_table;
		m_table=0;
	}	
	m_num=0;
}

int Hash::m_ctoi[256]={
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,24,25,0,1,2,3,4,5,
6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,
22,23,24,25,0,1,2,3,4,5,6,7,8,9,10,11,
12,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
15,16,17,18,19,20,21,22,23,24,25,13,14,15,16,17,
18,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
15,16,17,18,19,20,21,22,23,24,25,19,20,21,22,23,
24,25,0,1,2,3,4,5,6,7,8,9,10,11,12,13,
14,15,16,17,18,19,20,21,22,23,24,25,0,1,2,3,
4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
20,21,22,23,24,25,0,1,2,3,4,5,6,7,8,9,
10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,24,25,0,1,2,3,4,5,
6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};

int Hash::TableIndex(const char *string)
{
	unsigned int hashcode;
	int f;
	unsigned char c;
	const char *sp;

	sp=string;
	hashcode=0;

	if(!m_fixedlen)
	{
		while(*(sp))
		{
			c=(unsigned char)*(sp++);
			hashcode=m_ctoi[c]^(hashcode*26);			/* put in bits */
			hashcode^=hashcode>>m_tablebits;
			hashcode&=m_tablemask;
		}
	}
	else	/* binary search key of fixed length */
	{
		f=m_fixedlen;
		do
		{
			c=(unsigned char)*(sp++);
			c&=31;
			hashcode=c^(hashcode<<5);			/* put in bits */
			hashcode^=hashcode>>m_tablebits;
			hashcode&=m_tablemask;
		}while(--f);
	}
	return(hashcode);
}

#define HashLocate() \
if(!m_fixedlen)\
	{\
		if(m_casesensitive==false)\
		{\
			while(he)\
			{\
				delta=stricmp(he->m_string,string);\
				if(!delta)\
					break;		/* found */\
\
				lhe=he;\
				if(delta<0)\
					he=he->m_left;\
				else\
					he=he->m_right;\
			}\
		}\
		else\
		{\
			while(he)\
			{\
				delta=strcmp(he->m_string,string);\
				if(!delta)\
					break;		/* found */\
\
				lhe=he;\
				if(delta<0)\
					he=he->m_left;\
				else\
					he=he->m_right;\
			}\
		}\
	}\
	else\
	{\
		if(m_casesensitive==false)\
		{\
			while(he)\
			{\
				delta=strnicmp(he->m_string,string,m_fixedlen);\
				if(!delta)\
					break;		/* found */\
\
				lhe=he;\
				if(delta<0)\
					he=he->m_left;\
				else\
					he=he->m_right;\
			}\
		}\
		else\
		{\
			while(he)\
			{\
				delta=strncmp(he->m_string,string,m_fixedlen);\
				if(!delta)\
					break;		/* found */\
\
				lhe=he;\
				if(delta<0)\
					he=he->m_left;\
				else\
					he=he->m_right;\
			}\
		}\
	}\

void Hash::Add(const char *string,const void *data)
{
	HashEntry *he;
	HashEntry *lhe;
	int hashcode=TableIndex(string);
	int delta=0;
	int stringlen;
	char *allocdata;

	lhe=0;
	he=m_table[hashcode];

	HashLocate()

	if(he)
	{
			/* already exists, so just update the data area */
			memcpy(he->m_data,data,m_datalen);
	}
	else
	{
		/* entry was not found, add it */

		++m_num;

		if(!m_fixedlen)
			stringlen=(int)strlen(string)+1;
		else
			stringlen=m_fixedlen;

		allocdata=(char *)(m_heap->Alloc(sizeof(HashEntry)+m_datalen+stringlen));
		he=(HashEntry *)allocdata;
		{
			HashEntry *b;
			HashEntry *a;

			a=&m_tail;				/* record after our entry */
			b=m_tail.GetPrev();		/* record before our entry */

			he->m_left=0;
			he->m_right=0;

			b->m_next=he;
			he->m_prev=b;
			he->m_next=a;
			a->m_prev=he;
		}

		allocdata+=sizeof(HashEntry);	/* skip past class */

		if(m_datalen)
			memcpy(allocdata,data,m_datalen);
		memcpy(allocdata+m_datalen,string,stringlen);
		he->m_data=(void *)allocdata;
		he->m_string=allocdata+m_datalen;

		if(!lhe)							/* root entry */
			m_table[hashcode]=he;
		else
		{
			if(delta<0)
				lhe->m_left=he;
			else
				lhe->m_right=he;
		}
	}
}


/* this is the main find routine, it returns a pointer to the data area allocated */

void *Hash::Find(const char *string)
{
	HashEntry *he;
	HashEntry *lhe;
	int hashcode=TableIndex(string);
	int delta;

	he=m_table[hashcode];
	HashLocate()
	if(he)
		return(he->m_data);	/* found */
	return(0);
}

/* this is an alternate find routine, it returns a pointer to the string in the hash table if found */
/* it is used by the XML load code to cache tag names */

const char *Hash::FindName(const char *string)
{
	HashEntry *he;
	HashEntry *lhe;
	int hashcode=TableIndex(string);
	int delta;

	he=m_table[hashcode];
	HashLocate()
	if(he)
		return(he->m_string);	/* found */
	return(0);
}
