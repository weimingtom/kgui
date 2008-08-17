#ifndef __KGUITHREAD__
#define __KGUITHREAD__

#if defined(LINUX) || defined(MACINTOSH)
#include <pthread.h>
#include <signal.h>
#elif defined(MINGW)
#include <Wincon.h>
#elif defined(WIN32)
#else
#error
#endif

class kGUIThread
{
public:
	kGUIThread() {m_active=false;}
	void Start(void *codeobj,void (*code)(void *));
	void Close(bool now);
	volatile bool GetActive(void) {return m_active;}
private:
	volatile bool m_active;
#if defined(LINUX) || defined(MACINTOSH)
	pthread_t m_thread;
#elif defined(WIN32) || defined(MINGW)
	HANDLE m_thread;		/* only used for async */
#else
#error
#endif
};

/* call a program and send input or grab it's output */

enum
{
CALLTHREAD_READ,
CALLTHREAD_WRITE
};

#define CALLTHREADUSEFORK 1

class kGUICallThread
{
public:
	kGUICallThread();
	~kGUICallThread();
	void SetUpdateCallback(void *codeobj,void (*code)(void *)) {m_updatecallback.Set(codeobj,code);}
	bool Start(const char *line,int mode);
	void Stop(void);
	void SetString(kGUIString *s) {m_string.SetString(s);}
	kGUIString *GetString(void) {return &m_string;}
	volatile bool GetActive(void) {return m_active;}
private:
	kGUICallBack m_updatecallback;
	kGUIString m_string;
#if defined(LINUX) || defined(MACINTOSH)
	FILE *m_handle;
#if CALLTHREADUSEFORK
	long m_tid;
	int m_p[2];
	kGUIString m_sl;
	kGUIStringSplit m_ss;
	char **m_args;
#endif
#elif defined(WIN32) || defined(MINGW)
	PROCESS_INFORMATION m_pi; 
#else
#error
#endif
	volatile bool m_active:1;
	volatile bool m_closing:1;
};


#endif

