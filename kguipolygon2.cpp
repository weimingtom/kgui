/**********************************************************************************/
/* kGUI - kguipolygon2.cpp                                                        */
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

/* 
Concave Polygon Scan Conversion
by Paul Heckbert
from "Graphics Gems", Academic Press, 1990
*/

/*
 * concave: scan convert nvert-sided concave non-simple polygon
 * with vertices at (point[i].x, point[i].y) for i in 
 * [0..nvert-1] within the window win by
 * calling spanproc for each visible span of pixels.
 * Polygon can be clockwise or counterclockwise.
 * Algorithm does uniform point sampling at pixel centers.
 * Inside-outside test done by Jordan's rule: a point is 
 * considered inside if an emanating ray intersects the polygon 
 * an odd number of times.
 * drawproc should fill in pixels from xl to xr inclusive on scanline y,
 * e.g:
 *	drawproc(y, xl, xr)
 *	int y, xl, xr;
 *	{
 *	    int x;
 *	    for (x=xl; x<=xr; x++)
 *			pixel_write(x, y, pixelvalue);
 *	}
 *
 *  Paul Heckbert	30 June 81, 18 Dec 89
 */

#include "kgui.h"

static int n;			/* number of vertices */
static kGUIDPoint2 *pt;		/* vertices */

static int nact;		/* number of active edges */
static Edge *active;	/* active edge list:edges crossing scanline y */

/* comparison routines for qsort */
static int compare_ind(const void *u, const void *v)
{
	const int *iu=(const int *)u;
	const int *iv=(const int *)v;

	if(pt[*iu].y == pt[*iv].y)
		return 0;
	return pt[*iu].y < pt[*iv].y ? -1 : 1;
}

static int compare_active(const void *u, const void *v)
{
	const Edge *eu=(const Edge *)u;
	const Edge *ev=(const Edge *)v;

	if(eu->x == ev->x)
		return(0);
	return eu->x < ev->x ? -1 : 1;
}

static void pdelete(int i)		/* remove edge i from active list */
{
    int j;

    for (j=0; j<nact && active[j].i!=i; j++);
    if (j>=nact)
		return;
		/* edge not in active list; happens at win->y0*/
    nact--;
    memmove( &active[j],&active[j+1], (nact-j)*sizeof active[0]);
}

static void pinsert(int i, double y)		/* append edge i to end of active list */
{
    int j;
    double dx;
    kGUIDPoint2 *p, *q;

    j = i<n-1 ? i+1 : 0;
    if (pt[i].y < pt[j].y)
	{
		p = &pt[i]; q = &pt[j];
	}
    else
	{
		p = &pt[j];
		q = &pt[i];
	}
    /* initialize x position at intersection of edge with scanline y */
    active[nact].dx = dx = ((q->x-p->x))/((q->y-p->y));
    active[nact].x = dx*(y-p->y)+p->x;
    active[nact].i = i;
    nact++;
}

