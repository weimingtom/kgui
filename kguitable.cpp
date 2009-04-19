/*********************************************************************************/
/* kGUI - kguitable.cpp                                                          */
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

/*! @file kguitable.cpp
    @brief This is the table class. It is used for tables, doh! It is also used by kgui  
 for rendering comboboxes, listboxes and popup menus.                          
 A table holds a list of table row objects and each of those holds a list of   
 of regular kgui objects that are used for the cells.                          
                                                                               
 Tables handle scrolling up/down/left/right                                    
 Adding and deleteing rows                                                     
 Selecting and unselecting rows                                                
 Rearranging columns and hide/show columns (also save/load settings in XML)    
 Callbacks can be trigerred as the user moves the cursor around in the table   
 and also as table data is edited. */

#include "kgui.h"

#include "_text.h"

/***************** edit table column class ***************************/
/* this is the window that pops up when you right click on the       */
/* top left of the table.                                            */
/* it allows you to hide/show the various columns, re-order them and */
/* adjust their widths.                                              */
/*********************************************************************/

class kGUIEditTableColObj
{
public:
	kGUIEditTableColObj(kGUITableObj *table);
private:
	CALLBACKGLUEPTR(kGUIEditTableColObj,WindowEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIEditTableColObj,PressUp,kGUIEvent)
	CALLBACKGLUEPTR(kGUIEditTableColObj,PressDown,kGUIEvent)
	void WindowEvent(kGUIEvent *event);
	void PressUp(kGUIEvent *event);
	void PressDown(kGUIEvent *event);
	void Reload(void);

	int m_numcols;
	kGUIWindowObj m_window;
	kGUIButtonObj m_up;
	kGUIButtonObj m_down;
	kGUITableObj m_table;	
	kGUITableObj *m_t;		/* pointer to table being adjusted */
};

class kGUIEditTableColRowObj : public kGUITableRowObj
{
public:
	kGUIEditTableColRowObj(kGUITableObj *t,int col);
	inline int GetNumObjects(void) {return 3;}
	kGUIObj **GetObjectList(void) {return m_objptrs;}
	void Reload(void);
private:
	CALLBACKGLUEPTR(kGUIEditTableColRowObj,ChangeWidth,kGUIEvent)
	CALLBACKGLUEPTR(kGUIEditTableColRowObj,ChangeShow,kGUIEvent)
	void ChangeShow(kGUIEvent *event);
	void ChangeWidth(kGUIEvent *event);
	int m_numobjs;
	kGUITableObj *m_t;
	int m_col,m_xcol;
	kGUIObj *m_objptrs[3];

	kGUITickBoxObj m_tick;
	kGUIInputBoxObj m_width;
	kGUIInputBoxObj m_name;
};

/***************** table object functions **********************/

kGUITableObj::kGUITableObj()
{
	kGUIContainerObj::SetNumGroups(1);
	m_editmode=TABLEEDIT_NONE;
	m_numrows=0;
	m_numcols=0;
	m_allownew=false;
	m_lastselectedrow=-1;
	/* current cursor position */
	m_cursorrow=0;
	m_cursorcol=0;
	m_colheaderheight=0;
	m_allowmultiple=true;	/* default to allow multiple selections */
	m_releasecount=0;

	m_leftcol=0;
	m_rightcol=0;
	m_toprow=0;
	m_botrow=0;

	m_drawtoprow=0;
	m_drawbotrow=0;
	m_drawleftcol=0;
	m_drawrightcol=0;

	m_sizechanged=true;
	m_positionsdirty=true;
	m_rowscrollbar.SetParent(this);
	m_rowscrollbar.SetVert();
	m_colscrollbar.SetParent(this);
	m_colscrollbar.SetHorz();
	m_colscrollbar.SetEventHandler(this,& CALLBACKNAME(ScrollMoveCol));
	m_rowscrollbar.SetEventHandler(this,& CALLBACKNAME(ScrollMoveRow));

	/* set defaults, these can be turned off by the user */
	m_showrowscrollbar=true;
	m_showcolscrollbar=true;
	m_showcolheaders=true;
	m_showrowheaders=true;
	m_poprowheaders=false;
	m_selectmode=false;		/* table is being used for a combo-box */
	m_listmode=false;		/* table is being used for a listbox */
	m_allowdelete=true;
	m_wasover=false;
	m_lockrow=false;
	m_lockcol=false;
	m_allowadjrowheights=true;
	m_allowadjcolwidths=true;
	m_viewadd=false;
	m_wasoff=false;

	/* one entry per row, onlt used for select mode tables ( combo box/menus etc ) */
	m_available.SetGrow(true);
	m_scroll.SetEventHandler(this,CALLBACKNAME(Scrolled));
}

int kGUITableObj::GetRowSelectorWidth(void)
{
	return kGUI::GetSkin()->GetTableRowHeaderWidth();
}

int kGUITableObj::CalcTableWidth(void)
{
	unsigned int col;
	unsigned int xcol;
	int w;

	if(m_showcolheaders==true)
		w=kGUI::GetSkin()->GetTableRowHeaderWidth();
	else
		w=2;
	for(col=0;col<m_numcols;++col)
	{
		xcol=m_colorder.GetEntry(col);
		if(m_showcols.GetEntry(xcol)==true)
			w+=m_colwidths.GetEntry(xcol);
	}
	if(m_showcolscrollbar)
		w+=kGUI::GetSkin()->GetScrollbarWidth();

	w+=2;
	return(w);
}

int kGUITableObj::CalcTableHeight(void)
{
	unsigned int i;
	int h;

	h=0;
	if(m_showcolheaders==true)
		h+=m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
	for(i=0;i<m_numrows;++i)
	{
		kGUITableRowObj *rowobj;

		rowobj=static_cast<kGUITableRowObj *>(GetChild(i));
		/* get the height of this row */
		h+=rowobj->GetRowHeight();
	}

	if(m_showcolscrollbar)
		h+=kGUI::GetSkin()->GetScrollbarWidth();
	return(h);
}

void kGUITableObj::AddObject(kGUIObj *obj)
{
	assert(false,"Use AddRow!");
}

kGUITableRowObj *kGUITableObj::AddNewRow(void)
{
	unsigned int was=GetNumChildren();

	/* ask table event manager to add a new row to the table */
	CallEvent(EVENT_ADDROW);

	assert(was!=GetNumChildren(),"No new function set for table!");
	return static_cast<kGUITableRowObj *>(GetChild(GetNumChildren()-1));
}

/* this is called when a cell in a table has changed from one type to another, for example */
/* if the first cell in a table is a combobox and based on it, the next cell over is either */
/* a inputbox, tickbox, combobox or something else. So if the user code changes the pointer */
/* to the 2nd cell then this must be called to attach that cell and position it in the */
/* table properly */

void kGUITableRowObj::CellChanged(void)
{
	kGUITableObj *t;
	int col;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;

	t=static_cast<kGUITableObj *>(GetParent());
	/* if row has not been attached to the table yet then ignore */
	if(!t)
		return;

	/* force any newly added cells to have the row as their parent */
	cellobjptrs=GetObjectList();
	for(col=0;col<GetNumObjects();++col)
	{
		cellobj=cellobjptrs[col];
		cellobj->SetParent(this);
	}
	t->CellChanged();
	Dirty();
}

void kGUITableObj::Activate(void)
{
	if(this!=kGUI::GetActiveObj())
		kGUI::PushActiveObj(this);
}

void kGUITableObj::CallAfterUpdate(void)
{
	CallEvent(EVENT_AFTERUPDATE);
	kGUI::CallAAParents(this);
}

/* rebuild the array pointer table */
void kGUITableObj::TableDirty(void)
{
	m_toprow=0;
	m_positionsdirty=true;

	Dirty();
}

void kGUITableObj::DeleteRow(kGUITableRowObj *obj,bool purge)
{
	bool rc;

	rc=DeleteRowz(obj,purge);
	assert(rc,"delete row object not found!");
}

bool kGUITableObj::DeleteRowz(kGUITableRowObj *obj,bool purge)
{
	unsigned int i;

	for(i=0;i<m_numrows;++i)
	{
		if(obj==GetChild(i))
		{
			if(purge)
			{
				/* hmmm, this should NOT be called since events should only be trigerred */
				/* for user generated actions not regular code based actions */
				delete obj;
			}
			else
				DelObject(obj);		/* delete object from parent list but don't purge it */

			if((m_cursorrow>=i) && m_cursorrow)
				--m_cursorrow;
			--m_numrows;
			Dirty();
			m_positionsdirty=true;
			m_sizechanged=true;
			return(true);
		}
	}
	return(false);	/* row not found */
}

