#ifndef __KGUIDB__
#define __KGUIDB__

#if defined(MINGW)
#include "winsock.h"
#elif defined(LINUX)
#elif defined(MACINTOSH)
#else
#include "winsock.h"
#endif
#include <mysql.h>
#include <errmsg.h>
#include "hash.h"

bool atob(const char *p);
const char *btoa(bool b);

class kGUIDb
{
public:
	kGUIDb();
	~kGUIDb();
	void Connect(const char *servername,const char *dbname,const char *username=0,const char *password=0);
	void ReConnect(void);
	bool IsConnected(void) {return m_isconnected;}
	void Close(void);
	void DateToString(kGUIDate *date,kGUIString *s);	/* get date in a string in the standard database format */
	void StringToDate(const char *s,kGUIDate *date);	/* get date in a string in the standard database format */

	/* get todays date in a string in the standard database format */
	MYSQL *GetConn(void) { return m_conn;}
	void EncodeField(kGUIString *os,kGUIString *ns);
	void LockTable(const char *name);
	void UnLockTable(const char *name);
	void UpdateLocks(void);

	/* read array of table names */
	int ReadTables(kGUIString **names);

	/* performance */
	int GetNumQueries(void) {return m_numqueries;}
	void IncNumQueries(void) {++m_numqueries;}

	/* trade writes/reads to/from database */
	/* point to users string and append, null = trace not active */
	void SetTrace(kGUIString *trace) {m_trace=trace;}
	kGUIString *GetTrace(void) {return m_trace;}
private:
	MYSQL *m_conn;

	/* save these incase the connection is dropped and if so try and re-connect again */
	kGUIString m_servername;
	kGUIString m_dbname;
	kGUIString m_username;
	kGUIString m_password;
	kGUIString *m_trace;

	bool m_isconnected;
	bool m_lockschanged;
	Hash m_lockedtables;
	int m_numqueries;
};

class kGUIDbCommand
{
public:
	kGUIDbCommand(kGUIDb *db,const char *query,...);
};

class kGUIDbQuery
{
public:
	kGUIDbQuery(kGUIDb *db,const char *query,...);
	~kGUIDbQuery() {mysql_free_result(m_res_set);}
	int GetNumRows(void) {return (int)mysql_num_rows(m_res_set);} 
	int GetNumFields(void) {return (int)mysql_num_fields(m_res_set);}
	const char *GetFieldName(int i) {return mysql_fetch_fields(m_res_set)[i].name;}
	int GetFieldType(int i) {return mysql_fetch_fields(m_res_set)[i].type;}
	int GetFieldLength(int i) {return mysql_fetch_fields(m_res_set)[i].length;}
	unsigned int GetFieldFlags(int i) {return mysql_fetch_fields(m_res_set)[i].flags;}

	unsigned int GetIndex(const char *fieldname);

	/* these return fields based on the current record */
	const char *GetField(const char *fieldname) {return(m_crow[GetIndex(fieldname)]);}
	int GetFieldInt(const char *fieldname) {return atoi(GetField(fieldname));}
	double GetFieldDouble(const char *fieldname) {return atof(GetField(fieldname));}
	bool GetFieldBool(const char *fieldname) {return atob(GetField(fieldname));}

	const char **GetRow(void) {m_crow=(const char **)mysql_fetch_row(m_res_set);return m_crow;}
	void SeekRow(int r) {mysql_data_seek(m_res_set,r);}
private:
	MYSQL_RES *m_res_set; 
	const char **m_crow;	/* current row */
	Hash m_fi;				/* field names to indexes hash table */

};

/* used for updating a record (or records) on a table */

class kGUIDBRecordEntry
{
public:
	kGUIDBRecordEntry(int nf) {m_new=false;m_delete=false;m_fieldvalues=new kGUIString[nf];m_newfieldvalues=new kGUIString[nf];}
	~kGUIDBRecordEntry() {delete []m_fieldvalues;delete []m_newfieldvalues;}
	bool m_new;
	bool m_changed;	/* only used in the CompareTable function */
	bool m_delete;
	kGUIString *m_fieldvalues;
	kGUIString *m_newfieldvalues;
};

