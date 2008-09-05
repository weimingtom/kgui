/**********************************************************************************/
/* kGUI - kguixml.cpp                                                             */
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

/*! @file kguixml.cpp
    @brief This is a simple XML class, it handles reading and writing XML files.
	It is NOT a full implementation by any means just a simple reader / writer
    It also handles encoding/decoding for special &amp;xxx; characters. */

#include "kgui.h"
#include "kguixml.h"
#include "hash.h"

unsigned int kGUIXMLCODES::m_longest=0;
unsigned int kGUIXMLCODES::m_maxcode=0;
XCODE_DEF *kGUIXMLCODES::m_xcodes=0;
Hash *kGUIXMLCODES::m_longhash=0;
kGUIString *kGUIXMLCODES::m_utflist=0;

/*! @internal @struct CODES_DEF
    @brief Internal struct used by the kGUIXMLCODES class.
	This table contains the ampersand encoded strings and their equivalents. */

typedef struct
{
	int close;
	const char *longcode;
	int shortcode;
}CODES_DEF;

static CODES_DEF hcodes[] = {
	{0,"amp",'&'},
	{0,"lt",'<'},
	{0,"gt",'>'},
	{0,"quot",'"'},

	/* only go one way! */
	{1,"iexcl",'¡'},
	{1,"cent",'¢'},
	{1,"pound",'£'},
	{1,"curren",'¤'},
	{1,"yen",'¥'},
	{1,"brvbar",'¦'},
	{1,"sect",'§'},
	{1,"uml",'¨'},
	{1,"copy",'©'},
	{1,"ordf",'ª'},
	{1,"laquo",'«'},
	{1,"not",'¬'},
	{1,"shy",'­'},
	{1,"macr",'¯'},
	{1,"plusmn",'±'},
	{1,"sup2",'²'},
	{1,"sup3",'³'},
	{1,"acute",'´'},
	{1,"micro",'µ'},
	{1,"para",'¶'},
	{1,"middot",'·'},
	{1,"cedil",'¸'},
	{1,"sup1",'¹'},
	{1,"ordm",'º'},
	{1,"raquo",'»'},
	{1,"frac14",'¼'},
	{1,"frac12",'½'},
	{1,"frac34",'¾'},
	{1,"iquest",'¿'},
	{1,"Agrave",'À'},
	{1,"Aacute",'Á'},
	{1,"Acirc",'Â'},
	{1,"Atilde",'Ã'},
	{1,"Auml",'Ä'},
	{1,"Aring",'Å'},
	{1,"AElig",'Æ'},
	{1,"Ccedil",'Ç'},
	{1,"Egrave",'È'},
	{1,"Eacute",'É'},
	{1,"Ecirc",'Ê'},
	{1,"Euml",'Ë'},
	{1,"Igrave",'Ì'},
	{1,"Iacute",'Í'},
	{1,"Icirc",'Î'},
	{1,"Iuml",'Ï'},
	{1,"ETH",'Ð'},
	{1,"Ntilde",'Ñ'},
	{1,"Ograve",'Ò'},
	{1,"Oacute",'Ó'},
	{1,"Ocirc",'Ô'},
	{1,"Otilde",'Õ'},
	{1,"Ouml",'Ö'},
	{1,"times",'×'},
	{1,"Oslash",'Ø'},
	{1,"Ugrave",'Ù'},
	{1,"Uacute",'Ú'},
	{1,"Ucirc",'Û'},
	{1,"Uuml",'Ü'},
	{1,"Yacute",'Ý'},
	{1,"THORN",'Þ'},
	{1,"szlig",'ß'},
	{1,"agrave",'à'},
	{1,"aacute",'á'},
	{1,"acirc",'â'},
	{1,"atilde",'ã'},
	{1,"auml",'ä'},
	{1,"aring",'å'},
	{1,"aelig",'æ'},
	{1,"ccedil",'ç'},
	{1,"egrave",'è'},
	{1,"eacute",'é'},
	{1,"ecirc",'ê'},
	{1,"euml",'ë'},
	{1,"igrave",'ì'},
	{1,"iacute",'í'},
	{1,"icirc",'î'},
	{1,"iuml",'ï'},
	{1,"eth",'ð'},
	{1,"ntilde",'ñ'},
	{1,"ograve",'ò'},
	{1,"oacute",'ó'},
	{1,"ocirc",'ô'},
	{1,"otilde",'õ'},
	{1,"ouml",'ö'},
	{1,"divide",'÷'},
	{1,"oslash",'ø'},
	{1,"ugrave",'ù'},
	{1,"uacute",'ú'},
	{1,"ucirc",'û'},
	{1,"uuml",'ü'},
	{1,"yacute",'ý'},
	{1,"thorn",'þ'},
	{1,"yuml",'ÿ'},
	{1,"Yuml",'ÿ'},
	{1,"circ",'^'},
	{1,"tilde",'~'},
	{1,"lsquo",'‘'},
	{1,"rsquo",'’'},
	{1,"bdquo",'„'},
	{1,"dagger",8224},
	{1,"Dagger",8225},
	{1,"permil",'‰'},
	{1,"lsaquo",'‹'},
	{1,"rsaquo",'›'},
	{1,"reg",'®'},
	{1,"deg",'°'},
	{1,"euro",'€'},
	{1,"apos",'\''},
	{1,"nbsp",' '},
	{1,"sbquo",','},
	{1,"ndash",'-'},
	{1,"mdash",'-'},
	{1,"ldquo",'\"'},
	{1,"rdquo",'\"'},
	{1,"OElig",'E'},	
	{1,"oelig",'e'},
	{1,"Scaron",'S'},
	{1,"scaron",'s'},
	{1,"fnof",'f'},

	{1,"Alpha",  	913},  
	{1,"Beta",  	914},  
	{1,"Gamma",  	915},  
	{1,"Delta",  	916},  
	{1,"Epsilon",  	917},  
	{1,"Zeta",  	918},  
	{1,"Eta",  	919},  
	{1,"Theta",  	920},  
	{1,"Iota",  	921},  
	{1,"Kappa",  	922},  
	{1,"Lambda",  	923},  
	{1,"Mu",  	924},  
	{1,"Nu",  	925},  
	{1,"Xi",  	926},  
	{1,"Omicron",  	927},  
	{1,"Pi",  	928},  
	{1,"Rho",  	929},  
	{1,"Sigma",  	931},  
	{1,"Tau",  	932},  
	{1,"Upsilon",  	933},  
	{1,"Phi",  	934},  
	{1,"Chi",  	935},  
	{1,"Psi",  	936},  
	{1,"Omega",  	937},  
	{1,"alpha",  	945},  
	{1,"beta",  	946},  
	{1,"gamma",  	947},  
	{1,"delta",  	948},  
	{1,"epsilon",  	949},  
	{1,"zeta",  	950},  
	{1,"eta",  	951},  
	{1,"theta",  	952},  
	{1,"iota",  	953},  
	{1,"kappa",  	954},  
	{1,"lambda",  	955},  
	{1,"mu",  	956},  
	{1,"nu",  	957},  
	{1,"xi",  	958},  
	{1,"omicron",  	959},  
	{1,"pi",  	960},  
	{1,"rho",  	961},  
	{1,"sigmaf",  	962},  
	{1,"sigma",  	963},  
	{1,"tau",  	964},  
	{1,"upsilon",  	965},  
	{1,"phi",  	966},  
	{1,"chi",  	967},  
	{1,"psi",  	968},  
	{1,"omega",  	969},  
	{1,"thetasym",  	977},  
	{1,"upsih",  	978},  
	{1,"piv",  	982},  
	{1,"bull",  	8226},  
	{1,"hellip",  	8230},  
	{1,"prime",  	8242},  
	{1,"Prime",  	8243},  
	{1,"oline",  	8254},  
	{1,"frasl",  	8260},  
	{1,"weierp",  	8472},  
	{1,"image",  	8465},  
	{1,"real",  	8476},  
	{1,"trade",  	8482},  
	{1,"alefsym",  	8501},  
	{1,"larr",  	8592},  
	{1,"uarr",  	8593},  
	{1,"rarr",  	8594},  
	{1,"darr",  	8595},  
	{1,"harr",  	8596},  
	{1,"crarr",  	8629},  
	{1,"lArr",  	8656},  
	{1,"uArr",  	8657},  
	{1,"rArr",  	8658},  
	{1,"dArr",  	8659},  
	{1,"hArr",  	8660},  
	{1,"forall",  	8704},  
	{1,"part",  	8706},  
	{1,"exist",  	8707},  
	{1,"empty",  	8709},  
	{1,"nabla",  	8711},  
	{1,"isin",  	8712},  
	{1,"notin",  	8713},  
	{1,"ni",  	8715},  
	{1,"prod",  	8719},  
	{1,"sum",  	8721},  
	{1,"minus",  	8722},  
	{1,"lowast",  	8727},  
	{1,"radic",  	8730},  
	{1,"prop",  	8733},  
	{1,"infin",  	8734},  
	{1,"ang",  	8736},  
	{1,"and",  	8743},  
	{1,"or",  	8744},  
	{1,"cap",  	8745},  
	{1,"cup",  	8746},  
	{1,"int",  	8747},  
	{1,"there4",  	8756},  
	{1,"sim",  	8764},  
	{1,"cong",  	8773},  
	{1,"asymp",  	8776},  
	{1,"ne",  	8800},  
	{1,"equiv",  	8801},  
	{1,"le",  	8804},  
	{1,"ge",  	8805},  
	{1,"sub",  	8834},  
	{1,"sup",  	8835},  
	{1,"nsub",  	8836},  
	{1,"sube",  	8838},  
	{1,"supe",  	8839},  
	{1,"oplus",  	8853},  
	{1,"otimes",  	8855},  
	{1,"perp",  	8869},  
	{1,"sdot",  	8901},  
	{1,"lceil",  	8968},  
	{1,"rceil",  	8969},  
	{1,"lfloor",  	8970},  
	{1,"rfloor",  	8971},  
	{1,"lang",  	9001},  
	{1,"rang",  	9002},  
	{1,"loz",  	9674},  
	{1,"spades",  	9824},  
	{1,"clubs",  	9827},  
	{1,"hearts",  	9829},  
	{1,"diams",  	9830},  
};

