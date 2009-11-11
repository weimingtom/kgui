/**********************************************************************************/
/* kGUI - datahandle.cpp                                                          */
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

/*! @file datahandle.cpp 
    @brief A DataHandle is a container class for holding data. It can be attached to
	a disk file, or a section of memory, locally allocated memory or an area within
	a bigfile. A DataHandle can also contain encrypted data when attached to a
	kGUIProt class than handles the encryption and decryption automatically. 
	All objects in kGUI that require data use this class for loading and saving data.
	That way, for example: images ( or movies ) can be rendered that reside inside
	encryped bigfiles. */

#include "kgui.h"
#include "kguiprot.h"

#if defined(WIN32) || defined(MINGW)
#define _INTEGRAL_MAX_BITS 64
#include <sys\stat.h> 
#endif

int DataHandle::m_numbigfiles;
Array<BigFile *>DataHandle::m_bigfiles;

void DataHandle::InitStatic(void)
{
	m_numbigfiles=0;
	m_bigfiles.Alloc(16);
	m_bigfiles.SetGrow(true);
}

void DataHandle::PurgeStatic(void)
{
	int i;
	BigFile *bf;

	for(i=0;i<m_numbigfiles;++i)
	{
		bf=m_bigfiles.GetEntry(i);
		delete bf;
	}
	m_numbigfiles=0;
}


void DataHandle::AddBig(BigFile *bf)
{
	/* if the directory hasn't been loaded already then load it now */
	if(bf->GetDirLoaded()==false)
		bf->Load();
	m_bigfiles.SetEntry(m_numbigfiles,bf);
	++m_numbigfiles;
}

void DataHandle::RemoveBig(BigFile *bf)
{
	m_bigfiles.Delete(bf);
	--m_numbigfiles;
}

DataHandle::DataHandle()
{
	m_type=DATATYPE_UNDEFINED;
	m_startoffset=0;	/* offset to start of file, used for files inside of bigfiles */
	m_offset=0;			/* goes from 0 to filesize */
	m_filesize=0;
	m_handle=0;
	m_memory=0;
	m_open=false;		/* open for read */
	m_wopen=false;		/* open for write */
	m_prot=0;			/* default to no encryption */
	m_lock=false;		/* lock flag for diable/enable open */
	m_readstream=false;	/* default read-stream to off */
}

DataHandle::~DataHandle()
{
	if(m_handle)
		fclose(m_handle);
	assert(m_handle==0 && m_open==false && m_wopen==false,"File was left open, aarrgghh!");

	if(m_readstream==true)
		PurgeReadStream();
}

void DataHandle::SetEncryption(class kGUIProt *p)
{
	assert(m_open==false && m_wopen==false,"Encryption needs to be set before opening file!");
	m_prot=p;
}

void DataHandle::SetFilename(const char *fn)
{
	int i;
	BigFile *bf;
	BigFileEntry *bfe;
	FILE *fp;

	/* make sure file is closed */
	assert(m_open==false && m_wopen==false,"Last file needs to be closed before starting a new one!");

	m_ufn.SetString(fn);

	/* does this file exist inside a bigfile already? */
	for(i=0;i<m_numbigfiles;++i)
	{
		bf=m_bigfiles.GetEntry(i);
		bfe=bf->Locate(fn);
		if(bfe)
		{
			bf->CopyArea(this,bfe->m_offset,bfe->m_size,bfe->m_time);
			HandleChanged();
			return;
		}
	}

	m_type=DATATYPE_FILE;
	m_fn.SetString(fn);
	m_startoffset=0;
	m_filetime=kGUI::SysFileTime(fn);

	/* calc filesize */
	if(!fn[0])
		m_filesize=0;
	else
	{
		fp=fopen(fn,"rb");
		if(fp)
		{
#if defined(WIN32) || defined(MINGW)
			struct __stat64 fileStat; 
			int rc;

			rc = _stat64( fn, &fileStat ); 
			assert(rc==0,"Seek error!");
			m_filesize=fileStat.st_size; 
#else
			int sok;

			sok=fseek(fp,0,SEEK_END);
			assert(sok==0,"Seek error!");
			m_filesize=ftell(fp);
#endif
			fclose(fp);
		}
		else
			m_filesize=0;
	}
	HandleChanged();
}

