/*********************************************************************************/
/* kGUI - kguithread.cpp                                                         */
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
/* This is the thread class wrapper.                                             */
/* It is essentially a wrapper over the system thread class and is only here so  */
/* the application doesn't have to know about the differences between the systems*/
/*                                                                               */
/* Note that if your thread needs to call any of the kGUI functions it MUST first*/
/* call GetAccess ( or TryAccess ) to get exclusivce access to the system        */
/* and then call ReleaseAccess when it is finished.                              */
/*                                                                               */
/* There is also a callthread class for calling external programs                */
/* It also can also if desired send output to the thread or read the output      */
/* from the thread.                                                              */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguithread.h"

void kGUIThread::Start(void *codeobj,void (*code)(void *))
{
	m_active=true;
#if defined(LINUX)
	pthread_create (&m_thread, NULL,(void *(*)(void *))code,codeobj);
#elif defined(MACINTOSH)
	pthread_create (&m_thread, NULL,(void *(*)(void *))code,codeobj);
#else 
	m_thread = CreateThread( 
			NULL,							// default security attributes 
			0,								// use default stack size  
			(LPTHREAD_START_ROUTINE)code,   // thread function 
			codeobj,							// argument to thread function 
			0,								// use default creation flags 
			0);								// returns the thread identifier 
#endif
}

void kGUIThread::Close(bool now)
{
#if defined(LINUX)
	m_active=false;
	if(now)
		pthread_exit(0);
	//thread automatically closes when function returns
#elif defined(MACINTOSH)
	m_active=false;
	if(now)
		pthread_exit(0);
	//thread automatically closes when function returns
#elif defined(WIN32) || defined(MINGW)
//	printf("CloseHandle\n");
//	fflush(stdout);
	CloseHandle( m_thread );
	m_active=false;
//	printf("CloseHandle2\n");
//	fflush(stdout);
#else
#error
#endif
}

kGUICallThread::kGUICallThread()
{
	m_active=false;
}

kGUICallThread::~kGUICallThread()
{

}

bool kGUICallThread::Start(const char *line,int mode)
{
#if defined(LINUX) || defined(MACINTOSH)
#if CALLTHREADUSEFORK
	long tid;

	m_closing=false;
	if (pipe(m_p) < 0)
		return false;

	tid=fork();
	/* if the fork command is unsucessfull then it returns -1 */
	if(tid<0)
		return(false);
	/* if it is sucessfull then it spawns another process and returns 0 to that process */
	/* and it returns the child's process id to the parents process */
	if(!tid)
	{
		/* this is the child process */
		setpgid(0, 0);

	    if (mode==CALLTHREAD_READ)
		{
			fflush(stdout);
			fflush(stderr);
			close(1);
			if (dup(m_p[1]) < 0)
				perror("dup of write side of pipe failed");
			close(2);
			if (dup(m_p[1]) < 0)
				perror("dup of write side of pipe failed");
		}
		else
		{
			close(0);
			if (dup(m_p[0]) < 0)
		        perror("dup of read side of pipe failed");
		}

	    close(m_p[0]); /* close since we dup()'ed what we needed */
	    close(m_p[1]);

		/* split line into word pointers */
		{
			kGUIString sl;
			kGUIStringSplit ss;
			int i,num;
			const char **args;

			sl.SetString(line);
			num=ss.Split(&sl," ",false,true);
			args=new (char *)[num];
			for(i=0;i<num;++i)
				args[i]=ss.GetWord(i)->GetString();

		    execv(args[0], args);
			delete []args;
			/* ok, task is done */
			return(true);
		}
	}
	/* we are the parent process */
	m_tid=tid;
	if (mode==CALLTHREAD_READ)
	{
		close(m_p[1]);
		m_handle = fdopen(m_p[0], "r");
    }
	else
	{
		close(m_p[0]);
		m_handle = fdopen(m_p[1], "w");
    }

    return m_handle;
}
#else
	int status;

	m_closing=false;
	m_handle=popen(line,mode==CALLTHREAD_READ?"r":"w");
	if(!m_handle)
		return(false);
