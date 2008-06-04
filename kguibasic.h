#ifndef __KGUIBASIC__
#define __KGUIBASIC__

/* hmmmm, change all callbacks to use a single one with multiple union types for the code pointer types? */
/* todo: use vars for labels */
/* assign enums for errors */
/* add string types and double/float types */

#include "kguireq.h"
#include "kguithread.h"

#include "setjmp.h"

enum
{
VARTYPE_COMMAND,
VARTYPE_OPERATOR,
VARTYPE_LABEL,
VARTYPE_CFUNCTION,
VARTYPE_USUB,
VARTYPE_UFUNCTION,
VARTYPE_SYSCLASSFUNC,

VARTYPE_INTEGER,
VARTYPE_DOUBLE,
VARTYPE_STRING,
VARTYPE_BOOLEAN,
VARTYPE_VARIANT,
VARTYPE_STRUCTDEF,
VARTYPE_STRUCT,
VARTYPE_SYSCLASSDEF,
VARTYPE_UCLASSDEF,
VARTYPE_CLASS,
VARTYPE_ENUMDEF,

VARTYPE_IF,
VARTYPE_ELSEIF,
VARTYPE_ELSE,
VARTYPE_ENDIF,
VARTYPE_DO,
VARTYPE_LOOP,
VARTYPE_WHILE,
VARTYPE_WEND,
VARTYPE_FOR,
VARTYPE_NEXT,
VARTYPE_EXIT,
VARTYPE_SELECT,
VARTYPE_CASE,
VARTYPE_CASEELSE,
VARTYPE_ENDSELECT,

VARTYPE_UNDEFINED
};

enum
{
BASICTOKEN_UNKNOWN,
BASICTOKEN_IF,
BASICTOKEN_ELSEIF,
BASICTOKEN_ELSE,
BASICTOKEN_ENDIF,
BASICTOKEN_DO,
BASICTOKEN_LOOP,
BASICTOKEN_WEND,
BASICTOKEN_FOR,
BASICTOKEN_NEXT,
BASICTOKEN_SELECT,
BASICTOKEN_CASE,
BASICTOKEN_PRINT,
BASICTOKEN_INPUT,
BASICTOKEN_INPUTBOX,
BASICTOKEN_GOTO,
BASICTOKEN_GOSUB,
BASICTOKEN_CALL,
BASICTOKEN_OPEN,
BASICTOKEN_CLOSE,
BASICTOKEN_RETURN,
BASICTOKEN_LET,
BASICTOKEN_SET,
BASICTOKEN_DIM,
BASICTOKEN_REDIM,
BASICTOKEN_SUB,
BASICTOKEN_FUNCTION,
BASICTOKEN_EXIT,
BASICTOKEN_TYPE,
BASICTOKEN_ON,
BASICTOKEN_ERROR,
BASICTOKEN_CONST,
BASICTOKEN_CLASS,
BASICTOKEN_WITH,
BASICTOKEN_ENUM,
BASICTOKEN_END,

/* verbs */
BASICTOKEN_TO,
BASICTOKEN_STEP,
BASICTOKEN_THEN,
BASICTOKEN_WHILE,
BASICTOKEN_UNTIL,
BASICTOKEN_AS,
BASICTOKEN_NUMBER,
BASICTOKEN_PUBLIC,
BASICTOKEN_LINE,
BASICTOKEN_RESUME,
BASICTOKEN_STRING,
BASICTOKEN_INTEGER,
BASICTOKEN_DOUBLE,
BASICTOKEN_BOOLEAN,
BASICTOKEN_VARIANT,
BASICTOKEN_OUTPUT,
BASICTOKEN_BINARY,
BASICTOKEN_ACCESS,
BASICTOKEN_READ,
BASICTOKEN_WRITE,
BASICTOKEN_APPEND,
BASICTOKEN_BYVAL,
BASICTOKEN_BYREF,
BASICTOKEN_IS,
BASICTOKEN_PRESERVE,

/* operators */
BASICTOKEN_ADD,
BASICTOKEN_SUBTRACT,
BASICTOKEN_INCREMENT,
BASICTOKEN_DECREMENT,
BASICTOKEN_ADDTO,
BASICTOKEN_SUBTRACTFROM,
BASICTOKEN_MULTIPLY,
BASICTOKEN_DIVIDE,
BASICTOKEN_MODULO,
BASICTOKEN_TESTEQUAL,
BASICTOKEN_TESTNOTEQUAL,
BASICTOKEN_TESTLESSTHAN,
BASICTOKEN_TESTLESSTHANOREQUAL,
BASICTOKEN_TESTGREATERTHAN,
BASICTOKEN_TESTGREATERTHANOREQUAL,
BASICTOKEN_EQUALS,
BASICTOKEN_FIELDPREFIX,
BASICTOKEN_NOT,
BASICTOKEN_COMMA,
BASICTOKEN_SEMICOLON,
BASICTOKEN_OPENBRACKET,
BASICTOKEN_CLOSEBRACKET,
BASICTOKEN_LOGICALAND,
BASICTOKEN_LOGICALOR,
BASICTOKEN_BITWISEAND,
BASICTOKEN_BITWISEOR,
BASICTOKEN_BITWISEEXCLUSIVEOR,
BASICTOKEN_BITWISESHIFTLEFT,
BASICTOKEN_BITWISESHIFTRIGHT,
BASICTOKEN_EXPONENT,
BASICTOKEN_OPENSQUAREBRACKET,
BASICTOKEN_CLOSESQUAREBRACKET,
BASICTOKEN_QUESTIONMARK,

BASICTOKEN_EOL,
BASICTOKEN_FINISHED,
BASICTOKEN_NUMTOKENS
};

