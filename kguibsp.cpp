/**********************************************************************************/
/* kGUI - kguibsp.cpp                                                             */
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

#include "kgui.h"
#include "kguibsp.h"

/* this class is for BSP partitioning quads */

kGUIBSPRect::kGUIBSPRect()
{
	m_numalloc=0;
	m_numalloc4=0;
	m_numzones=0;
	m_rc=0;
	m_prc=0;
	m_zones=0;
	m_drawzones=0;
}

kGUIBSPRect::~kGUIBSPRect()
{
	if(m_rc)
		delete []m_rc;
	if(m_prc)
		delete []m_prc;
	if(m_zones)
		delete []m_zones;
	if(m_drawzones)
		delete []m_drawzones;
}

void kGUIBSPRect::Alloc(int n,int c)
{
	m_numentries=0;
	if(n>m_numalloc)
	{
		if(m_rc)
			delete []m_rc;
		if(m_prc)
			delete []m_prc;
		m_numalloc=n;
		m_numalloc4=n<<2;
		m_rc=new kGUIBSPCornerEntry[m_numalloc4];
		m_prc=new kGUIBSPCornerEntry *[m_numalloc4];
	}

	if(c>m_numzones)
	{
		if(m_zones)
			delete []m_zones;
		if(m_drawzones)
			delete []m_drawzones;

		m_numzones=c;
		m_zones=new kGUIBSPZoneEntry[1<<c];
		/* array of pointers to the above table for ones that were selected */
		m_drawzones=new kGUIBSPZoneEntry *[1<<c];
	}
	m_currc=m_rc;
	m_curprc=m_prc;

	m_numcuts=c;
	m_curzones=m_zones;
}

void kGUIBSPRect::AddEntry(kGUIBSPRectEntry *re)
{
	assert(m_numentries<m_numalloc4,"Buffer Overflow!");

	m_currc[0].m_c[0]=re->m_c.lx;
	m_currc[0].m_c[1]=re->m_c.ty;
	m_currc[0].m_rp=re;

	m_currc[1].m_c[0]=re->m_c.rx;
	m_currc[1].m_c[1]=re->m_c.ty;
	m_currc[1].m_rp=re;

	m_currc[2].m_c[0]=re->m_c.lx;
	m_currc[2].m_c[1]=re->m_c.by;
	m_currc[2].m_rp=re;

	m_currc[3].m_c[0]=re->m_c.rx;
	m_currc[3].m_c[1]=re->m_c.by;
	m_currc[3].m_rp=re;

	m_curprc[0]=m_currc;
	m_curprc[1]=m_currc+1;
	m_curprc[2]=m_currc+2;
	m_curprc[3]=m_currc+3;
	m_curprc+=4;
	m_currc+=4;
	m_numentries+=4;
}

//int kGUIBSPRect::m_sortaxis;

//our sort data is an array of pointers
int kGUIBSPRect::Sort0(const void *v1,const void *v2)
{
	kGUIBSPCornerEntry * const * e1;
	kGUIBSPCornerEntry * const * e2;

	e1=static_cast<kGUIBSPCornerEntry * const *>(v1);
	e2=static_cast<kGUIBSPCornerEntry * const *>(v2);

	return (e1[0]->m_c[0]-e2[0]->m_c[0]);
}

//our sort data is an array of pointers
int kGUIBSPRect::Sort1(const void *v1,const void *v2)
{
	kGUIBSPCornerEntry * const * e1;
	kGUIBSPCornerEntry * const * e2;

	e1=static_cast<kGUIBSPCornerEntry * const *>(v1);
	e2=static_cast<kGUIBSPCornerEntry * const *>(v2);

	return (e1[0]->m_c[1]-e2[0]->m_c[1]);
}


kGUIBSPZoneEntry *kGUIBSPRect::CutAxis(int start,int end,int depth)
{
	int num,axis;
	int v;
	kGUIBSPCornerEntry **prc;
	kGUIBSPZoneEntry *zone;

	/* get a zone */
	zone=m_curzones++;
	/* step 1, calculate the axis that covers the most area */
	zone->m_start=start;
	zone->m_end=end;
	zone->m_left=0;
	zone->m_right=0;

	if(!end)
		return(zone);
	for(axis=0;axis<2;++axis)
	{
		prc=m_prc+start;
		num=end-start;
		zone->m_minc[axis]=prc[0]->m_c[axis];
		zone->m_maxc[axis]=prc[0]->m_c[axis];
		while(num>0)
		{
			v=prc[0]->m_c[axis];
			if(v<zone->m_minc[axis])
				zone->m_minc[axis]=v;
			if(v>zone->m_maxc[axis])
				zone->m_maxc[axis]=v;
			++prc;
			--num;
		}
	}

	/* is this the last level? if so, no need to sort, just save */
	/* the bounding area and start/end indices */
	num=end-start;
	if(depth && (num>(32*4)))
	{
		int center=(end+start)>>1;
		/* 2. pick the axis that spans the largest area */
		if((zone->m_maxc[0]-zone->m_minc[0])>(zone->m_maxc[1]-zone->m_minc[1]))
			axis=0;
		else
			axis=1;

		/* testing... */
		//axis=0;

//		m_sortaxis=axis;
		qsort(m_prc+start,end-start,sizeof(kGUIBSPCornerEntry *),axis==0?Sort0:Sort1);
		/* 4. split zone in 2 and call recursively */
		zone->m_left=CutAxis(start,center,depth-1);
		zone->m_right=CutAxis(center,end,depth-1);
	}
	return(zone);
}

