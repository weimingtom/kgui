/*********************************************************************************/
/* kGUI - kguiwindow.cpp                                                         */
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
/* This is the window class, it handles the background window and also all       */
/* overlay windows                                                               */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

/* minimum window size */
#define WMINW 128
#define WMINH 64

kGUIWindowObj::kGUIWindowObj()
{
	m_title.SetWindow(this);

	m_frame=true;
	m_background=false;						/* only true for the 1 background window */
	m_minimized=false;
	m_full=false;
	m_activemode=WINDOWMODE_NONE;
	m_title.SetFontSize(14);
	m_title.SetColor(DrawColor(255,255,255));
	m_title.SetVAlign(FT_MIDDLE);
	SetNumGroups(1);						/* only 1 container group needed for a window */
	m_over=WINDOWBUTTON_NONE;
	m_allow=WINDOWBUTTON_CLOSE|WINDOWBUTTON_MINIMIZE|WINDOWBUTTON_FULL;
}

void kGUIWindowTitle::StringChanged(void)
{
	m_w->Dirty();
}

void kGUIWindowTitle::FontChanged(void)
{
	m_w->Dirty();
}

void kGUIWindowObj::Center(void)
{
	SetPos((kGUI::GetFullScreenWidth()-GetZoneW())>>1,(kGUI::GetFullScreenHeight()-GetZoneH())>>1);
}

/* set the position of the window */

void kGUIWindowObj::SetPos(int x,int y)
{
	if(m_background==false)
		kGUIObj::SetPos(x,y);
	else
		kGUI::SetWindowPos(x,y);
}

void kGUI::MoveWindowPos(int dx,int dy)
{
		int x,y,newx,newy,neww,newh;

		kGUI::GetWindowPos(&x,&y,&neww,&newh);
		newx=x+dx;
		newy=y+dy;

		kGUI::SetWindowPos(newx,newy);
		/* since the system may have not moved it where we asked we need to get it's actual position */
		kGUI::GetWindowPos(&newx,&newy,&neww,&newh);
		/* since mouse movements are relative to the window, we need to adjust */
		/* the mouse position if we move the window position */
		kGUI::AdjustMouse(x-newx,y-newy);
}

/* set the size of the window, this sets the external size of the */
/* window, not the child area. The child area will be smaller by the */
/* size of the frame which can vary based on the current skin code */

void kGUIWindowObj::SetSize(int w,int h)
{
	if(m_background==true)
	{
		int x,y;

		kGUI::SetScreenSize(w,h);
		kGUI::SetWindowSize(w,h);

		/* get actual size because sometimes it is made smaller then what was requested */
		kGUI::GetWindowPos(&x,&y,&w,&h);
	}
	kGUIContainerObj::SetSize(w,h);
}

/* the size requested is for the child area so add extra space to display the frame */
void kGUIWindowObj::SetInsideSize(int w,int h)
{
	if(m_frame==true)
	{
		kGUICorners offs;

		/* get 4 edge offsets from the skin engine */
		kGUI::GetSkin()->GetWindowEdges(&offs);
		SetSize(w+offs.lx+offs.rx,h+offs.ty+offs.by);
	}
	else
		SetSize(w,h);
}

/*  make the window as small as it can be to fit the child objects */

void kGUIWindowObj::Shrink(void)
{
	int e,nc;
	int cw,ch;
	int sw,sh;
	int oor,oob;
	kGUIObj *gobj;

	/* current size */
	cw=GetZoneW();
	ch=GetZoneH();

	sw=0;
	sh=0;

	nc=GetNumChildren();
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(e);
		oor=gobj->GetZoneRX();
		oob=gobj->GetZoneBY();
		if(oor>sw)
			sw=oor;
		if(oob>sh)
			sh=oob;
	}
	sw+=8;
	sh+=8;
	if(sw<cw)
		cw=sw;
	if(sh<ch)
		ch=sh;
	SetInsideSize(cw,ch);
}

/* close the window, delete it from the global window list */
/* and call the users close callback function */