/* delete all rows in the table */
void kGUITableObj::DeleteChildren(bool purge)
{
	kGUIContainerObj::DeleteChildren(purge);

	m_numrows=0;
	m_cursorrow=0;
	m_leftcol=0;
	m_rightcol=0;
	m_toprow=0;
	m_drawtoprow=0;
	m_botrow=0;
	m_drawbotrow=0;
	m_drawleftcol=0;
	m_drawrightcol=0;
	m_scroll.SetCurrent(0,0);
	m_scroll.SetDest(0,0);
	SetChildScroll(0,0);
	ReCalcPositions();
	Dirty();
}

/* this is called whenever the table size changes */
void kGUITableObj::CalcChildZone(void)
{
	unsigned int x,y;
	kGUIZone cz;

	if(m_poprowheaders)
		x=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	else if(m_showrowheaders)
		x=kGUI::GetSkin()->GetTableRowHeaderWidth();
	else
		x=2;
	if(m_showcolheaders)
	{
		/* calc height */
		unsigned int i;
		unsigned int h;
		kGUITableColTitleObj *t;

		y=0;
		for(i=0;i<m_numcols;++i)
		{
			t=m_coltitles.GetEntryPtr(i);
			h=t->GetLineHeight()+3+3;
			if(h>y)
				y=h;
		}
		m_colheaderheight=y;
//		y=kGUI::GetSkin()->GetTableColHeaderHeight();
	}
	else
		y=2;
	/* set the child zone */
	SetChildZone(x,y,(GetZoneW()-x)-2,GetZoneH()-y);

	/* set the position of the row scrollbar */
	if(m_showrowscrollbar)
	{
		CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());
		cz.SetZoneY(0);
		cz.SetZoneW(kGUI::GetSkin()->GetScrollbarWidth());
		m_rowscrollbar.MoveZone(&cz);
	}

	/* set the position of the column scrollbar */
	if(m_showcolscrollbar)
	{
		CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(0);
		cz.SetZoneY(cz.GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight());
		cz.SetZoneW(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());
		cz.SetZoneH(kGUI::GetSkin()->GetScrollbarHeight());
		m_colscrollbar.SetZone(&cz);
	}

	/* since this is called when the size of the table has changed */
	/* we need to call the recalc code */
	ReCalcPositions();

	/* also if the height of the table has been made smaller then the cursor may */
	/* have been pushed off of the bottom, so move it up if so */
	if(m_cursorrow>m_lastfullrow)
		MoveRow(m_lastfullrow-m_cursorrow);
}

void kGUITableObj::SetNumCols(unsigned int n)
{
	unsigned int i;
	kGUITableColTitleObj *t;
	
	m_numcols=n;
	m_colorder.Alloc(n);
	m_colwidths.Alloc(n);
	m_showcols.Alloc(n);
	m_coltitles.Init(n,1);
	m_colhints.Init(n,1);

	m_cxs.Alloc(n);
	m_cwidths.Alloc(n);

	/* default values for column widths / titles */
	for(i=0;i<n;++i)
	{
		m_colorder.SetEntry(i,i);
		m_colwidths.SetEntry(i,100);
		t=m_coltitles.GetEntryPtr(i);
		t->SetTable(this);
		m_showcols.SetEntry(i,true);
	}
}

void kGUITableObj::CalculateColWidth(int col)
{
	int e,nc;
	kGUIObj *gobj;
	int w;
	int maxw;
	kGUITableRowObj *rowobj;

	if(m_coltitles.GetEntryPtr(col))
		maxw=m_coltitles.GetEntryPtr(col)->GetWidth()+3+3;
	else
		maxw=10;

	nc=GetNumChildren();
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(e);
		rowobj=static_cast<kGUITableRowObj *>(gobj);

		/* get the width of this col */
		w=rowobj->GetObjectList()[col]->GetZoneW();
		if(w>maxw)
			maxw=w;
	}
	SetColWidth(col,maxw);
}

void kGUITableObj::Sort(int (*code)(const void *o1,const void *o2))
{
	SortObjects(0,code);

	/* recalc all pointers etc */
	m_cursorrow=0;
	m_toprow=0;
	m_botrow=0;
	m_sizechanged=true;
	ReCalcPositions();
	Dirty();
}

void kGUITableObj::ReCalcPositions(void)
{
	unsigned int e;
	unsigned int col;
	unsigned int xcol;
	unsigned int row;
	int sx,sy,colwidth,rowheight,plusnew;
	kGUITableRowObj *rowobj;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;
	kGUIZone zone;
	kGUIZone cz;

	m_positionsdirty=false;

	/* if rows have been added or deleted, or */
	/* column widths have changed, or rearragned, or shown/hidden */
	/* then reposition all rows in the table */

	if(m_sizechanged)
	{
		int w;
		const int *pxs;
		const int *pwidths;
		
		/* since number of rows has changed, check to see if any variables */
		/* are now off the end of the table */

		if(m_lastselectedrow>=(int)m_numrows)
			m_lastselectedrow=-1;

		if(m_toprow>m_numrows)
		{
			m_toprow=0;
			m_drawtoprow=0;
		}

		if(m_drawtoprow>=m_numrows)
			m_drawtoprow=0;

		if(m_cursorrow>m_numrows)
			m_cursorrow=0;
		if(m_cursorrow<m_toprow)		/* possible if many rows are deleted */
			m_cursorrow=m_toprow;

		/* calc col positions and widths */
		sx=0;
		for(col=0;col<m_numcols;++col)
		{
			xcol=m_colorder.GetEntry(col);
			m_cxs.SetEntry(xcol,sx);
			if(m_showcols.GetEntry(xcol)==false)
				m_cwidths.SetEntry(xcol,0);
			else
			{
				w=m_colwidths.GetEntry(xcol);	
				m_cwidths.SetEntry(xcol,w);
				sx+=w;
			}
		}

		if(m_poprowheaders==true)
			sx-=kGUI::GetSkin()->GetMenuRowHeaderWidth();

		/* now set positions for all cells in the table */
		sy=0;
		pxs=m_cxs.GetArrayPtr();
		pwidths=m_cwidths.GetArrayPtr();
		for(row=0;row<m_numrows;++row)
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(row));
			/* get the height of this row */
			rowheight=rowobj->GetRowHeight();
			cellobjptrs=rowobj->GetObjectList();

			for(col=0;col<m_numcols;++col)
			{
				cellobj=*(cellobjptrs++);
				cellobj->MoveZone(pxs[col],0,pwidths[col],rowheight);
			}

			/* set area for row object */
			rowobj->MoveZone(0,sy,sx,rowheight);
			sy+=rowheight;
		}
		m_sizechanged=false;
	}

	/* calculate the child zone space, subtract scrollbar space if shown */
	CopyChildZone(&cz);
	if(m_showcolscrollbar)
		cz.SetZoneH(cz.GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight());
	if(m_showrowscrollbar)
		cz.SetZoneW(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());

	/* calculate the leftcol, rightcol and last fullcol that fits in the area */

	m_lastfullcol=m_leftcol;/* just incase col is very wide and not fully on screen */
	sx=0;
	col=m_leftcol;
	
	if(m_numcols>0)
	{
		do
		{
			xcol=m_colorder.GetEntry(col);
			if(m_showcols.GetEntry(xcol)==true)
			{
				colwidth=m_colwidths.GetEntry(xcol);
				sx+=colwidth;
				if(sx<=cz.GetZoneW())
					m_lastfullcol=col;
			}
			if((col+1)==m_numcols)
				break;
			++col;
		}while(sx<cz.GetZoneW());
	}
	m_rightcol=col;

	/* calculate toprow, botrow and lastfullrow */ 

	m_lastfullrow=m_toprow;	/* just incase row is very large and not fully on screen */
	sy=0;

	for(e=m_toprow;e<m_numrows;++e)
	{
		rowobj=static_cast<kGUITableRowObj *>(GetChild(e));

		/* get the height of this row */
		rowheight=rowobj->GetRowHeight();
		if((sy+rowheight)<=cz.GetZoneH())
			m_lastfullrow=e;

		if(sy>=cz.GetZoneH())
			break;

		sy+=rowheight;
	}
	m_botrow=e;

	/* draw new record button at bottom of table? */
	m_drawaddbutton=false;
	plusnew=0;
	if(m_botrow==m_numrows)
	{
		if(m_allownew)
		{
			m_drawaddbutton=true;
			if(m_showcolheaders)
				m_drawaddy=sy+m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
			else
				m_drawaddy=sy;

			if((sy+m_colheaderheight)<=cz.GetZoneH())
				m_viewadd=true;
			else
				m_viewadd=false;
			plusnew=1;
		}
	}

	if(m_showrowscrollbar)
		m_rowscrollbar.SetValues(m_toprow,(m_botrow-m_toprow)+1,(m_numrows-1)-m_lastfullrow);
	if(m_showcolscrollbar)
	{
		/* since columns can be shown/hidden we can't just */
		/* use the column variables directly but need to calculate how */
		/* many visable columns there are on the left, shown and right*/
		int numleft,numright,numoffright;

		numleft=0;
		numright=0;
		numoffright=0;
		for(col=0;col<m_numcols;++col)
		{
			xcol=m_colorder.GetEntry(col);
			if(this->m_showcols.GetEntry(xcol))
			{
				if(col<m_leftcol)
					++numleft;
				if(col<m_rightcol)
					++numright;
				if(col>m_lastfullcol)
					++numoffright;
			}
		}
		m_colscrollbar.SetValues(numleft,(numright-numleft)+1,numoffright);
	}

	if(m_numrows>0)
		UpdateCurrentObj();

	if((!m_numrows) || (!m_numcols))
		m_scroll.SetDest(0,0);
	else
	{
		rowobj=static_cast<kGUITableRowObj *>(GetChild(m_toprow));
		m_scroll.SetDest(m_cxs.GetEntry(m_colorder.GetEntry(m_leftcol)),rowobj->GetZoneY());
	}
	CalcDrawBounds();
}

