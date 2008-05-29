/**********************************************************************************/
/* kGUI - kguipolygon.                                                            */
/*                                                                                */
/* Programmed by (See below)                                                      */
/*                                                                                */
/* http://www.scale18.com/cgi-bin/page/kgui.html	                              */
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
#include <math.h>

#ifndef PI
#define PI 3.1415926535f
#endif

#if !defined(max)
#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

#define ALLOC(ptr, type, n) \
		  ASSERT(ptr = (type *)malloc((n)*sizeof(type)))

typedef struct {		/* a polygon edge */
    double x;	/* x coordinate of edge's intersection with current scanline */
    double dx;	/* change in x with respect to y */
    int i;	/* edge number: edge i goes from pt[i] to pt[i+1] */
} Edge;

static int n;			/* number of vertices */
static kGUIPoint2 *pt;		/* vertices */

static int nact;		/* number of active edges */
static Edge *active;	/* active edge list:edges crossing scanline y */

#if 1
/* if inputs are integers, then use these defines */
#define xfloor
#define xceil
#define xROUND 0
#else
/* if inputs are floats or doubles, then use these defines */
#define xfloor floor
#define xceil ceil
#define xROUND .5
#endif

/* comparison routines for qsort */
int compare_ind(const void *u, const void *v)
{
	const int *iu=(const int *)u;
	const int *iv=(const int *)v;

	if(pt[*iu].y == pt[*iv].y)
		return 0;
	return pt[*iu].y < pt[*iv].y ? -1 : 1;
}

int compare_active(const void *u, const void *v)
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

static void pinsert(int i, int y)		/* append edge i to end of active list */
{
    int j;
    double dx;
    kGUIPoint2 *p, *q;

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
    active[nact].dx = dx = ((double)(q->x-p->x))/((double)(q->y-p->y));
    active[nact].x = dx*(y+xROUND-p->y)+p->x;
    active[nact].i = i;
    nact++;
}

void kGUI::DrawPoly(int nvert,kGUIPoint2 *point,kGUIColor c)
{
    int k, y0, y1, y, i, j, xl, xr;
    int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
	kGUIColor *cp;
	kGUIColor *cpe;

    n = nvert;
    pt = point;

    assert (n>1,"not enough points in polygon!");
	
	ind=new int[n];
	active = new Edge[n];

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)
		ind[k] = k;
	/* sort ind by pt[ind[k]].y */
    qsort(ind, n, sizeof ind[0], compare_ind);

    nact = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (int)(max(m_clipcorners.ty, xceil(pt[ind[0]].y-.5)));
										/* ymin of polygon */
    y1 = (int)(min(m_clipcorners.by, xfloor(pt[ind[n-1]].y-.5)));
										/* ymax of polygon */

    for (y=y0; y<=y1; y++)
	{  /* step through scanlines */

		/* scanline y is at y+xROUND in continuous coordinates */
		/* Check vertices between previous scanline  */
		/* and current one, if any */

		for (; k<n && pt[ind[k]].y<=y+xROUND; k++)
		{
	   		/* to simplify, if pt.y=y+xROUND, pretend it's above */
	   		/* invariant: y-.5 < pt[i].y <= y+xROUND */
	    	i = ind[k];	
	   	   /*
	     	* insert or delete edges before and after
			* vertex i  (i-1 to i, and i to i+1) from active 				* list if they cross scanline y
	    	*/
	    	j = i>0 ? i-1 : n-1;	/* vertex previous to i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(j);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(j, y);
	    	j = i<n-1 ? i+1 : 0;	/* vertex next after i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(i);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(i, y);
		}

		/* sort active edge list by active[j].x */
		qsort(active, nact, sizeof active[0], compare_active);

		/* draw horizontal segments for scanline y */
		for (j=0; j<nact; j+=2)
		{ /* draw horizontal segments */
		/* span 'tween j & j+1 is inside, span tween */
		/* j+1 & j+2 is outside */
	    	xl = (int)(ceil(active[j].x-.5));	/* left end of span */
	    	if (xl<m_clipcorners.lx)
				xl = m_clipcorners.lx;
	    	xr = (int)(floor(active[j+1].x-.5));
										/* right end of span */
	    	if (xr>=m_clipcorners.rx)
				xr = m_clipcorners.rx-1;
	    	if (xl<=xr)
			{
				/* draw pixels in span */
				cp=kGUI::GetSurfacePtrC(xl,y);
				if(cp)
				{
					cpe=cp+((xr+1)-xl);
					while(cp!=cpe)
						*(cp++)=c;
				}
			}
			/* increment edge coords */
	    	active[j].x += active[j].dx;
	    	active[j+1].x += active[j+1].dx;
		  }
    }
	delete ind;
	delete active;
}

