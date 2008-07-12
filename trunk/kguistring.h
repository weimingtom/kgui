#ifndef __KGUISTRING__
#define __KGUISTRING__

/* a simple text class */

#define DEFLEN 12

enum
{
ENCODING_8BIT,
ENCODING_UTF8,
};

#define TRIM_SPACE 1
#define TRIM_TAB 2
#define TRIM_CR 4
#define TRIM_QUOTES 8
#define TRIM_NULL 16
#define TRIM_ALL (TRIM_SPACE|TRIM_TAB|TRIM_CR)

class kGUIString
{
	friend class kGUIStringSplit;
public:
	kGUIString() {m_encoding=ENCODING_8BIT;m_len=0;m_defstring[0]=0;m_string=m_defstring;m_maxlen=DEFLEN-1;}
	virtual ~kGUIString() {if(m_string!=m_defstring) delete []m_string;}

	virtual void StringChanged(void) {}

	void Alloc(unsigned int l,bool preserve=false);
	void SetString(const char *t);
	void SetString(const char *t,unsigned int numchars);
	void SetString16(const char *t);
	inline void SetString(kGUIString *s) {SetString(s->GetString(),s->GetLen());SetEncoding(s->GetEncoding());}
	void SetFormattedInt(int value);	/* brackets for neg, and commas */
	inline const char *GetString(void) {return m_string;}
	unsigned int GetChar(unsigned int pos,unsigned int *numbytes);
	inline char GetChar(unsigned int pos) {return pos>=m_len?0:m_string[pos];}
	inline unsigned char GetUChar(unsigned int pos) {return (unsigned char)(pos>=m_len?0:m_string[pos]);}
	unsigned int GoBack(unsigned int pos);
	void SetChar(unsigned int pos,char c);
	void Clip(unsigned int len);
	void Clean(const char *validchars);
	/* get numeric values from string */
	inline int GetInt(void) {return (atoi(m_string));}
	inline double GetDouble(void) {return (atof(m_string));}
	void Proper(bool leaveup=false);
	void Upper(void);
	void Lower(void);
	int Sprintf(const char *fmt,...);
	int ASprintf(const char *fmt,...);
	void AVSprintf(const char *fmt,va_list args);
	inline void Clear(void) {Clip(0);}
	inline unsigned int GetLen(void) {return m_len;}

	inline void SetNull(void) {Clear();}
	inline bool IsNull(void) {return (GetLen()==0);}

	void CopyString(char *dest,unsigned int start,unsigned int len);
	void Delete(unsigned int index,unsigned int num);
	void Insert(unsigned int index,const char *text);
	void Append(char c);
	void Append(unsigned int c);
	void Append(unsigned char c) {Append((char)c);}
	void Append(int c) {Append((unsigned int)c);}
	void Append(const char *atext);
	void Append(const char *atext,unsigned int alen);
	inline void Append(kGUIString *a) {Append(a->GetString(),a->GetLen());}
	int Replace(const char *from,const char *to,unsigned int start=0,int casemode=0,int maxchanges=-1);
	void Trim(int what=TRIM_ALL);
	void TTZ(void);
	inline void RecalcLen(void) {m_len=(int)strlen(m_string);}	/* used if external code changes the string */

	bool RemoveQuotes(void);	/* remove quotes and return true if there was any */

	/* string compare functions */
	int StrStr(kGUIString *ss,unsigned int offset=0);
	int StrIStr(kGUIString *ss,unsigned int offset=0);
	int StrStr(const char *ss,unsigned int offset=0);
	int StrIStr(const char *ss,unsigned int offset=0);

	/* encoding handling */
	unsigned int GetEncoding(void) {return m_encoding;}
	void SetEncoding(unsigned int e) {m_encoding=e;}
	void ChangeEncoding(unsigned int e);
	void CheckBOM(void);	/* look at beginning of string for an Byte-Ordering-Mark ( encoding ) */
	unsigned int CursorToIndex(unsigned int cursor);
	unsigned int IndexToCursor(unsigned int index);

	/* convert encoding format string to number, return 0 if unknown */
	static unsigned int GetEncoding(const char *s);
	static bool IsWhiteSpace(char c) {if(c==' ' || c=='\n' || c=='\r' || c=='\t')return(true);return(false);}
	static bool IsDigit(char c) {if(c>='0' && c<='9')return(true);return(false);}
private:
	unsigned int m_len;			/* current length of string */
	unsigned int m_maxlen;		/* maximum space currently allocated for string */
	unsigned int m_encoding:8;		/* string mode */
	char *m_string;		/* pointer to string */
	char m_defstring[DEFLEN];	
};

class kGUIStringSplit
{
public:
	kGUIStringSplit();
	~kGUIStringSplit() {}
	unsigned int Split(kGUIString *s,const char *splitstring,int casemode=0,bool usequotes=true);
	unsigned int GetNumWords(void) {return m_numwords;};
	kGUIString *GetWord(unsigned int index);
	void SetTrim(bool trim) {m_trim=trim;}
	void SetIgnoreEmpty(bool ignoreempty) {m_ignoreempty=ignoreempty;}
private:
	bool m_trim:1;
	bool m_ignoreempty:1;
	unsigned int m_numwords;
	ClassArray<kGUIString>m_list;
};

/* a string class with a current index and code to read through it from start to end */
class kGUIReadString : public kGUIString
{
public:
	kGUIReadString() {Start();}
	void Start(void) {m_index=0;}
	bool AtEnd(void) {return m_index>=GetLen();}
	unsigned int ReadChar(void) {unsigned int c;unsigned int nb;c=GetChar(m_index,&nb);m_index+=nb;return c;}
	unsigned int PeekChar(void) {unsigned int c;unsigned int nb;c=GetChar(m_index,&nb);return c;}

private:
	unsigned int m_index;
};


#endif