void kGUI::DrawPoly(int nvert,kGUIDPoint2 *point,kGUIColor c,double alpha)
{
    int k, i, j;
	double y, y0,y1,xl,xr;
	int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
	double rowheight;

	n = nvert;
    pt = point;

	if(n<2)
		return;
	
	/*only allocates if not enough space already */
	m_polysortint.Alloc(n,false);
	m_polysortedge.Alloc(n,false);

	ind=m_polysortint.GetArrayPtr();
	active=m_polysortedge.GetArrayPtr();

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)
		ind[k] = k;
	/* sort ind by pt[ind[k]].y */
	qsort(ind, n, sizeof ind[0], compare_ind);

	m_subpixcollector.SetColor(c,alpha);

    nact = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (MAX(m_clipcornersd.ty, pt[ind[0]].y));
										/* ymin of polygon */
    y1 = (MIN(m_clipcornersd.by, pt[ind[n-1]].y));
										/* ymax of polygon */
	m_subpixcollector.SetColor(c,alpha);
	m_subpixcollector.SetBounds(y0,y1);

	y=y0;
	do
	{
		/* step through scanlines */

		/* scanline y is at y+xROUND in continuous coordinates */
		/* Check vertices between previous scanline  */
		/* and current one, if any */

		for (; k<n && pt[ind[k]].y<=y; k++)
		{
	   		/* to simplify, if pt.y=y+xROUND, pretend it's above */
	   		/* invariant: y-.5 < pt[i].y <= y+xROUND */
	    	i = ind[k];	
	   	   /*
	     	* insert or delete edges before and after
			* vertex i  (i-1 to i, and i to i+1) from active 				* list if they cross scanline y
	    	*/
	    	j = i>0 ? i-1 : n-1;	/* vertex previous to i */
	    	if (pt[j].y <= y)
			/* old edge, remove from active list */
				pdelete(j);
	    	else if (pt[j].y > y)
			/* new edge, add to active list */
				pinsert(j, y);
	    	j = i<n-1 ? i+1 : 0;	/* vertex next after i */
	    	if (pt[j].y <= y)
			/* old edge, remove from active list */
				pdelete(i);
	    	else if (pt[j].y > y)
			/* new edge, add to active list */
				pinsert(i, y);
		}

		/* sort active edge list by active[j].x */
		qsort(active, nact, sizeof active[0], compare_active);

		rowheight=0.1f;	//MIN(pt[ind[k]].y-y,1.0f);

		/* draw horizontal segments for scanline y */
		for (j=0; j<nact; j+=2)
		{ /* draw horizontal segments */
		/* span 'tween j & j+1 is inside, span tween */
		/* j+1 & j+2 is outside */
	    	xl = active[j].x;	/* left end of span */
	    	if (xl<m_clipcornersd.lx)
				xl = m_clipcornersd.lx;
	    	xr = active[j+1].x;
										/* right end of span */
	    	if (xr>m_clipcornersd.rx)
				xr = m_clipcornersd.rx;
	    	if (xl<=xr)
				m_subpixcollector.AddRect(xl,y,xr-xl,rowheight,1.0f);
			/* increment edge coords */
	    	active[j].x += active[j].dx*rowheight;
	    	active[j+1].x += active[j+1].dx*rowheight;
		  }
		y+=rowheight;
    }while(y<y1);
	m_subpixcollector.Draw();
}

void kGUI::DrawPolyLine(int nvert,kGUIDPoint2 *point,kGUIColor c)
{
	int i;
	for(i=0;i<(nvert-1);++i)
		DrawLine(point[i].x,point[i].y,point[i+1].x,point[i+1].y,c);
}

#define MAXENDPOINTS 20

static void Proj(kGUIDPoint2 *out,double x,double y,double r,double a)
{
	out->x=x-(cos(a)*r);
	out->y=y-(sin(a)*r);
}

void kGUI::DrawFatLine(double x1,double y1,double x2,double y2,kGUIColor c,double radius,double alpha)
{
	kGUIDPoint2 ends[2];

	ends[0].x=x1;
	ends[0].y=y1;
	ends[1].x=x2;
	ends[1].y=y2;
	DrawFatPolyLine(3,2,ends,c,radius,alpha);
}

static double Diff(double h1,double h2)
{
	double d;

	d=h1-h2;
	while(d>PI)
		d-=(PI*2);
	while(d<(-PI))
		d+=(PI*2);
	return(d);
}

static double Cross(kGUIDPoint2 *p1,kGUIDPoint2 *p2,kGUIDPoint2 *p3)
{
	return ( (p2->x - p1->x)*(p3->y - p1->y) - (p2->y - p1->y)*(p3->x - p1->x) );
}

/* convert to a polygon then draw using the poly code */