void kGUIXMLCODES::Init(void)
{
	unsigned int i;
	int c;
	unsigned int l;

	/* count largest shortcode index */
	m_maxcode=0;
	for(i=0;i<sizeof(hcodes)/sizeof(CODES_DEF);++i)
	{
		c=hcodes[i].shortcode;
		if(c<0)
			c&=0xff;
		++c;
		if((unsigned int)c>m_maxcode)
			m_maxcode=c;
	}

	m_utflist=new kGUIString[m_maxcode];
	m_xcodes=new XCODE_DEF[m_maxcode];
	m_longest=0;
	for(i=0;i<m_maxcode;++i)
	{
		m_xcodes[i].longcode=0;
		m_xcodes[i].longlen=0;

		/* generate UTF-8 versions of all 1 byte strings */
		m_utflist[i].SetEncoding(ENCODING_UTF8);
		m_utflist[i].Clear();
		m_utflist[i].Append(i);
	}

	m_longhash=new Hash();
	m_longhash->Init(8,sizeof(int));
	m_longhash->SetCaseSensitive(true);

	for(i=0;i<sizeof(hcodes)/sizeof(CODES_DEF);++i)
	{
		c=hcodes[i].shortcode;
		if(c<0)
			c&=0xff;

		assert(m_longhash->Find(hcodes[i].longcode)==0,"Duplicate code error!");

		m_longhash->Add(hcodes[i].longcode,&c);

		l=(unsigned int)strlen(hcodes[i].longcode);
		if(l>m_longest)
			m_longest=l;

		if(hcodes[i].close==0)
		{
			m_xcodes[c].longcode=hcodes[i].longcode;
			m_xcodes[c].longlen=(int)strlen(hcodes[i].longcode)+1;
		}
	}
}

