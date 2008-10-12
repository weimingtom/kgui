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
	m_iconwidth=0;
	m_hassubmenu=false;
	m_drawpoprow=true;				/* draw bar beside rows to make popup menu stand out */			
	m_poptableentries.Init(32,16);
	m_poptable = new kGUITableObj;
	m_poptable->NoColHeaders();
	m_poptable->NoRowHeaders();
	m_poptable->NoColScrollbar();
	m_poptable->NoRowScrollbar();
	m_poptable->SetNumCols(3);
	m_poptable->SetColWidth(0,0);	/* icon column */
	m_poptable->SetColWidth(2,0);	/* popmenu indicator column */
	m_poptable->SetSelectMode();
	m_poptable->SetEventHandler(this,CALLBACKNAME(TableEvent));
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
	m_poptableentries.Purge();
	delete m_poptable;
}

void kGUIMenuColObj::SetNumEntries(int n)
{
	int i;

	m_selection=0;
	m_poptable->DeleteChildren(false);

	m_numentries=n;
	for(i=0;i<n;++i)
		m_poptable->AddRow(m_poptableentries.GetEntryPtr(i));
}

void kGUIMenuColObj::SetEntry(int index,kGUIString *entryname,int entryval)
{
	kGUIMenuEntryObj *me;

	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	if(entryval==-1)
		entryval=index;

	me=m_poptableentries.GetEntryPtr(index);
	me->SetFontInfo(&m_fontinfo);
	me->SetString(entryname);
	me->SetRowHeight(m_poptableentries.GetEntryPtr(index)->GetHeight()+4);
	me->SetValue(entryval);
	m_poptable->SetEntryEnable(index,true);		/* default entry to enabled */
}

void kGUIMenuColObj::SetEntry(int index,const char *entryname,int entryval)
{
	kGUIMenuEntryObj *me;

	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	if(entryval==-1)
		entryval=index;

	me=m_poptableentries.GetEntryPtr(index);
	me->SetFontInfo(&m_fontinfo);
	me->SetString(entryname);
	me->SetRowHeight(me->GetHeight()+4);
	me->SetValue(entryval);
	m_poptable->SetEntryEnable(index,true);		/* default entry to enabled */
}

/* return selection by name */
const char *kGUIMenuColObj::GetSelectionString(void)
{
	kGUIMenuEntryObj *me;

	assert(m_selection>=0,"kGUIMenuColObj: index too small");
	assert(m_selection<m_numentries,"kGUIMenuColObj: index too large");

	me=m_poptableentries.GetEntryPtr(m_selection);
	return(me->GetString());
}

void kGUIMenuColObj::SetBGColor(int index,kGUIColor bg)
{
	kGUIMenuEntryObj *me;

	assert(index<m_numentries,"kGUIMenuColObj: index too large");
	me=m_poptableentries.GetEntryPtr(index);
	me->SetBGColor(bg);
}

void kGUIMenuColObj::SetEntryEnable(int entryval,bool e, bool updatecolor)
{
	int i;
	kGUIMenuEntryObj *me;

	for(i=0;i<m_numentries;++i)
	{
		me=m_poptableentries.GetEntryPtr(i);
		if(me->GetIsBar()==false)
		{
			if(me->GetValue()==entryval)
			{
				m_poptable->SetEntryEnable(i,e);
				if(updatecolor==true)
				{
					if(e==true)
						me->SetTextColor(DrawColor(32,32,32));
					else
						me->SetTextColor(DrawColor(192,192,192));
				}
				return;
			}
		}
	}
	assert(false,"kGUIMenuColObj: entryval not found in list!");
}

/* return the widest entry in screen pixels */
int kGUIMenuColObj::GetWidest(void)
{
	int i,w,widest;
	int	redge;
	kGUIMenuEntryObj *me;


	if(m_drawpoprow)
		redge=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	else
		redge=0;

	m_hassubmenu=false;
	widest=0;
	for(i=0;i<m_numentries;++i)
	{
		me=m_poptableentries.GetEntryPtr(i);
		/* extra space is added to account for the table edge bevels */
		/* and the scrollbar on the right */
		if(me->GetIsBar()==true)
		{
			me->SetRowHeight(3+2+3);
			m_poptable->SetEntryEnable(i,false);
		}
		else
		{
			w=me->GetWidth()+redge+10;
			if(w>widest)
				widest=w;
			if(me->GetSubMenu())
				m_hassubmenu=true;
		}
	}
	if(m_hassubmenu)
		widest+=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	return(widest);
}

/* called by full menu to see if current entry has a submenu */
kGUIMenuEntryObj *kGUIMenuColObj::GetCurrentEntry(void)
{
	return m_poptableentries.GetEntryPtr(m_poptable->GetCursorRow());
}

void kGUIMenuColObj::ReActivate(void)
{
	m_poptable->Activate();
}

int kGUIMenuColObj::GetSelection(void)
{
	kGUIMenuEntryObj *me;
	if(m_selection<0)
		return(-1);

	me=m_poptableentries.GetEntryPtr(m_selection);
	return me->GetValue();
}