unsigned long DataHandle::GetLoadableSize(void)
{
	if(m_filesize>((unsigned long long)1<<32))
		assert(false,"File is too long to be loaded into memory!");

	return((unsigned long)m_filesize);
}


void DataHandle::SetMemory(void)
{
	/* make sure file is closed */
	assert(m_open==false && m_wopen==false,"Last file needs to be closed before starting a new one!");

	m_type=DATATYPE_MEMORY;
	m_memory=0;
	m_offset=0;
	m_filesize=0;
	m_open=false;
	HandleChanged();
}

void DataHandle::SetMemory(const void *memory,unsigned long filesize)
{
	/* make sure file is closed */
	assert(m_open==false && m_wopen==false,"Last file needs to be closed before starting a new one!");

	m_type=DATATYPE_MEMORY;
	m_memory=(const unsigned char *)memory;
	m_offset=0;
	m_filesize=filesize;
	m_open=false;
	HandleChanged();
}

void DataHandle::CopyArea(DataHandle *dest,unsigned long offset, unsigned long size,long time)
{
	/* make sure file is closed */
	assert(dest->m_open==false && dest->m_wopen==false,"Last file needs to be closed before starting a new one!");

	/* note, lock is not copied from parent as child sub areas default to unlocked */
	/* unless specifically locked! */

	dest->m_fn.SetString(m_fn.GetString());
	dest->m_type=m_type;
	dest->m_filetime=time;
	dest->m_memory=m_memory;
	dest->m_startoffset=m_startoffset+offset;
	dest->m_filesize=size;
	dest->m_open=false;
	dest->m_handle=0;
	dest->m_prot=m_prot;	/* copy encrtyption model to child */
}

void DataHandle::Copy(DataHandle *from)
{
	unsigned long long i;
	unsigned long long fs;
	unsigned char c;

	m_offset=0;
	SetMemory();
	fs=from->GetSize();
	if(fs)
	{
		from->Open();
		OpenWrite("wb",(unsigned long)fs);
		for(i=0;i<fs;++i)
		{
			from->Read(&c,(unsigned long long)1L);
			Write(&c,1);
		}
		from->Close();
		m_filesize=fs;
		Close();
	}
}



bool DataHandle::Open(void)
{
	/* if this datafile encrypted, then completely */
	/* load the file into memory, unencrypt it and feed it */
	/* from memory, purge when code calls Close() */

	if(m_openmutex.TryLock()==false)
		return(false);

	assert(m_lock==false,"Error: file is locked!");
	switch(m_type)
	{
	case DATATYPE_FILE:
		m_offset=0;
		assert(m_open==false,"Already Open!");
		assert(m_fn.GetLen()!=0,"No Filename defined!");

		m_handle=fopen(m_fn.GetString(),"rb");
		if(m_handle)
		{
			if(m_startoffset)
			{
#if defined(WIN32) || defined(MINGW)
				fpos_t pos=m_startoffset;
				fsetpos(m_handle,&pos);
#else
				int sok;

				sok=fseek(m_handle,m_startoffset,SEEK_SET);
				assert(sok==0,"Seek error!");
#endif
			}
			m_open=true;

			/* if file is encrypted, then load the whole thing in */
			/* then decrypt it and feed it from the buffer */

			if(m_prot)	/* is this file encrypted? */
			{
				unsigned long loadablefilesize=GetLoadableSize();
				unsigned char *encdata;
				unsigned long numread;

				encdata=new unsigned char[loadablefilesize];
				numread=(unsigned long)fread(encdata,1,loadablefilesize,m_handle);
				assert(numread==loadablefilesize,"Error reading data!");
				fclose(m_handle);
				m_handle=0;
				
				/* only handle files less than 2gb size */
				m_memory=(const unsigned char *)m_prot->Decrypt(encdata,(unsigned long)loadablefilesize);
				delete []encdata;
				m_offset=0;
			}
		}
		else
		{
			// kGUI::Trace("Open couldn't open file %s %s\n",m_fn.GetString(),m_ufn.GetString());
		}
	break;
	case DATATYPE_MEMORY:
		m_open=true;
		m_offset=0;
	break;
	}
	if(m_open==false)
		m_openmutex.UnLock();

	return(m_open);
}