enum
{
ERROR_OK,						/* not an error */
ERROR_USERCANCEL,
ERROR_UNBALANCEDPAREN,
ERROR_NOEXPRESSION,
ERROR_EQUALSIGNEXPECTED,
ERROR_NOTAVARIABLE,
ERROR_LABELERROR,
ERROR_UNDEFINEDLABEL,
ERROR_THENEXPECTED,
ERROR_TOEXPECTED,
ERROR_FOREXPECTED,
ERROR_NEXTWITHOUTFOR,
ERROR_RETURNWITHOUTGOSUB,
ERROR_COMMAEXPECTED,
ERROR_OPENBRACKETEXPECTED,
ERROR_CLOSEBRACKETEXPECTED,
ERROR_BADTYPEFORCOMMAND,
ERROR_UNDEFINEDVAR,
ERROR_COLONEXPECTED,
ERROR_ELSEIFWITHOUTIF,
ERROR_ELSEWITHOUTIF,
ERROR_ENDIFWITHOUTIF,
ERROR_LOOPWITHOUTDO,
ERROR_WENDWITHOUTWHILE,
ERROR_UNTERMINATEDQUOTES,
ERROR_ASEXPECTED,
ERROR_UNKNOWNTYPE,
ERROR_UNINITIALIZEDVAR,
ERROR_NODOTOEXIT,
ERROR_NOFORTOEXIT,
ERROR_NOWHILETOEXIT,
ERROR_BADFORTYPE,
ERROR_UNTERMINATEDCOMMAND,
ERROR_COMMAORBRACKETEXPECTED,
ERROR_NOINDICES,
ERROR_ARRAYNOTDEFINED,
ERROR_VARIABLEISNOTARRAY,
ERROR_VARIABLEARRAYMISMATCH,
ERROR_ARRAYINDEXOUTOFRANGE,
ERROR_COMMANDNOTVALIDASGLOBAL,
ERROR_TYPEMISMATCH,
ERROR_ENDTYPEMISMATCH,
ERROR_EXITTYPEMISMATCH,
ERROR_FILENOTOPEN,
ERROR_FILEOPENERROR,
ERROR_FILEREADERROR,
ERROR_NOTACOMMAND,
ERROR_WHILEUNTILOREOLEXPECTED,
ERROR_EXITTYPENOTVALID,
ERROR_NUMBERSYMBOLEXPECTED,
ERROR_FILEHANDLEALREADYUSED,
ERROR_TOOMANYPARAMETERS,
ERROR_EXPESSIONPRIMITIVEEXPECTED,
ERROR_WRONGNUMBEROFPARAMETERS,
ERROR_EOLEXPECTED,
ERROR_GOTOEXPECTED,
ERROR_NEXTEXPECTED,
ERROR_NORETVALSET,
ERROR_VARTYPEMISMATCH,
ERROR_TRYINGTOSETCONSTANT,
ERROR_TRYINGTOSETARRAY,
ERROR_NOTASTRUCTFIELD,
ERROR_NOTACLASSFIELD,
ERROR_CASEWITHOUTSELECT,
ERROR_ENDSELECTWITHOUTSELECT,
ERROR_USERCANCELLEDINPUT,
ERROR_CANNOTPRESERVE,
ERROR_NAMEALREADYUSED,
ERROR_SYNTAX,
ERROR_USERERRORS
};


/* index offset for first variable type */
#define VARTYPE_FIRST VARTYPE_INTEGER

class kGUIBasic;

/* base class for all objects used by the Basic compiler/interpreter */
class kGUIBasicObj
{
public:
	inline kGUIBasicObj() {m_type=VARTYPE_UNDEFINED;}
	virtual ~kGUIBasicObj() {}
	inline void SetType(int type) {m_type=type;}
	inline int GetType(void) {return m_type;}
private:
	int m_type;
};

/*! @internal @struct BTOK_DEF
    @brief Internal struct used by the kGUIBasic class
	This struct contains the list of keywords or tokens and their associated information. */
typedef struct
{
	int type;
	int num;
	kGUIBasicObj *obj;
	class kGUIBasicFlowObj *fobj;
	const char *source;
	int len;
}BTOK_DEF;



