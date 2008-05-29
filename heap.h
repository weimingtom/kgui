#ifndef __HEAP__
#define __HEAP__

/* a small heap class */

typedef struct
{
	int m_size;
	int m_used;
	int m_left;
}HEAP_RECORD;

class Heap
{
public:
	Heap();
	~Heap() {Purge();}
	void SetBlockSize(int b) {m_blocksize=b;}
	void Reset(void);
	void Purge(void);
	void *Alloc(int size);
private:
	int m_blocksize;		/* size of each allocated block */
	int m_numblocks;
	Array<HEAP_RECORD *>m_blocks;
};

#endif
