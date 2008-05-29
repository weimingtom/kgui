/*********************************************************************************/
/* kGUI - kguiradio.cpp                                                          */
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

/* a radio button is essentially a tickbox but with different graphics, it is */
/* mainly used when turning one on will then turn another off */

/* radio buttons have no built-in mechanism for turning another radio object */
/* off if one is pressed, you need to have the logic for exclusive or multiple */
/* button selection controlled in your own code that would be triggered by the */
/* radio object afterupdate callback. Doing it this way gives you the most */
/* flexability for implementing your own set of rules. */

kGUIRadioObj::kGUIRadioObj()
{
	int w,h;

	/* get the usual size of a radio circle from the skin code */
	kGUI::GetSkin()->GetRadioSize(&w,&h);
	m_locked=false;
	m_selected=false;
	m_scale=false;
	m_hint=0;
	SetSize(w,h);
};

kGUIRadioObj::~kGUIRadioObj()
{
	if(m_hint)
		delete m_hint;
}

/* it's changed, so call it's after update callback and also */
/* call it's parent's afterupdate callback too */
void kGUIRadioObj::CallAfterUpdate(void)
{
	CallEvent(EVENT_AFTERUPDATE);
	kGUI::CallAAParents(this);
}

bool kGUIRadioObj::UpdateInput(void)
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
			if(GetSelected()==false)
			{
				SetCurrent();
				SetSelected(true);
				CallAfterUpdate();
			}
		}
	}
	return(true);
}

void kGUIRadioObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::GetSkin()->DrawRadio(&c,m_scale,m_selected,ImCurrent());
	}
	kGUI::PopClip();
}