void kGUI::DrawPoly(int nvert,kGUIPoint2 *point,kGUIColor c,double alpha)
{
    int k, y0, y1, y, i, j, xl, xr;
    int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
	kGUIColor *cp;
	kGUIColor *cpe;
	int dr,dg,db,br,bg,bb;
	int newr,newg,newb;
	double balpha=1.0f-alpha;
	int dra,dga,dba;

	DrawColorToRGB(c,dr,dg,db);
	dra=(int)(dr*alpha);
	dga=(int)(dg*alpha);
	dba=(int)(db*alpha);

	n = nvert;
    pt = point;

    assert (n>1,"not enough points in polygon!");
	
	ind=new int[n];
	active = new Edge[n];

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)
		ind[k] = k;
	/* sort ind by pt[ind[k]].y */
	qsort(ind, n, sizeof ind[0], compare_ind);

    nact = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (int)(max(m_clipcorners.ty, xceil(pt[ind[0]].y-.5)));
										/* ymin of polygon */
    y1 = (int)(min(m_clipcorners.by, xfloor(pt[ind[n-1]].y-.5)));
										/* ymax of polygon */

    for (y=y0; y<=y1; y++)
	{  /* step through scanlines */

		/* scanline y is at y+xROUND in continuous coordinates */
		/* Check vertices between previous scanline  */
		/* and current one, if any */

		for (; k<n && pt[ind[k]].y<=y+xROUND; k++)
		{
	   		/* to simplify, if pt.y=y+xROUND, pretend it's above */
	   		/* invariant: y-.5 < pt[i].y <= y+xROUND */
	    	i = ind[k];	
	   	   /*
	     	* insert or delete edges before and after
			* vertex i  (i-1 to i, and i to i+1) from active 				* list if they cross scanline y
	    	*/
	    	j = i>0 ? i-1 : n-1;	/* vertex previous to i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(j);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(j, y);
	    	j = i<n-1 ? i+1 : 0;	/* vertex next after i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(i);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(i, y);
		}

		/* sort active edge list by active[j].x */
		qsort(active, nact, sizeof active[0], compare_active);

		/* draw horizontal segments for scanline y */
		for (j=0; j<nact; j+=2)
		{ /* draw horizontal segments */
		/* span 'tween j & j+1 is inside, span tween */
		/* j+1 & j+2 is outside */
	    	xl = (int)(ceil(active[j].x-.5));	/* left end of span */
	    	if (xl<m_clipcorners.lx)
				xl = m_clipcorners.lx;
	    	xr = (int)(floor(active[j+1].x-.5));
										/* right end of span */
	    	if (xr>=m_clipcorners.rx)
				xr = m_clipcorners.rx-1;
	    	if (xl<=xr)
			{
				/* draw pixels in span */
				cp=kGUI::GetSurfacePtrC(xl,y);
				if(cp)
				{
					cpe=cp+((xr+1)-xl);
					while(cp!=cpe)
					{
						DrawColorToRGB(cp[0],br,bg,bb);
			
						newr=dra+(int)(br*balpha);
						newg=dga+(int)(bg*balpha);
						newb=dba+(int)(bb*balpha);
		
						*(cp++)=DrawColor(newr,newg,newb);
					}
				}
			}
			/* increment edge coords */
	    	active[j].x += active[j].dx;
	    	active[j+1].x += active[j+1].dx;
		  }
    }
	delete ind;
	delete active;
}


