/**********************************************************************************/
/* kGUI - kguigrid.cpp                                                            */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://code.google.com/p/kgui/	                                              */
/*                                                                                */
/*    kGUI is free software; you can redistribute it and/or modify                */
/*    it under the terms of the GNU Lesser General Public License as published by */
/*    the Free Software Foundation; version 2.                                    */
/*                                                                                */
/*    kGUI is distributed in the hope that it will be useful,                     */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/*    GNU General Public License for more details.                                */
/*                                                                                */
/*    http://www.gnu.org/licenses/lgpl.txt                                        */
/*                                                                                */
/*    You should have received a copy of the GNU General Public License           */
/*    along with kGUI; if not, write to the Free Software                         */
/*    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA  */
/*                                                                                */
/**********************************************************************************/

/*! @file kguigrid.cpp 
    @brief This is a scrolling grid class where each cell is of a pre-defined size.       
    There is a user-defined callback attached to the cells to render them                                                                                
    For Example:                                                                   
    It is used in GPSTurbo for the maps where each cell is 256x256 pixels and      
    as the user scrolls the grid around, new cells that appear on the edges are    
    rendered into and as cell scroll off they are freed up for re-use. */

#include "kgui.h"
#include "kguigrid.h"
#include <math.h>

kGUICellObj::kGUICellObj(kGUIGridObj *p,int w,int h)
{
	m_parent=p;
	m_surface.SetOffsets(0,0);
	m_surface.Init(w,h);
	m_image.SetMemImage(0,GUISHAPE_SURFACE,m_surface.GetWidth(),m_surface.GetHeight(),m_surface.GetBPP(),(const unsigned char *)m_surface.GetSurfacePtr(0,0));
	m_cellx=9999;
	m_celly=9999;
	m_onscreen=false;
	m_dirty=false;
}

/* this is used to re-draw the cell */
void kGUICellObj::Update(int cx,int cy)
{
	kGUIDrawSurface *savesurface;

	m_cellx=cx;
	m_celly=cy;

	kGUI::PushClip();
	savesurface=kGUI::GetCurrentSurface();
	kGUI::SetCurrentSurface(&m_surface);
	kGUI::ResetClip();	/* set clip to full surface on stack */
	m_parent->CallDraw(this);
	kGUI::SetCurrentSurface(savesurface);
	kGUI::PopClip();
}

void kGUICellObj::Draw(int sx,int sy)
{
	m_image.Draw(0,sx,sy);
}

kGUIGridObj::kGUIGridObj()
{
}

kGUIGridObj::~kGUIGridObj()
{
	int i;

	for(i=0;i<m_numcells;++i)
	{
		kGUICellObj *cobj=m_cells.GetEntry(i);
		delete cobj;
	}
}

void kGUIGridObj::Init(int gridwidth,int gridheight,int cellwidth,int cellheight)
{
	m_printmode=false;
	m_bounds=false;
	m_scrollx=0;
	m_scrolly=0;
	m_scrollxfrac=0;
	m_scrollyfrac=0;
	m_cellwidth=cellwidth;
	m_cellheight=cellheight;
	m_numcells=0;			/* number of cells currently in the cell pool */
	
	SetSize(gridwidth,gridheight);
}

void kGUIGridObj::ReSetCellSize(int cellwidth,int cellheight)
{
	int i;

	if((cellwidth==m_cellwidth) && (cellheight==m_cellheight))
		return;

	/* if size has changed then we need to free all old ones and reallocate */
	m_cellwidth=cellwidth;
	m_cellheight=cellheight;

	/* free and reallocate all cells as their size has changed! */
	for(i=0;i<m_numcells;++i)
	{
		kGUICellObj *cobj;
		
		cobj=m_cells.GetEntry(i);
		delete cobj;

		cobj=new kGUICellObj(this,cellwidth,cellheight);
		m_cells.SetEntry(i,cobj);
	}

	ReSize(m_gridwidth,m_gridheight);
}

