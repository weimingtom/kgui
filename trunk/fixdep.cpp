// bintoc.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <sys/stat.h>

char *loadfilez(char *filename,int *filesize)
{
	FILE *fp;
	char *filemem;
	int count,fs;
	long /*fpos_t*/ fsize;

	if(filesize)
		*(filesize)=0;	/* if there is any errors, then filesize=0 */
	
	fp=fopen(filename,"rb");
	
	if(!fp)			/* error! */
	{
		printf("Error: (OPEN) Unable to open input file '%s'\n",filename);
		return(0);
	}
	if (fseek (fp,0l,SEEK_END))
	{
		printf("Error: (SEEK) Unable to open input file '%s'",filename);
		return (0);
	}
	fsize=ftell(fp);
//if (fgetpozs (fp,&fsize))	
//	{
//		//printf("Error: (GETPOS) Unable to open input file '%s'",filename);
//		return (0);
//	}
	fs=(int)fsize;
	filemem=(char *)malloc(fs);	/* let's allocate the ram to load the file in! */
	if(!filemem)
	{
		printf("Error: (ALLOC) Unable allocate memory '%d'",fs);
		fclose(fp);
		return(0);		/* not enough ram to load the file in! */
	}
	if (fseek (fp,0l,SEEK_SET))
	{
		printf("Error: (SEEK) Unable to open input file '%s'",filename);
		return (0);
	}
	count = (int)fread (filemem,1,fs,fp);	/* read in the file */
	fclose(fp);
	if(fs!=count)
	{
		free(filemem);
		printf("Error: (COUNT MISMATCH) Unable to open input file '%s',%d!=%d",filename,count,fs);
		return(0);
	}		

	/* everything was ok! */
	if(filesize)
		*(filesize)=fs;
	return(filemem);
}

int main(int argc, char *argv[])
{
	char *filename;
	char *objdir;
	FILE *fout;
	int l;
	int sl;
	int el;
	int found;

	char *indata;
	int insize;

	if(argc<3)
	{	
		printf("usage: bintoc name objdir");
		return(0);
	}
	filename=argv[1];
	objdir=argv[2];

	indata=loadfilez(filename,&insize);
	if(!indata)
	{
		printf("Error opening file '%s'\n",filename);
		return(0);
	}

	fout=fopen(filename,"wb");
	if(!fout)
	{
		printf("Error opening output '%s'\n",filename);
		return(0);
	}

	/* write it back out and prefix each output filename with objdir/ */
	sl=0;
	do
	{
		el=sl+1;
		while(el<insize)
		{
			if(indata[el++]==0x0a)
				break;
		}
		/* ok, sl=start line index, el=end line index */
		found=0;
		for(l=sl;l<(el-2);++l)
		{
			if(!strncmp(indata+l,".o",2))
				found=1;
		}
		if(found)
			fprintf(fout,"%s",objdir);
		while(sl<el)
			fprintf(fout,"%c",indata[sl++]);
	}while(sl<insize);
	fclose(fout);
	free(indata);
	return 0;
}