class kGUIDBRecord
{
public:
	kGUIDBRecord();
	kGUIDBRecord(kGUIDb *db);
	~kGUIDBRecord();
	void SetDB(kGUIDb *db) {m_db=db;}
	/* Load is used for a single record */
	void New(const char *tablename,const char *keyname);
	void Load(const char *tablename,kGUIDbQuery *q);
	bool Loadz(const char *tablename,kGUIDbQuery *q);

	void Load(const char *tablename,kGUIDbQuery *q,const char **row);
	bool Load(const char *tablename,const char *keyname,const char *key);
	bool Load(const char *tablename,const char *keyname,int key) {kGUIString ckey;ckey.Sprintf("%d",key);return Load(tablename,keyname,ckey.GetString());}
	/* LoadGroup is used for loading a group of records */ 
	void NewGroup(const char *tablename,const char *keyname=0);
	void LoadGroup(const char *tablename,const char *keyname,const char *key=0,const char *sort=0);
	void LoadGroup(const char *tablename,kGUIDbQuery *q);
	bool CompareGroup(kGUIDbQuery *q);
	void PurgeEntries(void);
	
	void LockTable(void);
	void UnLockTable(void);

	/* only valid on a numerical key, gets last index returns it +1 */
	/* locks table, unlocks table in save function */
	int NextKey(void);

	void AddEntry(void);
	void SelectEntry(int num) {m_ce=m_entries.GetEntry(num);}
	void FlagEntryForDelete(void) {m_ce->m_delete=true;}
	/* copy items from recordentries into a record */
	void LoadTable(kGUITableObj *table);
	bool CompareTable(kGUITableObj *table);	/* is it the same or different? */
	void FlagAllEntriesForDelete(void);
	bool SaveTable(kGUITableObj *table);	/* true=ok, false=error */

	unsigned int GetIndex(const char *fieldname);
	const char *GetFieldName(int i) {return m_fieldnames.GetEntryPtr(i)->GetString();}
	const char *GetField(int i) {return m_ce->m_newfieldvalues[i].GetString();}
	const char *GetField(const char *fieldname);
	int GetFieldInt(const char *fieldname) {return atoi(GetField(fieldname));}
	double GetFieldDouble(const char *fieldname) {return atof(GetField(fieldname));}
	bool GetFieldBool(const char *fieldname) {return atob(GetField(fieldname));}

	void SetField(const char *fieldname,const char *value);
	void SetField(const char *fieldname,int value);
	void SetField(const char *fieldname,const char *format,int val);
	void SetField(const char *fieldname,const char *format,double val);
	bool Save(void);
	unsigned int GetNumFields(void) {return m_numfields;}
	unsigned int GetNumRecords(void) {return m_numentries;}
	
	bool GetChanged(void);		/* any pending changes not written? */
	bool GetDiff(kGUIString *diff);
	void SetTableName(const char *name) {m_tablename.SetString(name);}
	const char *GetTableName(void) {return m_tablename.GetString();}
private:
	void LoadQ(kGUIDbQuery *q,const char **row);
	void GetFieldNames(kGUIDbQuery *q);
	unsigned int m_numfields;
	unsigned int m_numentries;
	kGUIDBRecordEntry *m_ce;

	kGUIDb *m_db;
	kGUIString m_tablename;

	unsigned int m_numprikeyfields;
	Array<unsigned int>m_prikey;	/* array of fieldname indexes */

	/* todo: depreciate these */
	bool m_unique;
	kGUIString m_keyname;
	kGUIString m_key;

	ClassArray<kGUIString>m_fieldnames;
	Array<kGUIDBRecordEntry *>m_entries;
	bool m_locked;
};

/* used to link between database and table entries */
class kGUIDBTableRowObj : public kGUITableRowObj
{
public:
	kGUIDBTableRowObj() {m_re=0;}
	virtual void LoadTableEntry(kGUIDBRecord *re)=0;
	virtual void SaveTableEntry(kGUIDBRecord *re)=0;
	kGUIDBRecordEntry *GetRE(void) {return m_re;}
	void SetRE(kGUIDBRecordEntry *re) {m_re=re;}
private:
	kGUIDBRecordEntry *m_re;	/* set by load */
};

/*******************************************/
/* list all of the tables in this database */
/*******************************************/

class kGUIDbTablesListRowObj : public kGUITableRowObj
{
public:
	kGUIDbTablesListRowObj(kGUIDb *dbobj);
	~kGUIDbTablesListRowObj() {}