void kGUIGridObj::ReSize(int gridwidth,int gridheight)
{
	int i,nc;

	m_gridwidth=gridwidth;
	m_gridheight=gridheight;

	m_numdrawx=(m_gridwidth/m_cellwidth)+2;
	m_numdrawy=(m_gridheight/m_cellheight)+2;
	m_numdraw=m_numdrawx*m_numdrawy;
	m_drawcells.Alloc(m_numdraw);
	for(i=0;i<m_numdraw;++i)
		m_drawcells.SetEntry(i,0);	/* initlz array to no valid cells */
	
	/* do we have enough cells? */

	if(m_printmode==true)
		return;		/* just use the number of cells that it used for on-screen */

	nc=m_numdraw+(m_numdraw>>1);	/* need 150% so we have extra for scrolling */
	if(nc>m_numcells)
	{
		m_cells.Alloc(nc);
		for(i=m_numcells;i<nc;++i)
		{
			kGUICellObj *cobj;
		
			cobj=new kGUICellObj(this,m_cellwidth,m_cellheight);
			m_cells.SetEntry(i,cobj);
		}
		m_numcells=nc;
	}
}

void kGUIGridObj::FlushCell(int cx,int cy)
{
	int i;
	kGUICellObj *cobj;

	Dirty();
	for(i=0;i<m_numcells;++i)
	{
		cobj=m_cells.GetEntry(i);
		if(cobj->GetX()==cx && cobj->GetY()==cy)
		{
			cobj->Flush();
			return;
		}
	}
	/* cell is not in list so no need to flush it */
}

kGUICellObj *kGUIGridObj::GetCell(int cx,int cy)
{
	int i;
	double dist,fdist;
	kGUICellObj *cobj;
	kGUICellObj *fobj;

	/* does this cell already exist in the list? */
	for(i=0;i<m_numcells;++i)
	{
		cobj=m_cells.GetEntry(i);
		if(cobj->GetX()==cx && cobj->GetY()==cy)
		{
			if(cobj->GetDirty()==true)
			{
				cobj->SetDirty(false);
				cobj->Update(cx,cy);
			}
			return(cobj);
		}
	}

	/* pick the farthest one away to use */
	fobj=0;
	fdist=0.0f;
	for(i=0;i<m_numcells;++i)
	{
		cobj=m_cells.GetEntry(i);
		dist=hypot((double)(cobj->GetX()-cx),(double)(cobj->GetY()-cy));
		if(dist>fdist)
		{
			fobj=cobj;
			fdist=dist;
		}
	}
	/* draw cell contents for this new position */
	fobj->SetDirty(false);
	fobj->Update(cx,cy);
	return(fobj);
}

void kGUIGridObj::Draw(void)
{
	int d,x,y,cx,cy;
	int sx,sy;
	kGUICellObj *cobj;
	kGUICorners c;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	if(kGUI::ValidClip())
	{
		if(m_predrawcallback.IsValid())
			m_predrawcallback.Call();

		d=0;
		for(sy=c.ty-m_scrollyfrac,cy=m_scrolly,y=0;y<m_numdrawy;sy+=m_cellheight,++y,++cy)
		{
			for(sx=c.lx-m_scrollxfrac,cx=m_scrollx,x=0;x<m_numdrawx;sx+=m_cellwidth,++x,++cx)
			{
				cobj=m_drawcells.GetEntry(d);
				if(!cobj)
				{
					cobj=GetCell(cx,cy);
					m_drawcells.SetEntry(d,cobj);
				}
				else if(cobj->GetX()!=cx || cobj->GetY()!=cy)
				{
					cobj=GetCell(cx,cy);
					m_drawcells.SetEntry(d,cobj);
				}
				if(cobj->GetDirty()==true)
				{
					cobj->SetDirty(false);
					cobj->Update(cx,cy);
				}
			
				cobj->Draw(sx,sy);
				++d;
			}
		}
		/* mainly used for drawing a current position cursor etc */
		if(m_postdrawcallback.IsValid())
			m_postdrawcallback.Call();
	}
	kGUI::PopClip();
}

