/*********************************************************************************/
/* kGUI - kguiinput.cpp                                                          */
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
/* This is the standard inputbox object. It can be in the default mode where     */
/* all of the text shares a common font and point size, or it can be in          */
/* rich mode where each character can have it's own settings.                    */
/* if password mode is enabled then '*'s are rendered for each character         */
/*                                                                               */
/* todo: (m_numviewlines & m_numfullviewlines) remove these since it screws */
/* up in "rich" mode where each line can have a different height */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
//used for Find/Replace requestor
#include "kguireq.h"

#define ILGAP 2
#define IEDGE 4

kGUIInputBoxObj::kGUIInputBoxObj()
{
	m_ow=0;
	m_oh=0;
	m_xoff=0;
	m_hcursor=0;
	m_topoff=0;		/* lines */
	m_leftoff=0;	/* pixels */
	m_hstart=0;
	m_hrange=false;
	m_maxlen=-1;			/* default is no limit on size */
	m_locked=false;
	m_wrap=true;			/* default wrap mode to true */
	m_allowenter=false;
	m_allowtab=false;
	m_allowcursorexit=false;	
	m_allowfind=true;
	m_leaveselection=false;	
	m_leavescroll=false;	
	m_recalclines=true;
	m_usevs=false;
	m_usehs=false;
	m_allowundo=false;					/* default to off so as to not take to much memory */
	m_password=false;
	m_forceshowselection=false;
	m_hint=0;
	m_maxw=0;							/* longest line in pixels */
	m_valuetype=GUIINPUTTYPE_STRING;	/* default type is string */
	m_inputtype=GUIINPUTTYPE_STRING;	/* default type is string */
	SetUseBGColor(true);				/* default to white background for input boxes */

	m_valuetext=0;
	m_undotext=0;
	m_vscrollbar=0;
	m_hscrollbar=0;
	m_scroll=0;
	m_showcommas=false;
}

void kGUIInputBoxObj::Control(unsigned int command,KGCONTROL_DEF *data)
{
	switch(command)
	{
	case KGCONTROL_GETSKIPTAB:
		data->m_bool=m_locked;
	break;
	default:
		kGUIObj::Control(command,data);
	break;
	}
}

kGUIInputBoxObj::~kGUIInputBoxObj()
{
	if(m_hint)
		delete m_hint;
	if(m_valuetext)
		delete m_valuetext;
	if(m_undotext)
		delete m_undotext;

	if(m_scroll)
		delete m_scroll;
	if(m_vscrollbar)
		delete m_vscrollbar;
	if(m_hscrollbar)
		delete m_hscrollbar;
}

void kGUIInputBoxObj::DeleteSelection(void)
{
	m_hrange=false;
	if(m_hcursor==m_hstart)
		return;

	if(m_hcursor>m_hstart)
	{
		if(m_allowundo)
		{
			/*! @todo Handle mulitple Undo/Redo buffer */
		}
		Delete(m_hstart,m_hcursor-m_hstart);
		if(GetUseRichInfo()==true)
			DeleteRichInfo(m_hstart,m_hcursor-m_hstart);

		m_hcursor=m_hstart;
		PutCursorOnScreen();
	}
	else
	{
		if(m_allowundo)
		{
			/* todo: save in undo/redo buffer */
		}
		Delete(m_hcursor,m_hstart-m_hcursor);
		if(GetUseRichInfo()==true)
			DeleteRichInfo(m_hcursor,m_hstart-m_hcursor);
	}

	CalcLines(false);
	Dirty();
	m_hstart=m_hcursor;
}

/* calculate the line break table */
void kGUIInputBoxObj::CalcLines(bool full)
{
	int w;
	kGUICorners c;
//	int lineheight=GetHeight()+ILGAP;
	int boxheight;
	m_recalclines=false;

	m_ow=GetZoneW();
	m_oh=GetZoneH();

	GetCorners(&c);
	c.lx+=2;
	c.lx+=m_xoff;
	c.rx-=2;

	if(m_wrap==false)
	{
		if(!m_hscrollbar)
		{
			m_hscrollbar=new kGUIScrollBarObj;
			m_hscrollbar->SetHorz();
			m_hscrollbar->SetEventHandler(this,& CALLBACKNAME(ScrollMoveCol));
		}
		m_usehs=true;
	}
	else
		m_usehs=false;

	boxheight=c.by-c.ty;
	if(m_usehs)
		boxheight-=kGUI::GetSkin()->GetScrollbarHeight();
	CalcViewLines();

	/* if it fits on less than 3 lines then don't bother to show a scroll bar */
	CalcLineList(c.rx-c.lx);

	/* only show vert scroll if a large enough box */
	if(boxheight>50 && GetNumLines()>3)
	{
		if(!m_vscrollbar)
		{
			m_vscrollbar=new kGUIScrollBarObj;
			m_vscrollbar->SetVert();
			m_vscrollbar->SetEventHandler(this,& CALLBACKNAME(ScrollMoveRow));
		}
		m_usevs=true;
		c.rx-=kGUI::GetSkin()->GetScrollbarWidth();
	}
	else
		m_usevs=false;

	if((m_wrap==false) || (m_numviewlines==1))
		w=100000;
	else
		w=c.rx-c.lx;
	m_maxw=CalcLineList(w);
}

