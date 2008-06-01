/**********************************************************************************/
/* kGUI - kguidb.cpp                                                              */
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
/* These are classes for attaching to a mysql database. As well as the usual      */
/* queries, they can also load and save records and generate the sql strings to   */
/* send to the database based on only the fields that have changed.               */
/*                                                                                */
/* Todo: handle different database encodings UTF-8 etc.                           */
/* Todo: handle database disconnect errors more gracefully                        */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"
#include "kguidb.h"

kGUIDbCommand::kGUIDbCommand(kGUIDb *db,const char *command,...)
{
	kGUIString fcommand;
	const char *errormsg;
	va_list args;

	va_start(args, command);
	fcommand.AVSprintf(command,args);
	va_end(args);
	mysql_query(db->GetConn(),fcommand.GetString());

	if(db->GetTrace())
		db->GetTrace()->ASprintf("->%s\n",fcommand.GetString());

	errormsg=mysql_error(db->GetConn());
	if(errormsg[0])
		assert(false,errormsg);
}

kGUIDbQuery::kGUIDbQuery(kGUIDb *db,const char *query,...)
{
	int i;
	int nf;
	kGUIString fquery;
	const char *errormsg;
	va_list args;

	/* for performance analysis */
	db->IncNumQueries();

	va_start(args, query);
	fquery.AVSprintf(query,args);
	va_end(args);

	if(db->GetTrace())
		db->GetTrace()->ASprintf("->%s\n",fquery.GetString());

tryagain:;
	mysql_query(db->GetConn(),fquery.GetString());

	m_res_set = mysql_store_result(db->GetConn());

	errormsg=mysql_error(db->GetConn());
	if(errormsg[0])
	{
		if(!strcmp(errormsg,"Lost connection to MySQL server during query"))
		{
			/* try to re-connect! */
			db->ReConnect();
			goto tryagain;
		}
		else if(!strcmp(errormsg,"Server shutdown in progress") || !strcmp(errormsg,"MySQL server has gone away"))
		{
			/* hmmm, not sure what to do here! */
			return;
		}
		else
			assert(false,errormsg);
	}

	assert(m_res_set!=0,"Error, no result set!");

	/* use a hash table for field->index conversion */
	m_fi.Init(8,sizeof(int));
	nf=GetNumFields();
	for(i=0;i<nf;++i)
		m_fi.Add(GetFieldName(i),&i);
}

unsigned int kGUIDbQuery::GetIndex(const char *fieldname)
{
	unsigned int *ip;

	ip=(unsigned int *)m_fi.Find(fieldname);
	assert(ip!=0,"Field not found!");
	return(ip[0]);
}

kGUIDb::kGUIDb()
{
	m_conn=0;
	m_isconnected=false;
	m_lockschanged=false;
	m_lockedtables.Init(8,sizeof(int));
	m_numqueries=0;
	m_trace=0;
}

kGUIDb::~kGUIDb()
{
//	assert(m_numlocked==0,"Error locked tables left in list!" );
	UpdateLocks();
	if(m_conn)
		Close();
}

/* this is called if the connection is lost, it will try and re-connect automatically */
void kGUIDb::ReConnect(void)
{
	if(!mysql_real_connect(m_conn,m_servername.GetString(),m_username.GetString(),m_password.GetString(),m_dbname.GetString(),0,NULL,0))
		m_isconnected=false;
	else
		m_isconnected=true;
}

void kGUIDb::Connect(const char *servername,const char *dbname,const char *username,const char *password)
{
	m_servername.SetString(servername);
	m_dbname.SetString(dbname);
	m_username.SetString(username);
	m_password.SetString(password);

	m_conn = mysql_init(NULL);
	if(!mysql_real_connect(m_conn,servername,username,password,dbname,0,NULL,0))
	{
		m_isconnected=false;

//		char errorstring[256];

//		sprintf(errorstring,"Failed to connect to database: Error: %s\n",mysql_error(m_conn));
//		printf("%s",errorstring);
//		return;
	}
	else
	{
		MYSQL_RES *res;
		int z=0;
		const char **row;
		
		m_isconnected=true;

		/* add all tablenames to a hashtable with a locked count of 0 */
		res=mysql_list_tables(m_conn,0);	/* get list of tablenames */
		/* populate table */
		while ((row = (const char **)mysql_fetch_row(res)) != NULL) 
			m_lockedtables.Add(row[0],&z);	/* add tablename to locked list */
		mysql_free_result(res);
	}
}

/* increment lock table count for this table */

void kGUIDb::LockTable(const char *name)
{
	int *lockcount;

	lockcount=(int *)m_lockedtables.Find(name);
	if(!lockcount)
	{
		int z=0;

		/* table must have been created since connecting to DB */
		m_lockedtables.Add(name,&z);	/* add tablename to locked list */
		lockcount=(int *)m_lockedtables.Find(name);
	}

	assert(lockcount!=0,"table not found in database!");
	
	/* only set changed flag if count went from 0 to 1 */
	if(!lockcount[0])
		m_lockschanged=true;
	++lockcount[0];
}

void kGUIDb::UnLockTable(const char *name)
{
	int *lockcount;

	lockcount=(int *)m_lockedtables.Find(name);
	assert(lockcount!=0,"table not found in database!");
	
	/* only set changed flag if count went from 1 to 0 */
	--lockcount[0];
	if(!lockcount[0])
		m_lockschanged=true;
}

