/**********************************************************************************/
/* kGUI - vheap.cpp                                                                */
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

/*! @file heap.cpp 
    @brief VHeap is .... */

/* used for array template */
#include "kgui.h"
#include "vheap.h"

VHeap::VHeap()
{
	m_blocksizepow2=0;
	m_blocksizebytes=0;
	m_blocksizemask=0;
	m_maxblocksinmem=0;
	m_numblocks=0;
	m_used=0;
	m_file=0;

	/* init active linked list to no entries */
	m_head.m_next=&m_tail;
	m_tail.m_prev=&m_head;
}

VHeap::~VHeap()
{
	VHEAP_BLOCK *vr;

	/* purge all allocated blocks */
	vr=m_head.m_next;
	while(vr!=&m_tail)
	{
		delete []vr->m_addr;
		vr=vr->m_next;
	}

	if(m_file)
		fclose(m_file);
}

void VHeap::Init(unsigned int blocksizepow2,unsigned int maxblocksinmem,const char *filename)
{
	m_blocksizepow2=blocksizepow2;
	m_blocksizebytes=1<<blocksizepow2;
	m_blocksizemask=m_blocksizebytes-1;
	m_maxblocksinmem=maxblocksinmem;

	m_file=fopen(filename,"wb+");
	assert(m_file!=0,"Error opening VHeap file for write");

	m_used=0;
	m_numblocks=0;
	m_blocks.Init(maxblocksinmem,-1);	/* double in size if too small */

	/* init active linked list to no entries */
	m_head.m_next=&m_tail;
	m_tail.m_prev=&m_head;
}

vheap_offset VHeap::Alloc(unsigned int numbytes)
{
	vheap_offset off=m_used;
	unsigned int endblock;

	m_used+=numbytes;

	/* do we need to allocate another block?? */
	endblock=(unsigned int)(m_used>>m_blocksizepow2);
	while(m_numblocks<=endblock)
	{
		/* add a new end block */
		VHEAP_BLOCK *vb;

		vb=m_blocks.GetEntryPtr(m_numblocks);
		vb->m_offset=(vheap_offset)m_numblocks<<m_blocksizepow2;
		vb->m_dirty=0;
		if(m_numblocks<m_maxblocksinmem)
		{
			VHEAP_BLOCK *next;
			/* allocate ram for this block and add to in mem linked list */
			vb->m_addr=new unsigned char[m_blocksizebytes];
			vb->m_new=false;

			/* add to head of active linked list */
			next=m_head.m_next;
			vb->m_prev=&m_head;
			vb->m_next=next;
			m_head.m_next=vb;
			next->m_prev=vb;
		}
		else
		{
			/* not active yet */
			vb->m_addr=0;
			vb->m_new=true;
			vb->m_prev=0;
			vb->m_next=0;
		}
		++m_numblocks;
	}

	return(off);
}

void VHeap::Write(const void *data,vheap_offset offset,unsigned int numbytes)
{
	unsigned int startblock;
	unsigned int endblock;
	VHEAP_BLOCK *vb;
	unsigned int boffset;
	const unsigned char *cdata=(const unsigned char *)data;

	startblock=(unsigned int)(offset>>m_blocksizepow2);
	boffset=(unsigned int)(offset&m_blocksizemask);
	endblock=(unsigned int)((offset+numbytes)>>m_blocksizepow2);

	vb=Activate(startblock);
	if(startblock==endblock)
	{
		memcpy(vb->m_addr+boffset,cdata,numbytes);
		vb->m_dirty=true;
	}
	else
	{
		unsigned int chunk;

		/* write to remainer at the end of the start block */
		chunk=(unsigned int)(m_blocksizebytes-boffset);
		memcpy(vb->m_addr+boffset,cdata,chunk);
		vb->m_dirty=true;
		cdata+=chunk;
		numbytes-=chunk;

		/* copy full blocks */
		while(numbytes>=m_blocksizebytes)
		{
			vb=Activate(++startblock);
			memcpy(vb->m_addr,cdata,m_blocksizebytes);
			vb->m_dirty=true;
			cdata+=m_blocksizebytes;
			numbytes-=m_blocksizebytes;
		}
		if(numbytes)
		{
			vb=Activate(++startblock);
			memcpy(vb->m_addr,cdata,numbytes);
			vb->m_dirty=true;
		}
	}
}