/* used for keeping track of locations for flow control, things */
/* like if,elseif,endif,do,loop,goto etc */

class kGUIBasicFlowObj : public kGUIBasicObj
{
public:
	kGUIBasicFlowObj() {m_numlinks=0;m_links.SetGrow(true);}
	void SetAddr(const BTOK_DEF *addr) {m_addr=addr;}
	const BTOK_DEF *GetAddr(void) {return m_addr;}
	int GetNumChildren(void) {return m_numlinks;}
	void AddChild(kGUIBasicFlowObj *child) {m_links.SetEntry(m_numlinks,child);++m_numlinks;}
	kGUIBasicFlowObj *GetChild(int n) {return m_links.GetEntry(n);}
private:
	const BTOK_DEF *m_addr;
	int m_numlinks;
	Array<kGUIBasicFlowObj *>m_links;
};

class kGUIBasicClassObj : public kGUIBasicObj
{
public:
	kGUIBasicClassObj() {SetType(VARTYPE_CLASS);}
	virtual ~kGUIBasicClassObj() {}
	static kGUIBasicClassObj *Alloc(void);
	virtual kGUIBasicObj *GetObj(const char *name)=0;
	virtual int Function(int funcindex,class kGUIBasicVarObj *result,class CallParms *p)=0;
private:
};

/* used for variables */

class kGUIBasicStructObj;

class kGUIBasicIndices
{
public:
	kGUIBasicIndices() {m_numindices=0;m_indices.SetGrowSize(4);m_indices.SetGrow(true);}
	inline void ClearIndices(void) {m_numindices=0;}
	inline int GetNumIndices(void) {return m_numindices;}
	inline void AddIndice(int num) {m_indices.SetEntry(m_numindices++,num);}
	inline int GetIndice(int index) {return m_indices.GetEntry(index);}
	int GetTotalArrayEntries(void);
	/* only used to change an already added entry */
	inline void SetIndice(int index,int value) {m_indices.SetEntry(index,value);}

private:
	int m_numindices;
	Array<int>m_indices;
};

class kGUIBasicVarObj : public kGUIBasicObj
{
public:
	kGUIBasicVarObj() {m_value.i=0;m_isglobal=false;m_isvariant=false;m_isundefined=true;m_isarray=false;m_xarrayentries=0;m_isconstant=false;}
	virtual ~kGUIBasicVarObj() {if(GetType()==VARTYPE_STRING) delete m_value.s;else if(GetType()==VARTYPE_CLASS) delete m_value.def;if(m_xarrayentries){delete []m_xarrayentries;}}

	virtual void AllocArray(int n) {m_xarrayentries=new kGUIBasicVarObj[n];}
	virtual kGUIBasicVarObj *GetEntry(int n) {return m_xarrayentries+n;}
	virtual void PreRead(void) {}
	virtual void PostWrite(void) {}

	bool InitVarType(kGUIBasicObj *type,kGUIBasicIndices *indices);
	kGUIBasicVarObj *GetArrayVar(kGUIBasicIndices *indices,int *verror);
	int ReDim(kGUIBasicIndices *indices,kGUIBasicObj *newtype,bool preserve);
	void Set(int v);
	void Set(double v);
	void Set(kGUIString *s);
	void Set(const char *v);
	void Set(const char *v,int nc);
	void Set(bool b);
	int GetInt(void);
	double GetDouble(void);
	void GetString(kGUIString *s);
	kGUIString *GetStringObj(void);
	bool GetBoolean(void);
	bool CheckType(kGUIBasicVarObj *other);	/* make sure it matches the same type */
	void ChangeType(int newtype);	/* and convert value to new type */
	void Copy(kGUIBasicVarObj *v2);		/* copy type and value from v2 */

	inline bool GetIsVariant(void) {return m_isvariant;}
	inline void SetIsVariant(bool a) {m_isvariant=a;}

	inline bool GetIsConstant(void) {return m_isconstant;}
	inline void SetIsConstant(bool a) {m_isconstant=a;}

	inline bool GetIsUndefined(void) {return m_isundefined;}
	inline void SetIsUndefined(bool a) {m_isundefined=a;}

	inline bool GetIsArray(void) {return m_isarray;}
	inline void SetIsArray(bool a) {m_isarray=a;}

	inline bool GetIsGlobal(void) {return m_isglobal;}
	inline void SetIsGlobal(bool a) {m_isglobal=a;}

	inline kGUIBasicObj *GetDefPtr(void) {return m_value.def;}
	inline void SetSystemClassAllocator(kGUIBasicClassObj *(*func)(void)) {m_value.allocsysclass=func;}

