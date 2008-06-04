/*********************************************************************************/
/* kGUI - kguimenu.cpp                                                           */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                                */
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
/*    along with kGUI; if not, write to the Free Software                        */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */
/*                                                                               */
/*********************************************************************************/

/*********************************************************************************/
/*                                                                               */
/* This is a single column popup menu, it uses the table code extensively        */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

//todo, change to max based on size not an arbitrary number
#define MAXSHOWMENUCOL 24

kGUIMenuColObj::kGUIMenuColObj()
{
	m_isactive=false;
	m_numentries=0;
	m_selection=0;
	m_poptableentries=0;
	m_poptable = new kGUITableObj;
	m_poptable->PopRowHeaders();	/* draw bar beside rows to make popup menu stand out */
	m_poptable->NoColHeaders();
	m_poptable->NoRowHeaders();
	m_poptable->NoColScrollbar();
	m_poptable->NoRowScrollbar();
	m_poptable->SetNumCols(1);
	m_poptable->SetSelectMode();
	m_poptable->SetSelectedCallBack(this,CALLBACKNAME(SelectionDone));
}

/* user will manually call setentry for each menu entry */
void kGUIMenuColObj::Init(int num)
{
	SetNumEntries(num);
}

void kGUIMenuColObj::Init(int num, const char **strings)
{
	int i;

	SetNumEntries(num);
	for(i=0;i<num;++i)
	{
		SetEntry(i,strings[i],i);
		SetEntryEnable(i,true);
	}
}

void kGUIMenuColObj::Init(int num, const char **strings,int *nums)
{
	int i;

	SetNumEntries(num);
	for(i=0;i<num;++i)
	{
		SetEntry(i,strings[i],nums[i]);
		SetEntryEnable(i,true);
	}
}

void kGUIMenuColObj::Init(int num, kGUIString *strings,int *nums)
{
	int i;

	SetNumEntries(num);
	for(i=0;i<num;++i)
		SetEntry(i,strings[i].GetString(),nums[i]);
}


kGUIMenuColObj::~kGUIMenuColObj()
{
	m_poptable->DeleteChildren();
	delete m_poptable;
	if(m_poptableentries)
		delete []m_poptableentries;
}

void kGUIMenuColObj::SetNumEntries(int n)
{
	int i;

	m_selection=0;
	m_poptable->DeleteChildren();

	if(m_poptableentries)
		delete []m_poptableentries;

	m_numentries=n;
	m_poptableentries=new kGUIMenuEntryObj *[n];

	for(i=0;i<n;++i)
	{
		m_poptableentries[i]=new kGUIMenuEntryObj;
		m_poptable->AddRow(m_poptableentries[i]);
	}
}

void kGUIMenuColObj::SetEntry(int index,kGUIString *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	if(entryval==-1)
		entryval=index;

	m_poptableentries[index]->SetFontInfo(&m_fontinfo);
	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
	m_poptable->SetEntryEnable(index,true);		/* default entry to enabled */
}

void kGUIMenuColObj::SetEntry(int index,const char *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	if(entryval==-1)
		entryval=index;
	m_poptableentries[index]->SetFontInfo(&m_fontinfo);
	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
	m_poptable->SetEntryEnable(index,true);		/* default entry to enabled */
}

/* return selection by name */
const char *kGUIMenuColObj::GetSelectionString(void)
{
	assert(m_selection>=0,"kGUIMenuColObj: index too small");
	assert(m_selection<m_numentries,"kGUIMenuColObj: index too large");
	return(m_poptableentries[m_selection]->GetString());
}

void kGUIMenuColObj::SetBGColor(int index,kGUIColor bg)
{
	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	m_poptableentries[index]->SetBGColor(bg);
}

void kGUIMenuColObj::SetEntryEnable(int index,bool e, bool updatecolor)
{
	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	m_poptable->SetEntryEnable(index,e);
	if(updatecolor==true)
	{
		if(e==true)
			m_poptableentries[index]->SetTextColor(DrawColor(32,32,32));
		else
			m_poptableentries[index]->SetTextColor(DrawColor(192,192,192));
	}
}