void kGUIXMLCODES::Purge(void)
{
	if(m_xcodes)
		delete []m_xcodes;
	if(m_utflist)
		delete []m_utflist;
	if(m_longhash)
		delete m_longhash;
}

/* code assumed that shrinked string will never be longer than the source string */
/* except: in the case detailed below */

/* hmmm, I've come across an interesting issue, */
/* when shrinking a string that is in 8bit and it has an encoded char >8 bit then */
/* we need to convert from 8 to UTF-8 and in that case the destination string can be */
/* larger than the source string. */
/* so, if the encoding is changed from 8bit to UTF-8 then it allocates a 2x size for output */
/* just to be on the safe side */

void kGUIXMLCODES::Shrink(kGUIString *from,kGUIString *to)
{
	unsigned int j;
	int r;
	unsigned char c;
	unsigned char *cp;
	unsigned char *dp;
	unsigned char *cp2;
	unsigned int *xcp;
	char code[16];
	unsigned int encoding;
	kGUIString *sp;

	assert(m_longhash!=0,"kGUIXMLCODES::Init() - not called!\n");

	/* make sure to is big enough */
	to->Alloc(from->GetLen());
	/* dest encoding is same as source encoding */
	encoding=from->GetEncoding();
	to->SetEncoding(encoding);

	cp=(unsigned char *)from->GetString();
	dp=(unsigned char *)to->GetString();

	while((c=*(cp++)))
	{
		*(dp++)=c;
		if(c=='&')
		{
			/* grab characters until ';' */
			j=0;
			cp2=cp;
			do
			{
				c=*(cp2++);
				if(!c)
					goto next;		/* hit end of string */
				if(c==';')
					break;
				else
					code[j++]=c;
				if(j>m_longest || j==sizeof(code))
					goto next;	/* too long, longer than all codes in the list */
			}while(1);

			code[j]=0;
			xcp=(unsigned int *)m_longhash->Find(code);
			if(xcp)
			{
				r=((int)strlen(code)+2)-1;		/* number of characters to skip */
				cp+=r;
				j=*(xcp);
			}
			else if(code[0]=='#')
			{
				r=((int)strlen(code)+2)-1;		/* number of characters to skip */
				cp+=r;

				/* hex or decimal? */
				if(code[1]=='x' || code[1]=='X')
				{
					char *hcp=code+2;
					char hc;

					j=0;
					while(hcp[0])
					{
						hc=*(hcp++);					
						j=j<<4;
						if(hc>='0' && hc<='9')
							j|=(hc-'0');
						else if(hc>='a' && hc<='f')
							j|=(10+(hc-'a'));
						else if(hc>='A' && hc<='F')
							j|=(10+(hc-'A'));
						else
							break;	/* not a hex character */
					}
				}
				else
					j=atoi(code+1);
			}
			else
				j=0;

			if(j)
			{
				/* check for invalid character using current encoding */
				if(j>255 && encoding==ENCODING_8BIT)
				{
					/* not valid in 8 bit encoding so we must change the string to UTF-8 */
					*(dp)=0;
					to->RecalcLen();
					to->ChangeEncoding(ENCODING_UTF8);
					/* if the source string contained a lot of chars>127 then it could */
					/* grow to twice it's original size when converted to UTF-8 */
					to->Alloc(from->GetLen()<<1);
					encoding=ENCODING_UTF8;

					/* since string could have been moved or made longer we need to recalc */
					dp=(unsigned char *)(to->GetString()+to->GetLen());
				}

				switch(encoding)
				{
				case ENCODING_8BIT:
					*(dp-1)=j;
				break;
				case ENCODING_UTF8:
					if(j<m_maxcode)
					{
						sp=m_utflist+j;
						memcpy(dp-1,sp->GetString(),sp->GetLen());
						dp+=sp->GetLen()-1;
					}
					else
					{
						/* make it on the fly */
						kGUIString utf;

						utf.SetEncoding(ENCODING_UTF8);
						utf.Append(j);

						memcpy(dp-1,utf.GetString(),utf.GetLen());
						dp+=utf.GetLen()-1;
					}
				break;
				default:
					assert(false,"Unimplemented string format!");
				break;
				}
			}
			else
			{
				/* unknown command */
				kGUI::Trace("Unknown XML command '%s'\n",code);
			}
		}
next:;
	}
	*(dp)=0;
	to->RecalcLen();
}