void DataHandle::CheckWrite(void)
{
	assert(m_lock==false,"Error: file is locked!");
	assert(m_open==false,"File is already open for read!");
	assert(m_wopen==false,"File is already open for write!");
	switch(m_type)
	{
	case DATATYPE_MEMORY:
		assert(m_memory==0,"Error, only allocated memory type valid for writing!");
	break;
	case DATATYPE_FILE:
		assert(m_fn.GetLen()!=0,"Error, filename not set!");
	break;
	}
}

bool DataHandle::OpenWrite(const char *wtype,unsigned long defbufsize)		/* write mode string "wb" or "rb+" etc */
{
	if(m_openmutex.TryLock()==false)
		return(false);

	assert(m_lock==false,"Error: file is locked!");
	assert(m_open==false,"File is already open for read!");
	assert(m_wopen==false,"File is already open for write!");

	switch(m_type)
	{
	case DATATYPE_MEMORY:
		assert(m_memory==0,"Error, only allocated memory type valid for writing!");
		m_wopen=true;
	break;
	case DATATYPE_FILE:
		m_handle=fopen(m_fn.GetString(),wtype);
		if(m_handle)
		{
			if(m_startoffset)
			{
#if defined(WIN32) || defined(MINGW)
				fpos_t pos=m_startoffset;
				fsetpos(m_handle,&pos);
#else
				int sok;
	
				sok=fseek(m_handle,m_startoffset,SEEK_SET);
				assert(sok==0,"Seek error!");
#endif
			}
			m_wopen=true;
		}
	break;
	}
	if(m_wopen==true)
	{
		m_writebufferlen=0;
		m_writebuffer.Init(defbufsize,defbufsize>>1);
	}
	if(m_wopen==false)
		m_openmutex.UnLock();
	return(m_wopen);
}

void DataHandle::Sprintf(const char *fmt,...)
{
	kGUIString fstring;
	va_list args;

    va_start(args, fmt);
	fstring.AVSprintf(fmt,args);
    va_end(args);
	Write(fstring.GetString(),fstring.GetLen());
}

void DataHandle::Write(const void *buf,long numbytes)
{
	const unsigned char *cbuf=(const unsigned char *)buf;
	unsigned char *dest;
	assert(m_wopen==true,"File not open error!");

	/* make sure buffer is big enough */
	if((m_writebufferlen+numbytes)>m_writebuffer.GetNumEntries())
		m_writebuffer.Alloc(MAX(m_writebufferlen+numbytes,m_writebuffer.GetNumEntries()+65536));	

	/* use memcpy to fill in buffer */

	dest=m_writebuffer.GetArrayPtr()+m_writebufferlen;
	memcpy(dest,cbuf,numbytes);
	m_writebufferlen+=numbytes;
}

FILE *DataHandle::GetHandle(void)
{
	assert(m_open==true,"File is not open!");
	assert(m_type==DATATYPE_FILE,"Not a file!");
	assert(m_prot==0,"Not valid for encrypted file!");
	return m_handle;
}

