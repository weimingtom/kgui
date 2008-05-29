#ifndef __HASH__
#define __HASH__

/* a small hashcode class */

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

class Hash
{
public:
	Hash();
	~Hash();
	void Init(void);
	void Reset(void);
	void Init(int tablebits,int dl);
	void Purge(void);
	void SetCaseSensitive(bool cs) {m_casesensitive=cs;}
	void Add(const char *string,const void *data);
	void *Find(const char *string);
	const char *FindName(const char *string);
	inline int GetNum(void) {return m_num;}
	HashEntry *GetFirst(void) {return m_head.GetNext();}
	void SetFixedLen(int fixed) {m_fixedlen=fixed;}
	void SetDataLen(int dl) {m_datalen=dl;}
	int TableIndex(const char *string);
private:
	int m_num;
	int m_tablebits;
	int m_tablemask;
	int m_fixedlen;
	int m_datalen;
	bool m_casesensitive;
	static int m_ctoi[256];
	HashEntry m_head;	/* linked list of items used for purging upon closing */
	HashEntry m_tail;	/* linked list of items used for purging upon closing */
	HashEntry **m_table;
	class Heap *m_heap;
};

#endif
