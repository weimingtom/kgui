/*********************************************************************************/
/* kGUI - kguilist.cpp                                                          */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://www.scale18.com/cgi-bin/page/kgui.html                                 */
/*                                                                               */
/*    kGUI is free software; you can redistribute it and/or modify               */
/*    it under the terms of the GNU Lesser General Public License as published by*/
/*    the Free Software Foundation; version 2.                                   */
/*                                                                               */
/*    kGUI is distributed in the hope that it will be useful,                    */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/*    GNU General Public License for more details.                               */
/*                                                                               */
/*    http://www.gnu.org/licenses/lgpl.txt                                       */
/*                                                                               */
/*    You should have received a copy of the GNU General Public License          */
/*    along with GPSTurbo; if not, write to the Free Software                    */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

//todo, change to show as many as it can fit on the screen based on the row height
//etc and not on an arbitrary limit.

#define MAXSHOWCOMBO 16

enum
{
COMBOEDIT_POPLIST,
COMBOEDIT_USERTYPE,
COMBOEDIT_NONE
};

kGUIListboxObj::kGUIListboxObj()
{
	m_editmode=COMBOEDIT_NONE;
	m_hint=0;
	m_colormode=false;
	m_locked=false;
	m_numentries=0;
	m_type=COMBOTYPE_NUM;	/* default type */
	m_selection=0;
	m_tableentries=0;

	/* table settings */
	NoColHeaders();
	NoRowHeaders();
	NoColScrollbar();
	LockCol();				/* don't allow cursor left/right */
	SetListMode();

//	SetSelectedCallBack(this,CALLBACKNAME(SelectionDone));

//	m_poptable->SetZone(popx,popy,popw,poph);
//	m_poptable->SizeDirty();
//	Dirty();
}

kGUIText *kGUIListboxObj::GetEntryTextPtr(unsigned int index)
{
	return m_tableentries[index]->GetText();
}

void kGUIListboxObj::SetColorMode(unsigned int width)
{
	assert(m_tableentries==0,"Need to set this before setting the number of entries!");
	m_colorcolwidth=width;
	m_colormode=true;
}

void kGUIListboxObj::SetColorBox(unsigned int index,kGUIColor c)
{
	assert(index<m_numentries,"kGUIListboxObj: index too large");
	m_tableentries[index]->SetBox(m_colorcolwidth,c);
}

kGUIListboxObj::~kGUIListboxObj()
{
	unsigned int i;

	if(m_hint)
		delete m_hint;

	if(m_tableentries)
	{
		for(i=0;i<m_numentries;++i)
			delete m_tableentries[i];
		delete []m_tableentries;
	}
}

void kGUIListboxObj::SetNumEntries(unsigned int n)
{
	unsigned int i;

	/* todo, calc col width ? */
	if(m_colormode==true)
	{
		SetNumCols(2);
		SetColWidth(0,200-m_colorcolwidth);
		SetColWidth(1,m_colorcolwidth);
	}
	else
	{
		SetNumCols(1);
		SetColWidth(0,200);
	}

	if(m_tableentries)
	{
		DeleteChildren(true);
		delete []m_tableentries;
	}

	m_numentries=n;
	m_selection=0;
	m_tableentries=new kGUIComboTableRowObj *[n];

	for(i=0;i<n;++i)
	{
		m_tableentries[i]=new kGUIComboTableRowObj(m_colormode==true?2:1);
		m_tableentries[i]->SetRowHeight(m_tableentries[i]->GetHeight()+4);
		AddRow(m_tableentries[i]);
		SetEntryEnable(i,true);
	}
	Dirty();
}