bool DataHandle::Close(bool error)
{
	bool ok=true;
	bool changed=false;

//	kGUI::Trace("Close %x\n",this);
	if(m_open==true)
	{
		switch(m_type)
		{
		case DATATYPE_FILE:
			if(m_prot)
			{
				delete []m_memory;
				m_memory=0;
			}
			else
			{
				fclose(m_handle);
				m_handle=0;
			}
		break;
		case DATATYPE_MEMORY:
		break;
		}
		m_open=false;
	}
	else if(m_wopen==true)
	{
		changed=true;
		switch(m_type)
		{
		case DATATYPE_MEMORY:
			if(error==false)
				m_filesize=m_writebufferlen;	/* update since used in read and seek code */
			else
				m_filesize=0;
		break;
		case DATATYPE_FILE:
			if(error)
			{
				fclose(m_handle);
				/* should we delete it? */
				m_filesize=0;
			}
			else
			{
				if(m_prot)	/* encrypt data before writing */
				{
					const unsigned char *ebuf;
					
					/* only handle files <2gb */
					ebuf=m_prot->Encrypt((const unsigned char *)m_writebuffer.GetArrayPtr(),(long)m_writebufferlen);
					if(fwrite(ebuf,1,m_writebufferlen,m_handle)!=m_writebufferlen)
						ok=false;	/* write error! */
					delete []ebuf;
				}
				else
				{
					if(fwrite(m_writebuffer.GetArrayPtr(),1,m_writebufferlen,m_handle)!=m_writebufferlen)
						ok=false;	/* write error! */
				}
				fclose(m_handle);
			}
			m_handle=0;
		break;
		}
		m_wopen=false;
	}
	else
	{
		assert(false,"File is not open for either read or write!");
	}
	m_openmutex.UnLock();
	if(changed)
		HandleChanged();
	return(ok);
}

void DataHandle::Seek(unsigned long long index)
{
	assert(m_open==true,"File not open error!");

	m_offset=index;
	switch(m_type)
	{
	case DATATYPE_FILE:
		if(!m_prot)
		{
			if(!m_readstream)
			{
#if defined(WIN32) || defined(MINGW)
				fpos_t pos=m_startoffset+m_offset;
				fsetpos(m_handle,&pos);
#else
				int sok;

				sok=fseek(m_handle,m_startoffset+m_offset,SEEK_SET);
				assert(sok==0,"Seek error!");
#endif
			}
		}
	break;
	case DATATYPE_MEMORY:
	break;
	}
}

/* seek until a particular string if found, then current position is AFTER the string */
bool DataHandle::Seek(const char *string)
{
	long long curpos=m_offset;
	unsigned long n=(int)strlen(string);
	char *cmpstring;

	cmpstring=new char[n+1];
	cmpstring[n]=0;

	do
	{
		Seek(curpos);
		if(Read(cmpstring,n)!=n)
		{
			delete cmpstring;
			return false;	/* must have it end of file, no match found */
		}

		if(!strnicmp(cmpstring,string,n))
		{
			delete cmpstring;
			return(true);
		}
		++curpos;
	}while(1);
}

/* return plain text or html command */
void DataHandle::ReadHtmlString(kGUIString *s)
{
	char c;

	s->Clear();
	if(Eof()==true)
		return;	/* at end of file! */

	c=ReadChar();
	s->Append(c);

	if(c!='<')
	{
		/* not html command */
		while(Eof()==false)
		{
			c=PeekChar();
			if(c=='<')
				break;
			s->Append(c);
			ReadChar();	/* eat char */
		}
	}
	else
	{
		/* html command */
		while(Eof()==false)
		{
			c=ReadChar();
			s->Append(c);
			if(c=='>')
				break;
		}
	}
}

/* reads null terminated string */
void DataHandle::Read(kGUIString *s)
{
	unsigned char c;
	unsigned long nb;

	s->Clear();

	do{
		nb=Read(&c,(unsigned long)1);
		if(!nb)
			return;
		if(!c)
			return;
		s->Append(c);
	}while(1);
}

