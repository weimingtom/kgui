#ifndef __HEAP__
#define __HEAP__

/**********************************************************************************/
/* kGUI - heap.h                                                                  */
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

/*! @file heap.h
    @brief Heap is just a simple class for linearly releasing small chunks of allocated
	memory (internally it allocates them in large chunks). It is used by the Hash table
	code or any other object that needs small chunks of data and can free them all at once
	and not free them individually */

/*! @internal @struct HEAP_RECORD
	@brief Heap Chunk structure used by the Heap class
    @ingroup Heap */
typedef struct
{
	int m_size;
	int m_used;
	int m_left;
}HEAP_RECORD;

/*! @class Heap
	@brief A Heap class for allocating small chunks of memory
    @ingroup Heap */
class Heap
{
public:
	Heap();
	~Heap() {Purge();}
	/*! Set the large block size in bytes */
	/*! @param b blocksize in bytes */
	void SetBlockSize(int b) {m_blocksize=b;}
	/*! Start Again */
	void Reset(void);
	/*! Purge all data */
	void Purge(void);
	/*! Return a chunk of data from the Heap */
	void *Alloc(int size);
private:
	/*! size in bytes for future allocated blocks */
	int m_blocksize;
	/*! number of allocated blocks */
	int m_numblocks;
	/*! array of allocated blocks */
	Array<HEAP_RECORD *>m_blocks;
};

#endif
