/**********************************************************************************/
/* kGUI - kguibasic.cpp                                                           */
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

/*! @file kguibasic.cpp 
   @brief This is a version of the Basic language that can be used as a scripting
   language in applications. Using it you can add custom app commands and you can
   also add custom variables for for interacting between your app and the basic
   language. */

#include "kgui.h"
#include "kguibasic.h"
#include "kguithread.h"
#include <math.h>

#define COMMENTCOLOR DrawColor(64,255,64)
#define COMMANDCOLOR DrawColor(255,64,64)
#define COMMANDVERBCOLOR DrawColor(192,128,128)
#define FUNCTIONCOLOR DrawColor(64,64,255)
#define OPERATORCOLOR DrawColor(192,192,0)

////	"=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|="

/* token types */
enum
{
TT_UNDEFINED,
TT_DELIMITER,
TT_VARIABLE,
TT_NUMBER,
TT_QUOTE,
TT_LABEL,
TT_COMMAND,
TT_COMMANDVERB
};

static const char *errormessages[]=
{   
	"No error!",
	"User Cancelled", 
	"unbalanced parentheses", 
    "no expression present",
    "equals sign expected",
    "not a variable",
	"label name alreay exists or is a reserved name",
	"undefined label",
	"THEN expected",
    "TO expected",
    "FOR expected",
    "NEXT without FOR",
    "RETURN without GOSUB",
    "Comma Expected",
    "Open Bracket '(' Expected",
	"Close Bracket ')' Expected",
	"Operator does not support variable type",
	"Undefined variable",
    "Colon Expected",
    "ElseIf without If",
    "Else without If",
    "Endif without If",
    "Loop without Do",
    "Wend without While",
    "Unterminated quotes",
    "AS expected",
    "Unknown type",
    "Uninitialized variable",
    "No previous DO command to exit",
    "No active FOR command to exit",
    "No previous WHILE command to exit",
    "variable type not valid in for command",
	"End command found before previous nested command (if/for/do) was finished",
	"A comma or a close bracket was expected",
	"No indices on array reference were found",
	"Array not defined in advance, use the Dim command.",
	"Array reference to a non-array variable.",
	"Number of variable indices does not match definition.",
	"Array index is out of defined range for the array.",
	"Commands are not allowed in global space, they are only allowed in a subroutine or function.",
	"Type Mismatch ( this command doesn't work on the type supplied).",
	"End command type does not match current code type (sub or function).",
	"Exit command type does not match current code type (sub or function).",
	"File is not open.",
	"Cannot open file.",
	"Error reading from file.",
	"Not a command.",
	"While, Until or EOL expected.",
	"Exit type not valid.",
	"A Number (#) symbol was expected at this point.",
	"The file handle selected is already in use.",
	"Too many parameters.",
	"An Expression value, variable, function, or constant was expected here.",
	"An incorrect number of parameters was encountered.",
	"An EOL ( end of line ) was expected at this point.",
	"A Goto was expected at this point.",
	"A Next was expected at this point.",
	"Exiting a function without a return value set.",
	"Variable type does not match the expected type.",
	"Variable is a contstant and not allowed to be changed.",
	"Variable is an array and needs an index.",
	"Variable is not a field in the structure.",
	"Variable is not a field in the class.",
    "Case without Select",
    "End Select without Select",
    "User cancelled input",
	"Cannot preserve entries since num indices or types are different",
	"Item name has already been used.",
	"syntax error"
  }; 

/******************************************************/

void kGUIBasicVarObj::Set(int v)
{
	if(GetType()==VARTYPE_STRING)
		delete m_value.s;
	SetType(VARTYPE_INTEGER);
	m_value.i=v;

	PostWrite();
}

void kGUIBasicVarObj::Set(double v)
{
	if(GetType()==VARTYPE_STRING)
		delete m_value.s;
	SetType(VARTYPE_DOUBLE);
	m_value.d=v;

	PostWrite();
}

void kGUIBasicVarObj::Set(const char *v)
{
	if(GetType()!=VARTYPE_STRING)
		m_value.s=new kGUIString();
	SetType(VARTYPE_STRING);
	m_value.s->SetString(v);

	PostWrite();
}

void kGUIBasicVarObj::Set(const char *v,int nc)
{
	if(GetType()!=VARTYPE_STRING)
		m_value.s=new kGUIString();
	SetType(VARTYPE_STRING);
	m_value.s->SetString(v,nc);

	PostWrite();
}

void kGUIBasicVarObj::Set(kGUIString *s)
{
	if(GetType()!=VARTYPE_STRING)
		m_value.s=new kGUIString();
	SetType(VARTYPE_STRING);
	m_value.s->SetString(s->GetString(),s->GetLen());

	PostWrite();
}

void kGUIBasicVarObj::Set(bool b)
{
	if(GetType()==VARTYPE_STRING)
		delete m_value.s;
	SetType(VARTYPE_BOOLEAN);
	m_value.b=b;

	PostWrite();
}

int kGUIBasicVarObj::GetInt(void)
{
	PreRead();
	switch(GetType())
	{
	case VARTYPE_BOOLEAN:
		return (m_value.b==true?1:0);
	break;
	case VARTYPE_INTEGER:
		return (m_value.i);
	break;
	case VARTYPE_DOUBLE:
		return ((int)m_value.d);
	break;
	case VARTYPE_STRING:
		return m_value.s->GetInt();
	break;
	}
	assert(false,"Unsupported type!");
	return(0);
}

double kGUIBasicVarObj::GetDouble(void)
{
	PreRead();
	switch(GetType())
	{
	case VARTYPE_BOOLEAN:
		return (m_value.b==true?1.0f:0.0f);
	break;
	case VARTYPE_INTEGER:
		return((double)m_value.i);
	break;
	case VARTYPE_DOUBLE:
		return(m_value.d);
	break;
	case VARTYPE_STRING:
		return m_value.s->GetDouble();
	break;
	}
	assert(false,"Unsupported type!");
	return(0.0f);
}

kGUIString *kGUIBasicVarObj::GetStringObj(void)
{
	assert(GetType()==VARTYPE_STRING,"Not a string!");

	PreRead();
	return m_value.s;
}

void kGUIBasicVarObj::GetString(kGUIString *s)
{
	PreRead();
	switch(GetType())
	{
	case VARTYPE_BOOLEAN:
		if(m_value.b==true)
			s->SetString("True");
		else
			s->SetString("False");
	break;
	case VARTYPE_INTEGER:
		s->Sprintf("%d",m_value.i);
	break;
	case VARTYPE_DOUBLE:
		s->Sprintf("%f",m_value.d);
		if(strstr(s->GetString(),"."))
		{
			while(s->GetChar(s->GetLen()-1)=='0')
				s->Clip(s->GetLen()-1);

			if(s->GetLen()>1 && s->GetChar(s->GetLen()-1)=='.')
				s->Clip(s->GetLen()-1);
		}
	break;
	case VARTYPE_STRING:
		s->Sprintf("%s",m_value.s->GetString());
	break;
	default:
		assert(false,"Unsupported type!");
	break;
	}
}

bool kGUIBasicVarObj::GetBoolean(void)
{
	PreRead();

	switch(GetType())
	{
	case VARTYPE_BOOLEAN:
		return(m_value.b);
	break;
	case VARTYPE_INTEGER:
		return(m_value.i?true:false);
	break;
	case VARTYPE_DOUBLE:
		return(m_value.d!=0.0f?true:false);
	break;
	case VARTYPE_STRING:
		return(stricmp(m_value.s->GetString(),"true")==0);
	break;
	}
	assert(false,"Unsupported type!");
	return(false);
}

int kGUIBasicIndices::GetTotalArrayEntries(void)
{
	int i;
	int totalentries=GetIndice(0);

	for(i=1;i<GetNumIndices();++i)
		totalentries*=GetIndice(i);
	return(totalentries);
}

void kGUIBasicVarObj::Copy(kGUIBasicVarObj *v2)
{
	int i,n;

	v2->PreRead();

	SetIsUndefined(v2->GetIsUndefined());
	SetIsArray(v2->GetIsArray());
	SetIsConstant(v2->GetIsConstant());
	SetIsGlobal(v2->GetIsGlobal());
	SetIsVariant(v2->GetIsVariant());

	n=v2->GetNumIndices();
	if(GetNumIndices()!=n)
	{
		ClearIndices();
		for(i=0;i<n;++i)
			AddIndice(v2->GetIndice(i));
	}
	else
	{
		for(i=0;i<n;++i)
			SetIndice(i,v2->GetIndice(i));
	}

	if(n)
	{
		int numentries;

		if(m_xarrayentries)
			delete []m_xarrayentries;
		numentries=GetTotalArrayEntries();
		AllocArray(numentries);
		for(i=0;i<numentries;++i)
		{
			GetEntry(i)->InitVarType(this,0);
			GetEntry(i)->Copy(v2->GetEntry(i));
		}
	}
	else
	{
		switch(v2->GetType())
		{
		case VARTYPE_INTEGER:
			Set(v2->m_value.i);
		break;
		case VARTYPE_DOUBLE:
			Set(v2->m_value.d);
		break;
		case VARTYPE_STRING:
			Set(v2->m_value.s);
		break;
		case VARTYPE_BOOLEAN:
			Set(v2->m_value.b);
		break;
		case VARTYPE_STRUCT:
		{
			kGUIBasicStructObj *sobj;

			sobj=static_cast<kGUIBasicStructObj *>(m_value.def);
			n=sobj->GetNumFields();
			for(i=0;i<n;++i)
				GetEntry(i)->Copy(v2->GetEntry(i));
		}
		break;
		default:
			assert(false,"Unsupported copy type!");
		break;
		}
		SetIsUndefined(false);
	}
	PostWrite();
}

bool kGUIBasicVarObj::CheckType(kGUIBasicVarObj *other)
{
	/* make sure these are the same type of variable */
	if(other->GetIsVariant()==false)
	{
		if(GetType()!=other->GetType())
			return(false);
		/* todo, if it is a structure, make sure they also match */
	}
	if(GetIsArray()!=other->GetIsArray())
		return(false);
	if(GetNumIndices()!=other->GetNumIndices())
		return(false);

	/* everything matches! */
	return(true);
}

kGUIBasicVarObj *kGUIBasicVarObj::GetArrayVar(kGUIBasicIndices *indices,int *verror)
{
	int i,per,colsize,index;
	int totalindex;

	if(GetIsArray()==false)
	{
		verror[0]=ERROR_VARIABLEISNOTARRAY;		/* not an array! */
		return(0);
	}
	if(GetNumIndices()!=indices->GetNumIndices())
	{
		verror[0]=ERROR_VARIABLEARRAYMISMATCH;		/* incorrect number of indices */
		return(0);
	}

	totalindex=0;
	per=1;
	i=indices->GetNumIndices()-1;
	while(i>=0)
	{
		index=indices->GetIndice(i);
		colsize=GetIndice(i);
		if(index<0 || index>=colsize)
		{
			verror[0]=ERROR_ARRAYINDEXOUTOFRANGE;	/* index out of range! */
			return(0);
		}
		totalindex+=(index)*per;
		per*=colsize;
		--i;
	}
	return(GetEntry(totalindex));
}

void kGUIBasicVarObj::ChangeType(int newtype)
{
	/* quick return if these type is already correct */
	if(GetType()==newtype)
		return;	

	switch(newtype)
	{
	case VARTYPE_BOOLEAN:
		Set(GetBoolean());
	break;
	case VARTYPE_INTEGER:
		Set(GetInt());
	break;
	case VARTYPE_DOUBLE:
		Set(GetDouble());
	break;
	case VARTYPE_STRING:
	{
		kGUIString temp;

		GetString(&temp);
		Set(temp.GetString());
	}
	break;
	default:
		assert(false,"Internal Error, conversion not supported!");
	break;
	};
}

/*******************************************************************************/

void kGUIBasic::SetTable(bool *dest,const char *source)
{
	int i,num;

	for(i=0;i<256;++i)
		dest[i]=false;
	num=(int)strlen(source);
	for(i=0;i<num;++i)
		dest[(unsigned char)source[i]]=true;
}

#if 0
typedef struct
{
	int toknum,priority;
}EXPPRI_DEF;

#if 0
    { MODUL,    1, "%"        },
    { MULT,     1, "*"        },
    { DIVID,    1, "/"        },
    { PLUS,     2, "+"        },
    { MINUS,    2, "-"        },
    { SHIFT_L,  3, "<<"       },
    { SHIFT_L,  3, "SHIFT_L"  },
    { SHIFT_R,  3, ">>"       },
    { SHIFT_R,  3, "SHIFT_R"  },
    { GREAT,    4, ">"        },
    { GREAT,    4, "GREAT"    },
    { GREAT_EQ, 4, ">="       },
    { GREAT_EQ, 4, "GREAT_EQ" },
    { LESS,     4, "<"        },
    { LESS,     4, "LESS"     },
    { LESS_EQ,  4, "<="       },
    { LESS_EQ,  4, "LESS_EQ"  },
    { EQ,       5, "=="       },
    { EQ,       5, "EQ"       },
    { NOTEQ,    5, "!="       },
    { NOTEQ,    5, "NOTEQ"    },
    { BIT_AND,  6, "&"        },
    { BIT_AND,  6, "BIT_AND"  },
    { BIT_OR,   7, "|"        },
    { BIT_OR,   7, "BIT_OR"   },
    { BIT_XOR,  7, "^"        },
    { BIT_XOR,  7, "BIT_XOR"  },
    { AND,      8, "&&"       },
    { AND,      8, "AND"      },
    { OR,       9, "||"       },
    { OR,       9, "OR"       },
    { XOR,      9, "XOR"      }
#endif

/* all expression operators are here with their priority */
EXPPRI_DEF exppri[]={
	{BASICTOKEN_LOGICALOR,12},
	{BASICTOKEN_LOGICALAND,11},
	{BASICTOKEN_BITWISEOR,10},
	{BASICTOKEN_BITWISEEXCLUSIVEOR,9},
	{BASICTOKEN_BITWISEAND,8},
	{BASICTOKEN_TESTEQUAL,7},
	{BASICTOKEN_EQUALS,7},
	{BASICTOKEN_TESTNOTEQUAL,7},
	{BASICTOKEN_TESTLESSTHAN,6},
	{BASICTOKEN_TESTLESSTHANOREQUAL,6},
	{BASICTOKEN_TESTGREATERTHAN,6},
	{BASICTOKEN_TESTGREATERTHANOREQUAL,6},
	{BASICTOKEN_BITWISESHIFTLEFT,5},
	{BASICTOKEN_BITWISESHIFTRIGHT,5},
	{BASICTOKEN_ADD,4},
	{BASICTOKEN_SUBTRACT,4},
	{BASICTOKEN_MULTIPLY,3},
	{BASICTOKEN_DIVIDE,3},
	{BASICTOKEN_MODULO,3},
	{BASICTOKEN_EXPONENT,2},
	{BASICTOKEN_NOT,1}};
#endif