void kGUIDb::UpdateLocks(void)
{
	int i,numtables;
	HashEntry *he;
	int *lcount;
	kGUIString ls;
	int numlocked;
	kGUIDbCommand *c;

	if(m_lockschanged==false)
		return;
	
	c=new kGUIDbCommand(this,"UNLOCK TABLES;");
	delete c;

	numlocked=0;
	numtables=m_lockedtables.GetNum();
	he=m_lockedtables.GetFirst();
	ls.SetString("LOCK TABLES ");
	for(i=0;i<numtables;++i)
	{
		lcount=(int *)(he->m_data);	/* get data pointer */
		if(lcount[0])
		{
			if(numlocked)
				ls.Append(", ");
			ls.ASprintf("%s WRITE",he->m_string);
			++numlocked;
		}
		he=he->GetNext();
	}
	if(numlocked)
	{
		ls.Append(";");
		c=new kGUIDbCommand(this,ls.GetString());
		delete c;
	}
	m_lockschanged=false;
}

/* since this field might contain quotes, c/r, l/f etc, it needs to be encoded */
void kGUIDb::EncodeField(kGUIString *os,kGUIString *ns)
{
#if 1
	char temp[65536];

	assert(os->GetLen()<32767,"Buffer overflow is possible!");
	mysql_real_escape_string(m_conn,temp,os->GetString(),os->GetLen());
	ns->SetString(temp);
	ns->Trim(TRIM_SPACE);
#else
	ns->SetString(os->GetString());
	ns->Trim();
	ns->Replace("'","''");
#endif
}

/* read tables and return array of names */
int kGUIDb::ReadTables(kGUIString **names)
{
	MYSQL_RES *res_set; 
	int i,num;
	MYSQL_ROW row; 

	res_set=mysql_list_tables(GetConn(), 0); 
	num=(int)mysql_num_rows(res_set);
	if(!num)
		names[0]=0;
	else
	{
		names[0]=new kGUIString[num];
		/* populate table */
		for(i=0;i<num;++i)
		{
			row=mysql_fetch_row(res_set);
			names[0][i].SetString(row[0]);
		}
	}
	mysql_free_result(res_set);
	return(num);
}


void kGUIDb::Close(void)
{
	mysql_close(m_conn);
	m_conn=0;
}

kGUIDBRecord::kGUIDBRecord(kGUIDb *db)
{
	m_db=db;
	m_numfields=0;
	m_numentries=0;
	m_fieldnames.Init(12,4);
	m_numprikeyfields=0;
	m_prikey.Init(4,2);
	m_ce=0;
	m_locked=false;	
}

kGUIDBRecord::kGUIDBRecord()
{
	m_db=0;	/* must be defined later */
	m_numfields=0;
	m_numentries=0;
	m_fieldnames.Init(12,4);
	m_numprikeyfields=0;
	m_prikey.Init(4,2);
	m_ce=0;
	m_locked=false;	
}

void kGUIDBRecord::LockTable(void)
{
	if(m_locked==false)
	{
		m_db->LockTable(m_tablename.GetString());
		m_db->UpdateLocks();
		m_locked=true;
	}
}

void kGUIDBRecord::UnLockTable(void)
{
	if(m_locked==true)
	{
		m_db->UnLockTable(m_tablename.GetString());
		m_db->UpdateLocks();
		m_locked=false;
	}
}

int kGUIDBRecord::NextKey(void)
{
	int last;
	kGUIDbQuery *q;

	LockTable();
	m_db->UpdateLocks();

	q=new kGUIDbQuery(m_db,"SELECT %s from %s ORDER BY %s DESC LIMIT 0,1",m_keyname.GetString(),m_tablename.GetString(),m_keyname.GetString());
	if(q->GetNumRows())
		last=atoi(q->GetRow()[0])+1;
	else
		last=0;
	delete q;
	return(last);
}

void kGUIDBRecord::PurgeEntries(void)
{
	unsigned int i;
	kGUIDBRecordEntry *re;

	for(i=0;i<m_numentries;++i)
	{
		re=m_entries.GetEntry(i);
		delete re;
	}
	m_numentries=0;
}

kGUIDBRecord::~kGUIDBRecord()
{
	PurgeEntries();
//	if(m_fieldnames)
//	{
//		delete []m_fieldnames;
//		m_fieldnames=0;
//	}
	m_numfields=0;
}

void kGUIDBRecord::GetFieldNames(kGUIDbQuery *q)
{
	unsigned int i;
	unsigned int n=q->GetNumFields();

#if 1
	m_numprikeyfields=0;
	m_numfields=n;
	for(i=0;i<n;++i)
	{
		m_fieldnames.GetEntryPtr(i)->SetString(q->GetFieldName(i));
		if((q->GetFieldFlags(i)&PRI_KEY_FLAG)==PRI_KEY_FLAG)
			m_prikey.SetEntry(m_numprikeyfields++,i);
	}
#else
	if(m_numfields!=n)
	{
		if(m_fieldnames)
			delete []m_fieldnames;
		m_fieldnames=new kGUIString[n];
		m_numfields=n;
	}
	for(i=0;i<n;++i)
		m_fieldnames[i].SetString(q->GetFieldName(i));
#endif
}

void kGUIDBRecord::New(const char *tablename,const char *keyname)
{
	kGUIDbQuery *q;

	m_unique=true;
	m_tablename.SetString(tablename);
	m_keyname.SetString(keyname);

	/* do a query to just get all the field names */
	q=new kGUIDbQuery(m_db,"SELECT * from %s LIMIT 0,0",tablename);
	GetFieldNames(q);
	delete q;

	if(m_numentries)
		PurgeEntries();
	m_entries.Alloc(1);
	AddEntry();	/* 1 entry only */
	m_ce->m_new=true;
}

void kGUIDBRecord::NewGroup(const char *tablename,const char *keyname)
{
	kGUIDbQuery *q;

	m_unique=false;
	m_tablename.SetString(tablename);
	if(keyname)
		m_keyname.SetString(keyname);
	else
		m_keyname.Clear();

	/* do a query to just get all the field names */
	q=new kGUIDbQuery(m_db,"SELECT * from %s LIMIT 0,0",tablename);
	GetFieldNames(q);
	delete q;

	if(m_numentries)
		PurgeEntries();

	m_entries.SetGrow(true);
}