void kGUIXMLCODES::Expand(kGUIString *from,kGUIString *to)
{
	unsigned int l;
	unsigned int newl;
	unsigned char *dp;
	unsigned char *cp;
	unsigned char c;

	assert(m_longhash!=0,"kGUIXMLCODES::Init() - not called!\n");

	to->SetEncoding(from->GetEncoding());

	l=from->GetLen();
	newl=l;
	cp=(unsigned char *)from->GetString();
	while((c=*(cp++)))
	{
		/* todo, handle UTF-8 encoding and chars >255 */

		newl+=m_xcodes[c].longlen;	/* 'a' -> '&xxx;' (longlen=len-1) */
	}
	if(l==newl)
		to->SetString(from->GetString());
	else
	{
		to->Alloc(newl);	/* make sure there is enough space for expanded length version */

		cp=(unsigned char *)from->GetString();
		dp=(unsigned char *)to->GetString();
		while((c=*(cp++)))
		{
			if((l=m_xcodes[c].longlen))	/* number of characters to expand */
			{
				*(dp++)='&';
				memcpy(dp,m_xcodes[c].longcode,l-1);
				dp+=l-1;
				*(dp++)=';';
			}
			else
				*(dp++)=c;
		}
		*(dp)=0;
		to->RecalcLen();
		assert(to->GetLen()==newl,"Internal error!");
	}
}

kGUIXMLItem::~kGUIXMLItem()
{
	unsigned int i;
	kGUIXMLItem *child;

//	++numfxx;
	for(i=0;i<m_numchildren;++i)
	{
		child=m_children.GetEntry(i);
		delete child;
	}
}

void kGUIXMLItem::AddParm(const char *name,const char *value)
{
	kGUIXMLItem *parm=new kGUIXMLItem();

	//check name for illegal characters
	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	parm->m_parm=true;
	parm->m_name=name;
	if(value)
		parm->m_value.SetString(value);
	AddChild(parm);
}

void kGUIXMLItem::AddParm(const char *name,kGUIString *value)
{
	kGUIXMLItem *parm=new kGUIXMLItem();

	//check name for illegal characters
	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	parm->m_parm=true;
	parm->m_name=name;
	parm->m_value.SetString(value);
	AddChild(parm);
}


void kGUIXMLItem::DelChild(kGUIXMLItem *child,bool purge)
{
	unsigned int i;

	i=m_numchildren;
	while(i)
	{
		--i;
		if(m_children.GetEntry(i)==child)
		{
			if(purge)
				delete child;
			m_children.DeleteEntry(i);
			--m_numchildren;
			return;
		}
	}
	assert(false,"Child not found!");
}

kGUIXMLItem *kGUIXMLItem::AddChild(const char *name,kGUIString *value)
{
	kGUIXMLItem *child=new kGUIXMLItem();

	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	child->m_name=name;
	child->m_value.SetString(value);
	AddChild(child);
	return(child);
}

kGUIXMLItem *kGUIXMLItem::AddChild(const char *name,const char *value)
{
	kGUIXMLItem *child=new kGUIXMLItem();

	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	child->m_name=name;
	if(value)
		child->m_value.SetString(value);
	AddChild(child);
	return(child);
}