void kGUIInputBoxObj::CalcViewLines(void)
{
	int boxheight;

	boxheight=GetZoneH();
	if(m_usehs)
		boxheight-=kGUI::GetSkin()->GetScrollbarHeight();

	if(GetUseRichInfo()==false || !GetNumLines())
	{
		int lineheight=(GetLineHeight()+ILGAP);
		m_numviewlines=boxheight/lineheight;
		m_numfullviewlines=m_numviewlines;
		if((m_numviewlines*lineheight)>boxheight)
			--m_numfullviewlines;
	}
	else
	{
		unsigned int line;
		int y;

		m_numviewlines=m_numfullviewlines=GetLineNumPix(GetLineInfo(m_topoff)->ty+boxheight)-m_topoff;
		line=m_topoff+m_numviewlines;
		y=GetLineInfo(m_topoff+m_numviewlines)->by-GetLineInfo(m_topoff)->ty;
		if(y>boxheight)
			--m_numfullviewlines;
		else if((int)line==GetNumLines()-1 && m_topoff)
		{
			/* count number of lines back we can go since we have gone off the end */
			line=m_topoff-1;
			do
			{
				y+=(GetLineInfo(line)->by-GetLineInfo(line)->ty);
				if(y>boxheight)
					break;
				++m_numviewlines;
				if(!line)
					break;
				--line;
			}while(1);
		}
	}
	if(!m_numviewlines)
		m_numviewlines=1;
	if(m_numfullviewlines<1)
		m_numfullviewlines=1;
}

void kGUIInputBoxObj::PutCursorOnScreen(void)
{
	int cursorline;
	kGUIInputLineInfo *lbptr;
	int pixelx;
	int width;
	
	CallEvent(EVENT_MOVED);

	/* this event can change the string, so we need to check for recalclines here! */
	if(m_recalclines==true)
	{
		if(m_hcursor>GetLen())
			m_hcursor=GetLen();
		CalcLines(true);
	}

	cursorline=GetLineNum(m_hcursor);
	if(cursorline<0)
		return;

	lbptr=GetLineInfo(cursorline);
	if(cursorline<m_topoff)
	{
		m_topoff=cursorline;
		Dirty();
	}
	else
	{
		int boxheight=GetZoneH();
		if(m_usehs)
			boxheight-=kGUI::GetSkin()->GetScrollbarHeight();

		while((lbptr->by-GetLineInfo(m_topoff)->ty)>boxheight && m_topoff!=cursorline)
			++m_topoff;
	}
	CalcViewLines();
	/* check left / right now */

	width=GetZoneW();
	if(m_usevs)
		width-=kGUI::GetSkin()->GetScrollbarWidth();

	pixelx=GetWidthSub(lbptr->startindex,m_hcursor-lbptr->startindex);

	/* scroll left */
	if(pixelx<m_leftoff)
	{
		m_leftoff=pixelx;
		Dirty();
	}
	else if(((pixelx+24)-m_leftoff)>width)
	{
		/* scroll right */
		m_leftoff=(pixelx+24)-width;
		Dirty();
	}
}

bool kGUIInputBoxObj::MoveCursorRow(int delta)
{
	int line;
	int newline;
	int pixelx,nc;
	kGUIInputLineInfo *lbptr;
		
	if(m_recalclines==true)
		CalcLines(true);

	line=GetLineNum(m_hcursor);
	newline=line+delta;
	if(newline<0)
		newline=0;
	else if(newline>=GetNumLines())
		newline=MAX(0,GetNumLines()-1);
	
	/* already at top or bottom? */
	if(newline==line)
		return(false);	/* can't move anymore */

	lbptr=GetLineInfo(line);
	pixelx=GetWidthSub(lbptr->startindex,m_hcursor-lbptr->startindex);

	lbptr=GetLineInfo(newline);
	nc=CalcFitWidth(lbptr->startindex,lbptr->endindex-lbptr->startindex,pixelx);
	m_hcursor=lbptr->startindex+nc;
	PutCursorOnScreen();
	Dirty();
	return(true);
}

void kGUIInputBoxObj::MoveRow(int delta)
{
	m_topoff+=delta;
	CalcViewLines();
	if(m_topoff<0)
	{
		m_topoff=0;
		CalcViewLines();
	}
	else
	{
		while(m_topoff && (m_topoff+m_numviewlines)>GetNumLines())
		{
			--m_topoff;
			CalcViewLines();
		}
	}

	Dirty();
}

void kGUIInputBoxObj::MoveCol(int delta)
{
	unsigned int width=GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth();

	m_leftoff+=delta;
	if(m_leftoff+width>m_maxw)
		m_leftoff=m_maxw-width;
	if(m_leftoff<0)
		m_leftoff=0;
	Dirty();
}

