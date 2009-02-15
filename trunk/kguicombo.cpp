/*********************************************************************************/
/* kGUI - kguicombo.cpp                                                          */
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

kGUIComboBoxObj::kGUIComboBoxObj()
{
	m_allowtyping=false;	/* default to only pulldown, no typing */
	m_editmode=COMBOEDIT_NONE;
	m_hint=0;
	m_colormode=false;
	m_locked=false;
	m_numentries=0;
	m_type=COMBOTYPE_NUM;	/* default type */
	m_selection=0;
	m_poptableentries=0;
	m_poptable=0;
	m_typedstring=0;		/* only allocate it as it is needed */
	m_popped=false;
}

kGUIText *kGUIComboBoxObj::GetEntryTextPtr(unsigned int index)
{
	return m_poptableentries[index]->GetText();
}

void kGUIComboBoxObj::SetColorMode(unsigned int width)
{
	assert(m_poptableentries==0,"Need to set this (SetColorMode) before setting the number of entries!");
	m_colorcolwidth=width;
	m_colormode=true;
}

void kGUIComboBoxObj::SetColorBox(unsigned int index,kGUIColor c)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");
	m_poptableentries[index]->SetBox(m_colorcolwidth,c);
}

kGUIComboBoxObj::~kGUIComboBoxObj()
{
	unsigned int i;
	assert(m_poptable==0,"Table not deleted error!");

	if(m_hint)
		delete m_hint;

	if(m_poptableentries)
	{
		for(i=0;i<m_numentries;++i)
			delete m_poptableentries[i];
		delete []m_poptableentries;
	}
}

void kGUIComboBoxObj::SetNumEntries(unsigned int n)
{
	unsigned int i;

	if(m_poptableentries)
	{
		for(i=0;i<m_numentries;++i)
			delete m_poptableentries[i];
		delete []m_poptableentries;
	}
	m_numentries=n;
	m_selection=0;
	if(n)
		m_poptableentries=new kGUIComboTableRowObj *[n];
	else
		m_poptableentries=0;

	for(i=0;i<n;++i)
	{
		m_poptableentries[i]=new kGUIComboTableRowObj(m_colormode==true?2:1);
	}
	Dirty();
}