kGUIBasic::kGUIBasic()
{
	m_asyncactive=false;
	SetTable(m_singledelimtable,":()[],");
	SetTable(m_groupdelimtable,"#+-*^/%=;><&|!");
	SetTable(m_alldelimtable,":#+-*^/%=;(),><&|[]! .\n\t");
	SetTable(m_alphatable,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	SetTable(m_digittable,".0123456789");
	SetTable(m_nametable,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

	m_fordepth=0;
	m_fstack.Alloc(32);
	m_fstack.SetGrowSize(32);
	m_fstack.SetGrow(true);

	m_calldepth=0;
	m_callstack.Alloc(32);
	m_callstack.SetGrowSize(32);
	m_callstack.SetGrow(true);

	m_numfiles=0;
	m_files.Alloc(16);
	m_files.SetGrowSize(16);
	m_files.SetGrow(true);

	m_numlocals=0;
	m_lstack.Alloc(1024);
	m_lstack.SetGrowSize(1024);
	m_lstack.SetGrow(true);

	m_numleakentries=0;
	/* should be very few in here at any time */
	m_leaklist.Init(4,2);

	m_numpreverbs=0;
	m_preverbs.Alloc(8);
	m_preverbs.SetGrow(true);

	m_numbtokens=0;
	m_btoklist.Alloc(8192);
	m_btoklist.SetGrowSize(8192);
	m_btoklist.SetGrow(true);

	/* user error messages start here */
	m_errorindex=ERROR_USERERRORS;
	m_usererrors.Init(10,0);
}

kGUIBasic::~kGUIBasic()
{
	while(m_asyncactive==true)
		kGUI::Sleep(1);

	CloseFiles(false);
	PurgeVars();
}

/* close any open files */
void kGUIBasic::CloseFiles(bool report)
{
	int i;

	/* free any pending call returns and associated locals */
	while(m_calldepth)
		PopCall();

	assert(m_fordepth==0,"Error, unpopped 'fors' on the stack!");

	/* close any open files */
	for(i=0;i<m_numfiles;++i)
	{
		file_def *fd=m_files.GetArrayPtr()+i;
		if(fd->f)
		{
			fclose(fd->f);
			if(report==true)
			{
				kGUIString s;

				s.Sprintf("*** Closing open file, Handle #%d ***\n",fd->handle);
				Print(s.GetString());
			}
		}
	}
	m_numfiles=0;
}

void kGUIBasic::PurgeVars(void)
{
	int i;
	HashEntry *he;
	kGUIBasicVarObj *vp;
	int nv=m_varhash.GetNum();
	BTOK_DEF bt;

	/* free all flow objects */
	for(i=0;i<m_numbtokens;++i)
	{
		bt=m_btoklist.GetEntry(i);
		if(bt.fobj)
			delete bt.fobj;
	}
	m_numbtokens=0;

	/* purge variables in the leak list first */
	for(i=0;i<m_numleakentries;++i)
		delete m_leaklist.GetEntry(i);

	m_numleakentries=0;

	/*  purge all variables */
	he=m_varhash.GetFirst();
	for(i=0;i<nv;++i)
	{
		vp=*((kGUIBasicVarObj **)(he->m_data));
		if(vp)
			delete vp;
		he=he->GetNext();
	}

	m_varhash.Purge();
}

/* add kGUIBasicObj to the object hash table, fatal error if already exists */
bool kGUIBasic::RegObj(const char *name,kGUIBasicObj *var,bool fataldupl)
{
	kGUIBasicObj **pvp;
	kGUIBasicObj *vp;

	pvp=(kGUIBasicObj **)m_varhash.Find(name);
	if(pvp!=0)
	{
		if(fataldupl)
		{
			assert(pvp==0,"Error: Object already exists!");
		}
		else
			return(false);	/* error,already exists */
	}
	/* add it to the hash table and add a pointer to the object */
	vp=var;
	m_varhash.Add(name,&vp);
	return(true);	/* added ok! */
}

kGUIBasicObj *kGUIBasic::GetObj(const char *name)
{
	kGUIBasicObj **pvp;
	kGUIBasicObj *vp;
	
	pvp=(kGUIBasicObj **)m_varhash.Find(name);
	if(!pvp)
		return(0);

	vp=pvp[0];
	return(vp);
}

void kGUIBasic::SetVar(kGUIBasicVarObj *var,kGUIBasicVarObj *value)
{
	int oldtype;

	if(var->GetIsArray()==true)
		basicerror(ERROR_TRYINGTOSETARRAY);

	oldtype=var->GetType();
	var->Copy(value);

	/* if type is not a variant then force back to previous type */
	if(var->GetIsVariant()==false && oldtype!=var->GetType() && oldtype!=VARTYPE_UNDEFINED && oldtype!=VARTYPE_VARIANT  )
		var->ChangeType(oldtype);
}

/* get pointer to a variable, both single variables or an array entry */
/* traverse arrays, structures and classes to get to the final object */
/* typically the final object is a base type like int,double,boolean or string */
/* or class or structure or an function or subroutine */

kGUIBasicObj *kGUIBasic::GetFinalObj(void)
{
	kGUIBasicObj *obj;
	kGUIBasicVarObj *vobj;

	GetToken();
	obj=m_tokobj;	//GetObj(m_token);

	/* if var names starts with "." then use current "with" */
	if(m_tok==BASICTOKEN_FIELDPREFIX)
	{
		call_def cs;

		cs=m_callstack.GetEntry(m_calldepth-1);
		obj=cs.with;
		Rewind();	/* put '.' back */
	}

	if(!obj)
		basicerror(ERROR_NOTAVARIABLE);

	do
	{
		switch(obj->GetType())
		{
		case VARTYPE_UFUNCTION:
			kGUIBasicFuncObj *fobj;
			call_def cs;

			/* this instance is a bit of a special case */
			/* user functions can set the return value by setting the functionname=value */
			/* they can also reference the return value as a variable and increment, decrement */
			/* and do other regular math operations on it. */
			/* so this function can return either the pointer to the function object itself */
			/* or to the return value, it decides which one to return a pointer to by */
			/* looking to see if the var is followed by an open bracket for the parameters */

			GetToken();
			if(m_tok==BASICTOKEN_OPENBRACKET)
			{
				/* if open bracket is next then return pointer to function object, not */
				/* a pointer to the return value object */
				Rewind();
				return(obj);
			}
			Rewind();

			cs=m_callstack.GetEntry(m_calldepth-1);

			/* make sure that it is the current top function */
			if(cs.func==static_cast<kGUIBasicFlowObj *>(obj))
			{
				fobj=static_cast<kGUIBasicFuncObj *>(obj);
				obj=fobj->m_retval;
			}
			else
				return(obj);	/* an error will be triggered elsewhere */
		break;
		case VARTYPE_INTEGER:
		case VARTYPE_DOUBLE:
		case VARTYPE_STRING:
		case VARTYPE_BOOLEAN:
		case VARTYPE_VARIANT:
		case VARTYPE_CLASS:
		case VARTYPE_STRUCT:
			vobj=static_cast<kGUIBasicVarObj *>(obj);
			if(vobj->GetIsArray()==true)
			{
				int verror=0;
				kGUIBasicIndices indices;

				GetToken();
				if(m_tok!=BASICTOKEN_OPENSQUAREBRACKET)
				{
					Rewind();
					return(obj);
				}
				GetIndices(&indices);
				obj=vobj->GetArrayVar(&indices,&verror);
				if(!obj)
					basicerror(verror);
				Rewind();
			}
			else if(obj->GetType()==VARTYPE_CLASS || obj->GetType()==VARTYPE_STRUCT)
			{
				GetToken();
				if(m_tok!=BASICTOKEN_FIELDPREFIX)
				{
					Rewind();
					return(obj);
				}
				GetToken(true);	/* get name even if it conflicts with a command name */
				if(obj->GetType()==VARTYPE_STRUCT)
				{
					kGUIBasicStructObj *sobj;
					int field;

					sobj=static_cast<kGUIBasicStructObj *>(vobj->GetDefPtr());
					field=sobj->GetFieldIndex(m_token);
					if(field<0)
						basicerror(ERROR_NOTASTRUCTFIELD);
					obj=vobj->GetEntry(field);
				}
				else	/* class */
				{
					kGUIBasicClassObj *cobj;

					cobj=static_cast<kGUIBasicClassObj *>(vobj->GetDefPtr());
					obj=cobj->GetObj(m_token);
					if(!obj)
						basicerror(ERROR_SYNTAX);
				}
			}
			else
				return(obj);	/* base level variable */
		break;
		default:
			return(obj);
		break;
		}
	}while(1);
}

kGUIBasicVarObj *kGUIBasic::ToVar(kGUIBasicObj *obj)
{
	switch(obj->GetType())
	{
	case VARTYPE_INTEGER:
	case VARTYPE_DOUBLE:
	case VARTYPE_STRING:
	case VARTYPE_BOOLEAN:
	case VARTYPE_VARIANT:
	case VARTYPE_CLASS:
	case VARTYPE_STRUCT:
		return(static_cast<kGUIBasicVarObj *>(obj));
	break;
	}
	basicerror(ERROR_NOTAVARIABLE);
	return(0);
}

/* get pointer to a variable, stand alone or array entry */
kGUIBasicVarObj *kGUIBasic::GetVar(void)
{
	return(ToVar(GetFinalObj()));
}

void kGUIBasic::GetVarName(kGUIString *name)
{
	int i,l;
	unsigned char c;

	GetToken(true);	/* give me the name even if it is a command */

	/* if this is a reserved keyword then it will not be set to TT_VARIABLE */
	if(m_token_type!=TT_VARIABLE)
		basicerror(ERROR_NOTAVARIABLE);

	/* make sure this is a valid variablename */

	if(m_alphatable[(unsigned char)m_token[0]]==false)
		basicerror(ERROR_NOTAVARIABLE);
	l=(int)strlen(m_token);
	for(i=1;i<l;++i)
	{
		c=(unsigned char)m_token[i];
		if(c=='$')
		{
			if(i!=(l-1))	/* $ must be last character if present */
				basicerror(ERROR_NOTAVARIABLE);
		}
		else if(m_nametable[c]==false)
			basicerror(ERROR_NOTAVARIABLE);
	}

	name->SetString(m_token);
}

void kGUIBasic::GetVarDef(kGUIString *name,kGUIBasicVarObj *var)
{
	kGUIBasicIndices indices;

	GetVarName(name);

	/* get 'as' */
	
	GetToken();
	if(m_tok==BASICTOKEN_OPENSQUAREBRACKET)
		GetIndices(&indices);

	if(m_tok!=BASICTOKEN_AS)
		basicerror(ERROR_ASEXPECTED);
	GetToken();
	if(var->InitVarType(m_tokobj,&indices)==false)
		basicerror(ERROR_UNKNOWNTYPE);
}

/* parse array indices */
void kGUIBasic::GetIndices(kGUIBasicIndices *indices)
{
	kGUIBasicVarObj result;

	do
	{
		if(m_tok!=BASICTOKEN_OPENSQUAREBRACKET)
			break;

		GetToken();
		if(m_tok==BASICTOKEN_CLOSESQUAREBRACKET)
			indices->AddIndice(0);
		else
		{
			Rewind();
			exp_get(&result);
			indices->AddIndice(result.GetInt());
			GetToken();
			if(m_tok!=BASICTOKEN_CLOSESQUAREBRACKET)
				basicerror(ERROR_CLOSEBRACKETEXPECTED);
		}
		GetToken();
	}while(1);

	if(!indices->GetNumIndices())
		basicerror(ERROR_NOINDICES);		/* zero is not allowed! */
}

void kGUIBasic::RegGlobal(const char *name,kGUIBasicVarObj *var)
{
	kGUIBasicVarObj **pvp;

	var->SetIsGlobal(true);
	pvp=(kGUIBasicVarObj **)m_varhash.Find(name);
	if(pvp)
	{
		if(pvp[0])
			delete pvp[0];	/* free old one */
		pvp[0]=var;		/* replace old one */
	}
	else
	{
		kGUIBasicVarObj *vp=var;

		m_varhash.Add(name,&vp);
	}
}

/* register this local variable and add to the undo list for removal when the function/sub ends */

void kGUIBasic::RegLocal(int depth,const char *name,kGUIBasicVarObj *var,bool dopurge)
{
	kGUIBasicVarObj **pvp;
	kGUIBasicVarObj *prev;
	local_def ld;

	/* if this variable name already exists ( perhaps as a global or in a nested function call) */
	/* then save a pointer to it and put it back when this current function/sub exits */

	pvp=(kGUIBasicVarObj **)m_varhash.Find(name);
	if(pvp)
	{
		prev=pvp[0];
		pvp[0]=var;
	}
	else
	{
		kGUIBasicVarObj *vp=var;

		m_varhash.Add(name,&vp);
		prev=0;
		pvp=(kGUIBasicVarObj **)m_varhash.Find(name);
	}

	/* add undo info to the stack for winding back this variable */
	ld.level=depth;
	ld.varptr=pvp;
	ld.cur=var;			/* only needed for debugging */
	ld.prev=prev;
	ld.purge=dopurge;
	m_lstack.SetEntry(m_numlocals++,ld);
}

void kGUIBasic::DimVar(kGUIString *vn,kGUIBasicVarObj *var)
{
	kGUIBasicIndices indices;

	GetVarName(vn);

	/* get 'as' or '[' */
	
	GetToken();
	if(m_tok==BASICTOKEN_OPENSQUAREBRACKET)
		GetIndices(&indices);

	if(m_tok!=BASICTOKEN_AS)
		basicerror(ERROR_ASEXPECTED);
	GetToken();

	if(var->InitVarType(m_tokobj,&indices)==false)
		basicerror(ERROR_UNKNOWNTYPE);

	CheckEOL();
}

/* init var type to same type as another one */
bool kGUIBasicVarObj::InitVarType(kGUIBasicObj *type,kGUIBasicIndices *indices)
{
	int vtype;
	kGUIBasicObj *defobj;	/* pointer to struct or class where applicable */

	if(!type)
		return(false);
	
	defobj=0;
	if(type->GetType()==VARTYPE_COMMAND)
	{
		kGUIBasicCommandObj *cobj;
		
		cobj= static_cast<kGUIBasicCommandObj *>(type);
		vtype=kGUIBasic::GetTypeFromToken(cobj->GetID());
		if(vtype==-1)
			return(false);
	}
	else if(type->GetType()==VARTYPE_STRUCTDEF)
	{
		vtype=VARTYPE_STRUCT;
		/* pointer to struct definition */
		defobj=type;
	}
	else if(type->GetType()==VARTYPE_SYSCLASSDEF)
	{
		vtype=VARTYPE_CLASS;
		/* pointer to class definition */
		defobj=type;
	}
	else
	{
		kGUIBasicVarObj *vobj;

		vobj= static_cast<kGUIBasicVarObj *>(type);
		if(vobj->GetIsVariant()==true)
			vtype=VARTYPE_VARIANT;
		else
			vtype=type->GetType();
		if(vtype==VARTYPE_STRUCT || vtype==VARTYPE_CLASS)
			defobj=vobj->m_value.def;
	}

	/* this is an array of things */
	if(!indices)
		goto notarray;

	if(indices->GetNumIndices()>0)
	{
		int i,totalentries,numentries;

		if(vtype==VARTYPE_STRUCT || vtype==VARTYPE_CLASS)
			m_value.def=defobj;
		SetType(vtype);

		SetIsArray(true);
		numentries=indices->GetIndice(0);
		AddIndice(numentries);
		totalentries=numentries;
		for(i=1;i<indices->GetNumIndices();++i)
		{
			numentries=indices->GetIndice(i);
			AddIndice(numentries);
			totalentries*=numentries;
		}
		if(totalentries)
		{
			AllocArray(totalentries);
			for(i=0;i<totalentries;++i)
				GetEntry(i)->InitVarType(this,0);
		}
		else
			m_xarrayentries=0;
	}
	else
	{
notarray:;
		switch(vtype)
		{
		case VARTYPE_VARIANT:
			SetIsVariant(true);
			/* fall through to integer */
		case VARTYPE_INTEGER:
			SetType(VARTYPE_INTEGER);
			m_value.i=0;
		break;
		case VARTYPE_STRING:
			SetType(VARTYPE_STRING);
			m_value.s=new kGUIString();
		break;
		case VARTYPE_DOUBLE:
			SetType(VARTYPE_DOUBLE);
			m_value.d=0.0f;
		break;
		case VARTYPE_BOOLEAN:
			SetType(VARTYPE_BOOLEAN);
			m_value.b=true;
		break;
		case VARTYPE_STRUCT:
		{
			int i,ne;
			kGUIBasicVarObj *vfield;
			kGUIBasicStructObj *sobj;

			SetType(VARTYPE_STRUCT);
			m_value.def= defobj;

			sobj=static_cast<kGUIBasicStructObj *>(defobj);
			ne=sobj->GetNumFields();
			
			AllocArray(ne);
			for(i=0;i<ne;++i)
			{
				vfield=sobj->GetFieldType(i);
				GetEntry(i)->InitVarType(vfield,vfield->GetIndicePtr());
			}
		}
		break;
		case VARTYPE_CLASS:
		{
			SetType(VARTYPE_CLASS);

			if(defobj->GetType()==VARTYPE_SYSCLASSDEF)
			{
				kGUIBasicVarObj *vobj;

				/* call the static constructor to allocate the class */
				vobj=static_cast<kGUIBasicVarObj *>(defobj);
				m_value.def=vobj->m_value.allocsysclass();
			}
			else	/* VARTYPE_UCLASSDEF */
			{

			}
		}
		break;
		default:
			return(false);
		break;
		}
	}
	return(true);
}

/* if old was a[10][2][5] */
/* and new is a[5][12][3] */
/* then copy only copies the minimum [5][2][3] from old to new */

int kGUIBasicVarObj::ReDim(kGUIBasicIndices *indices,kGUIBasicObj *newtype,bool preserve)
{
	int i;
	int numentries;
	int totalentries;
	kGUIBasicVarObj oldvar;

	if(preserve==true)
	{
		if(indices->GetNumIndices()!=this->GetNumIndices())
			return(ERROR_CANNOTPRESERVE);

		/* save old indices */
		for(i=0;i<GetNumIndices();++i)
			oldvar.AddIndice(this->GetIndice(i));
	}
	oldvar.m_xarrayentries=m_xarrayentries;

	/* delete old entries if they exist */
		
	SetIsArray(true);
	numentries=indices->GetIndice(0);
	ClearIndices();
	AddIndice(numentries);
	totalentries=numentries;
	for(i=1;i<indices->GetNumIndices();++i)
	{
		numentries=indices->GetIndice(i);
		AddIndice(numentries);
		totalentries*=numentries;
	}
	AllocArray(totalentries);
	for(i=0;i<totalentries;++i)
		GetEntry(i)->InitVarType(newtype,0);

	/* copy minimum set of entries from  old to new */
	if(preserve)
	{
		int ni,ii,ival,err,mini;
		kGUIBasicIndices minindices;
		kGUIBasicIndices copyindices;
		
		ni=oldvar.GetNumIndices();
		for(ii=0;ii<ni;++ii)
		{
			mini=min(oldvar.GetIndice(ii),GetIndice(ii));
			if(!mini)
				goto nocopy;
			minindices.AddIndice(mini);
			copyindices.AddIndice(0);
		}

		oldvar.SetIsArray(true);
		/* start at [0][0][0], then [0][0][1]... */
		do
		{
			GetArrayVar(&copyindices,&err)->Copy(oldvar.GetArrayVar(&copyindices,&err));

			ii=0;
			do
			{
				ival=copyindices.GetIndice(ii)+1;
				if(ival==minindices.GetIndice(ii))
					ival=0;
				copyindices.SetIndice(ii,ival);
				if(ival)
					break;
				if(++ii==ni)
					break;
			}while(1);
		}
		while(ii<ni);
	}
nocopy:;
	return(ERROR_OK);
}

kGUIBasicFuncObj::~kGUIBasicFuncObj()
{
	int i;
	parm_def pa;

	for(i=0;i<m_numparms;++i)
	{
		pa=m_parms.GetEntry(i);
		delete pa.name;
		delete pa.type;
	}
}

void kGUIBasicFuncObj::AddParm(const char *name,kGUIBasicVarObj *var,int bymode)
{
	parm_def pa;

	m_parms.Alloc(m_numparms+1);
	pa.name=new kGUIString();
	pa.name->SetString(name);
	pa.type=var;
	pa.bymode=bymode;
	m_parms.SetEntry(m_numparms++,pa);
}

/*! @internal @struct COMMANDLIST_DEF
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	const char *name;
	int tokenid;
	void (kGUIBasic::*vcodev)(void);
}COMMANDLIST_DEF;

/*! @internal @struct COMMANDVERBLIST_DEF
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	const char *name;
	int tokenid;
}COMMANDVERBLIST_DEF;

/*! @internal @struct OPERATORLIST_DEF
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	const char *name;
	int tokenid;
}OPERATORLIST_DEF;

/*! @internal @struct FUNCLIST_DEF
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	int minnumparms;
	int maxnumparms;
	const char *name;
	void (kGUIBasic::*fcode)(kGUIBasicVarObj *result,CallParms *p);
	const char *parmdef;
}FUNCLIST_DEF;

/*! @internal @struct CONSTLISTINT_DEF
    @brief Internal struct used by the kGUIBasic class */
typedef struct
{
	const char *name;
	int value;
}CONSTLISTINT_DEF;

/* set the color for the range above in the source code */
void kGUIBasic::SetSourceColor(const char *pc1,const char *pc2,kGUIColor c)
{
	RICHINFO_DEF *ri;
	unsigned int i,si,ei;

	si=(int)(pc1-m_source->GetString());
	ei=(int)(pc2-m_source->GetString());
	
	for(i=si;i<ei;++i)
	{
		ri=m_source->GetRichInfoPtr(i);
		ri->fcolor=c;
	}
}

/* return true if this prefix verb is in the list and if so remove it */
bool kGUIBasic::IsPreVerb(int token)
{
	int i;

	for(i=0;i<m_numpreverbs;++i)
	{
		if(m_preverbs.GetEntry(i).pc->num==token)
		{
			m_preverbs.DeleteEntry(i);
			--m_numpreverbs;
			return(true);
		}
	}
	return(false);
}

/* compile the code */
bool kGUIBasic::Compile(kGUIText *source,kGUIString *output)
{
	unsigned int i;
	int level;
	int leveltype;
	int clevel;
	int clast;
	kGUIString sfname;	/* sub or function name */
	kGUIString vn;
	Array<kGUIBasicFlowObj *>condnames;
	kGUIBasicCommandObj *cobj;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *pfobj;
	kGUIBasicVarObj *vobj;
	kGUIBasicStructObj *structobj=0;
	kGUIBasicUClassObj *classobj=0;
	kGUIEnumObj *enumobj=0;
	const char *spc;

	static COMMANDLIST_DEF commandlist[]={
	{"sub",		BASICTOKEN_SUB,		&kGUIBasic::cmd_null},	/* never called at runtime */
	{"function",BASICTOKEN_FUNCTION,&kGUIBasic::cmd_null},	/* never called at runtime */
	{"const",	BASICTOKEN_CONST,	&kGUIBasic::cmd_null},	/* never called at runtime */
	{"type",	BASICTOKEN_TYPE,	&kGUIBasic::cmd_null},	/* never called at runtime */
	{"class",	BASICTOKEN_CLASS,	&kGUIBasic::cmd_null},	/* never called at runtime */
	{"enum",	BASICTOKEN_ENUM,	&kGUIBasic::cmd_null},	/* never called at runtime */
	{"if",		BASICTOKEN_IF,		&kGUIBasic::cmd_if},
	{"elseif",	BASICTOKEN_ELSEIF,	&kGUIBasic::cmd_else},
	{"else",	BASICTOKEN_ELSE,	&kGUIBasic::cmd_else},
	{"endif",	BASICTOKEN_ENDIF,	&kGUIBasic::cmd_endif},
	{"do",		BASICTOKEN_DO,		&kGUIBasic::cmd_do},
	{"loop",	BASICTOKEN_LOOP,	&kGUIBasic::cmd_loop},
	{"while",	BASICTOKEN_WHILE,	&kGUIBasic::cmd_while},
	{"wend",	BASICTOKEN_WEND,	&kGUIBasic::cmd_wend},
	{"for",		BASICTOKEN_FOR,		&kGUIBasic::cmd_for},
	{"next",	BASICTOKEN_NEXT,	&kGUIBasic::cmd_next},
	{"select",	BASICTOKEN_SELECT,	&kGUIBasic::cmd_select},
	{"case",	BASICTOKEN_CASE,	&kGUIBasic::cmd_case},
	{"print",	BASICTOKEN_PRINT,	&kGUIBasic::cmd_print},
	{"input",	BASICTOKEN_INPUT,	&kGUIBasic::cmd_input},
	{"inputbox",BASICTOKEN_INPUTBOX,&kGUIBasic::cmd_inputbox},
	{"goto",	BASICTOKEN_GOTO,	&kGUIBasic::cmd_goto},
	{"gosub",	BASICTOKEN_GOSUB,	&kGUIBasic::cmd_call},
	{"call",	BASICTOKEN_CALL,	&kGUIBasic::cmd_call},
	{"open",	BASICTOKEN_OPEN,	&kGUIBasic::cmd_open},
	{"close",	BASICTOKEN_CLOSE,	&kGUIBasic::cmd_close},
	{"return",	BASICTOKEN_RETURN,	&kGUIBasic::cmd_return},
	{"let",		BASICTOKEN_LET,		&kGUIBasic::cmd_set},
	{"set",		BASICTOKEN_SET,		&kGUIBasic::cmd_set},
	{"dim",		BASICTOKEN_DIM,		&kGUIBasic::cmd_dim},
	{"redim",	BASICTOKEN_REDIM,	&kGUIBasic::cmd_redim},
	{"exit",	BASICTOKEN_EXIT,	&kGUIBasic::cmd_exit},
	{"on",		BASICTOKEN_ON,		&kGUIBasic::cmd_on},
	{"with",	BASICTOKEN_WITH,	&kGUIBasic::cmd_with},
	{"end",		BASICTOKEN_END,		&kGUIBasic::cmd_end}};

	/* command verbs */
	static COMMANDVERBLIST_DEF commandverblist[]={
//	{"while",	BASICTOKEN_WHILE},
	{"until",	BASICTOKEN_UNTIL},
	{"then",	BASICTOKEN_THEN},
	{"as",		BASICTOKEN_AS},
	{"#",		BASICTOKEN_NUMBER},
	{"public",	BASICTOKEN_PUBLIC},
	{"line",	BASICTOKEN_LINE},
	{"string",	BASICTOKEN_STRING},
	{"integer",	BASICTOKEN_INTEGER},
	{"double",	BASICTOKEN_DOUBLE},
	{"variant",	BASICTOKEN_VARIANT},
	{"error",	BASICTOKEN_ERROR},
	{"output",	BASICTOKEN_OUTPUT},
	{"binary",	BASICTOKEN_BINARY},
	{"access",	BASICTOKEN_ACCESS},
	{"read",	BASICTOKEN_READ},
	{"write",	BASICTOKEN_WRITE},
	{"append",	BASICTOKEN_APPEND},
	{"byval",	BASICTOKEN_BYVAL},
	{"byref",	BASICTOKEN_BYREF},
	{"step",	BASICTOKEN_STEP},
	{"preserve",BASICTOKEN_PRESERVE},
	{"is",		BASICTOKEN_IS},
	{"resume",	BASICTOKEN_RESUME},
	{"to",		BASICTOKEN_TO}};

	/* expression operators */
	static OPERATORLIST_DEF operatorlist[]={
	{":",	BASICTOKEN_EOL},
	{"+",	BASICTOKEN_ADD},
	{"-",	BASICTOKEN_SUBTRACT},
	{"++",	BASICTOKEN_INCREMENT},
	{"--",	BASICTOKEN_DECREMENT},
	{"+=",	BASICTOKEN_ADDTO},
	{"-=",	BASICTOKEN_SUBTRACTFROM},
	{"*",	BASICTOKEN_MULTIPLY},
	{"/",	BASICTOKEN_DIVIDE},
	{"%",	BASICTOKEN_MODULO},
	{"mod",	BASICTOKEN_MODULO},
	{"==",	BASICTOKEN_TESTEQUAL},
	{"!=",	BASICTOKEN_TESTNOTEQUAL},
	{"<>",	BASICTOKEN_TESTNOTEQUAL},
	{"<",	BASICTOKEN_TESTLESSTHAN},
	{"<=",	BASICTOKEN_TESTLESSTHANOREQUAL},
	{">",	BASICTOKEN_TESTGREATERTHAN},
	{">=",	BASICTOKEN_TESTGREATERTHANOREQUAL},
	{"=",	BASICTOKEN_EQUALS},
	{".",	BASICTOKEN_FIELDPREFIX},
	{"?",	BASICTOKEN_QUESTIONMARK},
	{"not",	BASICTOKEN_NOT},
	{"!",	BASICTOKEN_NOT},
	{"~",	BASICTOKEN_NOT},
	{",",	BASICTOKEN_COMMA},
	{";",	BASICTOKEN_SEMICOLON},
	{"(",	BASICTOKEN_OPENBRACKET},
	{")",	BASICTOKEN_CLOSEBRACKET},
	{"&&",	BASICTOKEN_LOGICALAND},
	{"||",	BASICTOKEN_LOGICALOR},
	{"and",	BASICTOKEN_LOGICALAND},
	{"or",	BASICTOKEN_LOGICALOR},
	{"&",	BASICTOKEN_BITWISEAND},
	{"|",	BASICTOKEN_BITWISEOR},
	{"^",	BASICTOKEN_BITWISEEXCLUSIVEOR},
	{"xor",	BASICTOKEN_BITWISEEXCLUSIVEOR},
	{"<<",	BASICTOKEN_BITWISESHIFTLEFT},
	{">>",	BASICTOKEN_BITWISESHIFTRIGHT},
	{"**",	BASICTOKEN_EXPONENT},
	{"^^",	BASICTOKEN_EXPONENT},
	{"[",	BASICTOKEN_OPENSQUAREBRACKET},
	{"]",	BASICTOKEN_CLOSESQUAREBRACKET}};

	/* functions */
	static FUNCLIST_DEF funclist[]={
	{0,0,"rand",&kGUIBasic::func_rand,""},
	{1,1,"len",&kGUIBasic::func_len,"byval string"},
	{1,1,"asc",&kGUIBasic::func_asc,"byval string"},
	{1,1,"val",&kGUIBasic::func_val,"byval string"},
	{1,1,"dval",&kGUIBasic::func_dval,"byval string"},
	{1,1,"chr",&kGUIBasic::func_chr,"byval integer"},
	{1,1,"chr$",&kGUIBasic::func_chr,"byval integer"},
	{1,1,"str",&kGUIBasic::func_str,"byval variant"},
	{1,1,"str$",&kGUIBasic::func_str,"byval variant"},
	{1,1,"sin",&kGUIBasic::func_sin,"byval double"},
	{1,1,"cos",&kGUIBasic::func_cos,"byval double"},
	{1,1,"tan",&kGUIBasic::func_tan,"byval double"},
	{1,1,"asin",&kGUIBasic::func_asin,"byval double"},
	{1,1,"acos",&kGUIBasic::func_acos,"byval double"},
	{1,1,"atan",&kGUIBasic::func_atan,"byval double"},
	{1,1,"eof",&kGUIBasic::func_eof,"byval integer"},
	{1,1,"trim",&kGUIBasic::func_trim,"byval string"},
	{1,1,"fileexists",&kGUIBasic::func_fileexists,"byval string"},

	{2,2,"msgbox",&kGUIBasic::func_msgbox,"byval string,byval integer"},
	{2,2,"left",&kGUIBasic::func_left,"byval string,byval integer"},
	{2,2,"left$",&kGUIBasic::func_left,"byval string,byval integer"},
	{2,2,"right",&kGUIBasic::func_right,"byval string,byval integer"},
	{2,2,"right$",&kGUIBasic::func_right,"byval string,byval integer"},
	{2,2,"input",&kGUIBasic::func_input,"byval integer,byval integer"},

	{3,3,"iif",&kGUIBasic::func_iif,"byval boolean,byval variant,byval variant"},
	{3,3,"filereq",&kGUIBasic::func_filereq,"byval integer,byval string,byval string"},
	{1,3,"shellexec",&kGUIBasic::func_shellexec,"byval string,byval string,byval string"},

	/* variable # parms */
	{2,3,"mid$",&kGUIBasic::func_mid,"byval string,byval integer,byval integer"},
	{2,3,"mid",&kGUIBasic::func_mid,"byval string,byval integer,byval integer"},
	{1,2,"ubound",&kGUIBasic::func_ubound,"byref string,byval integer"},
	{3,4,"instr",&kGUIBasic::func_instr,"byval integer,byval string,byval string,byval integer"},
	{3,6,"replace",&kGUIBasic::func_replace,"byval string,byval string,byval string,byval integer,byval integer,byval integer"},
	{2,3,"split",&kGUIBasic::func_split,"byval string,byval string,byval integer"},

	{3,3,"sortobjects",&kGUIBasic::func_sortobjects,"byref variant,byval integer,byval string"}};

	/* integer constants */
	static CONSTLISTINT_DEF constlistint[]={
	{"MSGBOX_ABORT",MSGBOX_ABORT},	/* when window is closed without a button pressed */
	{"MSGBOX_NO",MSGBOX_NO},
	{"MSGBOX_YES",MSGBOX_YES},
	{"MSGBOX_CANCEL",MSGBOX_CANCEL},
	{"MSGBOX_DONE",MSGBOX_DONE},
	{"MSGBOX_OK",MSGBOX_OK},
	{"FILEREQ_OPEN",FILEREQ_OPEN},
	{"FILEREQ_SAVE",FILEREQ_SAVE}};

	if(setjmp(m_ebuf))
	{
		/* if a compile error is encountered, then go here */
		return(false);
	}

//	m_thread=0;
	m_asyncactive=false;
	m_compile=true;
	m_source=source;
	m_output=output;

	m_source->InitRichInfo();	/* default list to black/white for all characters */

	PurgeVars();
	m_varhash.Init(16,sizeof(kGUIBasicVarObj *));

	/* add commands to the hashtable */

	for(i=0;i<sizeof(commandlist)/sizeof(COMMANDLIST_DEF);++i)
	{
		cobj=new kGUIBasicCommandObj();
		cobj->SetType(VARTYPE_COMMAND);
		cobj->SetID(commandlist[i].tokenid);
		cobj->SetCode(commandlist[i].vcodev);
		RegObj(commandlist[i].name,cobj);
	}

	/* add command verbs to the hash table */
	for(i=0;i<sizeof(commandverblist)/sizeof(COMMANDVERBLIST_DEF);++i)
	{
		cobj=new kGUIBasicCommandObj();
		cobj->SetType(VARTYPE_COMMAND);
		cobj->SetID(commandverblist[i].tokenid);
		RegObj(commandverblist[i].name,cobj);
	}

	/* add operators to the hash table */
	for(i=0;i<sizeof(operatorlist)/sizeof(OPERATORLIST_DEF);++i)
	{
		cobj=new kGUIBasicCommandObj();
		cobj->SetType(VARTYPE_OPERATOR);
		cobj->SetID(operatorlist[i].tokenid);
		RegObj(operatorlist[i].name,cobj);
	}

	/* add functions */
	for(i=0;i<sizeof(funclist)/sizeof(FUNCLIST_DEF);++i)
	{
		kGUIBasicObj *obj;
		cobj=new kGUIBasicCommandObj();
		cobj->SetType(VARTYPE_CFUNCTION);
		cobj->SetMinNumParms(funclist[i].minnumparms);
		cobj->SetMaxNumParms(funclist[i].maxnumparms);
		cobj->SetCode(funclist[i].fcode);
		cobj->SetParmDef(funclist[i].parmdef);

		/* special case, name used for both command and function? */
		obj=GetObj(funclist[i].name);
		if(!obj)
			RegObj(funclist[i].name,cobj);
		else
			(static_cast<kGUIBasicCommandObj *>(obj))->SetExtra(cobj);
	}

	/* add constants with integer values */
	for(i=0;i<sizeof(constlistint)/sizeof(CONSTLISTINT_DEF);++i)
	{
		vobj=new kGUIBasicVarObj();
		vobj->Set(constlistint[i].value);
		vobj->SetIsConstant(true);
		vobj->SetIsUndefined(false);
		RegObj(constlistint[i].name,vobj);
	}

	/* true */
	vobj=new kGUIBasicVarObj();
	vobj->Set(true);
	vobj->SetIsConstant(true);
	vobj->SetIsUndefined(false);
	RegObj("True",vobj);

	/* false */
	vobj=new kGUIBasicVarObj();
	vobj->Set(false);
	vobj->SetIsConstant(true);
	vobj->SetIsUndefined(false);
	RegObj("False",vobj);

	/* adds the built in classes for things like dates, mysql and other classes */

	AddBuiltInClasses();

	/* add app specific objects to the variables list */
	/* mostly used for "C" Classes to access APP data but can also add */
	/* globals, consts or APP specific functions too */

	m_addappobjectscallback.Call();

	/* build list of tokens from source code */
	m_numbtokens=0;
	spc=source->GetString();
	do
	{
		BTOK_DEF bt;
		const char *tstart;

		spc=GetSourceToken(&tstart,spc);
		bt.type=m_token_type;
		bt.num=m_tok;
		bt.obj=m_tokobj;
		bt.fobj=0;
		bt.source=tstart;
		bt.len=(int)(spc-tstart);
		m_btoklist.SetEntry(m_numbtokens++,bt);
	}while(m_tok!=BASICTOKEN_FINISHED);

	m_numpreverbs=0;
	condnames.Alloc(32);
	condnames.SetGrow(true);
	level=0;		/* global=0, 1=sub/function */
	leveltype=0;	/* type of object, function, sub etc */
	clevel=0;	/* condition index level ( if/do/loop/for etc.) */

	/* if no source then we are done */
	if(!source->GetLen())
		return(true);

	/* since array is all allocated and can't be moved, we can just use */
	/* a pointer into it for speed */

	m_pc=m_btoklist.GetArrayPtr();
//	m_pc=source->GetString();
	/* iterate through the source and syntax check */
	do {
		GetToken();
		if(m_token_type==TT_LABEL)
		{
			if(!level)
				basicerror(ERROR_LABELERROR);
			else
			{
				/* generate a unique label name using the function name and the label name */
				vn.Sprintf("%s:%s",sfname.GetString(),m_token);

				if(GetObj(vn.GetString()))
					basicerror(ERROR_LABELERROR);

				fobj=new kGUIBasicFlowObj();
				fobj->SetType(VARTYPE_LABEL);
				fobj->SetAddr(m_pc);
				
				if(RegObj(vn.GetString(),fobj,false)==false)
					basicerror(ERROR_SYNTAX);	/* already defined */
				GetToken();
			}
		}
		/* grab verbs */
		while(m_token_type==TT_COMMANDVERB)
		{
			preverb_def pv;

//			pv.token=m_tok;
			pv.pc=m_lastpc;
//			pv.tpc=m_tpc;
			/* save list of prefix verbs */
			m_preverbs.SetEntry(m_numpreverbs++,pv);
			/* todo: save verbs in list */
			GetToken();
		}

		/* in global space? */
//		Print("compile token='%s'\n",m_token);
		if(!level)
		{
			if(m_token_type==TT_COMMAND)
			{
				switch(m_tok)
				{
				case BASICTOKEN_DIM:
				{
					kGUIBasicVarObj *var;
					kGUIString vn;

					var=new kGUIBasicVarObj();
					AddLeak(var);
					DimVar(&vn,var);
					RemoveLeak(var);
					RegGlobal(vn.GetString(),var);
				}
				break;
				case BASICTOKEN_CONST:
				{
					kGUIString vn;
					kGUIBasicVarObj *var;

					var=new kGUIBasicVarObj();
					AddLeak(var);
					GetVarName(&vn);
						
					/* get the equals sign */
					GetToken();
					if(m_tok!=BASICTOKEN_EQUALS)
						basicerror(ERROR_EQUALSIGNEXPECTED);
					
					/* take whatever type the expression is */
					var->SetIsVariant(true);
					exp_get(var);
					/* lock to  that type */
					var->SetIsVariant(false);

					RemoveLeak(var);
					RegGlobal(vn.GetString(),var);
					var->SetIsConstant(true);
					var->SetIsUndefined(false);

					/* is it public? */
					if(IsPreVerb(BASICTOKEN_PUBLIC)==true)
						var->SetIsGlobal(true);
				}
				break;
				case BASICTOKEN_SUB:
				case BASICTOKEN_FUNCTION:
				{
					kGUIBasicFuncObj *funcobj;

					leveltype=m_tok;
					level=1;

					GetToken();
					/* todo, syntax check to make sure it is a valid name */
					sfname.SetString(m_token);

					funcobj=new kGUIBasicFuncObj();
					if(leveltype==BASICTOKEN_SUB)
						funcobj->SetType(VARTYPE_USUB);
					else
						funcobj->SetType(VARTYPE_UFUNCTION);

					funcobj->SetName(m_token);
					if(IsPreVerb(BASICTOKEN_PUBLIC)==true)
						funcobj->SetIsPublic(true);

					if(RegObj(m_token,funcobj,false)==false)
						basicerror(ERROR_SYNTAX);	/* varname already used/defined */

					GetToken();
					if(m_tok==BASICTOKEN_EOL)
					{
						/* if EOL then sub/function takes no parms */
						/* also if EOL, assume function returns an integer */
						Rewind();
					}
					else
					{
						if(m_tok!=BASICTOKEN_OPENBRACKET)
							basicerror(ERROR_OPENBRACKETEXPECTED);
						do
						{
							int bymode;
							kGUIBasicVarObj *var;
							kGUIString varname;

							GetToken();
							if(m_tok==BASICTOKEN_CLOSEBRACKET)
								break;
							if(!funcobj->GetNumParms())
								Rewind();
							else
							{
								if(m_tok!=BASICTOKEN_COMMA)
									basicerror(ERROR_COMMAEXPECTED);
							}

							/* default to byval if none specified */
							bymode=BASICTOKEN_BYVAL;	
							/* is there a byval or byref prefix? */
							GetToken();
							if(m_tok==BASICTOKEN_BYVAL || m_tok==BASICTOKEN_BYREF)
								bymode=m_tok;
							else
								Rewind();

							var=new kGUIBasicVarObj();
							AddLeak(var);
							GetVarDef(&varname,var);
							
							/* arrays are always passed by reference */
							if(var->GetIsArray()==true)
								bymode=BASICTOKEN_BYREF;
							funcobj->AddParm(varname.GetString(),var,bymode);
							RemoveLeak(var);
						}while(1);
						if(m_tok!=BASICTOKEN_CLOSEBRACKET)
							basicerror(ERROR_CLOSEBRACKETEXPECTED);
						
						/* get return type for function */
						if(leveltype==BASICTOKEN_FUNCTION)
						{
							GetToken();
							if(m_tok!=BASICTOKEN_AS)
								basicerror(ERROR_ASEXPECTED);
							GetToken();
							if(funcobj->m_rettype.InitVarType(m_tokobj,0)==false)
								basicerror(ERROR_UNKNOWNTYPE);
						}
					}
					funcobj->SetAddr(m_pc);
				}
				break;
				case BASICTOKEN_TYPE:
					/* define a structure */
					leveltype=m_tok;
					level=1;

					GetToken();
					/* todo, syntax check to make sure it is a valid type name */
					sfname.SetString(m_token);

					structobj=new kGUIBasicStructObj();
					structobj->SetType(VARTYPE_STRUCTDEF);
					if(RegObj(m_token,structobj,false)==false)
						basicerror(ERROR_SYNTAX);	/* varname already used/defined */

					/* is it public? */
					if(IsPreVerb(BASICTOKEN_PUBLIC)==true)
						structobj->SetIsPublic(true);
				break;
				case BASICTOKEN_CLASS:
					/* define a class */
					leveltype=m_tok;
					level=1;

					GetToken();
					/* todo, syntax check to make sure it is a valid class name */
					sfname.SetString(m_token);

					classobj=new kGUIBasicUClassObj();
					classobj->SetType(VARTYPE_UCLASSDEF);
					if(RegObj(m_token,classobj,false)==false)
						basicerror(ERROR_SYNTAX);	/* varname already used/defined */

				break;
				case BASICTOKEN_ENUM:
					/* define a enum list */
					leveltype=m_tok;
					level=1;
						
					GetToken();
					/* todo, syntax check to make sure it is a valid type name */
					sfname.SetString(m_token);

					enumobj=new kGUIEnumObj();

					GetToken();
					if(m_tok==BASICTOKEN_AS)
					{
						GetToken();
						enumobj->SetVarType(m_tokobj);						
					}
					else
						enumobj->SetVarType(GetObj("Integer"));

					enumobj->SetType(VARTYPE_ENUMDEF);
					if(RegObj(sfname.GetString(),enumobj,false)==false)
						basicerror(ERROR_SYNTAX);	/* varname already used/defined */

					/* is it public? */
					if(IsPreVerb(BASICTOKEN_PUBLIC)==true)
						enumobj->SetIsPublic(true);
				break;
				default:
					basicerror(ERROR_COMMANDNOTVALIDASGLOBAL);	/* command is not valid outside of function or subroutine */
				break;
				}
			}
			else if(m_token[0]!=10 && m_token[0]!='\t' && m_token[0]!=0)
				basicerror(ERROR_NOTACOMMAND);
		}
		else
		{
			/* are we in a structure definition? */
			if(leveltype==BASICTOKEN_ENUM)
			{
				if(m_token_type==TT_COMMAND)
				{
					if(m_tok==BASICTOKEN_END)
					{
						GetToken();
						if(leveltype!=m_tok)
							basicerror(ERROR_ENDTYPEMISMATCH);	/* end type does not match current type */
						level=0;
						leveltype=0;
					}
					else
						basicerror(ERROR_SYNTAX);	/* no commands allowed inside a structure definition */
				}
				else
				{
					kGUIBasicIndices ind;
					kGUIBasicVarObj *var=new kGUIBasicVarObj();
					kGUIString *vn=new kGUIString();

					GetToken();	/* get name */
					vn->SetString(m_token);
					GetToken();	/* '=' */
					if(m_tok==BASICTOKEN_EQUALS)
					{
						AddLeak(var);
						var->InitVarType(enumobj->GetVarType(),&ind);
						exp_get(var);
						RemoveLeak(var);
						enumobj->SetLastValue(var->GetInt()+1);
					}
					else
					{
						var->Set(enumobj->GetLastValue());
						enumobj->SetLastValue(enumobj->GetLastValue()+1);
					}

					var->SetIsConstant(true);
					var->SetIsUndefined(false);

					/* add enum to the current enumlist */
					enumobj->AddField(vn,var);
					/* register it as a global enum */
					if(RegObj(vn->GetString(),var,false)==false)
						basicerror(ERROR_SYNTAX);	/* varname already used/defined */
				}
			}
			/* are we in an type definition? */
			else if(leveltype==BASICTOKEN_TYPE)
			{
				if(m_token_type==TT_COMMAND)
				{
					if(m_tok==BASICTOKEN_END)
					{
						GetToken();
						if(leveltype!=m_tok)
							basicerror(ERROR_ENDTYPEMISMATCH);	/* end type does not match current type */
						level=0;
						leveltype=0;
					}
					else
						basicerror(ERROR_SYNTAX);	/* no commands allowed inside a enum definition */
				}
				else
				{
					kGUIBasicVarObj *var;
					kGUIString *vn;

					var=new kGUIBasicVarObj();
					vn=new kGUIString();
					AddLeak(var);

					/* type [= value] */
					Rewind();

					DimVar(vn,var);
					RemoveLeak(var);
					/* hmm, leak string */
					structobj->AddField(vn,var);	/* add field to the current struct */
				}
			}
			/* are we in a class definition? */
			else if(leveltype==BASICTOKEN_CLASS)
			{
				if(m_token_type==TT_COMMAND)
				{
					if(m_tok==BASICTOKEN_END)
					{
						GetToken();
						if(leveltype!=m_tok)
							basicerror(ERROR_ENDTYPEMISMATCH);	/* end type does not match current type */
						level=0;
						leveltype=0;
					}
					else
						basicerror(ERROR_SYNTAX);	/* no commands allowed inside a structure definition */
				}
				else
				{
					kGUIBasicVarObj *var;
					kGUIString *vn;

					/* todo: hmmm, handle both function defs, and public/private keywords */
					var=new kGUIBasicVarObj();
					vn=new kGUIString();
					AddLeak(var);
					Rewind();
					DimVar(vn,var);
					RemoveLeak(var);
					/* hmm, leak string */
					classobj->AddField(vn,var);	/* add field to the current class */
				}
			}
			else
			{
				if(m_token_type==TT_COMMAND)
				{
					switch(m_tok)
					{
					case BASICTOKEN_INPUT:
						IsPreVerb(BASICTOKEN_LINE);	/* eat the "line" prefix so it won't trigger an error */
					break;
					case BASICTOKEN_END:

						/* end sub, function or select */

						GetToken();
						if(m_tok==BASICTOKEN_WITH)
						{
							/* do nothing */
						}
						else if(m_tok==BASICTOKEN_SELECT)
						{
							if(!clevel)
								basicerror(ERROR_ENDSELECTWITHOUTSELECT);

							pfobj=condnames.GetEntry(clevel-1);	/* parent */
							if(pfobj->GetType()!=VARTYPE_SELECT)
								basicerror(ERROR_ENDSELECTWITHOUTSELECT);

							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_ENDSELECT);
							fobj->SetAddr(m_pc);
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;

							/* add me to my parents list */
							pfobj->AddChild(fobj);
							/* add my parent to my list */
							fobj->AddChild(pfobj);
							--clevel;
						}
						else
						{
							if(m_tok==BASICTOKEN_IF)
								goto isendif;

							/* must be sub or function */
							if(leveltype!=m_tok)
								basicerror(ERROR_ENDTYPEMISMATCH);	/* end type does not match current type */

							/* make sure for/if/do stack is empty! */

							if(clevel)
							{
								pfobj=condnames.GetEntry(clevel-1);	/* get pointer to thingy */
								m_pc=pfobj->GetAddr();
								basicerror(ERROR_UNTERMINATEDCOMMAND);
							}

							level=0;
							leveltype=0;
						}
					break;
					case BASICTOKEN_IF:
					{
						if(skipto(BASICTOKEN_THEN)==false)
							basicerror(ERROR_THENEXPECTED);

						/* if EOL is after the "then"  then this is a multi line "if" */
						/* otherwise it is a single line "if" and no other else,elseif or endif commands are valid */
						GetToken();
						if(m_tok==BASICTOKEN_EOL)
						{
							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_IF);
							fobj->SetAddr(m_pc);
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;

							condnames.SetEntry(clevel,fobj);
							++clevel;
						}
					}
					break;
					case BASICTOKEN_ELSEIF:
						if(!clevel)
							basicerror(ERROR_ELSEIFWITHOUTIF);

						/* make sure "else" is not already in list */
						
						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_IF)
							basicerror(ERROR_ELSEIFWITHOUTIF);

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_ELSEIF);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
					break;
					case BASICTOKEN_ELSE:
					{
						int type=VARTYPE_ELSE;

						if(!clevel)
							basicerror(ERROR_ELSEWITHOUTIF);

						/* make sure "else" is not already in list */

						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_IF)
							basicerror(ERROR_ELSEWITHOUTIF);

						GetToken();
						if(m_tok==BASICTOKEN_IF)
							type=VARTYPE_ELSEIF;
						else
							Rewind();

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(type);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
					}
					break;
					case BASICTOKEN_ENDIF:
isendif:				if(!clevel)
							basicerror(ERROR_ENDIFWITHOUTIF);

						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_IF)
							basicerror(ERROR_ENDIFWITHOUTIF);

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_ENDIF);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
						--clevel;
					break;
					case BASICTOKEN_SELECT:
						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_SELECT);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						condnames.SetEntry(clevel,fobj);
						++clevel;
					break;
					case BASICTOKEN_CASE:
						if(!clevel)
							basicerror(ERROR_CASEWITHOUTSELECT);

						pfobj=condnames.GetEntry(clevel-1);		/* parent */
						if(pfobj->GetType()!=VARTYPE_SELECT)
							basicerror(ERROR_CASEWITHOUTSELECT);

						fobj=new kGUIBasicFlowObj();
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						GetToken();
						if(m_tok==BASICTOKEN_ELSE)
						{
							fobj->SetType(VARTYPE_CASEELSE);
							CheckEOL();
						}
						else
							fobj->SetType(VARTYPE_CASE);

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
					break;
					case BASICTOKEN_DO:
						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_DO);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						condnames.SetEntry(clevel,fobj);
						++clevel;
					break;
					case BASICTOKEN_WHILE:
						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_WHILE);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						condnames.SetEntry(clevel,fobj);
						++clevel;
					break;
					case BASICTOKEN_LOOP:
						if(!clevel)
							basicerror(ERROR_LOOPWITHOUTDO);

						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_DO)
							basicerror(ERROR_LOOPWITHOUTDO);

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_LOOP);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
						--clevel;
					break;
					case BASICTOKEN_WEND:
						if(!clevel)
							basicerror(ERROR_WENDWITHOUTWHILE);

						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_WHILE)
							basicerror(ERROR_WENDWITHOUTWHILE);

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_WEND);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
						--clevel;
					break;
					case BASICTOKEN_FOR:

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_FOR);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						condnames.SetEntry(clevel,fobj);
						++clevel;
					break;
					case BASICTOKEN_NEXT:
						if(!clevel)
							basicerror(ERROR_NEXTWITHOUTFOR);

						pfobj=condnames.GetEntry(clevel-1);	/* parent */
						if(pfobj->GetType()!=VARTYPE_FOR)
							basicerror(ERROR_NEXTWITHOUTFOR);

						fobj=new kGUIBasicFlowObj();
						fobj->SetType(VARTYPE_NEXT);
						fobj->SetAddr(m_pc);
						((BTOK_DEF *)(m_lastpc))->fobj=fobj;

						/* add me to my parents list */
						pfobj->AddChild(fobj);
						/* add my parent to my list */
						fobj->AddChild(pfobj);
						--clevel;
					break;
					case BASICTOKEN_EXIT:
						/* todo, generate pointer to head object */
						GetToken();
						switch(m_tok)
						{
						case BASICTOKEN_DO:
							/* find previous do */
							clast=-1;
							for(i=clevel-1;i>=0;--i)
							{
								if(condnames.GetEntry(i)->GetType()==VARTYPE_DO)
								{
									clast=i;
									break;
								}
							}
							if(clast<0)
								basicerror(ERROR_NODOTOEXIT);
							pfobj=condnames.GetEntry(clast);	/* pointer to last do */

							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_EXIT);
							fobj->SetAddr(m_pc);
							fobj->AddChild(pfobj);	/* add parent pointer */
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;
						break;
						case BASICTOKEN_WHILE:
							/* find previous while */
							clast=-1;
							for(i=clevel-1;i>=0;--i)
							{
								if(condnames.GetEntry(i)->GetType()==VARTYPE_WHILE)
								{
									clast=i;
									break;
								}
							}
							if(clast<0)
								basicerror(ERROR_NOWHILETOEXIT);
							pfobj=condnames.GetEntry(clast);	/* pointer to last do */

							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_EXIT);
							fobj->SetAddr(m_pc);
							fobj->AddChild(pfobj);	/* add parent pointer */
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;
						break;
						case BASICTOKEN_FOR:
							/* find previous for */
							clast=-1;
							for(i=clevel-1;i>=0;--i)
							{
								if(condnames.GetEntry(i)->GetType()==VARTYPE_FOR)
								{
									clast=i;
									break;
								}
							}
							if(clast<0)
								basicerror(ERROR_NOFORTOEXIT);
							pfobj=condnames.GetEntry(clast);	/* pointer to last do */

							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_EXIT);
							fobj->SetAddr(m_pc);
							fobj->AddChild(pfobj);	/* add parent pointer */
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;
						break;
						case BASICTOKEN_FUNCTION:
						case BASICTOKEN_SUB:
							if(leveltype!=m_tok)	/* needs to match current thingy type */
								basicerror(ERROR_EXITTYPEMISMATCH);

							fobj=new kGUIBasicFlowObj();
							fobj->SetType(VARTYPE_EXIT);
							fobj->SetAddr(m_pc);
							/* pointer to sub/function */
							fobj->AddChild(GetFObj(sfname.GetString()));
							((BTOK_DEF *)(m_lastpc))->fobj=fobj;
						break;
						default:
							basicerror(ERROR_EXITTYPENOTVALID);	/* exit bad */
						break;
						}
					break;
					}
				}
			}
		}
		if(m_numpreverbs>0)
		{
			preverb_def pv;

			pv=m_preverbs.GetEntry(0);
			m_pc=pv.pc;
//			m_tpc=pv.tpc;
			basicerror(ERROR_SYNTAX);	/* unused prefix */
		}
		if(m_tok!=BASICTOKEN_EOL)
			SkipToEOL();
	}while (m_tok != BASICTOKEN_FINISHED);
	return(true);
}

