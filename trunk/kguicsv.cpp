/**********************************************************************************/
/* kGUI - kguicsv.cpp                                                             */
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

/*! @file kguicsv.cpp 
   @brief A simple class for loading data from CSV files. The seperator can also be
   changed to a different character if desired */

#include "kgui.h"
#include "kguicsv.h"

kGUICSV::kGUICSV()
{
	m_numrows=0;
	m_rows.Alloc(256);
	m_rows.SetGrow(true);
	m_rows.SetGrowSize(128);

	/* default to comma, allow override */
	m_split.SetString(",");
}

void kGUICSV::Init(unsigned int numrows,unsigned int numcols)
{
	kGUICSVRow *row;

	PurgeRows();

	m_maxcols=numcols;

	while(m_numrows<numrows)
	{
		row=new kGUICSVRow(numcols);
		m_rows.SetEntry(m_numrows++,row);
	}
}


kGUICSV::~kGUICSV()
{
	PurgeRows();
}

void kGUICSV::PurgeRows(void)
{
	unsigned int i;

	for(i=0;i<m_numrows;++i)
		delete m_rows.GetEntry(i);
	m_numrows=0;
}

/* load from current datahandle */
bool kGUICSV::Load(void)
{
	unsigned int maxcols;
	unsigned int numcols;
	unsigned int n;
	kGUIString line;
	kGUICSVRow *row;
	kGUIStringSplit sl;
	unsigned char header[4];
	int bomsize;

	if(Open()==false)
		return(false);		/* can't open file for read */

	/* read in the first 4 bytes and look for a byte order marker */
	Read((void *)header,(unsigned long)sizeof(header));
	printf("%02x %02x %02x %02x\n",header[0],header[1],header[2],header[3]);
	m_encoding=kGUIString::CheckBOM(sizeof(header),header,&bomsize);
	printf("encoding=%d\n",m_encoding);
	line.SetEncoding(m_encoding);

	Seek((unsigned long long)bomsize);

	sl.SetIgnoreEmpty(false);
	maxcols=0;
	do
	{
		ReadLine(&line);
		if(line.GetLen())
		{
			numcols=sl.Split(&line,m_split.GetString());
			row=new kGUICSVRow(numcols);
			for(n=0;n<numcols;++n)
				row->GetFieldPtr(n)->SetString(sl.GetWord(n));

			m_rows.SetEntry(m_numrows++,row);
			if(numcols>maxcols)
				maxcols=numcols;
		}
	}while(!Eof());
	Close();
	m_maxcols=maxcols;
	return(true);
}

/* save to current datahandle */
bool kGUICSV::Save(void)
{
	unsigned int r,c;
	kGUICSVRow *row;
	kGUIString cell;
	char q;

	if(OpenWrite("wb",32768)==false)
		return(false);

	/*! @todo what to do about encoding? */

	for(r=0;r<m_numrows;++r)
	{
		row=m_rows.GetEntry(r);
		for(c=0;c<m_maxcols;++c)
		{
			row->GetField(c,&cell);
			if(c)
				Write(",",1);

			if(cell.StrStr(",")<0)
				Write(cell.GetString(),cell.GetLen());
			else
			{
				/* must enclose in quotes */
				if(cell.StrStr("\"")<0)
					q='\"';
				else
					q='\'';
				Write(&q,1);
				Write(cell.GetString(),cell.GetLen());
				Write(&q,1);
			}
		}
		Write("\n",(int)strlen("\n"));
	}
	return(Close());
}
