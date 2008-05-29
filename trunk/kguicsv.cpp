/**********************************************************************************/
/* kGUI - kguicsv.cpp                                                             */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://www.scale18.com/cgi-bin/page/kgui.html	                              */
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

#include "kgui.h"
#include "kguicsv.h"

unsigned int kGUICSVRow::Load(kGUIString *split,kGUIString *line)
{
	m_fields.SetIgnoreEmpty(false);
	return(m_fields.Split(line,split->GetString()));
}

kGUICSV::kGUICSV()
{
	m_numrows=0;
	m_rows.Alloc(256);
	m_rows.SetGrow(true);
	m_rows.SetGrowSize(128);

	/* default to comma, allow override */
	m_split.SetString(",");
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
	kGUIString line;
	kGUICSVRow *row;

	PurgeRows();

	if(Open()==false)
		return(false);		/* can't open file for read */

	maxcols=0;
	do
	{
		ReadLine(&line);
		if(line.GetLen())
		{
			row=new kGUICSVRow();
			numcols=row->Load(&m_split,&line);
			m_rows.SetEntry(m_numrows++,row);
			if(numcols>maxcols)
				maxcols=numcols;
		}
	}while(!Eof());
	Close();
	m_maxcols=maxcols;
	return(true);
}