/* read the screen and return false if any pixel is not "c" */

bool kGUI::ReadPoly(int nvert,kGUIPoint2 *point,kGUIColor c)
{
	bool rc=true;
    int k, y0, y1, y, i, j, xl, xr;
    int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
	kGUIColor *cp;
	kGUIColor *cpe;

    n = nvert;
    pt = point;
    assert (n>1,"not enough points in polygon!");

	ind=new int[n];
	active = new Edge[n];

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)
		ind[k] = k;
    qsort(ind, n, sizeof ind[0], compare_ind);
							/* sort ind by pt[ind[k]].y */

    nact = 0;				/* start with empty active list */
    k = 0;				/* ind[k] is next vertex to process */
    y0 = (int)(max(m_clipcorners.ty, xceil(pt[ind[0]].y-.5)));
										/* ymin of polygon */
    y1 = (int)(min(m_clipcorners.by, xfloor(pt[ind[n-1]].y-.5)));
										/* ymax of polygon */

    for (y=y0; y<=y1; y++) {  /* step through scanlines */

		/* scanline y is at y+xROUND in continuous coordinates */
		/* Check vertices between previous scanline  */
		/* and current one, if any */

		for (; k<n && pt[ind[k]].y<=y+xROUND; k++) {
	   		/* to simplify, if pt.y=y+xROUND, pretend it's above */
	   		/* invariant: y-.5 < pt[i].y <= y+xROUND */
	    	i = ind[k];	
	   	   /*
	     	* insert or delete edges before and after
			* vertex i  (i-1 to i, and i to i+1) from active 				* list if they cross scanline y
	    	*/
	    	j = i>0 ? i-1 : n-1;	/* vertex previous to i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(j);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(j, y);
	    	j = i<n-1 ? i+1 : 0;	/* vertex next after i */
	    	if (pt[j].y <= y-.5)
			/* old edge, remove from active list */
				pdelete(i);
	    	else if (pt[j].y > y+xROUND)
			/* new edge, add to active list */
				pinsert(i, y);
		}

		/* sort active edge list by active[j].x */
		qsort(active, nact, sizeof active[0], compare_active);

		/* draw horizontal segments for scanline y */
		for (j=0; j<nact; j+=2) { /* draw horizontal segments */
		/* span 'tween j & j+1 is inside, span tween */
		/* j+1 & j+2 is outside */
	    	xl = (int)(ceil(active[j].x-.5));	/* left end of span */
	    	if (xl<m_clipcorners.lx) xl = m_clipcorners.lx;
	    	xr = (int)(floor(active[j+1].x-.5));
										/* right end of span */
	    	if (xr>=m_clipcorners.rx) xr = m_clipcorners.rx-1;
	    	if (xl<=xr)
			{
				/* read pixels in span */
				cp=kGUI::GetSurfacePtrC(xl,y);
				if(cp)
				{
					cpe=cp+((xr+1)-xl);
					while(cp!=cpe)
					{
						if(*(cp++)!=c)
						{
							rc=false;
							goto done;
						}
					}
				}
			}
	    	active[j].x += active[j].dx;
									/* increment edge coords */
	    	active[j+1].x += active[j+1].dx;
		  }
    }
done:;
	delete ind;
	delete active;
	return(rc);
}

void kGUI::DrawPolyLine(int nvert,kGUIPoint2 *point,kGUIColor c)
{
	int i;
	for(i=0;i<(nvert-1);++i)
		DrawLine((int)point[i].x,(int)point[i].y,(int)point[i+1].x,(int)point[i+1].y,c);
}

/* convert to a polygon then draw using the poly code */

