#ifndef __HASH__
#define __HASH__

/**********************************************************************************/
/* kGUI - hash.h                                                                  */
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

/*! @file hash.h 
    @brief Hash table class definitions. The Hash table can use case sensitive or
	non-cs strings as indexs or it can use a fixed length binary string as an index too.
	The hash table can allocate "data" for each entry if desired or just confirm 
	that an object is already present. */

/*! @class HashEntry
	@brief An Entry in the Hash Table
    @ingroup HashTable */
class HashEntry
{
public:
	HashEntry() {m_left=0;m_right=0;m_next=0;m_string=0;m_data=0;}
	HashEntry *GetNext(void) {return m_next;}
	HashEntry *GetPrev(void) {return m_prev;}
	HashEntry *m_left;
	HashEntry *m_right;
	HashEntry *m_prev;	/* this is only used for freeing them all up upon closing */ 
	HashEntry *m_next;	/* this is only used for freeing them all up upon closing */ 
	char *m_string;
	void *m_data;	/* attached user data */
};

/*! @class Hash
	@brief A Hash Table
    @ingroup HashTable */
class Hash
{
public:
	Hash();
	~Hash();
	/*! Delete all entries in the Hash table */
	void Reset(void);
	/*! Set the size of the hash table lookup array and size of the user data attached to each entry
	    @param tablebits size of the lookup table in bits, size in entries = 1<<bits
	    @param dl size in bytes of allocated data for each hash table entry (DataLength) */
	void Init(int tablebits,int dl);
	/*! Delete all entries in the Hash table */
	void Purge(void);
	/*! Set Case Sensitivity of hash table input string */
	void SetCaseSensitive(bool cs) {m_casesensitive=cs;}
	/*! Add entry to the Hash Table 
	    @param string null terminated string, or binary data if FixedLength is set 
	    @param data pointer to data to copy to Hash Table Entry, length=current DataLength, pointer ignored if DataLength=0 */
	void Add(const char *string,const void *data);
	/*! Lookup the entry in the Hash Table */
	/*! @return Return NULL if not found, else return pointer to user data for entry */
	void *Find(const char *string);
	/*! Return pointer to internally stored name for HashEntry */
	/*! @return pointer to internally stored name string or NULL if not found */
	const char *FindName(const char *string);
	/*! Get Number of Entries in the Hash Table */
	/*! @return number of entries */
	inline int GetNum(void) {return m_num;}
	/*! @return pointer to first entry in the Hash Table, use linked list to get next */
	HashEntry *GetFirst(void) {return m_head.GetNext();}
	/*! Set fixed length for input name string. Set to 0 for null terminated string  */
	void SetFixedLen(int fixed) {m_fixedlen=fixed;}
	/*! Set current DataLen for newly added Hash Entries */
	void SetDataLen(int dl) {m_datalen=dl;}
	/*! Get Hash Index for string */
	int TableIndex(const char *string);
private:
	/*! Init the Hash table */
	void Init(void);
	/*! Number of Entries in the Hash Table */
	int m_num;
	/*! Number of bits for the lookup array, size of array = 1<<bits */
	int m_tablebits;
	/*! Mask to contain index to table size */
	int m_tablemask;
	/*! Fixed length of input string, 0 = null terminated string */
	int m_fixedlen;
	/*! Size of allocated UserData for newly added items */
	int m_datalen;
	/*! Flag for input string case sensitivity */
	bool m_casesensitive;
	/*! Quick convert table for generating index */
	static int m_ctoi[256];
	/*! Head for linked list of HashEntry objects */
	HashEntry m_head;
	/*! Tail for linked list of HashEntry objects */
	HashEntry m_tail;
	/*! Initial lookup table 1<<m_tablebits in size */
	HashEntry **m_table;
	/*! Heap used to allocate HashEntries, strings and UserData in */
	class Heap *m_heap;
};

#endif
