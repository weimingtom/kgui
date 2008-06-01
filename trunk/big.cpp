/**********************************************************************************/
/* kGUI - big.cpp                                                                 */
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

/* todo: if waste>10% then compress and rebuild file */

extern long MS_FileTime(const char *fn);

BigFile::BigFile()
{
	m_dirloaded=false;
	m_numdirblocks=0;
	m_dirchanged=false;
	m_dirblocks.Alloc(16);
	m_dirblocks.SetGrow(true);
	m_dirblocks.SetGrowSize(16);
	m_hash.Init(16,sizeof(BigFileEntry));
}

BigFile::~BigFile()
{
	UpdateDir();		/* if any pending changes then write them now */
	FreeDirBlocks();	/* only allocated if opened in edit mode */
}

BigFileEntry *BigFile::Locate(const char *fn)
{
	return((BigFileEntry *)m_hash.Find(fn));
}

void BigFile::FreeDirBlocks(void)
{
	unsigned int b;
	DIRINFO_DEF di;

	for(b=0;b<m_numdirblocks;++b)
	{
		di=m_dirblocks.GetEntry(b);

		delete []di.data;
	}
	m_numdirblocks=0;
	m_dirloaded=false;
}

/* if edit flag is set then keep directory blocks in memory */
/* since they are written out as a large block for encryption */

void BigFile::Load(bool edit)
{
	long doffset;
	long deoffset;
	char sentinel[4];
	DataHandle section;
	DIRINFO_DEF di;

	UpdateDir();	/* if any pending changes then write them now */
	FreeDirBlocks();

	m_dirloaded=true;
	m_edit=edit;
	m_hash.Init(16,sizeof(BigFileEntry));	/* re-init */
	m_empty=false;
	m_bad=false;

	/* since encryption is based on a specific size chunk, the bigfile */
	/* is processed in chunks */

	SetOpenLock(true);	/* no root file access, only via area sections */

	CopyArea(&section,0,12,GetTime());
	if(section.Open()==false)
	{
		m_empty=true;
		return;
	}
	section.Read(&sentinel,(unsigned long)4);

	if(!(sentinel[0]=='B' && sentinel[1]=='I' && sentinel[2]=='G' && sentinel[3]=='F'))
	{
		m_bad=true;
		section.Close();		/* bad file type, or wrong encryption */
		return;
	}
	/* read in the current end of file position */
	section.Read(&m_curend,(unsigned long)4);
	section.Read(&m_curwaste,(unsigned long)4);
	section.Close();

	/* load directory blocks and add entries to the hash table */
	doffset=12;
	do
	{
		char *dirblock;
		unsigned long dirsize;
		BigFileEntry bfe;
		BIGENTRY_DEF *be;

		/* load a directory block */
		CopyArea(&section,doffset,sizeof(m_lastheader),GetTime());
		section.Open();
		section.Read(&m_lastheader,(unsigned long)sizeof(m_lastheader));
		section.Close();

		/* load the directory block */
		dirsize=m_lastheader.blocksize-sizeof(m_lastheader);

		CopyArea(&section,doffset+sizeof(m_lastheader),dirsize,GetTime());
		section.Open();

		dirblock=new char[m_lastheader.blocksize];
		memcpy(dirblock,&m_lastheader,sizeof(m_lastheader));
		section.Read(dirblock+sizeof(m_lastheader),dirsize);
		section.Close();

		/* save block in memory if opened for edit */
		if(edit==true)
		{
			di.changed=false;
			di.size=m_lastheader.blocksize;
			di.offset=doffset;
			di.data=dirblock;
			m_dirblocks.SetEntry(m_numdirblocks,di);
			++m_numdirblocks;
		}

		be=(BIGENTRY_DEF *)(dirblock+sizeof(m_lastheader));
		deoffset=doffset+sizeof(m_lastheader);
		for(unsigned int i=0;i<m_lastheader.numfiles;++i)
		{
			if(be->nameoffset)	/* 0=deleted */
			{
				/* check for any corruption */
				assert((be->nameoffset>=sizeof(BIGHEADER_DEF)) && (be->nameoffset<=m_lastheader.blocksize),"Name offset error!");
				assert((be->dataoffset<=m_curend),"Data offset is past end of file!");

				bfe.m_dirblocknum=m_numdirblocks-1;	/* only valid in edit mode */
				bfe.m_diroffset=deoffset;
				bfe.m_offset=be->dataoffset;
				bfe.m_time=be->time;
				bfe.m_crc=be->crc;
				bfe.m_size=be->size;
				assert(m_curend>=(bfe.m_offset+bfe.m_size),"Internal Error!");

				/* add this filename to the hash table */
				m_hash.Add(dirblock+be->nameoffset,&bfe);
			}
			deoffset+=sizeof(BIGENTRY_DEF);
			++be;
		}
		if(edit==false)
			delete []dirblock;	/* delete and re-allocate just incase size changes from block to block */
		doffset=m_lastheader.nextblock;
	}while(doffset);
}