void kGUIInputBoxObj::CallAfterUpdate(void)
{
	/* only if different than undotext */

	Dirty();
	m_recalclines=true;

	if(strcmp(m_undotext->GetString(),GetString()))
	{
		/* only reset the numeric value string if the field has changed */
		if(m_valuetype==GUIINPUTTYPE_INT || m_valuetype==GUIINPUTTYPE_DOUBLE)
			m_valuetext->SetString(GetString());

		m_undotext->SetString(GetString());

		/* call AA parents first as the object callback could delete the object */
		kGUI::CallAAParents(this);
		CallEvent(EVENT_AFTERUPDATE);
	}
	else
		CallEvent(EVENT_NOCHANGE);
}

void kGUIInputBoxObj::Changed(void)
{
	m_valuetype=GUIINPUTTYPE_STRING;
	Dirty();
	m_recalclines=true;
}

void kGUIInputBoxObj::SetInt(int v)
{
	Sprintf("%d",v);
}

/* save the original value but only show it using the format */
/* supplied, so for example */
/* f="%.2f",v="123.45678" */
/* shown on the screen is "123.45" */
/* if you call the function GetValueString and the user didn't change */
/* the number then you will get back "123.45678", if the user did change */
/* it then you will get back the value shown on the screen. */

void kGUIInputBoxObj::SetDouble(const char *f,const char *v)
{
	Sprintf(f,atof(v));

	if(!m_valuetext)
		m_valuetext=new kGUIString;
	m_valuetext->SetString(v);
	m_valuetext->TTZ();

	m_valuetype=GUIINPUTTYPE_DOUBLE;	/* must be done after sprintf as it set it to string type */
}

void kGUIInputBoxObj::SetDouble(const char *format,double v)
{
	Sprintf(format,v);

	if(!m_valuetext)
		m_valuetext=new kGUIString;
	m_valuetext->SetString(GetString());
	m_valuetext->TTZ();
	m_valuetype=GUIINPUTTYPE_DOUBLE;	/* must be done after Sprintf as it set it to string type */
}

void kGUIInputBoxObj::SetInt(const char *v)
{
	SetString(v);

	if(!m_valuetext)
		m_valuetext=new kGUIString;
	m_valuetext->SetString(v);
	m_valuetype=GUIINPUTTYPE_INT;
}

const char *kGUIInputBoxObj::GetValueString(void)
{
	if(m_valuetype==GUIINPUTTYPE_INT || m_valuetype==GUIINPUTTYPE_DOUBLE)
		return m_valuetext->GetString();
	else
		return kGUIText::GetString();
}

#define INPUTSCROLLDELAY (TICKSPERSEC/30)

void kGUIInputBoxObj::Activate(void)
{
	if(this!=kGUI::GetActiveObj())
	{
		kGUI::PushActiveObj(this);
		SetCurrent();

		/* save copy into undo buffer */
		if(!m_undotext)
			m_undotext=new kGUIString;
		m_undotext->SetString(GetString());

		if(!m_scroll)
		{
			m_scroll=new kGUIScroll();
			m_scroll->SetEventHandler(this,CALLBACKNAME(ScrollEvent));
		}

		if(GetNumLines())		
			m_scroll->Init(m_leftoff,GetLineInfo(m_topoff)->ty);
		else
			m_scroll->Init(m_leftoff,0);
	}
}

void kGUIInputBoxObj::DeActivate(void)
{
	if(this==kGUI::GetActiveObj())
	{
		Dirty();
		kGUI::PopActiveObj();
		CallAfterUpdate();
		delete m_undotext;
		m_undotext=0;
		if(m_scroll)
		{
			delete m_scroll;
			m_scroll=0;
		}
	}
}

