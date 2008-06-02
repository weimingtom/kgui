/**********************************************************************************/
/* kGUI - heap.cpp                                                                */
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
    @brief Heap is just a simple class for linearly releasing small chunks of allocated
	memory (internally it allocates them in large chunks). It is used by the Hash table
	code or any other object that needs small chunks of data and can free them all at once
	and not free them individually */

/* used for array template */
#include "kgui.h"
#include "heap.h"

Heap::Heap()
{
	m_numblocks=0;
	m_blocksize=65536;	/* default to 64k */
	m_blocks.Init(16,4);
}

/* reset heap to all blocks empty */
void Heap::Reset(void)
{
	int i;
	HEAP_RECORD *hr;

	for(i=0;i<m_numblocks;++i)
	{
		hr=m_blocks.GetEntry(i);
		hr->m_used=0;
		hr->m_left=hr->m_size;
	}
}

void Heap::Purge(void)
{
	int i;
	HEAP_RECORD *hr;

	for(i=0;i<m_numblocks;++i)
	{
		hr=m_blocks.GetEntry(i);
		delete []hr;
	}
	m_numblocks=0;
}

void *Heap::Alloc(int size)
{
	int i;
	HEAP_RECORD *hr;
	int blocksize;
	char *place;

	/* align to 4 bytes */
	if(size&3)
		size+=4-(size&3);

	/* start with last block and if no room, look at previous blocks */
	i=m_numblocks-1;
	while(i>=0)
	{
		hr=m_blocks.GetEntry(i);
		if(size<=hr->m_left)
		{
			/* space found here! */
			place=((char *)(hr+1))+hr->m_used;
			hr->m_used+=size;
			hr->m_left-=size;
			return(place);
		}
		--i;
	}

	/* if requested size is greater than the blocksize then allocate a larger block */
	if(size>m_blocksize)
		blocksize=size;
	else
		blocksize=m_blocksize;

	/* no room in any blocks, allocate a new block */
	hr=(HEAP_RECORD *)new char[sizeof(HEAP_RECORD)+blocksize];
	hr->m_used=0;
	hr->m_size=blocksize;
	hr->m_left=blocksize;
	m_blocks.SetEntry(m_numblocks++,hr);

	hr->m_used=size;
	hr->m_left-=size;

	return(hr+1);	/* return pointer past header to data */
}