void kGUIComboBoxObj::SetEntry(unsigned int index,const char *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIComboBoxObj::SetEntry(unsigned int index,const char *entryname,const char *entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(index);
	m_poptableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIComboBoxObj::SetEntry(unsigned int index,kGUIString *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIComboBoxObj::SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(index);
	m_poptableentries[index]->SetValue(entryval);
	Dirty();
}

void kGUIComboBoxObj::RenameEntry(const char *oldname,const char *newname)
{
	unsigned int i;
	for(i=0;i<m_numentries;++i)
	{
		if(!strcmp(m_poptableentries[i]->GetString(),oldname))
		{
			m_poptableentries[i]->SetString(newname);
			if(m_selection==i)
				Dirty();
			return;
		}
	}
	assert(false,"name not found error!");
}

void kGUIComboBoxObj::SetSelection(int s)
{
	if(SetSelectionz(s)==false)
	{
		assert(false,"Selection number not found!");
	}
}

bool kGUIComboBoxObj::SetSelectionz(int s)
{
	unsigned int i;

	for(i=0;i<m_numentries;++i)
	{
		if(m_poptableentries[i]->GetValue()==s)
		{
			m_type=COMBOTYPE_NUM;
			m_selection=i;
			Dirty();
			return(true);
		}
	}
	return(false);	/* not found! return without an error */
}

void kGUIComboBoxObj::SetSelection(const char *string)
{
	if(SetSelectionz(string)==false)
		passert(false,"Selection '%s' not in list!",string);
}

bool kGUIComboBoxObj::SetSelectionz(const char *string)
{
	unsigned int i;

	m_type=COMBOTYPE_STRING;
	for(i=0;i<m_numentries;++i)
	{
		if(!stricmp(m_poptableentries[i]->GetString(),string))
		{
			m_selection=i;
			Dirty();
			return(true);
		}
	}
	return(false);
}

void kGUIComboBoxObj::SetSelectionString(const char *string)
{
	if(SetSelectionStringz(string)==false)
		passert(false,"Selection '%s' not in list!",string);
}

bool kGUIComboBoxObj::SetSelectionStringz(const char *string)
{
	unsigned int i;

	m_type=COMBOTYPE_STRING;
	for(i=0;i<m_numentries;++i)
	{
		assert(m_poptableentries[i]->GetIsTextValue()==true,"Error: entry doesn't have a text value set!");

		if(!stricmp(m_poptableentries[i]->GetTextValue()->GetString(),string))
		{
			m_selection=i;
			Dirty();
			return(true);
		}
	}
	return(false);
}


kGUIString *kGUIComboBoxObj::GetSelectionStringObj(void)
{
	assert(m_selection<m_numentries,"kGUIComboBoxObj: index too large");

	if(m_poptableentries[m_selection]->GetIsTextValue()==true)
		return(m_poptableentries[m_selection]->GetTextValue());
	else
		return(m_poptableentries[m_selection]->GetText());
}

/* return the widest entry in screen pixels */
int kGUIComboBoxObj::GetWidest(void)
{
	unsigned int i;
	int w,widest;

	widest=0;
	for(i=0;i<m_numentries;++i)
	{
		/* extra space is added to account for the table edge bevels */
		/* and the scrollbar on the right */
		w=m_poptableentries[i]->GetWidth()+kGUI::GetSkin()->GetScrollbarWidth()+10;
		if(w>widest)
			widest=w;
	}
	return(widest);
}


int kGUIComboBoxObj::GetSelection(void)
{
	return m_poptableentries[m_selection]->GetValue();
}

void kGUIComboBoxObj::SelectionDone(kGUIEvent *e)
{
	switch(e->GetEvent())
	{
	case EVENT_SELECTED:
		m_popped=false;

		if(m_poptable->GetSelected()>=0)
			m_selection=m_poptable->GetSelected();

		kGUI::DelWindow(m_poptable);
		delete m_poptable;
		m_poptable=0;
		SetCurrent();	/* I am now the top current object */
		Dirty();
		if(m_selection!=m_undoselection)
		{
			CallEvent(EVENT_AFTERUPDATE);
			kGUI::CallAAParents(this);
		}
		else
			CallEvent(EVENT_NOCHANGE);
		kGUI::PushActiveObj(this);
	break;
	}
}

/* returning true means I've used the input, false means pass input to someone else */

bool kGUIComboBoxObj::UpdateInput(void)
{
	unsigned int i;
	bool over;
	kGUICorners c;
	kGUIText text;
	int showentries;
	int key,popx,popy,popw,poph;
	bool usedkey=false;

	if(m_popped==true)
		return(false);

	GetCorners(&c);
	over=kGUI::MouseOver(&c);
	if(over==false && this!=kGUI::GetActiveObj())
		return(false);

	if(over==false && kGUI::GetMouseClick()==true)
	{
		if(this==kGUI::GetActiveObj())
		{
			if(m_typedstring)
			{
				delete m_typedstring;
				m_typedstring=0;
				if(m_selection!=m_undoselection)
				{
					CallEvent(EVENT_AFTERUPDATE);
					kGUI::CallAAParents(this);
				}
				else
					CallEvent(EVENT_NOCHANGE);
			}
			kGUI::PopActiveObj();
		}
		return(false);
	}

	if(kGUI::WantHint()==true && m_hint)
		kGUI::SetHintString(c.lx+10,c.ty-15,m_hint->GetString());

	if(this==kGUI::GetActiveObj())
	{
		switch(m_editmode)
		{
		case COMBOEDIT_POPLIST:
			/* this is only called so after the menu is closed by clicking */
			/* on the combo box that it doesn't  pop up again immediately */
			kGUI::PopActiveObj();		
		break;
		case COMBOEDIT_USERTYPE:
		{
			if(kGUI::GetMouseClickLeft()==true)
			{
				delete m_typedstring;
				m_typedstring=0;
				if(m_selection!=m_undoselection)
				{
					CallEvent(EVENT_AFTERUPDATE);
					kGUI::CallAAParents(this);
				}
				else
					CallEvent(EVENT_NOCHANGE);
				kGUI::PopActiveObj();
				goto click;
			}

addkey:;
			if(kGUI::GetDrawCursorChanged())
				Dirty(&c);

			/* user is going to type and use their typing to select a */
			/* match from the combo entries */
			key=kGUI::GetKey();
			if(key)
			{
				switch(key)
				{
				case GUIKEY_ESC:
					m_selection=m_undoselection;	/* undo and fall through */
				case GUIKEY_RETURN:
					kGUI::ClearKey();
					delete m_typedstring;
					m_typedstring=0;
					if(m_selection!=m_undoselection)
					{
						CallEvent(EVENT_AFTERUPDATE);
						kGUI::CallAAParents(this);
					}
					else
						CallEvent(EVENT_NOCHANGE);

					kGUI::PopActiveObj();
					Dirty();
					return(true);
				break;
				case GUIKEY_TAB:
				case GUIKEY_SHIFTTAB:
					delete m_typedstring;
					m_typedstring=0;
					if(m_selection!=m_undoselection)
					{
						CallEvent(EVENT_AFTERUPDATE);
						kGUI::CallAAParents(this);
					}
					else
						CallEvent(EVENT_NOCHANGE);

					kGUI::PopActiveObj();
					Dirty();
					return(false);	/* pass tab to parent object */
				break;
				case GUIKEY_DELETE:
				case GUIKEY_BACKSPACE:
				case GUIKEY_LEFT:
					usedkey=true;
					if(m_typedstring->GetLen())
						m_typedstring->Clip(m_typedstring->GetLen()-1);
				break;
				default:
					if((key>=' ') || (key>='a' && key<='z') || (key>='A' && key<='Z'))
					{
						m_typedstring->Append((unsigned int)key);
						usedkey=true;
					}
				break;
				}
				if(usedkey==true)
				{
					Dirty();
					kGUI::ClearKey();
					if(m_typedstring->GetLen()>0)
					{
						unsigned int s;
						int bests=-1;

						/* find first match alphabetically, if none, then delete typed character */
						kGUI::ClearKey();
						for(s=0;s<m_numentries;++s)
						{
							if(!strcmpin(m_poptableentries[s]->GetString(),m_typedstring->GetString(),m_typedstring->GetLen()))
							{
								if(bests==-1)
									bests=s;
								else
								{
									if(stricmp(m_poptableentries[s]->GetString(),m_poptableentries[bests]->GetString())<0)
										bests=s;
								}
							}
						}
						if(bests==-1)
						{
							if(m_typedstring->GetLen())
								m_typedstring->Clip(m_typedstring->GetLen()-1);
						}
						else
							m_selection=bests;
					}
					else
						m_selection=m_undoselection;	/* if nothing typed then go back to initial selection */
				}
			}
		}
		break;
		}
	}
	else if(m_locked==false)
	{
		if(kGUI::GetMouseClickLeft()==true)
		{
click:;
			/* decide if they clicked on the arrow or on the text area */
			int cw=kGUI::GetSkin()->GetComboArrowWidth();

			SetCurrent();
			CallEvent(EVENT_ENTER);
			m_undoselection=m_selection;

			if((kGUI::GetMouseX()<(c.rx-cw)) && (m_allowtyping==true))
			{
				m_editmode=COMBOEDIT_USERTYPE;
				assert(m_typedstring==0,"Error typed string already allocated!");
				m_typedstring=new kGUIString();
				kGUI::PushActiveObj(this);
				goto addkey;
			}
			else
			{
				m_editmode=COMBOEDIT_POPLIST;

				/* calculate position of popup selector table */
				showentries=m_numentries;
				if(showentries>MAXSHOWCOMBO)
					showentries=MAXSHOWCOMBO;
	//			text.SetFontInfo(&m_fontinfo);
				text.SetFontInfo(this);
				popx=c.lx;
				popw=GetZoneW();
				popy=c.ty+GetZoneH();
				poph=((text.GetLineHeight()+6)*showentries)+4;

				/* if this will go off of the bottom of the screen (with a little space at the bottom) */
				/* then move it above instead */

				if((popy+poph)>(kGUI::GetSurfaceHeight()-16))
					popy=(kGUI::GetSurfaceHeight()-16)-poph;

				/* ditto for right */
				if((popx+popw)>kGUI::GetSurfaceWidth())
					popx=kGUI::GetSurfaceWidth()-popw;

				Dirty();

				assert(m_poptable==0,"Already allocated error!");
				m_poptable = new kGUITableObj;
				m_poptable->NoColHeaders();
				m_poptable->NoRowHeaders();
				m_poptable->NoColScrollbar();
				if(m_colormode==true)
					m_poptable->SetNumCols(2);
				else
					m_poptable->SetNumCols(1);
				m_poptable->SetEventHandler(this,CALLBACKNAME(SelectionDone));
//				m_poptable->SetSelectedCallBack(this,CALLBACKNAME(SelectionDone));
				m_poptable->SetSelectMode();
				/* add entries here */
				for(i=0;i<m_numentries;++i)
				{
					m_poptableentries[i]->SetRowHeight(m_poptableentries[i]->GetHeight()+4);
					m_poptable->AddRow(m_poptableentries[i]);
					m_poptable->SetEntryEnable(i,true);
				}

				m_poptable->SetZone(popx,popy,popw,poph);
				m_poptable->SizeDirty();
				m_poptable->LockCol();	/* don't allow cursor left/right */
				if(m_colormode==false)
					m_poptable->SetColWidth(0,popw);
				else
				{
					m_poptable->SetColWidth(0,popw-m_colorcolwidth);
					m_poptable->SetColWidth(1,m_colorcolwidth);
				}
				m_poptable->Dirty();

				m_popped=true;
				kGUI::AddWindow(m_poptable);
				kGUI::PushActiveObj(m_poptable);

				m_poptable->GotoRow(m_selection);

				m_poptable->ClearOver();
				return(true);
			}
		}
	}
	return(true);
}

#define COMBOTEXTEDGEW 6
#define COMBOTEXTEDGEH 3

void kGUIComboBoxObj::Draw(void)
{
	int h;
	kGUICorners c;
	kGUIText *text;
	kGUIColor tc;
	int cw;

	if(!m_poptableentries || !m_numentries)
		return;	/* not allocated */

	GetCorners(&c);
	text=m_poptableentries[m_selection]->GetText();
	h=text->GetLineHeight()+4;

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRectBevelIn(c.lx,c.ty,c.rx,c.by);
		if(m_locked==true)
			tc=DrawColor(172,168,153);
		if(!ImCurrent())
			tc=text->GetColor();
		else
		{
			kGUI::DrawRect(c.lx+2,c.ty+2,c.lx+6+text->GetWidth(),c.by-2,DrawColor(16,16,16));
			tc=DrawColor(255,255,255);
		}

		/* is the user typing? */
		if(m_typedstring)
		{
			unsigned int tl;
			int tlx;

			tl=m_typedstring->GetLen();
			tlx=text->GetWidthSub(0,tl);

			kGUI::DrawRect(c.lx+2,c.ty+2,c.lx+COMBOTEXTEDGEW+tlx,c.by-2,DrawColor(255,255,255));
			text->DrawSection(0,tl,0,c.lx+COMBOTEXTEDGEW,c.ty+COMBOTEXTEDGEH,text->GetLineHeight(),DrawColor(16,16,16));

			/* draw cursor */
			kGUI::DrawRect(c.lx+COMBOTEXTEDGEW+tlx,c.ty+2,c.lx+COMBOTEXTEDGEW+tlx+2,c.by-2,kGUI::GetDrawCursor()?DrawColor(255,255,255):DrawColor(0,0,0));

			if(tl<text->GetLen())
			{
				kGUI::DrawRect(c.lx+COMBOTEXTEDGEW+tlx+2,c.ty+2,c.lx+COMBOTEXTEDGEW+text->GetWidth(),c.by-2,DrawColor(16,16,16));
				text->DrawSection(tl,text->GetLen(),0,c.lx+COMBOTEXTEDGEW+tlx,c.ty+COMBOTEXTEDGEH,text->GetLineHeight(),DrawColor(255,255,255));
			}
		}
		else
			text->Draw(c.lx+COMBOTEXTEDGEW,c.ty+COMBOTEXTEDGEH,0,0,tc);

		cw=kGUI::GetSkin()->GetComboArrowWidth();
		if(m_colormode)
			kGUI::DrawRect(c.rx-m_colorcolwidth-cw-3,c.ty+1,c.rx-cw-3,c.by-1,m_poptableentries[m_selection]->GetBoxColor());

		c.lx=c.rx-cw;
		kGUI::GetSkin()->DrawComboArrow(&c);
	}
	kGUI::PopClip();
}

/**********************************************************************/
/* shared combo box object */

kGUISharedComboboxObj::kGUISharedComboboxObj()
{
	m_locked=false;
	m_popped=false;
	m_selection=0;
	m_shared=0;		/* shared info has not been attached yet */
	m_typedstring=0;		/* only allocate it as it is needed */
	m_editmode=COMBOEDIT_NONE;
}

kGUISharedComboEntries::kGUISharedComboEntries()
{
	m_allowtyping=false;		/* default to only pulldown, no typing */
	m_hint=0;
	m_colormode=false;
	m_numentries=0;
	m_type=COMBOTYPE_NUM;	/* default type */
	m_poptableentries=0;
	m_poptable=0;
}

void kGUISharedComboEntries::CloseTable(void)
{
	kGUI::DelWindow(m_poptable);
	delete m_poptable;
	m_poptable=0;
}

kGUIText *kGUISharedComboEntries::GetEntryTextPtr(unsigned int index)
{
	return m_poptableentries[index]->GetText();
}

void kGUISharedComboEntries::SetColorMode(unsigned int width)
{
	assert(m_poptableentries==0,"Need to set this (SetColorMode) before setting the number of entries!");
	m_colorcolwidth=width;
	m_colormode=true;
}

void kGUISharedComboEntries::SetColorBox(unsigned int index,kGUIColor c)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");
	m_poptableentries[index]->SetBox(m_colorcolwidth,c);
}

