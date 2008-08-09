/*********************************************************************************/
/* kGUI - kguicontrol.cpp                                                        */
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

/*! @file kguicontrol.cpp 
   @brief A kGUIControlBoxObj is just a conatiner that holds other 'child' controls            
   it used used for automatic layout of object positions so you don't have to    
   'hard code' the positions of all the child objects, it automatically          
   positions them in rows based on their sizes. Child objects can be added to the
   control object one at a time or in groups. */

#include "kgui.h"

kGUIControlBoxObj::kGUIControlBoxObj()
{
	m_redo=false;
	m_drawbg=true;
	m_drawframe=true;
	m_bgcolor=DrawColor(230,230,230);

	/* default to full screen if not overwritten */
	m_maxwidth=kGUI::GetFullScreenWidth();
	m_maxheight=kGUI::GetFullScreenHeight();

	SetNumGroups(1);	/* only 1 group */
	Reset();
}

void kGUIControlBoxObj::Reset(void)
{
	m_currentx=0;
	m_currenty=0;
	m_tallest=0;
	m_bordergap=3;
	m_objectgap=3;
	m_numfixed=0;
	m_fixedobjects.Init(2,2);
}

/* calc available space for child objects! */
void kGUIControlBoxObj::CalcChildZone(void)
{
	SetChildZone(m_bordergap,m_bordergap,GetZoneW()-(m_bordergap<<1),GetZoneH()-(m_bordergap<<1));
}

/* force next controls to start back on the left edge */
void kGUIControlBoxObj::NextLine(void)
{
	int newh;

	if(!m_currenty)
		m_currenty=0;	//m_bordergap;
	else
		m_currenty+=m_objectgap;
	m_currenty+=m_objectgap+m_tallest;
	m_currentx=0;
	newh=m_currenty;	//+m_bordergap;

	m_tallest=0;
	if(newh>GetZoneH())
	{
		if(newh<=m_maxheight)
			SetZoneH(newh);
		else
			passert(false,"cannot fit control in the box! newh=%d,max=%d",newh,m_maxheight);
	}
}

/* given a width and a height, calculate the x y position */
/* where the child objects can fit */

void kGUIControlBoxObj::AllocSpace(int w,int h,int *x,int *y)
{
	int objx,objy;
	int neww,newh;
	int n;
	bool overlap;

	passert((w+(m_bordergap<<1))<=m_maxwidth,"Object is too wide to fit (%d>%d)!\n",w,m_maxwidth);

	if(m_currentx)
		m_currentx+=m_objectgap;
	else
		m_currentx=0;	//m_bordergap;

	objx=m_currentx;
	objy=m_currenty;	//+m_bordergap;

again:;
	neww=objx+w+(m_bordergap<<1);
	newh=objy+h+(m_bordergap<<1);

	/* will this overlap any fixed objects? */
	do
	{
		overlap=false;
		for(n=0;n<m_numfixed;++n)
		{
			kGUICorners c;
			kGUICorners *fc;

			c.lx=objx;
			c.rx=objx+w;
			c.ty=objy;
			c.by=objy+h;
			fc=m_fixedobjects.GetEntryPtr(n);
			if(kGUI::Overlap(&c,fc))
			{
				/* move to the right of the fixed object */
				objx=fc->rx+m_objectgap;
				neww=objx+w+(m_bordergap<<1);
				overlap=true;
			}
		}
	}while(overlap);

	if((neww>GetZoneW()))
	{
		if(neww<=m_maxwidth)
			SetSize(neww,GetZoneH());
		else
		{
			objx=0;
			if(m_currenty)
				m_currenty+=m_objectgap;
			m_currenty+=m_tallest;
			objy+=m_objectgap+m_tallest;
			m_tallest=0;

			/* check for collisions again */
			goto again;
		}
	}
	if(newh>GetZoneH())
	{
		if(newh<=m_maxheight)
			SetSize(GetZoneW(),newh);
		else
			passert(false,"cannot fit control in the box! newh=%d,max=%d",newh,m_maxheight);
	}
	m_currentx=objx+w;
	if(h>m_tallest)
		m_tallest=h;

	if(x)
		x[0]=objx;
	if(y)
		y[0]=objy;
}

/* this function uses the positions of the objects as offsets and adds them */
/* to the calculated position. */

/* the parameters are, number of objects followed by list of object pointers */
void kGUIControlBoxObj::AddObjects(unsigned int num,...)
{
    va_list ap;
	unsigned int i;
	Array<kGUIObj *>objlist;
	int w,h;
	int offx,offy;
	kGUIObj *obj;

	if(m_redo==false)
	{
		assert(GetParent()==0,"Control box should not be attached to parent until after child objects have all been added to it!");
	}
	objlist.Alloc(num);
	va_start(ap, num);
	for(i=0;i<num;++i)
	{
		obj = va_arg(ap, kGUIObj *);
		objlist.SetEntry(i,obj);
    }
    va_end(ap);

	/* ok, we have a list of objects, now calculate their bounds */
	w=0;
	h=0;
	for(i=0;i<num;++i)
	{
		obj=objlist.GetEntry(i);
		if((obj->GetZoneRX())>w)
			w=obj->GetZoneRX();
		if((obj->GetZoneBY())>h)
			h=obj->GetZoneBY();
	}
	/* allocate space for these objects */
	AllocSpace(w,h,&offx,&offy);
	/* set the relative positions for them now */
	for(i=0;i<num;++i)
	{
		obj=objlist.GetEntry(i);
		obj->SetPos(obj->GetZoneX()+offx,obj->GetZoneY()+offy);
		kGUIContainerObj::AddObject(obj);
	}
}

/* this addobject ignores the position of the object and overwrites it */
void kGUIControlBoxObj::AddObject(kGUIObj *obj)
{
	int x,y;

	if(m_redo==false)
	{
		assert(GetParent()==0,"Control box should not be attached to parent until after child objects have all been added to it!");
	}

	AllocSpace(obj->GetZoneW(),obj->GetZoneH(),&x,&y);
	obj->SetPos(x,y);
	kGUIContainerObj::AddObject(obj);
}

void kGUIControlBoxObj::AddFixedObject(kGUIObj *obj)
{
	kGUICorners c;

	assert(GetParent()==0,"Control box should not be attached to parent until after child objects have all been added to it!");

	/* save corners for overlap checking */
	c.lx=c.rx=obj->GetZoneX();
	c.ty=c.by=obj->GetZoneY();
	c.rx+=obj->GetZoneW();
	c.by+=obj->GetZoneH();
	m_fixedobjects.SetEntry(m_numfixed++,c);
	kGUIContainerObj::AddObject(obj);
}


bool kGUIControlBoxObj::UpdateInput(void)
{
	/* pass input to children */
	return(UpdateInputC(0));
}

void kGUIControlBoxObj::Draw(void)
{
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	if(m_drawbg)
	{
		if(m_drawframe)
			kGUI::DrawRectFrame(c.lx,c.ty,c.rx,c.by,m_bgcolor,DrawColor(0,0,0));
		else
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,m_bgcolor);
	}

	DrawC(0);				/* draw all children */
	kGUI::PopClip();
}