void kGUITableObj::UpdateCurrentObj(void)
{
	int xcol;
	kGUITableRowObj *rowobj;
	kGUIObj *gobj;
	kGUIObj **cellobjptrs;

	if(!m_numcols)
		return;	/* table is not fully initialized yet, so return */

	if(m_cursorrow<m_numrows)	/* it is valid to point to "new" entry */
	{
		rowobj=static_cast<kGUITableRowObj *>(GetChild(m_cursorrow));

		SetCurrentChild(m_cursorrow);

		/* select the whole row? */
		if(m_listmode==true || m_selectmode==true)
			rowobj->SetCurrentChild((kGUIObj *)0);		/* make the whole row current */
		else
		{
			cellobjptrs=rowobj->GetObjectList();
			xcol=GetColOrder(m_cursorcol);
			gobj=cellobjptrs[xcol];

			rowobj->SetCurrentChild(gobj);
		}
	}
}

int kGUITableObj::GetColTitleIndex(const char *name)
{
	unsigned int i;

	for(i=0;i<this->m_numcols;++i)
	{
		if(!stricmp(GetColTitle(i),name))
			return(i);
	}
	return(-1);
}

/* calc the height for this row */
unsigned int kGUITableObj::CalcRowHeight(unsigned int n)
{
	unsigned int col,numcols,h,maxh;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;
	kGUITableRowObj *row;

	/* calc the tallest cell in this row */
	row=static_cast<kGUITableRowObj *>(GetChild(n));
	maxh=0;

	numcols=row->GetNumObjects();
	cellobjptrs=row->GetObjectList();
	for(col=0;col<numcols;++col)
	{
		cellobj=cellobjptrs[col];
		h=cellobj->GetZoneH();
		if(h>maxh)
			maxh=h;
	}
	return(maxh);
}


void kGUITableObj::AddRow(kGUITableRowObj *obj,int place)
{
	int col,numcols,h,maxh;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;

	/* set the parent for each object in the row to the row object */
	maxh=obj->GetRowHeight();

	numcols=obj->GetNumObjects();
	cellobjptrs=obj->GetObjectList();
	for(col=0;col<numcols;++col)
	{
		cellobj=cellobjptrs[col];
		cellobj->SetParent(obj);
		h=cellobj->GetZoneH();
		if(h>maxh)
			maxh=h;
	}

	/* set the height of the row to the tallest */
	assert(maxh>0,"Error, row height is invalid");
	obj->SetRowHeight(maxh);

	switch(place)
	{
	case 0:
		kGUIContainerObj::AddObject(obj);	/* at end of table */
	break;
	case 1:
		MoveRow(1);		/* insert after cursor row, so, move down one and fall through to insert code below */
	case -1:
		kGUIContainerObj::InsertObject(obj,GetCursorRow());	/* insert before cursor line */
	break;
	default:
		assert(false,"Ummm, only -1,0,1 are valud values here!");
	break;
	}
	++m_numrows;
	m_positionsdirty=true;
	m_sizechanged=true;

	Dirty();
}

/* the table has scrolled so recalc the draw zones */

void kGUITableObj::Scrolled(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_MOVED)
	{
		SetChildScroll(m_scroll.GetCurrentX(),m_scroll.GetCurrentY());
		Dirty();
		CalcDrawBounds();
	}
}

void kGUITableObj::CalcDrawBounds(void)
{
	int sx,sy,xcol;
	kGUITableRowObj *rowobj;
	kGUIZone cz;
	kGUIZone sz;

	sx=m_scroll.GetCurrentX();
	sy=m_scroll.GetCurrentY();

	CopyChildZone(&cz);		/* get the child zone */

	/* set the position of the row scrollbar */
	if(m_showrowscrollbar)
	{
		sz.SetZoneX(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth()+sx);
		sz.SetZoneY(sy);
		sz.SetZoneW(kGUI::GetSkin()->GetScrollbarWidth());
		sz.SetZoneH(cz.GetZoneH());
		m_rowscrollbar.MoveZone(&sz);
	}

	/* set the position of the column scrollbar */
	if(m_showcolscrollbar)
	{
		sz.SetZoneX(sx);
		sz.SetZoneY(cz.GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight()+sy);
		sz.SetZoneW(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());
		sz.SetZoneH(kGUI::GetSkin()->GetScrollbarHeight());
		m_colscrollbar.MoveZone(&sz);
	}

	if(m_showcolscrollbar)
		cz.SetZoneH(cz.GetZoneH()-m_colscrollbar.GetZoneH());
	if(m_showrowscrollbar)
		cz.SetZoneW(cz.GetZoneW()-m_rowscrollbar.GetZoneW());

	if(!m_numrows)
	{
		m_drawtoprow=0;
		m_drawbotrow=0;
	}
	else
	{
		/* move up or down? */
		if(m_drawtoprow>=m_numrows)
			m_drawtoprow=m_numrows-1;
		do
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(m_drawtoprow));
			if(sy<rowobj->GetZoneY())
			{
				if(!m_drawtoprow)
					break;
				--m_drawtoprow;
			}
			else if(sy>(rowobj->GetZoneY()+rowobj->GetZoneH()))
			{
				if(m_drawtoprow==m_numrows-1)
					break;
				++m_drawtoprow;
			}
			else
				break;
		}while(1);
		
		m_drawbotrow=m_drawtoprow;
		sy+=cz.GetZoneH();				/* bottom line of draw area */
		do
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(m_drawbotrow));
			if(sy>rowobj->GetZoneY())
			{
				if(++m_drawbotrow==m_numrows)
				{
					m_drawbotrow=m_numrows;
					break;
				}
			}
			else
				break;
		}while(1);
	}

	if(!m_numcols)
	{
		m_drawleftcol=0;
		m_drawrightcol=0;
	}
	else
	{
		/* calc the start draw columns */
		do
		{
			xcol=m_colorder.GetEntry(m_drawleftcol);
			if(sx<m_cxs.GetEntry(xcol))
			{
				if(--m_drawleftcol==0)
					break;
			}
			else if(sx>=(m_cxs.GetEntry(xcol)+m_cwidths.GetEntry(xcol)))
			{
				if(m_drawleftcol==m_numcols-1)
					break;
				++m_drawleftcol;
			}
			else
				break;
		}while(1);
		/* calc the end draw column */
		m_drawrightcol=m_drawleftcol;
		sx+=cz.GetZoneW();				/* right edge of draw area */
		do
		{
			xcol=m_colorder.GetEntry(m_drawrightcol);
			if(sx>=(m_cxs.GetEntry(xcol)+m_cwidths.GetEntry(xcol)))
			{
				if(++m_drawrightcol>=(m_numcols-1))
				{
					m_drawrightcol=m_numcols-1;
					break;
				}
			}
			else
				break;
		}while(1);
	}
}