void kGUIDBRecord::AddEntry(void)
{
	kGUIDBRecordEntry *re;

	re=new kGUIDBRecordEntry(m_numfields);
	m_entries.SetEntry(m_numentries++,re);
	re->m_new=true;
	m_ce=re;	/* this is the current entry */
}

/* one and only 1 record should be found, any more/less is an error */
void kGUIDBRecord::Load(const char *tablename,kGUIDbQuery *q)
{
	assert(Loadz(tablename,q),"Error loading record!");
}

/* only 1 or zero is valid, more than 1 is an error */
bool kGUIDBRecord::Loadz(const char *tablename,kGUIDbQuery *q)
{
	m_unique=false;
	m_keyname.Clear();
	m_key.Clear();

	LoadGroup(tablename,q);
	assert(m_numentries<2,"Error: Too many matching records, use LoadGroup");
	return(m_numentries==1);
}

void kGUIDBRecord::Load(const char *tablename,kGUIDbQuery *q,const char **row)
{
	m_unique=false;
	m_keyname.Clear();
	m_key.Clear();
	m_tablename.SetString(tablename);
	LoadQ(q,row);
}

bool kGUIDBRecord::Load(const char *tablename,const char *keyname,const char *key)
{
	kGUIDbQuery *q;

	m_unique=true;
	q=new kGUIDbQuery(m_db,"SELECT * from %s WHERE %s='%s'",tablename,keyname,key);
	assert(q->GetNumRows()<2,"Error num matches>1");

	m_tablename.SetString(tablename);
	m_keyname.SetString(keyname);
	m_key.SetString(key);

	if(!q->GetNumRows())
	{
		delete q;
		if(m_numentries)
			PurgeEntries();
		m_numentries=0;
		m_ce=0;
		return(false);
	}
	LoadQ(q,q->GetRow());
	delete q;
	return(true);
}

void kGUIDBRecord::LoadQ(kGUIDbQuery *q,const char **row)
{
	unsigned int i;
	unsigned int n;
	kGUIDBRecordEntry *re;

	assert(row!=0,"No data!");
	n=q->GetNumFields();
	if(m_numentries)
		PurgeEntries();

	GetFieldNames(q);

	m_numentries=1;
	m_entries.Alloc(1);
	re=new kGUIDBRecordEntry(n);
	m_ce=re;
	m_entries.SetEntry(0,re);

	re->m_new=false;
	for(i=0;i<n;++i)
	{
		re->m_fieldvalues[i].SetString(row[i]);
		re->m_newfieldvalues[i].SetString(row[i]);
	}
}

void kGUIDBRecord::LoadGroup(const char *tablename,const char *keyname,const char *key)
{
	kGUIDbQuery *q;

	m_keyname.SetString(keyname);
	if(key)
		m_key.SetString(key);

	m_unique=false;
	if(strlen(keyname) && key)
		q=new kGUIDbQuery(m_db,"SELECT * from %s WHERE %s='%s'",tablename,keyname,key);
	else
		q=new kGUIDbQuery(m_db,"SELECT * from %s",tablename);
	LoadGroup(tablename,q);
	delete q;
}

bool kGUIDBRecord::CompareGroup(kGUIDbQuery *q)
{
	unsigned int i;
	unsigned int j;
	unsigned int ne;
	unsigned int nf;
	const char **row;
	kGUIDBRecordEntry *re;

	nf=q->GetNumFields();
	if(m_numfields!=nf)
		return(false);	/* different! */
	ne=q->GetNumRows();
	if(m_numentries!=ne)
		return(false);	/* different! */

	for(i=0;i<ne;++i)
	{
		re=m_entries.GetEntry(i);
		row=q->GetRow();

		for(j=0;j<nf;++j)
		{
			if(strcmp(re->m_fieldvalues[j].GetString(),row[j]))
			{
				q->SeekRow(0);
				return(false);
			}
		}
	}
	q->SeekRow(0);
	return(true);	/* same! */
}

void kGUIDBRecord::LoadGroup(const char *tablename,kGUIDbQuery *q)
{
	unsigned int i;
	unsigned int n;
	unsigned int j;
	const char **row;
	kGUIDBRecordEntry *re;

	m_tablename.SetString(tablename);
	m_unique=false;
	n=q->GetNumFields();

	GetFieldNames(q);
#if 0
	if(m_numfields!=n)
	{
		if(m_fieldnames)
			delete []m_fieldnames;
		m_fieldnames=new kGUIString[n];
		m_numfields=n;
	for(i=0;i<n;++i)
		m_fieldnames[i].SetString(q->GetFieldName(i));
	}
#endif

	if(m_numentries)
		PurgeEntries();

	m_numentries=q->GetNumRows();
	m_entries.Alloc(m_numentries);
	m_entries.SetGrow(true);
	m_entries.SetGrowSize(32);


	j=0;
	while((row = q->GetRow()) != NULL)
	{
		re=new kGUIDBRecordEntry(n);
		m_entries.SetEntry(j++,re);
		re->m_new=false;
		for(i=0;i<n;++i)
		{
			re->m_fieldvalues[i].SetString(row[i]);
			re->m_newfieldvalues[i].SetString(row[i]);
		}
	}
	if(m_numentries)
		SelectEntry(0);		/* default  to first entry is selected */
	else
		m_ce=0;				/* no current entry! */
}

void kGUIDBRecord::LoadTable(kGUITableObj *table)
{
	unsigned int i;
	kGUIDBRecordEntry *re;
	kGUITableRowObj *newrow;
	kGUIDBTableRowObj *newrowdb;

	table->DeleteChildren();
	for(i=0;i<m_numentries;++i)
	{
		re=m_entries.GetEntry(i);
		newrow=table->AddNewRow();
		newrowdb=static_cast<kGUIDBTableRowObj *>(newrow);
		m_ce=re;
		newrowdb->SetRE(re);
		newrowdb->LoadTableEntry(this);
	}
}