/* return the widest entry in screen pixels */
int kGUIMenuColObj::GetWidest(void)
{
	int i,w,widest;
	int	redge=kGUI::GetSkin()->GetMenuRowHeaderWidth();

	widest=0;
	for(i=0;i<m_numentries;++i)
	{
		/* extra space is added to account for the table edge bevels */
		/* and the scrollbar on the right */
		w=m_poptableentries[i]->GetWidth()+redge+10;
		if(w>widest)
			widest=w;
	}
	return(widest);
}

int kGUIMenuColObj::GetSelection(void)
{
	if(m_selection<0)
		return(-1);
	return m_poptableentries[m_selection]->GetValue();
}

/* calculate new height */
void kGUIMenuColObj::Resize(void)
{
	if(m_poptableentries)
	{
		SetZoneH(m_poptableentries[m_selection]->GetHeight()+4);
	}
}

void kGUIMenuColObj::Activate(int x,int y)
{
	kGUIText text;
	int showentries;

//	assert(m_evcallback.IsValid(),"No Callback defined yet, use SetEventHandler(...)!");

	/* return if already active */
	if(m_isactive==true)
		return;

	m_isactive=true;

	/* calculate position of popup selector table */
	m_popx=x;
	m_popw=GetWidest();
	m_popy=y;
	showentries=m_numentries;
	if(showentries>MAXSHOWMENUCOL)
		showentries=MAXSHOWMENUCOL;
	text.SetFontInfo(&m_fontinfo);
	m_poph=(((text.GetHeight())+4)*showentries);

	/* if this will go off of the bottom of the screen */
	/* then move it above instead */

	if((m_popy+m_poph)>kGUI::GetFullScreenHeight())
		m_popy=kGUI::GetFullScreenHeight()-m_poph;

	/* if this will go off of the right of the screen */
	/* then move it left */
	if((m_popx+m_popw)>kGUI::GetFullScreenWidth())
		m_popx=kGUI::GetFullScreenWidth()-m_popw;

	Dirty();
	m_poptable->SetZone(m_popx,m_popy,m_popw,m_poph);
	m_poptable->SizeDirty();
	m_poptable->SetColWidth(0,m_popw);
	m_poptable->Dirty();
	kGUI::PushActiveObj(this);
	kGUI::AddWindow(m_poptable);
	kGUI::PushActiveObj(m_poptable);

	/* check to see if entry 0 is disabled */
	if(m_poptable->GetEntryEnable(0)==false)
		m_poptable->MoveRow(1);	/* move down until at a valid row */
}

void kGUIMenuColObj::SelectionDone(void)
{
	kGUIEvent e;

	m_selection=m_poptable->GetSelected();

	/* in the case where all entries are disabled, it could */
	/* return an invalid result, so check! */
	if(m_selection>=0)
	{
		if(m_poptable->GetEntryEnable(m_selection)==false)
			m_selection=-1;
	}
	
	kGUI::DelWindow(m_poptable);
	Dirty();
	m_isactive=false;					//put before callback since we can be deleted in the callback

	/* todo, remove m_selection use local only */
	e.m_value[0].i=m_selection;
	/* hmm, todo, change to EVENT_CANCELLED if input==-1 */
	CallEvent(EVENT_SELECTED,&e);
}

bool kGUIMenuColObj::UpdateInput(void)
{
	/* finish selection */
	if(kGUI::GetActiveObj()==this)
		kGUI::PopActiveObj();
	/* -1 = clicked outside of menu */
	
	m_selection=m_poptable->GetSelected();

	/* in the case where all entries are disabled, it could */
	/* return an invalid result, so check! */
	if(m_selection>=0)
	{
		if(m_poptable->GetEntryEnable(m_selection)==false)
			m_selection=-1;
	}
	
	m_poptable->UpdateInput();
	kGUI::DelWindow(m_poptable);

	Dirty();
	CallEvent(EVENT_AFTERUPDATE);
	m_isactive=false;
	return(true);
}

