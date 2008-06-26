#ifndef __KGUISSL__
#define __KGUISSL__

extern "C" {

#include "matrixssl/MatrixSsl.h"
#include "matrixssl/examples/sslSocket.h"

}

class kGUISSL
{
public:
	kGUISSL(sslKeys_t *keys) {m_keys=keys;m_conn=0;}
	~kGUISSL() {if(m_conn)Close();}
	bool Connect(kGUIString *ip);
	bool Write(const char *buffer,int bytes);
	int Read(char *buffer,int bytes);
	void Close(void);
	bool IsOpen(void) {return m_conn!=0;}
private:
	sslKeys_t	*m_keys;	/* points to managers keys */
	sslConn_t	*m_conn;
};

class kGUISSLManager
{
public:
	kGUISSLManager(const char *cafilename);
	~kGUISSLManager();
	kGUISSL *GetSSL(void);
private:
	sslKeys_t	*m_keys;
};

#endif
