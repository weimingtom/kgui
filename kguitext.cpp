/*********************************************************************************/
/* kGUI - kguitext.cpp                                                           */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/                                 */
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

#define m_xoff 3
#define m_yoff 3

void kGUITextObj::Control(unsigned int command,KGCONTROL_DEF *data)
{
	switch(command)
	{
	case KGCONTROL_GETSKIPTAB:
		data->m_bool=true;
	break;
	default:
		kGUIObj::Control(command,data);
	break;
	}
}

void kGUITextObj::Changed()
{
	/* only expand to fit, don't shrink */
	SetSize(max((int)GetWidth()+(m_xoff<<1),GetZoneW()),max((int)GetHeight()+(m_yoff<<1),GetZoneH()));
	Dirty();
}

/* if the text is too wide, then shrink the font size until it fits */
void kGUITextObj::ShrinktoFit(void)
{
	int fontsize;
	int w=GetZoneW();
	int h=GetZoneH();
	
	fontsize=GetFontSize();
	while((((int)GetWidth()+m_xoff+m_xoff)>w) && (fontsize>1))
	{
		SetFontSize(--fontsize);
		SetSize(w,h);
	}
}

void kGUITextObj::Draw(void)
{
	kGUICorners c;
	int w=GetZoneW();
	int h=GetZoneH();

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
//		if(kGUI::GetCurrentObj()==this)
		if(ImCurrent())
			SetRevRange(0,GetLen());
		else
			SetRevRange(0,0);

		if(GetUseBGColor()==true)
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,GetBGColor());
		kGUIText::Draw(c.lx+m_xoff,c.ty+m_yoff,w-(m_xoff+m_xoff),h-(m_yoff+m_yoff));
	}
	kGUI::PopClip();
}

/* since this is static text there is no real input needed except */
/* that it can be used for clicking to trigger popup menus etc. */
/* so it handles click callbacks */

bool kGUITextObj::UpdateInput(void)
{
	if(kGUI::GetMouseReleaseLeft()==true)
		CallEvent(EVENT_LEFTCLICK);
	if(kGUI::GetMouseDoubleClickLeft()==true)
		CallEvent(EVENT_LEFTDOUBLECLICK);
	return(true);
}

/********************************************************************/

/* a scroll text object is just static text with a left arrow on the */
/* left and a right arrow on the right, if either of these arrows are */
/* pressed then a callback is triggered allowing the users code to change */
/* the text. An example would be for users selecting a "month", the text */
/* would default to a particular month, then pressing the left button would */
/* have the month decrement down to January and if the right arrow is pressed */
/* it would increment up to December. The callback would be the code to change */
/* the text itself. */

kGUIScrollTextObj::kGUIScrollTextObj()
{
	SetHAlign(FT_CENTER);
	SetVAlign(FT_MIDDLE);
}

bool kGUIScrollTextObj::UpdateInput(void)
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
		}
		else if(kGUI::GetMouseX()>(c.rx-w))
		{
			e.m_value[0].i=1;					
			CallEvent(EVENT_PRESSED,&e);	/* right arrow button was pressed */
		}
		else
		{
			if(kGUI::GetMouseDoubleClickLeft()==true)
				CallEvent(EVENT_LEFTDOUBLECLICK);
			else
				CallEvent(EVENT_LEFTCLICK);
		}
		kGUI::CallAAParents(this);
	}
	return(true);
}

void kGUIScrollTextObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::GetSkin()->DrawScrollHoriz(&c);
	SetColor(DrawColor(0,0,0));
	kGUIText::Draw(c.lx,c.ty,c.rx-c.lx,c.by-c.ty);
}