kGUISharedComboEntries::~kGUISharedComboEntries()
{
	unsigned int i;
	assert(m_poptable==0,"Table not deleted error!");

	if(m_hint)
		delete m_hint;

	if(m_poptableentries)
	{
		for(i=0;i<m_numentries;++i)
			delete m_poptableentries[i];
		delete []m_poptableentries;
	}
}

void kGUISharedComboEntries::SetNumEntries(unsigned int n)
{
	unsigned int i;

	if(m_poptableentries)
	{
		for(i=0;i<m_numentries;++i)
			delete m_poptableentries[i];
		delete []m_poptableentries;
	}
	m_numentries=n;
	m_selection=0;
	m_poptableentries=new kGUIComboTableRowObj *[n];

	for(i=0;i<n;++i)
	{
		m_poptableentries[i]=new kGUIComboTableRowObj(m_colormode==true?2:1);
	}
//	Dirty();
}


void kGUISharedComboEntries::SetEntry(unsigned int index,const char *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
//	Dirty();
}

void kGUISharedComboEntries::SetEntry(unsigned int index,const char *entryname,const char *entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(index);
	m_poptableentries[index]->SetValue(entryval);
//	Dirty();
}

void kGUISharedComboEntries::SetEntry(unsigned int index,kGUIString *entryname,int entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(entryval);
//	Dirty();
}

