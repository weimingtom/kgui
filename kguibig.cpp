/**********************************************************************************/
/* kGUI - kguibig.cpp                                                            */
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

/*! @file kguibig.cpp 
    @brief This is the stand along console app for bigfile generation and extraction.
	As well as the regular commands like adding, deleting and extracting files from
	within bigfiles it can also be used to synchronize two bigfiles. It can also be
	used to add files to encrypted bigfiles or extract files from encrypted bigfiles */

#include "kgui.h"
#include "kguiprot.h"

typedef struct
{
	time_t time;
	char root;
}FDATA;

bool optrecursive;
bool optverify;
bool optcompress;
bool optdelete;
bool optmissing;

enum
{
SOURCE_FILE,
SOURCE_DIR,
SOURCE_BIG
};

bool g_userabort=false;

#if defined(LINUX) || defined(MACINTOSH)
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(MINGW)
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#elif defined(WIN32)
#include <io.h>
#include <sys/stat.h>
#include <signal.h>
#else
#error
#endif

#include "kguidir.cpp"

#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

bool MS_IsDir (const char *file)
{
    struct stat sb;

    if (stat(file, &sb) < 0)
		return (false);
    return (S_ISDIR(sb.st_mode));
}

class kGUISystemBig : public kGUISystem
{
public:
	void FileShow(const char *filename) {}
	void ShellExec(const char *program,const char *parms,const char *dir) {}
	void Copy(kGUIString *s) {}
	void Paste(kGUIString *s) {}
	void Sleep(int ms) {}
	bool MakeDirectory(const char *name) {return false;}
	long FileTime(const char *fn);
	void HideWindow(void) {}
	void ShowWindow(void) {}
	void ReDraw(void) {}
	void ChangeMouse(void) {}
	void ShowMouse(bool show) {}
	void GetWindowPos(int *x,int *y,int *w,int *h) {}
	void SetWindowPos(int x,int y) {}
	void SetWindowSize(int w,int h) {}
	void Minimize(void) {}
	void MoveWindowPos(int dx,int dy) {}
	void AdjustMouse(int dx,int dy) {}
	bool IsDir(const char *fn) {return MS_IsDir(fn);}
	bool NeedRotatedSurfaceForPrinting(void) {return false;}

	unsigned int GetNumPrinters(void) {return 0;}
	void GetPrinterInfo(const char *name,int *pw,int *ph,int *ppih,int *ppiv) {}
	int GetDefPrinterNum(void) {return 0;}
	kGUIPrinter *GetPrinterObj(int pid) {return 0;}

	class kGUIPrintJob *AllocPrintJob(void) {return 0;}

private:
};

kGUISystem *kGUI::m_sys;

long FileSize(const char *filename)
{
	FILE *fp;
	long fs;

	fp=fopen(filename,"rb");
	if(!fp)
		return(0);
	fseek(fp,0,SEEK_END);
	fs=ftell(fp);
	fclose(fp);
	return(fs);
}


/**********************************************************************/


static void sigint_handler(int sig)  //static declaration
{
//	printf("MANAGER : Signal Detected.! (%d)\n",sig);
	switch (sig)
	{  // interrupt signal detected
	case SIGINT: 
		g_userabort=true;
	break;
	}
}
//Debug\kguibig _data.big big big/