	inline void ClearIndices(void) {m_indices.ClearIndices();};
	inline int GetNumIndices(void) {return m_indices.GetNumIndices();}
	inline void AddIndice(int num) {m_indices.AddIndice(num);}
	inline int GetIndice(int index) {return m_indices.GetIndice(index);}
	inline void SetIndice(int index,int value) {return m_indices.SetIndice(index,value);}
	inline int GetTotalArrayEntries(void) {return m_indices.GetTotalArrayEntries();}

	inline kGUIBasicIndices *GetIndicePtr(void) {return &m_indices;}

//	inline bool GetIsSingle(void) {return m_issingle;}
//	inline void SetIsSingle(bool a) {m_issingle=a;}

	/* these are usefull for system and app class variables */
	inline void *GetUserDataPtr(void) {return m_user.ptr;}
	inline void SetUserData(void *user) {m_user.ptr=user;}
	inline int GetUserDataInt(void) {return m_user.i;}
	inline void SetUserData(int user) {m_user.i=user;}
//	inline void SetPreReadCallback(void *codeobj,void (*code)(void *,kGUIBasicVarObj *)) {m_prereadcallback.Set(codeobj,code);}
//	inline void SetPostWriteCallback(void *codeobj,void (*code)(void *,kGUIBasicVarObj *)) {m_postwritecallback.Set(codeobj,code);}

	/* these are used if it is an array object, or for each field if it is a structure */
	kGUIBasicVarObj *m_xarrayentries;
private:
	bool m_isvariant:1;
	bool m_isundefined:1;
	bool m_isarray:1;
	bool m_isconstant:1;
	bool m_isglobal:1;
//	bool m_issingle:1;
	union
	{
		int i;										/* integer */
		double d;									/* double */
		bool b;										/* boolean */
		kGUIString *s;								/* allocated string */
		kGUIBasicObj *def;							/* only used if a structure or class */
		kGUIBasicClassObj *(*allocsysclass)(void);	/* function to call to allocate a system class */
	}m_value;

	kGUIBasicIndices m_indices;

	union
	{
		void *ptr;
		int i;
	}m_user;
//
//	kGUICallBackPtr<kGUIBasicVarObj> m_prereadcallback;
//	kGUICallBackPtr<kGUIBasicVarObj> m_postwritecallback;
};

typedef struct
{
	kGUIString *name;
	kGUIBasicVarObj *var;
}sfields_def;

class kGUIBasicStructObj : public kGUIBasicObj
{
public:
	kGUIBasicStructObj() {m_ispublic=false;m_numfields=0;m_fields.Alloc(8);m_fields.SetGrowSize(8);m_fields.SetGrow(true);m_fieldhash.Init(6,sizeof(int));}
	~kGUIBasicStructObj() {int i;sfields_def sd;for(i=0;i<m_numfields;++i){sd=m_fields.GetEntry(i);delete sd.name;delete sd.var;m_fieldhash.Purge();}}
	int GetNumFields(void) {return m_numfields;}
	void AddField(kGUIString *name,kGUIBasicVarObj *var) {sfields_def sd;m_fieldhash.Add(name->GetString(),&m_numfields);sd.name=name;sd.var=var;m_fields.SetEntry(m_numfields++,sd);}
	kGUIBasicVarObj *GetFieldType(int index) {return m_fields.GetEntry(index).var;}
	kGUIString *GetFieldName(int index) {return m_fields.GetEntry(index).name;}
	int GetFieldIndex(const char *name) {int *ip;ip=(int *)m_fieldhash.Find(name);if(!ip) return(-1);return(ip[0]);}

	bool GetIsPublic(void) {return m_ispublic;}
	void SetIsPublic(bool a) {m_ispublic=a;}
private:
	bool m_ispublic;
	int m_numfields;
	Array<sfields_def>m_fields;
	Hash m_fieldhash;			/* small hash list for field name to index conversion */
};

class kGUIEnumObj : public kGUIBasicObj
{
public:
	kGUIEnumObj() {m_vartype=0;m_lastval=0;m_ispublic=false;m_numfields=0;m_fields.Alloc(8);m_fields.SetGrowSize(8);m_fields.SetGrow(true);m_fieldhash.Init(6,sizeof(int));}
	~kGUIEnumObj() {int i;sfields_def sd;for(i=0;i<m_numfields;++i){sd=m_fields.GetEntry(i);delete sd.name;delete sd.var;m_fieldhash.Purge();}}
	int GetNumFields(void) {return m_numfields;}
	void AddField(kGUIString *name,kGUIBasicVarObj *var) {sfields_def sd;m_fieldhash.Add(name->GetString(),&m_numfields);sd.name=name;sd.var=var;m_fields.SetEntry(m_numfields++,sd);}
	kGUIBasicVarObj *GetFieldType(int index) {return m_fields.GetEntry(index).var;}
	kGUIString *GetFieldName(int index) {return m_fields.GetEntry(index).name;}
	int GetFieldIndex(const char *name) {int *ip;ip=(int *)m_fieldhash.Find(name);if(!ip) return(-1);return(ip[0]);}

