/*********************************************************************************/
/* kGUI - kguicookies.cpp                                                        */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                                */
/*                                                                               */
/*    kGUI is free software; you can redistribute it and/or modify               */
/*    it under the terms of the GNU Lesser General Public License as published by*/
/*    the Free Software Foundation; version 2.                                   */
/*                                                                               */
/*    kGUI is distributed in the hope that it will be useful,                    */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/*    GNU General Public License for more details.                               */
/*                                                                               */
/*    http://www.gnu.org/licenses/lgpl.txt                                       */
/*                                                                               */
/*    You should have received a copy of the GNU General Public License          */
/*    along with kGUI; if not, write to the Free Software                        */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

/*********************************************************************************/
/*                                                                               */
/* This is the cookie handler. It needs to be 'attached' to the kGUI framework   */
/* by calling SetCookieJar. The download class then uses the current CookieJar   */
/* for reading and writing cookies. Any applications that use this and need to   */
/* have persistent cookies need to load and save the cookies to a XML file       */
/* ( usually a preferences file for the app ) using the Load and Save cookies    */
/* functions.                                                                    */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguicookies.h"

enum
{
COOKIEATT_DOMAIN,
COOKIEATT_MAXAGE,
COOKIEATT_EXPIRES,
COOKIEATT_PATH,
COOKIEATT_SECURE,
COOKIEATT_VERSION,
COOKIEATT_COMMENT,
COOKIEATT_HTTPONLY
};

#define ANYPORT -1

kGUICookie::kGUICookie()
{
	m_port=ANYPORT;
	m_secure=false;
	m_permanent=false;
	m_expiry.SetToday();
	m_remove=false;
}

kGUICookie::~kGUICookie()
{

}

void kGUICookie::SetAttribute(unsigned int att,kGUIString *value,kGUIString *domain,kGUIString *subdomain,kGUIString *url)
{
	switch(att)
	{
	case COOKIEATT_DOMAIN:
	{
		bool validdomain;
		int offset=0;

		//remove leading '.' if necessary
		if(value->GetChar(0)=='.')
			offset=1;
		else
			offset=0;

		if(stricmp(value->GetString()+offset,domain->GetString()))
			validdomain=true;
		else
		{
			if(subdomain->GetLen())
			{

				if(stricmp(value->GetString()+offset,subdomain->GetString()))
					validdomain=true;
				else
					validdomain=false;
			}
			else
				validdomain=false;
		}
		if(validdomain)
			m_domain.SetString(value->GetString()+offset);
	}
	break;
	case COOKIEATT_PATH:
	{
		int vl;
		int pl;

		pl=url->GetLen();
		vl=value->GetLen();
		if(vl>pl)
			return;		/* error path len cannot be longer than url root */

		if(strnicmp(url->GetString(),value->GetString(),vl))
			return;	/* value is not substring of url root */

		m_path.SetString(value->GetString());
	}
	break;
	case COOKIEATT_MAXAGE:
	{
		int seconds;

		/* 0=discard this cookie ( well the previous version of it really ) */
		/* else number of seconds for this cookie to live */
		seconds=value->GetInt();
		if(!seconds)
			m_remove=true;	/* this is a remove previous instance of this cookie, cookie */
		else
		{
			m_expiry.SetToday();
			m_expiry.AddSeconds(seconds);
			m_permanent=true;
		}
	}
	break;
	case COOKIEATT_EXPIRES:
		/* date is in format Weekday, DD-Mon-YY HH:MM:SS GMT */
		m_expiry.Set(value->GetString());
		m_permanent=true;
	break;
	case COOKIEATT_SECURE:
		m_secure=stricmp(value->GetString(),"true")!=0;
	break;
	case COOKIEATT_HTTPONLY:
		/* todo: */
	break;
	case COOKIEATT_COMMENT:
	case COOKIEATT_VERSION:
		/* we don't bother saving these either */
	break;
	}
}

/*****************************************************************/


kGUICookieDomain::~kGUICookieDomain()
{
	kGUICookie *cookie;
	kGUICookie *nextcookie;

	/* free all attached cookies */
	cookie=m_links.GetFirst();
	while(cookie!=m_links.GetTail())
	{
		nextcookie=cookie->GetNext();
		delete cookie;
		cookie=nextcookie;
	}
}

/*****************************************************************/

typedef struct
{
	const char *commands;
	int id;
}COOKIEATTS_DEF;