/* calculate the crc for a buffer */

unsigned long BigFile::CrcBuffer(long startcrc,const unsigned char *buf,unsigned long buflen)
{
	unsigned long crc;
	unsigned char byte;
	unsigned long i;

	crc=startcrc;
	for(i=0;i<buflen;++i)
	{
		byte=*(buf++)&0xff;
		crc=byte^(crc<<6);
		crc^=crc>>24;
		crc&=0xffffff;
	}
	return(crc);
}

bool BigFile::CheckDuplicate(DataHandle *af)
{
	unsigned long crc;
	unsigned long fs;
	int e,nums;
	HashEntry *she;
	BigFileEntry *sfe;
	unsigned char crcbuf[4096];

	/* calc CRC on file */
	crc=0;
	af->Open();
	fs=af->GetSize();
	while(fs>sizeof(crcbuf))
	{
		af->Read(crcbuf,(unsigned long)sizeof(crcbuf));
		crc=CrcBuffer(crc,crcbuf,sizeof(crcbuf));
		fs-=sizeof(crcbuf);
	};
	/* read remainder */
	if(fs>0)
	{
		af->Read(crcbuf,fs);
		crc=CrcBuffer(crc,crcbuf,fs);
	}
	af->Close();
	fs=af->GetSize();

	nums=GetNum();
	she=GetFirst();
	for(e=0;e<nums;++e)
	{
		sfe=(BigFileEntry *)she->m_data;
		if(sfe->m_crc==crc && sfe->m_size==fs)
			return(true);		/* duplicate!!! */
		she=she->GetNext();
	}
	return(false);
}

/* fn is the full path name, addfn is the shorter name to use */