#endif
#elif defined(WIN32) || defined(MINGW)
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr;
	SECURITY_ATTRIBUTES saAttr; 
	STARTUPINFO siStartInfo;
	char c;
	DWORD cl;

// Set the bInheritHandle flag so pipe handles are inherited. 
 
	m_closing=false;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

// Get the handle to the current STDOUT. 
 
// Create a pipe for the child process's STDOUT. 
 
   if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) 
      return(false); 

// Ensure that the read handle to the child process's pipe for STDOUT is not inherited.

   SetHandleInformation( hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

// Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) 
      return(false); 

// Ensure that the write handle to the child process's pipe for STDIN is not inherited. 
 
   SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);
 
// Now create the child process. 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory( &m_pi, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
 
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = hChildStdoutWr;
   siStartInfo.hStdOutput = hChildStdoutWr;
   siStartInfo.hStdInput = hChildStdinRd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    
   if( !CreateProcess(NULL, 
      (LPSTR)line,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      CREATE_NO_WINDOW|CREATE_NEW_PROCESS_GROUP,   // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &m_pi))  // receives PROCESS_INFORMATION 
	     return(false);
 
#else
#error
#endif

   switch(mode)
	{
	case CALLTHREAD_READ:
		m_string.Clear();
#if defined(LINUX) || defined(MACINTOSH)
		m_active=true;
		while (1)
		{
			int c=fgetc(m_handle);

			if(c==EOF)
				break;
			m_string.Append(c);
			/* if callback set then call it now */
			if(m_updatecallback.IsValid())
				m_updatecallback.Call();
		}
		m_active=false;

		status = pclose(m_handle);
		if (status == -1)
			return(false);
#elif defined(WIN32) || defined(MINGW)
	   if (!CloseHandle(hChildStdoutWr)) 
		  return(false); 
 
// Read output from the child process, and write to parent's STDOUT. 
 
		m_active=true;
	   while(ReadFile( hChildStdoutRd, &c, sizeof(c), &cl,NULL))
	   {
		   if(!cl)
			   break;
		   m_string.Append(c);
			/* if callback set then call it now */
			if(m_updatecallback.IsValid())
				m_updatecallback.Call();
	   }
    // Wait until child process exits.
		m_active=false;
	   WaitForSingleObject( m_pi.hProcess, INFINITE );
	CloseHandle(m_pi.hProcess);
	CloseHandle(m_pi.hThread);
#else
#error
#endif
		return(true);
	break;
	case CALLTHREAD_WRITE:
#if defined(LINUX) || defined(MACINTOSH)
		m_active=true;
		if(m_string.GetLen())
			fprintf(m_handle,m_string.GetString());
		status = pclose(m_handle);
		m_active=false;
		if (status == -1)
			return(false);
#elif defined(WIN32) || defined(MINGW)
		/* to do, write to handle */
		assert(false,"Todo: CALLTHREAD_WRITE\n");
#else
#error
#endif
		return(true);
	break;
	}
	return(false);	/* should never get here! */
}

void kGUICallThread::Stop(void)
{
	if((m_closing==false) && (m_active==true))
	{
#if defined(WIN32) || defined(MINGW)
		AttachConsole(m_pi.dwProcessId);
		GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT,m_pi.dwProcessId );
		GenerateConsoleCtrlEvent( CTRL_C_EVENT,m_pi.dwProcessId);
		FreeConsole();
#elif defined(LINUX) || defined(MACINTOSH)
#if CALLTHREADUSEFORK
		kill(m_tid,SIGINT);
#else
		//todo how to send kill to a popened thread
		//		kill(m_handle,SIGINT);
//		kill(m_handle,SIGKILL);
#endif
#else
#error
#endif
		m_closing=true;
	}
}