bool kGUIInputBoxObj::UpdateInput(void)
{
	int key;
	int l;
	int lx,rx;
	kGUICorners c;
	kGUICorners cs;
	kGUIText text;
	int line;
	bool over;
	kGUIInputLineInfo *lbptr;
	bool used=false;
	bool callenter=false;

	/* have I been disabled by program code ( not the user )? */
	if(this==kGUI::GetActiveObj())
	{
		if(ImCurrent()==false)
			goto abort;
	}

	GetCorners(&c);
	if(kGUI::WantHint()==true && m_hint)
		kGUI::SetHintString(c.lx+10,c.ty-15,m_hint->GetString());

	if(m_recalclines==true)
		CalcLines(true);

	over=kGUI::MouseOver(&c);
	if(over==false && kGUI::GetMouseClick()==true)
	{
abort:;
		if(this==kGUI::GetActiveObj())
			DeActivate();

		PutCursorOnScreen();
		if(m_leaveselection==false)
			m_hrange=false;
		return(false);
	}

	if(kGUI::GetMouseDoubleClickLeft()==true)
	{
		SetCurrent();
		CallDoubleClick();
		return(true);
	}

	if(kGUI::GetMouseClickRight()==true)
	{
		SetCurrent();
		CallRightClick();
		return(true);
	}

	if(this!=kGUI::GetActiveObj())
	{
		if(kGUI::GetMouseClickLeft()==true || kGUI::GetKey())
		{
			SetCurrent();
			Activate();
			callenter=true;
		}
	}

	if(this==kGUI::GetActiveObj())
	{
		c.lx+=m_xoff;
		if(m_usevs==true)
		{
			int below=GetNumLines()-m_topoff-m_numviewlines;
			if(below<0)
				below=0;
			m_vscrollbar->SetValues(m_topoff,m_numviewlines,below);
		}
		if(m_usehs==true)
		{
			int width=c.rx-c.lx-kGUI::GetSkin()->GetScrollbarWidth();
			int right=m_maxw-width-m_leftoff;
			if(right<0)
				right=0;
			m_hscrollbar->SetValues(m_leftoff,width,right);
		}

		/* should we flash the cursor? */
		if(kGUI::GetDrawCursorChanged())
			Dirty();

		/* is the horizontal scroll bar on? */
		if(m_usehs==true)
		{
			if(m_hscrollbar->IsActive()==true)
				return(m_hscrollbar->UpdateInput());

			m_hscrollbar->GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
			{
				if(callenter)
					CallEvent(EVENT_ENTER);

				return(m_hscrollbar->UpdateInput());
			}
		}

		/* is the vertical scroll bar on? */
		if(m_usevs==true)
		{
			if(m_vscrollbar->IsActive()==true)
				return(m_vscrollbar->UpdateInput());

			m_vscrollbar->GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
			{
				if(callenter)
					CallEvent(EVENT_ENTER);
				return(m_vscrollbar->UpdateInput());
			}
		}
		/* is the mouse button down? */
		if(kGUI::GetMouseLeft()==true)
		{
			used=true;
			/* scroll left? */
			lx=c.lx+12;
			rx=c.rx;
			if(m_usehs)
				rx-=kGUI::GetSkin()->GetScrollbarWidth();
			if(kGUI::GetMouseX()<lx)
			{
				kGUIInputLineInfo *lbptr;
		
				line=GetLineNum(m_hcursor);
				lbptr=GetLineInfo(line);
				if(m_hcursor>lbptr->startindex)
				{
					if(m_hdelay.Update(INPUTSCROLLDELAY))
					{
						/* go back 1 character, handle multi byte character sets */
						m_hcursor-=GoBack(m_hcursor);
						Dirty();
						PutCursorOnScreen();
					}
				}
			}
			else if(kGUI::GetMouseX()>rx)
			{
				kGUIInputLineInfo *lbptr;
		
				line=GetLineNum(m_hcursor);
				lbptr=GetLineInfo(line);
				if(m_hcursor<lbptr->endindex)
				{
					if(m_hdelay.Update(INPUTSCROLLDELAY))
					{
						unsigned int x;

						/* go forward 1 character, handle multi byte character sets */
						GetChar(m_hcursor,&x);
						m_hcursor+=x;
						Dirty();
						PutCursorOnScreen();
					}
				}
			}
			else
			{
				int nc;

				if(kGUI::GetMouseY()<c.ty)
				{
					if(m_topoff)
					{
						if(m_hdelay.Update(INPUTSCROLLDELAY))
						{
							--m_topoff;
							m_hcursor=GetLineInfo(m_topoff)->startindex;
							PutCursorOnScreen();
							Dirty();
						}
					}
					return(true);
				}
				else if(kGUI::GetMouseY()>c.by)
				{
					if((m_topoff+m_numviewlines)<GetNumLines())
					{
						if(m_hdelay.Update(INPUTSCROLLDELAY))
						{
							++m_topoff;
							m_hcursor=GetLineInfo(m_topoff+m_numviewlines-1)->endindex;
							PutCursorOnScreen();
							Dirty();
						}
					}
					return(true);
				}
				else
				{
					/* this needs to handle varying height lines */
					line=GetLineNumPix(GetLineInfo(m_topoff)->ty+(kGUI::GetMouseY()-c.ty));
//					line=(kGUI::GetMouseY()-c.ty)/(GetHeight()+ILGAP)+m_topoff;
//					if(line>=GetNumLines())
//						line=GetNumLines()-1;
					lbptr=GetLineInfo(line);
					nc=CalcFitWidth(lbptr->startindex,lbptr->endindex-lbptr->startindex,(kGUI::GetMouseX()-c.lx)+m_leftoff);
					m_hcursor=lbptr->startindex+nc;
					PutCursorOnScreen();
				}
			}

			if(kGUI::GetKeyShift()==true || kGUI::GetMouseClickLeft()==true)
			{
				m_hstart=m_hcursor;
				m_hrange=false;
			}
			if(m_hstart!=m_hcursor)
				m_hrange=true;
			Dirty();
		}

		if(callenter)
			CallEvent(EVENT_ENTER);

		{
			int scroll=kGUI::GetMouseWheelDelta();
			kGUI::ClearMouseWheelDelta();
			if(scroll)
			{
				if(MoveCursorRow(-scroll)==false)
				{
					/* tried to cursor off of the field, if no changes then Deactivate */
					if(!strcmp(m_undotext->GetString(),GetString()))
					{
						DeActivate();
						return(false);	/* return false so table knows to use this key */
					}
				}
			}
		}

		key=kGUI::GetKey();
		if(key)
		{
			used=true;
			switch(key)
			{
			case GUIKEY_PGDOWN:
				/* todo: move down the number of lines currently displayed */
				MoveCursorRow(m_numfullviewlines);
			break;
			case GUIKEY_DOWN:
				if(MoveCursorRow(1)==false)
				{
					if(m_allowcursorexit==true)
						goto exitcell;
				}
			break;
			case GUIKEY_PGUP:
				/* todo: move up the number of lines currently displayed */
				MoveCursorRow(-m_numfullviewlines);
			break;
			case GUIKEY_UP:
				if(MoveCursorRow(-1)==false)
				{
					if(m_allowcursorexit==true)
						goto exitcell;
				}
			break;
			case GUIKEY_LEFT:
				if(m_hcursor>0)
				{
					/*handle multi byte strings like UTF-8 etc. */
					m_hcursor-=GoBack(m_hcursor);
					Dirty();
				}
			break;
			case GUIKEY_RIGHT:
				if(m_hcursor<(GetLen()))
				{
					/*handle multi byte strings like UTF-8 etc. */
					unsigned int x;

					GetChar(m_hcursor,&x);
					m_hcursor+=x;
					Dirty();
				}
			break;
			case GUIKEY_HOME:
				if(kGUI::GetKeyControl()==true)
				{
					if(m_hcursor>0)
					{
						m_hcursor=0;
						Dirty();
					}
				}
				else
				{
					/* beginning of current line */
					kGUIInputLineInfo *lbptr;

					lbptr=GetLineInfo(GetLineNum(m_hcursor));
					if(m_hcursor!=lbptr->startindex)
					{
						m_hcursor=lbptr->startindex;
						Dirty();
					}
				}
			break;
			case GUIKEY_F2:
			case GUIKEY_END:
				if(kGUI::GetKeyControl()==true)
				{
					unsigned int el=GetLen();
					if(m_hcursor!=el)
					{
						m_hcursor=el;
						Dirty();
					}
				}
				else
				{
					/* end of current line */
					kGUIInputLineInfo *lbptr;

					lbptr=GetLineInfo(GetLineNum(m_hcursor));
					if(m_hcursor!=lbptr->endindex)
					{
						m_hcursor=lbptr->endindex;
						Dirty();
					}
				}
			break;
			case GUIKEY_TAB:
				if(m_allowtab==true)
				{
					key='\t';
					goto keypressed;
				}
				/* fall through */
			case GUIKEY_SHIFTTAB:
exitcell:		m_leftoff=0;
				m_hcursor=0;
				m_hstart=0;
				m_hrange=false;
				PutCursorOnScreen();
				DeActivate();
				return(false);	/* return false so table knows to use this key */
			break;
			case GUIKEY_ESC:	/* undo */
				m_leftoff=0;
				m_hcursor=0;
				m_hstart=0;
				m_hrange=false;
				m_recalclines=true;
				SetString(m_undotext->GetString());
				PutCursorOnScreen();
				kGUI::ClearKey();
				DeActivate();
				return(true);
			break;
			case GUIKEY_SELECTALL:
				SelectAll();
				return(true);
			break;
			case GUIKEY_RETURN:
				if(m_allowenter==true)
					goto keypressed;

				m_leftoff=0;
				m_hcursor=0;
				m_hstart=0;
				m_hrange=false;
				PutCursorOnScreen();
				kGUI::ClearKey();
				DeActivate();
				CallEvent(EVENT_PRESSRETURN);
				return(true);
			break;
			case GUIKEY_DELETE:
				if(m_locked==false)
				{
					if(m_hrange==true)
						DeleteSelection();
					else if(m_hcursor<GetLen())
					{
						unsigned int nb;

						GetChar(m_hcursor,&nb);
						if(m_allowundo)
						{
							/* todo: save in undo/redo buffer */

						}
						/* handle multi-byte character sets */
						Delete(m_hcursor,nb);

						if(GetUseRichInfo()==true)
							DeleteRichInfo(m_hcursor,nb); 
						CalcLines(false);
						Dirty();
					}
				}
			break;
			case GUIKEY_BACKSPACE:
				if(m_locked==false)
				{
					if(m_hrange==true)
						DeleteSelection();
					else if(m_hcursor>0)
					{
						unsigned int nb;

						/* handle multi byte character sets */
						nb=GoBack(m_hcursor);
						if(m_allowundo)
						{
							/* todo: save in undo/redo buffer */

						}

						Delete(m_hcursor-nb,nb);
						if(GetUseRichInfo()==true)
							DeleteRichInfo(m_hcursor-nb,nb);
						Dirty();
						CalcLines(false);
						m_hcursor-=nb;
					}
					m_hstart=m_hcursor;
				}
			break;
			case GUIKEY_COPY:
				/* should we disable this if this is a password box? */
				if(m_hrange==true)
				{
					kGUIString paste;
					int start,end;

					start=MIN(m_hstart,m_hcursor);
					end=MAX(m_hstart,m_hcursor);
					if(end>start)
					{
						paste.SetString(GetString()+start,end-start);
						paste.SetEncoding(GetEncoding());
						kGUI::Copy(&paste);
					}
				}
			break;
			case GUIKEY_CUT:
				/* should we disable this if this is a password box? */
				if(m_hrange==true)
				{
					kGUIString paste;
					int start,end;

					start=MIN(m_hstart,m_hcursor);
					end=MAX(m_hstart,m_hcursor);
					if(end>start)
					{
						paste.SetString(GetString()+start,end-start);
						paste.SetEncoding(GetEncoding());
						kGUI::Copy(&paste);
						DeleteSelection();
					}
				}
			break;
			case GUIKEY_UNDO:
				//todo
			break;
			case GUIKEY_PASTE:
			{
				if(m_locked==false)
				{
					unsigned int ci;
					unsigned int nb;
					kGUIString paste;

					kGUI::Paste(&paste);

					if(m_hrange==true)
						DeleteSelection();
					
					paste.Replace("\r","");	/* remove c/r, only accept linefeeds */

					if(m_allowenter==false)
						paste.Replace("\n","");	/* remove c/r */

					if(m_allowtab==false)
						paste.Replace("\t","");	/* remove tabs */

					/* handle different string types */
					if(GetEncoding()!=paste.GetEncoding())
					{
						int charindex;

						/* convert both strings to UTF-8, then merge */
						/* convert character offset to a cursor position, and then back after change encoding */
						charindex=CursorToIndex(m_hcursor);
						ChangeEncoding(ENCODING_UTF8);
						m_hcursor=IndexToCursor(charindex);
						paste.ChangeEncoding(ENCODING_UTF8);
					}

					/* check all characters in the paste string and remove any */
					/* invalid characters */

					ci=0;
					while(ci<paste.GetLen())
					{
						key=paste.GetChar(ci,&nb);
						if(CheckInput(key)==false)
							paste.Delete(ci,nb);
						else
							ci+=nb;
					}
	
					if(paste.GetLen())
					{
						if(m_allowundo==true)
						{
							/* save in undo buffer */
						}

						Insert(m_hcursor,paste.GetString());
						if(GetUseRichInfo()==true)
							InsertRichInfo(m_hcursor,paste.GetLen());
						m_hcursor+=paste.GetLen();

						/* is new size too long? */
						if(m_maxlen>0)
						{
							if(GetLen()>(unsigned int)m_maxlen)
							{
								if(m_allowundo==true)
								{
									/* save in undo buffer */
								}
								if(GetUseRichInfo()==true)
									DeleteRichInfo(m_maxlen,GetLen()-m_maxlen);
								Clip((unsigned int)m_maxlen);
							}
						}
						if(m_hcursor>GetLen())
							m_hcursor=GetLen();
						CalcLines(false);
						PutCursorOnScreen();
						Dirty();
						m_hrange=false;
						m_hstart=m_hcursor;
					}
				}
			}
			break;
			case GUIKEY_FIND:
			case GUIKEY_REPLACE:
				if(m_allowfind)
				{
					kGUISearchReq::Open(this,this);
					m_forceshowselection=true;
				}
			break;
			default:
				/* insert a letter into the string */
keypressed:		if(m_locked==false)
				{
					if((key==10) || (key==13) || (key==GUIKEY_RETURN))	/* control return inside a string */
						key=10;	/* "\n" */
					
					if(CheckInput(key)==true)
					{
						/* any selected area to delete? */
						if(m_hrange==true)
							DeleteSelection();
						l=GetLen();
						if(l<m_maxlen || (m_maxlen<0))	/* -1 == no limit */
						{
							/* handle multi byte character sets */
							kGUIString kk;
							unsigned int nb;

							kk.Append((char)key);

							/* make encoding for typed character match the strings encoding */
							kk.ChangeEncoding(GetEncoding());
							Insert(m_hcursor,kk.GetString());

							GetChar(m_hcursor,&nb);
							if(GetUseRichInfo()==true)
								InsertRichInfo(m_hcursor,nb);
							m_hcursor+=nb;
							CalcLines(false);
							Dirty();
						}
						m_hrange=false;
						m_hstart=m_hcursor;
						PutCursorOnScreen();
					}
				}
			break;
			}
			kGUI::ClearKey();

			if(kGUI::GetKeyShift()==true)
				m_hrange=true;
			else
			{
				m_hrange=false;
				m_hstart=m_hcursor;
			}
			/* make sure cursor is onscreen, scroll if it is not */
			PutCursorOnScreen();
		}
	}
	else if(ImCurrent())
	{
		/* should we flash the cursor? */
		if(kGUI::GetDrawCursorChanged())
			Dirty();
	}
	return(used);
}