bool BigFile::AddFile(const char *fn,DataHandle *af,bool updatedir)
{
	BigFileEntry *bfe;
	BIGENTRY_DEF de;
	unsigned char copybuf[4096];
	long crc;
	DataHandle section;
	DIRINFO_DEF di;
	char *dirblock;
	unsigned long asize=af->GetSize();
	long ft=af->GetTime();
	unsigned long wasize;

	printf("Adding file '%s\n",af->GetFilename()->GetString());

	assert(m_edit==true,"BigFile not opened for write, use Load(true)");

	if(m_empty==true)
	{
		/* generate a blank directory header for this file */
		m_curend=FIRSTDBLOCKSIZE+12;
		m_curwaste=0;

		/* write in sections since if big file is encrypted then each section */
		/* is encrypted seperately */

		CopyArea(&section,0,12,GetTime());
		if(section.OpenWrite("wb")==false)
		{
			printf("Error making new file '%s'\n",(GetFilename()->GetString()));
			return(false);
		}
		section.Write("BIGF",4);
		section.Write(&m_curend,4);
		section.Write(&m_curwaste,4);
		section.Close();

		CopyArea(&section,12,sizeof(m_lastheader),GetTime());
		section.OpenWrite("rb+");
		m_lastheader.blocksize=FIRSTDBLOCKSIZE;
		m_lastheader.fntail=FIRSTDBLOCKSIZE;
		m_lastheader.offset=12;
		m_lastheader.numfiles=0;
		m_lastheader.nextblock=0;
		section.Write(&m_lastheader,sizeof(m_lastheader));
		section.Close();

		/* allocate block */
		dirblock=new char[m_lastheader.blocksize];
		memset(dirblock,m_lastheader.blocksize,0);
		memcpy(dirblock,&m_lastheader,sizeof(m_lastheader));

		di.changed=false;
		di.size=m_lastheader.blocksize;
		di.offset=m_lastheader.offset;
		di.data=dirblock;
		m_dirblocks.SetEntry(m_numdirblocks,di);
		++m_numdirblocks;

		m_empty=false;
	}

	/* does this file already exist? */
	bfe=(BigFileEntry *)m_hash.Find(fn);
	if(bfe)
	{
		unsigned long woffset;
		unsigned long deoffset;

		/* if it is the same size or smaller then just update the */
		/* length and time, otherwise add to end of file */
		
		/* copy the entry into the di struct */
		di=m_dirblocks.GetEntry(bfe->m_dirblocknum);
		deoffset=bfe->m_diroffset-di.offset;
		memcpy(&de,di.data+deoffset,sizeof(de));

		/* update the file time */
		de.time=ft;

		/* was it at the end of the file? */
		if((de.dataoffset+de.size)==m_curend)
		{
			/* overwrite old file and update new length of file */
			m_curend+=(asize-de.size);	/* update bigfile size */
			de.size=asize;				/* update the file size */
			woffset=de.dataoffset;		/* write at old place */
		}
		else if(asize<=de.size)
		{
			/* if equal or smaller then re-use the buffer */
			m_curwaste+=(de.size-asize);	/* add difference to waste */
			de.size=asize;	/* update the size */
			woffset=de.dataoffset;
		}
		else	/* add to end of file instead */
		{
			m_curwaste+=de.size;	/* add old size to waste */
			woffset=m_curend;
			de.size=asize;			/* update the size */
			de.dataoffset=m_curend;
			m_curend+=asize;
		}

		/* write large chunks, first */
		crc=0;
		CopyArea(&section,woffset,asize,GetTime());
		if(section.OpenWrite("rb+")==false)
			return(false);			/* error! */

		if(af->Open()==false)
		{
			/* cannot open input file for read */
			printf("Cannot open input file '%s' for read!\n",af->GetFilename()->GetString());
			return(false);
		}

		while(asize>sizeof(copybuf))
		{
			af->Read(copybuf,(unsigned long)sizeof(copybuf));
			crc=CrcBuffer(crc,copybuf,sizeof(copybuf));
			section.Write(copybuf,sizeof(copybuf));

			asize-=sizeof(copybuf);
		};
		/* write remainder */
		if(asize>0)
		{
			af->Read(copybuf,asize);
			crc=CrcBuffer(crc,copybuf,asize);
			section.Write(copybuf,asize);
		}
		af->Close();
		if(section.Close()==false)
			return(false);			/* write error, abort! */

		de.crc=crc;
		/* update directory entry */

		/* copy directory entry into directory block */
		memcpy(di.data+deoffset,&de,sizeof(de));

		/* flag the directory block as being changed*/
		di.changed=true;
		m_dirblocks.SetEntry(bfe->m_dirblocknum,di);

		/* flag the directory as changed */
		m_dirchanged=true;
	}
	else
	{
		unsigned int namelen;
		unsigned int desize;
		unsigned int deoffset;

		namelen=(unsigned int)strlen(fn)+1;
		/* is there room in the current directory block? */
		deoffset=sizeof(m_lastheader)+m_lastheader.numfiles*sizeof(BIGENTRY_DEF);
		desize=sizeof(BIGENTRY_DEF)+namelen;
		if((deoffset+desize)>m_lastheader.fntail)
		{
			/* ran out of space, add a new directory block */

			/* make previous block link to new one */

			di=m_dirblocks.GetEntry(m_numdirblocks-1);
			m_lastheader.nextblock=m_curend;
			memcpy(di.data,&m_lastheader,sizeof(m_lastheader));
			di.changed=true;
			m_dirblocks.SetEntry(m_numdirblocks-1,di);

			/* generate a new block at the end of the file */

			m_lastheader.blocksize=DBLOCKSIZE;
			m_lastheader.fntail=DBLOCKSIZE;
			m_lastheader.offset=m_curend;	/* block is here */
			m_lastheader.numfiles=0;
			m_lastheader.nextblock=0;

			/* update end of file variable */
			m_curend+=DBLOCKSIZE;
			deoffset=sizeof(m_lastheader);

			/* allocate a new block */
			dirblock=new char[m_lastheader.blocksize];
			memset(dirblock,m_lastheader.blocksize,0);
			memcpy(dirblock,&m_lastheader,sizeof(m_lastheader));

			di.changed=true;				/* write it to the file */
			di.size=m_lastheader.blocksize;
			di.offset=m_lastheader.offset;
			di.data=dirblock;
			m_dirblocks.SetEntry(m_numdirblocks,di);
			++m_numdirblocks;
		}

		/* write the data file first, then update the directory and file headers */

		assert(m_curend!=0,"Internal error!");
		CopyArea(&section,m_curend,asize,GetTime());
		if(section.OpenWrite("rb+",asize)==false)
			return(false);

		/* write large chunks, first */
		af->Open();
		crc=0;
		wasize=asize;
		while(wasize>sizeof(copybuf))
		{
			af->Read(copybuf,(unsigned long)sizeof(copybuf));
			crc=CrcBuffer(crc,copybuf,sizeof(copybuf));
			section.Write(copybuf,sizeof(copybuf));
			wasize-=sizeof(copybuf);
		};
		/* write remainder */
		if(wasize>0)
		{
			af->Read(copybuf,asize);
			crc=CrcBuffer(crc,copybuf,wasize);
			section.Write(copybuf,wasize);
		}
		af->Close();
		if(section.Close()==false)
			return(false);

		/* ok, fill in the directory entry */

		m_lastheader.fntail-=namelen;
		m_lastheader.numfiles+=1;

		/* copy struct to the direcctory block */
		di=m_dirblocks.GetEntry(m_numdirblocks-1);
		memcpy(di.data,&m_lastheader,sizeof(m_lastheader));
		di.changed=true;
		m_dirblocks.SetEntry(m_numdirblocks-1,di);

		de.nameoffset=m_lastheader.fntail;
		de.dataoffset=m_curend;
		de.size=asize;
		de.time=ft;
		de.crc=crc;

		/* copy directory entry into directory block */
		memcpy(di.data+deoffset,&de,sizeof(de));
		/* copy the filename next */
		memcpy(di.data+de.nameoffset,fn,namelen);

		m_curend+=asize;
		m_dirchanged=true;

	}
	if(updatedir==true)
		UpdateDir();
	return(true);
}

