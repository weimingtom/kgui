/*********************************************************************************/
/* kGUI - kguibutton.cpp                                                         */
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

kGUIButtonObj::kGUIButtonObj()
{
	m_frame=true;			/* default is framed text or image */
	m_showcurrent=true;
	m_enabled=true;
	m_pressed=false;
	m_image=0;
	m_hint=0;
	m_disabledimage=0;
	m_pushover=false;
	SetColor(DrawColor(0,0,0));		/* default is black text */
	SetHAlign(FT_CENTER);			/* default is text centered left/right */
	SetVAlign(FT_MIDDLE);			/* default is text centered top/bottom */
}

void kGUIButtonObj::Control(unsigned int command,KGCONTROL_DEF *data)
{
	switch(command)
	{
	case KGCONTROL_GETENABLED:
		data->m_bool=m_enabled;
	break;
	default:
		kGUIObj::Control(command,data);
	break;
	}
}

kGUIButtonObj::~kGUIButtonObj()
{
	if(m_disabledimage)
		delete m_disabledimage;
	if(m_hint)
		delete m_hint;
}


/* if this button has an image on it, then we need to generate */
/* an image to draw if the button is disabled */
/* we do this by copying the image and making a gray scale version */
/* and darkening it to show that is is a disabled button */

void kGUIButtonObj::MakeDisabledImage(kGUIImage *image)
{
	/* copy image yet turn all near black pixels to grey */
	if(!m_disabledimage)
		m_disabledimage=new kGUIImage;
	m_disabledimage->CopyImage(image);
	m_disabledimage->GreyImage(0);
}

/* returning true means I've used the input, false means pass input to someone else */

bool kGUIButtonObj::UpdateInput(void)
{
	bool used=false;
	bool lpressed=m_pressed;
	bool over;
	kGUICorners c;

	if(m_enabled==false)
		return(true);	/* stop input from being passed down to next layer */

	GetCorners(&c);
	over=kGUI::MouseOver(&c);

	if(kGUI::WantHint()==true && m_hint)
		kGUI::SetHintString(c.lx+10,c.ty-15,m_hint->GetString());

	/* if they are not over me and they clicked the mouse then abort */
	if(over==false && kGUI::GetMouseClick()==true)
	{
		m_pushover=false;
		if(this==kGUI::GetActiveObj())
			kGUI::PopActiveObj();
		return(false);
	}

	/* if they clicked the mouse then draw it pushed but don't */
	/* process unless they also release the button over me */
	if(kGUI::GetMouseClickLeft()==true)
	{
		m_pushover=true;
		used=true;
		SetCurrent();
		/* I need to be active because, if while pressing the mouse */
		/* they move it off of the button I need to still be called */
		/* and this only happens for active objects */
		if(kGUI::GetActiveObj()!=this)
			kGUI::PushActiveObj(this);	
		CallEvent(EVENT_LEFTCLICK);
	}
	else if(kGUI::GetMouseReleaseLeft()==true)
	{
		if(kGUI::GetActiveObj()==this)
			kGUI::PopActiveObj();
		/* only a valid press if still over the button when released */
		if(over==true && m_pushover==true)
		{
			m_pressed=false;
			Dirty();	/* force redraw of unpressed button */
			CallEvent(EVENT_PRESSED);
//			m_pressedcallback.Call();
//			kGUI::CallAAParents(this);
			return(true);
		}
		m_pushover=false;
	}
	else if(kGUI::GetMouseClickRight()==true)
	{
		if(over==true)
		{
			CallEvent(EVENT_RIGHTCLICK);
//			m_rpressedcallback.Call();
			return(true);
		}
	}
	else if((kGUI::GetKey()==' ') || (kGUI::GetKey()==GUIKEY_RETURN))
	{
		kGUI::ClearKey();
		if(kGUI::GetActiveObj()==this)
			kGUI::PopActiveObj();

		m_pressed=false;
		CallEvent(EVENT_PRESSED);
//		m_pressedcallback.Call();
//		kGUI::CallAAParents(this);
		return(true);
	}

	if(over==false)
		m_pressed=false;
	else
		m_pressed=kGUI::GetMouseLeft();

	/* if pressed status has changed then force redraw */
	if(m_pressed!=lpressed)
		Dirty();
	return(used);
}

#define BEDGE 3

void kGUIButtonObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		/* if I am not the current child then unpress me now! */
		if(GetParent())
		{
			if(GetParent()->GetCurrentChild()!=this)
				m_pressed=false;
		}

		if(m_frame==true)
		{
			kGUI::DrawRectBevel(c.lx,c.ty,c.rx,c.by,m_pressed);

			kGUI::PushClip();
			kGUI::ShrinkClip(c.lx+BEDGE,c.ty+BEDGE,c.rx-BEDGE,c.by-BEDGE);

			/* is this an image or text button? */
			if(m_image)
			{
				int offx,offy;
			
				/* centere the image */
				offx=((c.rx-c.lx)-m_image->GetImageWidth())>>1;
				offy=((c.by-c.ty)-m_image->GetImageHeight())>>1;

				if(m_enabled==true)
					m_image->Draw(0,c.lx+offx,c.ty+offy);
				else
					m_disabledimage->Draw(0,c.lx+offx,c.ty+offy);
			}
			else
			{
				int w=GetZoneW()-(BUTTONTEXTEDGE<<1);
				int h=GetZoneH()-(BUTTONTEXTEDGE<<1);

				if(m_enabled==true)
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE,c.ty+BUTTONTEXTEDGE,w,h,GetColor());
				else
				{
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE,c.ty+BUTTONTEXTEDGE,w,h,DrawColor(255,255,255));
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE-1,c.ty+BUTTONTEXTEDGE,w,h,DrawColor(172,168,153));
				}
			}
			kGUI::PopClip();
			if(ImCurrent() && m_showcurrent)
				kGUI::DrawCurrentFrame(c.lx+5,c.ty+5,c.rx-5,c.by-5);
		}
		else	/* no frame */
		{
			if(m_image)
			{
				int offx,offy;
			
				/* center the image */
				offx=((c.rx-c.lx)-m_image->GetImageWidth())>>1;
				offy=((c.by-c.ty)-m_image->GetImageHeight())>>1;

				if(m_enabled==true)
					m_image->Draw(0,c.lx+offx,c.ty+offy);
				else
					m_disabledimage->Draw(0,c.lx+offx,c.ty+offy);
			}
			else
			{
				int w=GetZoneW()-(BUTTONTEXTEDGE<<1);
				int h=GetZoneH()-(BUTTONTEXTEDGE<<1);

				if(m_enabled==true)
				{
					SetColor(DrawColor(16,16,16));
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE,c.ty+BUTTONTEXTEDGE,w,h);
				}
				else
				{
					SetColor(DrawColor(255,255,255));
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE,c.ty+BUTTONTEXTEDGE,w,h);
					SetColor(DrawColor(172,168,153));
					kGUIText::Draw(c.lx+BUTTONTEXTEDGE-1,c.ty+BUTTONTEXTEDGE,w,h);
				}
			}
			if(ImCurrent() && m_showcurrent)
				kGUI::DrawCurrentFrame(c.lx,c.ty,c.rx-1,c.by-1);
		}
	}
	kGUI::PopClip();
}
