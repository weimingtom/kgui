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
static kGUIFPoint2 *pt;		/* vertices */

static int nact;		/* number of active edges */

static FEdge *active;	/* active edge list:edges crossing scanline y */

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
	const FEdge *eu=(const FEdge *)u;
	const FEdge *ev=(const FEdge *)v;

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

static void pinsert(int i, float y)		/* append edge i to end of active list */
{
    int j;
    float dx;
    kGUIFPoint2 *p, *q;

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

void kGUI::DrawPoly(int nvert,kGUIFPoint2 *point,kGUIColor c,float alpha)
{
    int k, i, j;
	float y, y0,y1,xl,xr;
	int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
	float rowheight;

	n = nvert;
    pt = point;

	if(n<2)
		return;
	
	/*only allocates if not enough space already */
	m_polysortint.Alloc(n,false);
	m_polysortedgef.Alloc(n,false);

	ind=m_polysortint.GetArrayPtr();
	active=m_polysortedgef.GetArrayPtr();

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)
		ind[k] = k;
	/* sort ind by pt[ind[k]].y */
	qsort(ind, n, sizeof ind[0], compare_ind);

	m_subpixcollectorf.SetColor(c,alpha);

    nact = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (valmax(m_clipcornersf.ty, pt[ind[0]].y));
										/* ymin of polygon */
    y1 = (valmin(m_clipcornersf.by, pt[ind[n-1]].y));
										/* ymax of polygon */
	m_subpixcollectorf.SetColor(c,alpha);
	m_subpixcollectorf.SetBounds(y0,y1);

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

		rowheight=0.1f;	//valmin(pt[ind[k]].y-y,1.0f);

		/* draw horizontal segments for scanline y */
		for (j=0; j<nact; j+=2)
		{ /* draw horizontal segments */
		/* span 'tween j & j+1 is inside, span tween */
		/* j+1 & j+2 is outside */
	    	xl = active[j].x;	/* left end of span */
	    	if (xl<m_clipcornersf.lx)
				xl = m_clipcornersf.lx;
	    	xr = active[j+1].x;
										/* right end of span */
	    	if (xr>m_clipcornersf.rx)
				xr = m_clipcornersf.rx;
	    	if (xl<=xr)
				m_subpixcollectorf.AddRect(xl,y,xr-xl,rowheight,1.0f);
			/* increment edge coords */
	    	active[j].x += active[j].dx*rowheight;
	    	active[j+1].x += active[j+1].dx*rowheight;
		  }
		y+=rowheight;
    }while(y<y1);
	m_subpixcollectorf.Draw();
}

void kGUI::DrawPolyLine(int nvert,kGUIFPoint2 *point,kGUIColor c)
{
	int i;
	for(i=0;i<(nvert-1);++i)
		DrawLine(point[i].x,point[i].y,point[i+1].x,point[i+1].y,c);
}

#define MAXENDPOINTS 20

static void Proj(kGUIFPoint2 *out,float x,float y,float r,float a)
{
	out->x=x-(cos(a)*r);
	out->y=y-(sin(a)*r);
}

void kGUI::DrawFatLine(float x1,float y1,float x2,float y2,kGUIColor c,float radius,float alpha)
{
	kGUIFPoint2 ends[2];

	ends[0].x=x1;
	ends[0].y=y1;
	ends[1].x=x2;
	ends[1].y=y2;
	DrawFatPolyLine(3,2,ends,c,radius,alpha);
}

static float Diff(float h1,float h2)
{
	float d;

	d=h1-h2;
	while(d>PI)
		d-=(PI*2);
	while(d<(-PI))
		d+=(PI*2);
	return(d);
}

static float Cross(kGUIFPoint2 *p1,kGUIFPoint2 *p2,kGUIFPoint2 *p3)
{
	return ( (p2->x - p1->x)*(p3->y - p1->y) - (p2->y - p1->y)*(p3->x - p1->x) );
}

/* convert to a polygon then draw using the poly code */

