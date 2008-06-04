#ifndef __KGUICSV__
#define __KGUICSV__

/* a simple class for loading and saving CSV files */

class kGUICSVRow
{
public:
	kGUICSVRow(unsigned int numfields) {m_numfields=numfields;m_fields.Init(numfields,0);}
	unsigned int GetNumFields(void) {return m_numfields;}
	void GetField(unsigned int col,kGUIString *s) {if(col<m_numfields)s->SetString(m_fields.GetEntryPtr(col));else s->Clear();}
	void SetField(unsigned int col,kGUIString *s) {m_fields.GetEntryPtr(col)->SetString(s);}
	kGUIString *GetFieldPtr(unsigned int col) {return m_fields.GetEntryPtr(col);}
private:
	unsigned int m_numfields;
	ClassArray<kGUIString>m_fields;
};

class kGUICSV: public DataHandle
{
public:
	kGUICSV();
	~kGUICSV();
	void PurgeRows(void);
	bool Load(void);
	bool Save(void);
	void Init(unsigned int numrows,unsigned int numcols);
	unsigned int GetNumCols(void) {return m_maxcols;}
	unsigned int GetNumRows(void) {return m_numrows;}
	void GetField(unsigned int row,unsigned int col,kGUIString *s) {m_rows.GetEntry(row)->GetField(col,s);}
	void SetField(unsigned int row,unsigned int col,kGUIString *s) {m_rows.GetEntry(row)->SetField(col,s);}
	kGUIString *GetFieldPtr(unsigned int row,unsigned int col) {return m_rows.GetEntry(row)->GetFieldPtr(col);}
	void SetSplit(const char *s) {m_split.SetString(s);}
private:
	kGUIString m_split;
	unsigned int m_maxcols;
	unsigned int m_numrows;
	Array<kGUICSVRow *>m_rows;	
};

#endif