void kGUITableObj::Draw(void)
{
	int x,w,h,sx,xoff;
	unsigned int y;
	unsigned int col;
	unsigned int xcol;

	kGUICorners c;
	kGUICorners cc;
	kGUICorners rc;
	kGUIObj *gobj;
	int redge;

	sx=m_scroll.GetCurrentX();

	if(m_poprowheaders==true)
		redge=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	else if(m_showrowheaders==true)
		redge=kGUI::GetSkin()->GetTableRowHeaderWidth();
	else
		redge=0;

	if(m_positionsdirty==true)
		ReCalcPositions();

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRectBevelIn(c.lx,c.ty,c.rx,c.by);

		if(m_showcolheaders==true && (m_numcols>0))
		{
			y=c.ty;
			xoff=c.lx+redge-sx;
			/* draw column headers */
			for(col=m_drawleftcol;col<=m_drawrightcol;++col)
			{
				xcol=m_colorder.GetEntry(col);
				w=m_cwidths.GetEntry(xcol);
				if(w)
				{
					x=xoff+m_cxs.GetEntry(xcol);
					h=m_colheaderheight;
//					h=kGUI::GetSkin()->GetTableColHeaderHeight();
					kGUI::DrawRectBevel(x,y,x+w,y+h,false);
					if(m_coltitles.GetEntryPtr(xcol))
						m_coltitles.GetEntryPtr(xcol)->Draw(x+3,y+3,x+w,y+h);
				}
			}
		}
		/* draw upper left corner button */
		if((m_showcolheaders==true) || (m_showrowheaders==true) || (m_poprowheaders==true))
		{
			x=c.lx;
			y=c.ty;
			kGUI::DrawRectBevel(x,y,x+redge,y+m_colheaderheight /*kGUI::GetSkin()->GetTableColHeaderHeight()*/,false);
		}

		if(m_poprowheaders==true)
		{
			for(y=m_drawtoprow;y<m_drawbotrow;++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&rc);
				rc.lx=c.lx;
				kGUI::GetSkin()->DrawMenuRowHeader(&rc);
			}
		}
		else if(m_showrowheaders==true)
		{
			/* draw the row buttons */
			GetChildCorners(&cc);
			kGUI::PushClip();
			cc.lx=c.lx;
			if(m_showcolscrollbar)
				cc.by-=kGUI::GetSkin()->GetScrollbarHeight();
			kGUI::ShrinkClip(&cc);

			for(y=m_drawtoprow;y<m_drawbotrow;++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&rc);
				rc.lx=c.lx;
				rc.rx=c.lx+redge;
				kGUI::GetSkin()->DrawTableRowHeader(&rc,static_cast<kGUITableRowObj *>(gobj)->GetSelected(),y==m_cursorrow,false);
			}

			if(m_drawaddbutton==true)
			{
				xoff=c.lx+redge-sx;
				for(col=m_drawleftcol;col<=m_drawrightcol;++col)
				{
					xcol=m_colorder.GetEntry(col);
					w=m_cwidths.GetEntry(xcol);
					if(w)
					{
						x=xoff+m_cxs.GetEntry(xcol);
						kGUI::DrawRectBevelIn(x,c.ty+m_drawaddy,x+w,c.ty+m_drawaddy+m_colheaderheight /*kGUI::GetSkin()->GetTableColHeaderHeight()*/);
					}
				}
				rc.lx=c.lx;
				rc.rx=c.lx+redge;
				rc.ty=c.ty+m_drawaddy;
				rc.by=rc.ty+m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
				kGUI::GetSkin()->DrawTableRowHeader(&rc,false,false,true);

			}
			kGUI::PopClip();
		}

		/* these are inside the child areaa but on the edges */
		if(m_showrowscrollbar)
			m_rowscrollbar.Draw();
		if(m_showcolscrollbar)
			m_colscrollbar.Draw();

		GetChildCorners(&cc);
		if(m_showrowscrollbar)
			cc.rx-=kGUI::GetSkin()->GetScrollbarWidth();	/* remove scrollbar area */
		if(m_showcolscrollbar)
			cc.by-=kGUI::GetSkin()->GetScrollbarHeight();

		kGUI::PushClip();
		kGUI::ShrinkClip(&cc);
	
		/* is there anywhere to draw? */
		if(kGUI::ValidClip())
		{
			/* draw all row objects */
			for(y=m_drawtoprow;y<m_drawbotrow;++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&rc);	/* is this needed? */
				gobj->Draw();
			}

			/* should we flash the cursor? */
			if(m_selectmode==false && m_listmode==false && kGUI::GetActiveObj()==this)
			{
				gobj=kGUI::GetCurrentObj();
				if(gobj && kGUI::GetDrawCursor())
				{
					gobj->GetCorners(&c);
					kGUI::DrawRect(c.lx+2,c.ty+2,c.lx+5,c.by-2,DrawColor(32,32,32));
				}
			}
		}
		kGUI::PopClip();	/* table child area - scrollbar */
	}
	kGUI::PopClip();	/* table edges */
}

void kGUITableObj::MoveRow(int delta,bool clearsel)
{
	unsigned int destrow;
	unsigned int addrow;
	bool changed=false;

	if(!delta)
		return;

	if(clearsel)
		UnSelectRows();

	if(m_positionsdirty)
		ReCalcPositions();

	if(m_numrows==0 || m_lockrow==true)
		return;		/* table is currently empty */

	if(m_allownew==true)
		addrow=1;
	else
		addrow=0;

	/* first, check to see if cursor can be moved to the destination row */
	destrow=m_cursorrow+delta;
	if((int)destrow<0)
		destrow=0;
	if(destrow>=m_numrows)
		destrow=m_numrows-1;
	if(destrow==m_cursorrow)
	{
		/* make sure row is on the visible area */
		if(m_cursorrow<m_toprow)
			destrow=m_toprow;
		else if(m_cursorrow>m_lastfullrow)
			destrow=m_lastfullrow;
	}
	if(GetEntryEnable(destrow)==false)
	{
		/* can't move, here see if there is a valid entry below or above this one */
		do{
			if(delta<0)
			{
				if(!destrow)
					return;	/* can't go any lower! */
				--destrow;
			}
			else
			{
				if((destrow+1)==m_numrows)
					return;	/* can't go any higher! */
				++destrow;
			}
			if(GetEntryEnable(destrow)==true)
			{
				delta=destrow-m_cursorrow;
				break;	/* ok, go here! */
			}
		}while(1);
	}

	/* move up */
	while(delta<0 && m_cursorrow)
	{
		if(m_cursorrow==m_toprow)
		{
			--m_toprow;
		}
		--m_cursorrow;
		++delta;
		changed=true;
	}

	while(delta>0 && (m_cursorrow+1)!=(m_numrows+addrow))
	{
		while(m_cursorrow==(m_lastfullrow+addrow))
		{
			++m_toprow;
			ReCalcPositions();	/* need to update lastfull row each time */
		}

		if((m_cursorrow+1)<m_numrows)
			++m_cursorrow;

		/* if scrolling to the last row and new rows are enabled and the */
		/* add button is not on screen then scroll one more so we can see the add button */
		if(addrow && (m_cursorrow==(m_numrows-1)) && (m_viewadd==false) && (m_toprow<m_cursorrow))
		{
			++m_toprow;
			ReCalcPositions();	/* need to update lastfull row each time */
		}

		--delta;
		changed=true;
	}
	UpdateCurrentObj();
	ReCalcPositions();
	
	/* try to make cursor row fully on screen if possible instead of partially */
	while(m_cursorrow>m_lastfullrow && m_toprow<m_lastfullrow)
	{
		++m_toprow;
		ReCalcPositions();	/* need to update lastfull row each time */
	}

	Dirty();
	if(changed)
		CallEvent(EVENT_MOVED);
}

void kGUITableObj::ScrollRow(int delta,bool updatecursor)
{
	bool changed=false;
	int odelta=delta;

	if(m_positionsdirty)
		ReCalcPositions();

	if(m_numrows==0 || m_lockrow==true)
		return;		/* table is currently empty */

	/* move up */
	while(delta<0 && ( m_cursorrow || m_toprow ) )
	{
		if(m_toprow)
		{
			--m_toprow;
			ReCalcPositions();	/* recalc last shown row */
			if(updatecursor)
			{
				while(m_cursorrow>m_lastfullrow)
					--m_cursorrow;
			}
		}
		else if(updatecursor)
			--m_cursorrow;

		++delta;
		changed=true;
	}

	while(delta>0 && (m_cursorrow+1)!=m_numrows)
	{
		if((m_lastfullrow+1)==m_numrows)
		{
			if(updatecursor)
				++m_cursorrow;
		}
		else
		{
			++m_toprow;
			if(updatecursor)
			{
				if(m_cursorrow<m_toprow)
					++m_cursorrow;
			}
		}

		--delta;
		changed=true;
	}

	assert(m_cursorrow<m_numrows,"Error! m_cursorrow>=m_numrows");
	assert(m_toprow<m_numrows,"Error! m_toprow>=m_numrows");
	
	if(updatecursor)
	{
		assert(m_cursorrow>=m_toprow,"Cursorrow is off screen?");
	}

	UpdateCurrentObj();
	ReCalcPositions();

	/* if scrolling to the last row and new rows are enabled and the */
	/* add button is not on screen then scroll one more so we can see the add button */
	if(updatecursor)
	{
		if(odelta>0 && m_allownew && (m_cursorrow==(m_numrows-1)) && (m_viewadd==false) && (m_toprow<m_cursorrow))
		{
			++m_toprow;
			ReCalcPositions();	/* need to update lastfull row each time */
		}
		assert(m_cursorrow<=m_botrow,"Cursorrow is off screen?");
	}

	Dirty();
	if(changed)
		CallEvent(EVENT_MOVED);
}