void kGUI::DrawFatPolyLine(unsigned int ce,unsigned int nvert,kGUIFPoint2 *point,kGUIColor c,float radius,float alpha)
{
	unsigned int i,j,numep[2],numinsidepoints,numcp,pass;
	unsigned int numout;
	float lastheading,heading,hdelta,lastlen,len;
	float step,estep[2];
	float h,dist;
	kGUIFPoint2 ip2;
	kGUIFPoint2 *p1;
	kGUIFPoint2 *p2;
	kGUIFPoint2 *op;

	if(nvert<2)
		return;

	/* last point */
	numinsidepoints=nvert-2;
	p1=point;
	p2=point+1;

	/* make the number of endpoints vary depending on thickness */
	if(ce&1)
	{
		numep[0]=valmin(MAXENDPOINTS,(int)(radius+1.0f));
		/* end point step to cover 180 degrees */
		estep[0]=PI/(numep[0]-1);
	}
	else
	{
		numep[0]=2;
		estep[0]=PI;
	}
	if(ce&2)
	{
		numep[1]=valmin(MAXENDPOINTS,(int)(radius+1.0f));
		/* end point step to cover 180 degrees */
		estep[1]=PI/(numep[1]-1);
	}
	else
	{
		numep[1]=2;
		estep[1]=PI;
	}

	m_ffatpoints.Alloc(MAXENDPOINTS*nvert*2,false);

	/* number of out points */
	numout=0;
	op=m_ffatpoints.GetArrayPtr();

	heading=atan2f(p2->y-p1->y,p2->x-p1->x);
	len=(float)hypot(p2->y-p1->y,p2->x-p1->x);
	for(pass=0;pass<2;++pass)
	{
		/* build curved end for first point */
		h=(heading-(PI/2));
		for(i=0;i<numep[pass];++i)
		{
			if(!pass)
				Proj(op,p1->x,p1->y,radius,h);
			else
				Proj(op,p2->x,p2->y,radius,h);
			++op;
			++numout;
			h+=estep[pass];
		}

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
			len=(float)hypot(p1->y-p2->y,p1->x-p2->x);
			h=(lastheading+(PI/2));
			hdelta=Diff(heading,lastheading);
			numcp=valmin(MAXENDPOINTS,abs((int)(hdelta*radius*0.35f))+3);			/* number of points inserted  for the curve curved points */
			step=hdelta/(numcp-1);

			/* is this an inside or outside angle? */
			Proj(op,p1->x,p1->y,0.0f,0.0f);
			Proj(op+1,p1->x,p1->y,radius,h);
			Proj(op+2,p1->x,p1->y,radius,h+hdelta);
			if((Cross(op,op+1,op+2)<0.0f))
			{
				dist=-(radius*.5f)*tanf(hdelta*.5f);
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
	op=m_ffatpoints.GetArrayPtr();
	DrawPoly(numout,op,c,alpha);
}

bool kGUI::DrawLine(float x1,float y1,float x2,float y2,kGUIColor c,float alpha)
{
	float dx,dy,minx,maxx,miny,maxy;

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

	m_subpixcollectorf.SetBounds(y1,y2);
	m_subpixcollectorf.SetColor(c,alpha);

	dx=x2-x1;
	dy=y2-y1;

	if(fabs(dx)>fabs(dy))
	{
		float x,stepx;
		float stepy=dy/fabs(dx);
		float y;
		float length=fabs(dx);
		float size;

		if(dx>0.0f)
			stepx=1.0f;
		else
			stepx=-1.0f;
		y=y1;
		x=x1;
		do
		{
			size=valmin(length,1.0f);
			m_subpixcollectorf.AddRect(valmin(x,x+stepx),valmin(y,y+stepy),size,size,1.0f);
			x+=stepx;
			y+=stepy;
			length-=1.0f;
		}while(length>0.0f);
	}
	else
	{
		float y,stepy;
		float stepx=dx/fabs(dy);
		float x;
		float length=fabs(dy);
		float size;

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
			size=valmin(length,1.0f);
			m_subpixcollectorf.AddRect(valmin(x,x+stepx),valmin(y,y+stepy),size,size,1.0f);
			x+=stepx;
			y+=stepy;
			length-=1.0f;
		}while(length>0.0f);
	}
	m_subpixcollectorf.Draw();
	return(true);
}

void kGUI::DrawCircle(float x,float y,float r,kGUIColor color,double alpha)
{
	int i;
	kGUIFPoint2 points[360+1];

	if(OffClip((int)(x-r),(int)(y-r),(int)(x+r),(int)(y+r))==true)
		return;

	for(i=0;i<=360;++i)
	{
		points[i].x=x+(r*sin(i*(3.141592654f/180.0f)));
		points[i].y=y+(r*cos(i*(3.141592654f/180.0f)));
	}

	if(alpha==1.0f)
		kGUI::DrawPoly(360+1,points,color);
	else
		kGUI::DrawPoly(360+1,points,color,(float)alpha);
}