kGUIXMLItem *kGUIXMLItem::AddChild(const char *name,int value)
{
	kGUIString vs;

	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	kGUIXMLItem *child=new kGUIXMLItem();
	child->m_name=name;
	vs.Sprintf("%d",value);
	child->m_value.SetString(vs.GetString());
	AddChild(child);
	return(child);
}

kGUIXMLItem *kGUIXMLItem::AddChild(const char *name,unsigned int value)
{
	kGUIString vs;

	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	kGUIXMLItem *child=new kGUIXMLItem();
	child->m_name=name;
	vs.Sprintf("%d",value);
	child->m_value.SetString(vs.GetString());
	AddChild(child);
	return(child);
}

kGUIXMLItem *kGUIXMLItem::AddChild(const char *name,double value)
{
	kGUIString vs;

	assert(strstr(name," ")==0 && strstr(name,"&")==0 && strstr(name,"\"")==0 && strstr(name,"<")==0 && strstr(name,">")==0,"Illegal character in name");

	kGUIXMLItem *child=new kGUIXMLItem();
	child->m_name=name;
	vs.Sprintf("%f",value);
	child->m_value.SetString(vs.GetString());
	AddChild(child);
	return(child);
}

char *kGUIXMLItem::Load(kGUIXML *root,kGUIString *ts,char *fp,kGUIXMLItem *parent)
{
	int nl;
	char *sfp;
	char q;

	root->Update(fp);	/* call users callback if there is one */

	SetEncoding(root->GetEncoding());
	parent->AddChild(this);
	assert(fp[0]=='<',"Open bracket not found!");
	++fp;
	m_parm=false;
	sfp=fp;
	while(fp[0]!=0x0a && fp[0]!=0x0d && fp[0]!=' ' && fp[0]!=9 && fp[0]!='/' && fp[0]!='>')
		++fp;

	/* since there are many duplicate tags in XML files instead of saving each unique name */
	/* in each XMLItem class, instead they are cached (Using a hash table) and XMLItems */
	/* just refer to the cached names */

	ts->SetString(sfp,(int)(fp-sfp));
	m_name=root->CacheName(ts->GetString());

	nl=(int)(fp-sfp);

	while(fp[0]==0x0a || fp[0]==0x0d)
		++fp;
	if(fp[0]==' ' || fp[0]=='/')
	{
		if(fp[0]==' ')
			++fp;
		if(fp[0]=='/' && fp[1]=='>')
		{
			fp+=2;
			goto done;
		}
		/* this command has values in the header */
		while(fp[0]!='>')
		{
			kGUIXMLItem *parm=new kGUIXMLItem();
			
			/* apply file encoding to parms */
			parm->SetEncoding(root->GetEncoding());
			sfp=fp;
			parm->m_parm=true;
			while(fp[0]!='=')
				++fp;

			ts->SetString(sfp,(int)(fp-sfp));
			ts->Trim();
			parm->m_name=root->CacheName(ts->GetString());

			++fp;	/* skip '=' */
			while(fp[0]==' ')
				++fp;		/* skip any spaces between = and start quote */

			q=fp[0];
			assert((q=='\"' || q=='\''),"Quote not found!");
			++fp;
			sfp=fp;
			while(fp[0]!=q)
				++fp;

			ts->SetString(sfp,(int)(fp-sfp));
			kGUIXMLCODES::Shrink(ts,&parm->m_value);

			++fp;
			this->AddChild(parm);
			while(fp[0]==0x0a || fp[0]==0x0d || fp[0]==' ' || fp[0]==9)
				++fp;
			if(fp[0]=='?' && fp[1]=='>')
			{
				fp+=2;
				goto done;
			}
			else if(fp[0]=='/' && fp[1]=='>')
			{
				fp+=2;
				goto done;
			}
		}
		++fp;	/* skip '>' */
		while( (fp[0]==10) || (fp[0]==13) || (fp[0]==' ') || (fp[0]==9) )
			++fp;
	}
	else
		++fp;

	sfp=fp;

	while(fp[0]!='<')
		++fp;

	if(sfp!=fp)
	{
		ts->SetString(sfp,(int)(fp-sfp));
		kGUIXMLCODES::Shrink(ts,&m_value);
	}
	/* grab children until </name> is found */
	while(1)
	{
		kGUIXMLItem *child;

		/* check for comment here?? */
		while(fp[0]=='<' && fp[1]=='!' && fp[2]=='-' && fp[3]=='-')
		{
			fp+=4;
			while(!(fp[0]=='-' && fp[1]=='-' && fp[2]=='>'))
				++fp;

			fp+=3;
			/* eat whitespace */
			while(fp[0]==0x0a || fp[0]==0x0d || fp[0]==' ' || fp[0]==9 )
				++fp;
		}

		if((fp[0]=='<') && (fp[1]=='/') && (fp[nl+2]=='>'))
		{
			if(!strncmp(fp+2,m_name,nl))
			{
				fp+=nl+3;
				break;
			}
		}
		child=new kGUIXMLItem();
		fp=child->Load(root,ts,fp,this);
	}
done:;

	while((fp[0]==10) || (fp[0]==13) || (fp[0]==' ') || (fp[0]==9))
		++fp;
	return(fp);
}