	bool GetIsPublic(void) {return m_ispublic;}
	void SetIsPublic(bool a) {m_ispublic=a;}
	void SetVarType(kGUIBasicObj *type) {m_vartype=type;}
	kGUIBasicObj *GetVarType(void) {return m_vartype;}
	void SetLastValue(int n) {m_lastval=n;}
	int GetLastValue(void) {return m_lastval;}
private:
	bool m_ispublic;
	int m_numfields;
	kGUIBasicObj *m_vartype;
	int m_lastval;
	Array<sfields_def>m_fields;
	Hash m_fieldhash;			/* small hash list for field name to index conversion */
};


typedef struct
{
	kGUIString *name;
	kGUIBasicVarObj *type;
	int bymode;		/* either byval or byref */
}parm_def;

/* used for both functions and subroutines */
class kGUIBasicFuncObj : public kGUIBasicFlowObj
{
public:
	kGUIBasicFuncObj() {m_ispublic=false;m_numparms=0;m_parms.SetGrow(true);m_retval=0;}
	~kGUIBasicFuncObj();

	const char *GetName(void) {return m_name.GetString();}
	void SetName(const char *name) {m_name.SetString(name);}

	bool GetIsPublic(void) {return m_ispublic;}
	void SetIsPublic(bool a) {m_ispublic=a;}

	int GetNumParms(void) {return m_numparms;}
	void AddParm(const char *name,kGUIBasicVarObj *var,int bymode);
	const char *GetParmName(int index) {return m_parms.GetEntry(index).name->GetString();}
	kGUIBasicVarObj *GetParmType(int index) {return m_parms.GetEntry(index).type;}
	int GetParmByMode(int index) {return m_parms.GetEntry(index).bymode;}

	kGUIBasicVarObj *m_retval;	/* return value is stored here */
	kGUIBasicVarObj m_rettype;	/* return value type is stored here */
private:
	bool m_ispublic:1;
	int m_numparms:8;
	kGUIString m_name;
	Array<parm_def>m_parms;
};

class CallParms
{
public:
	CallParms() {m_numparms=0;m_purge.Alloc(10);m_purge.SetGrow(true);m_parms.Alloc(10);m_parms.SetGrow(true);}
	~CallParms() {int i;for(i=0;i<m_numparms;++i){if(m_purge.GetEntry(i)==true) delete m_parms.GetEntry(i);}}
	void AddParm(kGUIBasicVarObj *parm,bool purge) {m_parms.SetEntry(m_numparms,parm);m_purge.SetEntry(m_numparms++,purge);}
	int GetNumParms(void) {return m_numparms;}
	kGUIBasicVarObj *GetParm(int index) {return index>=m_numparms?0:m_parms.GetEntry(index);}
private:
	int m_numparms;
	Array<bool >m_purge;
	Array<kGUIBasicVarObj *>m_parms;
};

typedef struct
{
	int type;		/* basictoken_xxx */
	int bymode;		/* either byval or byref */
}cparm_def;

class ParmDef
{
public:
	ParmDef() {m_numparms=0;}
	void SetParmDef(const char *pdstring);
	int GetNumParms(void) {return m_numparms;}
	int  GetParmType(int index) {return m_parms.GetEntry(index).type;}
	int GetParmByMode(int index) {return m_parms.GetEntry(index).bymode;}
private:
	int m_numparms;
	Array<cparm_def>m_parms;
};

/* used for internal system class functions/subroutines */
class kGUIBasicSysClassFuncObj : public kGUIBasicObj, public ParmDef
{
public:
	kGUIBasicSysClassFuncObj() {SetType(VARTYPE_SYSCLASSFUNC);m_parent=0;m_index=0;}
	~kGUIBasicSysClassFuncObj() {}
	void Init(kGUIBasicClassObj *parent,int index,int numparms,const char *parmdef) {m_parent=parent;m_index=index;m_numparms=numparms;SetParmDef(parmdef);}
	int GetNumParms(void) {return m_numparms;}
	int GetIndex(void) {return m_index;}
	int Call(kGUIBasicVarObj *result,CallParms *p) {return m_parent->Function(m_index,result,p);}
private:
	int m_index;					/* command index to pass to class */
	int m_numparms;					/* number of parms for code */
	kGUIBasicClassObj *m_parent;	/* pointer to object for this function */
};

typedef struct
{
	kGUIString *name;
	kGUIBasicVarObj *var;
}cfields_def;

/* user class definition */
class kGUIBasicUClassObj : public kGUIBasicObj
{
public:
	kGUIBasicUClassObj() {}
	~kGUIBasicUClassObj() {}
	int GetNumFields(void) {return m_numfields;}
	void AddField(kGUIString *name,kGUIBasicVarObj *var) {cfields_def cd;m_fieldhash.Add(name->GetString(),&m_numfields);cd.name=name;cd.var=var;m_fields.SetEntry(m_numfields++,cd);}
	kGUIBasicVarObj *GetFieldType(int index) {return m_fields.GetEntry(index).var;}
	kGUIString *GetFieldName(int index) {return m_fields.GetEntry(index).name;}
	int GetFieldIndex(const char *name) {int *ip;ip=(int *)m_fieldhash.Find(name);if(!ip) return(-1);return(ip[0]);}
private:
	int m_numfields;
	Array<cfields_def>m_fields;
	Hash m_fieldhash;			/* small hash list for field name to index conversion */
};