/* move row itself down */
void kGUITableObj::SwapRow(int delta)
{
	int r1,r2;
	kGUIObj *row1;
	kGUIObj *row2;

	r1=m_cursorrow;
	r2=m_cursorrow+delta;
	row1=GetChild(r1);
	row2=GetChild(r2);
	SetChild(r1,row2);
	SetChild(r2,row1);
	m_positionsdirty=true;
	m_sizechanged=true;			/* todo: could be optimized to just recalc for these two */
}

void kGUITableObj::MoveCol(int delta,bool counthidden)
{
	bool changed=false;
	int lastcol=m_cursorcol;
	int lastleftcol=m_leftcol;

	if(!delta)
		return;

	UnSelectRows();
	if(m_positionsdirty)
		ReCalcPositions();

	if(m_lockcol==true)
		return;

	while(delta<0 && m_cursorcol>0)
	{
		if(m_cursorcol==m_leftcol)
		{
			--m_leftcol;
			ReCalcPositions();
		}
		--m_cursorcol;
		if(m_showcols.GetEntry(m_colorder.GetEntry(m_cursorcol))==true)
		{
			lastcol=m_cursorcol;
			lastleftcol=m_leftcol;
			++delta;
		}
		else if(counthidden==true)
			++delta;

		changed=true;
	}

	while (delta>0 && ((m_cursorcol+1)<m_numcols))
	{
		++m_cursorcol;
		while(m_cursorcol>m_lastfullcol && ((m_leftcol+1)<m_numcols))
		{
			++m_leftcol;
			ReCalcPositions();
		}
		if(m_showcols.GetEntry(m_colorder.GetEntry(m_cursorcol))==true)
		{
			lastcol=m_cursorcol;
			lastleftcol=m_leftcol;
			--delta;
		}
		else if(counthidden==true)
			--delta;
		changed=true;
	}

	/* was it sucessfully moved? */
	if(m_showcols.GetEntry(m_colorder.GetEntry(m_cursorcol))==false)
	{
		m_cursorcol=lastcol;
		m_leftcol=lastleftcol;
		ReCalcPositions();
	}

	UpdateCurrentObj();
	Dirty();
	if(changed)
		CallEvent(EVENT_MOVED);
}

void kGUITableObj::UnSelectRows(void)
{
	int nc,e;
	kGUITableRowObj *rowobj;

	if(m_lastselectedrow==-1)
		return;

	if((kGUI::GetKeyShift()==true) || (kGUI::GetKeyControl()==true))
		return;

	nc=GetNumChildren();
	for(e=0;e<nc;++e)
	{
		rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
		rowobj->SetSelected(false);
	}
	m_lastselectedrow=-1;
}

void kGUITableObj::SelectRow(int line,bool add)
{
	int nc,e;
	bool selected;

	kGUITableRowObj *rowobj;

	if(kGUI::GetKeyShift()==true && m_lastselectedrow!=-1 && m_allowmultiple)
	{
		int start,end;

		/* select all rows between us and the last selected row */
		start=MIN(m_lastselectedrow,line);
		end=MAX(m_lastselectedrow,line);
		for(e=start;e<=end;++e)
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
			rowobj->SetSelected(true);
		}
	}
	else if((kGUI::GetKeyControl()==true || add==true) && m_allowmultiple)
	{
		rowobj=static_cast<kGUITableRowObj *>(GetChild(line));
		selected=rowobj->GetSelected();
		rowobj->SetSelected(!selected);
	}
	else
	{
		/* only 1 object is selected */

		rowobj=static_cast<kGUITableRowObj *>(GetChild(line));
		selected=rowobj->GetSelected();

		nc=GetNumChildren();
		for(e=0;e<nc;++e)
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
			rowobj->SetSelected(false);
		}
		rowobj=static_cast<kGUITableRowObj *>(GetChild(line));
		rowobj->SetSelected(!selected);
	}
	m_lastselectedrow=line;
}

kGUIObj *kGUITableObj::GetCell(unsigned int row,unsigned int col)
{
	kGUITableRowObj *rowobj;
	kGUIObj **cellobjptrs;

	rowobj=static_cast<kGUITableRowObj *>(GetChild(row));
	cellobjptrs=rowobj->GetObjectList();
	return(cellobjptrs[col]);
}

