/**********************************************************************************/
/* kGUI - kguiemail.cpp                                                           */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://www.scale18.com/cgi-bin/page/kgui.html	                              */
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
#include "kguiemail.h"

#if defined(WIN32) 
#include <winsock.h>
#elif defined(LINUX) || defined(MACINTOSH) || defined(MINGW)
#else
#error
#endif

kGUIEmail::kGUIEmail()
{
	m_numcc=0;
	m_numbcc=0;
	m_ccname.SetGrow(true);
	m_ccaddr.SetGrow(true);
	m_bccname.SetGrow(true);
	m_bccaddr.SetGrow(true);
}

kGUIEmail::~kGUIEmail()
{
	int i;
	kGUIText *t;

	for(i=0;i<m_numcc;++i)
	{
		t=m_ccname.GetEntry(i);
		delete t;
		t=m_ccaddr.GetEntry(i);
		delete t;
	}
	for(i=0;i<m_numbcc;++i)
	{
		t=m_bccname.GetEntry(i);
		delete t;
		t=m_bccaddr.GetEntry(i);
		delete t;
	}
}

void kGUIEmail::AddCC(const char *name,const char *addr)
{
	kGUIText *t;

	t=new kGUIText();
	t->SetString(name);
	m_ccname.SetEntry(m_numcc,t);
	t->SetString(addr);
	m_ccaddr.SetEntry(m_numcc,t);
	++m_numcc;
}

void kGUIEmail::AddBCC(const char *name,const char *addr)
{
	kGUIText *t;

	t=new kGUIText();
	t->SetString(name);
	m_bccname.SetEntry(m_numbcc,t);
	t->SetString(addr);
	m_bccaddr.SetEntry(m_numbcc,t);
	++m_numbcc;
}

/* todo, change data to kGUIString and change send to send a string */

bool kGUIEmail::Send(void)
{
#if defined(WIN32)
    LPHOSTENT mHost;
    char *Buffer = new char[500];
    char Data[500];

    SOCKET Client = socket(AF_INET,SOCK_STREAM,0);
    if(Client == INVALID_SOCKET)  
            return false;

    mHost = gethostbyname(m_servername.GetString());
    SOCKADDR_IN mServerInfo; 
    mServerInfo.sin_family = AF_INET;      
    mServerInfo.sin_port = htons(25);   
    mServerInfo.sin_addr = *((LPIN_ADDR)*mHost->h_addr_list);

    if (connect(Client,(LPSOCKADDR)&mServerInfo,sizeof(mServerInfo))==SOCKET_ERROR)
        return false;
    ZeroMemory(Buffer,500);
    int rt = recv(Client,Buffer,500,0);
    if (rt == -1)
    {
        delete [] Buffer;
        return false;
    }
    strcpy(Data,"HELO MAIN \r\n");
    int sr = send(Client,Data,(int)strlen(Data),0);
    if (sr == -1)
    {
        delete [] Buffer;
        return false;
    }
    ZeroMemory(Buffer,500);
    rt = recv(Client,Buffer,100,0);

	sprintf(Data,"MAIL FROM: <%s>\r\n",/*m_fromname.GetString(), */m_fromaddr.GetString());
    send(Client,Data,(int)strlen(Data),0);

	recv(Client,Buffer,100,0);
	if(atoi(Buffer)!=250)
	{
        delete [] Buffer;
        return false;
	}

	sprintf(Data,"RCPT TO: <%s>\r\n",m_toaddr.GetString());
	send(Client,Data,(int)strlen(Data),0);

	rt = recv(Client,Buffer,100,0);
	if(atoi(Buffer)!=250)
	{
        delete [] Buffer;
        return false;
	}

	strcpy(Data,"DATA\r\n");
    send(Client,Data,(int)strlen(Data),0);

	rt = recv(Client,Buffer,100,0);

	sprintf(Data,"From: %s <%s>\r\n",m_fromname.GetString(),m_fromaddr.GetString());
    send(Client,Data,(int)strlen(Data),0);

	sprintf(Data,"To: %s <%s>\r\n",m_toname.GetString(),m_toaddr.GetString());
    send(Client,Data,(int)strlen(Data),0);

	sprintf(Data,"Subject: %s\r\n",m_subject.GetString());
    send(Client,Data,(int)strlen(Data),0);

    send(Client,m_body.GetString(),m_body.GetLen(),0);

	strcpy(Data,"\r\n.\r\n");
    send(Client,Data,(int)strlen(Data),0);

	rt = recv(Client,Buffer,100,0);
    strcpy(Data,"QUIT \r\n");
    send(Client,Data,(int)strlen(Data),0);
    rt = recv(Client,Buffer,100,0);
    delete [] Buffer;
#elif defined(LINUX) || defined(MACINTOSH) || defined(MINGW)
	assert(false,"TODO:GUIEmail::Send\n");
#else
#error
#endif
	return(true);
}
