/**********************************************************************************/
/* kGUI - zip.cpp                                                                 */
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
#include "zip.h"

#include "zlib/contrib/minizip/zip.h"
#include "zlib/contrib/minizip/zip.c"
#include "zlib/contrib/minizip/unzip.h"
#include "zlib/contrib/minizip/unzip.c"

voidpf ZCALLBACK fopen_file_func (voidpf opaque, const char *filename,int mode)
{
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
        mode_fopen = "rb";
    else
    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
        mode_fopen = "r+b";
    else
    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
        mode_fopen = "wb";

    if ((filename!=NULL) && (mode_fopen != NULL))
        file = fopen(filename, mode_fopen);
    return file;
}

uLong ZCALLBACK fread_file_func (voidpf opaque, voidpf stream, void *buf, uLong size)
{
    uLong ret;
    ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}


uLong ZCALLBACK fwrite_file_func (voidpf opaque, voidpf stream, const void *buf, uLong size)
{
    uLong ret;
    ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
    return ret;
}

long ZCALLBACK ftell_file_func (voidpf opaque, voidpf stream)
{
    long ret;
    ret = ftell((FILE *)stream);
    return ret;
}

long ZCALLBACK fseek_file_func (voidpf opaque, voidpf stream, uLong offset, int origin)
{
    int fseek_origin=0;
    long ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        fseek_origin = SEEK_CUR;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        fseek_origin = SEEK_END;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        fseek_origin = SEEK_SET;
        break;
    default: return -1;
    }
    ret = 0;
    fseek((FILE *)stream, offset, fseek_origin);
    return ret;
}

int ZCALLBACK fclose_file_func (voidpf opaque, voidpf stream)
{
    int ret;
    ret = fclose((FILE *)stream);
    return ret;
}

int ZCALLBACK ferror_file_func (voidpf opaque, voidpf stream)
{
    int ret;
    ret = ferror((FILE *)stream);
    return ret;
}

ZipFile::ZipFile()
{
	m_numentries=0;
	m_hash.Init(16,sizeof(ZipFileEntry *));
}

ZipFile::~ZipFile()
{

}

void fill_fopen_filefunc (zlib_filefunc_def *z)
{
}

#if defined(WIN32) || defined(MINGW)
uLong filetime(const char *f, tm_zip *tmzip, uLong *dt)
{
  int ret = 0;
  {
      FILETIME ftLocal;
      HANDLE hFind;
      WIN32_FIND_DATA  ff32;

      hFind = FindFirstFile(f,&ff32);
      if (hFind != INVALID_HANDLE_VALUE)
      {
        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
        FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
        FindClose(hFind);
        ret = 1;
      }
  }
  return ret;
}
#elif defined(LINUX) || defined(MACINTOSH)
uLong filetime(const char *f, tm_zip *tmzip, uLong *dt)
{
	struct tm *filedate;
	time_t tm_t=0;
	kGUIString name;

	name.SetString(f);
	if(!name.GetLen())
		return(0);

	/*remove trailing '/' if there was one */
	if(name.GetChar(name.GetLen()-1)=='/')
		name.Clip(name.GetLen()-1);

	tm_t=kGUI::FileTime(name.GetString());
	if(!tm_t)
		tm_t=time(NULL);	//file is probably a memory based file so set time to now!

	filedate = localtime(&tm_t);

	tmzip->tm_sec  = filedate->tm_sec;
	tmzip->tm_min  = filedate->tm_min;
	tmzip->tm_hour = filedate->tm_hour;
	tmzip->tm_mday = filedate->tm_mday;
	tmzip->tm_mon  = filedate->tm_mon ;
	tmzip->tm_year = filedate->tm_year;

	return 1;
}
#else
uLong filetime(const char *f, tm_zip *tmzip, uLong *dt)
{
    return 0;
}
#endif

void ZipFile::OpenSave(const char *fn,bool replace)
{
    zlib_filefunc_def ff;

    ff.zopen_file = fopen_file_func;
    ff.zread_file = fread_file_func;
    ff.zwrite_file = fwrite_file_func;
    ff.ztell_file = ftell_file_func;
    ff.zseek_file = fseek_file_func;
    ff.zclose_file = fclose_file_func;
    ff.zerror_file = ferror_file_func;
    ff.opaque = this;

	m_zf = zipOpen2(fn,replace==true?APPEND_STATUS_CREATE:APPEND_STATUS_ADDINZIP,NULL,&ff);
}