bool kGUITableObj::UpdateInput(void)
{
	int key;
	unsigned int y;
	unsigned int col;
	unsigned int xcol;
	kGUIObj *gobj;
	kGUICorners c;
	kGUICorners c2;
	kGUICorners rc;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;
	bool over;
	int redge;
	bool usedkey=true;

	if(m_poprowheaders==true)
		redge=kGUI::GetSkin()->GetMenuRowHeaderWidth();
	else if(m_showrowheaders==true)
		redge=kGUI::GetSkin()->GetTableRowHeaderWidth();
	else
		redge=0;

	if(m_positionsdirty==true)
		ReCalcPositions();

	switch(m_editmode)
	{
	case TABLEEDIT_NONE:
	break;
	case TABLEEDIT_COLWIDTH:
		if(kGUI::GetMouseReleaseLeft()==true)
		{
			m_editmode=TABLEEDIT_NONE;
			return(true);
		}
		else
		{
			int neww=GetColWidth(m_editcol)+kGUI::GetMouseDX();
			
			kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTHORIZ);
			if(neww<20)
				neww=20;	/* minimum width */
			SetColWidth(m_editcol,neww);
			ReCalcPositions();
			Dirty();
			return(true);
		}
	break;
	case TABLEEDIT_ROWHEIGHT:
		if(kGUI::GetMouseReleaseLeft()==true)
		{
			m_editmode=TABLEEDIT_NONE;
			return(true);
		}
		else
		{
			int newh=GetRowHeight(m_editrow)+kGUI::GetMouseDY();
			
			kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);
			if(newh<10)
				newh=10;	/* minimum height */
			SetRowHeight(m_editrow,newh);
			ReCalcPositions();
			Dirty();
			return(true);
		}
	break;
	};

	/* have I been disabled by program code ( not the user )? */
	if(this==kGUI::GetActiveObj())
	{
		if(ImCurrent()==false)
			goto abort;
	}

	GetCorners(&c);
	over=kGUI::MouseOver(&c);
	if(over==false)
	{
		CallEvent(EVENT_MOUSEOFF);
		m_wasoff=true;
		if((kGUI::GetMouseClick()==true || kGUI::GetMouseWheelDelta()))
		{
abort:;
			m_selected=-1;
			if(this==kGUI::GetActiveObj())
				kGUI::PopActiveObj();

			Dirty();/* this needs to be called before the callback as the callback can delete me */

			if((m_selectmode==true) || (m_listmode==true))
			{
				CallSelectedEvent();
				//m_selcallback.Call();
			}
			return(false);
		}
	}
	else
	{
		/* if the mouse was off and then came back on the trigger a moved event */
		if(m_wasoff==true)
		{
			CallEvent(EVENT_MOVED);
			m_wasoff=false;
		}
	}
	if(this!=kGUI::GetActiveObj())
	{
		if(kGUI::GetMouseClick()==true || kGUI::GetMouseWheelDelta())
			kGUI::PushActiveObj(this);
	}

	/* check for clicks on column buttons */

	if(m_showcolheaders==true)
	{
		GetCorners(&c2);
		c2.by=c2.ty+m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
		if(kGUI::MouseOver(&c2))	/* is mouse over the column headers? */
		{
			c2.rx=c2.lx+kGUI::GetSkin()->GetTableRowHeaderWidth();
			if(kGUI::MouseOver(&c2))	/* is it over the top/left corner */
			{
				if(kGUI::WantHint()==true)
					kGUI::SetHintString(c2.lx,c2.ty-15,"Right click to hide, show and reorder table columns.");

				if((kGUI::GetMouseClickRight()==true))
				{
					kGUIEditTableColObj *etc;

					/* bring up window for adjusting columns */
					etc=new kGUIEditTableColObj(this);
					return(true);
				}
			}
			else if(m_numcols)
			{
				c2.lx+=kGUI::GetSkin()->GetTableRowHeaderWidth();
				for(col=m_leftcol;col<=m_rightcol;++col)
				{
					xcol=m_colorder.GetEntry(col);
					if(m_showcols.GetEntry(xcol)==true)
					{
						c2.rx=c2.lx+m_colwidths.GetEntry(xcol);
						if(kGUI::MouseOver(&c2))
						{
							if(kGUI::WantHint()==true)
								kGUI::SetHintString(c2.lx,c2.ty-15,m_colhints.GetEntryPtr(xcol)->GetString());
							if(kGUI::GetMouseX()>(c2.rx-10))	/* adjust width? */
							{
								kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTHORIZ);
								if(kGUI::GetMouseClickLeft()==true)
								{
									if(m_allowadjcolwidths==true)
									{
										m_editmode=TABLEEDIT_COLWIDTH;
										m_editcol=xcol;
									}
									return(true);
								}
							}
							else 
							{
								if(kGUI::GetMouseClickRight()==true)
								{
									kGUIEvent e;

									m_cursorcol=col;
									/* is col partially off of the right? */
									if(m_cursorcol>m_lastfullcol)
										MoveCol(1);
									UpdateCurrentObj();
									Dirty();
									
									e.m_value[0].i=col;
									CallEvent(EVENT_COL_RIGHTCLICK,&e);
									return(true);
								}
							}
						}
						c2.lx=c2.rx;
					}
				}
			}
		}
	}

	/* check for adjust row heights? */
	if(m_showrowheaders==true)
	{
		GetCorners(&c2);
		c2.rx=c2.lx+redge;
		if(kGUI::MouseOver(&c2))
		{
			for(y=m_drawtoprow;y<m_drawbotrow;++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&rc);
				c2.ty=MAX(rc.ty,rc.by-5);
				c2.by=rc.by;
				if(kGUI::MouseOver(&c2))
				{
					kGUI::SetTempMouseCursor(MOUSECURSOR_ADJUSTVERT);
					if(kGUI::GetMouseClickLeft()==true)
					{
						if(m_allowadjrowheights==true)
						{
							m_editmode=TABLEEDIT_ROWHEIGHT;
							m_editrow=y;
						}
						return(true);
					}
				}
			}
		}
	}

	if(this==kGUI::GetActiveObj())
	{
		if(ImCurrent()==false)
			SetCurrent();
		if(!m_numcols)
			return(true);		/* empty table */

		/* flash cursor? */
		gobj=kGUI::GetCurrentObj();
		if(gobj)
		{
			if(kGUI::GetDrawCursorChanged())
			{
				gobj->GetCorners(&c2);
				Dirty(&c2);
			}
		}
		if(m_showrowscrollbar==true)
		{
			if(m_rowscrollbar.IsActive()==true)
				return(m_rowscrollbar.UpdateInput());
		}
		if(m_showcolscrollbar==true)
		{
			if(m_colscrollbar.IsActive()==true)
				return(m_colscrollbar.UpdateInput());
		}

		if(kGUI::GetMouseClickLeft()==true)
		{
			/* check for clicks on scroll bars */
			if(m_showrowscrollbar==true)
			{
				m_rowscrollbar.GetCorners(&c2);
				if(kGUI::MouseOver(&c2))
					return(m_rowscrollbar.UpdateInput());
			}
			if(m_showcolscrollbar==true)
			{
				m_colscrollbar.GetCorners(&c2);
				if(kGUI::MouseOver(&c2))
					return(m_colscrollbar.UpdateInput());
			}
		}
		/* have they selected a row in list mode? */
		if(m_listmode==true)
		{
			over=false;
			for(y=m_drawtoprow;(y<m_drawbotrow) && (over==false);++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&c2);
				
				if((m_showrowheaders==true) || (m_poprowheaders==true))
					c2.lx-=redge;	/* include row headers on left too */

				if(kGUI::MouseOver(&c2))	/* is the mouse over this row? */
				{
					over=true;
					if(kGUI::GetMouseClickLeft()==true || kGUI::GetMouseDoubleClickLeft()==true)
					{
						m_cursorrow=y;
						SelectRow(y);

						/* is row partially off of the bottom? */
						if(m_cursorrow>m_lastfullrow)
							MoveRow(1);
						UpdateCurrentObj();
						Dirty();
					}
				}
			}
		}
		else if(kGUI::GetMouseClick()==true || kGUI::GetMouseDoubleClick()==true)
		{
			/* check for clicks on row buttons */
			if((m_showrowheaders==true) || (m_poprowheaders==true))
			{
				GetCorners(&c2);
				c2.ty+=m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();

				/* add new record button/line */
				if(m_drawaddbutton==true)
				{
					rc.lx=c2.lx;
					rc.rx=c2.rx;
					rc.ty=c2.ty+m_drawaddy-m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
					rc.by=rc.ty+m_colheaderheight;	//kGUI::GetSkin()->GetTableColHeaderHeight();
					if(kGUI::MouseOver(&rc))
					{
						AddNewRow();
						ReCalcPositions();
						MoveRow( (int)m_numrows-(int)m_cursorrow);
						MoveCol(-(int)m_cursorcol);
						CallAfterUpdate();
						return(true);
					}
				}

				/* check for clicks on the row headers? */
				c2.rx=c2.lx+redge;
				if(kGUI::MouseOver(&c2))
				{
					for(y=m_drawtoprow;y<m_drawbotrow;++y)
					{
						gobj=GetChild(y);
						gobj->GetCorners(&rc);
						c2.ty=rc.ty;
						c2.by=rc.by;
						if(kGUI::MouseOver(&c2))
						{
							int ev;
							kGUIEvent e;

							SelectRow(y);
							m_cursorrow=y;
							/* is row partially off of the bottom? */
							if(m_cursorrow>m_lastfullrow)
								MoveRow(1);
							else
								CallEvent(EVENT_MOVED);

							UpdateCurrentObj();
							Dirty();

							if(kGUI::GetMouseDoubleClickLeft()==true)
								ev=EVENT_ROW_LEFTDOUBLECLICK;
							else if(kGUI::GetMouseDoubleClickRight()==true)
								ev=EVENT_ROW_RIGHTDOUBLECLICK;
							else if(kGUI::GetMouseClickLeft()==true)
								ev=EVENT_ROW_LEFTCLICK;
							else
								ev=EVENT_ROW_RIGHTCLICK;

							e.m_value[0].i=y;
							CallEvent(ev,&e);
							return(true);
						}
					}
				}
			}

			/* check for clicks on the cells */

			for(y=m_drawtoprow;y<m_drawbotrow;++y)
			{
				gobj=GetChild(y);
				gobj->GetCorners(&c2);
				if(kGUI::MouseOver(&c2))	/* is the mouse over this row? */
				{
					kGUITableRowObj *rowobj;
					rowobj=static_cast<kGUITableRowObj *>(gobj);
					cellobjptrs=rowobj->GetObjectList();

					for(col=m_leftcol;col<=m_rightcol;++col)
					{
						xcol=m_colorder.GetEntry(col);
						cellobj=cellobjptrs[xcol];
						cellobj->GetCorners(&c);
						if(kGUI::MouseOver(&c)==true)
						{
							/* set the cursorrow and cursorcol to this spot */
							if((m_cursorrow!=y) || (m_cursorcol!=col))
							{
								MoveCol(col-m_cursorcol,true);
								MoveRow(y-m_cursorrow);
								if(m_listmode==false && m_selectmode==false)
									goto passdown;	/* pass input down to child */
							}
							else
							{
								//didn't really move but user still might want to know
								CallEvent(EVENT_MOVED);
							}
							if(m_listmode==true)
							{
								if(kGUI::GetMouseDoubleClickLeft()==true)
								{
									m_selected=m_cursorrow;
									kGUI::PopActiveObj();
									if((m_selectmode==true) || (m_listmode==true))
									{
										CallSelectedEvent();
										//m_selcallback.Call();
									}
									return(true);
								}
							}
							break;
						}
					}
				}
			}
		}
		else if(m_selectmode==true)
		{
			/* make cursorrow follow the mouse pointer */
			int sw=kGUI::GetSkin()->GetScrollbarWidth();

			for(y=m_toprow;y<m_botrow;++y)
			{
				if(GetEntryEnable(y)==true)
				{
					gobj=GetChild(y);
					gobj->GetCorners(&c2);
					c2.rx-=sw;	
					if(kGUI::MouseOver(&c2))	/* is the mouse over this row? */
					{
						m_wasover=true;
						MoveRow(y-m_cursorrow);
						if(kGUI::GetMouseReleaseLeft()==true)
						{
							m_selected=m_cursorrow;
							kGUI::PopActiveObj();
							Dirty();	/* erase me */

							CallSelectedEvent();
							//m_selcallback.Call();
							return(true);
						}
						break;
					}
				}
			}

			if(kGUI::GetMouseReleaseLeft()==true)
			{
				/* allow release over combo box to not abort */
				if(m_wasover==false && m_releasecount==0)
					++m_releasecount;
				else
				{
					m_selected=-1;
					kGUI::PopActiveObj();
					Dirty();			/* erase me */
					CallSelectedEvent();
					//m_selcallback.Call();
					return(true);
				}
			}
			/* scroll */

			if(m_wasover==true)
			{
				/* need to add delay */
				if(kGUI::GetMouseY()<GetZoneY())
					MoveRow(-1);
				else if(kGUI::GetMouseY()>(GetZoneY()+GetZoneH()))
					MoveRow(1);
			}
		}

		if(this==kGUI::GetActiveObj())
		{
			/* have they clicked outside of me? */
			if(kGUI::GetMouseClickLeft()==true)
			{
				if(kGUI::MouseOver(&c)==false)
				{
					m_selected=-1;
					kGUI::PopActiveObj();
					Dirty();
					CallSelectedEvent();
					//m_selcallback.Call();
					return(false);	/* pass input to someone else */
				}
			}

			/* I am still the active container */

			{
				int scroll=kGUI::GetMouseWheelDelta();
				kGUI::ClearMouseWheelDelta();
				if(scroll)
					ScrollRow(-scroll);
			}

			key=kGUI::GetKey();
			if(key)
			{
				switch(key)
				{
				case GUIKEY_SELECTALL:
				{
					int e,nr;
					kGUITableRowObj *rowobj;

					nr=m_numrows;
					Dirty();
					for(e=0;e<nr;++e)
					{
						rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
						rowobj->SetSelected(true);
					}
				}
				break;
				case GUIKEY_TAB:
				{
					unsigned int r,c;
					kGUIObj *cell;

					/* increment through the table util a valid cell is found! */
					r=m_cursorrow;
					c=m_cursorcol;
					do
					{
						if(++c==m_numcols)
						{
							c=0;
							if(++r==m_numrows)
								r=0;			/* wrap from bottom to top */
						}
						/* are we back where we started? */
						if((r==m_cursorrow) && (c==m_cursorcol))
							break;

						cell=GetCell(r,c);
						if(cell)
						{
							if(cell->SkipTab()==false)
							{
								/* goto here! */
								this->MoveRow(r-m_cursorrow);
								this->MoveCol(c-m_cursorcol);
								break;
							}
						}
					}while(1);
				}
				break;
				case GUIKEY_SHIFTTAB:
				{
					unsigned int r,c;
					kGUIObj *cell;

					/* increment through the table util a valid cell is found! */
					r=m_cursorrow;
					c=m_cursorcol;
					do
					{
						if(!c)
						{
							c=m_numcols-1;
							if(!r)
								r=m_numrows-1;	/* wrap from top to bottom */
							--r;
						}
						else
							--c;
						/* are we back where we started? */
						if((r==m_cursorrow) && (c==m_cursorcol))
							break;
						cell=GetCell(r,c);
						if(cell)
						{
							if(cell->SkipTab()==false)
							{
								/* goto here! */
								this->MoveRow(r-m_cursorrow);
								this->MoveCol(c-m_cursorcol);
								break;
							}
						}
					}while(1);
				}
				break;
				case GUIKEY_HOME:
					if(m_cursorrow==m_toprow)	/* at top of view? */
						MoveRow(-(int)m_cursorrow);
					else
						MoveRow(-((int)m_cursorrow-(int)m_toprow));
				break;
				case GUIKEY_END:
					if(m_cursorrow==m_lastfullrow)	/* at bottom of view? */
						MoveRow(m_numrows-m_cursorrow);
					else
						MoveRow(m_lastfullrow-m_cursorrow);
				break;
				case GUIKEY_UP:
						MoveRow(-1);
				break;
				case GUIKEY_DOWN:
						MoveRow(1);
				break;
				case GUIKEY_PGUP:
					if(m_cursorrow==m_toprow)
						MoveRow(m_toprow-m_botrow-1);
					else
						MoveRow(m_toprow-m_cursorrow);
				break;
				case GUIKEY_PGDOWN:
					if(m_cursorrow!=m_lastfullrow)		/* already at bottom line? */
						MoveRow(m_lastfullrow-m_cursorrow);
					else
						MoveRow((m_botrow-m_toprow)-1);
				break;
				case GUIKEY_LEFT:
					MoveCol(-1);
				break;
				case GUIKEY_RIGHT:
					MoveCol(1);
				break;
				case GUIKEY_RETURN:
					if(m_selectmode==true || m_listmode==true)
					{
						m_selected=m_cursorrow;
						kGUI::ClearKey();
						kGUI::PopActiveObj();
						Dirty();	/* erase me */
						CallSelectedEvent();
						//m_selcallback.Call();
						return(true);
					}
				break;
				case GUIKEY_ESC:
					if(m_selectmode==true || m_listmode==true)
					{
						m_selected=-1;
						kGUI::ClearKey();
						kGUI::PopActiveObj();
						Dirty();	/* erase me */
						CallSelectedEvent();
						//m_selcallback.Call();
						return(true);
					}
				break;
				case GUIKEY_INSERT:
					if(m_allownew)
					{
						AddNewRow();
						ReCalcPositions();
						MoveRow(m_numrows-m_cursorrow);
					}
				break;
				case GUIKEY_DELETE:
					if(m_allowdelete==true && m_selectmode==false && m_listmode==false)
					{
						/* count number of selected rows */
						unsigned int e;
						int ns=0;
						kGUITableRowObj *rowobj;

						for(e=0;e<m_numrows;++e)
						{
							rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
							if(rowobj->GetSelected()==true)
								++ns;
						}
						if(ns>0)
						{
							kGUIMsgBoxReq *msg;
							kGUIString ask;

							if(ns==1)
								ask.SetString("Delete row?");
							else
								ask.Sprintf("Delete %d rows(s)?",ns);
							msg=new kGUIMsgBoxReq(MSGBOX_YES|MSGBOX_NO,this,CALLBACKNAME(DelSelRowsDone),false,ask.GetString());
						
						}
						else	/* no selected rows, so pass the delete to the current child */
						{
							/* is valid to point one past end to "new" record */
							if(m_cursorrow<m_numrows)
								usedkey=false;		/* pass key to cell under cursor */
						}
					}
				break;
				default:
					usedkey=false;
				break;
				}
				if(usedkey)
					kGUI::ClearKey();
				else if(kGUI::GetCurrentObj())
				{
					/* send input to child object */
					return(kGUI::GetCurrentObj()->UpdateInput());
				}
			}
		}

