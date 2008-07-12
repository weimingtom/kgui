#ifndef __KGUICOOKIES__
#define __KGUICOOKIES__

#include "kguixml.h"

/*! @class kGUICookie
	@brief this class is for holding cookies
    @ingroup Online */
class kGUICookie : public Links<kGUICookie>
{
public:
	kGUICookie();
	~kGUICookie();

	void SetAttribute(unsigned int att,kGUIString *value,kGUIString *domain,kGUIString *subdomain,kGUIString *url);

	void SetName(kGUIString *name) {m_name.SetString(name);}
	kGUIString *GetName(void) {return &m_name;}

	void SetValue(kGUIString *value) {m_value.SetString(value);}
	kGUIString *GetValue(void) {return &m_value;}

	void SetDomain(kGUIString *domain) {m_domain.SetString(domain);}
	kGUIString *GetDomain(void) {return &m_domain;}

	void SetPath(kGUIString *path) {m_path.SetString(path);}
	kGUIString *GetPath(void) {return &m_path;}

	void SetExpiry(kGUIString *expiry) {m_expiry.Set(expiry->GetString());}
	kGUIDate *GetExpiry(void) {return &m_expiry;}

	void SetPort(int port) {m_port=port;}
	int GetPort(void) {return m_port;}

	void SetRemove(bool r) {m_remove=r;}
	bool GetRemove(void) {return m_remove;}

	void SetSecure(bool s) {m_secure=s;}
	bool GetSecure(void) {return m_secure;}

	void SetPermanent(bool p) {m_permanent=p;}
	bool GetPermanent(void) {return m_permanent;}
private:
	bool Set(kGUIString *line);
	kGUIString m_name;
	kGUIString m_value;
	kGUIString m_domain;
	int m_port;
	kGUIString m_path;
	bool m_secure:1;
	bool m_remove:1;
	bool m_permanent:1;
	kGUIDate m_expiry;
};

/*! @class kGUICookieDomain
	@brief this class is for handling a domains cookies
    @ingroup Online */
class kGUICookieDomain
{
public:
	kGUICookieDomain() {}
	~kGUICookieDomain();
	kGUICookie *GetFirst(void) {return m_links.GetFirst();}
	kGUICookie *GetTail(void) {return m_links.GetTail();}
private:;
	//linked list of cookies attached to this domain, sorted so longer paths are first
	LinkEnds<kGUICookie,kGUICookie> m_links;
};

/*! @class kGUICookieJar
	@brief this class is for handling all the domains and their cookies
    @ingroup Online */
class kGUICookieJar
{
public:
	kGUICookieJar();
	~kGUICookieJar();
	void Load(kGUIXMLItem *root);
	void Save(kGUIXMLItem *root);
	void SetCookie(kGUIString *s,kGUIString *domain,kGUIString *url);
	void UpdateCookie(kGUICookie *cookie);
	void GetCookies(kGUIString *s,kGUIString *domain,kGUIString *url,bool issecure);

	unsigned int GetCookieList(Array<kGUICookie *>*cookielist);
private:
	void GetSubDomain(kGUIString *domain,kGUIString *subdomain);
	Hash m_atthash;
	Hash m_domainhash;
	Hash m_cookiehash;
	kGUIMutex m_busymutex;
};

#endif