/*! @internal @class kGUIBasicCommandObj
    @brief Internal struct used by the kGUIBasic class */
class kGUIBasicCommandObj : public kGUIBasicObj, public ParmDef
{
public:
	kGUIBasicCommandObj() {m_minnumparms=0;m_maxnumparms=0;m_code.vcodev=0;m_extra=0;}
	~kGUIBasicCommandObj() {if(m_extra) delete m_extra;}
	void SetID(int id) {m_id=id;}
	int GetID(void) {return m_id;}
	void SetMinNumParms(int nump) {m_minnumparms=nump;}
	void SetMaxNumParms(int nump) {m_maxnumparms=nump;}
	int GetMinNumParms(void) {return m_minnumparms;}
	int GetMaxNumParms(void) {return m_maxnumparms;}

	/* all the different type of code pointers */
	void SetCode(void (kGUIBasic::*code)(void)) {m_code.vcodev=code;}
	void SetCode(void (kGUIBasic::*code)(kGUIBasicVarObj *result,CallParms *p)) {m_code.fcode=code;}

	void SetExtra(kGUIBasicCommandObj *extra) {m_extra=extra;}
	kGUIBasicCommandObj *GetExtra(void) {return m_extra;}

	union	/* various code pointers */
	{
		void (kGUIBasic::*vcodev)(void);				
		void (kGUIBasic::*fcode)(kGUIBasicVarObj *result,CallParms *p);
	}m_code;
private:
	int m_id;						/* token id */
	int m_minnumparms;				/* number of parms for code */
	int m_maxnumparms;				/* number of parms for code */
	kGUIBasicCommandObj *m_extra;	/* duplicate thingy sharing this name */
};

/*! @internal @struct for_stack
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	int level;			/* sub/function level */
	kGUIBasicVarObj *vp;		/* counter variable */
	kGUIBasicVarObj *target;	/* target value */
	kGUIBasicVarObj *step;		/* step value */
	const BTOK_DEF *loc;	/* loop back to address */
}for_stack;

/*! @internal @struct call_def
    @brief Internal struct used by the kGUIBasic class */

typedef struct
{
	kGUIBasicFuncObj *func;		/* pointer to function variable */
	const BTOK_DEF *ret;		/* return address */
	union
	{
		const BTOK_DEF *go;		/* if a data error occurs then goto here! */
		int resume;
	}error;
	bool exit;
	kGUIBasicVarObj *with;		/* default "with" variable */
}call_def;

/* list of conditional commands */

enum
{
COND_ROOT,	/* root level */
COND_IF,
COND_FOR,
COND_DO,
COND_WHILE,
COND_SWITCH};

typedef struct
{
	int verbbit;
	const char *verbname;
}verbs_def;

typedef struct
{
	int handle;
	int mode;
	FILE *f;
}file_def;

/* used when a local variable has the same name as a currntly defined variable */
/* ( either from a previous level or a global ) */
/* in that case it saves a pointer to the previous one and then overrides the pointer */
/* when the sub/function exits, then the old pointer is replaced */

typedef struct
{
	int level;
	kGUIBasicVarObj **varptr;
	kGUIBasicVarObj *cur;			/* only needed for assert/debugging as it should already be in varptr[0] */
	kGUIBasicVarObj *prev;
	bool purge;						/* purge the variable upon popping */
}local_def;

typedef struct
{
	const BTOK_DEF *pc;
}preverb_def;

class kGUIBasic
{
public:
	kGUIBasic();
	~kGUIBasic();
	bool Compile(kGUIText *source,kGUIString *output);
	void Start(const char *subname,bool async);
	int GetPublicSubs(ClassArray<kGUIString> *subnames);
	void SetDoneCallback(void *codeobj,void (*code)(void *)) {m_donecallback.Set(codeobj,code);}
	void SetErrorCallback(void *codeobj,void (*code)(void *,int start,int end)) {m_errorcallback.Set(codeobj,code);}
	void SetAddAppObjectsCallback(void *codeobj,void (*code)(void *)) {m_addappobjectscallback.Set(codeobj,code);}
	void AddLeak(kGUIBasicObj *obj) {m_leaklist.SetEntry(m_numleakentries++,obj);}
	void RemoveLeak(kGUIBasicObj *obj) {m_leaklist.Delete(obj);--m_numleakentries;}
	CALLBACKGLUE(kGUIBasic,MainLoop);
	CALLBACKGLUEPTRVAL(kGUIBasic,InputDone,kGUIString,int);
	CALLBACKGLUEVAL(kGUIBasic,MessageDone,int);
	CALLBACKGLUEPTRVAL(kGUIBasic,FileReqDone,kGUIFileReq,int);
	void AddAppObject(const char *name,kGUIBasicObj *obj) {RegObj(name,obj);}
	void CheckType(kGUIBasicObj *obj,int type) {if(obj->GetType()!=type) basicerror(ERROR_TYPEMISMATCH);}
	kGUIBasicObj *GetObj(const char *name);
	void Cancel(void) {m_cancel=true;}
	int RegisterError(const char *message);

