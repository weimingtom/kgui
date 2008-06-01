/*********************************************************************************/
/* kGUI - kguitick.cpp                                                           */
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
/* a simple tickbox gui object, a callback is trigerred when it is changed       */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"

kGUITickBoxObj::kGUITickBoxObj()
{
	int w,h;

	/* get the usual size of a tickbox from the skin code */
	kGUI::GetSkin()->GetTickboxSize(&w,&h);
	m_hint=0;
	m_locked=false;
	m_selected=false;
	m_scale=false;
	SetSize(w,h);
};

kGUITickBoxObj::~kGUITickBoxObj()
{
	if(m_hint)
		delete m_hint;
}

/* it's changed, so call it's after update callback and also */
/* call it's parent's afterupdate callback too */
void kGUITickBoxObj::CallAfterUpdate(void)
{
	CallEvent(EVENT_AFTERUPDATE);
	kGUI::CallAAParents(this);
}

/* returning true means I've used the input, false means pass input to someone else */

bool kGUITickBoxObj::UpdateInput(void)
{
	/* does the user want a hint? */
	if(kGUI::WantHint()==true && m_hint)
	{
		kGUICorners c;

		GetCorners(&c);
		kGUI::SetHintString(c.lx+10,c.ty-15,m_hint->GetString());
	}

	if(m_locked==false)
	{
		if(kGUI::GetMouseClickLeft()==true || kGUI::GetKey()==' ')
		{
			SetCurrent();
			Click();
		}
	}
	return(true);
}

/* this is seperated out so the user can have a label next to a tick */
/* box and in the labels click callback they can then call this click */
/* to toggle the associated tickbox and have it's callback trigerred */

void kGUITickBoxObj::Click(void)
{
	SetSelected(!m_selected);
	CallAfterUpdate();
}

void kGUITickBoxObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::GetSkin()->DrawTickbox(&c,m_scale,m_selected,ImCurrent());
	}
	kGUI::PopClip();
}