	inline int GetNumObjects(void) {return 1;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
	inline void SetName(const char *t) {m_text.SetString(t);}
	inline const char *GetName(void) {return m_text.GetString();}
private:
	CALLBACKGLUEPTR(kGUIDbTablesListRowObj,NameEvent,kGUIEvent)
	void NameEvent(kGUIEvent *event);
	kGUIObj *m_objptrs[1];
	kGUIInputBoxObj m_text;
	kGUIDb *m_dbobj;
};

class kGUIDbTablesListWindowObj: public kGUIWindowObj
{
public:
	kGUIDbTablesListWindowObj(kGUIDb *dbobj);
	~kGUIDbTablesListWindowObj() {}
private:
	CALLBACKGLUEPTR(kGUIDbTablesListWindowObj,Data,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDbTablesListWindowObj,Design,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDbTablesListWindowObj,NewTable,kGUIEvent)
	CALLBACKGLUEPTRVAL(kGUIDbTablesListWindowObj,NewTable2,kGUIString,int)
	CALLBACKGLUEPTR(kGUIDbTablesListWindowObj,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	void Data(kGUIEvent *event);
	void Design(kGUIEvent *event);
	void NewTable(kGUIEvent *event);
	void NewTable2(kGUIString *name,int closebutton);
	kGUITableObj m_table;
	kGUIButtonObj m_def;
	kGUIButtonObj m_data;
	kGUIButtonObj m_newtable;
	kGUIDb *m_dbobj;
};

/*****************************************/
/* Show the data currently in this table */
/*****************************************/

class kGUIDbTableDataRowObj : public kGUITableRowObj
{
public:
	kGUIDbTableDataRowObj(int num,const int *coltypes,const char **row);
	~kGUIDbTableDataRowObj();

	inline int GetNumObjects(void) {return m_num;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
private:
	kGUIObj **m_objptrs;
	int m_num;
};

class kGUIDbTableDataWindowObj: public kGUIWindowObj
{
public:
	kGUIDbTableDataWindowObj(kGUIDb *dbobj,const char *tablename);
	~kGUIDbTableDataWindowObj() {}
private:
	CALLBACKGLUEPTR(kGUIDbTableDataWindowObj,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	kGUITableObj m_table;
};

class kGUIDbTableDefWindowObj: public kGUIWindowObj
{
public:
	kGUIDbTableDefWindowObj(kGUIDb *dbobj,const char *tablename);
	~kGUIDbTableDefWindowObj() {}
	const char *GetTableName(void) {return m_name.GetString();}
	kGUIDb *GetDB(void) {return m_db;}
private:
	CALLBACKGLUEPTR(kGUIDbTableDefWindowObj,WindowEvent,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	kGUIText m_name;
	kGUIDb *m_db;
	kGUITableObj m_table;
};

/*****************************************/
/* Show the data currently in this table */
/*****************************************/

class kGUIDbTableDefRowObj : public kGUITableRowObj
{
public:
	kGUIDbTableDefRowObj(kGUIDbTableDefWindowObj *parent,const char *name,int type,int length,int maxlength,bool pkey,bool indexed);
	~kGUIDbTableDefRowObj() {};

	inline int GetNumObjects(void) {return 6;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
	static int GetSqlTypeIndex(int mytype);
	const char *GetName(void) {return m_name.GetString();}
private:
	CALLBACKGLUEPTR(kGUIDbTableDefRowObj,ChangeNameorType,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDbTableDefRowObj,ChangePriKey,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDbTableDefRowObj,ChangeIndexed,kGUIEvent)
	void ChangeNameorType(kGUIEvent *event);
	void ChangePriKey(kGUIEvent *event);
	void ChangeIndexed(kGUIEvent *event);
	kGUIObj *m_objptrs[6];
	kGUIInputBoxObj m_name;			/* name of field */
	kGUIInputBoxObj m_oldname;		/* name of field */
	kGUIComboBoxObj m_type;			/* type of field */
	int m_oldtype;
	kGUIInputBoxObj m_length;		/* length field */
	kGUIInputBoxObj m_maxlength;	/* maximum used length field */
	kGUITickBoxObj m_prikey;		/* is primary key */
	kGUITickBoxObj m_indexed;		/* is indexed */

	kGUIDbTableDefWindowObj *m_parent;	/* pointer to parent object */
};

#endif
