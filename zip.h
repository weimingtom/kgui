#ifndef __ZIP__
#define __ZIP__

/**********************************************************************************/
/* kGUI - big.h                                                                   */
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

#include "datahandle.h"

class ZipFileEntry : public ContainerEntry
{
public:
	kGUIString *GetName(void) {return &m_name;}
	kGUIString m_name;
};

class ZipFile : public ContainerFile
{
public:
	ZipFile();
	~ZipFile();

	void OpenSave(const char *fn,bool replace);
	bool AddFile(const char *fn,DataHandle *af);
	void CloseSave(void);

	void SetFilename(const char *fn) {m_zipfn.SetString(fn);LoadDirectory();}
	void LoadDirectory(void);
	unsigned int GetNumEntries(void) {return m_numentries;}
	ContainerEntry *GetEntry(unsigned int n) {return (m_entries.GetEntryPtr(n));}
	ContainerEntry *LocateEntry(const char *f);
	void CopyEntry(ContainerEntry *cfe,DataHandle *dest);

	bool Extract(DataHandle *dest,const char *fn);
private:
	void *m_zf;
	kGUIString m_zipfn;

	unsigned int m_numentries;
	ClassArray<ZipFileEntry>m_entries;
	Hash m_hash;
};
#endif

