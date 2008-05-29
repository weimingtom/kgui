#ifndef __KGUICSV__
#define __KGUICSV__

/* a simple class for loading and saving CSV files */

class kGUICSVRow
{
public:
	unsigned int Load(kGUIString *split,kGUIString *line);
	unsigned int GetNumFields(void) {return m_fields.GetNumWords();}
	void GetField(int col,kGUIString *s) {s->SetString(m_fields.GetWord(col));}
	const char *GetFieldPtr(int col) {return m_fields.GetWord(col)->GetString();}
private:
	kGUIStringSplit m_fields;
};

class kGUICSV: public DataHandle
{
public:
	kGUICSV();
	~kGUICSV();
	void PurgeRows(void);
	bool Load(void);
	bool Save(void);
	unsigned int GetNumCols(void) {return m_maxcols;}
	unsigned int GetNumRows(void) {return m_numrows;}
	void GetField(unsigned int row,unsigned int col,kGUIString *s) {m_rows.GetEntry(row)->GetField(col,s);}
	const char *GetFieldPtr(unsigned int row,unsigned int col) {return m_rows.GetEntry(row)->GetFieldPtr(col);}
	void SetSplit(const char *s) {m_split.SetString(s);}
private:
	kGUIString m_split;
	unsigned int m_maxcols;
	unsigned int m_numrows;
	Array<kGUICSVRow *>m_rows;	
};

#endif