passdown:;
		if(m_selectmode==true || m_listmode==true)
			return(true);

		for(y=m_toprow;y<m_botrow;++y)
		{
			gobj=GetChild(y);
			gobj->GetCorners(&c);
			if(kGUI::MouseOver(&c)==true)
				return(gobj->UpdateInput());
		}
	}
	return(false);
}

void kGUITableObj::CallSelectedEvent(void)
{
	kGUIEvent e;

	e.SetObj(this);
	e.m_value[0].i=m_selected;
	CallEvent(EVENT_SELECTED,&e);
}

void kGUITableObj::DelSelRowsDone(int closebutton)
{
	unsigned int e;
	unsigned int n;
	kGUITableRowObj *rowobj;

	if(closebutton==MSGBOX_YES)
	{
		kGUIEvent ev;

		/* do a callback with the number of rows to be deleted */
		n=0;
		for(e=0;e<m_numrows;++e)
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
			if(rowobj->GetSelected()==true)
				++n;
		}
		ev.m_value[0].i=n;
		CallEvent(EVENT_DELETEROWSTART,&ev);

		for(e=m_numrows-1;(int)e>=0;--e)
		{
			rowobj=static_cast<kGUITableRowObj *>(GetChild(e));
			if(rowobj->GetSelected()==true)
			{
				ev.m_value[0].i=e;
				CallEvent(EVENT_DELETEROW,&ev);
				delete rowobj;
				if((m_cursorrow>=e) && m_cursorrow)
					--m_cursorrow;
				--m_numrows;
				m_sizechanged=true;
				m_positionsdirty=true;
			}
		}
		CallEvent(EVENT_DELETEROWEND,&ev);
		Dirty();
		ReCalcPositions();
		CallAfterUpdate();
	}
}


