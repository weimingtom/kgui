/*********************************************************************************/
/* kGUI - kguitab.cpp                                                            */
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

kGUITabObj::kGUITabObj()
{
	m_locked=false;
	m_track=false;
	m_curtab=0;
	m_numtabs=0;
	m_numgroups=0;
	m_starttabx=0;
	UpdateTabs();
}

/* the "z" means that if the name is not found, then just return false */
/* the same function with out the "z" suffix will assert */

bool kGUITabObj::SetCurrentTabNamez(const char *name)
{
	int i;

	for(i=0;i<m_numtabs;++i)
	{
		if(!strcmp(GetTabName(i),name))
		{
			SetCurrentTab(i);
			return(true);
		}
	}
	return(false);
}

void kGUITabObj::SetNumTabs(int numtabs,int numgroups)
{
	int i,j;
	kGUIText *t;

	/* delete old names if they exist and reallocate below */
	for(i=0;i<m_numtabs;++i)
	{
		delete m_tabnames.GetEntry(i);
		m_tabnames.SetEntry(i,0);
	}

	m_numtabs=numtabs;
	m_numgroups=numgroups;
	m_curtab=0;
	m_overtab=-1;
	SetNumGroups(numgroups);
	m_tabnames.Alloc(numtabs);
	m_tabgroups.Alloc(numtabs);
	m_tabx.Alloc(numtabs);
	m_taby.Alloc(numtabs);
	for(i=0;i<m_numtabs;++i)
	{
		t=new kGUIText;
		m_tabnames.SetEntry(i,t);
		j=i;
		if(j>=numgroups)
			j=numgroups-1;
		m_tabgroups.SetEntry(i,j);
	}
	UpdateTabs();
}

kGUITabObj::~kGUITabObj()
{
	int i;

	if(m_track==true)
	{
		kGUI::DelEvent(this,CALLBACKNAME(Track));
		m_track=false;
	}
	for(i=0;i<m_numtabs;++i)
	{
		delete m_tabnames.GetEntry(i);
		m_tabnames.SetEntry(i,0);
	}
}

/* calc the number of tab rows and positions of each tab */

void kGUITabObj::UpdateTabs(void)
{
	int i,x,y,tw;
	int	w;
	int l,r,h,exp;

	/* get the pixels size for the left/right and height of the tabs */
	/* from the current skin engine. "exp"=expand size when tab is selected. */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);

	w=(GetZoneW())-(exp<<1);	/* allow space for expanded (current) tab */
	m_numtabrows=1;				/* start with one row for tabs */
	y=exp;						/* larger (current) tab is exp pixels taller and wider*2 */
	x=exp+m_starttabx;			/* than all other tabs so leave room for it to expand */
	for(i=0;i<m_numtabs;++i)
	{
		tw=m_tabnames.GetEntry(i)->GetWidth()+8+l+r;	/* pixel space needed for tab */
		if((x+tw)>w)
		{
			x=exp;	/* need another row to fit tabs so start a new row */
			y+=h;
			++m_numtabrows;
		}
		m_tabx.SetEntry(i,x);	/* save tab position so we don't need to re-calc unless */
		m_taby.SetEntry(i,y);	/* tabs are added */
		x+=tw;
	}
	DirtyandCalcChildZone();	/* child size varies by space needed for the tabs so re-calc */
}

int kGUITabObj::GetTabRowHeight(void)
{
	int l,r,h,exp;

	/* how tall is a row of tabs? */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);
	return(exp+h);
}

void kGUITabObj::CalcChildZone(void)
{
	int l,r,h,exp,taby;

	/* how tall is a row of tabs? */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);
	/* space needed for tabs is height for tab * number of tabs rows */
	taby=exp+(h*m_numtabrows);
	/* children get remaining area */
	SetChildZone(2,taby,GetZoneW()-4,GetZoneH()-taby);
}

void kGUITabObj::Track(void)
{
	kGUICorners c;
	int l,r,h,exp;
	bool over;

	/* calc size of tab area */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);
	GetCorners(&c);
	c.by=c.ty+exp+(h*m_numtabrows);

	over=kGUI::MouseOver(&c);
	if(!over)
	{
		kGUI::DelEvent(this,CALLBACKNAME(Track));
		m_track=false;
		DirtyTab(m_overtab);
		m_overtab=-1;
	}
}