/* calculate new height */
void kGUIMenuColObj::Resize(void)
{
	if(m_numentries)
		SetZoneH(m_poptableentries.GetEntryPtr(m_selection)->GetHeight()+4);
}

void kGUIMenuColObj::Activate(int x,int y)
{
	int i,h,h2;
	int showentries;
	int subwidth;
	kGUIMenuEntryObj *me;
	kGUIText text;

//	assert(m_evcallback.IsValid(),"No Callback defined yet, use SetEventHandler(...)!");

	/* return if already active */
	if(m_isactive==true)
		return;

	m_isactive=true;

	/* calculate position of popup selector table */
	m_popx=x;
	m_poptable->SetPopRowHeaders(m_drawpoprow);	
	m_popw=GetWidest()+m_iconwidth+4;
	m_popy=y;
	showentries=m_numentries;
	if(showentries>MAXSHOWMENUCOL)
		showentries=MAXSHOWMENUCOL;

	/* calc size of popwindow */
	h=0;
	for(i=0;i<m_numentries;++i)
	{
		me=m_poptableentries.GetEntryPtr(i);
		h+=me->GetZoneH();
	}

	text.SetFontInfo(&m_fontinfo);
	h2=(((text.GetLineHeight())+4)*showentries);
	m_poph=min(h,h2)+4;

	/* if this will go off of the bottom of the screen */
	/* then move it above instead */

	if((m_popy+m_poph)>kGUI::GetScreenHeight())
		m_popy=kGUI::GetScreenHeight()-m_poph;

	/* if this will go off of the right of the screen */
	/* then move it left */
	if((m_popx+m_popw)>kGUI::GetScreenWidth())
		m_popx=kGUI::GetScreenWidth()-m_popw;

	if(m_hassubmenu)
		subwidth=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	else
		subwidth=0;

	Dirty();
	m_poptable->SetZone(m_popx,m_popy,m_popw,m_poph);
	m_poptable->SizeDirty();
	m_poptable->SetColWidth(0,m_iconwidth);
	m_poptable->SetColWidth(1,m_popw-(m_iconwidth+4+subwidth));
	m_poptable->SetColWidth(2,subwidth);
	m_poptable->Dirty();
	m_poptable->ClearOver();
	kGUI::PushActiveObj(this);
	kGUI::AddWindow(m_poptable);
	kGUI::PushActiveObj(m_poptable);

	/* check to see if entry 0 is disabled */
	if(m_poptable->GetEntryEnable(0)==false)
		m_poptable->MoveRow(1);	/* move down until at a valid row */
}

void kGUIMenuColObj::UpdateInput2(void)
{
	m_poptable->UpdateInput();
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

void kGUIMenuColObj::TableEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_MOUSEOFF:
		CallEvent(EVENT_MOUSEOFF);			/* pass event down to it's handler */
	break;
	case EVENT_MOVED:
		CallEvent(EVENT_MOVED);				/* pass event down to it's handler */
	break;
	case EVENT_SELECTED:
	{
		kGUIEvent e;
		kGUIMenuEntryObj *me;

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

		if(m_selection>=0)
		{
			me=m_poptableentries.GetEntryPtr(m_selection);
			e.m_value[0].i=me->GetValue();
		}
		else
			e.m_value[0].i=-1;

		CallEvent(EVENT_SELECTED,&e);
	}
	break;
	}
}

void kGUIMenuColObj::Close(void)
{
	if(m_isactive==true)
	{
		kGUI::DelWindow(m_poptable);
		Dirty();
		m_isactive=false;
	}
}

void kGUIMenuEntryObj::Draw(void)
{
	int col,numcols;
	kGUIObj **cellobjptrs=GetObjectList();
	kGUIObj *cellobj;
	kGUICorners c;
	int iconwidth=cellobjptrs[0]->GetZoneW();

	/* quick check for whole row */

	GetCorners(&c);

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		/* background for icons */
		if(iconwidth)
			kGUI::DrawRect(c.lx,c.ty,c.lx+iconwidth,c.by,DrawColor(246,243,226));

		if(GetIsBar())
		{
			/* draw a seperator bar */
			kGUI::DrawRect(c.lx+iconwidth,c.ty+3,c.rx,c.ty+3+1,DrawColor(128,128,128));
		}
		else
		{
			if(GetParent()->GetCurrentChild()==this)
				kGUI::DrawRectFrame(c.lx+1,c.ty+1,c.rx,c.by-1,DrawColor(192,192,255),DrawColor(64,64,255));

			numcols=GetNumObjects();
			
			for(col=0;col<numcols;++col)
			{
				cellobj=cellobjptrs[col];
				if(cellobj->GetZoneW()>0)	/* 0 width means column is hidden */
					cellobj->Draw();
			}
			if(m_submenu)
			{
				c.lx=c.rx-kGUI::GetSkin()->GetMenuRowHeaderWidth();
				kGUI::GetSkin()->DrawSubMenuMarker(&c);
			}
		}
	}


	kGUI::PopClip();
}