static COOKIEATTS_DEF cookieatts[]={
	{"domain",COOKIEATT_DOMAIN},
	{"max-age",COOKIEATT_MAXAGE},
	{"expires",COOKIEATT_EXPIRES},
	{"path",COOKIEATT_PATH},
	{"secure",COOKIEATT_SECURE},
	{"version",COOKIEATT_VERSION},
	{"httponly",COOKIEATT_HTTPONLY},	/* microsoft specific */
	{"comment",COOKIEATT_COMMENT}};

kGUICookieJar::kGUICookieJar()
{
	unsigned int i;

	/* we are going to use a hash table to convert Cookie attributes to IDS */
	m_atthash.Init(8,sizeof(int));
	for(i=0;i<(sizeof(cookieatts)/sizeof(COOKIEATTS_DEF));++i)
		m_atthash.Add(cookieatts[i].commands,&cookieatts[i].id);

	/* hash table for domain names */
	m_domainhash.Init(12,sizeof(kGUICookieDomain *));

	/* hash table for cookies */
	m_cookiehash.Init(16,sizeof(kGUICookie *));
}

kGUICookieJar::~kGUICookieJar()
{
	unsigned int i;
	unsigned int numdomains;
	kGUICookieDomain *domain;
	HashEntry *he;

	/* delete all domain entries */
	numdomains=m_domainhash.GetNum();
	he=m_domainhash.GetFirst();
	for(i=0;i<numdomains;++i)
	{
		domain=*((kGUICookieDomain **)he->m_data);
		delete domain;
		he=he->GetNext();
	}
}

/* build a list of all current cookes, this is used for the 'viewcookies' code */
unsigned int kGUICookieJar::GetCookieList(Array<kGUICookie *>*cookielist)
{
	unsigned int i,n,numdomains;
	kGUICookieDomain *domain;
	kGUICookie *cookie;
	HashEntry *he;

	/* since a cookie can be changed in another thread, we need to get exclusive access */
	m_busymutex.Lock();

	cookielist->Init(1024,256);
	
	n=0;
	numdomains=m_domainhash.GetNum();
	he=m_domainhash.GetFirst();
	for(i=0;i<numdomains;++i)
	{
		domain=*((kGUICookieDomain **)he->m_data);

		/* all valid cookies attached to this domain */
		cookie=domain->GetFirst();
		while(cookie!=domain->GetTail())
		{
			if(cookie->GetRemove()==false)
				cookielist->SetEntry(n++,cookie);
			cookie=cookie->GetNext();
		}
		he=he->GetNext();
	}

	m_busymutex.UnLock();
	return(n);
}

//load previous cookies from XML file
void kGUICookieJar::Load(kGUIXMLItem *root)
{
	unsigned int i,n;
	kGUIXMLItem *entry;
	kGUICookie *c;
	kGUIDate now;

	now.SetToday();

	n=root->GetNumChildren();
	for(i=0;i<n;++i)
	{
		entry=root->GetChild(i);
		c=new kGUICookie();

		c->SetName(entry->Locate("name")->GetValue());
		c->SetValue(entry->Locate("value")->GetValue());
		c->SetDomain(entry->Locate("domain")->GetValue());
		c->SetPath(entry->Locate("path")->GetValue());
		c->SetPort(entry->Locate("port")->GetValueInt());
		c->SetSecure(entry->Locate("secure")->GetValueInt()==1);
		c->SetPermanent(true);	/* wouldn't have been saved if it wasn't permanent */
		c->SetExpiry(entry->Locate("expiry")->GetValue());

		/* add non-expired cookies, delete expires ones */
		if(now.GetDiffSeconds(c->GetExpiry())>0)
			UpdateCookie(c);
		else
			delete c;
	}
}

//save current permanent cookies to XML file
void kGUICookieJar::Save(kGUIXMLItem *root)
{
	unsigned int i;
	unsigned int numdomains;
	kGUIDate now;
	kGUICookieDomain *domain;
	kGUICookie *cookie;
	HashEntry *he;
	kGUIXMLItem *entry;
	kGUIString dt;	/* date/time string */

	now.SetToday();
	
	/* save valid cookies */
	numdomains=m_domainhash.GetNum();
	he=m_domainhash.GetFirst();
	for(i=0;i<numdomains;++i)
	{
		domain=*((kGUICookieDomain **)he->m_data);

		/* save all valid cookies attached to this domain */
		cookie=domain->GetFirst();
		while(cookie!=domain->GetTail())
		{
			/* should we save it? */
			if(cookie->GetPermanent()==true && cookie->GetRemove()==false)
			{
				/* now check expiry */
				if(cookie->GetExpiry()->GetDiffSeconds(&now)<0)
				{
					/* ok, it is valid, let's save it */
					entry=root->AddChild("cookie");
					entry->AddParm("name",cookie->GetName());
					entry->AddParm("value",cookie->GetValue());
					entry->AddParm("domain",cookie->GetDomain());
					entry->AddParm("path",cookie->GetPath());
					entry->AddParm("port",cookie->GetPort());
					entry->AddParm("secure",cookie->GetSecure()==true?1:0);
					cookie->GetExpiry()->ShortDateTime(&dt);
					entry->AddParm("expiry",&dt);
				}
			}
			cookie=cookie->GetNext();
		}
		he=he->GetNext();
	}
}