void kGUIBSPRect::Select(kGUICorners *c)
{
	m_wpdrawzones=m_drawzones;	/* write pointer */
	m_rpdrawzones=m_drawzones;	/* read pointer */

	/* check to make sure table has items in it first */
	if(m_zones[0].m_start!=m_zones[0].m_end)
	{
		SelectZone(c,m_zones);
		if(m_rpdrawzones!=m_wpdrawzones)
			m_readcurrent=m_rpdrawzones[0]->m_start;
	}
}

void kGUIBSPRect::SelectZone(kGUICorners *c,kGUIBSPZoneEntry *zone)
{
	/* 1. compare zone against corners, if it is outside then return */
	if(c->lx>=zone->m_maxc[0])
		return;	/* off right */
	if(c->rx<=zone->m_minc[0])
		return;	/* off left */
	if(c->ty>=zone->m_maxc[1])
		return;	/* off bottom */
	if(c->by<=zone->m_minc[1])
		return;	/* off top */

	if(!zone->m_left)	/* no more levels to check, so add it */
		goto addzone;
	if((zone->m_minc[0]>=c->lx) && (zone->m_maxc[0]<=c->rx) && (zone->m_minc[1]>=c->ty) && (zone->m_maxc[1]<=c->by))
	{
		/* 2. if it is totally inside then add all records in this zone */
addzone:;
		*(m_wpdrawzones++)=zone;
	}
	else
	{
		/* 4. only partially inside the zone, call the children */
		SelectZone(c,zone->m_left);
		SelectZone(c,zone->m_right);
	}
}

kGUIBSPRectEntry *kGUIBSPRect::GetEntry(void)
{
	kGUIBSPZoneEntry *zone;
	kGUIBSPRectEntry *entry;

	if(m_rpdrawzones==m_wpdrawzones)
		return(0);	/* done */
	
	zone=m_rpdrawzones[0];
	entry=m_prc[m_readcurrent++]->m_rp;
	if(m_readcurrent==zone->m_end)
	{
		++m_rpdrawzones;
		if(m_rpdrawzones!=m_wpdrawzones)
			m_readcurrent=m_rpdrawzones[0]->m_start;
	}
	return(entry);
}

/* this class is for BSP partitioning points */

kGUIBSPPoint::kGUIBSPPoint()
{
	m_numalloc=0;
	m_numzones=0;
	m_prc=0;
	m_zones=0;
	m_drawzones=0;
}

kGUIBSPPoint::~kGUIBSPPoint()
{
	if(m_prc)
		delete []m_prc;
	if(m_zones)
		delete []m_zones;
	if(m_drawzones)
		delete []m_drawzones;
}

void kGUIBSPPoint::Alloc(int n,int c)
{
	m_numentries=0;
	if(n>m_numalloc)
	{
		if(m_prc)
			delete []m_prc;
		m_numalloc=n;
		m_prc=new kGUIBSPPointEntry *[m_numalloc];
	}

	if(c>m_numzones)
	{
		if(m_zones)
			delete []m_zones;
		if(m_drawzones)
			delete []m_drawzones;

		m_numzones=c;
		m_zones=new kGUIBSPZoneEntry[1<<c];
		/* array of pointers to the above table for ones that were selected */
		m_drawzones=new kGUIBSPZoneEntry *[1<<c];
	}
	m_curprc=m_prc;

	m_numcuts=c;
	m_curzones=m_zones;
}

void kGUIBSPPoint::AddEntry(kGUIBSPPointEntry *re)
{
	assert(m_numentries<m_numalloc,"Buffer Overflow!");

	m_curprc[0]=re;
	++m_curprc;
	++m_numentries;
}

//int kGUIBSPPoint::m_sortaxis;