void kGUI::DrawFatPolyLine(unsigned int ce,unsigned int nvert,kGUIDPoint2 *point,kGUIColor c,double radius,double alpha)
{
	unsigned int i,j,numep,numinsidepoints,numcp,pass;
	unsigned int numout;
	double lastheading,heading,hdelta,lastlen,len;
	double step,estep;
	double h,dist;
	kGUIDPoint2 ip2;
	kGUIDPoint2 *p1;
	kGUIDPoint2 *p2;
	kGUIDPoint2 *op;
	unsigned int ceb=1;

	if(nvert<2)
		return;

	{
		double minx,maxx,miny,maxy;
		CLIPOUT_DEF out1,out2;
		int ir=(int)(radius+1);

		/* check for totally off screen */
		p1=point;
		minx=maxx=p1->x;
		miny=maxy=p1->y;
		++p1;
		for(i=1;i<nvert;++i)
		{
			if(p1->x<minx)
				minx=p1->x;
			if(p1->x>maxx)
				maxx=p1->x;
			if(p1->y<miny)
				miny=p1->y;
			if(p1->y>maxy)
				maxy=p1->y;
			++p1;
		}
		if(OffClip((int)minx-ir,(int)miny-ir,(int)maxx+ir,(int)maxy+ir)==true)
			return;

		/* check for off-screen points at beginning */
		do{
			/* get outcode for the first point */
			p1=point;
			out1=GetClipOutCodeR((int)p1->x,(int)p1->y,ir);
			if(out1==CLIPOUT_ON)
				break;
			/* get outcode for the 2nd point */
			p2=point+1;
			out2=GetClipOutCodeR((int)p2->x,(int)p2->y,ir);
			
			if(out1&out2)
			{
				/* both points are off screen, so remove the first point */
				ce&=~1;
				++point;
				--nvert;
				if(nvert<2)
					return;
			}
			else
				break;
		}while(1);

		do{
			/* get outcode for the last point */
			p1=point+(nvert-1);
			out1=GetClipOutCodeR((int)p1->x,(int)p1->y,ir);
			if(out1==CLIPOUT_ON)
				break;
			/* get outcode for the 2nd to last point */
			p2=point+(nvert-2);
			out2=GetClipOutCodeR((int)p2->x,(int)p2->y,ir);
			
			if(out1&out2)
			{
				/* both points are off screen, so remove the last point */
				ce&=~2;
				--nvert;
				if(nvert<2)
					return;
			}
			else
				break;
		}while(1);
	}

	/* last point */
	numinsidepoints=nvert-2;
	p1=point;
	p2=point+1;

	/* make the number of endpoints vary depending on thickness */
	numep=MIN(MAXENDPOINTS,(int)(radius+1.0f));
	/* end point step to cover 180 degrees */
	estep=PI/(numep-1);

	m_dfatpoints.Alloc(MAXENDPOINTS*nvert*2,false);

	/* number of out points */
	numout=0;
	op=m_dfatpoints.GetArrayPtr();

	heading=atan2(p2->y-p1->y,p2->x-p1->x);
	len=hypot(p2->y-p1->y,p2->x-p1->x);
	for(pass=0;pass<2;++pass)
	{
		/* build curved end for first point */
		if(ce&ceb)
		{
			h=(heading-(PI/2));
			for(i=0;i<numep;++i)
			{
				if(!pass)
					Proj(op,p1->x,p1->y,radius,h);
				else
					Proj(op,p2->x,p2->y,radius,h);
				++op;
				++numout;
				h+=estep;
			}
		}
		ceb<<=1;

		/* ok, generate top edge */
		for(j=0;j<numinsidepoints;++j)
		{
			lastheading=heading;
			lastlen=len;
			if(pass)
			{
				--p1;
				--p2;
			}
			else
			{
				++p1;
				++p2;
			}
			if(!pass)
				heading=atan2(p2->y-p1->y,p2->x-p1->x);
			else
				heading=atan2(p1->y-p2->y,p1->x-p2->x);
			len=hypot(p1->y-p2->y,p1->x-p2->x);
			h=(lastheading+(PI/2));
			hdelta=Diff(heading,lastheading);
			numcp=MIN(MAXENDPOINTS,abs((int)(hdelta*radius*0.35f))+3);			/* number of points inserted  for the curve curved points */
			step=hdelta/(numcp-1);

			/* is this an inside or outside angle? */
			Proj(op,p1->x,p1->y,0.0f,0.0f);
			Proj(op+1,p1->x,p1->y,radius,h);
			Proj(op+2,p1->x,p1->y,radius,h+hdelta);
			if((Cross(op,op+1,op+2)<0.0f))
			{
				dist=-(radius*.5)*tan(hdelta*.5f);
				if(dist>len || dist>lastlen)
				{
					h=(heading+lastheading+(PI/2));
					/* put in a single point please */
					if(!pass)
						Proj(&ip2,p1->x,p1->y,radius,h);
					else
						Proj(&ip2,p2->x,p2->y,radius,h);
				}
				else
				{
					if(!pass)
						Proj(&ip2,p1->x,p1->y,(dist+radius)*2,h+hdelta*0.5f);
					else
						Proj(&ip2,p2->x,p2->y,(dist+radius)*2,h+hdelta*0.5f);
					h+=PI;
					for(i=0;i<numcp;++i)
					{
						Proj(op,ip2.x,ip2.y,radius,h);
						++op;
						++numout;
						h+=step;
					}
				}
			}
			else
			{
				for(i=0;i<numcp;++i)
				{
					if(!pass)
						Proj(op,p1->x,p1->y,radius,h);
					else
						Proj(op,p2->x,p2->y,radius,h);
					++op;
					++numout;
					h+=step;
				}
			}
		}

		heading+=PI;	/* go back 180 degrees */
	}
	assert(numout<=(MAXENDPOINTS*nvert*2),"Not enough allocated error!");
	op=m_dfatpoints.GetArrayPtr();
	DrawPoly(numout,op,c,alpha);
}

