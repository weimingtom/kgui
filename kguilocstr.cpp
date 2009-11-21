/**********************************************************************************/
/* kGUI - kguilocstr.cpp                                                            */
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

/*! @file kguilocstr.cpp 
    @brief This is the stand along console app for parsing localized strings
	it generates a .h and a .cpp file that contains enums and data for the text.
	The strings are encoded in UTF-8 format. Use the kGUILoc class. */

#include "kgui.h"
#include "kguicsv.h"


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

bool optrecursive;
bool optverify;
bool optcompress;
bool optdelete;
bool optmissing;

class kGUISystemLoc : public kGUISystem
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
	bool IsDir(const char *fn) {return false;}
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

bool g_userabort=false;

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

long MS_FileTime(const char *fn);


int main(int argc, char* argv[])
{
	int i;
	const char *iname=0;
	const char *hname=0;
	const char *cname=0;
	kGUICSV csv;
	DataHandle outh;
	DataHandle outc;
	kGUISystemLoc sysloc;
	const char defprefix[]={"TEXT_"};
	const char *prefix=defprefix;
	kGUIString s;
	int row,col,numrows,numcols;
	Array<int>offsets;
	unsigned int b;
	unsigned int index;
	unsigned int byte;
	int offset;

	kGUI::SetSystem(&sysloc);
	signal(SIGINT, sigint_handler);

	for(i=1;i<argc;++i)
	{
		if(argv[i][0]=='-' && ((i+1)<argc))
		{
			switch(argv[i][1])
			{
			case 'i':
				iname=argv[++i];
			break;
			case 'c':
				cname=argv[++i];
			break;
			case 'h':
				hname=argv[++i];
			break;
			case 'p':
				prefix=argv[++i];
			break;
			default:
				printf("Unknown parm '%s'\n",argv[i]);
				return(1);
			break;
			}
		}
		else
		{
			printf("Unknown parm '%s'\n",argv[i]);
			return(1);
		}
	}

	/* need at least 1 parm */
	if(!iname || !hname || !cname)
	{
		printf("kguilocstr: (c) 2008 Kevin Pickelll\n");
		printf("usage: kguilocstr -i input.txt -p TEXT_ -h outfile.h -c outfile.cpp\n");
		printf(" -i = inputfile\n");
		printf(" -p = prefix\n");
		printf(" -h = outputheader\n");
		printf(" -c = outputcpp\n");
		return(1);
	}

	/* if output files exists already and are newer than the input file then just return */
	/* just return as it is already up to date */
	{
		long intime;
		long outtime1;
		long outtime2;
		
		intime=MS_FileTime(iname);
		outtime1=MS_FileTime(hname);
		outtime2=MS_FileTime(cname);

		/* zero = file doesn't exist */
		if(outtime1>=intime && outtime2>=intime)
		{
			printf("kguilocstr: output files are newer so no need to re-generate them!\n");
			return(0);
		}
	}

	/* put code here */
	csv.SetSplit("\t");		/* split on TABS not commas */
	csv.SetFilename(iname);
	csv.Load();
	numrows=csv.GetNumRows();
	numcols=csv.GetNumCols();

	outh.SetFilename(hname);
	if(outh.OpenWrite("wb")==false)
	{
		printf("kguilocstr: Error opening output file '%s'\n",hname);
		return(1);
	}
	outh.Sprintf("#ifndef __%s__\n#define __%s__\n\nenum{\n",prefix,prefix);
	for(row=1;row<numrows;++row)
	{
		csv.GetField(row,0,&s);
		s.ChangeEncoding(ENCODING_UTF8);
		if(row>1)
			outh.Sprintf(",\n");
		outh.Sprintf("%s%s",prefix,s.GetString());
	}
	outh.Sprintf("};\n#endif\n",prefix);
	if(outh.Close()==false)
	{
		printf("kguilocstr: Error writing output file '%s'\n",hname);
		return(1);
	}

	outc.SetFilename(cname);
	if(outc.OpenWrite("wb")==false)
	{
		printf("kguilocstr: Error opening output file '%s'\n",cname);
		return(1);
	}
	
	/* generate text */
	offsets.Alloc((numrows-1)*(numcols-1));

	outc.Sprintf("static const unsigned char %sCHARS[]={\n",prefix);
	offset=0;
	index=0;
	for(col=1;col<numcols;++col)
	{
		for(row=1;row<numrows;++row)
		{
			csv.GetField(row,col,&s);
			s.ChangeEncoding(ENCODING_UTF8);

			offsets.SetEntry(index++,offset);
			for(b=0;b<s.GetLen()+1;++b)
			{
				if(offset)
					outc.Sprintf(",");
				if((++offset&15)==0)
					outc.Sprintf("\n");
				byte=((unsigned int)s.GetChar(b))&255;
				outc.Sprintf("%d",byte);
			}
		}
	}
	outc.Sprintf("};\n\n");

	/* output string pointers */
	outc.Sprintf("static const unsigned char *%sPOINTERS[]={\n",prefix);
	for(b=0;b<index;++b)
	{
		offset=offsets.GetEntry(b);
		outc.Sprintf("%sCHARS+%d",prefix,offset);
		if((b+1)<index)
			outc.Sprintf(",");
		if(((b+1)&3)==0)
			outc.Sprintf("\n");
	}
	outc.Sprintf("};\n\n");

	outc.Sprintf("LOCSTRING_DEF %sDEF={%d,%d,%sPOINTERS};\n\n",prefix,numcols-1,numrows-1,prefix);
	
	if(outc.Close()==false)
	{
		printf("kguilocstr: Error writing output file '%s'\n",cname);
		return(1);
	}

	printf("kguilocstr: numlangs=%d,numstrings=%d\n",csv.GetNumCols()-1,csv.GetNumRows()-1);

	return 0;
}

/*******************************************************/
/** select functions duplicated from the kgui library **/
/*******************************************************/

long kGUISystemLoc::FileTime(const char *fn)
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

long MS_FileTime(const char *fn)
{
#if defined(LINUX) || defined(MINGW) || defined(MACINTOSH)
	int result;
	struct stat buf;

	result=stat(fn,&buf);
	if(result==-1)
		return(0);
	return(buf.st_mtime);
#else
   struct __stat64 buf;
   int result;

   result = _stat64( fn, &buf );

   /* Check if statistics are valid: */
   if( result != 0 )
		return(0);
   else
		return((long)buf.st_mtime);
#endif
}

void fatalerror(const char *string)
{
	printf("%s",string);
	fflush(stdout);
	exit(1);
}