void kGUI::DrawFatPolyLine(unsigned int nvert,kGUIPoint2 *point,kGUIColor c,int width,double alpha)
{
	unsigned int i;
	double dx,dy,ldx,ldy;
	double heading;
	kGUIPoint2 *nplist;
	kGUIPoint2 *np1;
	kGUIPoint2 *np2;
	kGUIText t;
	int awidth,bwidth;	/* above, below */

	awidth=width>>1;
	bwidth=width-awidth;

	if(m_fatpoints.GetNumEntries()<(nvert<<1))
		m_fatpoints.Alloc(nvert<<1,false);

	nplist=m_fatpoints.GetArrayPtr();
	np1=nplist;
	np2=nplist+(nvert<<1);

	/* get heading of first section */
	dy=point[1].y-point[0].y;
	dx=point[0].x-point[1].x;
	heading=atan2(dy,dx)-(2.0f*PI*0.25f);	/* perpendicular */
	np1->x=(int)(point[0].x+cos(heading)*awidth);
	np1->y=(int)(point[0].y-sin(heading)*awidth);
	++np1;
	--np2;
	np2->x=(int)(point[0].x-cos(heading)*bwidth);
	np2->y=(int)(point[0].y+sin(heading)*bwidth);
	/* average 2 headings for each one in between */
	ldx=dx;
	ldy=dy;
	for(i=1;i<(nvert-1);++i)
	{
		dy=point[i+1].y-point[i].y;
		dx=point[i].x-point[i+1].x;
		heading=atan2(dy+ldy,dx+ldx)-(2.0f*PI*0.25f);
		np1->x=(int)(point[i].x+cos(heading)*awidth);
		np1->y=(int)(point[i].y-sin(heading)*awidth);
		++np1;
		--np2;
		np2->x=(int)(point[i].x-cos(heading)*bwidth);
		np2->y=(int)(point[i].y+sin(heading)*bwidth);
		ldx=dx;
		ldy=dy;
	}
	/* do last point */
	dy=point[nvert-1].y-point[nvert-2].y;
	dx=point[nvert-2].x-point[nvert-1].x;
	heading=atan2(dy,dx)-(2.0f*PI*0.25f);	/* perpendicular */
	np1->x=(int)(point[nvert-1].x+cos(heading)*awidth);
	np1->y=(int)(point[nvert-1].y-sin(heading)*awidth);
	--np2;
	np2->x=(int)(point[nvert-1].x-cos(heading)*bwidth);
	np2->y=(int)(point[nvert-1].y+sin(heading)*bwidth);

	if(alpha==1.0f)
		DrawPoly(nvert<<1,nplist,c);
	else
		DrawPoly(nvert<<1,nplist,c,alpha);
}

//
//  The function will return TRUE if the point x,y is inside the
//  polygon, or FALSE if it is not. If the point x,y is exactly on
//  the edge of the polygon, then the function may return TRUE or
//  FALSE.
//
//  Note that division by zero is avoided because the division is
//  protected by the "if" clause which surrounds it.

bool kGUI::PointInsidePoly(double px,double py,int nvert,kGUIPoint2 *point)
{
	int	i,j=0;
	bool odd=false;

	for (i=0; i<nvert; i++)
	{
	    j++;
		if (j==nvert)
			j=0;
		if (point[i].y<py && point[j].y>=py ||  point[j].y<py && point[i].y>=py)
		{
			if (point[i].x+(py-point[i].y)/(point[j].y-point[i].y)*(point[j].x-point[i].x)<px)
			{
		        odd=!odd;
			}
		}
	}
	return (odd);
}

bool kGUI::PointInsidePoly(double px,double py,int nvert,kGUIDPoint2 *point)
{
	int	i,j=0;
	bool odd=false;

	for (i=0; i<nvert; i++)
	{
	    j++;
		if (j==nvert)
			j=0;
		if (point[i].y<py && point[j].y>=py ||  point[j].y<py && point[i].y>=py)
		{
			if (point[i].x+(py-point[i].y)/(point[j].y-point[i].y)*(point[j].x-point[i].x)<px)
			{
		        odd=!odd;
			}
		}
	}
	return (odd);
}