//out sort data is an array of pointers
int kGUIBSPPoint::Sort0(const void *v1,const void *v2)
{
	kGUIBSPPointEntry * const * e1;
	kGUIBSPPointEntry * const * e2;

	e1=static_cast<kGUIBSPPointEntry * const *>(v1);
	e2=static_cast<kGUIBSPPointEntry * const *>(v2);

	return (e1[0]->m_c[0]-e2[0]->m_c[0]);
}

int kGUIBSPPoint::Sort1(const void *v1,const void *v2)
{
	kGUIBSPPointEntry * const * e1;
	kGUIBSPPointEntry * const * e2;

	e1=static_cast<kGUIBSPPointEntry * const *>(v1);
	e2=static_cast<kGUIBSPPointEntry * const *>(v2);

	return (e1[0]->m_c[1]-e2[0]->m_c[1]);
}

kGUIBSPZoneEntry *kGUIBSPPoint::CutAxis(int start,int end,int depth)
{
	int num,axis;
	int v;
	kGUIBSPPointEntry **prc;
	kGUIBSPZoneEntry *zone;

	/* get a zone */
	zone=m_curzones++;
	/* step 1, calculate the axis that covers the most area */
	zone->m_start=start;
	zone->m_end=end;
	zone->m_left=0;
	zone->m_right=0;

	if(!end)
		return(zone);
	for(axis=0;axis<2;++axis)
	{
		prc=m_prc+start;
		num=end-start;
		zone->m_minc[axis]=prc[0]->m_c[axis];
		zone->m_maxc[axis]=prc[0]->m_c[axis];
		while(num>0)
		{
			v=prc[0]->m_c[axis];
			if(v<zone->m_minc[axis])
				zone->m_minc[axis]=v;
			if(v>zone->m_maxc[axis])
				zone->m_maxc[axis]=v;
			++prc;
			--num;
		}
	}

	/* is this the last level? if so, no need to sort, just save */
	/* the bounding area and start/end indices */
	num=end-start;
	if(depth && (num>32))
	{
		int center=(end+start)>>1;
		/* 2. pick the axis that spans the largest area */
		if((zone->m_maxc[0]-zone->m_minc[0])>(zone->m_maxc[1]-zone->m_minc[1]))
			axis=0;
		else
			axis=1;

		/* testing... */
		//axis=0;

//		m_sortaxis=axis;
		qsort(m_prc+start,end-start,sizeof(kGUIBSPPoint *),axis==0?Sort0:Sort1);

		/* 4. split zone in 2 and call recursively */
		zone->m_left=CutAxis(start,center,depth-1);
		zone->m_right=CutAxis(center,end,depth-1);
	}
	return(zone);
}

void kGUIBSPPoint::Select(kGUICorners *c)
{
	m_wpdrawzones=m_drawzones;	/* write pointer */
	m_rpdrawzones=m_drawzones;	/* read pointer */

	/* check to make sure table has items in it first */
	if(m_zones[0].m_start!=m_zones[0].m_end)
	{
		SelectZone(c,m_zones);
		if(m_rpdrawzones!=m_wpdrawzones)
			m_readcurrent=m_rpdrawzones[0]->m_start;
	}
}

void kGUIBSPPoint::SelectZone(kGUICorners *c,kGUIBSPZoneEntry *zone)
{
	/* 1. compare zone against corners, if it is outside then return */
	if(c->lx>=zone->m_maxc[0])
		return;	/* off right */
	if(c->rx<=zone->m_minc[0])
		return;	/* off left */
	if(c->ty>=zone->m_maxc[1])
		return;	/* off bottom */
	if(c->by<=zone->m_minc[1])
		return;	/* off top */

	if(!zone->m_left)	/* no more levels to check, so add it */
		goto addzone;
	if((zone->m_minc[0]>=c->lx) && (zone->m_maxc[0]<=c->rx) && (zone->m_minc[1]>=c->ty) && (zone->m_maxc[1]<=c->by))
	{
		/* 2. if it is totally inside then add all records in this zone */
addzone:;
		*(m_wpdrawzones++)=zone;
	}
	else
	{
		/* 4. only partially inside the zone, call the children */
		SelectZone(c,zone->m_left);
		SelectZone(c,zone->m_right);
	}
}

kGUIBSPPointEntry *kGUIBSPPoint::GetEntry(void)
{
	kGUIBSPZoneEntry *zone;
	kGUIBSPPointEntry *entry;

	if(m_rpdrawzones==m_wpdrawzones)
		return(0);	/* done */
	
	zone=m_rpdrawzones[0];
	entry=m_prc[m_readcurrent++];
	if(m_readcurrent==zone->m_end)
	{
		++m_rpdrawzones;
		if(m_rpdrawzones!=m_wpdrawzones)
			m_readcurrent=m_rpdrawzones[0]->m_start;
	}
	return(entry);
}
