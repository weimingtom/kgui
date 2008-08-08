/*********************************************************************************/
/* kGUI - kguimenu2.cpp                                                           */
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
/* This is a full menu class with sub-menus, not finished yet, just starting     */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

#define TITLEGAPX 10
#define TITLEGAPY 4

kGUIMenuObj::kGUIMenuObj()
{
	m_numentries=0;
	m_depth=0;
	m_track=false;
	m_colhover=-1;
	m_title.Init(16,4);
	m_titlex.Init(16,4);
	m_titlew.Init(16,4);
	m_entry.Init(16,4);
	m_activeentry.Init(4,4);
}

kGUIMenuObj::~kGUIMenuObj()
{
	if(m_track==true)
	{
		kGUI::DelEvent(this,CALLBACKNAME(Track));
		m_track=false;
	}
}

void kGUIMenuObj::SetNumEntries(unsigned int n)
{
	unsigned int i;

	m_numentries=n;
	for(i=0;i<n;++i)
	{
		m_title.GetEntryPtr(i)->SetMenu(this);
		m_title.GetEntryPtr(i)->SetFontInfo(this,false);
	}
}

void kGUIMenuObj::Resize(void)
{
	int i;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;

	/* calc new size */
	x=0;
	y=0;
	for(i=0;i<m_numentries;++i)
	{
		w=m_title.GetEntryPtr(i)->GetWidth();
		h=m_title.GetEntryPtr(i)->GetHeight();
		m_titlex.SetEntry(i,x);
		m_titlew.SetEntry(i,w);
		w+=TITLEGAPX+TITLEGAPX;		/* left gap + right gap */
		x+=w;
		y=max(y,h);
	}
	SetZone(0,0,x,y+TITLEGAPY+TITLEGAPY);
}

void kGUIMenuObj::Track(void)
{
	bool over;
	kGUICorners c;

	GetCorners(&c);

	/* is the mouse over the title area? */
	over=kGUI::MouseOver(&c);
	if(over==false && !m_depth)
	{
		kGUI::DelEvent(this,CALLBACKNAME(Track));
		m_track=false;

		m_colhover=-1;
		Dirty();
	}
}

bool kGUIMenuObj::UpdateInput(void)
{
	int i;
	kGUICorners c;
	kGUICorners tc;
	bool over;

	GetCorners(&c);

	/* is the mouse over the title area? */
	over=kGUI::MouseOver(&c);
	if(over)
	{
		/* if i'm not active then activate me */
		/* I need to be active so I can track the mouse when it moves */
		/* off of the header area so I can unhilight the last header under the cursor */
		if(m_track==false)
		{
			m_track=true;
			kGUI::AddEvent(this,CALLBACKNAME(Track));
		}
		/* yes they have mouse over the tabs on the top */
		/* which tab is the mouse over? (if any) */
		for(i=0;i<m_numentries;++i)
		{
			tc.lx=c.lx+m_titlex.GetEntry(i);
			tc.rx=tc.lx+m_titlew.GetEntry(i);
			tc.ty=c.ty;
			tc.by=c.by;

			if(kGUI::MouseOver(&tc))
			{
				/* yes mouse is over this column */
				if(i!=m_colhover)
				{
					Dirty();
					m_colhover=i;
					if(m_depth)
					{
						CloseMenu();
						OpenMenu(m_entry.GetEntry(i),tc.lx,tc.by);
					}
				}

				/* are they clicking on the menu header? */
				if(kGUI::GetMouseClickLeft()==true)
				{
					kGUI::SetForceUsed(true);
					if(!m_depth)
						OpenMenu(m_entry.GetEntry(i),tc.lx,tc.by);
					else
						CloseMenu();
				}
				return(true);
			}
		}
	}
	return(false);
}

void kGUIMenuObj::OpenMenu(kGUIMenuColObj *menu,int x,int y)
{
	kGUIMenuEntryObj *me;

	/* this callback can be trapped to enable / disable menu entries */
	if(!m_depth)
		CallEvent(EVENT_ENTER);

	m_activeentry.SetEntry(m_depth++,menu);
	menu->SetEventHandler(0,0);
	menu->SetEventHandler(this,CALLBACKNAME(MenuEvent));
	menu->SetDrawPopRow(false);
	menu->Activate(x,y);
	Dirty();

	/* check to see if the first entry has a sub-menu */
	me=menu->GetCurrentEntry();
	if(me->GetSubMenu())
	{
		kGUICorners c;

		/* activate sub-menu */
		me->GetCorners(&c);
		OpenMenu(me->GetSubMenu(),c.rx-6,c.ty+2);
	}
}

