#ifndef __VHEAP__
#define __VHEAP__

/**********************************************************************************/
/* kGUI - vheap.h                                                                  */
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

/*! @file vheap.h
    @brief VHeap is... */

typedef unsigned long long vheap_offset;

/*! @internal @struct VHEAP_RECORD
	@brief VHeap Chunk structure used by the VHeap class
    @ingroup VHeap */
class VHEAP_BLOCK
{
private:
	friend class VHeap;
	VHEAP_BLOCK *m_prev;
	VHEAP_BLOCK *m_next;
	vheap_offset m_offset;
	unsigned char *m_addr;	/* only valid for active blocks, null for cached blocks */
	bool m_dirty;
	bool m_new;
};

/*! @class VHeap
	@brief A VHeap class for allocating small chunks of memory
    @ingroup VHeap */
class VHeap
{
public:
	VHeap();
	~VHeap();
	void Init(unsigned int blocksizepow2,unsigned int maxblocksinmem,const char *filename);
	vheap_offset Alloc(unsigned int numbytes);
	void Write(const void *data,vheap_offset offset,unsigned int numbytes);
	void Read(void *data,vheap_offset offset,unsigned int numbytes);
private:
	VHEAP_BLOCK *Activate(unsigned int blocknum);

	FILE *m_file;
	unsigned int m_blocksizepow2;
	unsigned int m_blocksizebytes;
	unsigned int m_blocksizemask;
	unsigned int m_maxblocksinmem;
	unsigned int m_numblocks;
	vheap_offset m_used;

	/*! array of allocated blocks */
	ClassArray<VHEAP_BLOCK>m_blocks;
	/*! head recrord for active blocks */
	VHEAP_BLOCK m_head;
	/*! tail record for active blocks */
	VHEAP_BLOCK m_tail;
};

#endif