/* only dirty a specific tab at the top, not the whole thing */
void kGUITabObj::DirtyTab(int tab)
{
	kGUICorners c;
	kGUICorners tc;
	int tw;
	int l,r,h,exp;

	/* -1 == no current overtab, so ignore */
	if(tab<0)
		return;

	/* calc size of tab area */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);
	GetCorners(&c);
	tw=m_tabnames.GetEntry(tab)->GetWidth()+8+l+r;
	tc.lx=c.lx+m_tabx.GetEntry(tab);
	tc.rx=tc.lx+tw;
	tc.ty=c.ty+m_taby.GetEntry(tab);
	tc.by=tc.ty+h;

	Dirty(&tc);
}


bool kGUITabObj::UpdateInput(void)
{
	kGUICorners c;
	kGUICorners tc;
	int i,tw;
	int l,r,h,exp;
	bool over;

	/* calc size of tab area */
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);
	GetCorners(&c);
	c.by=c.ty+exp+(h*m_numtabrows);

	/* is the mouse over the tab button area? */
	over=kGUI::MouseOver(&c);
	if(over && m_locked==false)
	{
		/* if i'm not active then activate me */
		/* I need to be active so I can track the mouse when it moves */
		/* off of the tab area so I can unhilight the last tab under the cursor */
		if(m_track==false)
		{
			m_track=true;
			kGUI::AddEvent(this,CALLBACKNAME(Track));
		}
		/* yes they have mouse over the tabs on the top */
		/* which tab is the mouse over? (if any) */
		for(i=0;i<m_numtabs;++i)
		{
			tw=m_tabnames.GetEntry(i)->GetWidth()+8+l+r;
			tc.lx=c.lx+m_tabx.GetEntry(i);
			tc.rx=tc.lx+tw;
			tc.ty=c.ty+m_taby.GetEntry(i);
			tc.by=tc.ty+h;

			if(kGUI::MouseOver(&tc))
			{
				/* yes mouse is over this tab */
				if(i!=m_overtab)
				{
					DirtyTab(m_overtab);
					DirtyTab(i);
					m_overtab=i;
				}

				/* are they rightclicking on the tab? */
				if(kGUI::GetMouseClickRight()==true)
				{
					if(m_overtab!=m_curtab)		/* set this to the current tab first */
					{
						kGUI::ClearActiveStack();
						m_curtab=m_overtab;
						/* then call tabclicked callback */
						CallEvent(EVENT_MOVED);
					}
					/* then call rightclick tab callback */
					CallEvent(EVENT_RIGHTCLICK);
				}
				if(kGUI::GetMouseReleaseLeft()==true)
				{
					if(m_overtab!=m_curtab)
					{
						kGUI::ClearActiveStack();
						m_curtab=m_overtab;
						Dirty();
						/* call the tabclicked callback */
						CallEvent(EVENT_MOVED);
					}
				}
				return(true);
			}
		}
	}

	/* if we got here then the mouse is not hovering over any of the tabs */
	/* so we need to reset the overtab variable back to "not over any" (-1) */
	if(m_overtab!=-1)
	{
		DirtyTab(m_overtab);	/* redraw */
		m_overtab=-1;
	}

	/* send input to children of currently selected tab */
	if(m_numtabs)
		return(UpdateInputC(m_tabgroups.GetEntry(m_curtab)));
	return(false);
}

void kGUITabObj::Draw(void)
{
	int i,x,y;
	kGUIText *text;
	kGUICorners c;
	kGUICorners cc;
	int l,r,h,exp;
	kGUI::GetSkin()->GetTabSize(&exp,&l,&r,&h);

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		/* draw background */
		kGUI::DrawRect(c.lx,c.ty+exp+(h*m_numtabrows),c.rx,c.ty+exp+(h*m_numtabrows)+1,DrawColor(145,167,180));
		kGUI::DrawRectBevel(c.lx,c.ty+exp+(h*m_numtabrows)+1,c.rx,c.by,false);
		
		/* draw tab names */
		for(i=0;i<m_numtabs;++i)
		{
			x=c.lx+m_tabx.GetEntry(i);
			y=c.ty+m_taby.GetEntry(i);
			text=m_tabnames.GetEntry(i);

			kGUI::GetSkin()->DrawTab(text,x,y,i==m_curtab,i==m_overtab);
		}

		if(m_numtabs)	/* if no tabs then m_curtab is not valid doh! */
		{
			kGUI::PushClip();
			GetChildCorners(&cc);
			kGUI::ShrinkClip(&cc);
			if(kGUI::ValidClip())
				DrawC(m_tabgroups.GetEntry(m_curtab));	/* draw all children of the currently selected tab */
			kGUI::PopClip();
		}
	}
	kGUI::PopClip();
}
