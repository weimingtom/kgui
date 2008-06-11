/**********************************************************************************/
/* kGUI - kguiprot.cpp                                                            */
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
/* This is a simple encryption class, it works as follows:                        */
/* Encryption and Decryption needs 3 inputs, 1 - a keyfile, 2 a numeric offset    */
/* into  that file and 3 a numeric length.  The keyfile can be any file. For      */
/* example it could be a jpeg image, a pdf file, a text file, essentially anything*/
/* it also doesn't even have to reside on the hard drive but could live on a      */
/* plugged in USB drive                                                           */
/*                                                                                */
/* The encrypt/decrypt code then gets the size of the object being encoded or     */
/* decoded and uses that as an offset from the other two key offset values to     */
/* read from the keyfile and populate the random number generators seed.          */
/*                                                                                */
/* Then the encrypt/decrypt code reads from the random number generator and       */
/* mixes the values with the data.                                                */
/*                                                                                */
/* Also to note: this class can be attached to a bigfile so items in the bigfile  */
/* can be automagically be encrypted as they are added and decrypted as they are  */
/* loaded. All objects that use the DataHandle class like showing images or       */
/* the movie player can handle encrypted files automatically with this mechanism  */
/*                                                                                */
/* It is not as strong as true encryption but much faster and good enough.        */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"
#include "kguiprot.h"

kGUIProt::kGUIProt()
{
	m_keyfile=0;
}

kGUIProt::~kGUIProt()
{
	if(m_keyfile)
	{
		free((void *)m_keyfile);
		m_keyfile=0;
	}
}

void kGUIProt::SetMemKey(const char *buf,long keysize,unsigned int keystart,unsigned int keylen)
{
	if(m_keyfile)
		free((void *)m_keyfile);

	/* copy key to local array */
	m_keyfile=new char[keysize];
	memcpy((void *)m_keyfile,buf,keysize);

	m_keystart=keystart;
	m_keylen=keylen;
	m_keyfilesize=keysize;
}

bool kGUIProt::SetKey(const char *fn,unsigned int keystart,unsigned int keylen,bool load)
{
	m_keyfilename.SetString(fn);
	m_keystart=keystart;
	m_keylen=keylen;
	if(m_keyfile)
	{
		free((void *)m_keyfile);
		m_keyfile=0;
	}
	if(load)
	{
		m_keyfile=(const char *)kGUI::LoadFile(m_keyfilename.GetString(),&m_keyfilesize);
		if(!m_keyfile)
			return(false);
	}
	return(true);
}

bool kGUIProt::LoadSeed(kGUIRandom *r,unsigned int offset)
{
	unsigned int i;
	unsigned char keychar;
	long long keyfilesize;
	long long keystartindex=m_keystart+offset;
	DataHandle dh;

	r->InitSeed();

	if(!m_keyfile)
	{
		if(!m_keyfilename.GetLen())
			return(false);				/* no filename set */

		dh.SetFilename(m_keyfilename.GetString());
		if(dh.Open()==false)
			return(false);

		keyfilesize=dh.GetSize();
	}
	else
		keyfilesize=m_keyfilesize;

	/* if start index>filesize then bring it back inside */
	while(keystartindex>=keyfilesize)
		keystartindex-=keyfilesize;

	if(!m_keyfile)
		dh.Seek(keystartindex);

	for(i=0;i<m_keylen;++i)
	{
		/* read in bytes and set random key */
		if(!m_keyfile)
		{
			/* read 1 byte from the keyfile */
			dh.Read(&keychar,(unsigned long)1);
		}
		else
			keychar=m_keyfile[keystartindex];

		r->AddEntropy(keychar);
		if(++keystartindex==keyfilesize)
		{
			keystartindex=0;
			if(!m_keyfile)
				dh.Seek((unsigned long long)0);
		}
	}
	if(!m_keyfile)
		dh.Close();
	return(true);
}

static char phex[]={"0123456789abcdef"};

const char *kGUIProt::EncryptToHex(const char *in,int insize, int *outlen)
{
	int i,ol;
	char c;
	char *outbuffer;
	char *ob;
	kGUIRandom r;

	LoadSeed(&r,insize);

	ol=insize<<1;
	if(outlen)
		*(outlen)=ol;
	outbuffer=new char[ol+1];	/* add a null to the end */
	ob=outbuffer;

	/* encrypt the source data to output data */
	for(i=0;i<insize;++i)
	{
		r.ExtractEntropy(&c,sizeof(c));
		c+=in[i];
		ob[0]=phex[(c>>4)&15];
		ob[1]=phex[c&15];
		ob+=2;
	}
	ob[0]=0;	/* put a null at the end */
	return(outbuffer);
}

