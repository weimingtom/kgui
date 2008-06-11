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
	void SetMemory(const unsigned char *memory,unsigned long filesize);

	//called whenever the SetFilename or SetMemory is called 
	virtual void HandleChanged(void) {}

	void SetOpenLock(bool lock) {m_lock=lock;}	/* if set then don't allow open */

	kGUIString *GetFilename(void) {return &m_ufn;}
	void CopyHandle(DataHandle *from);

	int GetDataType(void) {return m_type;}

	/* initialize a new datahandle for the sub-area of this one at the dest class */	
	void CopyArea(DataHandle *dest,unsigned long offset, unsigned long size,long time);

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
	unsigned char PeekChar(void);
	unsigned char ReadChar(void);

	bool Seek(const char *string);
	void Seek(unsigned long long index);
	unsigned long long GetOffset(void) {return m_offset;}
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
	unsigned long long m_filesize;		/* size of file */
	unsigned long long m_startoffset;		/* start offset into file, used for bigfiles */
	unsigned long long m_offset;			/* current offset into file */
	class kGUIProt *m_prot;		/* encryption */
	unsigned long long m_writebufferlen;
	Array<unsigned char>m_writebuffer;

	static int m_numbigfiles;					/* number of registered bigfiles */
	static Array<class BigFile *>m_bigfiles;	/* array of pointers to bigfiles */
	
};

#endif