void kGUIGridObj::ClipScrollX(void)
{
	int sx,newsx;
	
	if(m_bounds==true)
	{
		sx=(m_cellwidth*m_scrollx)+m_scrollxfrac;
		newsx=sx;
		if(sx<m_minx)
			newsx=m_minx;
		else
		{
			if(m_maxx<m_gridwidth)
				newsx=0;
			else if(sx>(m_maxx-m_gridwidth))
				newsx=m_maxx-m_gridwidth;
		}
		if(sx!=newsx)
		{
			m_scrollx=newsx/m_cellwidth;
			m_scrollxfrac=newsx-(m_scrollx*m_cellwidth);
		}
	}
}

void kGUIGridObj::ClipScrollY(void)
{
	int sy,newsy;
	
	if(m_bounds==true)
	{
		sy=(m_cellheight*m_scrolly)+m_scrollyfrac;
		newsy=sy;
		if(sy<m_miny)
			newsy=m_miny;
		else
		{
			if(m_maxy<m_gridheight)
				newsy=0;
			else if(sy>(m_maxy-m_gridheight))
				newsy=(m_maxy-m_gridheight);
		}
		if(sy!=newsy)
		{
			m_scrolly=newsy/m_cellheight;
			m_scrollyfrac=newsy-(m_scrolly*m_cellheight);
		}
	}
}

bool kGUIGridObj::UpdateInput(void)
{
	kGUICorners c;

	if(kGUI::GetMouseLeft()==true)
	{
		int dx,dy;

		dx=kGUI::GetMouseDX();
		dy=kGUI::GetMouseDY();
		if(dx || dy)
			Dirty();
		if(dx)
		{
			m_scrollxfrac-=dx;
			while(m_scrollxfrac<0)
			{
				m_scrollxfrac+=m_cellwidth;
				--m_scrollx;
			}
			while(m_scrollxfrac>=m_cellwidth)
			{
				m_scrollxfrac-=m_cellwidth;
				++m_scrollx;
			}
			ClipScrollX();
		}

		if(dy)
		{
			m_scrollyfrac-=dy;
			while(m_scrollyfrac<0)
			{
				m_scrollyfrac+=m_cellheight;
				--m_scrolly;
			}
			while(m_scrollyfrac>=m_cellheight)
			{
				m_scrollyfrac-=m_cellheight;
				++m_scrolly;
			}
			ClipScrollY();
		}
	}

	/* call user code */
	GetCorners(&c);
	if(kGUI::MouseOver(&c)==true)
	{
		if(m_overcallback.IsValid())
			m_overcallback.Call(kGUI::GetMouseX()-c.lx,kGUI::GetMouseY()-c.ty);
	}

	return(true);
}

void kGUIGridObj::GetScrollCorner(int *x,int *y)
{
	x[0]=((m_scrollx*m_cellwidth)+m_scrollxfrac);
	y[0]=((m_scrolly*m_cellheight)+m_scrollyfrac);
}


void kGUIGridObj::GetScrollCenter(int *x,int *y)
{
	GetScrollCorner(x,y);
	x[0]+=(GetZoneW()/2);
	y[0]+=(GetZoneH()/2);
}

void kGUIGridObj::SetScrollCorner(int x,int y)
{
	int oldx,oldxfrac,oldy,oldyfrac;

	oldx=m_scrollx;
	oldxfrac=m_scrollxfrac;
	oldy=m_scrolly;
	oldyfrac=m_scrollyfrac;

	m_scrollx=x/m_cellwidth;
	m_scrollxfrac=x-(m_scrollx*m_cellwidth);

	m_scrolly=y/m_cellheight;
	m_scrollyfrac=y-(m_scrolly*m_cellheight);
	ClipScrollX();
	ClipScrollY();

	/* only redraw if it has changed */
	if((oldx!=m_scrollx) || (oldxfrac!=m_scrollxfrac) || (oldy!=m_scrolly) || (oldyfrac!=m_scrollyfrac))
		Dirty();
}


void kGUIGridObj::SetScrollCenter(int x,int y)
{
	x-=(GetZoneW()/2);
	y-=(GetZoneH()/2);
	SetScrollCorner(x,y);
}


void kGUIGridObj::Flush(void)
{
	int i;
	kGUICellObj *cobj;

	for(i=0;i<m_numcells;++i)
	{
		cobj=m_cells.GetEntry(i);
		cobj->Flush();
	}
	Dirty();
}