bool ZipFile::AddFile(const char *fn,DataHandle *af)
{
	int err;
	zip_fileinfo zi;
    unsigned long crcFile=0;
	unsigned char copybuf[4096];
	unsigned long asize=af->GetLoadableSize();
    int opt_compress_level=Z_DEFAULT_COMPRESSION;
	kGUIString fullfn;
	kGUIString shortfn;
	kGUIString path;

    zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
    zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
    zi.dosDate = 0;
    zi.internal_fa = 0;

    filetime(fn,&zi.tmz_date,&zi.dosDate);

//    if ((password != NULL) && (err==ZIP_OK))
//		err = getFileCrc(filenameinzip,buf,size_buf,&crcFile);

	fullfn.SetString(fn);

	kGUI::SplitFilename(&fullfn,&path,&shortfn);

	err = zipOpenNewFileInZip3(m_zf,shortfn.GetString(),&zi,
                        NULL,0,NULL,0,NULL /* comment*/,
                        (opt_compress_level != 0) ? Z_DEFLATED : 0,
                        opt_compress_level,0,
                        /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                        -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                        0,crcFile);

    if (err != ZIP_OK)
	{
        printf("error in opening %s in zipfile\n",fn);
		return(false);
	}

	if(af->Open()==false)
	{
        printf("error trying to read file\n");
		return(false);
	}

	while(asize>sizeof(copybuf))
	{
		af->Read(copybuf,(unsigned long)sizeof(copybuf));
        err = zipWriteInFileInZip (m_zf,copybuf,sizeof(copybuf));
		if (err != ZIP_OK)
		{
			printf("error writing to zipfile\n");
			af->Close();
			return(false);
		}
		asize-=sizeof(copybuf);
	};

	/* write remainder */
	if(asize>0)
	{
		af->Read(copybuf,asize);
        err = zipWriteInFileInZip (m_zf,copybuf,asize);
		if (err != ZIP_OK)
		{
			printf("error writing to zipfile\n");
			af->Close();
			return(false);
		}
	}
	af->Close();
    err = zipCloseFileInZip(m_zf);
	if (err != ZIP_OK)
	{
		printf("error writing to zipfile\n");
		return(false);
	}
	return(true);		/* success */
}

void ZipFile::CloseSave(void)
{
	zipClose(m_zf,NULL);
}

/* throw away path */
void ZipFile::LoadDirectory(void)
{
	unsigned int i;
	zlib_filefunc_def ff;
    unz_global_info gi;
    int err;
    unz_file_info file_info;
    char filename_inzip[256];
	ZipFileEntry *ze;

	m_numentries=0;

    ff.zopen_file = fopen_file_func;
    ff.zread_file = fread_file_func;
    ff.zwrite_file = fwrite_file_func;
    ff.ztell_file = ftell_file_func;
    ff.zseek_file = fseek_file_func;
    ff.zclose_file = fclose_file_func;
    ff.zerror_file = ferror_file_func;
    ff.opaque = this;
	m_zf=unzOpen2(m_zipfn.GetString(),&ff);
	if(!m_zf)
		return;

    err = unzGetGlobalInfo (m_zf,&gi);
    if (err!=UNZ_OK)
		return;

	m_entries.Init(gi.number_entry,-1);
	m_hash.Init(16,sizeof(ZipFileEntry *));

    for (i=0;i<gi.number_entry;i++)
    {
		if(i)
		{
			err = unzGoToNextFile(m_zf);
			if (err!=UNZ_OK)
				break;
		}

		err = unzGetCurrentFileInfo(m_zf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
        if (err!=UNZ_OK)
			break;

		ze=m_entries.GetEntryPtr(m_numentries++);
		/* todo: remove the path from the filename if it has one */
		ze->m_name.SetString(filename_inzip);
		m_hash.Add(filename_inzip,&ze);
	}
    //unzCloseCurrentFile(m_zf);
	unzClose(m_zf);
}

ContainerEntry *ZipFile::LocateEntry(const char *fn)
{
	ZipFileEntry **pzfe;

	pzfe=(ZipFileEntry **)m_hash.Find(fn);
	if(!pzfe)
		return 0;

	return(pzfe[0]);
}

void ZipFile::CopyEntry(ContainerEntry *cfe,DataHandle *dest)
{
	bool ok;

	dest->SetMemory();
	ok=Extract(dest,cfe->GetName()->GetString());
	assert(ok==true,"Error extracting file from zipfile!\n");
}

/* extract the file from inside the zipfile to the datahandle */
bool ZipFile::Extract(DataHandle *dest,const char *fn)
{
	int err;
	zlib_filefunc_def ff;
    unz_file_info file_info;
    char filename_inzip[256];
	unsigned char copybuf[4096];

    ff.zopen_file = fopen_file_func;
    ff.zread_file = fread_file_func;
    ff.zwrite_file = fwrite_file_func;
    ff.ztell_file = ftell_file_func;
    ff.zseek_file = fseek_file_func;
    ff.zclose_file = fclose_file_func;
    ff.zerror_file = ferror_file_func;
    ff.opaque = this;
	m_zf=unzOpen2(m_zipfn.GetString(),&ff);
	if(!m_zf)
		return(false);

	err=unzLocateFile(m_zf,fn,0);
	if(err==UNZ_OK)
    {
	    err = unzGetCurrentFileInfo(m_zf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if(err==UNZ_OK)
		{
			err=unzOpenCurrentFilePassword(m_zf,0);
			if(err==UNZ_OK)
			{
				/* read chunks until 0=done, <0=error,>0=numbytes */
				dest->OpenWrite("wb");
				do
	            {
					err = unzReadCurrentFile(m_zf,copybuf,sizeof(copybuf));
	                if (err<0)
						break;	//error!!
		            if (err>0)
					{
						/* number of bytes to write */
						dest->Write(copybuf,err);
					}
				}while(err>0);
				dest->Close();
			    unzCloseCurrentFile(m_zf);
			}
		}
	}
	unzClose(m_zf);
	return(err==UNZ_OK);
}