void kGUIListboxObj::SetEntry(unsigned int index,const char *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIListboxObj: index too large");

	/* default to global, allow override */
	m_tableentries[index]->SetFontInfo(this);

	m_tableentries[index]->SetString(entryname);
	m_tableentries[index]->SetRowHeight(m_tableentries[index]->GetHeight()+4);
	m_tableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIListboxObj::SetEntry(unsigned int index,const char *entryname,const char *entryval)
{
	assert(index<m_numentries,"kGUIListboxObj: index too large");

	/* default to global, allow override */
	m_tableentries[index]->SetFontInfo(this);

	m_tableentries[index]->SetString(entryname);
	m_tableentries[index]->SetRowHeight(m_tableentries[index]->GetHeight()+4);
	m_tableentries[index]->SetValue(index);
	m_tableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIListboxObj::SetEntry(unsigned int index,kGUIString *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIListboxObj: index too large");

	/* default to global, allow override */
	m_tableentries[index]->SetFontInfo(this);

	m_tableentries[index]->SetString(entryname);
	m_tableentries[index]->SetRowHeight(m_tableentries[index]->GetHeight()+4);
	m_tableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIListboxObj::SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval)
{
	assert(index<m_numentries,"kGUIListboxObj: index too large");

	/* default to global, allow override */
	m_tableentries[index]->SetFontInfo(this);

	m_tableentries[index]->SetString(entryname);
	m_tableentries[index]->SetRowHeight(m_tableentries[index]->GetHeight()+4);
	m_tableentries[index]->SetValue(index);
	m_tableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIListboxObj::RenameEntry(const char *oldname,const char *newname)
{
	unsigned int i;
	for(i=0;i<m_numentries;++i)
	{
		if(!strcmp(m_tableentries[i]->GetString(),oldname))
		{
			m_tableentries[i]->SetString(newname);
			if(m_selection==i)
				Dirty();
			return;
		}
	}
	assert(false,"name not found error!");
}

void kGUIListboxObj::SetSelection(int s,bool add)
{
	if(SetSelectionz(s,add)==false)
	{
		assert(false,"Selection number not found!");
	}
}

bool kGUIListboxObj::SetSelectionz(int s,bool add)
{
	unsigned int i;

	for(i=0;i<m_numentries;++i)
	{
		if(m_tableentries[i]->GetValue()==s)
		{
			m_type=COMBOTYPE_NUM;
			SelectRow(i,add);
			Dirty();
			return(true);
		}
	}
	return(false);	/* not found! return without an error */
}

void kGUIListboxObj::SetSelection(const char *string,bool add)
{
	if(SetSelectionz(string,add)==false)
		passert(false,"Selection '%s' not in list!",string);
}

bool kGUIListboxObj::SetSelectionz(const char *string,bool add)
{
	unsigned int i;

	m_type=COMBOTYPE_STRING;
	for(i=0;i<m_numentries;++i)
	{
		if(!stricmp(m_tableentries[i]->GetString(),string))
		{
			SelectRow(i,add);
			Dirty();
			return(true);
		}
	}
	return(false);
}

void kGUIListboxObj::SetSelectionString(const char *string,bool add)
{
	if(SetSelectionStringz(string,add)==false)
		passert(false,"Selection '%s' not in list!",string);
}

bool kGUIListboxObj::SetSelectionStringz(const char *string,bool add)
{
	unsigned int i;

	m_type=COMBOTYPE_STRING;
	for(i=0;i<m_numentries;++i)
	{
		assert(m_tableentries[i]->GetIsTextValue()==true,"Error: not a text value entry!");
		if(!stricmp(m_tableentries[i]->GetTextValue()->GetString(),string))
		{
			SelectRow(i,add);
			Dirty();
			return(true);
		}
	}
	return(false);
}


kGUIString *kGUIListboxObj::GetSelectionStringObj(unsigned int entry)
{
	assert(entry<m_numentries,"kGUIListboxObj: index too large");

	if(m_tableentries[entry]->GetIsTextValue()==true)
		return(m_tableentries[entry]->GetTextValue());
	else
		return(m_tableentries[entry]->GetText());
}

/* return the widest entry in screen pixels */
int kGUIListboxObj::GetWidest(void)
{
	unsigned int i;
	int w,widest;

	widest=0;
	for(i=0;i<m_numentries;++i)
	{
		/* extra space is added to account for the table edge bevels */
		/* and the scrollbar on the right */
		w=m_tableentries[i]->GetWidth()+kGUI::GetSkin()->GetScrollbarWidth()+10;
		if(w>widest)
			widest=w;
	}
	return(widest);
}

/* the listbox can have a single selectionor multipleitems selected */
/* return number of selected lines and build array of indices */

unsigned int kGUIListboxObj::GetSelections(Array<unsigned int>*list)
{
	unsigned int i;
	unsigned int numsel;

	numsel=0;
	for(i=0;i<m_numentries;++i)
	{
		if(m_tableentries[i]->GetSelected())
			list->SetEntry(numsel++,i);
	}

	if(!numsel)	/* if none are selected, then the selection is the current cursor line */
		list->SetEntry(numsel++,m_selection);
	return(numsel);
}

/* what will the height be if I display 'numrows' lines */
unsigned int kGUIListboxObj::CalcHeight(unsigned int numrows)
{
	unsigned int h;
	unsigned int i;

	/* if there are not that many items then clip */
	if(numrows>m_numentries)
		numrows=m_numentries;
	if(!numrows)
		return(0);	/* no entries, so no height */

	/* if any columns have been changed then recalc row sizes */
	UpdateIfDirty();

	/* since it is possible to have varying line heights we will just use the first n */
	/* in the table and ignore the possibility that further lines may be taller */
	h=3;	/* assume extra for top/bottom */
	for(i=0;i<numrows;++i)
		h+=m_tableentries[i]->GetZoneH();

	return(h);
}
