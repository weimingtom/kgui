#ifndef __KGUISSL__
#define __KGUISSL__

/* this is the base class for the SSL handler */

class kGUISSL
{
public:
	virtual ~kGUISSL() {}
	virtual bool Connect(kGUIString *ip)=0;
	virtual bool Write(const char *buffer,int bytes)=0;
	virtual int Read(char *buffer,int bytes)=0;
	virtual void Close(void)=0;
	virtual bool IsOpen(void)=0;
private:
};

class kGUISSLManager
{
public:
	virtual ~kGUISSLManager() {}
	virtual kGUISSL *GetSSL(void)=0;
private:
};

#endif