/* does this record have any pending changes to write? */
bool kGUIDBRecord::GetChanged(void)
{
	unsigned int e;
	unsigned int i;
	kGUIDBRecordEntry *re;

	for(e=0;e<m_numentries;++e)
	{
		re=m_entries.GetEntry(e);
		
		if(re->m_delete==true)
			return(true);
		if(re->m_new==true)
			return(true);
		for(i=0;i<m_numfields;++i)
		{
			if(strcmp(re->m_newfieldvalues[i].GetString(),re->m_fieldvalues[i].GetString()))
				return(true);		/* changed */
		}
	}
	return(false);
}


/* this is mainly for debugging, it generates a string */
/* that shows differences between the current record and */

bool kGUIDBRecord::GetDiff(kGUIString *diff)
{
	unsigned int i;
	bool changed;
	kGUIDBRecordEntry *re;

	diff->Clear();
	changed=false;
	re=m_ce;

	for(i=0;i<m_numfields;++i)
	{
		if(strcmp(re->m_newfieldvalues[i].GetString(),re->m_fieldvalues[i].GetString()))
		{
			if(changed==true)
				diff->Append(", ");
			diff->ASprintf("%s ('%s'<>'%s')",GetFieldName(i),re->m_newfieldvalues[i].GetString(),re->m_fieldvalues[i].GetString());
			changed=true;
		}
	}
	return(changed);
}


bool kGUIDBRecord::CompareTable(kGUITableObj *table)
{
	unsigned int i;
	unsigned int f;
	kGUIObj *gobj;
	kGUIDBTableRowObj *row;
	kGUIDBRecordEntry *re;
	kGUIDBRecordEntry tempentry(m_numfields);

	/* set the deleted flag on all entries at first */
	for(i=0;i<m_numentries;++i)
	{
		re=m_entries.GetEntry(i);
		re->m_changed=true;
	}

	/* now iterate through the table and update all */
	/* records and reset the delete flag on them */

	for(i=0;i<table->GetNumChildren(0);++i)
	{
		gobj=table->GetChild(0,i);
		row=static_cast<kGUIDBTableRowObj *>(gobj);
		if(!row->GetRE())
			return(true);	/* changed, this is a new record */

		/* copy fields incase not all fields are updated */
		for(f=0;f<m_numfields;++f)
			tempentry.m_newfieldvalues[f].SetString(m_ce->m_fieldvalues[f].GetString());

		m_ce=&tempentry;			/* set current row to this one */
		row->SaveTableEntry(this);	/* save fields from table to this temp one */

		/* compare values in tempentry to last one */
		m_ce=row->GetRE();
		for(f=0;f<m_numfields;++f)
		{
			if(strcmp(m_ce->m_newfieldvalues[f].GetString(),tempentry.m_newfieldvalues[f].GetString()))
				return(true);	/* different */
		}
		m_ce->m_changed=false;
	}

	/* have any entries not been referenced? */
	for(i=0;i<m_numentries;++i)
	{
		re=m_entries.GetEntry(i);
		if(re->m_changed==true)
			return(true);
	}

	return(false);	/* same */
}

void kGUIDBRecord::FlagAllEntriesForDelete(void)
{
	unsigned int i;
	kGUIDBRecordEntry *re;

	/* set the deleted flag on all entries at first */
	for(i=0;i<m_numentries;++i)
	{
		re=m_entries.GetEntry(i);
		re->m_delete=true;
	}
}

bool kGUIDBRecord::SaveTable(kGUITableObj *table)
{
	unsigned int i;
	kGUIObj *gobj;
	kGUIDBTableRowObj *row;
	kGUIDBRecordEntry *re;

	FlagAllEntriesForDelete();

	/* now iterate through the table and update all */
	/* records and reset the delete flag on them */

	for(i=0;i<table->GetNumChildren(0);++i)
	{
		gobj=table->GetChild(0,i);
		row=static_cast<kGUIDBTableRowObj *>(gobj);
		if(row->GetRE())
			m_ce=row->GetRE();	/* set current row to this one */
		else
		{
			/* allocate a new record entry to put this into */
			re=new kGUIDBRecordEntry(m_numfields);
			m_entries.SetEntry(m_numentries++,re);
			re->m_new=true;

			m_ce=re;
			row->SetRE(m_ce);
		}
		row->SaveTableEntry(this);
	}

	return(Save());
}

unsigned int kGUIDBRecord::GetIndex(const char *fieldname)
{
	unsigned int i;

	for(i=0;i<m_numfields;++i)
	{
		if(!stricmp(GetFieldName(i),fieldname))
			return(i);
	}
	assert(false,"Field not found error!");
	return(0);
}

const char *kGUIDBRecord::GetField(const char *fieldname)
{
	return m_ce->m_newfieldvalues[GetIndex(fieldname)].GetString();
}

void kGUIDBRecord::SetField(const char *fieldname,const char *value)
{
	int i=GetIndex(fieldname);

	m_ce->m_newfieldvalues[i].SetString(value);
	m_ce->m_newfieldvalues[i].Trim(TRIM_SPACE);	/* remove leading and trailing spaces */
	m_ce->m_delete=false;
}

void kGUIDBRecord::SetField(const char *fieldname,int val)
{
	int i=GetIndex(fieldname);

	m_ce->m_newfieldvalues[i].Sprintf("%d",val);
	m_ce->m_newfieldvalues[i].Trim(TRIM_SPACE);		/* remove leading and trailing spaces */
	m_ce->m_delete=false;
}

void kGUIDBRecord::SetField(const char *fieldname,const char *format,int val)
{
	int i=GetIndex(fieldname);

	m_ce->m_newfieldvalues[i].Sprintf(format,val);
	m_ce->m_newfieldvalues[i].Trim(TRIM_SPACE);		/* remove leading and trailing spaces */
	m_ce->m_delete=false;
}

