#ifndef __KGUIMATRIXSSL__
#define __KGUIMATRIXSSL__

#include "kguissl.h"

extern "C" {

#include "matrixssl/matrixSsl.h"
#include "matrixssl/examples/sslSocket.h"

}

class kGUIMatrixSSL : public kGUISSL
{
public:
	kGUIMatrixSSL(sslKeys_t *keys) {m_keys=keys;m_conn=0;}
	~kGUIMatrixSSL() {if(m_conn)Close();}
	bool Connect(kGUIString *ip);
	bool Write(const char *buffer,int bytes);
	int Read(char *buffer,int bytes);
	void Close(void);
	bool IsOpen(void) {return m_conn!=0;}
private:
	sslKeys_t	*m_keys;	/* points to managers keys */
	sslConn_t	*m_conn;
};

class kGUIMatrixSSLManager : public kGUISSLManager
{
public:
	kGUIMatrixSSLManager(const char *cafilename);
	~kGUIMatrixSSLManager();
	kGUIMatrixSSL *GetSSL(void);
private:
	sslKeys_t	*m_keys;
};

#endif
