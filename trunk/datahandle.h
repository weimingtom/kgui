#ifndef __DATAHANDLE__
#define __DATAHANDLE__

/****************************************************************************/
/* datahandle																*/
/*																			*/
/* This is a class used for handling loading of data from various devices   */
/* The default is from a filesystem but it can also be from memory or any   */
/* other futurer device type.												*/
/****************************************************************************/

enum
{
	DATATYPE_FILE,
	DATATYPE_MEMORY,
	DATATYPE_UNDEFINED
};

class DataHandle
{
public:
	DataHandle();
	virtual ~DataHandle();
	
	void SetFilename(const char *fn);
	void SetFilename(kGUIString *fn) {SetFilename(fn->GetString());}
	void SetMemory(void);	/* write first, then can read from */
	void SetMemory(const unsigned char *memory,unsigned long filesize);

	//called whenever the SetFilename or SetMemory is called 
	virtual void HandleChanged(void) {}

	void SetOpenLock(bool lock) {m_lock=lock;}	/* if set then don't allow open */

	kGUIString *GetFilename(void) {return &m_ufn;}
	void CopyHandle(DataHandle *from);

	int GetDataType(void) {return m_type;}

	/* initialize a new datahandle for the sub-area of this one at the dest class */	
	void CopyArea(DataHandle *dest,unsigned long offset, unsigned long size,long time);

	long long GetSize(void) {return m_filesize;}
	long GetTime(void) {return m_filetime;}

	bool Open(void);
	bool GetOpen(void) {return (m_open || m_wopen);}
	unsigned long Read(void *dest,unsigned long numbytes);			/* returns number of bytes read */
	unsigned long Read(kGUIString *s,unsigned long numbytes);		/* returns number of bytes read */

	/* these are there to stop the warnings */
	unsigned long Read(void *dest,long long numbytes) {return(Read(dest,(unsigned long)numbytes));}
	unsigned long Read(kGUIString *s,long long numbytes) {return(Read(s,(unsigned long)numbytes));}

	void ReadLine(kGUIString *line);
	void ReadHtmlString(kGUIString *s);
	unsigned char PeekChar(void);
	unsigned char ReadChar(void);

	bool Seek(const char *string);
	void Seek(long long index);
	long long GetOffset(void) {return m_offset;}
	bool Eof(void) {return (m_offset==m_filesize);}
	FILE *GetHandle(void);	/* only valid for filesystem based data */

	bool OpenWrite(const char *wtype,unsigned long defbufsize=65536);		/* write mode string "wb" or "rb+" etc */
	void Sprintf(const char *fmt,...);
	void Write(const void *buf,long numbytes);
	bool Close(bool error=false);

	void *GetBufferPtr(void) {return m_writebuffer.GetArrayPtr();}

	void SetEncryption(class kGUIProt *p);

	/* static functions */
	static void InitStatic(void);
	static void PurgeStatic(void);
	/* add bigfile to global scan list */
	static void AddBig(class BigFile *bf);
	/* remove bigfile to global scan list */
	static void RemoveBig(class BigFile *bf);
private:
	int m_type;				/* handle or memory based */
	long m_filetime;		/* filetime */
	bool m_lock;			/* open enable/disable */
	bool m_open;			/* read open */
	bool m_wopen;			/* write open? */
	FILE *m_handle;			/* handle of file, only valid for a file with a associated handle */
	kGUIString m_fn;		/* filename (or bigfilename if in bigfile) */
	kGUIString m_ufn;		/* unique filename */
	const unsigned char *m_memory;	/* address of file, only valid for a memory based file */
	long long m_filesize;		/* size of file */
	long long m_startoffset;		/* start offset into file, used for bigfiles */
	long long m_offset;			/* current offset into file */
	class kGUIProt *m_prot;		/* encryption */
	long long m_writebufferlen;
	Array<unsigned char>m_writebuffer;

	static int m_numbigfiles;					/* number of registered bigfiles */
	static Array<class BigFile *>m_bigfiles;	/* array of pointers to bigfiles */
	
};

#endif