//if there is no valid subdomain then subdomain is an empty string
//todo: add list of known invalid subdomains ".co.ok" etc
void kGUICookieJar::GetSubDomain(kGUIString *domain,kGUIString *subdomain)
{
	const char *cp;

	subdomain->Clear();
	cp=strstr(domain->GetString(),".");
	if(!cp)
		return;

	/* subdomain must contain a period */
	subdomain->SetString(cp+1);
	cp=strstr(domain->GetString(),".");
	if(!cp)
	{
		subdomain->Clear();
		return;
	}
}


//RFC 2109
//4.2.2  Set-Cookie Syntax
//
//   The syntax for the Set-Cookie response header is
//
//   set-cookie      =       "Set-Cookie:" cookies
//   cookies         =       1#cookie
//   cookie          =       NAME "=" VALUE *(";" cookie-av)
//   NAME            =       attr
//   VALUE           =       value
//   cookie-av       =       "Comment" "=" value
//                   |       "Domain" "=" value
//                   |       "Max-Age" "=" value
//                   |       "Path" "=" value
//                   |       "Secure"
//                   |       "Version" "=" 1*DIGIT
// optional
// Expires=Wdy, DD-Mon-YY HH:MM:SS GMT

/* this is a "Set-Cookie" line it can have more than one cookie in it */

void kGUICookieJar::SetCookie(kGUIString *s,kGUIString *domain,kGUIString *url)
{
	unsigned int i;
	unsigned int numwords;
	kGUIStringSplit ss;
	kGUIStringSplit ws;
	kGUIString defpath;
	kGUIString subdomain;
	kGUIString word;
	kGUIString value;
	kGUICookie *c;
	bool gotname;
	unsigned int *ci;
	const char *cp;

	/* the Set-Cookie string can contain multiple cookies */
	/* the first name/value pair needs to be the cookie name and value */
	/* it is then followed by the cookie attributes and if a unknown attribute */
	/* is encountered then it is assumed to be the start of another cookie */

	numwords=ss.Split(s,";",0,true);

	assert(numwords>0,"Error: no parts to set-cookie string!\n");

	/* our paths don't start with '/' so pre-pend it. */
	defpath.Sprintf("/%S",url);
	/* get pointer to last slash since we are extracting just the path ( including the last '/' */
	cp=strrchr(defpath.GetString(),'/');
	if(cp)
		defpath.Clip((int)(cp-defpath.GetString())+1);
	GetSubDomain(domain,&subdomain);

	c=new kGUICookie();
	c->SetDomain(domain);
	c->SetPath(&defpath);
	gotname=false;
	for(i=0;i<numwords;++i)
	{
		cp=strstr(ss.GetWord(i)->GetString(),"=");
		if(!cp)
		{
			word.SetString(ss.GetWord(i));
			value.Clear();
		}
		else
		{
			int wl;

			/* calc offset to equals sign */
			wl=(int)(cp-ss.GetWord(i)->GetString());

			word.SetString(ss.GetWord(i));
			word.Clip(wl);
			value.SetString(ss.GetWord(i));
			value.Delete(0,wl+1);	/* remove the word chars and the '=' */
		}

		word.Trim();	/* clean any trailing spaces */
		value.Trim();	/* remove any leading spaces */

		if(word.GetLen())
		{
			if(gotname==true)
			{
				/* is this an attribute for the current cookie or a new cookie? */
				ci=(unsigned int *)m_atthash.Find(word.GetString());
				if(!ci)
				{
					/* this is not an attrbiute, so, update the current cookie as we are done with it */
					/* if domain was not set then use the current one */
					UpdateCookie(c);

					/* start a new one cookie */
					c=new kGUICookie();
					c->SetName(&word);
					c->SetValue(&value);
					c->SetDomain(domain);
					c->SetPath(&defpath);
				}
				else
					c->SetAttribute(*(ci),&value,domain,&subdomain,&defpath);
			}
			else
			{
				c->SetName(&word);
				c->SetValue(&value);
				gotname=true;
			}
		}
	}
	if(gotname)
		UpdateCookie(c);
	else
		delete c;
}