/***************** tablerow object functions **********************/

void kGUITableRowObj::SetRowHeight(unsigned int h)
{
	kGUIContainerObj *parentobj;
	kGUITableObj *parenttable;

	/* if the same then don't set the dirty flags since it hasn't really changed */
	if(m_rowheight!=h)
	{
		m_rowheight=h;
		
		parentobj=GetParent();
		if(parentobj)
		{
			parenttable=static_cast<kGUITableObj *>(parentobj);
			parenttable->SizeDirty();
		}
	}
}

void kGUITableRowObj::SetCurrentChild(int num)
{
	assert(false,"not implemented yet");
}

void kGUITableRowObj::SetCurrentChild(kGUIObj *cobj)
{
	kGUIObj *oldtopobj;
	kGUIObj *newtopobj;

	if(m_currentcell==cobj)
		return;

	oldtopobj=kGUI::GetCurrentObj();

	m_currentcell=cobj;		/* update */

	newtopobj=kGUI::GetCurrentObj();
	if(oldtopobj!=newtopobj)
	{
		/* force redraw for both objects */
		if(oldtopobj)
			oldtopobj->Dirty();
		if(newtopobj)
			newtopobj->Dirty();
	}
}

void kGUITableRowObj::CalcChildZone(void)
{
	SetChildZone(0,0,GetZoneW(),GetZoneH());
}

bool kGUITableRowObj::UpdateInput(void)
{
	int col,numcols;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;
	kGUICorners c;
	
	numcols=GetNumObjects();
	cellobjptrs=GetObjectList();
	for(col=0;col<numcols;++col)
	{
		cellobj=cellobjptrs[col];
		cellobj->GetCorners(&c);
		if(kGUI::MouseOver(&c)==true)
			return(cellobj->UpdateInput());
	}
	return(false);
}

/* if I am the current active object, then set the active object for each item */
/* in the cell before drawing, then put it back to me when done. */

void kGUITableRowObj::Draw(void)
{
	int col,numcols;
	kGUIObj **cellobjptrs;
	kGUIObj *cellobj;
	kGUICorners c;
	kGUICorners cc;
	kGUIObj *saveobj=kGUI::m_forcecurrentobj;
	bool hiliterow;		/* hilite all row items? */
	kGUITableObj *t;
	bool drawframe;

	t=static_cast<kGUITableObj *>(GetParent());
	/* don't draw a frame if select or listmode, only in regular table mode */
	drawframe=(t->GetSelectMode()==false && t->GetListMode()==false);

	/* quick check for whole row */

	GetCorners(&c);

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(t->GetListMode()==true)
		{
			if(GetSelected()==true || (ImCurrent()==true))
				hiliterow=true;
			else
				hiliterow=false;
		}
		else
		{
			/* if this row is the current row and there is no current cell then hilite the whole  row */
			if(((!m_currentcell) && (ImCurrent()==true)))
				hiliterow=true;
			else
				hiliterow=false;
		}

		numcols=GetNumObjects();
		cellobjptrs=GetObjectList();
		
		for(col=0;col<numcols;++col)
		{
			cellobj=cellobjptrs[col];
			if(cellobj->GetZoneW()>0)	/* 0 width means column is hidden */
			{
				if(hiliterow==true)
					kGUI::m_forcecurrentobj=cellobj;

				if(drawframe==true)
				{
					cellobj->GetCorners(&cc);
					kGUI::DrawRectBevelIn(cc.lx,cc.ty,cc.rx,cc.by);
				}
				cellobj->Draw();
			}
		}
	}
	kGUI::m_forcecurrentobj=saveobj;
	kGUI::PopClip();
}

/*******************************************************************************/

kGUIEditTableColRowObj::kGUIEditTableColRowObj(kGUITableObj *t,int col)
{
	m_objptrs[0]=&m_tick;
	m_objptrs[1]=&m_width;
	m_objptrs[2]=&m_name;
	m_name.SetLocked(true);
	m_tick.SetEventHandler(this,CALLBACKNAME(ChangeShow));
	m_width.SetEventHandler(this,CALLBACKNAME(ChangeWidth));
	m_t=t;
	m_col=col;
	Reload();
}

void kGUIEditTableColRowObj::ChangeShow(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
		m_t->SetColShow(m_xcol,m_tick.GetSelected());
}

void kGUIEditTableColRowObj::ChangeWidth(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
		m_t->SetColWidth(m_xcol,m_width.GetInt());
}

void kGUIEditTableColRowObj::Reload(void)
{
	m_xcol=m_t->GetColOrder(m_col);

	m_tick.SetSelected(m_t->GetColShow(m_xcol));
	m_width.Sprintf("%d",m_t->GetColWidth(m_xcol));
	m_name.SetString(m_t->GetColTitle(m_xcol));
}

kGUIEditTableColObj::kGUIEditTableColObj(kGUITableObj *table)
{
	int i,n;

	m_t=table;
	m_up.SetString(kGUI::GetString(KGUISTRING_UP));
	m_up.Contain();
	m_up.SetPos(10,10);
	m_up.SetEventHandler(this,CALLBACKNAME(PressUp));
	m_window.AddObject(&m_up);

	m_down.SetString(kGUI::GetString(KGUISTRING_DOWN));
	m_down.Contain();
	m_down.SetPos(80,10);
	m_down.SetEventHandler(this,CALLBACKNAME(PressDown));
	m_window.AddObject(&m_down);

	m_table.SetPos(10,40);
	m_table.SetSize(400,400);
	m_table.SetNumCols(3);
	m_table.SetColTitle(0,"Show");
	m_table.SetColTitle(1,"Width");
	m_table.SetColTitle(2,"Column Name");
	m_table.SetColWidth(0,50);
	m_table.SetColWidth(1,75);
	m_table.SetColWidth(2,400-50-75-30);
	m_window.AddObject(&m_table);

	n=table->GetNumCols();
	m_numcols=n;
	for(i=0;i<n;++i)
	{
		kGUIEditTableColRowObj *tcr;

		tcr=new kGUIEditTableColRowObj(table,i);
		m_table.AddRow(tcr);
	}

	m_window.SetSize(450,480);
	m_window.SetPos(kGUI::GetMouseX(),kGUI::GetMouseY());
	m_window.SetTop(true);
	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	kGUI::AddWindow(&m_window);
	
}

void kGUIEditTableColObj::WindowEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_CLOSE)
	{
		m_table.DeleteChildren();
		delete this;
	}
}

void kGUIEditTableColObj::PressUp(kGUIEvent *event)
{
	int line;
	int xcol;

	if(event->GetEvent()==EVENT_PRESSED)
	{
		line=m_table.GetCursorRow();
		if(line>0 && line<m_numcols)
		{
			xcol=m_t->GetColOrder(line);
			m_t->SetColOrder(line,m_t->GetColOrder(line-1));
			m_t->SetColOrder(line-1,xcol);
			m_table.MoveRow(-1);
			Reload();
			m_table.SelectRow(m_table.GetCursorRow());
		}
	}
}

void kGUIEditTableColObj::PressDown(kGUIEvent *event)
{
	int line;
	int xcol;

	if(event->GetEvent()==EVENT_PRESSED)
	{
		line=m_table.GetCursorRow();
		if(line<(m_numcols-1))
		{
			xcol=m_t->GetColOrder(line+1);
			m_t->SetColOrder(line+1,m_t->GetColOrder(line));
			m_t->SetColOrder(line,xcol);
			m_table.MoveRow(1);
			Reload();
			m_table.SelectRow(m_table.GetCursorRow());
		}
	}
}

void kGUIEditTableColObj::Reload(void)
{
	int i;
	kGUIEditTableColRowObj *tcr;

	for(i=0;i<m_numcols;++i)
	{
		tcr=static_cast<kGUIEditTableColRowObj *>(m_table.GetChild(i));
		tcr->Reload();
	}
}

/******************************************************************/

void kGUITableColTitleObj::StringChanged(void)
{
	m_t->SetPositionsDirty();
	m_t->DirtyandCalcChildZone();
}

void kGUITableColTitleObj::FontChanged(void)
{
	m_t->SetPositionsDirty();
	m_t->DirtyandCalcChildZone();
}
