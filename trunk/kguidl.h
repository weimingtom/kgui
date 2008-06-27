#ifndef __KGUIDL__
#define __KGUIDL__

#include "kguithread.h"

enum
{
DOWNLOAD_ERROR,
DOWNLOAD_SAME,
DOWNLOAD_OK
};

/* realm handler class for pages that need authentication */
/* add passes in "domain:realm" and base64 encoded name/password */

class kGUIDownloadAuthenticateRealms
{
public:
	kGUIDownloadAuthenticateRealms() {m_num=0;m_hash.Init(6,sizeof(kGUIString *));m_enc.Init(4,4);}
	kGUIString *Find(kGUIString *domrealm);
	void Add(kGUIString *domrealm,kGUIString *encnp);
private:
	Hash m_hash;
	unsigned int m_num;
	ClassArray<kGUIString>m_enc;
};


/* synchronous and async download in one class! (using threads)*/

#define DLDEBUG 0

class kGUIDownloadEntry
{
public:
	kGUIDownloadEntry();
	~kGUIDownloadEntry();
	int DownLoad(DataHandle *dh,kGUIString *fullurl);
	void SetPostData(kGUIString *post) {m_post.SetString(post);}
	void SetAuthHandler(kGUIDownloadAuthenticateRealms *ah) {m_ah=ah;}
	void AsyncDownLoad(DataHandle *dh,kGUIString *fullurl);
	void AsyncDownLoad(DataHandle *dh,kGUIString *fullurl,void *codeobj,void (*code)(void *,int status));
	void Abort(void) {if(m_asyncactive==true)m_abort=true;}
	int GetStatus(void) {return m_status;}
	bool GetAsyncActive(void) {return m_asyncactive;}
	void SetURL(kGUIString *url) {m_url.SetString(url);}
	void SetIfMod(kGUIString *ifmod) {ifmod==0?m_ifmod.Clear():m_ifmod.SetString(ifmod);}
	kGUIString *GetURL(void) {return &m_url;}
	kGUIString *GetExpiry(void) {return &m_expiry;}
	kGUIString *GetLastModified(void) {return &m_lastmod;}
	kGUIString *GetEncoding(void) {return &m_encoding;}
	kGUIString *GetHeader(void) {return &m_header;}
	kGUIString *GetErrorPage(void) {return &m_errorpage;}
	unsigned int GetReturnCode(void) {return m_returncode;}
	bool GetSecure(void) {return m_secure;}
	void WaitFinished(void);
	/* this needs to be called before download */
	void SetReferer(const char *s){m_referer.SetString(s);}
	void SetReferer(kGUIString *s){m_referer.SetString(s);}
	void SetRedirectURL(kGUIString *s) {if(s)m_newurl.SetString(s);else m_newurl.Clear();}
	kGUIString *GetRedirectURL(void) {return &m_newurl;}
	CALLBACKGLUE(kGUIDownloadEntry,Download)
	bool ExtractFromHeader(const char *prefix,kGUIString *s,unsigned int *poffset=0);
	void SetAllowCookies(bool a) {m_allowcookies=a;}
private:
	void Download(void);	/* calls donecallback when finished */
	DataHandle *m_dh;
	kGUIThread m_thread;	/* only used for async */
#if DLDEBUG
	int m_debugstate;
#endif
	static kGUIMutex m_gethostmutex;
	volatile bool m_asyncactive;
	volatile bool m_abort;			/* async download aborted */
	bool m_allowcookies;
	kGUIString m_referer;
	kGUIString m_url;
	kGUIString m_newurl;			/* set if redirect was triggered, can be full or relative url */
	kGUIString m_post;
	kGUIString m_header;
	kGUIString m_expiry;
	kGUIString m_lastmod;
	kGUIString m_ifmod;
	kGUIString m_encoding;
	bool m_secure:1;
	unsigned int m_returncode;
	kGUIString m_errorpage;
	kGUICallBackInt m_donecallback;
	int m_status;
	kGUIDownloadAuthenticateRealms *m_ah;
};
#endif
