/**********************************************************************************/
/* kGUI - kguimutex.cpp                                                           */
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

/**********************************************************************************/
/*                                                                                */
/* This is a mutex class and is NOT re-entrant from within the same thread.       */
/* It is used internally by KGUI to make sure that the Draw and UpdateInput       */
/* is never re-entered, it is also used by the AsyncDownload code to fix          */
/* possible re-entry problems with DNS lookups.                                   */
/*                                                                                */
/* It is needed to manually stop same thread re-entry as in some cases Windows    */
/* can try and re-enter the Screen Update code from within the same thread.       */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"

#if defined(LINUX) || defined(MACINTOSH)
#include <pthread.h>
#elif defined(WIN32) || defined(MINGW)
#else
#error
#endif

kGUIMutex::kGUIMutex()
{
	m_locked=false;
#if defined(WIN32) || defined(MINGW)
	m_mutex = CreateMutex(0, FALSE, 0);
	assert(m_mutex!=0,"Error, couldn't create MUTEX!");
#elif defined(LINUX) || defined(MACINTOSH)
	int rc;

	pthread_mutexattr_init (&m_attr);
	rc=pthread_mutex_init(&m_mutex, &m_attr);
	assert(rc==0,"Error, couldn't create MUTEX!");
#else
#error
#endif
}

kGUIMutex::~kGUIMutex()
{
#if defined(WIN32) || defined(MINGW)
	while(WaitForSingleObject(m_mutex, (DWORD)0)==WAIT_TIMEOUT );
	CloseHandle(m_mutex);
#elif defined(LINUX) || defined(MACINTOSH)
	while(pthread_mutex_destroy(&m_mutex)!=0);
#else
#error
#endif
}

/* waits forever until it can get access */
void kGUIMutex::Lock(void)
{
#if defined(WIN32) || defined(MINGW)
	while ((WaitForSingleObject(m_mutex, INFINITE)) == WAIT_FAILED);
#elif defined(LINUX) || defined(MACINTOSH)
	while (pthread_mutex_lock(&m_mutex));
#else
#error
#endif
	assert(m_locked==false,"Mutex error, already locked?");
	m_locked=true;
}

/* true=got lock, false=didn't */
bool kGUIMutex::TryLock(void)
{
	if(m_locked)
		return(false);
#if defined(WIN32) || defined(MINGW)
	int status;

	status=WaitForSingleObject(m_mutex, 10);
	if(status==WAIT_ABANDONED || status==WAIT_TIMEOUT)
		return(false);
#elif defined(LINUX) || defined(MACINTOSH)
	if(pthread_mutex_lock(&m_mutex)!=0)
		return(false);
#else
#error
#endif
	assert(m_locked==false,"Mutex error, already locked?");
	m_locked=true;
	return(true);
}

void kGUIMutex::UnLock(void)
{
	assert(m_locked==true,"Mutex error, already unlocked?");
	m_locked=false;
#if defined(WIN32) || defined(MINGW)
	while(!ReleaseMutex(m_mutex));
#elif defined(LINUX) || defined(MACINTOSH)
	while (pthread_mutex_unlock(&m_mutex));
#else
#error
#endif
}