void kGUICookieJar::UpdateCookie(kGUICookie *cookie)
{
	kGUICookieDomain **dp;
	kGUICookieDomain *domain;
	kGUICookie **cc;
	kGUICookie *prev;
	kGUICookie *c;
	kGUIString hashstring;

	/* check for replacing an existing cookie? */
	hashstring.Sprintf("%s:%s:%s",	cookie->GetDomain()->GetString(),
									cookie->GetName()->GetString(),
									cookie->GetPath()->GetString());

	/* this code can be called asynchronously via many threads so it needs */
	/* to be thread safe */
	m_busymutex.Lock();

	cc=(kGUICookie **)m_cookiehash.Find(hashstring.GetString());
	if(cc)
	{
		/* todo, if user is viewing cookies and this cookie is replaced then it */
		/* will crash, so this code should just update the contents of the previous cookie? */

		/* cookie already exists, replace it with this one */
		c=*(cc);
		prev=c->GetPrev();		/* get previous entry */
		c->Unlink();			/* unlink me from my parents linked list */
		cookie->Link(prev);		/* attach new cookie to place where old one was */
		*(cc)=cookie;			/* update hash pointer to new entry */

		delete c;				/* free old cookie */
	}
	else
	{
		/* if we got here then this is a NEW cookie */
		m_cookiehash.Add(hashstring.GetString(),&cookie);

		/* is this a new domain too? */
		dp=(kGUICookieDomain **)m_domainhash.Find(cookie->GetDomain()->GetString());
		if(!dp)
		{
			domain=new kGUICookieDomain();
			m_domainhash.Add(cookie->GetDomain()->GetString(),&domain);
		}
		else
			domain=*(dp);

		/* put longest paths first */
		c=domain->GetFirst();
		while(c!=domain->GetTail())
		{
			if(c->GetPath()->GetLen()<=cookie->GetPath()->GetLen())
				break;
			c=c->GetNext();
		}
		/* attach cookie here */
		if(c==domain->GetTail())
			c=c->GetPrev();
		cookie->Link(c);
	}
	/* free mutex */
	m_busymutex.UnLock();
}

void kGUICookieJar::GetCookies(kGUIString *s,kGUIString *domain,kGUIString *url)
{
	unsigned int i;
	unsigned int nc;
	kGUIString ds;
	kGUICookieDomain **dp;
	kGUICookieDomain *d;
	kGUICookie *cookie;
	kGUIDate now;
	unsigned int urllen=url->GetLen();
	unsigned int pathlen;

	nc=0;			/* number of valid cookies found */
	s->Clear();		/* cookie results string, null=no cookies found */
	now.SetToday();

	/* this code can be called asynchronously via many threads so it needs */
	/* to be thread safe */
	m_busymutex.Lock();

	/* two passes, first pass is exact domain match, 2nd pass is next sbudomain ( if valid ) */
	for(i=0;i<2;++i)
	{
		if(!i)
			ds.SetString(domain);
		else
		{
			GetSubDomain(domain,&ds);
			if(!ds.GetLen())
				break;			/* no valid subdomain */
		}

		/* is this domain in the domain list? */
		dp=(kGUICookieDomain **)m_domainhash.Find(ds.GetString());
		if(dp)
		{
			d=*(dp);

			/* ok, now find valid cookies with matching paths */

			cookie=d->GetFirst();
			while(cookie!=d->GetTail())
			{
				if(cookie->GetRemove()==false)
				{
					/* now check expiry */
					if((cookie->GetPermanent()==false) || (cookie->GetExpiry()->GetDiffSeconds(&now)<0))
					{
						/* now check path, URLs don't have leading '/', paths Do */
						pathlen=cookie->GetPath()->GetLen()-1;
						if(pathlen<=urllen)
						{
							if(!strnicmp(url->GetString(),cookie->GetPath()->GetString()+1,pathlen))
							{
								/* ok, we have a valid cookie, is it the first? */
#if 1
								if(!nc++)
									s->Append("Cookie: ");
								else
									s->Append("; ");
								s->ASprintf("%s=%s",cookie->GetName()->GetString(),cookie->GetValue()->GetString());
#else
								if(!nc++)
									s->Append("Cookie: $Version=\"1\"; ");
								else
									s->Append("; ");
								s->ASprintf("%s=\"%s\"; $Path=\"%s\"; $Domain=\"%s\"",
										cookie->GetName()->GetString(),
										cookie->GetValue()->GetString(),
										cookie->GetPath()->GetString(),
										cookie->GetDomain()->GetString());
#endif
							}
						}
					}
				}
				cookie=cookie->GetNext();
			}
		}
	}
	/* free mutex */
	m_busymutex.UnLock();
}
