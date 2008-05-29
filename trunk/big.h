#ifndef __BIG__
#define __BIG__

/*! The BigFile is a class used to manipulate .big files. Bigfiles are files that contain
    other files within them. */

/* size of first directory block */
#define FIRSTDBLOCKSIZE 4096
/* size of subsequent directory blocks */
#define DBLOCKSIZE 65536

/****************************************************/

typedef struct
{
	unsigned int offset;	/* offset to me */
	unsigned int blocksize;	/* size of directory block */
	unsigned int nextblock;			/* offset to next block, 0=last block */
	unsigned int numfiles;			/* number of files in this block */
	unsigned int fntail;				/* offset to lowest filename in this block */
}BIGHEADER_DEF;

typedef struct
{
	unsigned int nameoffset;	/* offset within this block */
	unsigned int dataoffset;	/* offset within the file */
	unsigned int size;			/* size of data */
	int time;					/* time */
	unsigned int crc;					/* to make sure data is ok? */
}BIGENTRY_DEF;

/* this is the data that is added to the hash table */

/*! This is the individual file directory block within the bigfile */
class BigFileEntry
{
public:
	/* name is stored by hash code as just after this structure */
	inline const char *GetName(void) {return ((const char *)(this+1));}
	unsigned int m_dirblocknum;		/* directory block number where entry is stored, only valid in edit mode */
	unsigned int m_diroffset;		/* offset into file where directory entry is */
	unsigned int m_offset;			/* offset to data from beginning of file */
	unsigned int m_size;			/* size of file */
	unsigned int m_crc;				/* crc of file */
	int m_time;						/* time */
};

/*! This is the header block of each directory inside the bigfile */
typedef struct
{
	bool changed;				/* does this need to be rewritten to the file? */
	unsigned long offset;		/* offset into file */
	char *data;
	unsigned long size;			/* size of this directory block */
}DIRINFO_DEF;


/*! @class BigFile
	@brief This is the class for reading from and writing to big files
	it uses the DataHandle class so the big file can be a disk based file,
	or memory resident or inside another big file */
class BigFile : public DataHandle
{
public:
	BigFile();
	~BigFile();

	/*! Load the directory for the Bigfile into the class.
	    @param edit set to true if files will be added to the bigfile */
	void Load(bool edit=false);	/* load the bigfile */
	/*! Free the directory blocks */
	void FreeDirBlocks(void);

	/*! Returns the number of files inside the BigFile */
	inline int GetNum(void) {return m_hash.GetNum();}
	/*! Returns a HashEntry class that references the first entry in the BigFile.
	    Then follow the HashEntry linked list for each subsequent entry. */
	inline HashEntry *GetFirst(void) {return ((HashEntry *)m_hash.GetFirst());}

	/*! If the attached bigfile is not valid then return true */
	inline bool IsBad(void) {return(m_bad);}
	/*! Locate the named file, return NULL if not found */
	BigFileEntry *Locate(const char *fn);
	/*! Add the named file to the BigFile
	    @param fn filename
		@param af handle to data
        @param updatedir if true then the directory is updated right away, if false then it is deferred until after multiple files have been added */
	bool AddFile(const char *fn,DataHandle *af,bool updatedir=true);
	/*! write all updated directory blocks to the bigfile */
	void UpdateDir(void);	/* write directory if changes are pending */
	/*! check to see if any files in the bigfile have this exact data in them */
	bool CheckDuplicate(DataHandle *af);	
	/*! calculate a crc for a given block of data
	    @param startcrc seed value
		@param buf pointer to data
		@param buflen size of data in bytes */
	unsigned long CrcBuffer(long startcrc,const unsigned char *buf,unsigned long buflen);
	/*! extract a given file from the bigfile and write it to the disk
	    @param name name of file to extract
		@param filename name to save as, note will change to DataHandle in the future */
	bool Extract(const char *name,const char *filename);
	/*! delete the named file from inside the bigfile 
	    @param fn filename of file to remove */
	void Delete(const char *fn);
	/*! check to see if directory has been loaded */
	bool GetDirLoaded(void) {return m_dirloaded;}
private:
	/*! newly created bigfile flag */
	bool m_empty:1;
	/*! not empty, but not a bigfile either */
	bool m_bad:1;
	/*! the directories have changed in memory and not been written yet */
	bool m_dirchanged:1;
	/*! the directories have been loaded into memory */
	bool m_dirloaded:1;
	/*! true = directory has been loaded for edit, false=read only */
	bool m_edit:1;
	/*! current end of file position */
	unsigned long m_curend;
	/*! amount of wasted space in the file (sum of gaps between files) */
	unsigned long m_curwaste;
	/*! last header in bigfile */
	BIGHEADER_DEF m_lastheader;
	/*! hash table that holds filenames and pointers to BigFileEntry class for each file in the bigfile */
	Hash m_hash;
	/*! number of directory blocks in bigfile */
	unsigned int m_numdirblocks;
	/*! array of directory block headers */
	Array<DIRINFO_DEF> m_dirblocks;
};

#endif