/* fill user supplied array with public subnames and offsets */
/* only add public subs that take no parameters */

int kGUIBasic::GetPublicSubs(ClassArray<kGUIString> *subnames)
{
	int numpubicsubs=0;
	int nv=m_varhash.GetNum();
	int i;
	HashEntry *he;
	kGUIBasicObj *obj;
	kGUIBasicFuncObj *fobj;

	/* allocate the list and allow it to grow as needed */
	subnames->Init(32,16);

	/*  purge all variables */
	he=m_varhash.GetFirst();
	for(i=0;i<nv;++i)
	{
		obj=*((kGUIBasicObj **)(he->m_data));
		if(obj)
		{
			if(obj->GetType()==VARTYPE_USUB)
			{
				fobj = static_cast<kGUIBasicFuncObj *>(obj);
				if(fobj->GetNumParms()==0)
					subnames->GetEntryPtr(numpubicsubs++)->SetString(he->m_string);
			}
		}
		he=he->GetNext();
	}
	return(numpubicsubs);
}

void kGUIBasic::Start(const char *subname,bool async)
{
	kGUIBasicFlowObj *sv;
	call_def cs;

	/* hmm, globals are set to whatever they ended with last time? */

	m_cancel=false;
	m_compile=false;	/* compile pass done, now in runtime pass */
	m_numfiles=0;		/* no open files */
	m_numlocals=0;		/* no locals */
	m_fordepth = 0;			/* no 'for' stack entries */

	/* get pointer to subroutine */
	sv=GetFObj(subname);
	m_pc=sv->GetAddr();

	cs.func=static_cast<kGUIBasicFuncObj *>(sv);
	cs.ret=0;					/* end! */
	cs.exit=true;
	cs.error.go=0;				/* default error goto for this sub */
	cs.with=0;
	m_callstack.SetEntry(0,cs);

	m_calldepth = 1;			/* one 'call' stack entry, this function! */
	m_eolnext=false;

	m_asyncactive=async;
	if(async)
	{
		m_thread.Start(this,CALLBACKNAME(MainLoop));
	}
	else
	{
//		m_thread=0;
		MainLoop();
	}
}

