/**********************************************************************************/
/* kGUI - kguissl.cpp                                                             */
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

/* NOTE: This SSL wrapper uses MatrixSSL for the actual SSL processing. */
/* MatrixSSL is GPL'ed but not LGPL'ed so you CANNOT use MatrixSSL in a commercial */
/* product unless you get a commercial license from them directly */

#include "kgui.h"
#include "kguissl.h"

#define HTTPS_PORT	443

extern "C" {
#include "matrixssl/examples/sslSocket.c"
}

//forward declaration
static int certChecker(sslCertInfo_t *cert, void *arg);

kGUIMatrixSSLManager::kGUIMatrixSSLManager(const char *cafilename)
{
	int rc;

	matrixSslOpen();
	if(cafilename)
	{
		rc=matrixSslReadKeys(&m_keys, NULL, NULL, NULL, cafilename);
		assert(rc>=0,"Error reading keys");
	}
}

kGUIMatrixSSL *kGUIMatrixSSLManager::GetSSL(void)
{
	kGUIMatrixSSL *ssl;

	ssl=new kGUIMatrixSSL(m_keys);
	return(ssl);
}

kGUIMatrixSSLManager::~kGUIMatrixSSLManager()
{
	matrixSslFreeKeys(m_keys);
}

bool kGUIMatrixSSL::Connect(kGUIString *ip)
{
	sslSessionId_t		*sessionId=0;
	short cipherSuite = 0x0000;
	SOCKET fd;
	int err;

	/* currently connected? */
	if(m_conn)
		Close();

	if ((fd = socketConnect((char *)ip->GetString(), HTTPS_PORT, &err)) == INVALID_SOCKET)
	{
		//fprintf(stdout, "Error connecting to server %s:%d\n", ip, HTTPS_PORT);
		return(false);
	}

	if (sslConnect(&m_conn, fd, m_keys, sessionId, cipherSuite, certChecker) < 0)
	{
		socketShutdown(fd);
		//fprintf(stderr, "Error connecting to %s:%d\n", ip, HTTPS_PORT);
		return(false);
	}

	return(true);
}

bool kGUIMatrixSSL::Write(const char *buffer,int bytes)
{
	int rc,status;

	assert(m_conn!=0,"No Open Connection!");
	do
	{
		rc = sslWrite(m_conn, (char *)buffer, bytes, &status);
		if (rc < 0)
		{
			//fprintf(stdout, "Internal sslWrite error\n");
			socketShutdown(m_conn->fd);
			sslFreeConnection(&m_conn);
			m_conn=0;
			return(false);
		}
		else if(rc>0)
			return(true);
	}while(1);
}

int kGUIMatrixSSL::Read(char *buffer,int bytes)
{
	int rc,status;

	assert(m_conn!=0,"No Open Connection!");
	rc=sslRead(m_conn,buffer,bytes,&status);
	if ((rc<0) || (status == SSLSOCKET_EOF) || (status == SSLSOCKET_CLOSE_NOTIFY))
	{
		socketShutdown(m_conn->fd);
		sslFreeConnection(&m_conn);
		m_conn=0;
		rc=-1;
	}
	return(rc);
}

#if 0
		if ((rc = sslRead(conn, c, sizeof(buf) - (int)(c - buf), &status)) > 0) {
			c += rc;
			if (c - buf < 4 || memcmp(c - 4, "\r\n\r\n", 4) != 0) {
				goto readMore;
			}
		} else {
			if (rc < 0) {
				fprintf(stdout, "sslRead error.  dropping connection.\n");
			}
			if (rc < 0 || status == SSLSOCKET_EOF ||
					status == SSLSOCKET_CLOSE_NOTIFY) {
				socketShutdown(conn->fd);
				sslFreeConnection(&conn);
				continue;
			}
			goto readMore;
		}
#endif

#if 0
#if REUSE
		matrixSslFreeSessionId(sessionId);
		matrixSslGetSessionId(conn->ssl, &sessionId);
/*
		This example shows how a user might want to limit a client to
		resuming handshakes only with authenticated servers.  In this
		example, the client will force any non-authenticated (anonymous)
		server to go through a complete handshake each time.  This is
		strictly an example of one policy decision an implementation 
		might wish to make.
*/
		matrixSslGetAnonStatus(conn->ssl, &anonStatus);
		if (anonStatus) {
			matrixSslFreeSessionId(sessionId);
			sessionId = NULL;
		}
#endif
/*
		Send a closure alert for clean shutdown of remote SSL connection
		This is for good form, some implementations just close the socket
*/
		sslWriteClosureAlert(conn);
/*
		Session done.  Connect again if more iterations remaining
*/
		socketShutdown(conn->fd);
		sslFreeConnection(&conn);
		connectAgain = 1;
	}
#endif

void kGUIMatrixSSL::Close(void)
{
/*
		Send a closure alert for clean shutdown of remote SSL connection
		This is for good form, some implementations just close the socket
*/
	sslWriteClosureAlert(m_conn);
/*
		Session done.  Connect again if more iterations remaining
*/
	socketShutdown(m_conn->fd);
	sslFreeConnection(&m_conn);
	m_conn=0;
}


static int certChecker(sslCertInfo_t *cert, void *arg)
{
	sslCertInfo_t	*next;
	sslKeys_t		*keys;
/*
	Make sure we are checking the last cert in the chain
*/
	next = cert;
	keys = (sslKeys_t *)arg;
	while (next->next != NULL) {
		next = next->next;
	}
#if ENFORCE_CERT_VALIDATION
/*
	This case passes the true RSA authentication status through
*/
	return next->verified;
#else
/*
	This case passes an authenticated server through, but flags a
	non-authenticated server correctly.  The user can call the
	matrixSslGetAnonStatus later to see the status of this connection.
*/
	if (next->verified != 1) {
		return SSL_ALLOW_ANON_CONNECTION;
	}
	return next->verified;
#endif /* ENFORCE_CERT_VALIDATION */
}		