void kGUIXMLItem::Copy(kGUIXMLItem *copy)
{
	unsigned int i;
	kGUIXMLItem *c;

	/* free any old children */
	for(i=0;i<m_numchildren;++i)
		delete m_children.GetEntry(i);

	m_parm=copy->m_parm;
	m_name=copy->m_name;
	m_value.SetString(&copy->m_value);

	m_numchildren=copy->m_numchildren;
	m_children.Alloc(m_numchildren);
	for(i=0;i<m_numchildren;++i)
	{
		c=new kGUIXMLItem();
		c->Copy(copy->m_children.GetEntry(i));
		m_children.SetEntry(i,c);
	}
}

/* todo: change from using fopen etc to using DataHandle */

void kGUIXMLItem::Save(kGUIXML *top,kGUIString *ts,FILE *fp,unsigned int level)
{
	unsigned int i,nc;
	kGUIXMLItem *child;
	bool open=true;

	for(i=0;i<level;++i)
		fprintf(fp,"  ");

	if(m_name)
	{
		ts->SetEncoding(ENCODING_8BIT);
		ts->SetString(m_name);
		ts->ChangeEncoding(top->GetEncoding());
		fprintf(fp,"<%s",ts->GetString());
	}
	else
		open=false;
	nc=0;

	/* export all parms */
	for(i=0;i<m_numchildren;++i)
	{
		child=GetChild(i);
		if(child->m_parm==true)
		{
			ts->SetEncoding(ENCODING_8BIT);
			ts->SetString(child->m_name);
			ts->ChangeEncoding(top->GetEncoding());
			fprintf(fp," %s=",ts->GetString());

			kGUIXMLCODES::Expand(&child->m_value,ts);

			ts->ChangeEncoding(top->GetEncoding());
			/* if string contains double quotes then use single quotes in string */
			if(!strstr(ts->GetString(),"\""))
				fprintf(fp,"\"%s\"",ts->GetString());
			else
				fprintf(fp,"'%s'",ts->GetString());
		}
	}
	/* export all children */
	for(i=0;i<m_numchildren;++i)
	{
		child=GetChild(i);
		if(child->m_parm==false)
		{
			++nc;	/* number of non-parm  children */
			if(open)
			{
				open=false;
				fprintf(fp,">\n");
			}
			GetChild(i)->Save(top,ts,fp,level+1);
		}
	}
	if(open)
		fprintf(fp,">");

	if(m_value.GetLen())
	{
		kGUIXMLCODES::Expand(&m_value,ts);

		ts->ChangeEncoding(top->GetEncoding());
		fprintf(fp,"%s",ts->GetString());
	}
	if(m_name)
	{
		if(m_name[0]!='?')
		{
			if(nc)
			{
				for(i=0;i<level;++i)
					fprintf(fp,"  ");
			}

			ts->SetEncoding(ENCODING_8BIT);
			ts->SetString(m_name);
			ts->ChangeEncoding(top->GetEncoding());
			fprintf(fp,"</%s>\n",ts->GetString());
		}
	}
}

kGUIXMLItem *kGUIXMLItem::Locate(const char *name,bool add)
{
	unsigned int i;
	kGUIXMLItem *child;

	for(i=0;i<m_numchildren;++i)
	{
		child=m_children.GetEntry(i);

		if(!strcmp(child->m_name,name))
			return(child);
	}
	if(add==true)
		return(AddChild(name));
	return(0);
}

kGUIXML::kGUIXML()
{
	m_encoding=ENCODING_8BIT;
	m_root=new kGUIXMLItem();

	/* namecache must be defined before loading an XML file */
	m_namecache=0;
	m_namecachelocal=false;

	m_numinpool=0;
	m_itempool.Init(256,-1);
}

kGUIXML::~kGUIXML()
{
	unsigned int i;

	if(m_namecachelocal==true)
		delete m_namecache;

	/* delete any items left in the pool */
	for(i=0;i<m_numinpool;++i)
		delete m_itempool.GetEntry(i);

	delete m_root;
}

const char *kGUIXML::CacheName(const char *name)
{
	const char *n;

	n=m_namecache->FindName(name);
	if(!n)
	{
		m_namecache->Add(name,0);
		n=m_namecache->FindName(name);
	}
	return(n);
}