void kGUIBasic::MainLoop(void)
{
	if(setjmp(m_ebuf))
	{
		/* if a runtime error is encountered, then go here */
		return;
	}

	Loop();

	Print("\n*** Program finished sucessfully ***\n");
	CloseFiles(true);

	if(m_thread.GetActive())
		m_thread.Close(false);

	m_asyncactive=false;
	if(m_donecallback.IsValid())
		m_donecallback.Call();

}

void kGUIBasic::InputDone(kGUIString *s,int closebutton)
{
	m_input.SetString(s);
	m_inputdone=true;
	m_inputcancel=closebutton!=MSGBOX_OK;
}

void kGUIBasic::MessageDone(int result)	/* result=buttons pressed to exit msgbox */
{
	m_msgresult=result;
}

void kGUIBasic::FileReqDone(kGUIFileReq *req,int pressed)
{
	m_filereqstring.SetString(req->GetFilename());
	m_filereqresult=pressed;
}

/* error if an EOL is not next! */
void kGUIBasic::CheckEOL(void)
{
	if(m_tok!=BASICTOKEN_EOL)
	{
		GetToken();
		if(m_tok!=BASICTOKEN_EOL)
			basicerror(ERROR_EOLEXPECTED);
	}
}

void kGUIBasic::Loop(void)
{
	m_exit=false;
	
	do {
		/* abort execution? */
		if(m_cancel==true)
			basicerror(ERROR_USERCANCEL);

		GetToken();
		
		/* get prefix verbs first */
		while(m_token_type==TT_COMMANDVERB)
		{
			preverb_def pv;

//			pv.token=m_tok;
			pv.pc=m_lastpc;
//			pv.tpc=m_tpc;
			/* save list of prefix verbs */
			m_preverbs.SetEntry(m_numpreverbs++,pv);
			/* todo: save verbs in list */
			GetToken();
		}

		if(m_token_type==TT_COMMAND)
		{
			kGUIBasicCommandObj *cobj;

			cobj = static_cast<kGUIBasicCommandObj *>(m_tokobj);
			assert(cobj->m_code.vcodev!=0,"Error, undefined call function!");
			(this->*cobj->m_code.vcodev)();
		}
		else if(m_token_type==TT_VARIABLE || m_tok==BASICTOKEN_FIELDPREFIX)
		{
			kGUIBasicObj *obj;

			/* hmmm, could be a variable assignment or a function/sub call */
			Rewind();
			obj=GetFinalObj();
			switch(obj->GetType())
			{
			case VARTYPE_CFUNCTION:
			case VARTYPE_USUB:
			case VARTYPE_UFUNCTION:
			case VARTYPE_SYSCLASSFUNC:
			{
				kGUIBasicVarObj result;

				/* throw away result if this is a function */
				Call(obj,&result);
			}
			break;
			case VARTYPE_INTEGER:
			case VARTYPE_DOUBLE:
			case VARTYPE_STRING:
			case VARTYPE_BOOLEAN:
			case VARTYPE_VARIANT:
			{
				kGUIBasicVarObj *vobj;

				vobj=static_cast<kGUIBasicVarObj *>(obj);
				Set(vobj);	/* must be assignment statement */
			}
			break;
			default:
				basicerror(ERROR_NOTAVARIABLE);
			break;
			}
		}
		else if(m_tok==BASICTOKEN_INCREMENT || m_tok==BASICTOKEN_DECREMENT)
		{
			kGUIBasicVarObj result;

			Rewind();
			exp_get(&result);
		}
		else if(m_token[0]!=10 && m_token[0]!='\t' && m_token[0]!=0 && m_token_type!=TT_LABEL && m_tok!=BASICTOKEN_EOL)
			basicerror(ERROR_NOTACOMMAND);

		/* any unused prefix verbs? if so, error! */
		if(m_numpreverbs>0)
		{
			preverb_def pv;

			pv=m_preverbs.GetEntry(0);
			m_pc=pv.pc;
//			m_tpc=pv.tpc;
			basicerror(ERROR_SYNTAX);	/* unused prefix */
		}
		if(m_eolnext==true)
			CheckEOL();
		m_eolnext=true;
	}while (m_exit==false);
	m_exit=false;
}

void kGUIBasic::PopCall(void)
{
	call_def cs;
	local_def ld;
	for_stack fs;

	/* purge any open 'for' loops at this level */
	while(m_fordepth)
	{
		fs=m_fstack.GetEntry(m_fordepth-1);
		if(fs.level==m_calldepth)
		{
			PurgeFor(&fs);
			--m_fordepth;
		}
		else
			break;
	}

	/* purge all local variables for this level */
	while(m_numlocals)
	{
		ld=m_lstack.GetEntry(m_numlocals-1);
		if(ld.level==m_calldepth)
		{
			assert(ld.varptr[0]==ld.cur,"Error, variable pointer internal error!");
			if(ld.purge==true)
				delete ld.cur;
			ld.varptr[0]=ld.prev;	/* put pointer back to previous definition! */
			--m_numlocals;
		}
		else
			break;
	}

	--m_calldepth;

	/* re-enable locals from previous level */
	EnablePrevLocals();

	cs=m_callstack.GetEntry(m_calldepth);
	m_pc=cs.ret;
	m_exit=cs.exit;
	if(m_exit==true)
		m_eolnext=false;
}

/* Return from subroutine/function. */
/* same as exit except that it can also pass a return value if it is a function */
void kGUIBasic::cmd_return(void)
{
	call_def cs;
	kGUIBasicFuncObj *fobj;

	assert(m_calldepth>0,"Illegal 'end' statment, compiler should hav caught it!");

	/* todo, if this was a function, then check if return '(value)' */
	cs=m_callstack.GetEntry(m_calldepth-1);
	fobj=static_cast<kGUIBasicFuncObj *>(cs.func);

	if(fobj->GetType()==VARTYPE_UFUNCTION)
	{
		kGUIBasicVarObj value;
	
		/* get return value and put it in the function return value variable */
		exp_get(&value);
		SetVar(fobj->m_retval,&value);
	}

	/* make sure EOL is next */
	GetToken();
	if(m_tok!=BASICTOKEN_EOL)
		basicerror(ERROR_EOLEXPECTED);

	PopCall();
//	SkipToEOL();
}

void kGUIBasic::cmd_end(void)
{
	kGUIBasicFuncObj *funcobj;
	call_def cs;

	GetToken();	/* end what? */
	if(m_tok==BASICTOKEN_SELECT || m_tok==BASICTOKEN_IF)
	{
		/* do nothing, just continue with next statment */
	}
	else if(m_tok==BASICTOKEN_WITH)
	{
		call_def cs;

		cs=m_callstack.GetEntry(m_calldepth-1);
		cs.with=0;
		m_callstack.SetEntry(m_calldepth-1,cs);
	}
	else
	{
		assert(m_calldepth>0,"Illegal 'end' statment, compiler should hav caught it!");
		cs=m_callstack.GetEntry(m_calldepth-1);
		funcobj=static_cast<kGUIBasicFuncObj *>(cs.func);

		CheckEOL();	/* eol should be next! */

		if(funcobj->GetType()==VARTYPE_UFUNCTION)
		{
			/* make sure function return value has been set */
			if(funcobj->m_retval->GetIsUndefined()==true)
				basicerror(ERROR_NORETVALSET);
		}

		PopCall();
	}
}

void kGUIBasic::Print(const char *s)
{
	if(m_asyncactive==true)
		kGUI::GetAccess();
	m_output->Append(s);
	if(m_asyncactive==true)
		kGUI::ReleaseAccess();
}

/* Execute a simple version of the BASIC PRINT statement */
void kGUIBasic::cmd_print(void)
{
	kGUIBasicVarObj value;
	int spaces;
	char last_delim=0;
	file_def *fd=0;
	kGUIString ps;		/* print to this string */

	GetToken();
	if(m_tok==BASICTOKEN_NUMBER)
	{
		exp_get(&value);		/* get handle # */

		fd=GetFileHandle(value.GetInt());
		if(!fd)
			basicerror(ERROR_FILENOTOPEN);	/* file not open error */

		/* print #x, "aaaa" */
		/* or print #x */

		GetToken(); /* get next list item */
		if(m_tok==BASICTOKEN_EOL)
			Rewind();
		else if(m_tok!=BASICTOKEN_COMMA)
			basicerror(ERROR_COMMAEXPECTED);	/* comma expected */
	}
	else
		Rewind();

	do {
		GetToken(); /* get next list item */
		if(m_tok==BASICTOKEN_EOL || m_tok==BASICTOKEN_FINISHED)
			break;

		/* is expression */
		Rewind();
		exp_get(&value);
		GetToken();
		switch(value.GetType())
		{
		case VARTYPE_INTEGER:
			ps.ASprintf("%d",value.GetInt());
		break;
		case VARTYPE_DOUBLE:
			ps.ASprintf("%f",value.GetDouble());
		break;
		case VARTYPE_BOOLEAN:
			ps.ASprintf("%s",value.GetBoolean()==true?"True":"False");
		break;
		case VARTYPE_STRING:
			ps.Append(value.GetStringObj());
		break;
		default:
			assert(false,"Internal error, Unsupported type!");
			break;
		}
		last_delim = *m_token; 

		if(m_tok==BASICTOKEN_COMMA)
		{
			/* compute TT_NUMBER of spaces to move to next tab */
			spaces = 8 - (ps.GetLen() % 8); 
			//len += spaces; /* add in the tabbing position */
			while(spaces)
			{ 
				//printf(" ");
				ps.Append(" ");
				spaces--;
			}
		}
		else if(m_tok!=BASICTOKEN_SEMICOLON && m_tok!=BASICTOKEN_EOL && m_tok!=BASICTOKEN_FINISHED)
			basicerror(ERROR_SYNTAX); 
	}while(m_tok==BASICTOKEN_SEMICOLON || m_tok==BASICTOKEN_COMMA);

	if(m_tok==BASICTOKEN_EOL || m_tok==BASICTOKEN_FINISHED)
	{
		if(last_delim != ';' && last_delim!=',')
			ps.Append("\r\n");
	}
	else
		basicerror(ERROR_SYNTAX); /* error is not , or ; */

	/* send string to file or console? */
	if(fd)
		fwrite(ps.GetString(),1,ps.GetLen(),fd->f);
	else
		Print(ps.GetString());
}

/* Find the start of the next line. */
void kGUIBasic::SkipToEOL(void)
{
	do
	{
		GetToken();
		if(m_tok==BASICTOKEN_EOL || m_tok==BASICTOKEN_FINISHED)
			return;
	}while(1);
}

/* Execute a GOTO statement. */
void kGUIBasic::cmd_goto(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIString label;

	/* get the label name */
	GetToken();

	/* generate a unique label by appending the label name to the function name */
	label.Sprintf("%s:%s",m_callstack.GetEntry(m_calldepth-1).func->GetName(),m_token);

	fobj=GetFObj(label.GetString());
	if(!fobj)
		basicerror(ERROR_UNDEFINEDLABEL);
	if(fobj->GetType()!=VARTYPE_LABEL)
		basicerror(ERROR_UNDEFINEDLABEL);
	m_pc=fobj->GetAddr();
	m_eolnext=false;
}

/* Execute an IF statement. */
void kGUIBasic::cmd_if(void)
{
	bool doit;
	int i,nc;
	kGUIBasicVarObj result;
//	kGUIString vn;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *efobj;

	exp_get(&result);
	doit=result.GetBoolean();

	GetToken();
	if(m_tok!=BASICTOKEN_THEN)
		basicerror(ERROR_THENEXPECTED);

	/* if nothing is after the then, then it is a multi-line if statment */
	GetToken();
	if(m_tok!=BASICTOKEN_EOL)
	{
		/* single line if */
		if(doit==false)
			SkipToEOL();
		else
		{
			Rewind();
			m_eolnext=false;	/* allow command to continue on same line */
		}
	}
	else if(doit==false)
	{

		/* get pointer to me */
//		fobj=GetFObj(vn.GetString());
		fobj=m_lastpc->fobj;

		assert(fobj!=0,"Internal Error, if not found in table");
		assert(fobj->GetType()==VARTYPE_IF,"Internal Error, var type mismatch");
		nc=fobj->GetNumChildren();
		assert(nc>0,"Internal Error, no endif in list");

		for(i=0;i<nc;++i)
		{
			efobj=fobj->GetChild(i);
			m_pc=efobj->GetAddr();
			if(efobj->GetType()==VARTYPE_ELSEIF)
			{
				exp_get(&result);
				doit=result.GetBoolean();

				GetToken();
				if(m_tok!=BASICTOKEN_THEN)
					basicerror(ERROR_THENEXPECTED);
				if(doit==true)
				{
					GetToken();
					return;		/* ok, found a valid block to do */
				}
			}
			else
			{
				assert(efobj->GetType()==VARTYPE_ELSE || efobj->GetType()==VARTYPE_ENDIF,"Internal Error, var type mismatch");
				GetToken();	/* skip the else or endif, then continue */
				return;
			}
		}
	}
}

/* used for both else and elseif */
void kGUIBasic::cmd_else(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *pfobj;
	kGUIBasicFlowObj *efobj;

	/* get pointer to me */
//	fobj=GetFObj(vn.GetString());
	fobj=m_lastpc->fobj;
	if(!fobj)
	{
		/* must be "else if */
		GetToken();
		fobj=m_lastpc->fobj;
	//	fobj=GetFObj(vn.GetString());
	}
	assert(fobj!=0,"Internal Error, Else not found in table");
	assert(fobj->GetType()==VARTYPE_ELSEIF || fobj->GetType()==VARTYPE_ELSE,"Internal Error, var type mismatch");
	/* get pointer to parent */
	pfobj=fobj->GetChild(0);
	assert(pfobj->GetType()==VARTYPE_IF,"Internal Error, var type mismatch");
	/* last entry in parent is the endif */
	efobj=pfobj->GetChild(pfobj->GetNumChildren()-1);
	assert(efobj->GetType()==VARTYPE_ENDIF,"Internal Error, var type mismatch");

	/* goto the endif statment */
	m_pc=efobj->GetAddr();
}

void kGUIBasic::cmd_endif(void)
{
	/* don't do anything */
}

/* exit do, exit function, exit sub, exit for */

void kGUIBasic::cmd_exit(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *pfobj;
	kGUIBasicFlowObj *efobj;

	GetToken();	/* thing to exit */

	/* get pointer to me */
	fobj=m_lastpc->fobj;

	CheckEOL();	/* eol should be next! */

	assert(fobj!=0,"Internal Error, Exit not found in table");
	assert(fobj->GetType()==VARTYPE_EXIT,"Internal Error, var type mismatch");
	/* get pointer to thing we are exiting */
	pfobj=fobj->GetChild(0);

	switch(pfobj->GetType())
	{
	case VARTYPE_FOR:
	case VARTYPE_DO:
	case VARTYPE_WHILE:
		/* last entry in parent is the next or while */
		efobj=pfobj->GetChild(pfobj->GetNumChildren()-1);
		/* goto the last statment, then skip the rest of that line */
		m_pc=efobj->GetAddr();
		SkipToEOL();
		m_eolnext=false;
	break;
	case VARTYPE_USUB:
		PopCall();
	break;
	case VARTYPE_UFUNCTION:
	{
		kGUIBasicFuncObj *funcobj;
		call_def cs;

		cs=m_callstack.GetEntry(m_calldepth-1);
		funcobj=static_cast<kGUIBasicFuncObj *>(cs.func);
		
		/* make sure function return type has been set */
		if(funcobj->m_retval->GetIsUndefined()==true)
			basicerror(ERROR_NORETVALSET);

		PopCall();
	}
	break;
	default:
		assert(false,"Unimplemented exit");
	break;
	}
}

void kGUIBasic::cmd_on(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIString label;

	GetToken();
	if(m_tok==BASICTOKEN_ERROR)
	{
		call_def cs;

		cs=m_callstack.GetEntry(m_calldepth-1);

		/* set the error goto line position, only valid for this call level, reset to previous upon poplevel */
		GetToken();
		if(m_tok==BASICTOKEN_RESUME)
		{
			GetToken();
			if(m_tok!=BASICTOKEN_NEXT)
				basicerror(ERROR_NEXTEXPECTED); /* read and discard the NEXT */
			cs.error.resume=1;
			goto okerr;
		}
		else if(m_tok!=BASICTOKEN_GOTO)
			basicerror(ERROR_GOTOEXPECTED); /* read and discard the GOTO */

		/* get the label name */
		GetToken();

		if(!strcmp(m_token,"0"))
			cs.error.go=0;
		else
		{
			/* generate a unique label by appending the label name to the function name */
			label.Sprintf("%s:%s",m_callstack.GetEntry(m_calldepth-1).func->GetName(),m_token);

			fobj=GetFObj(label.GetString());
			if(!fobj)
				basicerror(ERROR_UNDEFINEDLABEL);
			if(fobj->GetType()!=VARTYPE_LABEL)
				basicerror(ERROR_UNDEFINEDLABEL);

			cs.error.go=fobj->GetAddr();
		}
okerr:;
		m_callstack.SetEntry(m_calldepth-1,cs);
	}
	else
	{

	}
	/* todo */
}

void kGUIBasic::cmd_with(void)
{
	kGUIBasicVarObj *var;
	call_def cs;

	var=GetVar();
	cs=m_callstack.GetEntry(m_calldepth-1);
	cs.with=var;
	m_callstack.SetEntry(m_calldepth-1,cs);
}

void kGUIBasic::cmd_do(void)
{
	bool doit=false;
	kGUIBasicVarObj result;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *lfobj;

	/* "eol", "while" or until */
	fobj=m_lastpc->fobj;
	assert(fobj!=0,"Internal Do not found in table");
	assert(fobj->GetType()==VARTYPE_DO,"Internal Error, var type mismatch");

	GetToken();
	switch(m_tok)
	{
	case BASICTOKEN_WHILE:
		exp_get(&result);
		doit=result.GetBoolean();
	break;
	case BASICTOKEN_UNTIL:
		exp_get(&result);
		doit=!result.GetBoolean();
	break;
	case BASICTOKEN_EOL:
		doit=true;		/* do it once, check at end for repeat */
	break;
	default:
		basicerror(ERROR_WHILEUNTILOREOLEXPECTED);	/* expected while, until or eol */
	break;
	}

	/* skip ahead to loop */
	if(doit==false)
	{
		/* skip ahead to loop */
		/* get pointer to loop */
		lfobj=fobj->GetChild(0);
		assert(lfobj->GetType()==VARTYPE_LOOP,"Internal Error, var type mismatch");

		/* goto the loop statment, then skip that line */
		m_pc=lfobj->GetAddr();
		SkipToEOL();
	}
}

void kGUIBasic::cmd_loop(void)
{
	bool doit=false;
	kGUIBasicVarObj result;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *dfobj;

	fobj=m_lastpc->fobj;

	/* "eol", "while" or until */
	GetToken();
	switch(m_tok)
	{
	case BASICTOKEN_WHILE:
		exp_get(&result);
		doit=result.GetBoolean();
	break;
	case BASICTOKEN_UNTIL:
		exp_get(&result);
		doit=!result.GetBoolean();
	break;
	case BASICTOKEN_EOL:
		doit=true;
	break;
	default:
		basicerror(ERROR_WHILEUNTILOREOLEXPECTED);	/* expected while, until or eol */
	break;
	}

	if(doit==true)
	{
		/* get pointer to me */
		assert(fobj!=0,"Internal Loop not found in table");
		assert(fobj->GetType()==VARTYPE_LOOP,"Internal Error, var type mismatch");
		/* get pointer to loop */
		dfobj=fobj->GetChild(0);
		assert(dfobj->GetType()==VARTYPE_DO,"Internal Error, var type mismatch");

		/* goto the do statment */
		m_pc=dfobj->GetAddr();
		m_lastpc=m_pc-1;
		cmd_do();
	}
}

void kGUIBasic::cmd_while(void)
{
	kGUIBasicVarObj result;
	kGUIString vn;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *lfobj;

	fobj=m_lastpc->fobj;

	exp_get(&result);

	if(result.GetBoolean()==false)
	{
		/* skip ahead to wend */
		assert(fobj!=0,"Internal While not found in table");
		assert(fobj->GetType()==VARTYPE_WHILE,"Internal Error, var type mismatch");
		/* get pointer to loop */
		lfobj=fobj->GetChild(0);
		assert(lfobj->GetType()==VARTYPE_WEND,"Internal Error, var type mismatch");

		/* goto the loop statment, then skip that line */
		m_pc=lfobj->GetAddr();
		SkipToEOL();
	}
}

void kGUIBasic::cmd_wend(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *dfobj;

    /* get pointer to me */
	fobj=m_lastpc->fobj;
	assert(fobj!=0,"Internal While not found in table");
	assert(fobj->GetType()==VARTYPE_WEND,"Internal Error, var type mismatch");
	/* get pointer to loop */
	dfobj=fobj->GetChild(0);
	assert(dfobj->GetType()==VARTYPE_WHILE,"Internal Error, var type mismatch");

	/* goto the while statment */
	m_pc=dfobj->GetAddr();
	m_lastpc=m_pc-1;
	cmd_while();
}

void kGUIBasic::PurgeFor(for_stack *fs)
{
	delete fs->target;
	if(fs->step)
		delete fs->step;
}

/* Execute a FOR loop. */
void kGUIBasic::cmd_for(void)
{
	for_stack fs;
	kGUIBasicVarObj startvalue;
	kGUIBasicVarObj *vobj;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *nfobj;
	int type;
	bool again;

	/* get pointer to me */
	fobj=m_lastpc->fobj;
	assert(fobj!=0,"Internal Loop not found in table");
	assert(fobj->GetType()==VARTYPE_FOR,"Internal Error, var type mismatch");

	/* get pointer to variable */
	vobj=GetVar();

	GetToken(); /* read the equals sign */
	if(m_tok!=BASICTOKEN_EQUALS)
		basicerror(ERROR_EQUALSIGNEXPECTED);

	exp_get(&startvalue);		/* get initial value */

	fs.vp=vobj;
	SetVar(vobj,&startvalue);
	type=fs.vp->GetType();
	if(!(type==VARTYPE_INTEGER || type==VARTYPE_DOUBLE))
		basicerror(ERROR_BADFORTYPE);

	GetToken();
	if(m_tok!=BASICTOKEN_TO)
		basicerror(ERROR_TOEXPECTED); /* read and discard the TO */

	fs.target=new kGUIBasicVarObj();
	exp_get(fs.target);					/* get target value */
	fs.target->ChangeType(type);		/* force type to match variable type */

	GetToken();
	if(m_tok==BASICTOKEN_STEP)
	{
		fs.step=new kGUIBasicVarObj();
		exp_get(fs.step);			/* get the step value */
		fs.step->ChangeType(type);	/* force type to match variable type */
	}
	else
	{
		Rewind();
		fs.step=0;					/* no step value */
	}

#if 1
	again=exp_vardelta(fs.vp,fs.target)<=0;
#else
	switch(type)
	{
	case VARTYPE_INTEGER:
		again=(fs.vp->m_value.i<=fs.target->m_value.i);
	break;
	case VARTYPE_DOUBLE:
		again=(fs.vp->m_value.d<=fs.target->m_value.d);
	break;
	default:
		assert(false,"Unimplemented for type!");
	break;
	}
#endif

	/* if loop can execute at least once, push info on stack */
	if(again==true)
	{ 
		fs.level=m_calldepth;
		fs.loc = m_pc;
		m_fstack.SetEntry(m_fordepth++,fs);
	}
	else  /* otherwise, skip loop code altogether */
	{
		nfobj=fobj->GetChild(0);					/* get pointer to next */
		assert(nfobj->GetType()==VARTYPE_NEXT,"Internal Error, var type mismatch");

		m_pc=nfobj->GetAddr();
		SkipToEOL();

		PurgeFor(&fs);
	}
}