bool kGUIInputBoxObj::CheckInput(int key)
{
	switch(m_inputtype)
	{
	case GUIINPUTTYPE_STRING:
		if((key==10) || (key=='\t') || (key>=' ' && key<=255) || (key>='a' && key<='z') || (key>='A' && key<='Z') )
			return(true);
	break;
	case GUIINPUTTYPE_INT:
		if((key>='0' && key<='9') || (key=='-') )
			return(true);
	break;
	case GUIINPUTTYPE_DOUBLE:
		if((key>='0' && key<='9') || (key=='.') || (key=='$') || (key=='-') || (key=='e'))
			return(true);
	break;
	}
	return(false);
}

void kGUIInputBoxObj::Draw(void)
{
	int i,x;
	int hs,he,w;
	unsigned int schar;
	unsigned int echar;
	int llen;
	kGUICorners cc;
	kGUICorners c;
	bool imactive;
	kGUIInputLineInfo *lbptr;
	int h,y;
	kGUIZone sz;
	bool drawcursor;
	int topoff,leftoff;
	kGUIString save;
	kGUIInputLineInfo saveli;
	unsigned int hc=m_hcursor;

	if((m_recalclines==true) || (m_ow!=GetZoneW()) || (m_oh!=GetZoneH()) )
		CalcLines(true);

//	m_forceshowselection

	drawcursor=false;
	imactive=(this==kGUI::GetActiveObj());
	if(!imactive && m_leaveselection==false && m_forceshowselection==false)
	{
		if(ImCurrent())
		{
			if(GetLen())
			{
				m_hrange=true;
				m_hstart=GetLen();
				m_hcursor=0;
			}
			else
			{
				/* since there is no text to hilight, draw the cursor instead */
				drawcursor=kGUI::GetDrawCursor()==true;
				m_hrange=false;
				m_hstart=0;
				m_hcursor=0;
			}
		}
		else
		{
			m_hrange=false;
			m_hstart=m_hcursor;
		}
	}
	else
	{
		if(kGUI::GetDrawCursor()==true && (m_hrange==false))
			drawcursor=true;
	}
	GetCorners(&cc);	/* get topcorner */

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(m_hrange==false)
		{
			hs=-1;
			he=-1;
		}
		else if(m_hstart<m_hcursor)
		{
			hs=m_hstart;
			he=m_hcursor;
		}
		else
		{
			hs=m_hcursor;
			he=m_hstart;
		}

		if(m_password==true)
		{
			unsigned int cpos;

			/* save string and put back after drawing */
			save.SetString(this);
			lbptr=GetLineInfo(0);
			saveli=*(lbptr);

			/* replace all chars with asterisks */
			for(cpos=0;cpos<GetLen();++cpos)
				SetChar(cpos,'*');

			CalcLineList(GetZoneW());
		}
		else if(m_showcommas)
		{
			int cpos;
			int slen;

			/* I am assuming that the string is numbers only and on a single line */
			/* at this point I am also assuming 8 bit characters since they should all be */
			/* 0-9 digits only */
			/* todo: make handle negative numbers */

			/* save string and put back after drawing */
			save.SetString(this);
			lbptr=GetLineInfo(0);
			saveli=*(lbptr);

			if(GetChar(0)=='-')
				slen=1;
			else
				slen=0;

			cpos=GetLen()-3;
			while(cpos>slen)
			{
				Insert(cpos,",");
				if(hc>=(unsigned int)cpos)
					hc+=1;
				if(hs>=cpos)
					hs+=1;
				if(he>=cpos)
					he+=1;
				cpos-=3;
			}
			CalcLineList(GetZoneW());
		}

		if(GetUseBGColor())
			kGUI::DrawRectBevelIn(c.lx,c.ty,c.rx,c.by,GetBGColor());
		if(imactive==true || m_leavescroll)
		{
			if(m_usevs==true)
			{
				sz.SetZone(c.rx-kGUI::GetSkin()->GetScrollbarWidth(),c.ty,kGUI::GetSkin()->GetScrollbarWidth(),c.by-c.ty);
				m_vscrollbar->MoveZone(&sz);
				m_vscrollbar->Draw();
			}
			if(m_usehs==true)
			{
				sz.SetZone(c.lx,c.by-kGUI::GetSkin()->GetScrollbarHeight(),(c.rx-c.lx)-kGUI::GetSkin()->GetScrollbarWidth(),kGUI::GetSkin()->GetScrollbarHeight());
				m_hscrollbar->MoveZone(&sz);
				m_hscrollbar->Draw();
			}
			if(m_usehs==true || m_usevs==true)
			{
				if(m_usevs)
					c.rx-=kGUI::GetSkin()->GetScrollbarWidth();
				if(m_usehs)
					c.by-=kGUI::GetSkin()->GetScrollbarHeight();
			}
		}

		c.ty+=IEDGE;
		c.lx+=IEDGE;
		c.lx+=m_xoff;
		kGUI::ShrinkClip(&c);

		h=GetLineHeight()+ILGAP;
		y=c.ty;

		if(m_scroll)
		{
			int pixely;

			m_scroll->SetDest(m_leftoff,GetLineInfo(m_topoff)->ty);
			
			/* since scrolling is still happening get the actual topoff */
			leftoff=m_scroll->GetCurrentX();
			pixely=m_scroll->GetCurrentY();

			/* get the line num at pixel down position y */
			topoff=GetLineNumPix(pixely);
			y-=pixely-GetLineInfo(topoff)->ty;
		}
		else
		{
			topoff=m_topoff;
			leftoff=m_leftoff;
		}

		SetRevRange(hs,he);
		x=c.lx-leftoff;
		for(i=topoff;i<GetNumLines();++i)
		{
			lbptr=GetLineInfo(i);

			schar=lbptr->startindex;
			echar=lbptr->endindex;
			llen=echar-schar;
			if(lbptr->hardbreak==false)
				--echar;

			DrawSection(schar,llen,x,x,y,lbptr->pixheight);	

			/* is the cursor on this line? */
			if((hc>=schar) && (hc<=echar))
			{
				if(drawcursor==true)
				{
					w=GetWidthSub(schar,hc-schar);
					if(drawcursor==true)
						kGUI::DrawRect(x+w,y,x+w+3,y+lbptr->pixheight-4,GetColor());
				}
			}
			y+=lbptr->pixheight+2;
			if(y>c.by)
				break;
		}

		if(m_showcommas || m_password)
		{
			/* since string was changed, put it back now */
			SetString(&save);
			*(GetLineInfo(0))=saveli;
		}
	}
	kGUI::PopClip();
}		

