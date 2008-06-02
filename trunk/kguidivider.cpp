/*********************************************************************************/
/* kGUI - kguidivider.cpp                                                        */
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

/*! @file kguidivider.cpp
    @brief A divider object is just a horizontal bar that you use to move around         
 it can be used as a split object between a table and a map for example and    
 it's callback can be used to resize the table and map whenever the bar is     
 moved by the user */

/*! @todo Code vertical divider as well */

void kGUIDividerObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::DrawRectBevel(c.lx,c.ty,c.rx,c.by,false);
}

bool kGUIDividerObj::UpdateInput(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(kGUI::MouseOver(&c)==true)
	{
		kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);

		if(this!=kGUI::GetActiveObj() && kGUI::GetMouseClickLeft()==true)
		{
			kGUI::PushActiveObj(this);
			SetCurrent();
		}
	}

	if(this==kGUI::GetActiveObj())
	{
		int dy;
		if(kGUI::GetMouseReleaseLeft()==true)
		{
			kGUI::PopActiveObj();
			return(true);
		}

		dy=kGUI::GetMouseDY();
		if(dy)
		{
			kGUIEvent e;

			e.m_value[0].i=dy;
			CallEvent(EVENT_AFTERUPDATE,&e);
		}
		kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);

		return(true);
	}
	else
		return(false);
}