/* Execute a NEXT statement. */
void kGUIBasic::cmd_next(void)
{
	for_stack fs;
	bool again;
	kGUIString vn;

	if(m_fordepth==0)
		basicerror(ERROR_NEXTWITHOUTFOR);

	GetToken();
	Rewind();
	if(m_tok!=BASICTOKEN_EOL)
	{
		kGUIBasicVarObj *vobj;

		/* get pointer to variable */
		vobj=GetVar();

		/* find the one for we are looking for */
		fs=m_fstack.GetEntry(m_fordepth-1);
		while(vobj!=fs.vp)
		{
			PurgeFor(&fs);
			--m_fordepth;
			if(!m_fordepth)
				basicerror(ERROR_NEXTWITHOUTFOR);
			fs=m_fstack.GetEntry(m_fordepth-1);
		}
	}
	else
		fs=m_fstack.GetEntry(m_fordepth-1);

	/* is this one on the same level? */
	if(fs.level!=m_calldepth)
		basicerror(ERROR_NEXTWITHOUTFOR);

	switch(fs.vp->GetType())
	{
	case VARTYPE_INTEGER:
		fs.vp->Set(fs.vp->GetInt()+(fs.step?fs.step->GetInt():1));
	break;
	case VARTYPE_DOUBLE:
		fs.vp->Set(fs.vp->GetDouble()+(fs.step?fs.step->GetDouble():1.0f));
	break;
	default:
		assert(false,"Unimplemented for type!");
	break;
	}
	again=exp_vardelta(fs.vp,fs.target)<=0;

	if(again==true)
	{
		m_pc = fs.loc;  /* loop */
		m_eolnext=false;
	}
	else
	{
		PurgeFor(&fs);
		--m_fordepth;
	}
}

void kGUIBasic::cmd_select(void)
{
	int i,nc;
	kGUIBasicVarObj result;
	kGUIBasicVarObj compare;
	kGUIBasicVarObj compare2;
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *efobj;
	int cmpmode,cmp;
	bool v;

	/* get pointer to me for all the case addresses and the end select address */
	/* get pointer to me */
	fobj=m_lastpc->fobj;
	assert(fobj!=0,"Internal Error, if not found in table");
	assert(fobj->GetType()==VARTYPE_SELECT,"Internal Error, var type mismatch");
	nc=fobj->GetNumChildren();
	assert(nc>0,"Internal Error, no end select in list");

	exp_get(&result);
	GetToken();
	if(m_tok!=BASICTOKEN_EOL)
		basicerror(ERROR_EOLEXPECTED);

	for(i=0;i<nc;++i)
	{
		efobj=fobj->GetChild(i);
		m_pc=efobj->GetAddr();
		if(efobj->GetType()==VARTYPE_CASE)
		{
			do
			{
				//[ Is ] comparisonoperator expression
				//expression1 To expression2
				//expression
				GetToken();
				cmpmode=BASICTOKEN_TESTEQUAL;
				if(m_tok==BASICTOKEN_IS)
				{
					GetToken();
					cmpmode=m_tok;	/* comparision mode to use */
					exp_get(&compare);
				}
				else
				{
					Rewind();
					exp_get(&compare);
					GetToken();
					if(m_tok==BASICTOKEN_TO)
					{
						exp_get(&compare2);
						cmpmode=-1;		/* special value for range compare mode */
					}
					else
						Rewind();
				}
				/* ok, cmpmode==-1 is range, else it is the compare mode to use */
				cmp=exp_vardelta(&result,&compare);
				v=false;
				switch(cmpmode)
				{
				case -1:
					v=false;
					if(cmp>=0)
					{
						if(exp_vardelta(&result,&compare2)<=0)
							v=true;
					}
				break;
				case BASICTOKEN_EQUALS:
				case BASICTOKEN_TESTEQUAL:
					v=(cmp==0);
				break;
				case BASICTOKEN_TESTNOTEQUAL:
					v=(cmp!=0);
				break;
				case BASICTOKEN_TESTLESSTHAN:
					v=(cmp==-1);
				break;
				case BASICTOKEN_TESTLESSTHANOREQUAL:
					v=(cmp<=0);
				break;
				case BASICTOKEN_TESTGREATERTHAN:
					v=(cmp==1);
				break;
				case BASICTOKEN_TESTGREATERTHANOREQUAL:
					v=(cmp>=0);
				break;
				default:
					basicerror(ERROR_SYNTAX);	/* bad comp operator */
				break;
				}
				if(v==true)
				{
					SkipToEOL();
					return;		/* ok, found a valid block to do */
				}
				GetToken();
			}while(m_tok==BASICTOKEN_COMMA);
			CheckEOL();
		}
		else
		{
			assert(efobj->GetType()==VARTYPE_CASEELSE || efobj->GetType()==VARTYPE_ENDSELECT,"Internal Error, var type mismatch");
			SkipToEOL();
			return;
		}
	}
	/* should never get here! */
}

/* if we encountered this, then skip ahead till the end select! */
void kGUIBasic::cmd_case(void)
{
	kGUIBasicFlowObj *fobj;
	kGUIBasicFlowObj *pfobj;
	kGUIBasicFlowObj *efobj;

	/* skip ahead to endif */
	/* get pointer to me */
	fobj=m_lastpc->fobj;
	assert(fobj!=0,"Internal Error, Else not found in table");
	assert(fobj->GetType()==VARTYPE_CASE || fobj->GetType()==VARTYPE_CASEELSE,"Internal Error, var type mismatch");
	/* get pointer to parent */
	pfobj=fobj->GetChild(0);
	assert(pfobj->GetType()==VARTYPE_SELECT,"Internal Error, var type mismatch");
	/* last entry in parent is the endif */
	efobj=pfobj->GetChild(pfobj->GetNumChildren()-1);
	assert(efobj->GetType()==VARTYPE_ENDSELECT,"Internal Error, var type mismatch");

	/* goto the end select statment */
	m_pc=efobj->GetAddr();
}

void kGUIBasic::cmd_input(void)
{
	kGUIBasicVarObj handle;
	file_def *fd;
	kGUIBasicVarObj *var;
	kGUIBasicVarObj value;
	bool linemode;
	kGUIString s;
	char c;

	linemode=IsPreVerb(BASICTOKEN_LINE);

	GetToken();
	if(m_tok!=BASICTOKEN_NUMBER)
		basicerror(ERROR_NUMBERSYMBOLEXPECTED); /* read and discard the '#' */

	exp_get(&handle);
	fd=GetFileHandle(handle.GetInt());
	if(!fd)
		basicerror(ERROR_FILENOTOPEN); /* file is not open! */

	GetToken();
	if(m_tok!=BASICTOKEN_COMMA)
		basicerror(ERROR_COMMAEXPECTED);

	if(linemode==true)
	{
		var=GetVar();
		/* read a line until c/r */
		do
		{
			if(!fread(&c,1,1,fd->f))
				break;
			if(c==0x0d)	/* c/r */
			{
				long curpos=ftell(fd->f);

				fread(&c,1,1,fd->f);
				if(c!=0x0a)	/* l/f */
					fseek(fd->f,curpos,SEEK_SET);
				break;
			}
			s.Append(&c,1);
		}while(1);
		var->Set(&s);
		var->SetIsUndefined(false);
	}
	else
	{
		do
		{
			var=GetVar();
			/* read a line until c/r or comma */
			s.Clear();
			do
			{
				if(!fread(&c,1,1,fd->f))
					break;
				if(c==',')
					break;
				if(c==0x0d)	/* c/r */
				{
					long curpos=ftell(fd->f);

					if(!fread(&c,1,1,fd->f))
						break;
					if(c!=0x0a)	/* l/f */
						fseek(fd->f,curpos,SEEK_SET);
					break;
				}
				s.Append(&c,1);
			}while(1);
			value.Set(&s);
			SetVar(var,&value);	/* converts from string to whatever type the variable is */
			GetToken();
			if(m_tok!=BASICTOKEN_COMMA)
				break;
		}while(1);
	}
}

/* ask the user for input */
void kGUIBasic::cmd_inputbox(void)
{
	kGUIBasicVarObj prompt;
	kGUIBasicVarObj *var;
	kGUIBasicVarObj input;
	kGUIInputBoxReq *inputbox;

	exp_get(&prompt);	/* string to show to user */
	prompt.ChangeType(VARTYPE_STRING);

	GetToken();
	if(m_tok!=BASICTOKEN_COMMA)
		basicerror(ERROR_COMMAEXPECTED);
	var=GetVar();

	/* only valid in async mode */
	if(m_asyncactive==false)
		basicerror(ERROR_COMMAEXPECTED);

	kGUI::GetAccess();
	m_inputdone=false;
	inputbox=new kGUIInputBoxReq(this,CALLBACKNAME(InputDone),prompt.GetStringObj()->GetString());
	kGUI::ReleaseAccess();

	while(m_inputdone==false)
		kGUI::Sleep(10);

	input.Set(m_input.GetString());

	SetVar(var,&input);
	if(m_inputcancel)
		basicerrortrap(ERROR_USERCANCELLEDINPUT);
}

/* used for both set and let */
void kGUIBasic::cmd_set(void)
{
	Set(GetVar());
}

void kGUIBasic::Set(kGUIBasicVarObj *vobj)
{

	if(vobj->GetIsConstant()==true)
		basicerror(ERROR_TRYINGTOSETCONSTANT);

	/* get the equals sign ( or other operator ) */
	GetToken();
	switch(m_tok)
	{
	case BASICTOKEN_EQUALS:
		if(vobj->GetIsArray()==true)
			exp_get(vobj);
		else
		{
			kGUIBasicVarObj value;

			exp_get(&value);
			SetVar(vobj,&value);
		}
	break;
	case BASICTOKEN_ADDTO:
	{
		kGUIBasicVarObj value;

		if(vobj->GetIsArray()==true)
			basicerror(ERROR_TRYINGTOSETARRAY);
		exp_get(&value);
		exp_add(vobj,&value);
	}
	break;
	case BASICTOKEN_SUBTRACTFROM:
	{
		kGUIBasicVarObj value;
		if(vobj->GetIsArray()==true)
			basicerror(ERROR_TRYINGTOSETARRAY);
		exp_get(&value);
		exp_sub(vobj,&value);
	}
	break;
	case BASICTOKEN_INCREMENT:
		if(vobj->GetIsArray()==true)
			basicerror(ERROR_TRYINGTOSETARRAY);
		vobj->Set(vobj->GetInt()+1);
	break;
	case BASICTOKEN_DECREMENT:
		if(vobj->GetIsArray()==true)
			basicerror(ERROR_TRYINGTOSETARRAY);
		vobj->Set(vobj->GetInt()-1);
	break;
	default:
		basicerror(ERROR_EQUALSIGNEXPECTED);
	break;
	}
}


void kGUIBasic::cmd_dim(void)
{
	kGUIBasicVarObj *var;
	kGUIString vn;

	var=new kGUIBasicVarObj();
	AddLeak(var);
	DimVar(&vn,var);
	RemoveLeak(var);
	RegLocal(m_calldepth,vn.GetString(),var);
}

void kGUIBasic::cmd_redim(void)
{
	kGUIBasicIndices indices;
	kGUIBasicObj *obj;
	kGUIBasicVarObj *vobj;
	bool preserve;
	int error;

	GetToken();
	preserve=(m_tok==BASICTOKEN_PRESERVE);
	if(preserve)
		GetToken();

	obj=m_tokobj;
	vobj=ToVar(obj);		/* error if not a variable type */

	/* get 'as' or '[' */
	
	GetToken();
	if(m_tok==BASICTOKEN_OPENSQUAREBRACKET)
		GetIndices(&indices);

	if(m_tok!=BASICTOKEN_AS)
		basicerror(ERROR_ASEXPECTED);
	GetToken();

	error=vobj->ReDim(&indices,m_tokobj,preserve);
	if(error!=ERROR_OK)
		basicerror(error);

	CheckEOL();
}


/* disable all locals from previous level */
void kGUIBasic::DisablePrevLocals(void)
{
	int l;
	local_def ld;

	l=m_numlocals-1;
	while(l>=0)
	{
		ld=m_lstack.GetEntry(l);
		if(ld.level!=m_calldepth)
			break;

		ld.varptr[0]=0;
		if(ld.prev)
		{
			if(ld.prev->GetIsGlobal())
				ld.varptr[0]=ld.prev;
		}
		--l;
	}
}

/* re-enable all locals from previous level */
void kGUIBasic::EnablePrevLocals(void)
{
	int l;
	local_def ld;

	l=m_numlocals-1;
	while(l>=0)
	{
		ld=m_lstack.GetEntry(l);
		if(ld.level!=m_calldepth)
			break;

		ld.varptr[0]=ld.cur;
		--l;
	}
}

/* Execute a call / gosub command. */
void kGUIBasic::cmd_call(void)
{
	kGUIBasicVarObj result;

	/* throw away result if this is a function */
	Call(GetFinalObj(),&result);
}

#define MAXSYSTEMPARMS 5

/* this is used to call both user subroutines and functions */
/* and system functions and class subroutines and functions */

void kGUIBasic::Call(kGUIBasicObj *obj,kGUIBasicVarObj *result)
{
	kGUIBasicFuncObj *fobj;
	call_def cs;
	Array<kGUIBasicVarObj *>varlist;

	if(!obj)
		basicerror(ERROR_SYNTAX);

	switch(obj->GetType())
	{
	case VARTYPE_USUB:
		fobj=static_cast<kGUIBasicFuncObj *>(obj);
		ParseUserParms(fobj);

		/* push call info onto stack! */
		cs.func=fobj;
		cs.ret=m_pc;
		cs.exit=false;
		cs.error.go=0;					/* default error goto for this call */
		cs.with=0;
		m_callstack.SetEntry(m_calldepth++,cs);

		m_pc=fobj->GetAddr();
		return;
	break;
	case VARTYPE_UFUNCTION:
	{
		kGUIBasicFuncObj *fobj;
		call_def cs;
		kGUIBasicVarObj *prev;
		kGUIBasicVarObj **pvp;
		local_def ld;
		kGUIBasicVarObj *rettype;
		
		fobj=static_cast<kGUIBasicFuncObj *>(obj);

		/* get parms to send to the function */
		ParseUserParms(fobj);

		/* save old results var and replace with ours */
		prev=fobj->m_retval;
		pvp=&fobj->m_retval;
		fobj->m_retval=result;
		
		rettype=&fobj->m_rettype;
		result->InitVarType(rettype,0);

		/* add undo info to the stack for winding back this variable */
		ld.level=m_calldepth+1;
		ld.varptr=pvp;
		ld.cur=result;			/* only needed for debugging */
		ld.purge=false;
		ld.prev=prev;
		m_lstack.SetEntry(m_numlocals++,ld);

		/* push call info onto stack! */
		cs.func=fobj;
		cs.ret=m_pc;
		cs.exit=true;					/* exit loop at this point */
		cs.error.go=0;					/* default error goto for this call */
		cs.with=0;
		m_callstack.SetEntry(m_calldepth++,cs);

		m_pc=fobj->GetAddr();
		Loop();
	}
	break;
	case VARTYPE_CFUNCTION:
	{
		int np;
		CallParms parms;
		kGUIBasicCommandObj *cobj;

		varlist.Alloc(10);
		varlist.SetGrow(true);
		cobj = static_cast<kGUIBasicCommandObj *>(obj);
		np=ParseSystemParms(cobj,&parms);
		if(np<cobj->GetMinNumParms() || np>cobj->GetMaxNumParms())
			basicerror(ERROR_WRONGNUMBEROFPARAMETERS);	/* wrong num of parms */

		(this->*cobj->m_code.fcode)(result,&parms);
	}
	break;
	case VARTYPE_SYSCLASSFUNC:
	{
		/* handle varying number of parameters */
		kGUIBasicSysClassFuncObj *scfobj;
		int np,err;
		CallParms parms;

		scfobj=static_cast<kGUIBasicSysClassFuncObj *>(obj);
		np=ParseSystemParms(scfobj,&parms);
		err=scfobj->Call(result,&parms);
		if(err!=ERROR_OK)
			basicerrortrap(err);	/* all system errors should be trappable */
		return;
	}
	break;
	}
}

/* parse the parm type string */
void ParmDef::SetParmDef(const char *pdstring)
{
	unsigned int i,num;
	kGUIString s;
	kGUIStringSplit ss;
	cparm_def pd;
	const char *typestring;

	s.SetString(pdstring);
	num=ss.Split(&s,",");
	m_numparms=num;
	m_parms.Alloc(num);
	for(i=0;i<num;++i)
	{
		//"byval type"or "byref type"
		if(!strncmp(ss.GetWord(i)->GetString(),"byval",5))
			pd.bymode=BASICTOKEN_BYVAL;
		else if(!strncmp(ss.GetWord(i)->GetString(),"byref",5))
			pd.bymode=BASICTOKEN_BYREF;
		else
		{
			pd.bymode=BASICTOKEN_BYVAL;
			assert(false,"Error, should be either byval or byref");
		}
		typestring=ss.GetWord(i)->GetString()+6;
		if(!strcmp(typestring,"boolean"))
			pd.type=BASICTOKEN_BOOLEAN;
		else if(!strcmp(typestring,"integer"))
			pd.type=BASICTOKEN_INTEGER;
		else if(!strcmp(typestring,"double"))
			pd.type=BASICTOKEN_DOUBLE;
		else if(!strcmp(typestring,"string"))
			pd.type=BASICTOKEN_STRING;
		else if(!strcmp(typestring,"variant"))
			pd.type=BASICTOKEN_VARIANT;
		else
		{
			pd.type=BASICTOKEN_BOOLEAN;
			assert(false,"Error, unknown type!");
		}
		m_parms.SetEntry(i,pd);
	}
}

/* grab parms and return the number found */
int kGUIBasic::ParseSystemParms(ParmDef *def,CallParms *parms)
{
	int np=0;
	kGUIBasicVarObj *var;
	int vtype;

	GetToken();
	if(m_tok!=BASICTOKEN_OPENBRACKET)
		basicerror(ERROR_OPENBRACKETEXPECTED);

	do
	{
		GetToken();
		if(m_tok==BASICTOKEN_CLOSEBRACKET)
			return(np);
		if(np>0)
		{
			if(m_tok!=BASICTOKEN_COMMA)
				basicerror(ERROR_COMMAEXPECTED);
			GetToken();
		}
		Rewind();
		if(np==def->GetNumParms())
			basicerror(ERROR_WRONGNUMBEROFPARAMETERS);

		if(def->GetParmByMode(np)==BASICTOKEN_BYVAL)
		{
			var=new kGUIBasicVarObj();

			AddLeak(var);
			exp_get(var);

			/* make sure type is correct */
			vtype=GetTypeFromToken(def->GetParmType(np));
			if(vtype!=VARTYPE_VARIANT)
				var->ChangeType(vtype);

			parms->AddParm(var,true);
			RemoveLeak(var);
		}
		else
		{
			/* since this value is passed as a reference, then it */
			/* cannot be an expression but can only be a single */
			/* variable */
			var=GetVar();
			parms->AddParm(var,false);
		}
		/* todo, change to requested type if not variant */
		++np;
	}while(1);
}

/* parse user parameters for both functions and subroutines */

void kGUIBasic::ParseUserParms(kGUIBasicFuncObj *fobj)
{
	int v,np;
	int bymode;
	kGUIBasicVarObj *var;
	Array<kGUIBasicVarObj *>varlist;
	kGUIBasicVarObj *parmtype;
	kGUIBasicVarObj *vobj;

	np=fobj->GetNumParms();
	/* parse parameters */
	GetToken();

	if(m_tok==BASICTOKEN_EOL && !np)
	{
		Rewind();
		/* flag all variables from previous level as unavailable */
		DisablePrevLocals();
	}
	else
	{
		if(m_tok!=BASICTOKEN_OPENBRACKET)
			basicerror(ERROR_OPENBRACKETEXPECTED);

		if(np>0)
		{
			varlist.Alloc(np);
			for(v=0;v<np;++v)
			{
				if(v)
				{
					GetToken();
					if(m_tok!=BASICTOKEN_COMMA)
						basicerror(ERROR_COMMAEXPECTED);
				}
				parmtype=fobj->GetParmType(v);
				bymode=fobj->GetParmByMode(v);
				if(bymode==BASICTOKEN_BYVAL)
				{
					/* get expression to pass as parameter */
					var=new kGUIBasicVarObj();
					/* tofix if error exists while getting parms then this is leaked */
					varlist.SetEntry(v,var);
					exp_get(var);
					/* make sure type is correct */
					if(parmtype->GetIsVariant()==false)
						var->ChangeType(parmtype->GetType());
					var->SetIsUndefined(false);
				}
				else
				{
					/* since this value is passed as a reference, then it */
					/* cannot be an expression but can only be a single */
					/* variable */
					vobj=GetVar();
					varlist.SetEntry(v,vobj);
					/* make sure types match, else error */
					if(vobj->CheckType(parmtype)==false)
						basicerror(ERROR_VARTYPEMISMATCH);
				}
			}
			/* flag all variables from previous level as unavailable */
			DisablePrevLocals();

			/* these need to be done in a seperate loop since one of the */
			/* parms above may share names with the name used in the function */
			for(v=0;v<np;++v)
				RegLocal(m_calldepth+1,fobj->GetParmName(v),varlist.GetEntry(v),fobj->GetParmByMode(v)==BASICTOKEN_BYVAL);
		}
		GetToken();
		if(m_tok!=BASICTOKEN_CLOSEBRACKET)
			basicerror(ERROR_CLOSEBRACKETEXPECTED);
	}
}

file_def *kGUIBasic::GetFileHandle(int num,int *slot)
{
	int i;
	file_def *fd;

	for(i=0;i<m_numfiles;++i)
	{
		fd=m_files.GetArrayPtr()+i;
		if(fd->handle==num)
		{
			if(slot)
				slot[0]=i;
			return(fd);
		}
	}
	return(0);	/* not a valid handle */
}


/* Open filename For [Output/Input...] As #x */

#define OPEN_INPUT 1
#define OPEN_OUTPUT 2
#define OPEN_BINARY 4
#define OPEN_APPEND 8

void kGUIBasic::cmd_open(void)
{
	int mode;
	kGUIBasicVarObj filename;
	kGUIBasicVarObj handle;
	file_def fd;

	exp_get(&filename);

	GetToken();
	if(m_tok!=BASICTOKEN_FOR)
		basicerror(ERROR_FOREXPECTED); /* read and discard the FOR */

	mode=0;
	do
	{
		GetToken();
		switch(m_tok)
		{
		case BASICTOKEN_INPUT:
			mode|=OPEN_INPUT;
		break;
		case BASICTOKEN_OUTPUT:
			mode|=OPEN_OUTPUT;
		break;
		case BASICTOKEN_BINARY:
			mode|=OPEN_BINARY;
		break;
		case BASICTOKEN_ACCESS:	/* not really used?? */
		break;
		case BASICTOKEN_READ:
			mode|=OPEN_INPUT;
		break;
		case BASICTOKEN_WRITE:
			mode|=OPEN_OUTPUT;
		break;
		case BASICTOKEN_APPEND:
			mode|=OPEN_APPEND;
		break;
		case BASICTOKEN_AS:
			/* exit */
		break;
		default:
			basicerror(ERROR_ASEXPECTED);
		break;
		}
	}while(m_tok!=BASICTOKEN_AS);

	/* todo: check for valid mode */

	GetToken();
	if(m_tok!=BASICTOKEN_NUMBER)
		basicerror(ERROR_NUMBERSYMBOLEXPECTED); /* read and discard the '#' */

	exp_get(&handle);

	filename.ChangeType(VARTYPE_STRING);

	fd.handle=handle.GetInt();
	if(GetFileHandle(fd.handle))
		basicerror(ERROR_FILEHANDLEALREADYUSED); /* file handle already used! */

	/* open and add to handle list */
	fd.mode=mode;
	if(mode&OPEN_INPUT)
		fd.f=fopen(filename.GetStringObj()->GetString(),"rb");
	else
		fd.f=fopen(filename.GetStringObj()->GetString(),"wb");

	if(!fd.f)
		basicerrortrap(ERROR_FILEOPENERROR);
	else
		m_files.SetEntry(m_numfiles++,fd);
}

/* Close #x */
void kGUIBasic::cmd_close(void)
{
	kGUIBasicVarObj handle;
	file_def *fd;
	int slot=0;

	GetToken();
	if(m_tok!=BASICTOKEN_NUMBER)
		basicerror(ERROR_NUMBERSYMBOLEXPECTED); /* read and discard the '#' */

	exp_get(&handle);

	/* close end remove from handle list */
	fd=GetFileHandle(handle.GetInt(),&slot);
	if(!fd)
		basicerror(ERROR_FILENOTOPEN); /* file is not open! */
	fclose(fd->f);
	m_files.DeleteEntry(slot);
	--m_numfiles;
}