int main(int argc, char* argv[])
{
	int i;
	int nums;
	FDATA fdata;
	Hash AddHash;
	BigFile DestBigFile;
	BigFile SourceBigFile;
	BigFileEntry *bfe;
	int numok,numnew,numupdated;
	int sourcetype;
	bool update;
	char *p1=0;
	char *p2=0;
	char *p3=0;
	int tosspath=0;
	DataHandle addhandle;
	int vargc;
	char **vargv;
	vargc=argc;
	vargv=argv;

	/* handle encrypted file */
	kGUIProt DestProt;
	bool usedestprot;
	kGUIProt SourceProt;
	bool usesourceprot;
	kGUISystemBig sysbig;

	kGUI::SetSystem(&sysbig);
	signal(SIGINT, sigint_handler);
	optcompress=false;
	optrecursive=true;
	optverify=false;
	optdelete=false;
	optmissing=false;
	usedestprot=false;
	usesourceprot=false;

#if 0
	p1="/source/kgui/_data.big";
	p2="/source/kgui/big";
	p3="/source/kgui/big/";
	optverify=true;
#endif

	for(i=1;i<vargc;++i)
	{
		if(vargv[i][0]=='-')
		{
			switch(vargv[i][1])
			{
			case 'c':
				optcompress=true;
			break;
			case 'd':
				optdelete=true;
			break;
			case 'm':
				optmissing=true;	/* delete missing files from subdir */
			break;
			case 'v':
				optverify=true;
			break;
			case 'r':
				optrecursive=false;
			break;
			case 'k':			
				if(vargv[i][2]=='d')	/* encryption key on destination file */
				{
					printf("using dest key\n");
					if(DestProt.SetKey(vargv[i+1],atoi(vargv[i+2]),atoi(vargv[i+3]),true)==false)
					{
						printf("Error loading dest keyfile '%s'\n",vargv[i+1]);
						return(0);
					}
					usedestprot=true;
					i+=3;
				}
				else if(vargv[i][2]=='s') /* encryption key on source file */
				{
					printf("using source key\n");
					if(SourceProt.SetKey(vargv[i+1],atoi(vargv[i+2]),atoi(vargv[i+3]),true)==false)
					{
						printf("Error loading source keyfile '%s'\n",vargv[i+1]);
						return(0);
					}
					usesourceprot=true;
					i+=3;
				}
				optrecursive=false;
			break;
			default:
				printf("Unknown parm '%s'\n",vargv[i]);
				return(0);
			break;
			}
		}
		else
		{
			if(!p1)
				p1=vargv[i];
			else if(!p2)
				p2=vargv[i];
			else if(!p3)
			{
				p3=vargv[i];
				tosspath=(int)strlen(p3);
			}
			else
			{
				printf("Unknown parm '%s'\n",vargv[i]);
				return(0);
			}
		}
	}

	/* need at least 1 parm */
	if(!p1)
	{
		printf("kguibig: (c) 2005 Kevin Pickelll\n");
		printf("usage: kguibig bigfile.big path [root]\n");
		printf(" -c = compress\n");
		printf(" -v = verify\n");
		printf(" -r = don't recurse\n");
		printf(" -k[d,s] = filename offset len // source/dest key\n");
		return(0);
	}

	DestBigFile.SetFilename(p1);
	if(usedestprot==true)
		DestBigFile.SetEncryption(&DestProt);
	DestBigFile.Load(true);
	if(DestBigFile.IsBad()==true)
	{
		printf("Dest file exists and is not a bigfile, or decryption key is incorrect!\n");
		return(0);
	}

	/* list, verify or compress ? */
	if(!p2)
	{
		if(optcompress)
		{
		}
		else
		{
			unsigned long crc;
			unsigned long vfsize;
			BigFileEntry *sfe;
			unsigned char copybuf[65536];
			DataHandle checkhandle;

			/* verify or list */
			nums=DestBigFile.GetNumEntries();
			for(i=0;((i<nums) && (g_userabort==false));++i)
			{
				sfe=(BigFileEntry *)DestBigFile.GetEntry(i);

				if(optverify)
				{
					/* check crc and print if no match */
					printf("%d%c",i,13);
					vfsize=sfe->m_size;
	
					crc=0;
					DestBigFile.CopyArea(&checkhandle,sfe->m_offset,sfe->m_size,sfe->m_time);
					checkhandle.Open();
					while(vfsize>sizeof(copybuf))
					{
						checkhandle.Read(&copybuf,(unsigned long)sizeof(copybuf));
						crc=DestBigFile.CrcBuffer(crc,copybuf,sizeof(copybuf));
						vfsize-=sizeof(copybuf);
					};
					/* write remainder */
					if(vfsize>0)
					{
						checkhandle.Read(&copybuf,vfsize);
						crc=DestBigFile.CrcBuffer(crc,copybuf,vfsize);
					}
					checkhandle.Close();
					if(crc!=sfe->m_crc)
						printf("CRC Error on file '%s' %06x<>%06x\n",sfe->GetName()->GetString(),(int)crc,sfe->m_crc); 
				}
				else	/* assume list if verify is not set */
					printf("%s, len=%d,crc=%06x\n",sfe->GetName()->GetString(),sfe->m_size,sfe->m_crc);
			}
		}
		return(0);
	}

	AddHash.Init(16,sizeof(FDATA));

	/* is p2 a bigfile? */
	if(strstr(p2,".big"))
	{

		SourceBigFile.SetFilename(p2);
		if(usesourceprot==true)
			SourceBigFile.SetEncryption(&SourceProt);
		SourceBigFile.Load(true);
		if(SourceBigFile.IsBad()==false)
			sourcetype=SOURCE_BIG;
		else
		{
			printf("Source file exists and is not a bigfile, or decryption key is incorrect!\n");
			return(0);
		}
	}
	else if(kGUI::IsDir(p2)==false)
	{
		fdata.time=kGUI::SysFileTime(p2);
		//fdata.root=p3;
		AddHash.Add(p2,&fdata);
		sourcetype=SOURCE_FILE;
	}
	else
	{
		unsigned int df;

		const char *name;
		kGUIDir dir;

		printf("loading directory!\n");
		dir.LoadDir(p2,true,true);
		for(df=0;df<dir.GetNumFiles();++df)
		{
			name=dir.GetFilename(df);
			fdata.time=kGUI::SysFileTime(name);
			AddHash.Add(name,&fdata);
		}
//		scandir(&AddHash,p2);
		sourcetype=SOURCE_DIR;
	}
	/* now, look for differences between bigfile and files in the addhash list */

	numok=0;
	numnew=0;
	numupdated=0;

	/* todo: optdelete function */

	/* add from source bigfile to dest bigfile will not work */
	/* if source is encrypted so I need to rewrite addfile to use a datahandle */
	/* instead of a filestream */

	if(sourcetype==SOURCE_BIG)
	{
		BigFileEntry *sfe;

		nums=SourceBigFile.GetNumEntries();
		for(i=0;((i<nums) && (g_userabort==false));++i)
		{
			sfe=(BigFileEntry *)SourceBigFile.GetEntry(i);
			bfe=DestBigFile.Locate(sfe->GetName()->GetString());
			if(!bfe)
			{
//				printf("File '%s' not in destination set!\n",she->m_string);
				update=true;
				++numnew;
			}
			else
			{
				if(sfe->m_time==bfe->m_time)
				{
					update=false;
					++numok;
				}
				else
				{
					int deltatime;

					deltatime=abs(sfe->m_time-bfe->m_time);
					if(deltatime==46400 || deltatime==3600)
					{
						update=false;
						++numok;
					}
					else
					{
						printf("File '%s' %d,%d times are different!(%d)\n",sfe->GetName()->GetString(),sfe->m_time,bfe->m_time,deltatime);
						++numupdated;
						update=true;
					}
				}
			}

			if(update==true)
			{
				SourceBigFile.CopyArea(&addhandle,sfe->m_offset,sfe->m_size,sfe->m_time);
//				addsize=sfe->m_size;
//				addtime=sfe->m_time;
//				fseek(addhandle,sfe->m_offset,SEEK_SET);

				/* add the file to the bigfile */
				DestBigFile.AddFile(sfe->GetName()->GetString(),&addhandle,false);
			}
		}
		DestBigFile.UpdateDir();
	}
	else
	{
		HashEntry *she;
		FDATA *sfdata;

		nums=AddHash.GetNum();
		she=AddHash.GetFirst();
		for(i=0;((i<nums) && (g_userabort==false));++i)
		{
			sfdata=(FDATA *)she->m_data;
			
			bfe=DestBigFile.Locate(she->m_string+tosspath);
			if(!bfe)
			{
//				printf("File '%s' not in destination set!\n",she->m_string+tosspath);
				++numnew;
				update=true;
			}
			else
			{
				if(sfdata->time==bfe->m_time)
				{
					update=false;
					++numok;
				}
				else
				{
					int deltatime;

					deltatime=(abs((int)sfdata->time-bfe->m_time));
					if(deltatime==46400 || deltatime==3600)
					{
						update=false;
						++numok;
					}
					else
					{
						printf("File '%s' %d,%d times are different!(%d)\n",she->m_string+tosspath,(int)sfdata->time,bfe->m_time,deltatime);
						++numupdated;
						update=true;
					}
				}
			}
			if(update==true)
			{
				addhandle.SetFilename(she->m_string);
				/* add the file to the bigfile */
				DestBigFile.AddFile(she->m_string+tosspath,&addhandle,false);
			}

			she=she->m_next;
		}
		DestBigFile.UpdateDir();
	}
	printf("numok=%d,numnew=%d,numupdated=%d\n",numok,numnew,numupdated);

	return 0;
}