	static int GetTypeFromToken(int token);
	static const char *GetInstructions(void);

	static void exp_synctypes(kGUIBasicVarObj *v1,kGUIBasicVarObj *v2);
	static int exp_vardelta(kGUIBasicVarObj *v1,kGUIBasicVarObj *v2);
private:
	void SetTable(bool *dest,const char *source);
	void InputDone(kGUIString *s,int closebutton);
	void MessageDone(int result);
	void FileReqDone(kGUIFileReq *req,int pressed);
	void MainLoop(void);
	void Loop(void);
	void CheckEOL(void);
	void SetSourceColor(const char *pc1,const char *pc2,kGUIColor c);
	bool RegObj(const char *name,kGUIBasicObj *var,bool fataldupl=true);
	void RegGlobal(const char *name,kGUIBasicVarObj *var);
	void RegLocal(int depth,const char *name,kGUIBasicVarObj *var,bool dopurge=true);
	kGUIBasicFlowObj *GetFObj(const char *name) {return static_cast<kGUIBasicFlowObj *>(GetObj(name));}

	kGUIBasicObj *GetFinalObj(void);
	kGUIBasicVarObj *ToVar(kGUIBasicObj *obj);
	kGUIBasicVarObj *GetVar(void);
	void Call(kGUIBasicObj *obj,kGUIBasicVarObj *result);
	void Set(kGUIBasicVarObj *vobj);

	void ParseUserParms(kGUIBasicFuncObj *fobj);
	int ParseSystemParms(ParmDef *def,CallParms *parms);

	void DisablePrevLocals(void);
	void EnablePrevLocals(void);

	void PopCall(void);
	void PurgeVars(void);
	void PurgeFor(for_stack *fs);
	void CloseFiles(bool report);
	void Print(const char *s);
	void GetVarName(kGUIString *name);
	void GetVarDef(kGUIString *name,kGUIBasicVarObj *var);	
	void SetVar(kGUIBasicVarObj *var,kGUIBasicVarObj *value);
	kGUIBasicVarObj *GetArrayVar(const char *name,kGUIBasicIndices *indices);
	void GetIndices(kGUIBasicIndices *indices);
	void DimVar(kGUIString *vn,kGUIBasicVarObj *var);
	bool IsPreVerb(int token);

	file_def *GetFileHandle(int num,int *slot=0);
	void UpdateActive(void);
	void AddBuiltInClasses(void);

	void basicerror(int error);
	void basicerrortrap(int error);

	/* get token from the source code */
	const char *GetSourceToken(const char **startpc,const char *pc);
	void GetToken(bool getname=false);			/* get token from compiled token list */
	bool skipto(int toknum);
	void SkipToEOL(void);
	inline void Rewind(void) {--m_pc;}
	/* commands */
	void cmd_null(void){}
	void cmd_if(void);
	void cmd_else(void);	/* else and elseif */
	void cmd_endif(void);
	void cmd_do(void);
	void cmd_loop(void);
	void cmd_for(void);
	void cmd_while(void);
	void cmd_wend(void);
	void cmd_goto(void);
	void cmd_end(void);
	void cmd_print(void);
	void cmd_next(void);
	void cmd_select(void);
	void cmd_case(void);
	void cmd_input(void);
	void cmd_inputbox(void);
	void cmd_return(void);
	void cmd_call(void);
	void cmd_open(void);
	void cmd_close(void);
	void cmd_set(void);
	void cmd_dim(void);
	void cmd_redim(void);
	void cmd_exit(void);
	void cmd_on(void);
	void cmd_with(void);

	/* expression evaluation */
	void exp_get(kGUIBasicVarObj *result);

	void exp_cond(kGUIBasicVarObj *r);	
	void exp_lor(kGUIBasicVarObj *r);	
	void exp_land(kGUIBasicVarObj *r);	
	void exp_bor(kGUIBasicVarObj *r);	
	void exp_bxor(kGUIBasicVarObj *r);	
	void exp_band(kGUIBasicVarObj *r);	
	void exp_eqneq(kGUIBasicVarObj *r);	
	void exp_cmp(kGUIBasicVarObj *r);	
	void exp_shift(kGUIBasicVarObj *r);	