void kGUIInputBoxObj::GetCursorRange(unsigned int *si,unsigned int *ei)
{
	if(m_hrange==false)
	{
		*(si)=m_hcursor;
		*(ei)=m_hcursor+1;
	}
	else if(m_hstart<m_hcursor)
	{
		*(si)=m_hstart;
		*(ei)=m_hcursor;
	}
	else
	{
		*(si)=m_hcursor;
		*(ei)=m_hstart;
	}
}

void kGUIInputBoxObj::StringSearch(kGUIString *from,bool matchcase,bool matchword)
{
	unsigned int c=m_hcursor+1;
	bool wrap=false;
	int newc;

	if(c>=GetLen())
	{
		c=0;
		wrap=true;
	}

	while(1)
	{
		newc=Str(from->GetString(),matchcase,matchword,c);
		if(newc>=0)
		{
			m_hcursor=newc;
			m_hstart=newc+from->GetLen();
			m_hrange=true;
			PutCursorOnScreen();
			Dirty();
			return;
		}
		if(wrap)
			return;
		c=0;
		wrap=true;
	}
}

void kGUIInputBoxObj::StringReplace(kGUIString *from,bool matchcase,bool matchword,kGUIString *to)
{
	unsigned int c=m_hcursor;
	bool wrap=false;
	int newc;

	if(c>=GetLen())
	{
		c=0;
		wrap=true;
	}

	while(1)
	{
		newc=Str(from->GetString(),matchcase,matchword,c);
		if(newc>=0)
		{
			Replace(from->GetString(),to->GetString(),newc,matchcase,1);

			m_hcursor=newc;
			m_hstart=newc+to->GetLen();
			m_hrange=true;
			PutCursorOnScreen();
			Dirty();
			return;
		}
		if(wrap)
			return;
		c=0;
		wrap=true;
	}
}

