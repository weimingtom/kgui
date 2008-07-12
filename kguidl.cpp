/**********************************************************************************/
/* kGUI - kguidl.cpp                                                              */
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
#include "kguidl.h"
#include "kguicookies.h"
#include "kguissl.h"

/*! @file kguidl.cpp 
    @brief This is a download class for downloading pages and other assets from the      
 internet. It can be run synchronously or asynchronously in it's down thread.  
 It can handle sending and receiving cookies if there is an attached CookieJar 
 As well as using GET, it can do a POST and send along POST data with the request.
 It can handle redirects, if-modififed since headers automatically as well.     
 Content is downloaded to a DataHandle class so it can download to a file, or  
 memory or any other type that DataHandle classes can support. */

/*! @todo Add Secure Certificate handler similar to CookieJar handler for Cookies */
/*! @todo Add Secure HTTPS mode support */

//make pc version use MINGW version so we do our own cookie handling
#if defined(WIN32)
#if !defined(MINGW)
#define MINGW
#endif
#endif

#if defined(MINGW)

#include <winsock2.h>
#include <sys/utime.h>
#include <time.h>
//#include <sys/time.h>

#elif defined(LINUX) || defined(MACINTOSH)

#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#elif defined(WIN32)

#include <afxinet.h>
#include <sys/utime.h>

#else
#error
#endif

kGUIMutex kGUIDownloadEntry::m_gethostmutex;

/********************************/
/* download from internet class */
/********************************/

kGUIDownloadEntry::kGUIDownloadEntry()
{
#if DLDEBUG
	m_debugstate=0;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	m_secure=false;
	m_dh=0;
	m_ah=0;		/* authenticate handler */
	m_allowcookies=true;
	m_status=DOWNLOAD_ERROR;
	m_abort=false;
	m_asyncactive=false;
}

kGUIDownloadEntry::~kGUIDownloadEntry()
{
	/* if currently in the process of doing an async */
	/* download then set abort and wait to finish */
	if(m_thread.GetActive())
		WaitFinished();
}

void kGUIDownloadEntry::WaitFinished(void)
{
	m_abort=true;
	while(m_thread.GetActive())
	{
//		printf("Waiting for thread to finish!\n");
//		fflush(stdout);
		kGUI::Sleep(1);

	}
}

void kGUIDownloadEntry::AsyncDownLoad(DataHandle *dh,kGUIString *fullurl)
{
#if DLDEBUG
	m_debugstate=1;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	if(GetAsyncActive())
		WaitFinished();

#if DLDEBUG
	m_debugstate=2;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	m_asyncactive=true;
	m_abort=false;
	m_url.SetString(fullurl);
	m_donecallback.Set(0,0);

#if DLDEBUG
	m_debugstate=3;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	/* createthread for download */
	m_dh=dh;
	m_thread.Start(this,CALLBACKNAME(Download));
}


void kGUIDownloadEntry::AsyncDownLoad(DataHandle *dh,kGUIString *fullurl,void *codeobj,void (*code)(void *,int status))
{
#if DLDEBUG
	m_debugstate=1;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	if(GetAsyncActive())
		WaitFinished();

#if DLDEBUG
	m_debugstate=2;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	m_asyncactive=true;
	m_abort=false;
	m_url.SetString(fullurl);
	m_donecallback.Set(codeobj,code);

#if DLDEBUG
	m_debugstate=3;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	/* createthread for download */
	m_dh=dh;
	m_thread.Start(this,CALLBACKNAME(Download));
}

int kGUIDownloadEntry::DownLoad(DataHandle *dh,kGUIString *fullurl)
{
	m_abort=false;
	m_url.SetString(fullurl);
	m_donecallback.Set(0,0);
	m_dh=dh;
	Download();
	return(m_status);
}

#if defined(LINUX) || defined(MACINTOSH) || defined(MINGW)
#define MODE_HTTP 0
#define MODE_HTTPS 1

static bool alldigits(kGUIString *s)
{
	unsigned int i;
	unsigned int nb;

	for(i=0;i<s->GetLen();i+=nb)
	{
		switch(s->GetChar(i,&nb))
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '.':
		break;
		default:
			return(false);
		break;
		}
	}
	return(true);
}

#elif defined(WIN32)

