/**********************************************************************************/
/* kGUI - kguidir.cpp                                                             */
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

/**********************************************************************************/
/*                                                                                */
/* A simple class for reading directories. Filenames and directory names are      */
/* added to a hash table. Stored filenames can be local names or full path names. */
/* It can also scan just the given directory or recursively follow subdirectories */
/* It can also be given a list of 'extensions' and only collect filenames that    */
/* use those suffixes.                                                            */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"

#if defined(LINUX)

#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <linux/types.h>
#include <linux/unistd.h>

#elif defined(MACINTOSH)

#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/file.h>

#else

#include <errno.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <sys/utime.h>
#include <shlwapi.h>
/* used for get desktop name function */
#include <shlobj.h>
#endif

kGUIDir::~kGUIDir()
{
	Purge();
}

void kGUIDir::Purge(void)
{
	m_numfiles=0;
	m_numdirs=0;
}

void kGUIDir::LoadDir(const char *path,bool recursive,bool fullnames,const char *ext)
{
	int numexts;
	kGUIString extstring;
	kGUIStringSplit exts;

	if(ext)
	{
		extstring.SetString(ext);
		numexts=exts.Split(&extstring,";");
	}
	Purge();
	LoadDir2(path,recursive,fullnames,&exts);
}

void kGUIDir::LoadDir2(const char *path,bool recursive,bool fullnames,kGUIStringSplit *exts)
{
	char tempname[1024];
	char *nameplace;
	char *fn;
	bool isdir;
	char dirc[]={DIRCHAR};
#if defined(LINUX) || defined(MACINTOSH)
	int dir_handle;
	int bufsize;
	char buffer[8192];
	struct direct *direntry;
	struct stat statbuf;
#if _FILE_OFFSET_BITS==64
	long long zzzz;
#else
	long zzzz;
#endif
#else
	intptr_t dir_handle;
	struct _finddata_t  dir_info;
#endif

#if defined(LINUX) || defined(MACINTOSH)
	strcpy(tempname,path);
	if(strlen(tempname)>1)
	{
		//remove trailing '/'
		if(tempname[strlen(tempname)-1]==dirc[0])
			tempname[strlen(tempname)-1]=0;
	}
	dir_handle=open(tempname,O_RDONLY);				/* open directory for reading */
	if(dir_handle==-1)
		return;	/* cannot open dir */
	direntry=(struct direct *)buffer;
	fn=direntry->d_name;
	bufsize=0;

	// if path doesn't end in '/' then append '/'
	if(tempname[strlen(tempname)-1]!=dirc[0])
		strcat(tempname,DIRCHAR);
	nameplace=tempname+strlen(tempname);
#else
	strcpy(tempname,path);
	if(strlen(tempname)==0)
		strcat(tempname,DIRCHAR);
	else if(tempname[strlen(tempname)-1]!=dirc[0])
		strcat(tempname,DIRCHAR);
	nameplace=tempname+strlen(tempname);
	strcat(tempname,"*");
	dir_handle = _findfirst(tempname, &dir_info);
	if(dir_handle<0)
		return;			/* path must be unmounted or something is wrong! */
	fn=dir_info.name;
#endif
	do
	{
#if defined(LINUX) || defined(MACINTOSH)
		if(bufsize<=20)
			bufsize+=getdirentries(dir_handle,buffer+bufsize,sizeof(buffer)-bufsize,&zzzz);
		if(bufsize<=0)
			break;
#endif
		if(fn[0]!='.')
		{
			strcpy(nameplace,fn);	/* put name after path/ */

			isdir=false;
#if defined(LINUX) || defined(MACINTOSH)
			if(stat(tempname,&statbuf)!=-1)
			{
				if(S_ISDIR(statbuf.st_mode))
					isdir=true;
			}
#else
			if(dir_info.attrib&16)
				isdir=true;
#endif
			if(isdir)
			{
				kGUIString *t;

				t=m_dirnames.GetEntryPtr(m_numdirs++);
				if(fullnames==true)
					t->SetString(tempname);
				else
					t->SetString(fn);

				if(recursive==true)
					LoadDir2(tempname,true,fullnames,exts);
			}
			else
			{
				unsigned int e;
				bool addme;
				const char *eplace;

				addme=true;
				if(exts->GetNumWords())
				{
					addme=false;
					for(e=0;e<exts->GetNumWords() && addme==false;++e)
					{
						eplace=strstri(fn,exts->GetWord(e)->GetString());
						if(eplace)
						{
							if(strlen(eplace)==exts->GetWord(e)->GetLen())
								addme=true;
						}
					}
				}
				if(addme==true)
				{
					kGUIString *t;

					t=m_filenames.GetEntryPtr(m_numfiles++);
					if(fullnames==true)
						t->SetString(tempname);
					else
						t->SetString(fn);
				}
			}
		}
#if defined(LINUX) || defined(MACINTOSH)
		bufsize-=direntry->d_reclen;
		memmove(buffer,buffer+direntry->d_reclen,bufsize);
#else
		if(_findnext(dir_handle, &dir_info)==-1)
			break;
#endif
	}while(1);
#if defined(LINUX) || defined(MACINTOSH)
	close(dir_handle);
#else
	_findclose(dir_handle);
#endif
}

void kGUIDir::LoadDrives(void)
{
#if defined (WIN32) || defined(MINGW)
	int i,blen;
	char dstrings[4096];
#endif

	Purge();

#if defined (WIN32) || defined(MINGW)
	/* add desktop to start of list */
//	if(SHGetFolderPath(HWND_DESKTOP, CSIDL_DESKTOP,NULL, 0,dstrings)==TRUE)
	if(SHGetSpecialFolderPath(NULL, dstrings, CSIDL_DESKTOP, 0)==TRUE)
	{
		kGUIString *t;
		
		t=m_dirnames.GetEntryPtr(m_numdirs++);
		t->SetString(dstrings);
	}

	blen=GetLogicalDriveStrings(sizeof(dstrings)-1,dstrings);
	i=0;
	while(i<blen)
	{
		kGUIString *t;
		
		t=m_dirnames.GetEntryPtr(m_numdirs++);
		t->SetString(dstrings+i);
		t->Clip(2);
		i+=(int)strlen(dstrings+i)+1;
	}
#endif
}

/* scan entries from within a bigfile */
void kGUIDir::LoadDir(BigFile *bf,const char *ext)
{
	int i,num;
	HashEntry *she;
	BigFileEntry *sfe;
	kGUIString *t;

	m_numfiles=0;
	m_numdirs=0;

	num=bf->GetNum();
	she=bf->GetFirst();
	for(i=0;i<num;++i)
	{
		sfe=(BigFileEntry *)she->m_data;

		t=m_filenames.GetEntryPtr(m_numfiles++);
		t->SetString(sfe->GetName());
		she=she->GetNext();
	}
}