void kGUIWindowObj::Close(void)
{
	/* if not already done */
	kGUI::DelWindow(this);

	CallEvent(EVENT_CLOSE);
}

/* expand window to fit all child objects */
void kGUIWindowObj::ExpandToFit(void)
{
	int x,y,maxw,maxh;
	int e,nc;
	kGUIObj *gobj;

	/* start with current size */
	maxw=GetZoneW()-4;
	maxh=GetZoneH()-4;

	nc=GetNumChildren();
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(e);
		x=gobj->GetZoneRX();
		y=gobj->GetZoneBY();
		if(x>maxw)
			maxw=x;
		if(y>maxh)
			maxh=y;
	}
	SetInsideSize(maxw+4,maxh+4);
}

/* calculate the area for child objects by taking the full window */
/* size and subtracting the area that the skin code uses for the frame */

void kGUIWindowObj::CalcChildZone(void)
{
	kGUICorners offs;

	if(m_frame==true)
	{
		/* get 4 edge offsets from the skin engine */
		kGUI::GetSkin()->GetWindowEdges(&offs);
		SetChildZone(offs.lx,offs.ty,GetZoneW()-(offs.lx+offs.rx),GetZoneH()-(offs.ty+offs.by));
	}
	else
		SetChildZone(0,0,GetZoneW(),GetZoneH());
}

/* process user input */