/* this is a trappable error */
void kGUIBasic::basicerrortrap(int error)
{
	call_def cs;

	cs=m_callstack.GetEntry(m_calldepth-1);
	if(!cs.error.go)
		basicerror(error);

	/* resume next? */
	if(cs.error.resume==1)
		return;				/* yes, just ignore the error */

	/* goto here, reset to stop possible endless loop  */
	m_pc=cs.error.go;
	cs.error.go=0;
	m_callstack.SetEntry(m_calldepth-1,cs);
	m_eolnext=false;
}

/* display an error message */
void kGUIBasic::basicerror(int error)
{
	if(m_errorcallback.IsValid() && m_pc)
	{
		if(m_asyncactive==true)
			kGUI::GetAccess();
		m_errorcallback.Call((int)((m_pc-1)->source-m_source->GetString()),(int)(m_pc->source-m_source->GetString()));
		if(m_asyncactive==true)
			kGUI::ReleaseAccess();
	}

	if(error<ERROR_USERERRORS)
		Print(errormessages[error]);
	else
	{
		kGUIString s;

		/* lookup error message in the user error messages hash table */
		s.Sprintf("%d",error);
		Print((const char *)(m_usererrors.Find(s.GetString())));
	}
	Print("\n");

	CloseFiles(false);

	if(m_thread.GetActive())
		m_thread.Close(false);

	m_asyncactive=false;
	if(m_donecallback.IsValid())
		m_donecallback.Call();

	longjmp(m_ebuf, 1); /* return to save point */
}

/* Get a token. */

const char *kGUIBasic::GetSourceToken(const char **startpc,const char *pc)
{
	register char *temp;
	const char *tpc;

	m_token_type=TT_UNDEFINED;
	m_tok=BASICTOKEN_UNKNOWN;
	temp=m_token;
	m_tokobj=0;

	if(*pc=='\0')
	{ 
		/* end of file */
		*m_token=0;
		m_tok = BASICTOKEN_FINISHED;
		m_token_type=TT_DELIMITER;
		return(pc);
	}

	while((*pc==' ') || (*pc=='\t'))
		++pc;  /* skip over white space */

	tpc=pc;	/* save pc for start of token */
	/* comment? */
	if(pc[0]=='\'')
	{
		pc++;
		while(*pc!='\n')
			++pc;
		SetSourceColor(tpc,pc,COMMENTCOLOR);
	}

	tpc=pc;	/* save pc for start of token */
	*(startpc)=tpc;

	if(*pc=='\n')
	{
		++pc;
		m_tok = BASICTOKEN_EOL;
		m_token[0]='\n';
		m_token[1]=0;
		m_token_type = TT_DELIMITER;
		return(pc);
	}

	if(*pc=='"')
	{ 
		/* quoted string */
		*temp++=*pc++;
		while(*pc!='"')
		{
		if(*pc=='\n')
			basicerror(ERROR_UNTERMINATEDQUOTES);
		*temp++=*pc++;
		}
		*temp++=*pc++;
		*temp=0;
		m_token_type=TT_QUOTE;
		return(pc);
	}
	else if(m_digittable[(unsigned char)*pc]==true)
	{
		*temp++=*pc++;
		/* TT_NUMBER */
		while(m_digittable[(unsigned char)*pc]==true)
			*temp++=*pc++;
		*temp = 0;

		if(m_token[0]=='.' && m_token[1]==0)
			m_token_type=TT_DELIMITER;
		else
		{
			m_token_type = TT_NUMBER;
			return(pc);
		}
	}
	else if(m_singledelimtable[(unsigned char)*pc]==true)
	{
		/* these are single character delimiters, do not append other characters to them ever */
		*(temp++)=*(pc++);
		*temp = 0;
		m_token_type=TT_DELIMITER;
	}
	else if(m_groupdelimtable[(unsigned char)*pc]==true)
	{
		*(temp++)=*(pc++);
		while(m_groupdelimtable[(unsigned char)*pc]==true)
		{
//			if(m_singledelimtable[(unsigned char)*pc]==true)	/* if it is one of these, then stop appending characters */
//				break;
			*(temp++)=*(pc++);
		}
		temp[0]=0;

		/* find longest delimiter that matches */
		m_tokobj=GetObj(m_token);
		while(m_tokobj==0)
		{
			if(--temp==m_token)	/* go back 1 characger */
				break;
			--pc;
			*(temp)=0;
			m_tokobj=GetObj(m_token);
		}
		m_token_type=TT_DELIMITER;
	}
	else if(m_alphatable[(unsigned char)*pc])
	{
		/* var or TT_COMMAND */
		while(m_alldelimtable[(unsigned char)*pc]==false)
			*temp++=*pc++;

		*temp = 0;
		if(*(pc)==':')
			m_token_type=TT_LABEL;	/* possibly a label? */
		else
			m_token_type=TT_VARIABLE;
	}
	else
	{
		*temp++=*pc++;	/* what the heck is this? */
		*temp=0;
		basicerror(ERROR_SYNTAX);
	}

	/* see if a string is a TT_VARIABLE or TT_DELIMITER, then lookup in the token table */
	if(m_token_type==TT_VARIABLE || m_token_type==TT_DELIMITER || m_token_type==TT_LABEL)
	{
		if(!m_tokobj)
			m_tokobj=GetObj(m_token);
		if(m_tokobj)
		{
			if(m_tokobj->GetType()==VARTYPE_COMMAND)
			{
				kGUIBasicCommandObj *cobj;

				cobj = static_cast<kGUIBasicCommandObj *>(m_tokobj);
				if(cobj->m_code.vcodev==0)
				{
					m_token_type = TT_COMMANDVERB;
					SetSourceColor(tpc,pc,COMMANDVERBCOLOR);
				}
				else
				{
					m_token_type =TT_COMMAND;
					SetSourceColor(tpc,pc,COMMANDCOLOR);
				}
				m_tok=cobj->GetID();
			}
			else if(m_tokobj->GetType()==VARTYPE_OPERATOR)
			{
				kGUIBasicCommandObj *cobj;

				cobj = static_cast<kGUIBasicCommandObj *>(m_tokobj);
				m_tok=cobj->GetID();
				SetSourceColor(tpc,pc,OPERATORCOLOR);
			}
			else if(m_tokobj->GetType()==VARTYPE_CFUNCTION)
				SetSourceColor(tpc,pc,FUNCTIONCOLOR);
		}
	}
	assert(m_token_type!=TT_UNDEFINED,"Unknown token type");
	return(pc);
}

/* get token from compiled token list */

void kGUIBasic::GetToken(bool getname)
{
	m_lastpc=m_pc;	/* save pointer to last token */

	m_token_type=m_pc->type;
	m_tok=m_pc->num;
	m_tokobj=m_pc->obj;

	if(!m_tokobj)
	{
		strncpy(m_token,m_pc->source,m_pc->len);
		m_token[m_pc->len]=0;

		if(m_token_type==TT_VARIABLE || m_token_type==TT_LABEL)
			m_tokobj=GetObj(m_token);
	}
	else
	{
		if(getname==true)
		{
			strncpy(m_token,m_pc->source,m_pc->len);
			m_token[m_pc->len]=0;
		}
		else
			m_token[0]=0;	/* no need to get token */
	}
	/* if hit end of source then don't increment, just stay there */
	if(m_tok!=BASICTOKEN_FINISHED)
		++m_pc;
}

/* skip to token TT_NUMBER supplied and abort if hit EOL */
bool kGUIBasic::skipto(int toknum)
{
	do
	{
		GetToken();
		if(m_tok==toknum)
			return(true);
		if(m_tok==BASICTOKEN_EOL)
			return(false);		/* token not found! */
	}while(1);
}

void kGUIBasic::exp_synctypes(kGUIBasicVarObj *v1,kGUIBasicVarObj *v2)
{
	int newtype;

	/* quick return if these types are already matched */
	if(v1->GetType()==v2->GetType())
		return;	

	if(v1->GetType()==VARTYPE_STRING || v2->GetType()==VARTYPE_STRING)
		newtype=VARTYPE_STRING;
	else if(v1->GetType()==VARTYPE_DOUBLE || v2->GetType()==VARTYPE_DOUBLE)
		newtype=VARTYPE_DOUBLE;
	else if(v1->GetType()==VARTYPE_INTEGER || v2->GetType()==VARTYPE_INTEGER)
		newtype=VARTYPE_INTEGER;
	else
	{
		assert(false,"Internal Error, conversion not supported!");
		return;
	}
	v1->ChangeType(newtype);
	v2->ChangeType(newtype);
}

/* return -1,0,1 */
int kGUIBasic::exp_vardelta(kGUIBasicVarObj *v1,kGUIBasicVarObj *v2)
{
	exp_synctypes(v1,v2);
	switch(v1->GetType())
	{
	case VARTYPE_INTEGER:
		if(v1->GetInt()==v2->GetInt())
			return(0);
		if(v1->GetInt()>v2->GetInt())
			return(1);
		else
			return(-1);
	break;
	case VARTYPE_DOUBLE:
		if(v1->GetDouble()==v2->GetDouble())
			return(0);
		else if(v1->GetDouble()>v2->GetDouble())
			return(1);
		else
			return(-1);
	break;
	case VARTYPE_BOOLEAN:
		if(v1->GetBoolean()==v2->GetBoolean())
			return(0);
		else if(v1->GetBoolean()==true && v2->GetBoolean()==false)
			return(1);
		else
			return(-1);
	break;
	case VARTYPE_STRING:
	{
		/* since strings may contain nulls use memcmp instead of strcmp */
		int d=memcmp(v1->GetStringObj()->GetString(),v2->GetStringObj()->GetString(),max(v1->GetStringObj()->GetLen(),v2->GetStringObj()->GetLen()));
		if(d>1)
			d=1;
		else if(d<-1)
			d=-1;

		return(d);
	}
	break;
	default:
		assert(false,"Internal Error, comparision not supported!");
	break;
	}
	return(0);	/* never gets here, just making the compiler stop complaining */
}

//	"id++", "id--"		[post-increment and post-decrement]
//	"++id", "--id"		[pre-increment and pre-decrement]
//	"-", "+"		[(unary operators)]
//	"!", "~"
//	"**"			[(exponentiation)]
//	"*", "/", "%"
//	exp_addsub		"+", "-"
//	exp_shift		"<<", ">>"
//	"<=", ">=", "<", ">"
//	"==", "!="
//	"&"
//	"^"
//	"|"
//	"&&"
//	"||"
//	"expr ? expr : expr"
//	"=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|="

/* Entry point into expression evaluator. */
void kGUIBasic::exp_get(kGUIBasicVarObj *result)
{
	GetToken();
	exp_cond(result);
	Rewind(); /* return last m_token read to input stream */
}

/* value?v1:v2 */

void kGUIBasic::exp_cond(kGUIBasicVarObj *r)
{
	exp_lor(r);
	while (m_tok==BASICTOKEN_QUESTIONMARK)
	{
		kGUIBasicVarObj r1;
		kGUIBasicVarObj r2;

		GetToken(); 

		exp_get(&r1);
		if(m_token[0]!=':')
			basicerror(ERROR_COLONEXPECTED);
		GetToken(); 
		exp_get(&r2);

		if(r->GetBoolean()==true)
			r->Copy(&r1);
		else
			r->Copy(&r2);
    }
}

/* logical or */
void kGUIBasic::exp_lor(kGUIBasicVarObj *r)
{
	exp_land(r);
	while(m_tok==BASICTOKEN_LOGICALOR)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_land(&hold);

		r->ChangeType(VARTYPE_INTEGER);
		hold.ChangeType(VARTYPE_INTEGER);
		r->Set(r->GetInt() || hold.GetInt());
	}
}

/* logical and */
void kGUIBasic::exp_land(kGUIBasicVarObj *r)
{
	exp_bor(r);
	while(m_tok==BASICTOKEN_LOGICALAND)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_bor(&hold);

		r->ChangeType(VARTYPE_INTEGER);
		hold.ChangeType(VARTYPE_INTEGER);
		r->Set(r->GetInt() && hold.GetInt());
	}
}

/* bitwise or */
void kGUIBasic::exp_bor(kGUIBasicVarObj *r)
{
	exp_bxor(r);
	while(m_tok==BASICTOKEN_BITWISEOR)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_bxor(&hold);

		r->ChangeType(VARTYPE_INTEGER);
		hold.ChangeType(VARTYPE_INTEGER);
		r->Set(r->GetInt() | hold.GetInt());
	}
}

/* bitwise xor */
void kGUIBasic::exp_bxor(kGUIBasicVarObj *r)
{
	exp_band(r);
	while(m_tok==BASICTOKEN_BITWISEEXCLUSIVEOR)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_band(&hold);

		r->ChangeType(VARTYPE_INTEGER);
		hold.ChangeType(VARTYPE_INTEGER);
		r->Set(r->GetInt() ^ hold.GetInt());
	}
}

/* bitwise and */
void kGUIBasic::exp_band(kGUIBasicVarObj *r)
{
	exp_eqneq(r);

	while(m_tok==BASICTOKEN_BITWISEAND)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_eqneq(&hold);

		/* if either is a string then '&' is for concatenation and not boolean "and" */
		if((r->GetType()==VARTYPE_STRING) || (hold.GetType()==VARTYPE_STRING))
		{
			r->ChangeType(VARTYPE_STRING);
			hold.ChangeType(VARTYPE_STRING);
			r->GetStringObj()->Append(hold.GetStringObj());
		}
		else
			r->Set(r->GetInt() & hold.GetInt());
	}
}

void kGUIBasic::exp_eqneq(kGUIBasicVarObj *r)
{
	exp_cmp(r);
	while(m_tok==BASICTOKEN_TESTEQUAL || m_tok==BASICTOKEN_EQUALS || m_tok==BASICTOKEN_TESTNOTEQUAL)
	{
		int op;
		int v;
		kGUIBasicVarObj hold; 

		op=m_tok;

		GetToken(); 
		exp_cmp(&hold);

		if(exp_vardelta(r,&hold)==0)
			v=1;
		else
			v=0;
		if(op==BASICTOKEN_TESTNOTEQUAL)
			v^=1;

		r->Set(v);
	}
}

void kGUIBasic::exp_cmp(kGUIBasicVarObj *r)
{
	exp_shift(r);

	while(m_tok==BASICTOKEN_TESTLESSTHAN || m_tok==BASICTOKEN_TESTLESSTHANOREQUAL || m_tok==BASICTOKEN_TESTGREATERTHAN || m_tok==BASICTOKEN_TESTGREATERTHANOREQUAL)
	{
		int op,cmp;
		bool v=false;
		kGUIBasicVarObj hold; 

		op=m_tok;

		GetToken(); 
		exp_shift(&hold);

		cmp=exp_vardelta(r,&hold);
		switch(op)
		{
		case BASICTOKEN_TESTLESSTHAN:
			v=(cmp==-1);
		break;
		case BASICTOKEN_TESTLESSTHANOREQUAL:
			v=(cmp<=0);
		break;
		case BASICTOKEN_TESTGREATERTHAN:
			v=(cmp==1);
		break;
		case BASICTOKEN_TESTGREATERTHANOREQUAL:
			v=(cmp>=0);
		break;
		}
		r->Set(v==true?1:0);
	}
}

/* left shift '<<' or right shift '>>' */
void kGUIBasic::exp_shift(kGUIBasicVarObj *r)
{
	exp_addsub(r);
	while(m_tok==BASICTOKEN_BITWISESHIFTLEFT || m_tok==BASICTOKEN_BITWISESHIFTRIGHT)
	{
		int op;
		kGUIBasicVarObj hold; 

		op=m_tok;

		GetToken(); 
		exp_addsub(&hold);

		r->ChangeType(VARTYPE_INTEGER);
		hold.ChangeType(VARTYPE_INTEGER);
		if(m_tok==BASICTOKEN_BITWISESHIFTLEFT)
			r->Set(r->GetInt()<<(hold.GetInt()));
		else
			r->Set(r->GetInt()>>(hold.GetInt()));
	}
}

void kGUIBasic::exp_add(kGUIBasicVarObj *r,kGUIBasicVarObj *a)
{
	exp_synctypes(r,a);

	switch(r->GetType())
	{
	case VARTYPE_INTEGER:
		r->Set(r->GetInt()+a->GetInt());
	break;
	case VARTYPE_DOUBLE:
		r->Set(r->GetDouble()+a->GetDouble());
	break;
	case VARTYPE_STRING:
		r->GetStringObj()->Append(a->GetStringObj());
	break;
	default:
		/* I don't know how to add these */
		basicerror(ERROR_BADTYPEFORCOMMAND);
	break;
	}
}

void kGUIBasic::exp_sub(kGUIBasicVarObj *r,kGUIBasicVarObj *a)
{
	exp_synctypes(r,a);

	switch(r->GetType())
	{
	case VARTYPE_INTEGER:
		r->Set(r->GetInt()-a->GetInt());
	break;
	case VARTYPE_DOUBLE:
		r->Set(r->GetDouble()-a->GetDouble());
	break;
	default:
		/* I don't know how to add these */
		basicerror(ERROR_BADTYPEFORCOMMAND);
	break;
	}
}

/*  Add or subtract two terms. */
void kGUIBasic::exp_addsub(kGUIBasicVarObj *r)
{
	int op; 

	exp_muldivmod(r);
	while((op = m_tok) == BASICTOKEN_ADD || op == BASICTOKEN_SUBTRACT)
	{
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_muldivmod(&hold);

		if(op==BASICTOKEN_ADD)
			exp_add(r,&hold);
		else
			exp_sub(r,&hold);
	}
}

/* Multiply or divide two factors. */
void kGUIBasic::exp_muldivmod(kGUIBasicVarObj *r)
{
	int  op; 
	exp_exponent(r); 
	while((op = m_tok) == BASICTOKEN_MULTIPLY || op == BASICTOKEN_DIVIDE || op == BASICTOKEN_MODULO)
	{
		kGUIBasicVarObj hold;

		GetToken(); 
		exp_exponent(&hold); 

		if(op==BASICTOKEN_MULTIPLY)
		{
			exp_synctypes(r,&hold);
			switch(r->GetType())
			{
			case VARTYPE_INTEGER:
				r->Set(r->GetInt()*hold.GetInt());
			break;
			case VARTYPE_DOUBLE:
				r->Set(r->GetDouble()*hold.GetDouble());
			break;
			default:
				basicerror(ERROR_BADTYPEFORCOMMAND);
			break;
			}
		}
		else if(op==BASICTOKEN_DIVIDE)
		{
			exp_synctypes(r,&hold);
			switch(r->GetType())
			{
			case VARTYPE_INTEGER:
				r->Set(r->GetInt()/hold.GetInt());
			break;
			case VARTYPE_DOUBLE:
				r->Set(r->GetDouble()/hold.GetDouble());
			break;
			default:
				basicerror(ERROR_BADTYPEFORCOMMAND);
			break;
			}
		}
		else //	if(op==BASICTOKEN_MODULO)
		{
			r->ChangeType(VARTYPE_INTEGER);		/* int is only valid type for this */
			hold.ChangeType(VARTYPE_INTEGER);	/* int is only valid type for this */
			r->Set(r->GetInt()%hold.GetInt());
		}
	}
}

/* Process integer exponent. */
void kGUIBasic::exp_exponent(kGUIBasicVarObj *r)
{
	exp_not(r); 
	if(m_tok==BASICTOKEN_EXPONENT)
	{
		int i,n,per;
		kGUIBasicVarObj hold; 

		GetToken(); 
		exp_exponent(&hold); 

		n=hold.GetInt();
		if(n==0)
			r->Set((int)1);
		else if(n>0)
		{
			per=r->GetInt();
			for(i=0;i<n;++i)
				r->Set(r->GetInt()*per);
		}
		else	/* less than zero = 1/(r^n) */
		{
			n=-n;
			per=r->GetInt();
			for(i=0;i<n;++i)
				r->Set(r->GetInt()*per);
			r->Set(1/r->GetInt()*per);
		}
	}
}

/* not */
void kGUIBasic::exp_not(kGUIBasicVarObj *r)
{
	int op; 

	if(m_tok==BASICTOKEN_NOT)
	{
		op = m_tok; 
		GetToken(); 
	}
	else
		op=0;

	exp_unary(r); 
	if(op==BASICTOKEN_NOT)
	{
		switch(r->GetType())
		{
		case VARTYPE_INTEGER:
			r->Set(!r->GetInt());
		break;
		case VARTYPE_BOOLEAN:
			r->Set(!r->GetBoolean());
		break;
		default:
			basicerror(ERROR_BADTYPEFORCOMMAND);
		break;
		}
	}
}

/* Is a unary + or -. */
void kGUIBasic::exp_unary(kGUIBasicVarObj *r)
{
	int op; 

	if((m_tok==BASICTOKEN_ADD || m_tok==BASICTOKEN_SUBTRACT))
	{
		op = m_tok; 
		GetToken(); 
	}
	else
		op=0;

	exp_parenthesis(r); 
	if(op==BASICTOKEN_SUBTRACT)
	{
		switch(r->GetType())
		{
		case VARTYPE_INTEGER:
			r->Set(-r->GetInt());
		break;
		case VARTYPE_DOUBLE:
			r->Set(-r->GetDouble());
		break;
		default:
			basicerror(ERROR_BADTYPEFORCOMMAND);
		break;
		}
	}
}

/* Process parenthesized expression. */
void kGUIBasic::exp_parenthesis(kGUIBasicVarObj *result)
{
	int vtype;
	
	if(m_tok==BASICTOKEN_OPENBRACKET)
	{
		/* is this a type cast? */		
		GetToken();
		switch(m_tok)
		{
		case BASICTOKEN_STRING:
			vtype=VARTYPE_STRING;
		break;
		case BASICTOKEN_INTEGER:
			vtype=VARTYPE_INTEGER;
		break;
		case BASICTOKEN_DOUBLE:
			vtype=VARTYPE_DOUBLE;
		break;
		case BASICTOKEN_BOOLEAN:
			vtype=VARTYPE_BOOLEAN;
		break;
		default:
			/* regular brackets, nothing to see here, move along */
			Rewind();
			exp_get(result); /* does a Rewind at end */

			GetToken(); 
			if(m_tok!=BASICTOKEN_CLOSEBRACKET)
				basicerror(ERROR_UNBALANCEDPAREN);
			GetToken();
			return;
		break;
		}
		/* cast was detected, get primitive then change to desired type */
		GetToken();
		if(m_tok!=BASICTOKEN_CLOSEBRACKET)
			basicerror(ERROR_UNBALANCEDPAREN);
	
		GetToken();
		exp_primitive(result);
		/* change type to desired type */
		result->ChangeType(vtype);
	}
	else
		exp_primitive(result);
}

/* Find value of constant or variable. */
void kGUIBasic::exp_primitive(kGUIBasicVarObj *result)
{
	bool predec=false;
	bool preinc=false;
	kGUIBasicObj *obj;

	if(m_tok==BASICTOKEN_INCREMENT)
	{
		preinc=true;
		GetToken();
	}
	else if(m_tok==BASICTOKEN_DECREMENT)
	{
		predec=true;
		GetToken();
	}

	if(m_tok==BASICTOKEN_FIELDPREFIX)
		goto isvar;

	switch(m_token_type)
	{
	case TT_COMMAND:
		/* special case where command and function have the same name */
		/* currently the input command is both a command and a function */
		obj=(static_cast<kGUIBasicCommandObj *>(m_tokobj))->GetExtra();
		if(!obj)
			basicerror(ERROR_EXPESSIONPRIMITIVEEXPECTED);
		goto isfunc;
	break;
	case TT_VARIABLE:
	{
isvar:	Rewind();
		obj=GetFinalObj();
isfunc:;
		switch(obj->GetType())
		{
		case VARTYPE_COMMAND:
		break;
		case VARTYPE_CFUNCTION:
		case VARTYPE_USUB:
		case VARTYPE_UFUNCTION:
		case VARTYPE_SYSCLASSFUNC:
			Call(obj,result);
		break;
		case VARTYPE_INTEGER:
		case VARTYPE_DOUBLE:
		case VARTYPE_STRING:
		case VARTYPE_BOOLEAN:
		case VARTYPE_VARIANT:
		{
			kGUIBasicVarObj *vobj;

			vobj=static_cast<kGUIBasicVarObj *>(obj);
			if(preinc==true)
			{
				preinc=false;
				vobj->Set(vobj->GetInt()+1);
			}
			else if(predec==true)
			{
				predec=false;
				vobj->Set(vobj->GetInt()-1);
			}
			result->Copy(vobj);	/* copy var value to result */

			/* i'm doing this after instead of before since var may have a pre-load callback that sets it's value */
			if(vobj->GetIsUndefined()==true)
				basicerror(ERROR_UNINITIALIZEDVAR);
			
			GetToken();
			if(m_tok==BASICTOKEN_INCREMENT)
			{
				CheckType(vobj,VARTYPE_INTEGER);
				vobj->Set(vobj->GetInt()+1);
			}
			else if(m_tok==BASICTOKEN_DECREMENT)
			{
				CheckType(vobj,VARTYPE_INTEGER);
				vobj->Set(vobj->GetInt()-1);
			}
			else
				Rewind();

		}
		break;
		default:
			basicerror(ERROR_NOTAVARIABLE);
		break;
		}
	}
	break; 
	case TT_NUMBER:
		if(strstr(m_token,"."))
			result->Set(atof(m_token));
		else
			result->Set(atoi(m_token));
	break;
	case TT_QUOTE:
	{
		int l=(int)strlen(m_token+1);

		m_token[l]=0;			/* put null over closing quote */
		result->Set(m_token+1);	/* skip opening quote */
	}
	break;
	default:
		basicerror(ERROR_EXPESSIONPRIMITIVEEXPECTED);
	break;
	}
	/* if these are still around then they were not "used" and therefore there is a problem houston! */
	if(preinc==true || predec==true)
		basicerror(ERROR_SYNTAX);
	GetToken();
}

