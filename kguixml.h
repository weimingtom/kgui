#ifndef __KGUIXML__
#define __KGUIXML__

/* I have not read the XML documentation at all, but it looks */
/* like a simple heiarchical name/value based text file */

class kGUIXMLItem
{
	friend class kGUIXML;
public:
	kGUIXMLItem() {m_parm=false;m_name=0;m_numchildren=0;m_children.SetGrow(true);m_children.SetGrowSize(-1);}
	~kGUIXMLItem();
	void SetEncoding(unsigned int e) {m_value.SetEncoding(e);};
	char *Load(class kGUIXML *root,kGUIString *ts,char *fp,kGUIXMLItem *parent);
	void Copy(kGUIXMLItem *copy);
	void Save(class kGUIXML *top,kGUIString *ts,FILE *fp,unsigned int level);
	void AddChild(kGUIXMLItem *child) {m_children.SetEntry(m_numchildren,child);++m_numchildren;}
	void CopyChild(kGUIXMLItem *copy) {kGUIXMLItem *child=new kGUIXMLItem();child->Copy(copy);m_children.SetEntry(m_numchildren,child);++m_numchildren;}
	void DelChild(kGUIXMLItem *child,bool purge=true);
	kGUIXMLItem *Locate(const char *name,bool add=false);

	void SetName(const char *n) {m_name=n;}
	const char *GetName(void) {return m_name;}

	void SetValue(const char *v) {m_value.SetString(v);}
	kGUIString *GetValue(void) {return &m_value;}
	const char *GetValueString(void) {return m_value.GetString();}
	int GetValueInt(void) {return m_value.GetInt();}
	double GetValueDouble(void) {return m_value.GetDouble();}

	unsigned int GetNumChildren(void) {return m_numchildren;}
	kGUIXMLItem *GetChild(int i) {return m_children.GetEntry(i);}

	void AddParm(const char *name,const char *value=0);
	void AddParm(const char *name,kGUIString *value);
	void AddParm(const char *name,double value) {kGUIString s;s.Sprintf("%f",value);AddParm(name,&s);}
	void AddParm(const char *name,int value) {kGUIString s;s.Sprintf("%d",value);AddParm(name,&s);}
	kGUIXMLItem *AddChild(const char *name,const char *value=0);
	kGUIXMLItem *AddChild(const char *name,kGUIString *value);
	kGUIXMLItem *AddChild(const char *name,int value);
	kGUIXMLItem *AddChild(const char *name,double value);
	kGUIXMLItem *AddChild(const char *name,unsigned int value);
private:
	const char *m_name;
	kGUIString m_value;
	unsigned int m_numchildren:31;
	bool m_parm:1;
	Array<kGUIXMLItem *>m_children;
};

/* backwards table for converting char to &xxx; */
typedef struct
{
	const char *longcode;
	int longlen;
}XCODE_DEF;


class kGUIXMLCODES
{
public:
	static void Init(void);
	static void Purge(void);

	static void Shrink(kGUIString *from,kGUIString *to);
	static void Expand(kGUIString *from,kGUIString *to);
private:
	static unsigned int m_longest;
	static unsigned int m_maxcode;
	static class Hash *m_longhash;
	static XCODE_DEF *m_xcodes;
	static kGUIString *m_utflist;
};

class kGUIXML
{
public:
	kGUIXML();
	virtual ~kGUIXML();
	bool Load(const char *filename);
	bool StreamLoad(const char *filename);
	bool Save(const char *filename);
	/* this is called after loading each object */
	virtual void ChildLoaded(kGUIXMLItem *child,kGUIXMLItem *parent) {}

	kGUIXMLItem *GetRootItem(void) {return m_root;}
	void SetLoadingCallback(void *codeobj,void (*code)(void *,int,int)) {m_loadcallback.Set(codeobj,code);}
	void Update(char *cur);
	unsigned int GetEncoding(void) {return m_encoding;}
	void SetEncoding(unsigned int e) {m_encoding=e;}
	
	void SetNameCache(Hash *namecache) {m_namecache=namecache;}
	const char *CacheName(const char *name);

	/* item pool */
#if 0
	inline kGUIXMLItem *PoolGet(void) {return m_pool.Get();}
	void PoolAdd(kGUIXMLItem *item) {m_pool.Release(item);for(unsigned int j=0;j<item->m_numchildren;++j){PoolAdd(item->m_children.GetEntry(j));}item->m_numchildren=0;item->m_parm=false;item->m_value.Clear();}
#else
	inline kGUIXMLItem *PoolGet(void) {if(!m_numinpool)return new kGUIXMLItem();return m_itempool.GetEntry(--m_numinpool);}
	void PoolAdd(kGUIXMLItem *item) {m_itempool.SetEntry(m_numinpool++,item);for(unsigned int j=0;j<item->m_numchildren;++j){PoolAdd(item->m_children.GetEntry(j));}item->m_numchildren=0;item->m_parm=false;item->m_value.Clear();}
#endif
private:
	kGUIXMLItem *StreamLoad(kGUIXMLItem *parent);	/* recursive call */
	char *m_fd;
	unsigned long m_filesize;
	unsigned int m_encoding;
	kGUIXMLItem *m_root;
	kGUICallBackIntInt m_loadcallback;
	bool m_namecachelocal;
	Hash *m_namecache;

	DataHandle m_dh;
	kGUIString m_tempstring;
#if 0
	ClassPool<kGUIXMLItem>m_pool;
#else
	unsigned int m_numinpool;
	Array<kGUIXMLItem *>m_itempool;
#endif
};

#endif