void kGUIDBRecord::SetField(const char *fieldname,const char *format,double val)
{
	int i=GetIndex(fieldname);

	m_ce->m_newfieldvalues[i].Sprintf(format,val);
	m_ce->m_newfieldvalues[i].Trim(TRIM_SPACE);		/* remove leading and trailing spaces */
	m_ce->m_delete=false;
}

bool kGUIDBRecord::Save(void)
{
	unsigned int i;
	unsigned int e;
	int numchanged;
	bool changed;
	bool where;
	bool uselimit;
	kGUIString update;
	kGUIString field;
	kGUIDBRecordEntry *re;
	const char *errormsg;

	assert(m_tablename.GetLen()!=0,"Name not defined for table, call SetTableName(name)!");

	/* locate using old key */
	/* make sure all fields at the same as before changed */
	/* if any are diffent, then return changed by other user error code, unlock table */
	/* update fields that are different */
	/* unlock table */

	update.Alloc(8192);
	field.Alloc(1024);

	for(e=0;e<m_numentries;++e)
	{
		re=m_entries.GetEntry(e);
		
		if(re->m_delete==true)
		{
			uselimit=true;
			where=true;
			changed=true;
			update.SetString("DELETE FROM ");
			update.Append(m_tablename.GetString());
		}
		else if(re->m_new==true)
		{
			where=false;
			uselimit=false;
			update.SetString("INSERT ");
			update.Append(m_tablename.GetString());

			update.Append(" SET ");		
			changed=false;
			for(i=0;i<m_numfields;++i)
			{
				if(changed==true)
					update.Append(", ");
				update.Append(GetFieldName(i));		
				update.Append("='");

				m_db->EncodeField(&re->m_newfieldvalues[i],&field);
				update.Append(field.GetString());		
				update.Append("'");
				changed=true;
			}
		}
		else	/* update an existing record */
		{
			update.SetString("UPDATE ");
			update.Append(m_tablename.GetString());

			where=true;
			uselimit=true;
			update.Append(" SET ");		
			changed=false;
			for(i=0;i<m_numfields;++i)
			{
				if(strcmp(re->m_newfieldvalues[i].GetString(),re->m_fieldvalues[i].GetString()))
				{
					if(changed==true)
						update.Append(", ");
					update.Append(GetFieldName(i));		
					update.Append("='");

					m_db->EncodeField(&re->m_newfieldvalues[i],&field);
					update.Append(field.GetString());		
					update.Append("'");
					changed=true;
				}
			}
		}

		if(changed==true)
		{
			/* does this table have a unique primary key? */
			if(where==true)
			{
				update.Append(" WHERE ");
				if(m_numprikeyfields)
				{
					unsigned int f;

					for(i=0;i<m_numprikeyfields;++i)
					{
						f=m_prikey.GetEntry(i);
						if(i)
							update.Append("AND ");
						update.Append(GetFieldName(f));		
						update.Append("='");
						m_db->EncodeField(&re->m_fieldvalues[f],&field);
						update.Append(field.GetString());		
						update.Append("' ");
					}
				}
#if 0
				if(m_unique==true)
				{
					update.Append(m_keyname.GetString());
					update.Append("='");
					update.Append(m_key.GetString());
					update.Append("'");
				}
#endif
				else
				{
					for(i=0;i<m_numfields;++i)
					{
						if(i)
							update.Append("AND ");
						update.Append(GetFieldName(i));		
						update.Append("='");
						m_db->EncodeField(&re->m_fieldvalues[i],&field);
						update.Append(field.GetString());		
						update.Append("' ");
					}
				}
			}

			/* this is necessary as there could be multiple matches */
			if(uselimit==true)
				update.Append(" LIMIT 1");

			/* lock table */
			LockTable();
			m_db->UpdateLocks();

			mysql_query(m_db->GetConn(),update.GetString());

			if(m_db->GetTrace())
				m_db->GetTrace()->ASprintf("->%s\n",update.GetString());

			errormsg=mysql_error(m_db->GetConn());
			if(errormsg[0])
				assert(false,errormsg);

			numchanged=(int)mysql_affected_rows(m_db->GetConn());
			assert(numchanged==1,"Error,number of records changed should have been one!");

			/* fields were sucessfully written, copy new fields over previous ones */
			/* hmmm, deleted entries? */
			for(i=0;i<m_numfields;++i)
			{
				re->m_new=false;
				if(strcmp(re->m_newfieldvalues[i].GetString(),re->m_fieldvalues[i].GetString()))
					re->m_fieldvalues[i].SetString(re->m_newfieldvalues[i].GetString());
			}
			/* remove lock from this table */
			UnLockTable();
			m_db->UpdateLocks();
		}
	}

	/* delete all entries that are flagged for deletion */
	i=0;
	for(e=0;e<m_numentries;++e)
	{
		re=m_entries.GetEntry(e-i);
		if(re->m_delete)
		{
			delete re;
			m_entries.DeleteEntry(e-i);
			++i;	/* ajust for scrolling entries */
		}
	}
	m_numentries-=i;	/* update number of entries */

	return(true);	/* ok! */
}

/*******************************************/
/* list all of the tables in this database */
/*******************************************/

kGUIDbTablesListRowObj::kGUIDbTablesListRowObj(kGUIDb *dbobj)
{
	SetRowHeight(20);
	m_objptrs[0]=&m_text;
	m_dbobj=dbobj;

	m_text.SetEventHandler(this,& CALLBACKNAME(NameEvent));
}

/* save tablename */
void kGUIDbTablesListRowObj::NameEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_ENTER:
		/* save before name */
	break;
	case EVENT_AFTERUPDATE:
		/* after name */
	break;
	}
}