bool kGUI::DrawLine(double x1,double y1,double x2,double y2,kGUIColor c,double alpha)
{
	double dx,dy,minx,maxx,miny,maxy;

	if(x1<=x2)
	{
		minx=x1;
		maxx=x2;
	}
	else
	{
		maxx=x1;
		minx=x2;
	}
	if(y1<=y2)
	{
		miny=y1;
		maxy=y2;
	}
	else
	{
		maxy=y1;
		miny=y2;
	}
	if(OffClip((int)minx,(int)miny,(int)maxx,(int)maxy)==true)
		return(false);

	m_subpixcollector.SetBounds(y1,y2);
	m_subpixcollector.SetColor(c,alpha);

	dx=x2-x1;
	dy=y2-y1;

	if(fabs(dx)>fabs(dy))
	{
		double x,stepx;
		double stepy=dy/fabs(dx);
		double y;
		double length=fabs(dx);
		double size;

		if(dx>0.0f)
			stepx=1.0f;
		else
			stepx=-1.0f;
		y=y1;
		x=x1;
		do
		{
			size=MIN(length,1.0f);
			m_subpixcollector.AddRect(MIN(x,x+stepx),MIN(y,y+stepy),size,size,1.0f);
			x+=stepx;
			y+=stepy;
			length-=1.0f;
		}while(length>0.0f);
	}
	else
	{
		double y,stepy;
		double stepx=dx/fabs(dy);
		double x;
		double length=fabs(dy);
		double size;

		if(dy>0)
			stepy=1.0f;
		else if(dy<0)
			stepy=-1.0f;
		else
			stepy=0.0f;
		x=x1;
		y=y1;
		do
		{
			size=MIN(length,1.0f);
			m_subpixcollector.AddRect(MIN(x,x+stepx),MIN(y,y+stepy),size,size,1.0f);
			x+=stepx;
			y+=stepy;
			length-=1.0f;
		}while(length>0.0f);
	}
	m_subpixcollector.Draw();
	return(true);
}