void kGUISharedComboEntries::SetEntry(unsigned int index,kGUIString *entryname,kGUIString *entryval)
{
	assert(index<m_numentries,"kGUIComboBoxObj: index too large");

	/* default to global, allow override */
	m_poptableentries[index]->SetFontInfo(this);

	m_poptableentries[index]->SetString(entryname);
	m_poptableentries[index]->SetRowHeight(m_poptableentries[index]->GetHeight()+4);
	m_poptableentries[index]->SetValue(index);
	m_poptableentries[index]->SetValue(entryval);
//	Dirty();
}

void kGUISharedComboEntries::RenameEntry(const char *oldname,const char *newname)
{
	unsigned int i;
	for(i=0;i<m_numentries;++i)
	{
		if(!strcmp(m_poptableentries[i]->GetString(),oldname))
		{
			m_poptableentries[i]->SetString(newname);
//			if(m_selection==i)
//				Dirty();
			return;
		}
	}
	assert(false,"name not found error!");
}

void kGUISharedComboboxObj::SetSelection(int s)
{
	if(SetSelectionz(s)==false)
	{
		assert(false,"Selection number not found!");
	}
}

bool kGUISharedComboboxObj::SetSelectionz(int s)
{
	unsigned int i;

	for(i=0;i<m_shared->GetNumEntries();++i)
	{
		if(m_shared->GetRow(i)->GetValue()==s)
		{
			m_shared->SetType(COMBOTYPE_NUM);
			m_selection=i;
			Dirty();
			return(true);
		}
	}
	return(false);	/* not found! return without an error */
}