const char *kGUIProt::DecryptFromHex(const char *in,int insize, int *outlen)
{
	int i,ol;
	char c;
	char c2;
	char cs[2];
	const char *hp;
	char *outbuffer;
	char *ob;
	kGUIRandom r;

	ol=insize>>1;
	LoadSeed(&r,ol);

	if(outlen)
		*(outlen)=ol;
	outbuffer=new char[ol+1];	/* add a null to the end */
	ob=outbuffer;

	cs[1]=0;
	/* decrypt the source data to output data */
	for(i=0;i<ol;++i)
	{
		r.ExtractEntropy(&c2,sizeof(c2));
		cs[0]=in[0];
		hp=strstr(phex,cs);
		assert(hp!=0,"invalid character in encrypted string!");
		c=(char)((hp-phex)<<4)&255;
		cs[0]=in[1];
		hp=strstr(phex,cs);
		assert(hp!=0,"invalid character in encrypted string!");
		c|=(hp-phex)&255;
		in+=2;
		*(ob++)=(c-c2)&255;
	}
	ob[0]=0;	/* put a null at the end */
	return(outbuffer);
}

/* initially this called the random number generator for every byte in the */
/* file being encrypted / decrypted. This was pretty slow on very large files so... */
/* now it uses 2 smaller buffers (EMAXBUFFER/PMAXBUFFER) and inter mixes them */
/* so it has a lot less calls to the random number generator */

#define EMAXBUFFER 8192
#define PMAXBUFFER 1024

const unsigned char *kGUIProt::Encrypt(const unsigned char *in,long insize)
{
	long i;
	unsigned char c;
	unsigned char *outbuffer;
	unsigned char *ob;
	int ecount,epass,eindex;
	unsigned char ebuffer[EMAXBUFFER];
	unsigned char pbuffer[PMAXBUFFER];
	kGUIRandom r;

	LoadSeed(&r,insize);

	outbuffer=new unsigned char[insize];
	ob=outbuffer;

	/* use 8k buffer because fully encrypting every byte is way too slow! */

	for(i=0;i<EMAXBUFFER;++i)
	{
		r.ExtractEntropy((char *)&c,sizeof(c));
		ebuffer[i]=c;
	}
	/* page buffer, eor values for each page */
	for(i=0;i<PMAXBUFFER;++i)
	{
		r.ExtractEntropy((char *)&c,sizeof(c));
		pbuffer[i]=c;
	}

	/* encrypt the source data to output data */
	eindex=0;
	ecount=0;
	epass=0;
	for(i=0;i<insize;++i)
	{
		c=ebuffer[eindex]^pbuffer[epass];
		c+=in[i];
		*(ob++)=c;
		eindex+=in[i]+1;
		if(eindex>=EMAXBUFFER)
			eindex-=EMAXBUFFER;
		if(++ecount==EMAXBUFFER)
		{	
			ecount=0;
			if(++epass==PMAXBUFFER)
				epass=0;
		}
	}
	return(outbuffer);
}

const unsigned char *kGUIProt::Decrypt(const unsigned char *in,long insize)
{
	long i;
	unsigned char c;
	unsigned char *outbuffer;
	unsigned char *ob;
	int epass,eindex,ecount;
	unsigned char ebuffer[EMAXBUFFER];
	unsigned char pbuffer[PMAXBUFFER];
	kGUIRandom r;

	LoadSeed(&r,insize);

	outbuffer=new unsigned char[insize];
	ob=outbuffer;

	for(i=0;i<EMAXBUFFER;++i)
	{
		r.ExtractEntropy((char *)&c,sizeof(c));
		ebuffer[i]=c;
	}
	/* page buffer, eor values for each page */
	for(i=0;i<PMAXBUFFER;++i)
	{
		r.ExtractEntropy((char *)&c,sizeof(c));
		pbuffer[i]=c;
	}

	/* decrypt the source data to output data */
	ecount=0;
	epass=0;
	eindex=0;
	for(i=0;i<insize;++i)
	{
		c=(in[i]-(ebuffer[eindex]^pbuffer[epass]))&255;
		*(ob++)=c;
		eindex+=c+1;
		if(eindex>=EMAXBUFFER)
			eindex-=EMAXBUFFER;
		if(++ecount==EMAXBUFFER)
		{	
			ecount=0;
			if(++epass==PMAXBUFFER)
				epass=0;
		}
	}
	return(outbuffer);
}