	void exp_addsub(kGUIBasicVarObj *r);
	void exp_muldivmod(kGUIBasicVarObj *r);
	void exp_exponent(kGUIBasicVarObj *r);
	void exp_not(kGUIBasicVarObj *r);
	void exp_unary(kGUIBasicVarObj *r);
	void exp_parenthesis(kGUIBasicVarObj *result);
	void exp_primitive(kGUIBasicVarObj *result);

	void exp_add(kGUIBasicVarObj *r,kGUIBasicVarObj *a);
	void exp_sub(kGUIBasicVarObj *r,kGUIBasicVarObj *a);

	/* functions */
	void func_rand(kGUIBasicVarObj *result,CallParms *p);

	void func_mid(kGUIBasicVarObj *result,CallParms *p);

	void func_len(kGUIBasicVarObj *result,CallParms *p);
	void func_asc(kGUIBasicVarObj *result,CallParms *p);
	void func_chr(kGUIBasicVarObj *result,CallParms *p);
	void func_val(kGUIBasicVarObj *result,CallParms *p);
	void func_dval(kGUIBasicVarObj *result,CallParms *p);
	void func_str(kGUIBasicVarObj *result,CallParms *p);
	void func_sin(kGUIBasicVarObj *result,CallParms *p);
	void func_cos(kGUIBasicVarObj *result,CallParms *p);
	void func_tan(kGUIBasicVarObj *result,CallParms *p);
	void func_asin(kGUIBasicVarObj *result,CallParms *p);
	void func_acos(kGUIBasicVarObj *result,CallParms *p);
	void func_atan(kGUIBasicVarObj *result,CallParms *p);
	void func_eof(kGUIBasicVarObj *result,CallParms *p);

	void func_left(kGUIBasicVarObj *result,CallParms *p);
	void func_right(kGUIBasicVarObj *result,CallParms *p);

	void func_iif(kGUIBasicVarObj *result,CallParms *p);

	void func_msgbox(kGUIBasicVarObj *result,CallParms *p);
	void func_filereq(kGUIBasicVarObj *result,CallParms *p);
	void func_shellexec(kGUIBasicVarObj *result,CallParms *p);

	void func_fileexists(kGUIBasicVarObj *result,CallParms *p);
	void func_trim(kGUIBasicVarObj *result,CallParms *p);
	void func_split(kGUIBasicVarObj *result,CallParms *p);
	void func_instr(kGUIBasicVarObj *result,CallParms *p);
	void func_replace(kGUIBasicVarObj *result,CallParms *p);

	void func_ubound(kGUIBasicVarObj *result,CallParms *p);

	void func_input(kGUIBasicVarObj *result,CallParms *p);
	void func_sortobjects(kGUIBasicVarObj *result,CallParms *p);

	int m_numbtokens;
	Array<BTOK_DEF>m_btoklist;
	const BTOK_DEF *m_lastpc;
	const BTOK_DEF *m_pc;

	char m_token[80];
	char m_token_type;
	char m_tok;
	kGUIBasicObj *m_tokobj;

	int m_fordepth;			/* index to top of FOR stack */
	int m_calldepth;			/* index into call/function stack */
	int m_numlocals;	/* number of locals */
	int m_numfiles;		/* number of open files */

	kGUIString m_input;
	volatile bool m_inputdone;
	volatile bool m_inputcancel;
	int m_msgresult;
	kGUIString m_filereqstring;
	int m_filereqresult;

	unsigned int m_errorindex;
	Hash m_usererrors;

	jmp_buf m_ebuf; /* hold environment for longjmp() */

	/************************************************************************/
	bool m_compile;						/* in compile pass? */
	bool m_eolnext;						/* eol should be next flag */
	Array<for_stack>m_fstack;			/* for stack */
	Array<call_def>m_callstack;			/* call stack */
	Array<file_def>m_files;				/* files */
	Array<local_def>m_lstack;			/* local stack */
	int m_numpreverbs;
	Array<preverb_def>m_preverbs;

	kGUIText *m_source;					/* source string */
	kGUIString *m_output;				/* output is appended to this string */
	bool m_exit;
	bool m_cancel;

	Hash m_varhash;						/* hash list for variables */
	kGUICallBack m_donecallback;
	kGUICallBackIntInt m_errorcallback;
	/************************************************************************/
	int m_numleakentries;
	Array<kGUIBasicObj *>m_leaklist;
	/************************************************************************/
	kGUICallBack m_addappobjectscallback;
	/************************************************************************/
	volatile bool m_asyncactive;	/* true if running in async mode */
	kGUIThread m_thread;			/* only used for async */
	bool m_singledelimtable[256];
	bool m_groupdelimtable[256];
	bool m_alldelimtable[256];
	bool m_alphatable[256];
	bool m_digittable[256];
	bool m_nametable[256];
//	int m_exppri[BASICTOKEN_NUMTOKENS];
};

/* after adding errors here, add them to the error string array in kguibasic.cpp */

#endif