void kGUISharedComboboxObj::SetSelection(const char *string)
{
	if(SetSelectionz(string)==false)
		passert(false,"Selection '%s' not in list!",string);
}

bool kGUISharedComboboxObj::SetSelectionz(const char *string)
{
	unsigned int i;

	m_shared->SetType(COMBOTYPE_STRING);
	for(i=0;i<m_shared->GetNumEntries();++i)
	{
		if(!stricmp(m_shared->GetRow(i)->GetString(),string))
		{
			m_selection=i;
			Dirty();
			return(true);
		}
	}
	return(false);
}

kGUIString *kGUISharedComboboxObj::GetSelectionStringObj(void)
{
	assert(m_selection<m_shared->GetNumEntries(),"kGUIComboBoxObj: index too large");

	if(m_shared->GetRow(m_selection)->GetIsTextValue()==true)
		return(m_shared->GetRow(m_selection)->GetTextValue());
	else
		return(m_shared->GetRow(m_selection)->GetText());
}

/* return the widest entry in screen pixels */
int kGUISharedComboEntries::GetWidest(void)
{
	unsigned int i;
	int w,widest;

	widest=0;
	for(i=0;i<m_numentries;++i)
	{
		/* extra space is added to account for the table edge bevels */
		/* and the scrollbar on the right */
		w=m_poptableentries[i]->GetWidth()+kGUI::GetSkin()->GetScrollbarWidth()+10;
		if(w>widest)
			widest=w;
	}
	return(widest);
}