void BigFile::UpdateDir(void)
{
	unsigned int b;
	DIRINFO_DEF di;
	DataHandle section;

	if(m_dirchanged==true)
	{
		/* write directory blocks that have changed */

		for(b=0;b<m_numdirblocks;++b)
		{
			di=m_dirblocks.GetEntry(b);
			if(di.changed==true)
			{
				/* since each directory block can be sized differently, and encryption */
				/* is based on the data size, we need to write each directory block in */
				/* two chunks, first the header, then the variable size fileentry area */

				CopyArea(&section,di.offset+sizeof(BIGHEADER_DEF),di.size-sizeof(BIGHEADER_DEF),GetTime());
				section.OpenWrite("rb+");
				section.Write(di.data+sizeof(BIGHEADER_DEF),di.size-sizeof(BIGHEADER_DEF));
				section.Close();

				CopyArea(&section,di.offset,sizeof(BIGHEADER_DEF),GetTime());
				section.OpenWrite("rb+");
				section.Write(di.data,sizeof(BIGHEADER_DEF));
				section.Close();

				/* clear the changed flag on this block since it has now been written */
				di.changed=false;
				m_dirblocks.SetEntry(b,di);
			}
		}

		/* update header block of file */
		CopyArea(&section,0,12,GetTime());
		section.OpenWrite("rb+");
		section.Write("BIGF",4);
		section.Write(&m_curend,4);
		section.Write(&m_curwaste,4);
		section.Close();
		m_dirchanged=false;
	}
}

#define MAXBUF 65536

bool BigFile::Extract(const char *name,const char *filename)
{
	BigFileEntry *bfe;
	FILE *f2;
	char *buf;
	unsigned int bufsize;
	unsigned int readsize;
	unsigned int left;
	bool writeerror=false;
	DataHandle section;

	bfe=Locate(name);
	if(!bfe)
		return(false);	/* error file doesn't exist */

	CopyArea(&section,bfe->m_offset,bfe->m_size,GetTime());
	if(section.Open()==false)
		return(false);
	
	/* allocate buffer to copy into and out of */

	left=bfe->m_size;	/* bytes left to copy */
	
	if(bfe->m_size>MAXBUF)
		bufsize=MAXBUF;
	else
		bufsize=bfe->m_size;

	buf=new char[bufsize];	/* alloc buffer to copy file via */
	if(!buf)
		return(false);	/* not enough memory to copy file */

	f2=fopen(filename,"wb");
	if(!f2)
	{
		delete buf;
		return(false);	/* error opening output file */
	}

	while(left>0 && writeerror==false)
	{
		if(left>bufsize)
			readsize=bufsize;
		else
			readsize=left;
		if(section.Read(buf,(unsigned long)readsize)!=readsize)
			writeerror=true;
		if(fwrite(buf,1,readsize,f2)!=readsize)
			writeerror=true;
		left-=readsize;
	}
	section.Close();
	fclose(f2);
	delete buf;
	if(writeerror)
		return(false);
	return(true);	/* ok */
}

/* todo, only work in edit mode and flag dir as changed and call updatedir */

void BigFile::Delete(const char *fn)
{
	BigFileEntry *bfe;
	DIRINFO_DEF di;

	assert(m_edit==true,"Error: file is not opened for edit, use Load(true)\n");

	bfe=(BigFileEntry *)m_hash.Find(fn);

	if(!bfe)
		return;

	/* clear the directory entry for this file and flag it for update */

	di=m_dirblocks.GetEntry(bfe->m_dirblocknum);
	memset(di.data+(bfe->m_diroffset-di.offset),0,sizeof(BIGENTRY_DEF));
	di.changed=true;
	m_dirblocks.SetEntry(bfe->m_dirblocknum,di);
	m_dirchanged=true;

	UpdateDir();	/* write to directory */
}
