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

#include "kgui.h"
#include "kguiprot.h"

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
}

DataHandle::~DataHandle()
{
	if(m_handle)
		fclose(m_handle);
	assert(m_handle==0 && m_open==false && m_wopen==false,"File was left open, aarrgghh!");
}

void DataHandle::SetEncryption(class kGUIProt *p)
{
	assert(m_open==false && m_wopen==false,"Encryption needs to be set before opening file!");
	m_prot=p;
}

void DataHandle::SetFilename(const char *fn)
{
	int i,sok;
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
			sok=fseek(fp,0,SEEK_END);
			assert(sok==0,"Seek error!");

			m_filesize=ftell(fp);
			fclose(fp);
		}
		else
			m_filesize=0;
	}
	HandleChanged();
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

void DataHandle::SetMemory(const unsigned char *memory,unsigned long filesize)
{
	/* make sure file is closed */
	assert(m_open==false && m_wopen==false,"Last file needs to be closed before starting a new one!");

	m_type=DATATYPE_MEMORY;
	m_memory=memory;
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

bool DataHandle::Open(void)
{
	int sok;
	/* if this datafile encrypted, then completely */
	/* load the file into memory, unencrypt it and feed it */
	/* from memory, purge when code calls Close() */

	if(m_wopen==true)
		return(false);	/* cannot open for read, someone is writing to it aleady */

	assert(m_lock==false,"Error: file is locked!");
	switch(m_type)
	{
	case DATATYPE_FILE:
#if 0
		if(!m_filesize)
		{
			/* if file didn't extst when SetFilename was called then try again now */
			SetFilename(GetFilename());
		}
#endif
		m_offset=0;
		assert(m_open==false,"Already Open!");
		m_handle=fopen(m_fn.GetString(),"rb");
		if(m_handle)
		{
			if(m_startoffset)
			{
				sok=fseek(m_handle,m_startoffset,SEEK_SET);
				assert(sok==0,"Seek error!");
			}
			m_open=true;

			/* if file is encrypted, then load the whole thing in */
			/* then decrypt it and feed it from the buffer */

			if(m_prot)	/* is this file encrypted? */
			{
				unsigned char *encdata=new unsigned char[m_filesize];
				long long numread;

				numread=(long long)fread(encdata,1,m_filesize,m_handle);
				assert(numread==m_filesize,"Error reading data!");
				fclose(m_handle);
				m_handle=0;
				
				/* only handle files less than 2gb size */
				m_memory=(const unsigned char *)m_prot->Decrypt(encdata,(long)m_filesize);
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
	return(m_open);
}

bool DataHandle::OpenWrite(const char *wtype,unsigned long defbufsize)		/* write mode string "wb" or "rb+" etc */
{
	int sok;

	assert(m_lock==false,"Error: file is locked!");
	assert(m_open==false,"File is already open for read!");
	assert(m_wopen==false,"File is already open for write!");
//	assert(m_type==DATATYPE_FILE,"only valid for files!");

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
				sok=fseek(m_handle,m_startoffset,SEEK_SET);
				assert(sok==0,"Seek error!");
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
	return(m_wopen);
};

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
		m_writebuffer.Alloc(max(m_writebufferlen+numbytes,m_writebuffer.GetNumEntries()+65536));	

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
		HandleChanged();
	}
	else
	{
		assert(false,"File is not open for either read or write!");
	}
	return(ok);
}

void DataHandle::Seek(long long index)
{
	int sok;
	assert(m_open==true,"File not open error!");

	m_offset=index;
	switch(m_type)
	{
	case DATATYPE_FILE:
		if(!m_prot)
		{
			sok=fseek(m_handle,m_startoffset+m_offset,SEEK_SET);
			assert(sok==0,"Seek error!");
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

unsigned char DataHandle::PeekChar(void)
{
	char c;

	Read(&c,(unsigned long)1);
	Seek(m_offset-1);	/* go back 1 char */
	return c;
}

unsigned char DataHandle::ReadChar(void)
{
	char c;

	Read(&c,(unsigned long)1);
	return c;
}

unsigned long DataHandle::Read(kGUIString *s,unsigned long numbytes)
{
	unsigned long numread;
	char *buffer;

	buffer=new char [numbytes];
	numread=Read(buffer,numbytes);
	s->SetString(buffer,numread);
	delete []buffer;
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
			fread(dest,1,numbytes,m_handle);
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
		if((c==0x0d) || (c==0x0a))
			break;
		line->Append(c);
	}

	if(Eof()==true)
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