/*******************************************************/
/** select functions duplicated from the kgui library **/
/*******************************************************/

long kGUISystemBig::FileTime(const char *fn)
{
#if defined(LINUX) || defined(MINGW) || defined(MACINTOSH)
	int result;
	struct stat buf;

	result=stat(fn,&buf);
	if(result==-1)
		return(0);
	return(buf.st_mtime);
#elif defined(WIN32)
   struct __stat64 buf;
   int result;

   result = _stat64( fn, &buf );

   /* Check if statistics are valid: */
   if( result != 0 )
		return(0);
   else
		return((long)buf.st_mtime);
#else
#error
#endif
}

unsigned char *kGUI::LoadFile(const char *filename,unsigned long *filesize)
{
	unsigned char *fm;
	unsigned long fs;
	DataHandle dh;

	if(filesize)
		filesize[0]=0;
	dh.SetFilename(filename);
	if(dh.Open()==false)
	{
		printf("Error: cannot open file '%s'\n",filename);
		return(0);	/* file not found or empty file */
	}

	fs=dh.GetLoadableSize();
	/* allocate space for file to load info */
	fm=new unsigned char[fs+1];
	if(!fm)
	{
		dh.Close();
		return(0);
	}
	fm[fs]=0;	/* allocate an extra byte and put a null at the end */
	dh.Read(fm,fs);
	dh.Close();
	if(filesize)
	{
		//todo: if filesize>32 bits, then assert, else return ok
		filesize[0]=fs;
	}
	return(fm);
}

unsigned char strcmpin(const char *lword,const char *sword,int n)
{
	int i;
	unsigned char delta;

	for(i=0;i<n;++i)
	{
		delta=lc(lword[i])-lc(sword[i]);
		if(delta)
			return(delta);
	}
	return(0);	/* match! */
}

char lc(char c)
{
	if(c>='A' && c<='Z')
		return((c-'A')+'a');
	return(c);
}

char *strstri(const char *lword,const char *sword)
{
	int i,j,p;
	int delta;
	int llen=(int)strlen(lword);
	int slen=(int)strlen(sword);

	p=(llen-slen)+1;	/* number of possible positions to compare */
	if(p<0)
		return(0);	/* can't match! as lword needs to >=len of sword */

	for(j=0;j<p;++j)
	{
		delta=0;
		for(i=0;((i<slen) && (!delta));++i)
			delta=lc(lword[j+i])-lc(sword[i]);
		if(!delta)
			return((char *)(lword+j));	/* match occured here! */

	}
	return(0);	/* no matches */
}

void fatalerror(const char *string)
{
	printf("%s",string);
	fflush(stdout);
	exit(1);
}