static char acctypes[]={"text/html, image/gif, image/x-xbitmap, image/jpeg, image/pjpeg,*/*"};

LPCTSTR  lpacctypes=acctypes;
#define MODE_HTTP INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_AUTO_REDIRECT
#define MODE_HTTPS INTERNET_FLAG_SECURE|INTERNET_FLAG_NO_AUTO_REDIRECT
#else
#error
#endif


void kGUIDownloadEntry::Download(void)
{
#if defined(LINUX)  || defined(MACINTOSH) || defined(MINGW)
	struct sockaddr_in addr ;
	int sock;
	kGUIString request;
	kGUIString got;
	bool findblank;
	int b;
	unsigned int ll;
	int tries;
#elif defined(WIN32)
	CInternetSession *session=0;	//("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.0.8) Gecko/20061025 Firefox/1.5.0.8", 1 /*The context usually 1*/, /*your access type*/PRE_CONFIG_INTERNET_ACCESS);
	CHttpConnection* pServer=0; 
	CHttpFile *pFile=0;
	SYSTEMTIME ptime;
	CString strHeaders = _T("Content-Type: application/x-www-form-urlencoded");
	struct utimbuf ut;
	int rsize;
	time_t tt;
	tm date_tm;
	int tval;
	bool oktime;
#else
#error
#endif
	char filedata[65536];
	const char *cp;
	int mode;
	int useport=80;
	const char *fullurl=m_url.GetString();
	kGUIString servername;
	kGUIString prodname;
	kGUIString url;
	bool ignorepost=false;
	bool wasredirected=false;
	bool wasblocked=false;
	kGUIString referer;
	kGUIString ifmod;
	kGUIString cc;
	kGUIString authorization;
	kGUICookieJar *jar=kGUI::GetCookieJar();
	kGUISSL *ssl=0;

	assert(m_dh->GetDataType()!=DATATYPE_UNDEFINED,"No destination format selected, use SetMemory or SetFilename");

	/* if cookies are not to be sent or accepted then just ignore the jar */
	if(m_allowcookies==false)
		jar=0;

	referer.SetString(&m_referer);
	ifmod.SetString(&m_ifmod);

	m_secure=false;
	m_newurl.Clear();
	m_header.Clear();
	m_expiry.Clear();
	m_lastmod.Clear();
	m_encoding.Clear();
	m_errorpage.Clear();
	m_returncode=0;			/* this will be the error if we can't get a connection at all */
#if DLDEBUG
	m_debugstate=5;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	prodname.Sprintf("Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.11) Gecko/20071127 Firefox/2.0.0.11");
#if defined(LINUX) || defined(MACINTOSH) || defined(MINGW)
#elif defined(WIN32)
	session=new CInternetSession(prodname.GetString(), 1 /*The context usually 1*/, /*your access type*/PRE_CONFIG_INTERNET_ACCESS);
#else
#error
#endif

	if(m_abort)
		goto done;
	m_status=DOWNLOAD_ERROR;
#if DLDEBUG
	m_debugstate=6;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

again:;
	wasredirected=false;
	wasblocked=false;
	if(strncmp(fullurl,"http://",7)==0)
	{
		mode=MODE_HTTP;
		cp=strstr(fullurl+9,"/");
		if(cp)
		{
			url.SetString(cp+1);
			servername.SetString(fullurl+7,(int)(cp-(fullurl+7)));
		}
		else
		{
			cp=strstr(fullurl+9,"?");
			if(cp)
			{
				url.SetString(cp);
				servername.SetString(fullurl+7,(int)(cp-(fullurl+7)));
			}
			else
			{
				servername.SetString(fullurl+7);
				url.Clear();
			}
		}
		useport=80;
	}
	else if(strncmp(fullurl,"https://",8)==0)
	{
		kGUISSLManager *sslmanager=kGUI::GetSSLManager();
		mode=MODE_HTTPS;

		if(!sslmanager)
		{
			//cannot do SSL as there is no SSL handler attached, use 
			//kGUI::SetSSL(); to attach a SLL handler
			goto done;
		}
		ssl=sslmanager->GetSSL();
		cp=strstr(fullurl+10,"/");
		if(cp)
		{
			url.SetString(cp+1);
			servername.SetString(fullurl+8,(int)(cp-(fullurl+8)));
		}
		else
		{
			cp=strstr(fullurl+10,"?");
			if(cp)
			{
				url.SetString(cp);
				servername.SetString(fullurl+8,(int)(cp-(fullurl+8)));
			}
			else
			{
				servername.SetString(fullurl+8);
				url.Clear();
			}
		}
		useport=443;
	}
	else
	{
//		printf("Unknown URL prefix not 'http' or 'https'\n");
		goto done;
	}

//	printf("Opening page '%s' on server '%s'\n",url.GetString(),servername.GetString());

#if DLDEBUG
	m_debugstate=7;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
#if defined(LINUX) || defined(MACINTOSH) || defined(MINGW)
	//printf("aaaa\n");
	memset((void *)&addr,0,sizeof(addr)) ;   /* zero structure */
	addr.sin_family = AF_INET ;
	addr.sin_port = htons(useport);
	if (alldigits(&servername))
	{
	//printf("aaaa.1\n");
		addr.sin_addr.s_addr = inet_addr(servername.GetString()) ;
	}
	else
	{
		/* convert domain name into IP number */
		struct hostent *phostent;

		/* This is NOT threadsafe so we have wrapped it in our own mutex class */
		/* if multiple threads are calling it at the same time then they are all */
		/* fighting over the same static buffer */

		m_gethostmutex.Lock();
		phostent = gethostbyname(servername.GetString()) ;

		if (phostent == NULL)
		{
//			printf("Name lookup for '%s' failed!\n",servername.GetString());
//			fflush(stdout);
			m_gethostmutex.UnLock();
			goto done;
		}
		addr.sin_addr = *((struct in_addr *)phostent->h_addr_list[0]) ;
		m_gethostmutex.UnLock();
	//printf("aaaa.2\n");
	}

	// secure mode is not yet implemented
	if(mode==MODE_HTTPS)
	{
		kGUIString ips;

		//SSL wants an IP address in format n.n.n.n
		ips.SetString(inet_ntoa(addr.sin_addr));
		if(ssl->Connect(&ips)==false)
			goto done;
		m_secure=true;
	}
	else
	{
		//printf("bbbb\n");
		/* get a socket */
		sock=(int)socket(AF_INET, SOCK_STREAM,0);
		if(sock<0)
		{
			//printf("Socket failed!\n");
			goto done;/* socket failed */
		}
		//printf("cccc\n");

		/* try a few times before giving up */
		tries=2;
		do
		{
			if(connect(sock,(struct sockaddr *)&addr,sizeof(addr))>=0)
				break;	/* connected! */

			if(m_abort)
				goto done;
			if(!tries)
			{
				printf("Connect failed sock=%d, addr=%08x!\n",(int)sock,(int)addr.sin_addr.s_addr);
				goto done;/* connect failed */
			}
			/* sleep a bit and then try again */
			kGUI::Sleep(1);
			--tries;
		}while(1);
	}

	//printf("Connect ok!\n");
	/* build the string */

//	printf("Building request for url '%s'\n",url.GetString());
//	fflush(stdout);

	request.Clear();
	if(m_post.GetLen() && ignorepost==false)
		request.Append("POST /");
	else
		request.Append("GET /");
	request.Append(&url);

//	printf("Building 1\n");
//	fflush(stdout);

	request.Append(" HTTP/1.0\r\nAccept: */*\r\n");
	/* add more header stuff here */
	request.ASprintf("User-Agent: %s\r\n",prodname.GetString());
	request.ASprintf("Host: %s\r\n",servername.GetString());
	request.ASprintf("Referer: %s\r\n",referer.GetString());

	/* do we have this in the expired cache ? */
	if(ifmod.GetLen())
	{
		request.ASprintf("If-Modified-Since: %s\r\n",ifmod.GetString());
	}

	/* is there an authenticate name/password pair? (base64encoded) */
	if(authorization.GetLen())
		request.ASprintf("Authorization: Basic %s\r\n",authorization.GetString());

//	printf("Building 2\n");
//	fflush(stdout);

	if(jar)
	{
		jar->GetCookies(&cc,&servername,&url,mode==MODE_HTTPS);
		if(cc.GetLen())
			request.ASprintf("%s\r\n",cc.GetString());
	}
//	printf("Building 3\n");
//	fflush(stdout);

	if(m_post.GetLen() && ignorepost==false)
	{
		request.ASprintf("Content-type: application/x-www-form-urlencoded\r\n");
		request.ASprintf("Content-length: %d\r\n",m_post.GetLen());
	}
//	printf("Building 4\n");
//	fflush(stdout);

	request.Append("\r\n");
	//printf("request='%s'\n",request.GetString());
	if(mode==MODE_HTTPS)
	{
		if(ssl->Write(request.GetString(),request.GetLen())==false)
			goto done;
	}
	else
	{
#if defined(MINGW)
		if(send(sock,request.GetString(),request.GetLen(),0)!=(int)request.GetLen())
#elif defined(LINUX) || defined(MACINTOSH)
		if(write(sock,request.GetString(),request.GetLen())!=(int)request.GetLen())
#else
#error
#endif
		{
			//printf("HTTP Write error!\n");
			goto done;
		}
	}

	/* send post data too if applicable */
	if(m_post.GetLen() && ignorepost==false)
	{
		if(mode==MODE_HTTPS)
		{
			if(ssl->Write(m_post.GetString(),m_post.GetLen())==false)
				goto done;
		}
		else
		{
#if defined(MINGW)
			if(send(sock,m_post.GetString(),m_post.GetLen(),0)!=(int)m_post.GetLen())
#elif defined(LINUX) || defined(MACINTOSH)
			if(write(sock,m_post.GetString(),m_post.GetLen())!=(int)m_post.GetLen())
#else
#error
#endif
			{
				//printf("HTTP Post Write error!\n");
				//fflush(stdout);
				goto done;
			}
		}
	}

	/* header string is the loaded header and data */
	got.Clear();
	do
	{
		if(mode==MODE_HTTPS)
		{
			b=ssl->Read(filedata,sizeof(filedata));
			if(b<0)
				break;
		}
		else
		{
#if defined(MINGW)
			b=recv(sock,filedata,sizeof(filedata),0);
			if(b<=0)
				break;
#elif defined(LINUX) || defined(MACINTOSH)
			b=read(sock,filedata,sizeof(filedata));
			if(b<=0)
				break;
#else
#error
#endif
		}
		got.Append(filedata,b);
	}while(1);
	//printf("got='%s'\n",got.GetString());

	/* find end of header */
	if(strncmp(got.GetString(),"HTTP",4))
	{
		printf("bad header returned! '%s'\n",got.GetString());
		goto done;
	}
	cp=strchr(got.GetString(),' ');
	if(!cp)
	{
		printf("bad header returned, cannot find return code!\n");
		goto done;
	}
	m_returncode=atoi(cp);

	/* find blank line between the header and the body */
	b=0;
	ll=0;
	findblank=false;
	do
	{
		switch(got.GetChar(b++))
		{
		case '\r':
		break;
		case '\n':
			if(!ll)
				findblank=true;
			ll=0;
		break;
		default:
			++ll;
		break;
		}
	}while(findblank==false && b<(int)got.GetLen());
	//printf("b=%d\n",b);
	m_header.SetString(got.GetString(),b);

	/* process returned data */
	if(jar)
	{
		unsigned int hoffset;

		hoffset=0;
		while(ExtractFromHeader("Set-Cookie: ",&cc,&hoffset))
			jar->SetCookie(&cc,&servername,&url);
	}

	switch(m_returncode)
	{
	case 200:
		ExtractFromHeader("Last-Modified: ",&m_lastmod);
		ExtractFromHeader("Content-Type: ",&m_encoding);
		if(!ExtractFromHeader("Expires: ",&m_expiry))
		{
			int seconds;
			kGUIDate now;

			seconds=24*60*60;	//expire in 24 hours if no expiry info found
			if(ExtractFromHeader("Cache-Control: ",&m_expiry))
			{
				/* convert to an expiry */
				if(ExtractFromHeader("max-age=",&cc))
					seconds=cc.GetInt();
			}
			now.SetToday();
			now.AddSeconds(seconds);
			now.ShortDateTime(&m_expiry);
		}

		if(m_dh->OpenWrite("wb",50*1024)==true)
		{
			/* write data starting after the header */
			m_dh->Write(got.GetString()+b,got.GetLen()-b);
			if(m_dh->Close()==true)
				m_status=DOWNLOAD_OK;
		}
		if(m_status!=DOWNLOAD_OK)
		{
			printf("Error saving output!\n");
		}
	break;
	case 304:	/* this is the result code if we send an if-modified-since and the file */
				/* has not been changed */
		m_status=DOWNLOAD_SAME;
		ExtractFromHeader("Content-Encoding: ",&m_encoding);
	break;
	case 301:
	case 302:
	case 307:
	{
		kGUIString shorturl;
		kGUIString longurl;

		if(ExtractFromHeader("Location: ",&shorturl))
		{
			/* convert from possible local URL to global URL */
			/* has this been redirected multiple times? */
			if(m_newurl.GetLen())
				kGUI::MakeURL(&m_newurl,&shorturl,&longurl);
			else
				kGUI::MakeURL(&m_url,&shorturl,&longurl);

			m_newurl.SetString(&longurl);
			fullurl=m_newurl.GetString();
			wasredirected=true;
			ignorepost=true;
		}
	}
	break;
	case 401:
		/* if we have not already tried, and there is an authenticate handler attached */
		if(!authorization.GetLen() && m_ah)
		{
			kGUIString *s;
			kGUIString aa;
			kGUIString realm;
			kGUIString drealm;
			const char *cp;

			//WWW-Authenticate: Basic realm="File Download Authorization"
			ExtractFromHeader("WWW-Authenticate:",&aa);
			cp=strstri(aa.GetString(),"realm=");
			if(cp)
			{
				realm.SetString(cp+6);
				realm.Trim(TRIM_SPACE|TRIM_TAB|TRIM_CR|TRIM_QUOTES);

				drealm.Sprintf("%s:%s",servername.GetString(),realm.GetString());
				s=m_ah->Find(&drealm);
				if(s)
				{
					authorization.SetString(s);
					wasblocked=true;
				}
			}
		}
	case 403:
		/* no idea why, but if we have a referrer tag then try again without one */
		if(referer.GetLen())
		{
			referer.Clear();
			wasblocked=true;		/* set the try again flag */
		}
		else
			goto rcerror;
	break;
	default:
rcerror:
		m_errorpage.SetString(got.GetString()+b,got.GetLen()-b);
//		printf("Error code=%d\n",m_returncode);
	break;
	}
#elif defined(WIN32)
	TRY
	{
		pServer = session->GetHttpConnection(servername.GetString(), (INTERNET_PORT)useport);
		if(m_abort)
			goto done;
#if DLDEBUG
		m_debugstate=8;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

//testing only!
		if(jar)
			jar->GetCookies(&cc,&servername,&url);

		/* if we have been redirected then don't post */
		if(m_post.GetLen() && ignorepost==false)
		{
			pFile = pServer->OpenRequest(
				CHttpConnection::HTTP_VERB_POST /*used if you want to get(read) a file */, 
				url.GetString() /*The File name to read*/, 
				referer.GetString(),
				1 /*The context usually 1*/,
				&lpacctypes, 
				NULL,
				mode);

			//need to send this when posting to form or ASP.net stuff DNW!
			pFile->AddRequestHeaders(strHeaders);

			/* send the post data too */
			pFile->SendRequestEx(m_post.GetLen(),HSR_INITIATE,1);
			pFile->Write((LPCTSTR)m_post.GetString(),m_post.GetLen());
			pFile->EndRequest();
		}
		else
		{
			pFile = pServer->OpenRequest(
				CHttpConnection::HTTP_VERB_GET /*used if you want to get(read) a file */, 
				url.GetString() /*The File name to read*/, 
				referer.GetString() /* the referer */,
				1 /*The context usually 1*/, 
				&lpacctypes /*Context types */,
				NULL,
				mode);

			if(m_abort)
				goto done;

#if DLDEBUG
			m_debugstate=9;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif


			pFile->SendRequest();
			if(m_abort)
				goto done;
#if DLDEBUG
			m_debugstate=10;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
		}
		
		pFile->QueryInfoStatusCode(m_returncode);		
		if(m_abort)
			goto done;
#if DLDEBUG
		m_debugstate=11;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

		{
			CString strHeader;
			pFile->QueryInfo( HTTP_QUERY_RAW_HEADERS_CRLF, strHeader );
			m_header.SetString(( LPCTSTR ) strHeader);
		}

		if(jar)
		{
			if(ExtractFromHeader("Set-Cookie: ",&cc))
			{
				unsigned int hoffset;

				hoffset=0;
				while(ExtractFromHeader("Set-Cookie: ",&cc,&hoffset))
					jar->SetCookie(&cc,&servername,&url);
			}
		}

		switch(m_returncode)
		{
		case HTTP_STATUS_FORBIDDEN:
			/* no idea why, but if we have a referrer tag then try again without one */
			if(referer.GetLen())
			{
				referer.Clear();
				wasblocked=true;		/* set the try again flag */
			}
			else
				goto rcerror;
		break;	
		case HTTP_STATUS_MOVED:
		case HTTP_STATUS_REDIRECT:
		{
			kGUIString shorturl;
			kGUIString longurl;

			if(ExtractFromHeader("Location: ",&shorturl))
			{
				/* convert from possible local URL to global URL */
				/* has this been redirected multiple times? */
				if(m_newurl.GetLen())
					kGUI::MakeURL(&m_newurl,&shorturl,&longurl);
				else
					kGUI::MakeURL(&m_url,&shorturl,&longurl);

				m_newurl.SetString(&longurl);
				fullurl=m_newurl.GetString();
				wasredirected=true;
				ignorepost=true;
			}
		}
		break;
		case HTTP_STATUS_OK:
			ExtractFromHeader("Last-Modified: ",&m_lastmod);
			ExtractFromHeader("Content-Type: ",&m_encoding);
			if(!ExtractFromHeader("Expires: ",&m_expiry))
			{
				int seconds;
				kGUIDate now;

				seconds=24*60*60;	//expire in 24 hours if no expiry info found
				if(ExtractFromHeader("Cache-Control: ",&m_expiry))
				{
					/* convert to an expiry */
					if(ExtractFromHeader("max-age=",&cc))
						seconds=cc.GetInt();
				}
				now.SetToday();
				now.AddSeconds(seconds);
				now.ShortDateTime(&m_expiry);
			}
//			oktime=ExtractFromHeader("Last-Modified: ",&modtime);

			if(pFile->QueryInfo(HTTP_QUERY_LAST_MODIFIED, &ptime) == 0 )
				oktime=false;
			else
			{
				oktime=true;
			}

#if DLDEBUG
			m_debugstate=12;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

			if(m_dh->OpenWrite("wb",50*1024)==true)
			{
#if DLDEBUG
				m_debugstate=13;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
				while(rsize=pFile->Read(filedata,sizeof(filedata)))
				{
					if(m_abort)
					{
						m_dh->Close(true);
						goto done;
					}
					m_dh->Write(filedata,rsize);
				}
#if DLDEBUG
			m_debugstate=14;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
				if(m_dh->Close()==true)
					m_status=DOWNLOAD_OK;
#if DLDEBUG
			m_debugstate=15;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
			break;
			default:
rcerror:		m_errorpage.SetString(got.GetString()+b,got.GetLen()-b);
			break;
			}
			if(m_status==DOWNLOAD_OK)
			{
				if(oktime==true)
				{
					date_tm.tm_hour  =ptime.wHour;
					date_tm.tm_min   =ptime.wMinute;
					date_tm.tm_mon   =ptime.wMonth-1;
					date_tm.tm_sec   =ptime.wSecond;
					date_tm.tm_wday  =0; //Day of week (0-6; Sunday = 0)
					date_tm.tm_yday  =0;
					date_tm.tm_year  =ptime.wYear-1900;
					date_tm.tm_isdst =-1; //Positive if Daylight Saving Time is in effect;
										//0 if Daylight Saving Time is not in effect; 
										//Negative if status of DST is unknown.

					date_tm.tm_mday   =ptime.wDay;

#if DLDEBUG
					m_debugstate=16;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
					tt = mktime(&date_tm);
					ut.actime=tt;
					ut.modtime=tt;
					if(m_dh->GetDataType()==DATATYPE_FILE)
						tval=utime(m_dh->GetFilename()->GetString(),&ut);
#if DLDEBUG
					m_debugstate=17;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
				}
			}
		break;
		}
		goto done;
	}
	CATCH (CInternetException, x)
    {
		//int err=GetLastError();
		char errormessage[4096];
		UINT errormessagesize=4096;
		UINT index=0;

#if DLDEBUG
		m_debugstate=24;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
		x->GetErrorMessage(errormessage,errormessagesize,&index);
#if DLDEBUG
		m_debugstate=25;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
		if(m_dh->GetOpen())
			m_dh->Close(true);
		/* error, of some sort */
		goto done;
    }
	END_CATCH
#else
#error
#endif

done:;
#if defined(MINGW) || defined(LINUX) || defined(MACINTOSH)
	//close sock???

	if(ssl)
	{
		if(ssl->IsOpen())
			ssl->Close();
		delete ssl;
	}
#elif defined(WIN32)
	if(pFile)
	{
		pFile->Close();
		delete pFile;
		pFile=0;
	}
	if(pServer)
	{
		pServer->Close();
		delete pServer;
		pServer=0;
	}
#else
#error
#endif
	if((wasblocked==true || wasredirected==true) && m_abort==false)
	{
		if(wasredirected==true)
			ifmod.Clear();	/* if we are being redirected then ifmod should be aborted */
		goto again;
	}

#if defined(MINGW) || defined(LINUX) || defined(MACINTOSH)
#elif defined(WIN32)
	if(session)
	{
		session->Close();
		delete session;
	}
#else
#error
#endif

#if DLDEBUG
	m_debugstate=32;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

	/* the done callback can't re-issue a download right away since the thread */
	/* has not been killed yet so it must be deferred till later */
	if(m_donecallback.IsValid())
	{
#if DLDEBUG
		m_debugstate=35;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
		do
		{
			if(m_abort==true)
			{
				m_donecallback.Set(0,0);
				break;
			}
			else
			{
				if(kGUI::TryAccess()==true)
				{
#if DLDEBUG
					m_debugstate=36;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
					m_donecallback.Call(m_status);
#if DLDEBUG
					m_debugstate=37;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
					m_donecallback.Set(0,0);
#if DLDEBUG
					m_debugstate=38;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
					kGUI::ReleaseAccess();
#if DLDEBUG
					m_debugstate=39;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
					break;
				}
			}
		}while(1);
	}
#if DLDEBUG
	m_debugstate=40;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	m_asyncactive=false;
#if DLDEBUG
	m_debugstate=41;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif

	//printf("Download almost finished!");
	if(m_thread.GetActive())
		m_thread.Close(false);
#if DLDEBUG
	m_debugstate=42;
	printf("m_debugstate=%d\n",m_debugstate);
	fflush(stdout);
#endif
	//printf("Download finished!");
}