void VHeap::Read(void *data,vheap_offset offset,unsigned int numbytes)
{
	unsigned int startblock;
	unsigned int endblock;
	unsigned int boffset;
	VHEAP_BLOCK *vb;
	unsigned char *cdata=(unsigned char *)data;

	startblock=(unsigned int)(offset>>m_blocksizepow2);
	boffset=(unsigned int)(offset&m_blocksizemask);
	endblock=(unsigned int)((offset+numbytes)>>m_blocksizepow2);

	vb=Activate(startblock);
	if(startblock==endblock)
		memcpy(cdata,vb->m_addr+boffset,numbytes);
	else
	{
		unsigned int chunk;

		/* read to remainer at the end of the start block */
		chunk=(unsigned int)(m_blocksizebytes-boffset);
		memcpy(cdata,vb->m_addr+boffset,chunk);
		cdata+=chunk;
		numbytes-=chunk;

		/* copy full blocks */
		while(numbytes>=m_blocksizebytes)
		{
			vb=Activate(++startblock);
			memcpy(cdata,vb->m_addr,m_blocksizebytes);
			cdata+=m_blocksizebytes;
			numbytes-=m_blocksizebytes;
		}
		if(numbytes)
		{
			vb=Activate(++startblock);
			memcpy(cdata,vb->m_addr,numbytes);
		}
	}
}

VHEAP_BLOCK *VHeap::Activate(unsigned int blocknum)
{
	VHEAP_BLOCK *vr=m_blocks.GetEntryPtr(blocknum);
	VHEAP_BLOCK *prev;
	VHEAP_BLOCK *next;
	VHEAP_BLOCK *old;
#if defined(WIN32) || defined(MINGW)
	fpos_t pos;
#endif
	int sok;

	assert(blocknum<m_numblocks,"Error: referencing unallocated block");

	/* if we are already the first in the linked list of active blocks then return */
	if(m_head.m_next==vr)
		return(vr);

	/* are we in memory already? */
	if(vr->m_addr)
	{
		/* remove from old spot */
		prev=vr->m_prev;
		next=vr->m_next;
		prev->m_next=next;
		next->m_prev=prev;
		/* add to the top */
		next=m_head.m_next;
		vr->m_prev=&m_head;
		vr->m_next=next;
		m_head.m_next=vr;
		next->m_prev=vr;
		return(vr);
	}

	/* we are not in memory at this point, so we will */
	/* write out the last referenced block to the cache file */
	old=m_tail.m_prev;
	if(old->m_dirty)
	{
		old->m_dirty=false;
#if defined(WIN32) || defined(MINGW)
		pos=old->m_offset;
		sok=fsetpos(m_file,&pos);
#else
		sok=fseek(m_file,old->m_offset,SEEK_SET);
#endif
		assert(sok==0,"Seek error!");
		if(fwrite(old->m_addr,1,m_blocksizebytes,m_file)!=m_blocksizebytes)
		{
			assert(false,"Write Error!");
		}
	}
	/* remove it from the active list */
	prev=old->m_prev;
	next=old->m_next;
	prev->m_next=next;
	next->m_prev=prev;

	/* move the memory pointer to new block, clear it in old block */
	vr->m_addr=old->m_addr;
	old->m_addr=0;

	/* if this is a new block then we don't load it */
	if(vr->m_new)
	{
		vr->m_new=false;
		/* clear the buffer, probably not really necessary */
		memset(vr->m_addr,0,m_blocksizebytes);
		vr->m_dirty=true;
	}
	else
	{
		/* load our data back from the cache file */
#if defined(WIN32) || defined(MINGW)
		pos=vr->m_offset;
		sok=fsetpos(m_file,&pos);
#else
		sok=fseek(m_file,vr->m_offset,SEEK_SET);
#endif
		assert(sok==0,"Seek error!");
		if(fread(vr->m_addr,1,m_blocksizebytes,m_file)!=m_blocksizebytes)
		{
			assert(false,"Read Error!");
		}
		vr->m_dirty=false;
	}

	/* add us to the head of the active list */
	next=m_head.m_next;
	vr->m_prev=&m_head;
	vr->m_next=next;
	m_head.m_next=vr;
	next->m_prev=vr;
	return(vr);
}