int kGUISharedComboboxObj::GetSelection(void)
{
	return m_shared->GetRow(m_selection)->GetValue();
}

void kGUISharedComboboxObj::SelectionDone(kGUIEvent *e)
{
	switch(e->GetEvent())
	{
	case EVENT_SELECTED:
		m_popped=false;

		if(m_shared->GetTable()->GetSelected()>=0)
			m_selection=m_shared->GetTable()->GetSelected();

		m_shared->CloseTable();

		SetCurrent();	/* I am now the top current object */
		Dirty();
		if(m_selection!=m_undoselection)
		{
			CallEvent(EVENT_AFTERUPDATE);
			kGUI::CallAAParents(this);
		}
		else
			CallEvent(EVENT_NOCHANGE);

		kGUI::PushActiveObj(this);
	break;
	}
}

/* returning true means I've used the input, false means pass input to someone else */

bool kGUISharedComboboxObj::UpdateInput(void)
{
	unsigned int i;
	bool over;
	kGUICorners c;
	kGUIText text;
	int showentries;
	int popx,popy,popw,poph;
	bool usedkey=false;

	if(m_popped==true)
		return(false);

	GetCorners(&c);
	over=kGUI::MouseOver(&c);
	if(over==false && kGUI::GetMouseClick()==true)
	{
		if(this==kGUI::GetActiveObj())
		{
			if(m_typedstring)
			{
				delete m_typedstring;
				m_typedstring=0;
				if(m_selection!=m_undoselection)
				{
					CallEvent(EVENT_AFTERUPDATE);
					kGUI::CallAAParents(this);
				}
				else
					CallEvent(EVENT_NOCHANGE);
			}
			kGUI::PopActiveObj();
		}
		return(false);
	}

	if(kGUI::WantHint()==true && m_shared->GetHint())
		kGUI::SetHintString(c.lx+10,c.ty-15,m_shared->GetHint()->GetString());

	if(this==kGUI::GetActiveObj())
	{
		switch(m_editmode)
		{
		case COMBOEDIT_POPLIST:
			/* this is only called so after the menu is closed by clicking */
			/* on the combo box that it doesn't  pop up again immediately */
			kGUI::PopActiveObj();		
		break;
		case COMBOEDIT_USERTYPE:
		{
			int key;
			if(kGUI::GetMouseClickLeft()==true)
			{
				delete m_typedstring;
				m_typedstring=0;
				if(m_selection!=m_undoselection)
				{
					CallEvent(EVENT_AFTERUPDATE);
					kGUI::CallAAParents(this);
				}
				else
					CallEvent(EVENT_NOCHANGE);
				kGUI::PopActiveObj();
				goto click;
			}

addkey:;
			if(kGUI::GetDrawCursorChanged())
				Dirty(&c);

			/* user is going to type and use their typing to select a */
			/* match from the combo entries */
			key=kGUI::GetKey();
			if(key)
			{
				switch(key)
				{
				case GUIKEY_ESC:
					m_selection=m_undoselection;	/* undo and fall through */
				case GUIKEY_RETURN:
					kGUI::ClearKey();
					delete m_typedstring;
					m_typedstring=0;
					if(m_selection!=m_undoselection)
					{
						CallEvent(EVENT_AFTERUPDATE);
						kGUI::CallAAParents(this);
					}
					else
						CallEvent(EVENT_NOCHANGE);

					kGUI::PopActiveObj();
					Dirty();
					return(true);
				break;
				case GUIKEY_TAB:
				case GUIKEY_SHIFTTAB:
					delete m_typedstring;
					m_typedstring=0;
					if(m_selection!=m_undoselection)
					{
						CallEvent(EVENT_AFTERUPDATE);
						kGUI::CallAAParents(this);
					}
					else
						CallEvent(EVENT_NOCHANGE);
					kGUI::PopActiveObj();
					Dirty();
					return(false);	/* pass tab to parent object */
				break;
				case GUIKEY_DELETE:
				case GUIKEY_BACKSPACE:
				case GUIKEY_LEFT:
					usedkey=true;
					if(m_typedstring->GetLen())
						m_typedstring->Clip(m_typedstring->GetLen()-1);
				break;
				default:
					if((key>=' ') || (key>='a' && key<='z') || (key>='A' && key<='Z'))
					{
						m_typedstring->Append((unsigned int)key);
						usedkey=true;
					}
				break;
				}
				if(usedkey==true)
				{
					Dirty();
					kGUI::ClearKey();
					if(m_typedstring->GetLen()>0)
					{
						unsigned int s;
						int bests=-1;

						/* find first match alphabetically, if none, then delete typed character */
						kGUI::ClearKey();
						for(s=0;s<m_shared->GetNumEntries();++s)
						{
							if(!strcmpin(m_shared->GetRow(s)->GetString(),m_typedstring->GetString(),m_typedstring->GetLen()))
							{
								if(bests==-1)
									bests=s;
								else
								{
									if(stricmp(m_shared->GetRow(s)->GetString(),m_shared->GetRow(bests)->GetString())<0)
										bests=s;
								}
							}
						}
						if(bests==-1)
						{
							if(m_typedstring->GetLen())
								m_typedstring->Clip(m_typedstring->GetLen()-1);
						}
						else
							m_selection=bests;
					}
					else
						m_selection=m_undoselection;	/* if nothing typed then go back to initial selection */
				}
			}
		}
		break;
		}
	}
	else if(m_locked==false && (kGUI::GetMouseClickLeft()==true))
	{
click:;
		/* decide if they clicked on the arrow or on the text area */
		int cw=kGUI::GetSkin()->GetComboArrowWidth();

		SetCurrent();
		CallEvent(EVENT_ENTER);
		m_undoselection=m_selection;

		if((kGUI::GetMouseX()<(c.rx-cw)) && (m_shared->GetAllowTyping()==true))
		{
			m_editmode=COMBOEDIT_USERTYPE;
			assert(m_typedstring==0,"Error typed string already allocated!");
			m_typedstring=new kGUIString();
			kGUI::PushActiveObj(this);
			goto addkey;
		}
		else
		{
			kGUITableObj *t;

			m_editmode=COMBOEDIT_POPLIST;

			/* calculate position of popup selector table */
			showentries=m_shared->GetNumEntries();
			if(showentries>MAXSHOWCOMBO)
				showentries=MAXSHOWCOMBO;
//			text.SetFontInfo(&m_fontinfo);
			text.SetFontInfo(m_shared);
			popx=c.lx;
			popw=GetZoneW();
			popy=c.ty+GetZoneH();
			poph=(text.GetLineHeight()+6)*showentries;

			/* if this will go off of the bottom of the screen (with a little space at the bottom) */
			/* then move it above instead */

			if((popy+poph)>(kGUI::GetSurfaceHeight()-16))
				popy=(kGUI::GetSurfaceHeight()-16)-poph;

			/* ditto for right */
			if((popx+popw)>kGUI::GetSurfaceWidth())
				popx=kGUI::GetSurfaceWidth()-popw;

			Dirty();

			assert(m_shared->GetTable()==0,"Already allocated error!");
			t= new kGUITableObj();
			m_shared->SetTable(t);
			t->NoColHeaders();
			t->NoRowHeaders();
			t->NoColScrollbar();
			if(m_shared->GetColorMode()==true)
				t->SetNumCols(2);
			else
				t->SetNumCols(1);
			t->SetEventHandler(this,CALLBACKNAME(SelectionDone));
			//t->SetSelectedCallBack(this,CALLBACKNAME(SelectionDone));
			t->SetSelectMode();
			/* add entries here */
			for(i=0;i<m_shared->GetNumEntries();++i)
			{
				m_shared->GetRow(i)->SetRowHeight(m_shared->GetRow(i)->GetHeight()+4);
				t->AddRow(m_shared->GetRow(i));
				t->SetEntryEnable(i,true);
			}

			t->SetZone(popx,popy,popw,poph);
			t->SizeDirty();
			t->LockCol();	/* don't allow cursor left/right */
			if(m_shared->GetColorMode()==false)
				t->SetColWidth(0,popw);
			else
			{
				t->SetColWidth(0,popw-m_shared->GetColorWidth());
				t->SetColWidth(1,m_shared->GetColorWidth());
			}
			t->Dirty();

			m_popped=true;
			kGUI::AddWindow(t);
			kGUI::PushActiveObj(t);

			t->GotoRow(m_selection);

			t->ClearOver();
			return(true);
		}
	}
	return(true);
}