/* extract string from header, false=not found */

/* this is the entry point for finsing multiple instances of the string in the header */
/* for example, set-cookie can appear multiple times */

bool kGUIDownloadEntry::ExtractFromHeader(const char *prefix,kGUIString *s,unsigned int *poffset)
{
	char *l;
	char *le;
	unsigned int offset;

	if(poffset)
		offset=*(poffset);
	else
		offset=0;

	l=strstri ( m_header.GetString()+offset, prefix );
	if(!l)
	{
		s->Clear();
		return(false);
	}
	/* skip 'prefix:' */
	l+=strlen(prefix);
	le=l;
	while(le[0]>=' ')
		++le;

	s->SetString(l,(int)(le-l));

	/* return offset to just past end of command */
	if(poffset)
		*(poffset)=(unsigned int)(le-m_header.GetString());
	return(true);
}

kGUIString *kGUIDownloadAuthenticateRealms::Find(kGUIString *domrealm)
{
	void *v;

	v=m_hash.Find(domrealm->GetString());
	if(!v)
		return(0);	/* this domain / realm pair is not the hash table */

	/* return pointer to string (base64 encoded) that contains the 'name:password' string */
	return(*((kGUIString **)v));
}

/* add this password for the domain/realm pair */
void kGUIDownloadAuthenticateRealms::Add(kGUIString *domrealm,kGUIString *encnp)
{
	kGUIString *s;

	s=Find(domrealm);
	if(s)
		s->SetString(encnp);	/* already was there so just update it */
	else
	{
		/* allocate a string for the encoded name:password */
		s=m_enc.GetEntryPtr(m_num++);
		m_hash.Add(domrealm->GetString(),&s);
	}
}
