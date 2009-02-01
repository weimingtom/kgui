#ifndef __DATAHANDLE__
#define __DATAHANDLE__

/**********************************************************************************/
/* kGUI - datahandle.h                                                            */
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

/*! @file datahandle.h 
    @brief DataHandle class definitions. A DataHandle is the class that is used for
	loading and or saving data. A datahandle can directly reference a file, or a chunk
	of memory, or a file within a bigfile. A DataHandle can also reference encrypted
	data using the attached kGUIProt class. All of the internal functions in kGUI that
	reference files use the DataHandle class for input so for example the movie player
	can play a movie that is encrypted and inside of a bigfile. */

enum
{
	DATATYPE_FILE,
	DATATYPE_MEMORY,
	DATATYPE_UNDEFINED
};

/* these are interal structures used for the read-caching system */
typedef struct
{
	unsigned long long m_startindex;
	unsigned long long m_endindex;
	char *m_data;
	struct s_DataSection *m_datasection;
	unsigned int m_priority;
}StreamBlock;

typedef struct s_DataSection
{
	unsigned long long m_startindex;
	unsigned long long m_endindex;
	StreamBlock *m_streamblock;
}DataSection;

/*! @class DataHandle
	@brief This is the DataHandle class. It is used as a container class for all objects
	that read / write files. It can reference a disk file, memory, or a file inside of a 
	bigfile. Also DataHandles can be encrypted by simply attaching a kGUIProt class */
class DataHandle
{
public:
	DataHandle();
	virtual ~DataHandle();
	
	void SetFilename(const char *fn);
	void SetFilename(kGUIString *fn) {SetFilename(fn->GetString());}
	void SetMemory(void);	/* write first, then can read from */
	void SetMemory(const void *memory,unsigned long filesize);

	//called whenever the SetFilename or SetMemory is called 
	virtual void HandleChanged(void) {}

	void SetOpenLock(bool lock) {m_lock=lock;}	/* if set then don't allow open */

	kGUIString *GetFilename(void) {return &m_ufn;}
	void CopyHandle(DataHandle *from);

	int GetDataType(void) {return m_type;}

	/* initialize a new datahandle for the sub-area of this one at the dest class */	
	void CopyArea(DataHandle *dest,unsigned long offset, unsigned long size,long time);

	/* copy from a different datahandle to memory */
	void Copy(DataHandle *from);

	unsigned long long GetSize(void) {return m_filesize;}
	/* if larger than 'loadable' then assert */
	unsigned long GetLoadableSize(void);
	long GetTime(void) {return m_filetime;}

	bool Open(void);
	bool GetOpen(void) {return (m_open || m_wopen);}
	unsigned long Read(void *dest,unsigned long numbytes);			/* returns number of bytes read */
	unsigned long Read(kGUIString *s,unsigned long numbytes);		/* returns number of bytes read */

	/* these are there to stop the warnings */
	unsigned long Read(void *dest,unsigned long long numbytes) {return(Read(dest,(unsigned long)numbytes));}
	unsigned long Read(kGUIString *s,unsigned long long numbytes) {return(Read(s,(unsigned long)numbytes));}

	void ReadLine(kGUIString *line);
	void ReadHtmlString(kGUIString *s);
	inline unsigned char PeekChar(void) {char c;Read(&c,(unsigned long)1);Seek(m_offset-1);return c;}
	inline unsigned char ReadChar(void) {char c;Read(&c,(unsigned long)1);return c;}
	inline void Skip(long numbytes) {Seek(m_offset+numbytes);}		/* pos or neg */

	bool Seek(const char *string);
	void Seek(unsigned long long index);
	unsigned long long GetOffset(void) {return m_offset;}
	bool Eof(void) {return (m_offset==m_filesize);}
	FILE *GetHandle(void);	/* only valid for filesystem based data */

	void CheckWrite(void);
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

	/* calc crc for file */
	long CRC(void);

	/* new caching system init */
	void InitReadStream(unsigned int blocksizebits,unsigned int numblocks);
	char StreamCmp(const void *cmpstring,unsigned long numbytes,long offset=0,bool cs=true);
	inline unsigned char StreamPeekChar(void) {unsigned long blockoff;StreamBlock *cb;cb=LoadStreamBlock(m_offset,&blockoff,true);return cb->m_data[blockoff];}
	inline unsigned char StreamReadChar(void) {unsigned long blockoff;StreamBlock *cb;cb=LoadStreamBlock(m_offset,&blockoff,true);++m_offset;return cb->m_data[blockoff];}
	inline void StreamSkip(long numbytes) {m_offset+=numbytes;}		/* pos or neg */

private:
	StreamBlock *LoadStreamBlock(unsigned long long index,unsigned long *poffset,bool setcurrent);
	void ReadStreamBlock(StreamBlock *cb);
	void StreamRead(void *dest,unsigned long numbytes);
	void StreamRead(kGUIString *s,unsigned long numbytes);
	void PurgeReadStream(void);

	kGUIMutex m_openmutex;	/* open mutex */
	int m_type;				/* handle or memory based */
	long m_filetime;		/* filetime */
	bool m_lock;			/* open enable/disable */
	bool m_open;			/* read open */
	bool m_wopen;			/* write open? */
	FILE *m_handle;			/* handle of file, only valid for a file with a associated handle */
	kGUIString m_fn;		/* filename (or bigfilename if in bigfile) */
	kGUIString m_ufn;		/* unique filename */
	const unsigned char *m_memory;	/* address of file, only valid for a memory based file */
	unsigned long long m_filesize;		/* size of file */
	unsigned long long m_startoffset;		/* start offset into file, used for bigfiles */
	unsigned long long m_offset;			/* current offset into file */
	class kGUIProt *m_prot;		/* encryption */
	unsigned long long m_writebufferlen;
	Array<unsigned char>m_writebuffer;

	static int m_numbigfiles;					/* number of registered bigfiles */
	static Array<class BigFile *>m_bigfiles;	/* array of pointers to bigfiles */

	/* new caching system */
	bool m_readstream;					/* true=read Stream enabled */
	unsigned int m_nextstreampriority;

	/* section data */
	unsigned int m_numdatasections; 
	Array<DataSection>m_datasections;

	/* Streamblock data */
	StreamBlock *m_currentstreamblock;
	unsigned long long m_currentstreamstartindex;
	unsigned long long m_currentstreamendindex;
	unsigned long long m_currentseekindex;

	Array<StreamBlock>m_streamblocks;
	unsigned long m_streamblockshift;	/* shift index by this to convert to a block# */
	unsigned long m_streamblockmask;		/* mask index by this to convert to an offset into the block */
	unsigned long m_streamblocksize;
	unsigned long m_numstreamblocks;
};

#endif