kGUIDbTablesListWindowObj::kGUIDbTablesListWindowObj(kGUIDb *dbobj)
{
	MYSQL *conn;
	MYSQL_RES *res_set; 
	MYSQL_ROW row; 

	m_dbobj=dbobj;
	SetEventHandler(this,CALLBACKNAME(WindowEvent));

	SetTitle("Database tables");
	SetPos(50,50);
	SetSize(600,500);

	m_def.SetPos(25,10);
	m_def.SetSize(150,20);
	m_def.SetString("Design View");
	m_def.SetEventHandler(this,CALLBACKNAME(Design));
	AddObject(&m_def);

	m_data.SetPos(200,10);
	m_data.SetSize(150,20);
	m_data.SetString("Data View");
	m_data.SetEventHandler(this,CALLBACKNAME(Data));
	AddObject(&m_data);

	m_newtable.SetPos(400,10);
	m_newtable.SetSize(150,20);
	m_newtable.SetString("New Table");
	m_newtable.SetEventHandler(this,CALLBACKNAME(NewTable));
	AddObject(&m_newtable);

	m_table.SetPos(0,50);
	m_table.SetSize(400,400);
	m_table.SetNumCols(1);
	m_table.SetColTitle(0,"Name");
	m_table.SetColWidth(0,200);
	m_table.SetEventHandler(this,&CALLBACKNAME(WindowEvent));
	m_table.SetAllowAddNewRow(true);

	AddObject(&m_table);

	conn=dbobj->GetConn();
	res_set=mysql_list_tables(conn, 0); 
	/* populate table */
	while ((row = mysql_fetch_row(res_set)) != NULL) 
	{ 
		kGUIDbTablesListRowObj *robj=new kGUIDbTablesListRowObj(dbobj);
		robj->SetName(row[0]);
		m_table.AddRow(robj);
	} 

	mysql_free_result(res_set);
	/* automatically calculate */
	//m_table.CalculateColWidths();

	kGUI::AddWindow(this);
}

void kGUIDbTablesListWindowObj::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		m_table.DeleteChildren();
		delete this;
	break;
	case EVENT_ADDROW:
	{
		kGUIDbTablesListRowObj *row;

		row=new kGUIDbTablesListRowObj(m_dbobj);
		m_table.AddRow(row);
	}
	break;
	}
}

void kGUIDbTablesListWindowObj::Data(kGUIEvent *event)
{
	kGUIDbTableDataWindowObj *w;
	kGUIDbTablesListRowObj *row;

	if(event->GetEvent()==EVENT_PRESSED)
	{
		if(m_table.GetNumChildren())
		{
			row=static_cast<kGUIDbTablesListRowObj *>(m_table.GetCursorRowObj());
			if(row)
				w=new kGUIDbTableDataWindowObj(m_dbobj,row->GetName());
		}
	}
}

void kGUIDbTablesListWindowObj::Design(kGUIEvent *event)
{
	kGUIDbTableDefWindowObj *w;
	kGUIDbTablesListRowObj *row;

	if(event->GetEvent()==EVENT_PRESSED)
	{
		if(m_table.GetNumChildren())
		{
			row=static_cast<kGUIDbTablesListRowObj *>(m_table.GetCursorRowObj());
			if(row)
				w=new kGUIDbTableDefWindowObj(m_dbobj,row->GetName());
		}
	}
}


/*****************************************/
/* Show the data currently in this table */
/*****************************************/

kGUIDbTableDataRowObj::kGUIDbTableDataRowObj(int num,const int *coltypes,const char **row)
{
	int i;

	SetRowHeight(20);
	m_num=num;
	m_objptrs=new kGUIObj *[num];

	for(i=0;i<num;++i)
	{
		if(coltypes[i]==0)
		{
			kGUIInputBoxObj *tobj;

			tobj=new kGUIInputBoxObj;
			tobj->SetString(row[i]);
			m_objptrs[i]=tobj;
		}
		else
		{
			kGUITickBoxObj *tbobj;

			tbobj=new kGUITickBoxObj;
			tbobj->SetSelected(row[i][0]=='1');
			m_objptrs[i]=tbobj;
		}
	}
}

kGUIDbTableDataRowObj::~kGUIDbTableDataRowObj()
{
	int i;

	for(i=0;i<m_num;++i)
		delete m_objptrs[i];

	delete []m_objptrs;
}

kGUIDbTableDataWindowObj::kGUIDbTableDataWindowObj(kGUIDb *dbobj,const char *tablename)
{
	kGUIDbQuery *q;
	const char **row; 
	unsigned int i;
	unsigned int numfields;
	int *coltypes;

	SetEventHandler(this,CALLBACKNAME(WindowEvent));

	SetTitle(tablename);
	SetPos(50,50);
	SetSize(800,500);
	m_table.SetPos(0,0);
	m_table.SetSize(700,400);

	kGUI::SetMouseCursor(MOUSECURSOR_BUSY);
	q=new kGUIDbQuery(dbobj,"SELECT * FROM %s",tablename);
	numfields=q->GetNumFields();
	m_table.SetNumCols(numfields);
	for(i=0;i<numfields;++i)
	{
		m_table.SetColTitle(i,q->GetFieldName(i));
		m_table.SetColWidth(i,200);
	}

	/* build a table of field types for display */
	coltypes=new int[numfields];
	for(i=0;i<numfields;++i)
	{
		if(q->GetFieldType(i)==MYSQL_TYPE_TINY && q->GetFieldLength(i)==1)
			coltypes[i]=1;
		else
			coltypes[i]=0;
	}
	/* populate table */
	while ((row = q->GetRow()) != NULL) 
	{ 
		kGUIDbTableDataRowObj *robj=new kGUIDbTableDataRowObj(numfields,coltypes,row);
		m_table.AddRow(robj);
	} 
	/* automatically calculate */
	m_table.CalculateColWidths();

	delete q;
	delete []coltypes;

	AddObject(&m_table);
	kGUI::AddWindow(this);
	kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
}