/*****************************************************************/

void kGUIBasic::func_rand(kGUIBasicVarObj *result,CallParms *p)
{
	result->Set(rand());
}

/* return the length of a string as an integer */
void kGUIBasic::func_len(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	v1->ChangeType(VARTYPE_STRING);
	result->Set((int)v1->GetStringObj()->GetLen());
}

/* return ascii value of first character in string, return 0 if null string */
void kGUIBasic::func_asc(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	int c;

	v1->ChangeType(VARTYPE_STRING);
	if(!v1->GetStringObj()->GetLen())
		c=0;
	else
		c=v1->GetStringObj()->GetChar(0);

	result->Set(c);
}

/* generate a string character from an integer */
void kGUIBasic::func_chr(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	char s[2];

	/* i've coded this like this since it can be char(0) and it would screw up otherwise */
	s[0]=(char)v1->GetInt();
	s[1]=0;
	result->Set(s,1);
}

/* convert a number into a string */
void kGUIBasic::func_str(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIString s;

	v1->ChangeType(VARTYPE_STRING);
	result->Set(v1->GetStringObj()->GetString());
}

/* convert a string into a number */
void kGUIBasic::func_val(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIString s;

	result->Set(v1->GetInt());
}

/* convert a string into a number */
void kGUIBasic::func_dval(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIString s;

	result->Set(v1->GetDouble());
}


void kGUIBasic::func_sin(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(sin(v1->GetDouble()));
}

void kGUIBasic::func_cos(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(cos(v1->GetDouble()));
}

void kGUIBasic::func_tan(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(tan(v1->GetDouble()));
}

void kGUIBasic::func_asin(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(asin(v1->GetDouble()));
}

void kGUIBasic::func_acos(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(acos(v1->GetDouble()));
}

void kGUIBasic::func_atan(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);

	result->Set(atan(v1->GetDouble()));
}

void kGUIBasic::func_eof(kGUIBasicVarObj *result,CallParms *p)
{
	file_def *fd;

	fd=GetFileHandle(p->GetParm(0)->GetInt());
	if(!fd)
		basicerror(ERROR_FILENOTOPEN); /* file is not open! */

	result->Set(feof(fd->f)!=0);
}


/* left(string,numchars) */
void kGUIBasic::func_left(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIBasicVarObj *v2=p->GetParm(1);
	int nc,sl;
	kGUIString s;

	v1->ChangeType(VARTYPE_STRING);

	nc=v2->GetInt();
	sl=v1->GetStringObj()->GetLen();
	if(nc>sl)
		nc=sl;

	s.SetString(v1->GetStringObj()->GetString(),nc);

	result->Set(s.GetString());
}

/* right(string,numchars) */
void kGUIBasic::func_right(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIBasicVarObj *v2=p->GetParm(1);
	int nc,sl;
	kGUIString s;

	v1->ChangeType(VARTYPE_STRING);

	nc=v2->GetInt();
	sl=v1->GetStringObj()->GetLen();
	if(nc>sl)
		nc=sl;

	s.SetString(v1->GetStringObj()->GetString()+sl-nc);
	result->Set(s.GetString());
}

/* remember in basic the first character is index 1 ( not zero ) */
void kGUIBasic::func_mid(kGUIBasicVarObj *result,CallParms *p)
{
	int si,sl;
	unsigned int nc;
	kGUIString s;
	kGUIBasicVarObj *v1;
	kGUIBasicVarObj *v2;
	kGUIBasicVarObj *v3;

	switch(p->GetNumParms())
	{
	case 2:
		v1=p->GetParm(0);
		v2=p->GetParm(1);
		v1->ChangeType(VARTYPE_STRING);
		sl=v1->GetStringObj()->GetLen();
		si=v2->GetInt()-1;
		if(si<sl && si>=0)
			s.SetString(v1->GetStringObj()->GetString()+si);
		result->Set(s.GetString());
	break;
	case 3:
		v1=p->GetParm(0);
		v2=p->GetParm(1);
		v3=p->GetParm(2);

		v1->ChangeType(VARTYPE_STRING);
		sl=v1->GetStringObj()->GetLen();
		si=v2->GetInt()-1;
		nc=(unsigned int)v3->GetInt();
		if(si<sl && si>=0)
		{
			s.SetString(v1->GetStringObj()->GetString()+si);
			if(s.GetLen()>nc)
				s.Clip(nc);
		}
		result->Set(s.GetString());
	break;
	default:
		basicerror(ERROR_WRONGNUMBEROFPARAMETERS);	/* wrong num of parms */
	break;
	}
}

/* example: clicked=MsgBox("message",MSGBOX_YES+MSGBOX_NO) */

void kGUIBasic::func_msgbox(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj *v1=p->GetParm(0);
	kGUIBasicVarObj *v2=p->GetParm(1);
	kGUIMsgBoxReq *message;

	/* only valid in async mode */
	if(m_asyncactive==false)
		basicerror(ERROR_COMMAEXPECTED);

	kGUI::GetAccess();
	m_msgresult=0;
	message=new kGUIMsgBoxReq(v2->GetInt(),this,CALLBACKNAME(MessageDone),false,v1->GetStringObj()->GetString());
	kGUI::ReleaseAccess();

	while(m_msgresult==0)
		kGUI::Sleep(10);

	/* set results to button that was pressed */
	result->Set(m_msgresult);
}

/* example: filename=FileReq(mode,path,extension) */

void kGUIBasic::func_filereq(kGUIBasicVarObj *result,CallParms *p)
{
	int mode=p->GetParm(0)->GetInt();
	const char *path=p->GetParm(1)->GetStringObj()->GetString();
	const char *ext=p->GetParm(2)->GetStringObj()->GetString();
	kGUIFileReq *req;

	/* only valid in async mode */
	if(m_asyncactive==false)
		basicerror(ERROR_COMMAEXPECTED);

	kGUI::GetAccess();
	m_filereqresult=0;
	req=new kGUIFileReq(mode,path,ext,this,CALLBACKNAME(FileReqDone));
	kGUI::ReleaseAccess();

	while(m_filereqresult==0)
		kGUI::Sleep(10);

	result->Set(m_filereqstring.GetString());
	if(m_filereqresult==MSGBOX_CANCEL)
		basicerrortrap(ERROR_USERCANCELLEDINPUT);
}

/* shellexec(program,parms,directory) */
void kGUIBasic::func_shellexec(kGUIBasicVarObj *result,CallParms *p)
{
	const char *program=p->GetParm(0)->GetStringObj()->GetString();
	const char *parms=p->GetParm(1)==0?0:p->GetParm(1)->GetStringObj()->GetString();
	const char *dir=p->GetParm(2)==0?0:p->GetParm(2)->GetStringObj()->GetString();

	kGUI::ShellExec(program,parms,dir);
}

/* sort an array of objects */
/* sortobjects(onedimclass/structur ptr,numentries,sortstring) */

typedef struct
{
	int numsortfields;
	int *sortfields;
	bool *sortdirs;
}SORTINFO_DEF;

static int qs_sortobjects(const void *v1,const void *v2)
{
	int f,delta;
	SORTINFO_DEF *si;

	kGUIBasicVarObj *b1;
	kGUIBasicVarObj *b2;

	b1=*((kGUIBasicVarObj **)v1);
	b2=*((kGUIBasicVarObj **)v2);

	si=(SORTINFO_DEF *)b1->GetUserDataPtr();;

	f=0;
	do
	{
		delta=kGUIBasic::exp_vardelta(b1->GetEntry(si->sortfields[f]),b2->GetEntry(si->sortfields[f]));
		if(si->sortdirs[f]==true)	/* reverse? */
			delta=-delta;
		++f;
	}while(f<si->numsortfields && !delta);
	
	return(delta);
}

void kGUIBasic::func_sortobjects(kGUIBasicVarObj *result,CallParms *p)
{
	int i,num;
	kGUIBasicVarObj *v1;
	kGUIBasicVarObj *v2;
	kGUIBasicVarObj *v3;
	kGUIStringSplit ss;
	kGUIBasicStructObj *sobj;
	const char *cp;
	SORTINFO_DEF si;
	Array<kGUIBasicVarObj *>ptrs;
	Array<kGUIBasicVarObj *>newptrs;

	v1=p->GetParm(0);		/* array pointer */
	v2=p->GetParm(1);		/* number of entries */
	v3=p->GetParm(2);		/* sort fields string */

	if(v1->GetType()!=VARTYPE_STRUCT)
		basicerror(ERROR_SYNTAX);

	sobj=static_cast<kGUIBasicStructObj *>(v1->GetDefPtr());


	si.numsortfields=ss.Split(v3->GetStringObj(),",");
	si.sortfields=new int[si.numsortfields];
	si.sortdirs=new bool[si.numsortfields];
	
	for(i=0;i<si.numsortfields;++i)
	{
		cp=ss.GetWord(i)->GetString();
		si.sortdirs[i]=cp[0]=='-';
		if(cp[0]=='-')
			++cp;
		si.sortfields[i]=sobj->GetFieldIndex(cp);
		if(si.sortfields[i]<0)
			basicerror(ERROR_SYNTAX);	/* field not in struct */
	}
	num=v2->GetInt();

	ptrs.Alloc(num);
	newptrs.Alloc(num);
	for(i=0;i<num;++i)
	{
		ptrs.SetEntry(i,v1->GetEntry(i));
		v1->GetEntry(i)->SetUserData(&si);
	}
	qsort(ptrs.GetArrayPtr(),num,sizeof(kGUIBasicVarObj *),qs_sortobjects);
	/* build a list of new entry pointers */
	for(i=0;i<num;++i)
		newptrs.SetEntry(i,ptrs.GetEntry(i)->m_xarrayentries);
	/* update the pointers */
	for(i=0;i<num;++i)
		v1->GetEntry(i)->m_xarrayentries=newptrs.GetEntry(i);
	delete []si.sortfields;
	delete []si.sortdirs;
}

/* a$=input(#chars,handle) */

void kGUIBasic::func_input(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIBasicVarObj handle;
	file_def *fd;
	int numchars=p->GetParm(0)->GetInt();
	int numread;
	char *buffer;

	fd=GetFileHandle(p->GetParm(1)->GetInt());
	if(!fd)
		basicerror(ERROR_FILENOTOPEN); /* file is not open! */

	buffer=new char[numchars+1];
	numread=(int)fread(buffer,numchars,1,fd->f);
	buffer[numread]=0;
	result->Set(buffer,numread);	/* send length since it may contain nulls */
	delete []buffer;

	/* hmmm, if numread !=numchars then goto "on error" function? */

	if(numread!=numchars)
		basicerrortrap(ERROR_FILEREADERROR);
}

/* if result is "true" result = parm1, else result=parm2 */
void kGUIBasic::func_iif(kGUIBasicVarObj *result,CallParms *p)
{
	if(p->GetParm(0)->GetBoolean()==true)
		result->Copy(p->GetParm(1));
	else
		result->Copy(p->GetParm(2));
}

void kGUIBasic::func_trim(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIString s;

	p->GetParm(0)->GetString(&s);
	s.Trim();
	result->Set(&s);
}

void kGUIBasic::func_fileexists(kGUIBasicVarObj *result,CallParms *p)
{
	result->Set(kGUI::FileExists(p->GetParm(0)->GetStringObj()->GetString()));
}

/* return array size for index requested */
void kGUIBasic::func_ubound(kGUIBasicVarObj *result,CallParms *p)
{
	int index=1;
	kGUIBasicVarObj *var;

	if(p->GetNumParms()<1 || p->GetNumParms()>2)
		basicerror(ERROR_WRONGNUMBEROFPARAMETERS);	/* wrong num of parms */

	if(p->GetNumParms()==2)
		index=p->GetParm(1)->GetInt();

	var=p->GetParm(0);
	if(index>var->GetNumIndices() || index<0)
		basicerror(ERROR_ARRAYINDEXOUTOFRANGE);

	result->Set(var->GetIndice(index-1));
}

// array=split(string,delimiter,casemode)
void kGUIBasic::func_split(kGUIBasicVarObj *result,CallParms *p)
{
	int i;
	kGUIString s;
	int num,mode;
	kGUIStringSplit	ss;
	kGUIBasicIndices indices;

	mode=p->GetParm(2)!=0?p->GetParm(2)->GetInt():0;

	s.SetString(p->GetParm(0)->GetStringObj());
	num=ss.Split(&s,p->GetParm(1)->GetStringObj()->GetString(),mode);

	indices.AddIndice(num);
	result->ReDim(&indices,GetObj("String"),false);
	for(i=0;i<num;++i)
	{
		result->GetEntry(i)->Set(ss.GetWord(i)->GetString());
		result->GetEntry(i)->SetIsUndefined(false);
	}
}

//InStr([start, ] string1, string2 [, compare ] )
void kGUIBasic::func_instr(kGUIBasicVarObj *result,CallParms *p)
{
	int startchar=p->GetParm(0)->GetInt();
	int fslen=p->GetParm(1)->GetStringObj()->GetLen();
	const char *fullstring=p->GetParm(1)->GetStringObj()->GetString();
	const char *look=p->GetParm(2)->GetStringObj()->GetString();
	int mode;
	const char *foundat;

	mode=p->GetParm(3)!=0?p->GetParm(3)->GetInt():0;

	if(startchar<1 || startchar>fslen)
		basicerror(ERROR_SYNTAX);		/* illegal index */

	/* case sensative? */
	if(!mode)
		foundat=strstr(fullstring+(startchar-1),look);
	else
		foundat=strstri(fullstring+(startchar-1),look);

	if(!foundat)
		result->Set((int)0);
	else
		result->Set((int)(foundat-fullstring)+1);
}

//Replace(expression, find, replace [, start ] [, count ] [, compare ] )
void kGUIBasic::func_replace(kGUIBasicVarObj *result,CallParms *p)
{
	kGUIString s;
	int startchar;
	int mode;
	int maxchanges;

	startchar=p->GetParm(3)!=0?p->GetParm(3)->GetInt():1;
	maxchanges=p->GetParm(4)!=0?p->GetParm(4)->GetInt():-1;
	mode=p->GetParm(5)!=0?p->GetParm(5)->GetInt():0;

	p->GetParm(0)->GetString(&s);
	if(startchar<1 || (startchar-1)>(int)s.GetLen())
		basicerror(ERROR_SYNTAX);		/* illegal index */

	s.Replace(	p->GetParm(1)->GetStringObj()->GetString(),	/* from */
				p->GetParm(2)->GetStringObj()->GetString(),	/* to */
				startchar-1,								/* start offset */
				mode,										/* compare mode */
				maxchanges);								/* maxchanges */
	result->Set(&s);
}

int kGUIBasic::GetTypeFromToken(int token)
{
	int vtype;

	switch(token)
	{
	case BASICTOKEN_STRING:
		vtype=VARTYPE_STRING;
	break;
	case BASICTOKEN_INTEGER:
		vtype=VARTYPE_INTEGER;
	break;
	case BASICTOKEN_DOUBLE:
		vtype=VARTYPE_DOUBLE;
	break;
	case BASICTOKEN_BOOLEAN:
		vtype=VARTYPE_BOOLEAN;
	break;
	case BASICTOKEN_VARIANT:
		vtype=VARTYPE_VARIANT;
	break;
	default:
		return(-1);
	break;
	}
	return(vtype);
}


/************************************************************************************/
/* put built-in classes here														*/
/************************************************************************************/

typedef struct
{
	int id;
	const char *name;
	int np;
	const char *parmdef;
}fntoid_def;

enum
{
DATE_SETNOW,
DATE_SETDATE,
DATE_SETDAY,
DATE_SETMONTH,
DATE_SETYEAR,
DATE_SETHOUR,
DATE_SETMINUTE,
DATE_SETSECOND,
DATE_GETDAY,
DATE_GETMONTH,
DATE_GETYEAR,
DATE_GETHOUR,
DATE_GETMINUTE,
DATE_GETSECOND,
DATE_GETDAYOFWEEK,
DATE_GETDIFFSECONDS,
DATE_GETDIFFMINUTES,
DATE_GETDIFFHOURS,
DATE_GETDIFFDAYS,
DATE_COPY,
DATE_GETSHORTDATE,
DATE_GETLONGDATE,
DATE_ADDDAYS,
DATE_NUMCOMMANDS};

/*! @internal @class BasicDate
    @brief Internal class used by the kGUIBasic class.
	Date class for basic programs. */
class BasicDate : public kGUIBasicClassObj
{
public:
	BasicDate();
	~BasicDate() {}
	static kGUIBasicClassObj *Alloc(void) {return new BasicDate();}
	kGUIBasicObj *GetObj(const char *name);
	int Function(int fieldid,kGUIBasicVarObj *result,CallParms *p);
private:
	kGUIDate m_date;
	kGUIBasicSysClassFuncObj m_commands[DATE_NUMCOMMANDS];
	Hash m_fieldhash;	/* small hash list for field name to index conversion */
};


fntoid_def DateFuncs[]={
	{DATE_SETNOW,			"SetNow",			0,""},
	{DATE_SETDATE,			"SetDate",			1,"byval string"},
	{DATE_SETDAY,			"SetDay",			1,"byval integer"},
	{DATE_SETMONTH,			"SetMonth",			1,"byval integer"},
	{DATE_SETYEAR,			"SetYear",			1,"byval integer"},
	{DATE_SETHOUR,			"SetHour",			1,"byval integer"},
	{DATE_SETMINUTE,		"SetMinute",		1,"byval integer"},
	{DATE_SETSECOND,		"SetSecond",		1,"byval integer"},
	{DATE_ADDDAYS,			"AddDays",			1,"byval integer"},
	{DATE_GETDAY,			"GetDay",			0,""},
	{DATE_GETMONTH,			"GetMonth",			0,""},
	{DATE_GETYEAR,			"GetYear",			0,""},
	{DATE_GETHOUR,			"GetHour",			0,""},
	{DATE_GETMINUTE,		"GetMinute",		0,""},
	{DATE_GETSECOND,		"GetSecond",		0,""},
	{DATE_GETDAYOFWEEK,		"GetDayofWeek",		0,""},
	{DATE_GETDIFFSECONDS,	"GetDiffSeconds",	1,"byref variant"},
	{DATE_GETDIFFMINUTES,	"GetDiffMinutes",	1,"byref variant"},
	{DATE_GETDIFFHOURS,		"GetDiffHours",		1,"byref variant"},
	{DATE_GETDIFFDAYS,		"GetDiffDays",		1,"byref variant"},
	{DATE_COPY,				"Copy",				1,"byref variant"},
	{DATE_GETSHORTDATE,		"GetShortDate",		0,""},
	{DATE_GETLONGDATE,		"GetLongDate",		0,""}};

BasicDate::BasicDate()
{
	int i;
	SetType(VARTYPE_CLASS);
	m_fieldhash.Init(8,sizeof(int));

	/* add functions next */
	for(i=0;i<DATE_NUMCOMMANDS;++i)
	{
		m_fieldhash.Add(DateFuncs[i].name,&DateFuncs[i].id);
		m_commands[DateFuncs[i].id].Init(this,DateFuncs[i].id,DateFuncs[i].np,DateFuncs[i].parmdef);
	}
}

kGUIBasicObj *BasicDate::GetObj(const char *name)
{
	int *pid;

	pid=(int *)m_fieldhash.Find(name);
	if(!pid)
		return(0);
	return(&(m_commands[pid[0]]));	/* return pointer to object */
}

int BasicDate::Function(int fieldid,kGUIBasicVarObj *result,CallParms *p)
{
	switch(fieldid)
	{
	case DATE_SETNOW:
		m_date.SetToday();
	break;
	case DATE_SETDATE:
		m_date.Set(p->GetParm(0)->GetStringObj()->GetString());
	break;
	case DATE_SETDAY:
		m_date.SetDay(p->GetParm(0)->GetInt());
	break;
	case DATE_SETMONTH:
		m_date.SetMonth(p->GetParm(0)->GetInt());
	break;
	case DATE_SETYEAR:
		m_date.SetYear(p->GetParm(0)->GetInt());
	break;
	case DATE_SETHOUR:
		m_date.SetHour(p->GetParm(0)->GetInt());
	break;
	case DATE_SETMINUTE:
		m_date.SetMinute(p->GetParm(0)->GetInt());
	break;
	case DATE_SETSECOND:
		m_date.SetSecond(p->GetParm(0)->GetInt());
	break;
	case DATE_ADDDAYS:
		m_date.AddDays(p->GetParm(0)->GetInt());
	break;
	case DATE_GETDAY:
		result->Set(m_date.GetDay());
	break;
	case DATE_GETMONTH:
		result->Set(m_date.GetMonth());
	break;
	case DATE_GETYEAR:
		result->Set(m_date.GetYear());
	break;
	case DATE_GETHOUR:
		result->Set(m_date.GetHour());
	break;
	case DATE_GETMINUTE:
		result->Set(m_date.GetMinute());
	break;
	case DATE_GETSECOND:
		result->Set(m_date.GetSecond());
	break;
	case DATE_GETDAYOFWEEK:
		result->Set(m_date.GetDayofWeek());
	break;
	case DATE_GETDIFFSECONDS:
	{
		BasicDate *date2;

		/* todo, check to make sure it is correct pointer type */
		date2=static_cast<BasicDate *>(p->GetParm(0)->GetDefPtr());
		result->Set(m_date.GetDiffSeconds(&date2->m_date));
	}
	break;
	case DATE_GETDIFFMINUTES:
	{
		BasicDate *date2;

		/* todo, check to make sure it is correct pointer type */
		date2=static_cast<BasicDate *>(p->GetParm(0)->GetDefPtr());
		result->Set(m_date.GetDiffMinutes(&date2->m_date));
	}
	break;
	case DATE_GETDIFFHOURS:
	{
		BasicDate *date2;

		/* todo, check to make sure it is correct pointer type */
		date2=static_cast<BasicDate *>(p->GetParm(0)->GetDefPtr());
		result->Set(m_date.GetDiffHours(&date2->m_date));
	}
	break;
	case DATE_GETDIFFDAYS:
	{
		BasicDate *date2;

		/* todo, check to make sure it is correct pointer type */
		date2=static_cast<BasicDate *>(p->GetParm(0)->GetDefPtr());
		result->Set(m_date.GetDiffDays(&date2->m_date));
	}
	break;
	case DATE_COPY:
	{
		BasicDate *date2;

		/* todo, check to make sure it is correct pointer type */
		date2=static_cast<BasicDate *>(p->GetParm(0)->GetDefPtr());
		m_date.Copy(&date2->m_date);
	}
	break;
	case DATE_GETSHORTDATE:
	{
		kGUIString date;

		m_date.ShortDate(&date);
		result->Set(date.GetString());
	}
	break;
	case DATE_GETLONGDATE:
	{
		kGUIString date;

		m_date.LongDate(&date);
		result->Set(date.GetString());
	}
	break;
	default:
		assert(false,"Unknown command!");
	break;
	}
	return(ERROR_OK);
}

/*************************************************************************************/