unsigned long DataHandle::Read(kGUIString *s,unsigned long numbytes)
{
	unsigned long numread;
	char *buffer;

	if(m_readstream)
	{
		/* this one doesn't need a temporary buffer */
		numread=numbytes;
		StreamRead(s,numbytes);
		m_offset+=numread;
	}
	else
	{
		/* change to malloc/free since they return null if out of memory */
		buffer=(char *)malloc(numbytes);
		if(!buffer)
			return(0);
		//buffer=new char [numbytes];
		numread=Read(buffer,numbytes);
		s->SetString(buffer,numread);
		//delete []buffer;
		free(buffer);
	}
	return(numread);
}

unsigned long DataHandle::Read(void *dest,unsigned long numbytes)
{
	assert(m_open==true,"File not open error!");
	//todo: assert eof check

	/* trying to read too many? */
	if((numbytes+m_offset)>m_filesize)
		numbytes=(unsigned long)(m_filesize-m_offset);

	switch(m_type)
	{
	case DATATYPE_FILE:
		if(!m_prot)
		{
			if(m_readstream)
				StreamRead(dest,numbytes);
			else
				fread(dest,1,numbytes,m_handle);
		}
		else
			memcpy(dest,m_memory+m_offset,numbytes);
	break;
	case DATATYPE_MEMORY:
		if(m_memory)
			memcpy(dest,m_memory+m_startoffset+m_offset,numbytes);
		else
			memcpy(dest,m_writebuffer.GetArrayPtr()+m_offset,numbytes);
	break;
	}
	m_offset+=numbytes;

	return(numbytes);
}

void DataHandle::ReadLine(kGUIString *line)
{
	char c;
	char c2;

	line->Clear();
	while(Eof()==false)
	{
		Read(&c,(unsigned long)1);
		if((c==0x0d) || (c==0x0a) ||(!c))
			break;
		line->Append(c);
	}

	if((Eof()==true) || (!c))
		return;
	Read(&c2,(unsigned long)1);
	if((!( (c==0x0d) && (c2=0x0a)) || ((c==0x0a) && (c2=0x0d))))
		Seek(GetOffset()-1);		/* go back 1 */
}

void DataHandle::CopyHandle(DataHandle *from)
{
	m_open=false;
	
	if(m_open==true || m_wopen==true)
		Close();

	m_type=from->m_type;
	m_startoffset=from->m_startoffset;
	m_filetime=from->m_filetime;
	m_filesize=from->m_filesize;
	m_ufn.SetString(from->m_ufn.GetString());
	m_fn.SetString(from->m_fn.GetString());
	m_memory=from->m_memory;		/* assumed that other memory area will stick around ?? */
	m_handle=0;

	/* if allocated memory then copy it */
	if(m_type==DATATYPE_MEMORY)
	{
		if(!m_memory)
		{
			m_writebuffer.Alloc(m_filesize);
			memcpy(m_writebuffer.GetArrayPtr(),from->m_writebuffer.GetArrayPtr(),m_filesize);
		}
	}
}

long DataHandle::CRC(void)
{
	unsigned long long i;
	unsigned long long fs;
	long hashcode=0;
	unsigned char byte;

	if(Open()==false)
		return(0);
	fs=GetSize();

	for(i=0;i<fs;++i)
	{
		Read(&byte,(unsigned long)1);
		hashcode=(long)byte^(hashcode<<6);
		hashcode^=hashcode>>24;
		hashcode&=0xffffff;
	}
	Close();
	return(hashcode);
}

/********************************************************************************************/
/* new functions to handle read caching for large files that are too big to fit into memory */
/********************************************************************************************/