void kGUIDbTableDataWindowObj::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		m_table.DeleteChildren();
		delete this;
	break;
	}
}

/*****************************************/
/* Show the fields currently in this table */
/*****************************************/

typedef struct
{
	int num;
	const char *name;
	int size;
}SQLTYPES_DEF;

SQLTYPES_DEF sqltypes[]={
	{MYSQL_TYPE_DECIMAL,"decimal",0},
	{MYSQL_TYPE_TINY,"tiny",0},
	{MYSQL_TYPE_SHORT,"short",0},
	{MYSQL_TYPE_LONG,"long",0},
	{MYSQL_TYPE_FLOAT,"float",0},
	{MYSQL_TYPE_DOUBLE,"double",0},
	{MYSQL_TYPE_NULL,"null",0},
	{MYSQL_TYPE_TIMESTAMP,"timestamp",0},
	{MYSQL_TYPE_LONGLONG,"longlong",0},
	{MYSQL_TYPE_INT24,"int24",0},
	{MYSQL_TYPE_DATE,"date",0},
	{MYSQL_TYPE_TIME,"time",0},
	{MYSQL_TYPE_DATETIME,"datetime",0},
	{MYSQL_TYPE_YEAR,"year",0},
	{MYSQL_TYPE_NEWDATE,"newdate",0},
	{MYSQL_TYPE_VARCHAR,"varchar",1},
	{MYSQL_TYPE_BIT,"bit",0},
	{MYSQL_TYPE_NEWDECIMAL,"newdecimal",0},
	{MYSQL_TYPE_ENUM,"enum",0},
	{MYSQL_TYPE_SET,"set",0},
	{MYSQL_TYPE_TINY_BLOB,"tinyblob",0},
	{MYSQL_TYPE_MEDIUM_BLOB,"mediumblob",0},
	{MYSQL_TYPE_LONG_BLOB,"longblob",0},
	{MYSQL_TYPE_BLOB,"blob",0},
	{MYSQL_TYPE_VAR_STRING,"varstring",1},
	{MYSQL_TYPE_STRING,"string",0},
	{MYSQL_TYPE_GEOMETRY,"geometry",0}};

/* assert if type not in list */
int	kGUIDbTableDefRowObj::GetSqlTypeIndex(int type)
{
	unsigned int i;

	for(i=0;i<sizeof(sqltypes)/sizeof(SQLTYPES_DEF);++i)
	{
		if(type==sqltypes[i].num)
			return(i);
	}
	passert(false,"Unknown SQL type! %d\n",type);
	return(-1);	/* never gets here, but stop compiler from complaining... */
}

kGUIDbTableDefRowObj::kGUIDbTableDefRowObj(kGUIDbTableDefWindowObj *parent,const char *name,int type,int length,int maxlength,bool prikey,bool indexed)
{
	unsigned int i;

	m_parent=parent;
	m_name.SetString(name);
	m_name.SetEventHandler(this,CALLBACKNAME(ChangeNameorType));
	m_oldname.SetString(name);

	m_type.SetNumEntries(sizeof(sqltypes)/sizeof(SQLTYPES_DEF));
	for(i=0;i<sizeof(sqltypes)/sizeof(SQLTYPES_DEF);++i)
		m_type.SetEntry(i,sqltypes[i].name,sqltypes[i].num);
	
	m_type.SetSelection(type);
	m_oldtype=m_type.GetSelection();
	m_type.SetEventHandler(this,CALLBACKNAME(ChangeNameorType));

	m_length.SetInt(length);
	m_maxlength.SetInt(maxlength);

	m_prikey.SetSelected(prikey);
	m_prikey.SetEventHandler(this,CALLBACKNAME(ChangePriKey));

	m_indexed.SetSelected(indexed);
	m_indexed.SetEventHandler(this,CALLBACKNAME(ChangeIndexed));

	m_objptrs[0]=&m_name;
	m_objptrs[1]=&m_type;
	m_objptrs[2]=&m_length;
	m_objptrs[3]=&m_maxlength;
	m_objptrs[4]=&m_prikey;
	m_objptrs[5]=&m_indexed;
	SetRowHeight(20);
}

void kGUIDbTableDefRowObj::ChangeNameorType(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		kGUIDbCommand *q;
		int typeindex;

		typeindex=GetSqlTypeIndex(m_type.GetSelection());

		if(sqltypes[typeindex].size)
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s CHANGE %s %s %s(%d);",m_parent->GetTableName(),m_oldname.GetString(),m_name.GetString(),sqltypes[typeindex].name,m_length.GetInt());
		else
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s CHANGE %s %s %s;",m_parent->GetTableName(),m_oldname.GetString(),m_name.GetString(),sqltypes[typeindex].name);
		delete q;
		m_oldname.SetString(m_name.GetString());
		m_oldtype=m_type.GetSelection();
	}
}

void kGUIDbTableDefRowObj::ChangePriKey(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		kGUIDbCommand *q;
		if(m_prikey.GetSelected()==true)
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s ADD PRIMARY KEY(%s);",m_parent->GetTableName(),m_name.GetString());
		else
		{
			// drops all primary keys, not sure how to just drop one????
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s DROP PRIMARY KEY;",m_parent->GetTableName(),m_name.GetString());
		}
		delete q;
	}
}

void kGUIDbTableDefRowObj::ChangeIndexed(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		kGUIDbCommand *q;
		if(m_indexed.GetSelected()==true)
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s ADD INDEX(%s);",m_parent->GetTableName(),m_name.GetString());
		else
			q=new kGUIDbCommand(m_parent->GetDB(),"ALTER TABLE %s DROP INDEX %s;",m_parent->GetTableName(),m_name.GetString());
		delete q;
	}
}