void kGUIBasic::AddBuiltInClasses(void)
{
	kGUIBasicVarObj *vobj;

	vobj=new kGUIBasicVarObj();
	vobj->SetType(VARTYPE_SYSCLASSDEF);
	vobj->SetSystemClassAllocator(&(BasicDate::Alloc));
	AddAppObject("Date",vobj);
}

/*************************************************************************************/

static char basicinstructions[]={
	"<xTABSTART>kBasic<xTABEND>"
	"<xGROUPSTART>"
	"kBasic is a variant of Basic that is similar to Visual Basic and has some expression functions borrowed from C++.<BR>"
	"All commands and variable names are not case sensative.<BR>"
	"All variables must be defined before being used.<BR>"
	"All variables default to uninitialized and are not explicitly set to zero or null.<BR>"
	"<xGROUPEND>"
	"<BR><BR>"
	"<xTABSTART>Data&nbsp;Types<xTABEND>"
	"<xGROUPSTART>"
	"<table border=1 cellpadding=4 cellspacing=0>"
	"<tr><xCOLSTART>Type<xCOLEND><xCOLSTART>Description<xCOLEND><xCOLSTART>Example<xCOLEND></tr>"
	"<tr><td>Boolean</td><td>True or False</td><td>Dim VarName as Boolean</td></tr>"
	"<tr><td>Integer</td><td>32 Bit Signed Integer Value</td><td>Dim VarName as Integer</td></tr>"
	"<tr><td>Double</td><td>Floating Point Value</td><td>Dim VarName as Double</td></tr>"
	"<tr><td>String</td><td>String</td><td>Dim VarName as String</td></tr>"
	"<tr><td>Variant</td><td>Can automatically change between Boolean, Integer, Double and String</td><td>Dim VarName as Variant</td></tr>"
	"<tr><td>Date</td><td>Date and Time Class</td><td>Dim VarName as Date</td></tr>"
	"<tr><td>Enum XXX [as Type]<BR>VarName [=Value]<BR>End Enum</td><td>User Enumerated Constants. These are only to be defined in the global scope outside of a Subroutine or Function.<BR>If Values are not supplied then increasing values starting with zero are used.</td><td>Enum Cars as Integer<BR> Porsche<BR> Ferrari<BR> Lamborghini<BR>End Enum<BR>print Porsche, Ferrari, Lamborghini<BR><B>0 1 2</B></td></tr>"
	"<tr><td>Type XXX<BR>End Type</td><td>Define User Types (Structure)</td><td>Type LogData<BR>id as Integer<BR>LogDate as String<BR>Name as String<BR>End Type</td></tr>"
	"<tr><td>Arrays</td><td>All types can be in multi dimensional arrays</td><td>Dim VarName[10] as String<BR>Dim VarName[10][256] as Integer<BR>Dim VarName[] as Double</td></tr>"
	"</table>"
	"<xGROUPEND>"
	"<BR><BR>"
	"<xTABSTART>Subroutines&nbsp;and&nbsp;Functions<xTABEND>"
	"<xGROUPSTART>"
	"<table border=0 cellpadding=4 cellspacing=0>"
	"<tr><td>"
	"Subroutines and Functions are the the same except for the fact that functions return a value.<BR>"
	"Both Subroutines and Functions can take multiple parameters. Each parameter can have it's own type and can be passed by value or by reference.<BR>"
	"User Types (Structures) and Arrays as always passed by Reference.<BR>"
	"Functions can return the result value in two different ways. One way is to reference the function name as a variable and assign it the return value, you can also use the return command with the return value as a parameter.<BR>"
	"<BR><B>Syntax:</B><BR>"
	"[Public] Sub[([{ByVal|ByRef}] varname As type [,...])]<BR>"
	"[Public] Function[([{ByVal|ByRef}] varname As type [,...])] As returntype<BR><BR>"
	"<B>A Few Examples:</B><BR><BR>"
	"Public Sub PrintSum(count as Integer)<BR>"
	"Dim i as Integer<BR>"
	"Dim thesum as Integer<BR>"
	"thesum=0<BR>"
	"For i=1 to count<BR>"
	"thesum+=i<BR>"
	"Next i<BR>"
	"Print &quot;The Sum from 1 to &quot; &amp; count &amp; &quot; is &quot; &amp; thesum<BR>"
	"End Sub<BR><BR>"
	"Public Function ReturnSum(count as Integer) as Integer<BR>"
	"Dim i as Integer<BR>"
	"ReturnSum=0<BR>"
	"For i=1 to count<BR>"
	"ReturnSum+=i<BR>"
	"Next i<BR>"
	"End Function<BR><BR>"
	"Public Sub AppendSpaces(ByRef s as String,ByVal count as Integer)<BR>"
	"Dim i as Integer<BR>"
	"For i=1 to count<BR>"
	"s+=&quot; &quot;<BR>"
	"Next i<BR>"
	"End Sub<BR><BR>"
	"</td></tr>"
	"</table>"
	"<xGROUPEND>"
	"<BR><BR>"
	"<xTABSTART>Commands<xTABEND>"
	"<xGROUPSTART>"
	"<table border=1 cellpadding=4 cellspacing=0>"
	"<tr><xCOLSTART>Command<xCOLEND><xCOLSTART>Description<xCOLEND><xCOLSTART>Example<xCOLEND></tr>"
	"<tr><td>If<BR>ElseIf or Else If<BR>Else<BR>Endif or End&nbsp;If</td><td><B>Single line If:</B><BR>If expr statments<BR><B>Multi line If:</B><BR>If expr Then<BR> statments<BR>[ElseIf expr Then]<BR> statments<BR>[Else]<BR> statments<BR>Endif</td><td>If a==5 b=21<BR>If a==5 goto finished<BR><BR>If a==5 then<BR> Print &quot;Hello&quot;<BR>Elseif a==6 Then<BR> Print &quot;World&quot;<BR>ElseIf a==7 then<BR> Print &quot;Today&quot;<BR>Else<BR> Print &quot;Unknown&quot;<BR>Endif</td></tr>"
	"<tr><td>Do<BR>Loop</td><td><B>The Do/Loop commands can have expressions at both the Do and Loop lines. Use the Exit Loop Command to skip to the next statment following the Loop.</B><BR>Do [{While|Until} expr]<BR>statments<BR>Loop [{While|Until} expr]</td><td>Do While a&lt;99<BR> Print a<BR> ++a<BR>Loop<BR><BR>Do<BR> Print a<BR> ++a<BR>Loop Until=100<BR></td></tr>"
	"<tr><td>While<BR>Wend</td><td>While expr<BR>statments<BR>wend</td><td>While a&lt;100<BR> Print ++a<BR>Wend</td></tr>"
	"<tr><td>For<BR>Next</td><td><B>The Loop variable needs to be an Integer or Double.<BR>If no Step is defined then the variable increments by 1 per loop.</B><BR>For var = startexpr to endexpr [Step incrementexpr]<BR> statments<BR>Next [var]</td><td>For a = 1 to 100<BR>Print a<BR>Next<BR><BR>For b=1.0 to 5.5 Step 0.5<BR> Print b<BR>Next b</td></tr>"
	"<tr><td>Select<BR>Case</td><td><B>The testexpression needs to be one of the basic types: Integer, Double or String</B><BR>Select testexpression<BR>Case expressionlist<BR> statments<BR>[Case expressionlist]<BR> [statments]<BR>[Case Else]<BR> [statments]<BR>End Select<BR><B>The expressionlist can be one of three types.<BR></B>expr1 To expr2 <B>Range of values<BR></B>Is {=,&lt;=,&gt;=,!=,&lt;^gt;}expression <B>Comparision using comparator<BR></B>expr <B>Equals value</B></td><td>Select inputvar<BR>Case &quot;y&quot;,&quot;Y&quot;<BR> Print &quot;Yes&quot;<BR>Case &quot;n&quot;,&quot;N&quot;<BR> Print &quot;No&quot;<BR>Case Else<BR> Print &quot;Unknown&quot;<BR>End Select</td></tr>"
	"<tr><td>Goto</td><td><B>Labels in subroutines and functions are defined by having a labelname followed by a Colon at the beginning of a line.<BR>You can only use the Goto command to go to a label that is within the current Subroutine or Function.</B><BR>Goto label<BR><BR></td><td>goto skipall<BR>Print &quot;Hello World&quot;<BR>skipall: Print &quot;Hello Again&quot;</td></tr>"
	"<tr><td>Call or GoSub</td><td><B>The Call or Gosub Command is optional and will automatically be done if a Subroutine or Function name in encountered.<BR>If the Function or Subroutine doesn't take any parameters then the brackets are optional.</B><BR>Call funcname [(parameters)]</td><td>Call&nbsp;MySort(arrayname,numentries,direction)<BR>MySort(arrayname,numentries,direction)<BR>Call DoThings()<BR>DoThings()<BR>DoThings</td></tr>"
	"<tr><td>Return</td><td><B>The return command exits the current Subroutine or Function. When returning from a Function a return value can be supplied after the return command.</B><BR>Return [expr]<BR></td><td>Public Function MyFunc(InValue as Integer) as Double<BR>If (InValue==0) Then<BR> MyFunc=999.0<BR> Return<BR>EndIf<BR>Return(3.14)<BR>End Function</td></tr>"

	"<tr><td>Print</td><td><B>Print expressions to the console or if specified, a file.</B><BR>Expressions seperated by a comma have a tab character inserted between the output, expressions seperated by semi-colons have no space between their output. If the last expression is not followed by a semi-colon then a carraige return is output last.</B><BR>Print [#handle [,]] expr [{,;}expr....][;]</td><td>Print #1<BR>Print #1,chr(13);<BR>Print &quot;Hello World&quot;<BR>Print a,b,c;<BR>Print cos(PI)</td></tr>"
	"<tr><td>Input</td><td><B>If the Line prefix is used then input only reads one string variable and assigns input until the next end of line or end of file.<BR>If the Line prefix is not present then variables are input using comma delimiters or end of line or end of file.<BR>Note: There is also an Input Function, see it's syntax in the Function table below.</B><BR>[Line] Input #handle,var1[,var2...]</td><td>Line Input #1, a<BR>Input #1,astring,anumber2,anumber3</td></tr>"
	"<tr><td>InputBox</td><td><B>If the user presses the cancel button then a trappable user abort error is generated. See the On Error commands for more information on trapping user errors.</B><BR>InputBox prompt,variable</td><td>Input &quot;Input A Number&quot;,num<BR>InputBox prompt$,username$</td></tr>"
	"<tr><td>Open</td><td><B>If the file cannot be opened then a trappable file error is generated. See the On Error commands for more information on trapping file errors.</B><BR>Open filename For {INPUT | OUTPUT | READ | WRITE | BINARY | ACCESS | APPEND} as #handle</td><td>Open &quot;output.txt&quot; FOR INPUT as #1<BR>Open &quot;input.txt&quot; For OUTPUT as #1<BR>Open name$ For BINARY READ as #2<BR>Open fn$ for BINARY WRITE APPEND as #2</td></tr>"
	"<tr><td>Close</td><td>Close #handle</td><td>Close #1<BR>Close #curhandle</td></tr>"
	"<tr><td>Set or Let</td><td><B>The Set or Let Command is optional and will automatically be done if a variable followed by an assignment operator is encountered, of a variable with a modifier (++ or --).</B><BR>Set variable {=,+=,-=} expr<BR>Let variable {=,+=,-=} expr<BR>variable {=,+=,-=} expr</td><td>Set a=5<BR>Let a$=&quot;Hello&quot;<BR>a=99<BR>circumference+=123<BR>++counter<BR>--countdown</td></tr>"
	"<tr><td>Dim</td><td><B>Arrays can be multi-dimensional and square brackets are used around each array entry size. If no expression is inside the square brackets then the array data is unallocated and would need to be allocated using the ReDim command below. Variables Dimed in the global scope are globals and can be referenced by all Functions and Subroutines. Variables Dimed inside a Function or Subroutine are local and only accessable within that object unless they are passed by reference to another Function or Subroutine.</B><BR>Dim varname[arrayindices] as vartype</td><td>Dim name as String<BR>Dim name$ as String<BR>Dim table[100] as Integer<BR>Dim counter as Integer<BR>Dim coords[32][10][2] as Double<BR>Dim days[] as Boolean<BR>Dim LogInfo[] as LogRecord</td></tr>"
	"<tr><td>ReDim</td><td><B>The ReDim command is for changing the array size or type ( or both ) of an existing variable. If the type is the same and the number of dimensions are the same then the Preserve modifier can be used to save and existing values.</B><BR>ReDim [Preserve] varname[indices] as type</td><td>Dim list[][] As Integer<BR>ReDim list[10][10] As Integer<BR>Dim names[100] As String<BR>ReDim Preserve names[200] As String</td></tr>"
	"<tr><td>Exit</td><td>The Exit command is used to exit from a control loop or a Function or Subroutine</B><BR> Exit {For|Do|While|Func|Sub}</td><td>For a=1 to 10<BR>If a==5 Exit For<BR>Next</td></tr>"
	"<tr><td>On</td><td><B>The On Error command is used to redirect execution when file or input errors occur. If there is no current setting then program execution stops.</B><BR>On Error {Resume Next|Goto 0|Goto label}</td><td>On Error Goto fileopenerror<BR>Open fn$ for Read as #1<BR>On Error Goto 0<BR>...<BR>Exit Sub<BR>fileopenerror:<BR>MsgBox(&quot;Error Opening File '&quot; &amp; fn$ &&quot;'&quot;,MSGBOX_OK)</td></tr>"
	"<tr><td>With</td><td><B>The With command is used as a shortcut to reference user types (Structures) without having to type the variable name over and over.</B><BR>With [varname]</td><td>Type UserStruct<BR>ID as Integer<BR>Name as String<BR>Money as Double<BR>End Type<BR><BR>Dim CurrentUser as UserStruct<BR>With CurrentUser<BR>Print .ID,.Name,.Money<BR>.Money+=10<BR>End With</td></tr>"
	"<tr><td>End</td><td>End {With|Select|Sub|Function}</td><td>With Var<BR>....<BR>End With<BR><BR>Select Var<BR>...<BR>End Select<BR><BR>Public Sub MySub()<BR>...<BR>End Sub<BR><BR>Public Function MyFunc() as Integer<BR>...<BR>End Function</td></tr>"
	"</table>"
	"<xGROUPEND>"
	"<BR><BR>"
	"<xTABSTART>Expression&nbsp;Operators<xTABEND>"
	"<xGROUPSTART>"
	"<table border=1 cellpadding=4 cellspacing=0>"
	"<tr><xCOLSTART>Syntax<xCOLEND><xCOLSTART>Description<xCOLEND></tr>"
	"<tr><td>expr ? expr1 : expr2</td><td>if expr=true then result=expr1 else result=expr2</td></tr>"
	"<tr><td>&quot;||&quot; or &quot;or&quot; </td><td>logical or</td></tr>"		
	"<tr><td>&quot;&&&quot; or &quot;and&quot; </td><td>logical and</td></tr>"		
	"<tr><td>&quot;|&quot; </td><td>bitwise or</td></tr>"		
	"<tr><td>&quot;^&quot; or &quot;xor&quot;</td><td>bitwise exclusive or</td></tr>"		
	"<tr><td>&quot;&&quot; </td><td>bitwise and</td></tr>"
	"<tr><td>&quot;==&quot; or &quot;=&quot; or &quot;!=&quot; &quot;&lt;&gt;&quot;</td><td>test equals or test not equals</td></tr>"
	"<tr><td>&quot;&lt;&quot; or &quot;&lt;=&quot; or &quot;&gt;&quot; &quot;&gt;=&quot;</td><td>test less then, less than or equal, greater than, greater than or equal</td></tr>"
	"<tr><td>&quot;&lt;&lt;&quot; or &quot;&gt;&gt;&quot;</td><td>bitwise shift left, bitwise shift right</td></tr>"		
	"<tr><td>&quot;+&quot;, &quot;-&quot;, &quot;%&quot; or &quot;mod&quot;</td><td>add, subtract, mod</td></tr>"		
	"<tr><td>&quot;^^&quot; or &quot;**&quot;</td><td>exponent</td></tr>"
	"<tr><td>&quot;not&quot; or &quot;!&quot; or &quot;~&quot;</td><td>not</td></tr>"
	"<tr><td>&quot;-&quot; </td><td>negative</td></tr>"
	"<tr><td>&quot;(&quot;, &quot;)&quot;</td><td>parenthesis</td></tr>"
	"<tr><td>&quot;++var&quot;, &quot;--var&quot;</td><td>pre increment variable, pre decrement variable</td></tr>"
	"<tr><td>&quot;var++&quot;, &quot;var--&quot;</td><td>post increment variable, post decrement variable</td></tr>"
	"<tr><td>&quot;var+=expr&quot;, &quot;var-=expr&quot;</td><td>add expression to variable, subtract expression from variable</td></tr>"
	"</table>"
	"<xGROUPEND>"
	"<BR><BR>"
	"<xTABSTART>Functions<xTABEND>"
	"<xGROUPSTART>"
	"<table border=1 cellpadding=4 cellspacing=0>"
	"<tr><xCOLSTART>Syntax<xCOLEND><xCOLSTART>Example<xCOLEND></tr>"
	"<tr><td>rand</td><td><B>Return a random number</B><BR>rand() as integer</td></tr>"
	"<tr><td>len</td><td><B>Return length of string</B><BR>len(var as string) as integer</td></tr>"
	"<tr><td>asc</td><td><B>Return the ascii value of the first character in the string</B><BR>asc(var as string) as integer</td></tr>"
	"<tr><td>val</td><td><B>Return the Integer value of the string</B><BR>val(var as string) as integer</td></tr>"
	"<tr><td>dval</td><td><B>Return the Double value of the string</B><BR>dval(var as string) as double</td></tr>"
	"<tr><td>chr<BR>chr$</td><td><B>Return a single character ascii string from the Integer supplied</B><BR>chr(num as integer) as string</td></tr>"
	"<tr><td>str<BR>str$</td><td><B>Return a string representing the value supplied</B><BR>chr(num as {integer|double|boolean}) as string</td></tr>"
	"<tr><td>sin</td><td><B>Returns the sine of the radian supplied.</B><BR>sin(var as double) as double</td></tr>"
	"<tr><td>cos</td><td>Returns the cosine of the radian supplied.</B><BR>cos(var as double) as double</td></tr>"
	"<tr><td>tan</td><td>Returns the tangent of the radian supplied.</B><BR>tan(var as double) as double</td></tr>"
	"<tr><td>asin</td><td>Returns the radian value of the sine supplied.</B><BR>asin(var as double) as double</td></tr>"
	"<tr><td>acos</td><td>Returns the radian value of the cosine supplied.</B><BR>acos(var as double) as double</td></tr>"
	"<tr><td>atan</td><td>Returns the radian value of the tangent supplied.</B><BR>atan(var as double) as double</td></tr>"
	"<tr><td>eof</td><td><B>Return True if the end of file is encountered for the file handle</B><BR>eof(var as integer) as boolean</td></tr>"
	"<tr><td>trim</td><td><B>Returns the string with leading and trailing white space removed</B><BR>trim(var as string) as string</td></tr>"
	"<tr><td>fileexists</td><td><B>Returns True if the filename exists</B><BR>fileexists(filename as string) as boolean</td></tr>"
	"<tr><td>left<BR>left$</td><td><B>Returns the leftmost number of string characters</B><BR>left(var as string,numchars as integer) as string</td></tr>"
	"<tr><td>right<BR>right$</td><td><B>Returns the rightmost number of string characters</B><BR>right(var as string,numchars as integer) as string</td></tr>"
	"<tr><td>mid<BR>mid$</td><td><B>Returns the middle number of string characters starting at the start character and if the number of characters parameter is not present then it returns the rest of the string.</B><BR>mid(var as string,startchar as string[,numchars as integer]) as string</td></tr>"
	"<tr><td>instr</td><td><B>If string2 is found within string1 - InStr returns the position at which match is found. Cmpmode (0=case sensative,1=not case sensative).</B><BR>instr(startpos as integer,string1 as string,string2 as string [,cmpmode as integer=0]) as integer</td></tr>"
	"<tr><td>replace</td><td><B>Returns a string in which a specified substring has been replaced with another substring a specified number of times.</B><BR>replace(before as string,subfrom as string,subto as string [,startchar=1 [,maxchanges=unlimited [,cmpmode=0]]]) as string</td></tr>"
	"<tr><td>split</td><td><B>Returns a zero-based, one-dimensional array containing a specified number of substrings based on the split string. Use the ubound function to find out how many string were generated.</B><BR>split(original as string,splitstring as string [,cmpmode=0]) as string array</td></tr>"
	"<tr><td>iif</td><td><B>Returns expression1 if testextpression is True, or expression2 if it is False</B><BR>iif(testexpression as boolean,expression1 as variant,expression2 as variant) as variant</td></tr>"
	"<tr><td>input</td><td><B>Returns a string read from an open file and only reads in the number of characters specified. If chr$(0) characters are encountered when reading then the resultant string will contain those characters within it.</B><BR>input(numchars as integer,handle as integer) as string</td></tr>"
	"<tr><td>msgbox</td><td><B>This function displays a message box on the screen and waits for the user to press one of the buttons to continue. The button pressed value is returned. If the window close icon ( upper right ) is pushed then the value MSGBOX_ABORT is returned. For the buttons to display parameter just add the constant values together for all the buttons you wish to have displayed, valid button values are: MSGBOX_YES, MSGBOX_NO, MSGBOX_OK, MSGBOX_CANCEL or MSGBOX_DONE</B><BR>msgbox(prompt as string,buttons as integer) as integer</td></tr>"
	"<tr><td>filereq</td><td><B>This function is used to display a filerequestor to allow the user to select a directory and or filename. The mode parameter should be one of two constants FILEREQ_OPEN or FILEREQ_SAVE. Next is the default path to use and finally the file extension to use if applicable, use the format &quot;.xxx&quot; or use an empty string &quot;&quot; if all extensions are valid. The function returns the full filename with path as a string. If the user presses the cancel button then a trappable error is generated. See the On Error commands for more information on trapping user errors.</B><BR>filereq(mode as integer, path as string, extension as string) as string</td></tr>"
	"<tr><td>shellexec</td><td><B>This function calls an external program, it returns immediately (TODO: add param to signal for waiting for completion! )</B><BR>shellexec(program as string,parms as string,workingdir as string)</td></tr>"
	"<tr><td>ubound</td><td><B>Returns the highest available subscript for the indicated dimension of an array. If the dimension is not passed then the first dimension (0) is assumed.</B><BR>ubound(arrayname as variant [,dimension as integer=0]) as integer</td></tr>"
	"<tr><td>sortobjects</td><td><B>This function is used to sort a single dimensional array of user Types (structures) or Classes. The sortstring is a string containing comma delimited field names to sort by, ascending order is assumed, for descending order put a '-' prefix before the field name.</B><BR>sortobjects(arrayname as variant,numentries as integer,sortstring as string)</td></tr>"
	"</table>"
	"<xGROUPEND>"};

#if 0

Set objectvar = {[New] objectexpression | Nothing}
Static varname[([subscripts])] [As [New] type]
Math Functions
UCase(string), LCase(string), Len(string)
LTrim(string), RTrim(string), Trim(string)
Asc(string), Val(string), Oct(number), Hex(number
										   
Split(expression[, delimiter[, count[, compare]]])
Join(list[, delimiter])
Replace(expression, find, replacewith[, start[, count[, compare]]])
StrComp(string1, string2[, compare])
Filter(InputStrings, Value[, Include[, Compare]])
StrReverse(string1)
InStr([start, ]string1, string2[, compare])
InstrRev(string1, string2[, start[, compare]])

For Each element In group
[statements]
[Exit For]
[statements]
Next [element]


File Operation
Open pathname For mode [Access access] [lock] As [#]
filenumber [Len=reclength]
Input #filenumber, varlist
Print #filenumber, [outputlist]
Line Input #filenumber, varname
Write #filenumber, [outputlist]
Get [#]filenumber, [recnumber], varname
Put [#]filenumber, [recnumber], varname
Loc(filenumber)
Seek [#]filenumber, position
Eof(filenumber)
Lof(filenumber)
Reset
Close [filenumberlist]
Lock [#]filenumber[, recordrange]
Unlock [#]filenumber[, recordrange]
#endif

const char *kGUIBasic::GetInstructions(void)
{
	return basicinstructions;
}

/* user classes can register their own error messages */
int kGUIBasic::RegisterError(const char *message)
{
	kGUIString s;

	s.Sprintf("%d",m_errorindex);

	m_usererrors.SetDataLen((int)strlen(message)+1);
	m_usererrors.Add(s.GetString(),message);
	return(m_errorindex++);
}