void DataHandle::InitReadStream(unsigned int blocksizebits,unsigned int numblocks)
{
	unsigned int i;
	StreamBlock *cb;
	DataSection *ds;
	unsigned long long index;

	/* since it loads in the first block automatically the file must be open for read */
	assert(m_open==true,"File not open error!");

	if(m_readstream==true)
		PurgeReadStream();

	m_readstream=true;
	m_nextstreampriority=2;
	m_currentseekindex=0;

	m_streamblocksize=1<<blocksizebits;		/* bytes per block */
	m_streamblockshift=blocksizebits;		/* shift right to convert offset to block# */
	m_streamblockmask=m_streamblocksize-1;	/* mask to get block offset */

	/* now allocate the sections, this is the filesize / the block size */
	m_numdatasections=(unsigned int)((m_filesize+(m_streamblocksize-1))>>blocksizebits);

//	kGUI::Trace("NumDataSections=%d!\n",m_numdatasections);

	m_datasections.Alloc(m_numdatasections);
	index=0;
	for(i=0;i<m_numdatasections;++i)
	{
		ds=m_datasections.GetEntryPtr(i);

		ds->m_streamblock=0;
		ds->m_startindex=index;
		index+=m_streamblocksize;
		if(index>m_filesize)
			index=m_filesize;
		ds->m_endindex=index;
	}

	/* if the whole file can fit then cut back the number of datasections */
	if(numblocks>m_numdatasections)
		numblocks=m_numdatasections;

	m_numstreamblocks=numblocks;
//	kGUI::Trace("NumStreamBlocks=%d!\n",m_numstreamblocks);
	m_streamblocks.Init(m_numstreamblocks,0);
	for(i=0;i<m_numstreamblocks;++i)
	{
		cb=m_streamblocks.GetEntryPtr(i);

		cb->m_data=new char[m_streamblocksize];

		/* load the first block */
		ds=m_datasections.GetEntryPtr(i);
		if(!i)
		{
			cb->m_startindex=ds->m_startindex;
			cb->m_endindex=ds->m_endindex;
			ReadStreamBlock(cb);

			cb->m_priority=1;
			m_currentstreamblock=cb;	/* set as the current stream block */
			m_currentstreamstartindex=cb->m_startindex;
			m_currentstreamendindex=cb->m_endindex;
		}
		else
		{
			/* attach to streamblock to datasection but don't load it */
			cb->m_startindex=0;
			cb->m_endindex=0;
			cb->m_priority=0;
		}
		ds->m_streamblock=cb;
		cb->m_datasection=ds;
	}
}

void DataHandle::ReadStreamBlock(StreamBlock *cb)
{
	unsigned long long saveindex;
	
//	kGUI::Trace("Read %ld\n",cb->m_startindex);

	m_readstream=false;
	saveindex=m_offset;

	if(cb->m_startindex!=m_currentseekindex)
		Seek(cb->m_startindex);

	Read(cb->m_data,cb->m_endindex-cb->m_startindex);
	m_offset=saveindex;
	m_readstream=true;
	m_currentseekindex=cb->m_endindex;
}

/* load block if it is not currently loaded */
StreamBlock *DataHandle::LoadStreamBlock(unsigned long long index,unsigned long *poffset,bool setcurrent)
{
	DataSection *ds;
	StreamBlock *cb;

	/* quick optimization check */
	if(index>=m_currentstreamstartindex && index<m_currentstreamendindex)
	{
		*(poffset)=(unsigned long)(index-m_currentstreamstartindex);
		return(m_currentstreamblock);
	}

	assert(index<m_filesize,"Trying to read past end!");

	ds=m_datasections.GetEntryPtr((unsigned int)(index>>m_streamblockshift));
	if(ds->m_streamblock)
	{
		/* it has a stream block already attached, so let's see if it is valid */
		cb=ds->m_streamblock;
		if(cb->m_startindex!=ds->m_startindex || cb->m_endindex!=ds->m_endindex)
		{
			/* it has an attached streamblock but it has not been loaded yet */
			cb->m_startindex=ds->m_startindex;
			cb->m_endindex=ds->m_endindex;
			ReadStreamBlock(cb);
		}
	}
	else
	{
		/* this datasection doesn't have an attached streamblock, so let's scan */
		/* all streamblocks and use the one with the lowest priority ( last used ) */
		StreamBlock *lcb;
		unsigned int i;

		cb=m_streamblocks.GetEntryPtr(0);
		for(i=1;i<m_numstreamblocks;++i)
		{
			lcb=m_streamblocks.GetEntryPtr(i);
			if(lcb->m_priority<cb->m_priority)
				cb=lcb;
		}

		/* cb is the one with the lowest priority so we are going to use it */
		/* detach it from it's previous datasection */
		cb->m_datasection->m_streamblock=0;

		/* attach it to this datasection */
		cb->m_datasection=ds;
		ds->m_streamblock=cb;

		cb->m_startindex=ds->m_startindex;
		cb->m_endindex=ds->m_endindex;
		ReadStreamBlock(cb);
	}

	cb->m_priority=m_nextstreampriority++;
	if(setcurrent)
	{
		m_currentstreamblock=cb;
		m_currentstreamstartindex=cb->m_startindex;
		m_currentstreamendindex=cb->m_endindex;
	}
	*(poffset)=(unsigned long)(index-cb->m_startindex);
	return(cb);
}