void kGUISharedComboboxObj::Draw(void)
{
	int h;
	kGUICorners c;
	kGUIText *text;
	kGUIColor tc;
	int cw;

	if(!m_shared->IsValid())
		return;	/* not entries have been added to the shared object yet */

	GetCorners(&c);
	text=m_shared->GetRow(m_selection)->GetText();
	h=text->GetLineHeight()+4;

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRectBevelIn(c.lx,c.ty,c.rx,c.by);
		if(m_locked==true)
			tc=DrawColor(172,168,153);
		if(!ImCurrent())
			tc=text->GetColor();
		else
		{
			kGUI::DrawRect(c.lx+2,c.ty+2,c.lx+6+text->GetWidth(),c.by-2,DrawColor(16,16,16));
			tc=DrawColor(255,255,255);
		}

		/* is the user typing? */
		if(m_typedstring)
		{
			unsigned int tl;
			int tlx;

			tl=m_typedstring->GetLen();
			tlx=text->GetWidthSub(0,tl);

			kGUI::DrawRect(c.lx+2,c.ty+2,c.lx+COMBOTEXTEDGEW+tlx,c.by-2,DrawColor(255,255,255));
			text->DrawSection(0,tl,c.lx+COMBOTEXTEDGEW,c.ty+COMBOTEXTEDGEH,text->GetLineHeight(),DrawColor(16,16,16));

			/* draw cursor */
			kGUI::DrawRect(c.lx+COMBOTEXTEDGEW+tlx,c.ty+2,c.lx+COMBOTEXTEDGEW+tlx+2,c.by-2,kGUI::GetDrawCursor()?DrawColor(255,255,255):DrawColor(0,0,0));

			if(tl<text->GetLen())
			{
				kGUI::DrawRect(c.lx+COMBOTEXTEDGEW+tlx+2,c.ty+2,c.lx+COMBOTEXTEDGEW+text->GetWidth(),c.by-2,DrawColor(16,16,16));
				text->DrawSection(tl,text->GetLen(),c.lx+COMBOTEXTEDGEW+tlx,c.ty+COMBOTEXTEDGEH,text->GetLineHeight(),DrawColor(255,255,255));
			}
		}
		else
			text->Draw(c.lx+COMBOTEXTEDGEW,c.ty+COMBOTEXTEDGEH,0,0,tc);

		cw=kGUI::GetSkin()->GetComboArrowWidth();
		if(m_shared->GetColorMode())
			kGUI::DrawRect(c.rx-m_shared->GetColorWidth()-cw-3,c.ty+1,c.rx-cw-3,c.by-1,m_shared->GetRow(m_selection)->GetBoxColor());

		c.lx=c.rx-cw;
		kGUI::GetSkin()->DrawComboArrow(&c);
	}
	kGUI::PopClip();
}