kGUIDbTableDefWindowObj::kGUIDbTableDefWindowObj(kGUIDb *dbobj,const char *tablename)
{
	kGUIDbQuery *q;
	unsigned int i;
	unsigned int numfields;
	int *maxlengths;
	const char **row; 
	const int tdcwidths[]={200,200,200,200,100,100};
	const char *tdcnames[]={"Field Name","Field Type","Field Length","Current Max Length","PriKey","Indexed"};

	SetEventHandler(this,CALLBACKNAME(WindowEvent));

	m_db=dbobj;
	m_name.SetString(tablename);

	SetTitle(tablename);
	SetPos(50,50);
	SetSize(800,500);
	m_table.SetPos(0,0);
	m_table.SetSize(700,400);

	kGUI::SetMouseCursor(MOUSECURSOR_BUSY);
	/* scan whole table because we show the max used length for each field */
	q=new kGUIDbQuery(dbobj,"SELECT * FROM %s",tablename);

	m_table.SetNumCols(6);
	for(i=0;i<6;++i)
	{
		m_table.SetColTitle(i,tdcnames[i]);
		m_table.SetColWidth(i,tdcwidths[i]);
	}

	numfields=q->GetNumFields();
	maxlengths=new int[numfields];
	for(i=0;i<numfields;++i)
		maxlengths[i]=0;

	/* calculate current maximum used field lengths */
	while ((row = q->GetRow()) != NULL) 
	{ 
		for(i=0;i<numfields;++i)
		{
			if(row[i])	/* null for undefined text type */
			{
				int l=(int)strlen(row[i]);
				if(l>maxlengths[i])
					maxlengths[i]=l;
			}
		}
	} 

	for(i=0;i<numfields;++i)
	{
		unsigned int flags=q->GetFieldFlags(i);
		kGUIDbTableDefRowObj *robj=new kGUIDbTableDefRowObj(this,q->GetFieldName(i),q->GetFieldType(i),q->GetFieldLength(i),maxlengths[i],(flags&PRI_KEY_FLAG)==PRI_KEY_FLAG,(flags&MULTIPLE_KEY_FLAG)==MULTIPLE_KEY_FLAG);
		m_table.AddRow(robj);
	}
	/* automatically calculate */
	m_table.CalculateColWidths();

	delete q;
	delete maxlengths;

	m_table.SetAllowAddNewRow(true);
	m_table.SetEventHandler(this,CALLBACKNAME(WindowEvent));

	AddObject(&m_table);
	kGUI::AddWindow(this);
	kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
}

void kGUIDbTableDefWindowObj::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		m_table.DeleteChildren();
		delete this;
	break;
	case EVENT_ADDROW:
	{
		unsigned int i;
		kGUIDbTableDefRowObj *row;
		bool exists;
		int index;
		kGUIString name;

		index=0;
		do
		{
			exists=false;
			if(!index)
				name.SetString("field");
			else
				name.Sprintf("field%d",index);
			++index;
			
			/* check to see if this field name already exists */
			for(i=0;i<m_table.GetNumChildren();++i)
			{
				row=static_cast<kGUIDbTableDefRowObj *>(m_table.GetChild(i));
				if(!stricmp(row->GetName(),name.GetString()))
				{
					exists=true;
					break;
				}
			}
		}while(exists);

		//add field
		kGUIDbCommand *q;

		//ALTER TABLE t1 CHANGE oldname newname newtype;
		q=new kGUIDbCommand(GetDB(),"ALTER TABLE %s ADD COLUMN %s %s;",GetTableName(),name.GetString(),sqltypes[row->GetSqlTypeIndex(MYSQL_TYPE_LONG)].name);
		delete q;

		row=new kGUIDbTableDefRowObj(this,name.GetString(),row->GetSqlTypeIndex(MYSQL_TYPE_LONG),4,4,false,false);
		m_table.AddRow(row);
	}
	break;
	}
}

void kGUIDbTablesListWindowObj::NewTable(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		kGUIInputBoxReq *gn;

		gn=new kGUIInputBoxReq(this,CALLBACKNAME(NewTable2),"New Table Name?");
	}
}

void kGUIDbTablesListWindowObj::NewTable2(kGUIString *s,int closebutton)
{
	if(s->GetLen())
	{
		kGUIDbCommand *c;
		MYSQL_RES *res_set; 
		MYSQL_ROW row; 

		c=new kGUIDbCommand(m_dbobj,"CREATE TABLE %s (id INT);",s->GetString());

		m_table.DeleteChildren();
		/* repopulate the table */
		res_set=mysql_list_tables(m_dbobj->GetConn(), 0); 
		/* populate table */
		while ((row = mysql_fetch_row(res_set)) != NULL) 
		{ 
			kGUIDbTablesListRowObj *robj=new kGUIDbTablesListRowObj(m_dbobj);
			robj->SetName(row[0]);
			m_table.AddRow(robj);
		} 

		mysql_free_result(res_set);
		/* automatically calculate */
		m_table.CalculateColWidths();
	}
}

/* this is in the database code as it generates today's date as a string */
/* in the database's native date format */

void kGUIDb::DateToString(kGUIDate *date,kGUIString *s)
{
	s->Sprintf("%04d-%02d-%02d",date->GetYear(),date->GetMonth(),date->GetDay());
}

void kGUIDb::StringToDate(const char *s,kGUIDate *date)
{
	int d,m,y;

	y=atoi(s);
	m=atoi(s+5);
	d=atoi(s+8);
	date->Set(d,m,y);
}

/* convert true/false from database to boolean true/false */
bool atob(const char *p)
{
	if(!stricmp(p,"true"))
		return(true);
	if(!stricmp(p,"false"))
		return(false);

	if(p[0]=='1')
		return(true);
	return(false);
}

/* convert boolean true/false to string "0","1" */

const char *btoa(bool b)
{
	static const char *btoastrings[2]={"0","1"};

	if(b==true)
		return btoastrings[1];
	else
		return btoastrings[0];
}