void DataHandle::StreamRead(void *dest,unsigned long numbytes)
{
	unsigned char *dp=(unsigned char *)dest;
	unsigned long numavail;
	unsigned long blockoffset;
	StreamBlock *cb;
	unsigned long long index=m_offset;

	/* traverse stream blocks if necessary */
	while(numbytes>0)
	{
		cb=LoadStreamBlock(index,&blockoffset,true);

		/* num bytes that are available in this block */
		numavail=MIN(m_streamblocksize-blockoffset,numbytes);
		memcpy(dp,cb->m_data+blockoffset,numavail);
		/* subtract number of bytes copied and loop if necessary */
		numbytes-=numavail;
		index+=numavail;
		dp+=numavail;
	}
}

void DataHandle::StreamRead(kGUIString *s,unsigned long numbytes)
{
	unsigned long numavail;
	unsigned long blockoffset;
	StreamBlock *cb;
	unsigned long long index=m_offset;

	s->Clear();
	/* traverse stream blocks if necessary */
	while(numbytes>0)
	{
		cb=LoadStreamBlock(index,&blockoffset,true);

		/* num bytes that are available in this block */
		numavail=MIN(m_streamblocksize-blockoffset,numbytes);

		/* add this chunk to the string */
		s->Append(cb->m_data+blockoffset,numavail);

		/* subtract number of bytes copied and loop if necessary */
		numbytes-=numavail;
		index+=numavail;
	}
}

/* this function is ONLY available if caching is turned on */
char DataHandle::StreamCmp(const void *cmpstring,unsigned long numbytes,long offset,bool cs)
{
	const char *cp=(char *)cmpstring;
	unsigned long numavail;
	unsigned long blockoffset;
	StreamBlock *cb;
	char delta;
	unsigned long long index=m_offset+offset;

	assert(m_readstream==true,"This function is only valid if Caching has been enabled!");

	/* traverse stream blocks if necessary */
	while(numbytes>0)
	{
		cb=LoadStreamBlock(index,&blockoffset,false);

		/* num bytes that are available in this block */
		numavail=MIN(m_streamblocksize-blockoffset,numbytes);
	
		/* compare the number of bytes available */
		if(!cs)
			delta=strncmp(cp,cb->m_data+blockoffset,numavail);
		else
			delta=strcmpin(cp,cb->m_data+blockoffset,numavail);
		if(delta)
			return(delta);

		/* subtract number of compared copied and loop if necessary */
		numbytes-=numavail;
		index+=numavail;
		cp+=numavail;
	}
	return(0);	/* same */
}

void DataHandle::PurgeReadStream(void)
{
	StreamBlock *cb;
	unsigned int i;

	for(i=0;i<m_numstreamblocks;++i)
	{
		cb=m_streamblocks.GetEntryPtr(i);
		delete []cb->m_data;
	}
	m_numstreamblocks=0;
	m_numdatasections=0;
	m_readstream=false;
}