bool kGUIXML::Load(const char *filename)
{
	char *fd;
	char *fdend;
	kGUIXMLItem *xitem;
	kGUIXMLItem *encoding;
	char *filedata;
	kGUIString tempstring;
	bool gotencoding=false;

	/*  if not defined then we will */
	if(!m_namecache)
	{
		m_namecachelocal=true;	/* tells us to free it upon exit */

		m_namecache=new Hash();
		m_namecache->Init(8,0);
	}

	assert(m_namecache!=0,"Name Cache needs to be defined before loading an XML file");
	tempstring.Alloc(65536);
	delete m_root;
	m_root=new kGUIXMLItem();

	filedata=(char *)kGUI::LoadFile(filename,&m_filesize);
	if(!filedata)
		return(false);

	/* parse the file! */
	fd=filedata;
	fdend=filedata+m_filesize;
	while(fd[0]==' ')
		++fd;

	m_fd=fd;	/* only used for callback */

	/* eat any trailing c/r or spaces etc */
	while(fdend>fd && fdend[-1]!='>')
		--fdend;

	while(fd<fdend)
	{
		xitem= new kGUIXMLItem();
		fd=xitem->Load(this,&tempstring,fd,m_root);

		/* can we get the encoding for this file? */
		if(gotencoding==false)
		{
			if(!strcmp("?xml",xitem->GetName()))
			{
				encoding=xitem->Locate("encoding");
				if(encoding)
				{
					SetEncoding(kGUIString::GetEncoding(encoding->GetValueString()));
					tempstring.SetEncoding(GetEncoding());
					gotencoding=true;
				}
			}
		}

	}

	free((void *)filedata);

	return(true);
}

void kGUIXML::Update(char *cur)
{
	if(m_loadcallback.IsValid())
		m_loadcallback.Call((int)(cur-m_fd),m_filesize);
}

/* todo: change to use a datahandle instead of fopen */