void kGUIMenuObj::MenuEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_MOUSEOFF:
		/* pop the menu as the mouse is no longer over it */
		if(m_depth==1)
			UpdateInput();
		else
		{
			if(event->GetObj()==this)
			{
				if(m_depth>1)
					CloseMenu();
			}
			else
			{
				int d;
				bool found;

				kGUIMenuColObj *menu;
				kGUIMenuEntryObj *me;
				kGUICorners c;

				found=false;
				for(d=0;d<m_depth;++d)
				{
					if(event->GetObj()==m_activeentry.GetEntry(d))
					{
						found=true;
						break;
					}
				}
				assert(found,"Huh, object not in list!");
				if(d>0)
				{
					menu=m_activeentry.GetEntry(d-1);
					me=menu->GetCurrentEntry();

					me->GetCorners(&c);
					if(kGUI::MouseOver(&c)==false)
						CloseMenu();
				}				
			}
		}
	break;
	case EVENT_MOVED:
	{
		int d;
		bool found;
		kGUIMenuColObj *menu;
		kGUIMenuEntryObj *me;

		found=false;
		for(d=0;d<m_depth;++d)
		{
			if(event->GetObj()==m_activeentry.GetEntry(d))
			{
				found=true;
				break;
			}
		}
		assert(found,"Huh, object not in list!");
		/* check for open/close submenu */
		while(d<(m_depth-1))
			CloseMenu();

		/* open sub-menu? */
		if(m_depth>0)
		{
			menu=m_activeentry.GetEntry(m_depth-1);
			me=menu->GetCurrentEntry();
			if(me->GetSubMenu())
			{
				kGUICorners c;

				/* activate sub-menu */
				me->GetCorners(&c);
				OpenMenu(me->GetSubMenu(),c.rx-6,c.ty+2);
			}
		}
	}		
	break;
	case EVENT_SELECTED:
		/* pass selected event back to parent event handler */
		CallEvent(EVENT_SELECTED,event);
		while(m_depth)
			CloseMenu();
	break;
	}
}

void kGUIMenuObj::CloseMenu(void)
{
	kGUIMenuColObj *col;
#if 0
	kGUIMenuColObj *menu;
	kGUIMenuEntryObj *me;
#endif

	/* close the current open menu */
	if(m_depth)
	{
		col=m_activeentry.GetEntry(m_depth-1);
		col->Close();
		--m_depth;
		Dirty();
		if(m_depth)
		{
			col=m_activeentry.GetEntry(m_depth-1);
			col->ReActivate();
		}
	}

#if 0
	/* check for open-submenu */
	if(m_depth>0)
	{
		menu=m_activeentry.GetEntry(m_depth-1);
		me=menu->GetCurrentEntry();
		if(me->GetSubMenu())
		{
			kGUICorners c;
			/* activate sub-menu */
			me->GetCorners(&c);
			OpenMenu(me->GetSubMenu(),c.rx-6,c.ty+2);
		}
	}
#endif
}

void kGUIMenuObj::Draw(void)
{
	int i;
	kGUICorners c;

	GetCorners(&c);
//	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(128,128,128));
	for(i=0;i<m_numentries;++i)
	{
		if(i==m_colhover)
		{
			if(!m_depth)
				kGUI::DrawRectFrame(c.lx+m_titlex.GetEntry(i),c.ty,c.lx+m_titlex.GetEntry(i)+m_titlew.GetEntry(i)+(TITLEGAPX<<1),c.by,DrawColor(192,192,255),DrawColor(64,64,255));
			else
			{
				kGUI::DrawRect(c.lx+m_titlex.GetEntry(i),c.ty,c.lx+m_titlex.GetEntry(i)+m_titlew.GetEntry(i)+(TITLEGAPX<<1),c.ty+1,DrawColor(128,128,128));
				kGUI::DrawRect(c.lx+m_titlex.GetEntry(i),c.ty,c.lx+m_titlex.GetEntry(i)+1,c.by,DrawColor(128,128,128));
				kGUI::DrawRect(c.lx+m_titlex.GetEntry(i)+m_titlew.GetEntry(i)+(TITLEGAPX<<1)-1,c.ty,c.lx+m_titlex.GetEntry(i)+m_titlew.GetEntry(i)+(TITLEGAPX<<1),c.by,DrawColor(128,128,128));
			}
		}
		m_title.GetEntryPtr(i)->Draw(c.lx+TITLEGAPX+m_titlex.GetEntry(i),c.ty+TITLEGAPY,0,0);
	}
}


void kGUIMenuColTitleObj::StringChanged(void)
{
	m_m->Resize();
	m_m->Dirty();
}

void kGUIMenuColTitleObj::FontChanged(void)
{
	m_m->Resize();
	m_m->Dirty();
}