bool kGUIWindowObj::UpdateInput(void)
{
	int dx,dy;
	int oldx,oldy,oldw,oldh;
	int newx,newy,neww,newh;
	kGUICorners c;
	kGUICorners c2;
	kGUICorners cedges;
	unsigned int newover=WINDOWBUTTON_NONE;
	kGUICorners cclose;
	kGUICorners cfull;
	kGUICorners cminimize;

	GetCorners(&c);
	
	/* if the user clicks on me then move me to the top unless I am the */
	/* background window, or the current top window . */
	if(kGUI::GetMouseClick())
	{
		if(kGUI::AmITheTopWindow(this)==false)
		{
			/* has the current window been told to stay on top? */
			if(kGUI::StayTopWindow()==true)
				return(false);

			/* I can be moved to the top, unless I am the background */
			if(m_background==false)
				kGUI::TopWindow(this);
		}
	}
#if 0
	if(GetTop()==true && ImCurrent()==false)
	{
		/* if i'm to stay on top, then make me active! */
		m_activemode=WINDOWMODE_NONE;
		kGUI::PushActiveObj(this);
	}
#endif

	if(kGUI::GetKey()==GUIKEY_TAB)
	{
		/* iterate control though the child objects */
		Tab(1);
		/* eat the key so other objects don't get it */
		kGUI::ClearKey();
	}
	else if(kGUI::GetKey()==GUIKEY_SHIFTTAB)
	{
		/* iterate control though the child objects */
		Tab(-1);
		/* eat the key so other objects don't get it */
		kGUI::ClearKey();
	}

	/* get size of skin edges so we can detect when the user is clicking */
	/* on the edges for moving and or resizeing the window etc */
	kGUI::GetSkin()->GetWindowEdges(&cedges);

	/* am I already the active object? */
	if(this==kGUI::GetActiveObj() && m_activemode!=WINDOWMODE_NONE)
	{
		/* must be in the process of positioning or re-sizing */
		if(kGUI::GetMouseLeft()==true)
		{
			dx=kGUI::GetMouseDX();
			dy=kGUI::GetMouseDY();
			if(dx||dy)
			{
				oldx=newx=GetZoneX();
				oldy=newy=GetZoneY();
				oldw=neww=GetZoneW();
				oldh=newh=GetZoneH();

				switch(m_activemode)
				{
				case WINDOWMODE_ADJPOS:
					if(GetBackground()==true)
						kGUI::MoveWindowPos(dx,dy);
					else
					{
						newx+=dx;
						newy+=dy;
						if(newx<0)
						{
							newx=0;
							kGUI::MoveWindowPos(dx,0);
						}
						if(newy<0)
						{
							newy=0;
							kGUI::MoveWindowPos(0,dy);
						}
					}
				break;
				case WINDOWMODE_ADJWIDTH:
					if(GetBackground()==true)
					{
						/* unimplemented */
					}
					else
					{
						kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTHORIZ);
						neww+=dx;
						if(neww<WMINW)
							neww=WMINW;
						else if(neww>kGUI::GetBackground()->GetZoneW())
							neww=kGUI::GetBackground()->GetZoneW();
					}
				break;
				case WINDOWMODE_ADJHEIGHT:
					if(GetBackground()==true)
					{
						/* unimplemented */
					}
					else
					{
						kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);
						newh+=dy;
						if(newh<WMINH)
							newh=WMINH;
						else if(newh>kGUI::GetBackground()->GetZoneH())
							newh=kGUI::GetBackground()->GetZoneH();
					}
				break;
				case WINDOWMODE_ADJSIZE:
					if(GetBackground()==true)
					{
						/* unimplemented */
					}
					else
					{
						kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTSIZE);
						neww+=dx;
						newh+=dy;
						if(neww<WMINW)
							neww=WMINW;
						else if(neww>kGUI::GetBackground()->GetZoneW())
							neww=kGUI::GetBackground()->GetZoneW();
						if(newh<WMINH)
							newh=WMINH;
						else if(newh>kGUI::GetBackground()->GetZoneH())
							newh=kGUI::GetBackground()->GetZoneH();
					}
				break;
				case WINDOWMODE_NONE:
				break;
				}
				/* have I changed? */
				if((newx!=oldx) || (newy!=oldy) || (neww!=oldw) || (newh!=oldh)) 
				{
					SetZone(newx,newy,neww,newh);
					/* turn off full pixel interpolation as it is to slow when */
					/* dragging a window around */
					kGUI::SetFastDraw(true);

					CallEvent(EVENT_SIZECHANGED);
				}
				return(true);	/* stay on top! */
			}
		}
		else
		{
			/* is pixel interpolation mode off? */
			if(kGUI::GetFastDraw()==true)
			{
				/* we have stopped moving so redraw one more time back in full quality mode */
				kGUI::SetFastDraw(false);
				Dirty();
			}
			if(GetTop()==true)
			{
				m_activemode=WINDOWMODE_NONE;
				return(true);	/* stay on top! */
			}
			else
			{
				kGUI::PopActiveObj();
				return(false);
			}
		}
	}

	/* get position of buttons on top bar */
	if(m_frame==true)
	{
		kGUI::GetSkin()->GetWindowButtonPositions(m_allow,&c,&cclose,&cfull,&cminimize);

		if(kGUI::MouseOver(&cminimize))
		{
			newover=WINDOWBUTTON_MINIMIZE;
			if(kGUI::GetMouseReleaseLeft()==true)
			{
				if(m_full==true)
				{
					if(GetBackground()==false)
					{
						m_full=false;
						SetZone(&m_savezone);
					}
				}
				if(m_minimized==false)
				{
					if(GetBackground()==true)
						kGUI::Minimize();
					else
					{
						m_minimized=true;
						CopyZone(&m_savezone);
						kGUI::FindMinimizeSpot(&c2);
						SetZone(c2.lx,c2.ty,c2.rx-c2.lx,c2.by-c2.ty);
					}
				}
				else
				{
					m_minimized=false;
					SetZone(&m_savezone);
				}
				return(true);
			}
		}
		else if(kGUI::MouseOver(&cfull))
		{
			if(GetTop()==false)
			{
				newover=WINDOWBUTTON_FULL;
				if(kGUI::GetMouseReleaseLeft()==true)
				{
					if(GetBackground()==false)
					{
						if(m_minimized==true)
						{
							m_minimized=false;
							SetZone(&m_savezone);
						}
						if(m_full==false)
						{
							kGUIContainerObj *bg;

							m_full=true;
							CopyZone(&m_savezone);
							
							/* get full screen size from background object */
							bg=kGUI::GetBackground();
							SetZone(bg->GetChildZoneX(),bg->GetChildZoneY(),bg->GetChildZoneW(),bg->GetChildZoneH());
						}
						else
						{
							m_full=false;
							SetZone(&m_savezone);
						}
					}
					/* make full */
					return(true);
				}
			}
		}
		else if(kGUI::MouseOver(&cclose))
		{
			/* make close */
			newover=WINDOWBUTTON_CLOSE;
			if(kGUI::GetMouseReleaseLeft()==true)
			{
				if(GetBackground()==true)
				{
					kGUI::CloseApp();
				}
				else
				{
					Dirty();
					kGUI::DelWindow(this);
					Close();
				}
				return(true);
			}
		}
		else if(kGUI::GetMouseClickLeft()==true)
		{
			/* check for clicking on top area */
			c2.lx=c.lx;
			c2.ty=c.ty;
			c2.rx=c.rx;
			c2.by=c2.ty+cedges.ty;

			if(kGUI::MouseOver(&c2))
			{
				m_activemode=WINDOWMODE_ADJPOS;
				if(kGUI::GetActiveObj()!=this)
					kGUI::PushActiveObj(this);
				return(true);
			}
		}

		if(newover!=m_over)
		{
			/* only need to dirty area where 3 buttons are */
			m_over=newover;
			Dirty(&cfull);
			Dirty(&cminimize);
			Dirty(&cclose);
		}

		/* should I become active? */
		if(m_minimized==false)
		{
			/* check for clicking on bottom right corner (adjust both w&h) */
			c2.lx=c.rx-cedges.rx;
			c2.ty=c.by-cedges.by;
			c2.rx=c.rx;
			c2.by=c.by;

			if(kGUI::MouseOver(&c2))
			{
				kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTSIZE);
				if(kGUI::GetMouseClickLeft()==true)
				{
					m_activemode=WINDOWMODE_ADJSIZE;
					if(kGUI::GetActiveObj()!=this)
						kGUI::PushActiveObj(this);
					return(true);
				}
			}
			else
			{
				/* check for clicking on right edge */
				c2.lx=c.rx-cedges.rx;
				c2.ty=c.ty;
				c2.rx=c.rx;
				c2.by=c.by;

				if(kGUI::MouseOver(&c2))
				{
					kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTHORIZ);
					if(kGUI::GetMouseClickLeft()==true)
					{
						m_activemode=WINDOWMODE_ADJWIDTH;
						if(kGUI::GetActiveObj()!=this)
							kGUI::PushActiveObj(this);
						return(true);
					}
				}
				else
				{
					/* check for clicking on bottom edge */

					c2.lx=c.lx;
					c2.ty=c.by-cedges.by;
					c2.rx=c.rx;
					c2.by=c.by;

					if(kGUI::MouseOver(&c2))
					{
						kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);
						if(kGUI::GetMouseClickLeft()==true)
						{
							m_activemode=WINDOWMODE_ADJHEIGHT;
							if(kGUI::GetActiveObj()!=this)
								kGUI::PushActiveObj(this);
							return(true);
						}
					}
				}
			}
		}
	}

	if(GetTop()==true)
	{
		UpdateInputC(0);	/* pass input to children */
		return(true);		/* don't allow input to go to other windows */
	}

	return(UpdateInputC(0));	/* pass input to children */
}

/* draw the window and all children */
void kGUIWindowObj::Draw(void)
{
	kGUICorners c;
	kGUICorners cc;

	GetCorners(&c);
	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		/* call the skin draw code to draw the window edges */
		if(m_frame==true)
			kGUI::GetSkin()->DrawWindow(this,&c,m_allow,m_over);
		else
			kGUI::GetSkin()->DrawWindowNoFrame(this,&c);

		if(m_minimized==false)
		{
			/* draw children, clip against child zone */
			kGUI::PushClip();
			GetChildCorners(&cc);
			kGUI::ShrinkClip(&cc);
			if(kGUI::ValidClip())
				DrawC(0);			/* draw all children */
			kGUI::PopClip();
		}
	}
	kGUI::PopClip();
}
