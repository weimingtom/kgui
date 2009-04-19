/**********************************************************************************/
/* kGUI - kguisubpixel.cpp                                                        */
/*                                                                                */
/* Programmed by (See below)                                                      */
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

#include "kgui.h"

kGUISubPixelCollector::kGUISubPixelCollector()
{
	m_lines.Init(2048,256);
	m_chunks.SetBlockSize(65536);
}

void kGUISubPixelCollector::SetBounds(double y1,double y2)
{
	int y;
	SUBLINE_DEF *list;

	m_chunkindex=0;
	m_topy=(int)MIN(y1,y2);
	if(m_topy<kGUI::m_clipcorners.ty)
		m_topy=kGUI::m_clipcorners.ty;
	m_bottomy=(int)MAX(y1,y2)+1;
	if(m_bottomy>=kGUI::m_clipcorners.by)
		m_bottomy=kGUI::m_clipcorners.by-1;
	list=m_lines.GetArrayPtr();
	for(y=m_topy;y<=m_bottomy;++y)
	{
		list->chunk=0;
		list++;
	}
}

void kGUISubPixelCollector::SetColor(kGUIColor c,double alpha)
{
	int r,g,b;

	DrawColorToRGB(c,r,g,b);
	m_color=c;
	m_red=(double)r;
	m_green=(double)g;
	m_blue=(double)b;
	m_alpha=alpha;
}

void kGUISubPixelCollector::AddRect(double x,double y,double w,double h,double weight)
{
	int ty,by;
	double rx,th;

	rx=x+w;
	if(x<kGUI::m_clipcornersd.lx)
		x=kGUI::m_clipcornersd.lx;
	if(rx>kGUI::m_clipcornersd.rx)
		rx=kGUI::m_clipcornersd.rx;
	if(rx<=x)
		return;	/* off */

	/* split into integer raster line chunks */
	ty=(int)y;
	by=(int)(y+h);

	/* is this all on a single line? */
	if(ty==by)
		AddChunk(ty,x,rx,h*weight);
	else
	{
		/* calc weight of top line */
		th=(double)(ty+1)-y;
		AddChunk(ty,x,rx,weight*th);
		h-=th;

		/* add full chunks */
		while(h>=1.0f)
		{
			AddChunk(++ty,x,rx,weight);
			h-=1.0f;
		}
		if(h>0.0f)
			AddChunk(++ty,x,rx,h*weight);
	}
}

void kGUISubPixelCollector::AddChunk(int y,double lx,double rx,double weight)
{
	int lineindex;
	SUBLINEPIX_DEF *chunk;
	SUBLINE_DEF *line;
	SUBLINEPIX_DEF *prev;

	/* off of clip area? */
	if((y<m_topy) || (y>m_bottomy))
		return;

	lineindex=y-m_topy;
	line=m_lines.GetEntryPtr(lineindex);
	prev=line->chunk;

	chunk=(SUBLINEPIX_DEF *)m_chunks.Alloc(sizeof(SUBLINEPIX_DEF));
	chunk->next=prev;
	chunk->weight=weight;
	chunk->leftx=lx;
	chunk->width=rx-lx;

	if(rx==kGUI::m_clipcornersd.rx)
		rx-=1.0f;

	line->chunk=chunk;
	if(!prev)
	{
		line->leftx=lx;
		line->rightx=rx;
	}
	else
	{
		line->leftx=MIN(lx,line->leftx);
		line->rightx=MAX(rx,line->rightx);
	}
}

/* ok done, collecting, now draw */
void kGUISubPixelCollector::Draw(void)
{
	int br,bg,bb;
	int newr,newg,newb;
	SUBLINE_DEF *lines;
	SUBLINEPIX_DEF *chunk;
	int x,y,lx,rx,clx,crx;
	//int gindex;
	double m_weights[2048];		/* hmm, this should not really be hardcoded */
	double weight,bweight,width,fwidth;
	kGUIColor *cp;

	lines=m_lines.GetArrayPtr();
	for(y=m_topy;y<=m_bottomy;++y)
	{
		assert(y>=0 && y<kGUI::m_clipcorners.by,"Error!");

		/* process a raster line */
		/* if chunk = 0 then there are no chunks for this raster line */
		chunk=lines->chunk;
		if(chunk)
		{
			/* this is the bounding left / right edges for all chunks in this raster line */
			lx=(int)lines->leftx;
			rx=(int)lines->rightx;
			for(x=lx;x<=rx;++x)
				m_weights[x]=0.0f;

			/* each raster line has a null terminated linked list of chunks */
			do
			{
				weight=chunk->weight;
				clx=(int)chunk->leftx;
				width=chunk->width;
				crx=(int)(chunk->leftx+width);
				if(clx==crx)
				{
					m_weights[clx]+=width*weight;
//					assert(m_weights[clx]<=1.0f,"Overflow!");
				}
				else
				{
					fwidth=1.0f-(chunk->leftx-(double)clx);
					m_weights[clx++]+=fwidth*weight;
//					assert(m_weights[clx]<=1.01f,"Overflow!");
					width-=fwidth;
					while(width>=1.0f)
					{
						m_weights[clx++]+=weight;
//						assert(m_weights[clx]<=1.01f,"Overflow!");
						width-=1.0f;
					}
					if(width>0.0f)
					{
						m_weights[clx]+=width*weight;
//						assert(m_weights[clx]<1.01f,"Overflow!");
					}
				}
				chunk=chunk->next;
			}while(chunk);

			/* ok, now blend line */
			cp=kGUI::GetSurfacePtrC(lx,y);
			for(x=lx;x<=rx;++x)
			{
				weight=m_weights[x];
#if 0
				/* ok now do gamma correction */
				gindex=(int)(weight*256.0f);
				if(gindex<0)
					gindex=0;
				else if(gindex>256)
					gindex=256;
				weight=m_gamma256[gindex];
#endif
				weight*=m_alpha;
				if(weight>=1.0f)
					*(cp++)=m_color;
				else if(weight>0.0f)
				{
					bweight=1.0f-weight;

					DrawColorToRGB(*(cp),br,bg,bb);
					newr=(int)((m_red*weight)+(br*bweight));
					newg=(int)((m_green*weight)+(bg*bweight));
					newb=(int)((m_blue*weight)+(bb*bweight));
					*(cp++)=DrawColor(newr,newg,newb);
				}
				else
					++cp;
			}

		}
		++lines;
	}
}
