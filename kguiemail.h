#ifndef __KGUIEMAIL__
#define __KGUIEMAIL__

class kGUIEmail
{
public:
	kGUIEmail();
	~kGUIEmail();
	void SetServerName(const char *servername) {m_servername.SetString(servername);}
	void SetFrom(const char *name,const char *addr) {m_fromname.SetString(name);m_fromaddr.SetString(addr);}
	void SetTo(const char *name,const char *addr) {m_toname.SetString(name);m_toaddr.SetString(addr);}
	void AddCC(const char *name,const char *addr);
	void AddBCC(const char *name,const char *addr);
	void SetSubject(const char *subject) {m_subject.SetString(subject);}
	void SetBody(const char *body) {m_body.SetString(body);}
	bool Send(void);
private:
	kGUIText m_servername;
	kGUIText m_fromname;
	kGUIText m_fromaddr;
	kGUIText m_toname;
	kGUIText m_toaddr;
	int m_numcc;
	Array<kGUIText *>m_ccname;
	Array<kGUIText *>m_ccaddr;
	int m_numbcc;
	Array<kGUIText *>m_bccname;
	Array<kGUIText *>m_bccaddr;
	kGUIText m_subject;
	kGUIText m_body;
};
#endif