/***********************************************************/
kGUIScrollInputBoxObj::kGUIScrollInputBoxObj()
{
	SetHAlign(FT_CENTER);
	SetVAlign(FT_MIDDLE);
}

bool kGUIScrollInputBoxObj::UpdateInput(void)
{
	bool used;
	int x,w;

	x=GetZoneX();w=GetZoneW();
	MoveZoneX(x+16);MoveZoneW(w-32);
	used=kGUIInputBoxObj::UpdateInput();
	MoveZoneX(x);MoveZoneW(w);
	if(used==false)
	{
		if(kGUI::GetMouseClickLeft()==true)
		{
			kGUICorners c;
			kGUIEvent e;

			int w=kGUI::GetSkin()->GetScrollHorizButtonWidths();

			GetCorners(&c);
			if(kGUI::GetMouseX()<(c.lx+w))
			{
				e.m_value[0].i=-1;					
				CallEvent(EVENT_PRESSED,&e);	/* left arrow button was pressed */
				kGUI::CallAAParents(this);
				used=true;
			}
			else if(kGUI::GetMouseX()>(c.rx-w))
			{
				e.m_value[0].i=1;					
				CallEvent(EVENT_PRESSED,&e);	/* right arrow button was pressed */
				kGUI::CallAAParents(this);
				used=true;
			}
		}
	}
	return(used);
}

void kGUIScrollInputBoxObj::Draw(void)
{
	kGUICorners c;
	int x,w;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::GetSkin()->DrawScrollHoriz(&c);

		x=GetZoneX();w=GetZoneW();
		MoveZoneX(x+16);MoveZoneW(w-32);
		kGUIInputBoxObj::Draw();
		MoveZoneX(x);MoveZoneW(w);
	}
	kGUI::PopClip();
}