bool kGUIXML::Save(const char *filename)
{
	FILE *fp;
	kGUIString tempstring;

	fp=fopen(filename,"wb");
	if(!fp)
		return(false);	/* error opening file */
	
	tempstring.Alloc(65536);

	if(m_encoding==ENCODING_8BIT)
		fprintf(fp,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	else
		fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	
	/* todo, convert all strings to encoding */
	m_root->Save(this,&tempstring,fp,0);
	fclose(fp);
	return(true);
}

/*********************************************************************************/
/* this is for loading really BIG xml files that are too large to fit in memory */

bool kGUIXML::StreamLoad(const char *filename)
{
	kGUIXMLItem *xitem;
	kGUIXMLItem *encoding;
	bool gotencoding=false;
	unsigned long long fs;

	/*  if not defined then we will */
	if(!m_namecache)
	{
		m_namecachelocal=true;	/* tells us to free it upon exit */

		m_namecache=new Hash();
		m_namecache->Init(8,0);
	}

	assert(m_namecache!=0,"Name Cache needs to be defined before loading an XML file");
	m_tempstring.Alloc(65536);
	delete m_root;
	m_root=new kGUIXMLItem();

	m_dh.SetFilename(filename);
	if(!m_dh.Open())
		return(false);

	m_dh.InitReadStream(16,2);	/* use 2 64k buffers */ 

	m_fd=0;	/* only used for callback */

	/* pull end back to last '>' character */
	fs=m_dh.GetSize();
	do
	{
		m_dh.Seek(fs-1);
		if(m_dh.StreamPeekChar()=='>')
			break;
		--fs;
	}while(fs>1);

	/* eat any leading spaces */
	m_dh.Seek((unsigned long long)0);
	while(m_dh.StreamPeekChar()==' ')
		m_dh.Skip(1);

	while(m_dh.GetOffset()<fs)
	{
		xitem=StreamLoad(m_root);

		/* can we get the encoding for this file? */
		if(gotencoding==false)
		{
			if(!strcmp("?xml",xitem->GetName()))
			{
				encoding=xitem->Locate("encoding");
				if(encoding)
				{
					SetEncoding(kGUIString::GetEncoding(encoding->GetValueString()));
					m_tempstring.SetEncoding(GetEncoding());
					gotencoding=true;
				}
			}
		}

	}
	m_dh.Close();
	return(true);
}

kGUIXMLItem *kGUIXML::StreamLoad(kGUIXMLItem *parent)
{
	kGUIXMLItem *item;
	long nl;
	char c;
	char q;
	int sl;

	Update((char *)m_dh.GetOffset());	/* call users callback if there is one */

	/* change to get from pool */
	item=PoolGet();

	item->SetEncoding(GetEncoding());
	parent->AddChild(item);
	assert(m_dh.StreamPeekChar()=='<',"Open bracket not found!");
	m_dh.StreamSkip(1);				/* skip the '<' */
	item->m_parm=false;

	nl=0;
	c=m_dh.StreamPeekChar();
	while(c!=0x0a && c!=0x0d && c!=' ' && c!=9 && c!='/' && c!='>')
	{
		m_dh.StreamSkip(1);
		c=m_dh.StreamPeekChar();
		++nl;
	}

	/* since there are many duplicate tags in XML files instead of saving each unique name */
	/* in each XMLItem class, instead they are cached (Using a hash table) and XMLItems */
	/* just refer to the cached names */

	m_dh.StreamSkip(-nl);		/* go back to beginning of string */
	m_dh.Read(&m_tempstring,(unsigned long)nl);	/* read in the string */
	item->m_name=CacheName(m_tempstring.GetString());

	while(c==0x0a || c==0x0d)
	{
		m_dh.StreamSkip(1);
		c=m_dh.StreamPeekChar();
	}

	if(c==' ' || c=='/')
	{
		if(c==' ')
		{
			m_dh.StreamSkip(1);
			c=m_dh.StreamPeekChar();
		}
		if(!m_dh.StreamCmp("/>",2))
		{
			m_dh.StreamSkip(2);
			c=m_dh.StreamPeekChar();
			goto done;
		}
		/* this command has values in the header */
		while(c!='>')
		{
			kGUIXMLItem *parm=PoolGet();
			
			/* apply file encoding to parms */
			parm->SetEncoding(GetEncoding());
			sl=0;
			parm->m_parm=true;
			while(c!='=')
			{
				m_dh.StreamSkip(1);
				c=m_dh.StreamPeekChar();
				++sl;
			}

			m_dh.StreamSkip(-sl);		/* go back to beginning of string */
			m_dh.Read(&m_tempstring,(unsigned long)sl);	/* read in the string */
			m_tempstring.Trim();
			parm->m_name=CacheName(m_tempstring.GetString());

			m_dh.StreamSkip(1);			/* skip '=' */
			c=m_dh.StreamPeekChar();

			while(c==' ')
			{
				m_dh.StreamSkip(1);
				c=m_dh.StreamPeekChar();
			}

			q=c;
			assert((q=='\"' || q=='\''),"Quote not found!");

			m_dh.StreamSkip(1);			/* skip the opening quote */
			c=m_dh.StreamPeekChar();

			sl=0;
			while(c!=q)
			{
				m_dh.StreamSkip(1);
				c=m_dh.StreamPeekChar();
				++sl;
			}
			m_dh.StreamSkip(-sl);		/* go back to beginning of string */
			m_dh.Read(&m_tempstring,(unsigned long)sl);	/* read in the string */
			kGUIXMLCODES::Shrink(&m_tempstring,&parm->m_value);

			m_dh.StreamSkip(1);			/* skip the close quote */
			c=m_dh.StreamPeekChar();

			item->AddChild(parm);

			while(c==0x0a || c==0x0d || c==' ' || c==9)
			{
				m_dh.StreamSkip(1);
				c=m_dh.StreamPeekChar();
			}

			if(!m_dh.StreamCmp("?>",2) || !m_dh.StreamCmp("/>",2))
			{
				m_dh.StreamSkip(2);
				c=m_dh.StreamPeekChar();
				goto done;
			}
		}
		m_dh.StreamSkip(1);			/* skip '>' */		
		c=m_dh.StreamPeekChar();
		while( (c==10) || (c==13) || (c==' ') || (c==9) )
		{
			m_dh.StreamSkip(1);
			c=m_dh.StreamPeekChar();
		}
	}
	else
	{
		m_dh.StreamSkip(1);
		c=m_dh.StreamPeekChar();
	}

	sl=0;
	while(c!='<')
	{
		m_dh.StreamSkip(1);
		c=m_dh.StreamPeekChar();
		++sl;
	}

	if(sl)
	{
		m_dh.StreamSkip(-sl);		/* go back to beginning of string */
		m_dh.Read(&m_tempstring,(unsigned long)sl);	/* read in the string */
		kGUIXMLCODES::Shrink(&m_tempstring,&item->m_value);
	}

	/* grab children until </name> is found */
	while(1)
	{
		/* check for comment here?? */
		while(!m_dh.StreamCmp("<!--",4))
		{
			m_dh.StreamSkip(4);		/* skip comment start */

			while(m_dh.StreamCmp("-->",3))
				m_dh.StreamSkip(1);

			m_dh.StreamSkip(3);		/* skip comment end */
			c=m_dh.StreamPeekChar();

			/* eat whitespace */
			while(c==0x0a || c==0x0d || c==' ' || c==9 )
			{
				m_dh.StreamSkip(1);
				c=m_dh.StreamPeekChar();
			}
		}

		if(!m_dh.StreamCmp("</",2) && !m_dh.StreamCmp(">",1,nl+2))
		{
			if(!m_dh.StreamCmp(item->m_name,nl,2))
			{
				m_dh.StreamSkip(nl+3);	/* skip </name> */
				c=m_dh.StreamPeekChar();
				break;
			}
		}
		/* recursively load a child */
		StreamLoad(item);
	}
done:;

	while((c==10) || (c==13) || (c==' ') || (c==9))
	{
		m_dh.StreamSkip(1);
		c=m_dh.StreamPeekChar();
	}
	/* this is a virtual function */
	ChildLoaded(item,parent);

	return(item);
}
