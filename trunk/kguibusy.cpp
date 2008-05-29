/*********************************************************************************/
/* kGUI - kguibusy.cpp                                                           */
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

kGUIBusy::kGUIBusy(int w)
{
	int h=48;

	m_w=w;
	m_window.SetPos((kGUI::GetSurfaceWidth()-w)/2,(int)((kGUI::GetSurfaceHeight()*0.75f)-h));
	m_window.SetSize(w+4,h+6);
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);

	m_busyrect.SetPos(2,2);
	m_lastw=0;
	m_busyrect.SetSize(0,16);
	m_window.AddObject(&m_busyrect);

	kGUI::ReDraw();
}

void kGUIBusy::SetCur(int v)
{
	double dw;
	int w;

	/* only re-draw if it changes the pixel width */
	dw=(((double)m_w*(double)v)/(double)m_max)/8.0f;
	w=(int)dw;
	if(w!=m_lastw)
	{
		m_busyrect.SetSize(w<<3,16);
		m_lastw=w;
		kGUI::ReDraw();
	}
}

void kGUIBusy::SetMax(int v)
{
	if(m_max!=v)
	{
		m_max=v;
		m_lastw=0;
		m_busyrect.SetSize(0,16);
		kGUI::ReDraw();
	}
}

kGUIBusy::~kGUIBusy()
{
	kGUI::DelWindow(&m_window);
	kGUI::ReDraw();
}

void kGUIBusyRectObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::GetSkin()->DrawBusy(&c);
}
