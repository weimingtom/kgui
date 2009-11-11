/**********************************************************************************/
/* kGUI - kguiobj.cpp                                                             */
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

/**********************************************************************************/
/*                                                                                */
/* This contains the shared/common functions for the kGUIObj and kGUIContainerObj */
/* renderable gui objects. This is the code to handle screen positioning,         */
/* event handling, update calling and other basic control functions               */
/*                                                                                */
/**********************************************************************************/

#include "kgui.h"
#include <math.h>

void kGUIObj::CallEvent(unsigned int event)
{
	kGUIEvent eobj;eobj.SetObj(this);

	eobj.SetEvent(event);

	kGUI::CallEventListeners(&eobj);
	m_eventcallback.Call(&eobj);
}

void kGUIObj::CallEvent(unsigned int event,kGUIEvent *eobj)
{
	eobj->SetObj(this);
	eobj->SetEvent(event);

	kGUI::CallEventListeners(eobj);
	m_eventcallback.Call(eobj);
}

/* if this object is currently the "active" object then unactive it */
/* if it has a parent, then tell the parent that it is going bye bye! */

kGUIObj::~kGUIObj()
{
	/* todo: also, scan active stack and if it is anywhere in the stack then delete it */

	if(kGUI::GetActiveObj()==this)
		kGUI::PopActiveObj();
	if(m_parent)
		m_parent->DelObject(this);

	CallEvent(EVENT_DESTROY);
}

/* this is a virtual function that is replaced for Container objects and can be */
/* replaced for other custom objects */
void kGUIObj::Control(unsigned int command,KGCONTROL_DEF *data)
{
	switch(command)
	{
	case KGCONTROL_GETISCONTAINER:
	case KGCONTROL_GETISWINDOW:
		/* is this a single object or a container? */
		data->m_bool=false;
	break;
	case KGCONTROL_GETSKIPTAB:
		/* should we skip this when the user tabs onto it? */
		data->m_bool=false;
	break;
	case KGCONTROL_GETENABLED:
		/* is it enabled (editable) or disabled (locked)? */
		data->m_bool=true;
	break;
	default:
		assert(false,"Unhandled object control!");
	break;
	}
}

void kGUIObj::SetEventHandler(void *codeobj,void (*code)(void *,kGUIEvent *))
{
	kGUIEvent e;

	m_eventcallback.Set(codeobj,code);

	/* make sure even handler calls GetEvent() from the passed event class */
	if(codeobj)
	{
		CallEvent(EVENT_UNDEFINED,&e);
		assert(e.WasGetEventCalled()==true,"Error, EventHandler never called GetEvent");
	}
}

void kGUIScroll::SetEventHandler(void *codeobj,void (*code)(void *,kGUIEvent *))
{
	kGUIEvent e;

	m_eventcallback.Set(codeobj,code);

	/* make sure even handler calls GetEvent() from the passed event class */

	CallEvent(EVENT_UNDEFINED,&e);
	assert(e.WasGetEventCalled()==true,"Error, EventHandler never called GetEvent");
}


/* set "me" as the current object and then traverse all my parents and */
/* set them as current until there are no more parents */

void kGUIObj::SetCurrent(void)
{
	kGUIObj *oldcurrent;
	if(m_parent)
	{
		oldcurrent=m_parent->GetCurrentChild();
		if(oldcurrent!=this)
		{
			if(oldcurrent)
				oldcurrent->Dirty();
		}
		m_parent->SetCurrentChild(this);
		Dirty();
		m_parent->SetCurrent();
	}
}

/* return true if I am the current object of my parent and my parents */
/* are also the "current" object of all their parents too! */

bool kGUIObj::ImCurrent(void)
{
	kGUIObj *o;
	kGUIContainerObj *p;

	/* this is used by draw code to force an object to appear in reverse */
	if(this==kGUI::m_forcecurrentobj)
		return(true);

	o=this;
	p=m_parent;
	if(!p)
		return(false);	/* if I have NO parents then I'm not current */
	
	while(p)
	{
		if(p->GetCurrentChild()!=o)
			return(false);					/* not in chain of current objects */
		o=p;
		p=o->GetParent();
	}
	return(true);	/* yes I am in the chain of current objects */
}


/* calculate the 4 corners of the object by traversing the parent */
/* containers and adding their positionsas each childs position is */
/* relative to each of it's parents */

void kGUIObj::GetCorners(kGUICorners *c)
{
	int lx,ty;
	kGUIContainerObj *p;

	/* get my x,y offsets from my parents position */
	lx=GetZoneX();
	ty=GetZoneY();

	/* get my parent */
	p=GetParent();
	while(p)
	{
		/* add the offset of the parents "child" area to the current pos */
		/* then add the offset to the parents position to the current pos */
		/* then subtract the scroll offsets */
		lx+=p->GetChildZoneX()+p->GetZoneX()-p->GetChildScrollX();
		ty+=p->GetChildZoneY()+p->GetZoneY()-p->GetChildScrollY();
		p=p->GetParent();
	}
	/* calculate the right and bottom edges */
	c->lx=lx;
	c->ty=ty;
	c->rx=lx+GetZoneW();
	c->by=ty+GetZoneH();
}

/* this is the main mechanism for forcing a re-draw on a portion of the screen. */
/* dirty the area of the screen where the object resides */

void kGUIObj::Dirty(void)
{
	unsigned int i;
	unsigned int nc;
	kGUICorners c;
	kGUICorners wc;
	kGUIContainerObj *root;
	kGUIObj *rootparent;
	kGUIObj *w;

	if(!m_parent)
		return;		/* only trigger dirty if attached to something */

	GetCorners(&c);

	/* clip to edges of screen */
	if(c.lx<0)
		c.lx=0;
	if(c.rx>kGUI::GetSurfaceWidth())
		c.rx=kGUI::GetSurfaceWidth();
	if(c.ty<0)
		c.ty=0;
	if(c.by>kGUI::GetSurfaceHeight())
		c.by=kGUI::GetSurfaceHeight();

	root=kGUI::GetRootObject();
	rootparent=this;
	do
	{
		if(!rootparent->m_parent)
			return;						/* not attached to render tree yet */
		if(rootparent->m_parent==root)
			break;						/* yes we are attached to the render tree */
		rootparent=rootparent->m_parent;
	}while(1);

	/* check to see if this dirty area is hidden under a higher priority window */
	nc=root->GetNumChildren();
	for(i=0;i<nc;++i)
	{
		w=root->GetChild(nc-i-1);
		if(rootparent==w)
			break;
		w->GetCorners(&wc);
		if(kGUI::Inside(&c,&wc))
			return;						/* im under this higher priority window! */
	}

	/* todo: check for overlap and reduce size if just sticking out? */

	kGUI::Dirty(&c);
}

/* dirty an arbitrary portion of the screen */
void kGUIObj::Dirty(const kGUICorners *c)
{
	kGUICorners c2;

	c2=*(c);

	/* clip to edges of screen */
	if(c2.lx<0)
		c2.lx=0;
	if(c2.rx>kGUI::GetSurfaceWidth())
		c2.rx=kGUI::GetSurfaceWidth();
	if(c2.ty<0)
		c2.ty=0;
	if(c2.by>kGUI::GetSurfaceHeight())
		c2.by=kGUI::GetSurfaceHeight();

	kGUI::Dirty(&c2);
}

/***************** container object functions **********************/

/* containers are just like the basic object except that they */
/* can contain other "child" objects. */
/* they can contain mulitple groups of objects or just a single group */
/* tables, windows etc contain a single groups of objects */
/* tab objects can contain multiple groups of objects depending on the */
/* number of tabs selected. */

kGUIContainerObj::kGUIContainerObj()
{
	m_numchildren.SetGrow(true);
	m_children=0;
	m_numgroups=0;
	m_scrollx=0;
	m_scrolly=0;
	m_staytop=false;
	m_allowoverlappingchildren=false;
	m_containchildren=true;
}

void kGUIContainerObj::Control(unsigned int command,KGCONTROL_DEF *data)
{
	switch(command)
	{
	case KGCONTROL_GETISCONTAINER:
		data->m_bool=true;
	break;
	default:
		kGUIObj::Control(command,data);
	break;
	}
}

void kGUIContainerObj::SetTop(bool t)
{
	m_staytop=t;
	if(t==true)
		kGUI::PushActiveObj(this);
}

/* this function removes all children from all lists in the container */
/* object. If the purge flag is set then it also calls the destructor */
/* for each child object. */

void kGUIContainerObj::DeleteChildren(bool purge)
{
	unsigned int g;
	unsigned int nc;
	unsigned int i;
	kGUIObj *gobj;

	/* remove all child objects */
	for(g=0;g<m_numgroups;++g)
	{
		if(purge==false)
		{
			/* since we are not deleteing them but removing them from our list */
			/* we need to detach them by setting their parent to null */
			nc=m_numchildren.GetEntry(g);
			for(i=0;i<nc;++i)
			{
				gobj=GetChild(g,i);
				gobj->SetParent(0);
			}
			/* then we just set number of children in this list to 0 */
			m_numchildren.SetEntry(g,0);
		}
		else
		{
			/* this code just keeps deleting the first entry in the list as */
			/* the delete child object callback will call the parent code to remove */
			/* it from the parents list and therefore the parents array of child object */
			/* pointers will have the object removed from it's list automagically. */

			while(m_numchildren.GetEntry(g)>0)
			{
				gobj=GetChild(g,0);
				delete gobj;
			}
		}
	}
}

/* this sort function calls the standard "array" template sort function using the */
/* users decision function for determining if entries are to be swapped */
/* see the kgui.h file for the actual sort code */

void kGUIContainerObj::SortObjects(int group,int (*code)(const void *o1,const void *o2))
{
	Array<class kGUIObj *>*grouplist;

	grouplist=&m_children[group];
	grouplist->Sort(m_numchildren.GetEntry(group),code);
}

/* every container has a currentgroup that is active and within */
/* each group it has a current child, it is perfectly valid to */
/* have no children and also to have no groups and in this case */
/* the code returns -1 and callers need to check for that case */

int kGUIContainerObj::GetCurrentChildNum(void)
{
	int childindex;

	if(!m_numgroups)
		return -1;
	if(!GetNumChildren())
		return -1;

	/* get current child number from array ( one entry per group ) */
	childindex=m_current.GetEntry(CurrentGroup());
	if(childindex<0)
		return -1;		/* special case for no current object */
	return childindex;
}

/* this is just like the function above except that instead */
/* of returning the child index, it instead returns a pointer */
/* to the child obhect and null if there is no current child */

kGUIObj *kGUIContainerObj::GetCurrentChild(void)
{
	int childindex;

	if(!m_numgroups)
		return 0;
	if(!GetNumChildren())
		return 0;
	childindex=m_current.GetEntry(CurrentGroup());
	if(childindex<0)
		return 0;		/* special case for no current object */
	return GetChild(childindex);
}

/* set the passed child object pointer as the current */
/* child for the currentgroup */
/* null is passed to signify that there is no current child */

void kGUIContainerObj::SetCurrentChild(kGUIObj *cobj)
{
	int e,nc;

	/* if I am already the current child then return */
	if(GetCurrentChild()==cobj)
		return;

	if(!cobj)
	{
		SetCurrentChild((int)-1);
		return;
	}
	else
	{
		/* find the child in the list since we keep track of */
		/* the current child via a number and not a pointer */

		nc=GetNumChildren();
		for(e=0;e<nc;++e)
		{
			if(GetChild(e)==cobj)
			{
				SetCurrentChild(e);
				return;
			}
		}
	}
	assert(false,"Can't set this as current as it isn't in the child list!");
}

/* this is the same as the function above except that */
/* instead of passing a pointer to the child, it is passed */
/* the child number instead, -1 means no child is current */

void kGUIContainerObj::SetCurrentChild(int num)
{
	kGUIObj *oldtopobj;
	kGUIObj *newtopobj;

	/* -1 = n current object, else 0->n-1 is valid index */
	passert( ((num>=-1) && (num<(int)GetNumChildren())),"Illegal Child index, must be between -1 and %d!",GetNumChildren());

	/* if I am already the current child then return */
	if(GetCurrentChild()==GetChild(num))
		return;

	/* grab the current object at the top of the current objects chain */
	oldtopobj=kGUI::GetCurrentObj();

	m_current.SetEntry(CurrentGroup(),num);

	/* grab the new current object at the top of the current objects chain */
	newtopobj=kGUI::GetCurrentObj();

	/* if they are different, then force a re-draw on both as typically */
	/* their draw code is slightly different if they are "current" */
	if(oldtopobj!=newtopobj)
	{
		/* remember, null is a valid response if there is no current object */
		if(oldtopobj)
			oldtopobj->Dirty();
		if(newtopobj)
			newtopobj->Dirty();
	}
}

/* this is called when the user presses TAB, or SHIFTTAB and it is used for iterating */
/* through the current object through the container objects */
/* items that return true for the SkipTab call are skipped as this is the */
/* usual response from gui items that are static like text and images */

/* return false if no valid objects can be found to set current */
/* dir should be 1 or -1 only! */

bool kGUIContainerObj::Tab(int dir)
{
	int e,nc,startc;
	kGUIObj *oldobj;
	kGUIObj *gobj;

	assert((dir==1) || (dir==-1),"Invalid direction specified!");

	nc=GetNumChildren();
	if(!nc)
		return(false);		/* no children to tab through! */

	/* save starting index so we don't go forever if there are no valid */
	/* items to tab to */

	startc=m_current.GetEntry(CurrentGroup());
	if(startc==-1)	/* special case for no current item */
		startc=0;

	assert(startc<nc,"Current object index is invalid!");

	oldobj=GetChild(startc);
	e=startc;
	do
	{
		e+=dir;		/* next! */

		if(e==-1)		/* wrap back to end of list? */
			e=nc-1;
		else if(e>=nc)	/* wrap forward to start of list? */
			e=0;
		if(e==startc)	/* did we get back to where we started from? */
		{
			/* error, no valid items to choose from arrrggh */
			/* just leave the current child where it was */
			return(false);
		}

		gobj=GetChild(e);
		/* this check is for skipping items that are disabled like buttons */
		/* the skiptab check is for skipping static items like text and images */
		if((gobj->GetEnabled()==true) && (gobj->SkipTab()==false))
		{
			/* ok, we found a new child to be the current one so if there */
			/* was a previous current child, then call the dirty code to redraw it */
			if(oldobj)
				oldobj->Dirty();

			/* set this new one as current and tell it to redraw too. */
			gobj->Dirty();
			m_current.SetEntry(CurrentGroup(),e);
			return(true);	/* success */
		}
	}while(1);
}

/* child objects are not automatically deleted */
/* the delete function needs to be specifically called if they are to be purged */

kGUIContainerObj::~kGUIContainerObj()
{
	unsigned int g,nc,e;
	kGUIObj *gobj;

	/* flag any remaining children as orphans! */

	/* delete all child objects */
	for(g=0;g<m_numgroups;++g)
	{
		nc=m_numchildren.GetEntry(g);
		for(e=0;e<nc;++e)
		{
			gobj=GetChild(g,e);
			gobj->SetParent(0);	/* I'm not your parent anymore */
		}
		m_numchildren.SetEntry(g,0);	/* no children anymore! */
	}
	if(m_children)
	{
		delete []m_children;
		m_children=0;
	}
}

/* this code is used to allocate the number of groups that this */
/* container needs to hold. Simple objects like windows and */
/* tables have a single group, tab controls havee a group for each */
/* tab. */

void kGUIContainerObj::SetNumGroups(int num)
{
	int i;

	m_numgroups=num;

	m_numchildren.Alloc(num);
	m_current.Alloc(num);

	/* if this was set before and is changing then we need to delete the */
	/* old list before allocating a new one */
	if(m_children)
		delete []m_children;
	m_children=new Array<class kGUIObj *>[num];
	for(i=0;i<num;++i)
	{
		m_numchildren.SetEntry(i,0);	/* group as no children yet */
		m_children[i].SetGrow(true);	/* let group grow as big as it wants */
		m_children[i].SetGrowSize(32);	/* when full, allocate n more slots */
		m_current.SetEntry(i,-1);		/* default to no current object */
	}
}

/* this fucntion is used to add a child object to the container */
/* the child object is added to the current group */

void kGUIContainerObj::AddObject(kGUIObj *obj)
{
	int g=CurrentGroup();				/* get current group number */
	int nc=m_numchildren.GetEntry(g);	/* get the current number of children in this group */

	obj->SetParent(this);			/* set the childs parent to me */
	m_numchildren.SetEntry(g,nc+1);	/* increment the number of children in this group */
	m_children[g].SetEntry(nc,obj);	/* set the array entry to point to the child */
	obj->Dirty();
}

/* this fucntion is used to add a child object to the container */
/* the child object is added to the current group */

void kGUIContainerObj::InsertObject(kGUIObj *obj,int index)
{
	int g=CurrentGroup();					/* get current group number */
	int nc=m_numchildren.GetEntry(g);		/* get the current number of children in this group */

	obj->SetParent(this);					/* set the childs parent to me */
	m_numchildren.SetEntry(g,nc+1);			/* increment the number of children in this group */
	m_children[g].InsertEntry(nc,index,1);
	m_children[g].SetEntry(index,obj);		/* set the array entry to point to the child */
	obj->Dirty();
}


/* this function is used to remove the child from it's child list */
/* it scans all groups to find the child and once found, the entry */
/* in the child array for that group as the child deleted and the */
/* number of children in that group decremented */

void kGUIContainerObj::DelObject(kGUIObj *obj)
{
	unsigned int e,g,nc,cur;
	
	for(g=0;g<m_numgroups;++g)
	{
		/* num children in this group */
		nc=m_numchildren.GetEntry(g);
		for(e=0;e<nc;++e)
		{
			if(m_children[g].GetEntry(e)==obj)
			{
				m_children[g].DeleteEntry(e);	/* remove this entry from the array */
				m_numchildren.SetEntry(g,--nc);	/* update number of children */

				/* since we deleted an entry, do we need to change the "current" */
				/* child index too? */
				cur=m_current.GetEntry(g);
				if(cur>=e)
				{
					if(cur)		/* yes, decrement the current child index */
						--cur;
					/* todo: if current object is disabled then move to next */
					m_current.SetEntry(g,cur);
				}
				obj->SetParent(0);
				return;
			}
		}
	}
	/* items inside of tablerow objects don't need their children */
	/* linked to the row itself, they use a different mechanism so */
	/* I had to take this assert out. */
//	assert(false,"Object is not in parents container!");
}

#if 0
/* scan all lists looking for this object */
kGUIObj *kGUIContainerObj::LocateObj(const char *n)
{
	int i,e,nc;
	const char *name;
	kGUIObj *gobj;
	kGUIContainerObj *co;

	for(i=0;i<m_numgroups;++i)
	{
		nc=m_numchildren.GetEntry(i);
		for(e=0;e<nc;++e)
		{
			gobj=GetChild(i,e);
			name=gobj->GetName();
			if(name)
			{
				if(!strcmp(name,n))
					return(gobj);
			}

			/* if this is also a container object then */
			/* scan it and all it's children */

			if(gobj->IsContainer()==true)
			{
				co = static_cast<kGUIContainerObj *>(gobj);
				kGUIObj *cog;
				cog=co->LocateObj(n);
				if(cog)
					return(cog);
			}
		}
	}

	return(0);
}
#endif

/* call updateinput for all children in the specified group */
bool kGUIContainerObj::UpdateInputC(int num)
{
	int e,nc;
	bool objret;
	kGUIObj *gobj;
	kGUICorners c;

	nc=m_numchildren.GetEntry(num);
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(num,e);
		gobj->GetCorners(&c);
		/* only call if the mouse is over the object */
		if(kGUI::MouseOver(&c)==true)
		{
			objret=gobj->UpdateInput();

			/* if I am the active object then by default I must also */
			/* be the current object too, so force this if it is not already so */
			if( gobj==kGUI::GetActiveObj() )
			{
				if(gobj->ImCurrent()==false)
					gobj->SetCurrent();
			}
			if(objret==true)
				return(objret);
			if(m_allowoverlappingchildren==false)
			{
				/* since the mouse was over this object, no need to check against */
				/* any other child objects */
				return(objret);	
			}
		}
	}
	return(false);	/* input not used, mouse was not over any children */
}

/* draw all the child objects that are in the group specified */

void kGUIContainerObj::DrawC(int num)
{
	int e,nc;
	int cindex;	//debugging
	kGUIObj *gobj;
	kGUICorners c;

	/* get the corners of the childarea for this containier */
	if(m_containchildren==true)
	{
		GetChildCorners(&c);

		kGUI::PushClip();
		kGUI::ShrinkClip(&c);
	}

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		/* draw all child objects in this group */
		nc=m_numchildren.GetEntry(num);
		for(e=0;e<nc;++e)
		{
			gobj=GetChild(num,e);
			cindex=kGUI::GetClipIndex();
			gobj->Draw();
			assert(cindex==kGUI::GetClipIndex(),"Error: Draw function left clip on stack!");
		}
	}
	if(m_containchildren==true)
		kGUI::PopClip();
}

void kGUIContainerObj::SkinChanged(void)
{
	unsigned int g,e,nc;
	kGUIObj *gobj;
	kGUIContainerObj *gcobj;

	CalcChildZone();

	for(g=0;g<m_numgroups;++g)
	{
		nc=m_numchildren.GetEntry(g);
		for(e=0;e<nc;++e)
		{
			gobj=GetChild(g,e);
			if(gobj->IsContainer())
			{
				gcobj=static_cast<kGUIContainerObj *>(gobj);
				gcobj->SkinChanged();
			}
		}
	}
}

/***************** scrollbar obj functions **********************/

kGUIScrollBarObj::kGUIScrollBarObj()
{
	m_active=false;
	m_isvert=true;		/* default */
	m_showends=true;
	m_fixed=0;
	m_endclick=1;		/* default move value when clicking on end buttons */
}

/* this is in a common routine as it is shared be both the input and */
/* draw code, so no need to code it twice */

void kGUIScrollBarObj::CalcBarPos(const kGUICorners *c,kGUICorners *sl)
{
	int rawtotal;
	int ty=0,by=0,lx=0,rx=0;
	int above,below,middle;
	int fixed=0;

	kGUI::GetSkin()->GetScrollbarSize(m_isvert,&above,&below,&middle);

	if(m_showends==false)
	{
		above=0;
		below=0;
	}

	if(m_fixed)
	{
		rawtotal=m_numabove+m_numbelow;
		fixed=MIN(0,m_fixed-middle);
	}
	else
		rawtotal=m_numabove+m_numshown+m_numbelow;
	
	if(m_isvert==true)
	{
		ty=c->ty+above;
		by=c->by-below;

		m_barlength=(by-ty)-middle;
	}
	else	/* horizontal */
	{
		lx=c->lx+above;
		rx=c->rx-below;
		m_barlength=(rx-lx)-middle;
	}
	if(!rawtotal)
	{
		m_topoffset=0;
		m_botoffset=0;
	}
	else
	{
		if(m_fixed)
		{
			m_topoffset=(int)((m_numabove*(m_barlength-fixed))/rawtotal);
			m_botoffset=(int)(m_barlength-((m_numbelow*(m_barlength-fixed))/(double)rawtotal)+middle);
		}
		else
		{
			m_topoffset=(int)((m_numabove*m_barlength)/(double)rawtotal);
			m_botoffset=(int)(m_barlength-((m_numbelow*m_barlength)/(double)rawtotal)+middle);
		}
	}
	if(m_isvert==true)
	{
		sl->ty=ty+m_topoffset;
		sl->by=ty+m_botoffset;
		sl->lx=c->lx;
		sl->rx=c->rx;
	}
	else
	{
		sl->lx=lx+m_topoffset;
		sl->rx=lx+m_botoffset;
		sl->ty=c->ty;
		sl->by=c->by;
	}
}

void kGUIScrollBarObj::Draw(void)
{
	kGUICorners c;
	kGUICorners spos;

	GetCorners(&c);
	CalcBarPos(&c,&spos);

	kGUI::GetSkin()->DrawScrollbar(m_isvert,&c,&spos,m_showends);
}

bool kGUIScrollBarObj::UpdateInput(void)
{
	kGUICorners c;
	kGUICorners c2;
	kGUICorners spos;
	int above,below,width;
	kGUI::GetSkin()->GetScrollbarSize(m_isvert,&above,&below,&width);
	int move=0;

	if(m_showends==false)
	{
		above=0;
		below=0;
	}

	move=0;
	GetCorners(&c);
	CalcBarPos(&c,&spos);

	if(kGUI::GetMouseClickLeft()==true)
	{
		if(m_showends==false)
			goto onmiddle;
		/* over the top button? */
		if(m_isvert==true)		/* vertical scroll bar? */
		{
			c2.lx=c.lx;
			c2.rx=c.rx;
			c2.ty=c.ty;
			c2.by=c.ty+above;
		}
		else /* horizontal */
		{
			c2.lx=c.lx;
			c2.rx=c.lx+above;
			c2.ty=c.ty;
			c2.by=c.by;
		}
		if(kGUI::MouseOver(&c2)==true)
		{
			move=-m_endclick;
			m_active=true;			/* this need to be set so the slider code uses (eats) the mouse release */
			m_mode=SCROLLMODE_CLICK;
		}
		else
		{
			/* over the bottom button? */
			if(m_isvert==true)		/* vertical scroll bar? */
			{
				c2.lx=c.lx;
				c2.rx=c.rx;
				c2.ty=c.by-below;
				c2.by=c.by;
			}
			else /* horizontal */
			{
				c2.lx=c.rx-below;
				c2.rx=c.rx;
				c2.ty=c.ty;
				c2.by=c.by;
			}
			if(kGUI::MouseOver(&c2)==true)
			{
				move=m_endclick;
				m_active=true;	/* this need to be set so the slider code uses (eats) the mouse release */
				m_mode=SCROLLMODE_CLICK;
			}
			else
			{
onmiddle:;
				/* is it over the slider? */
				if(kGUI::MouseOver(&spos)==true)
				{
					m_active=true;
					m_mode=SCROLLMODE_ADJUST;
				}
				else
				{
					/* clicked above or below the slider, do pgup or pgdn */
					if(m_isvert==true)
					{
						int my;
						my=kGUI::GetMouseY();
						if(my<spos.ty)
							move=-m_numshown;	/* page up */
						else
							move=m_numshown;	/* page down */
					}
					else
					{
						int mx;
						mx=kGUI::GetMouseX();
						if(mx<spos.lx)
							move=-m_numshown;	/* page up */
						else
							move=m_numshown;	/* page down */
					}
				}
			}
		}
	}
	else if(m_active)
	{
		if(kGUI::GetMouseLeft()==false)
			m_active=false;
		else if(m_mode==SCROLLMODE_ADJUST)
		{
			int rawmove,rawtotal;

			/* adjust slider */
			rawtotal=m_numabove+m_numshown+m_numbelow;
			if(m_isvert==true)
				rawmove=kGUI::GetMouseDY();
			else
				rawmove=kGUI::GetMouseDX();
			if(rawmove)
			{
				move=(rawmove*rawtotal)/m_barlength;
				if(!move)
					move=rawmove;
			}

			/* clip move amount to max allowed */
			if(move<0)
			{
				if(move<-m_numabove)
					move=-m_numabove;
			}
			else if(move>0)
			{
				if(move>m_numbelow)
					move=m_numbelow;
			}
		}
	}

	if(move)
	{
		kGUIEvent e;

		e.m_value[0].i=move;
		CallEvent(EVENT_AFTERUPDATE,&e);
	}

	if(m_active==false && kGUI::GetActiveObj()==this)
		kGUI::PopActiveObj();

	return(true);
}

/********************************************/

void kGUIRectObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,m_color);
}

/***************** kGUIScroll **********************/

kGUIScroll::~kGUIScroll()
{
	if(m_attached==true)
		kGUI::DelUpdateTask(this,CALLBACKNAME(Update));
}

/* attach or detach the update event from the event list */
void kGUIScroll::Attach(void)
{
	bool diff;

	diff=( (m_cx!=m_dx) || (m_cy!=m_dy));
	if(diff==m_attached)
		return;			/* already in correct state */

	if(diff==true)
		kGUI::AddUpdateTask(this,CALLBACKNAME(Update));
	else
		kGUI::DelUpdateTask(this,CALLBACKNAME(Update));
	m_attached=diff;
}

void kGUIScroll::Update(void)
{
	bool moved;
	int delta,adelta,move;

	moved=false;

	/* move x */
	delta=m_dx-m_cx;
	if(delta)
	{
		adelta=abs(delta);
		move=adelta;
		if(move>180)
			move=move-180;
		else
		{
			move=MAX(3,(int)(sin((move/2)*(3.141592654/180.0f))*0.35f*kGUI::GetET()));
			if(!move)
				move=1;
			else if(move>adelta)
				move=adelta;
		}
		if(delta>0)
			m_cx+=move;
		else
			m_cx-=move;
		moved=true;
	}

	/* move y */
	delta=m_dy-m_cy;
	if(delta)
	{
		adelta=abs(delta);
		move=adelta;
		if(move>180)
			move=move-180;
		else
		{
			move=MAX(3,(int)(sin((move/2)*(3.141592654/180.0f))*0.35f*kGUI::GetET()));
			if(!move)
				move=1;
			else if(move>adelta)
				move=adelta;
		}
		if(delta>0)
			m_cy+=move;
		else
			m_cy-=move;
		moved=true;
	}
	Attach();

	if(moved==true)
		CallEvent(EVENT_MOVED);
}

/***************** scroll area container **************/

kGUIScrollContainerObj::kGUIScrollContainerObj()
{
	SetNumGroups(1);
	m_maxwidth=0;
	m_maxheight=0;
	m_usehs=true;
	m_usevs=true;

	m_vscrollbar.SetClickSize(10);
	m_vscrollbar.SetParent(this);
	m_vscrollbar.SetVert();
	m_vscrollbar.SetEventHandler(this,CALLBACKNAME(ScrollMoveRow));

	m_hscrollbar.SetClickSize(10);
	m_hscrollbar.SetParent(this);
	m_hscrollbar.SetHorz();
	m_hscrollbar.SetEventHandler(this,CALLBACKNAME(ScrollMoveCol));

	m_scroll.SetEventHandler(this,CALLBACKNAME(ScrollEvent));
}

void kGUIScrollContainerObj::SetInsideSize(int w,int h)
{
	w+=kGUI::GetSkin()->GetScrollbarWidth();
	h+=kGUI::GetSkin()->GetScrollbarHeight();
	SetSize(w,h);
}

void kGUIScrollContainerObj::CalcChildZone(void)
{
	SetChildZone(0,0,GetZoneW(),GetZoneH());
	Scrolled();
}

void kGUIScrollContainerObj::MoveRow(int delta)
{
	int y=m_scroll.GetDestY();
	int vh=GetZoneH();

	if(m_usehs)
		vh-=kGUI::GetSkin()->GetScrollbarWidth();

	y+=delta;
	if(y>(m_maxheight-vh))
		y=m_maxheight-vh;
	if(y<0)
		y=0;

    m_scroll.SetDestY(y);
	UpdateScrollBars();
}

void kGUIScrollContainerObj::MoveCol(int delta)
{
	int x=m_scroll.GetDestX();
	int vw=GetZoneW();

	if(m_usevs)
		vw-=kGUI::GetSkin()->GetScrollbarWidth();

	x+=delta;
	if(x>(m_maxwidth-vw))
		x=m_maxwidth-vw;
	if(x<0)
		x=0;

    m_scroll.SetDestX(x);
	UpdateScrollBars();
}

void kGUIScrollContainerObj::Scrolled(void)
{
	kGUIZone cz;

	SetChildScroll(m_scroll.GetCurrentX(),m_scroll.GetCurrentY());
	UpdateScrollBars();

	/* set the position of the row scrollbar */
	if(m_usevs)
	{
		CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(m_scroll.GetCurrentX()+(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth()));
		cz.SetZoneY(m_scroll.GetCurrentY());
		cz.SetZoneW(kGUI::GetSkin()->GetScrollbarWidth());
		m_vscrollbar.MoveZone(&cz);
	}

	/* set the position of the column scrollbar */
	if(m_usehs)
	{
		CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(m_scroll.GetCurrentX());
		cz.SetZoneY(m_scroll.GetCurrentY()+(cz.GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight()));
		cz.SetZoneW(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());
		cz.SetZoneH(kGUI::GetSkin()->GetScrollbarHeight());
		m_hscrollbar.SetZone(&cz);
	}

	Dirty();
}

void kGUIScrollContainerObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(m_usehs)
	{
		c.by-=kGUI::GetSkin()->GetScrollbarHeight();
		m_hscrollbar.Draw();
	}
	if(m_usevs)
	{
		c.rx-=kGUI::GetSkin()->GetScrollbarWidth();
		m_vscrollbar.Draw();
	}

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	/* default to white */
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,DrawColor(255,255,255));
	DrawC(0);
	kGUI::PopClip();
}

void kGUIScrollContainerObj::UpdateScrollBars(void)
{
	int vh=GetZoneH();
	int vw=GetZoneW();

	if(m_usehs)
		vh-=kGUI::GetSkin()->GetScrollbarWidth();

	if(m_usevs)
		vw-=kGUI::GetSkin()->GetScrollbarWidth();

	if(m_usevs==true)
	{
		int below;

		below=m_maxheight-vh-m_scroll.GetDestY();
		if(below<0)
			below=0;
		m_vscrollbar.SetValues(m_scroll.GetDestY(),vh,below);
	}
	if(m_usehs==true)
	{
		int right;

		right=m_maxwidth-vw-m_scroll.GetDestX();
		if(right<0)
			right=0;
		m_hscrollbar.SetValues(m_scroll.GetDestX(),vw,right);
	}
}

bool kGUIScrollContainerObj::UpdateInput(void)
{
	bool over;
	kGUICorners c;
	kGUICorners cs;

	GetCorners(&c);

//	UpdateScrollBars();
	over=kGUI::MouseOver(&c);
	if(over)
	{
		/* is the horizontal scroll bar on? */
		if(m_usehs==true)
		{
			if(m_hscrollbar.IsActive()==true)
				return(m_hscrollbar.UpdateInput());

			m_hscrollbar.GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
				return(m_hscrollbar.UpdateInput());
		}

		/* is the vertical scroll bar on? */
		if(m_usevs==true)
		{
			if(m_vscrollbar.IsActive()==true)
				return(m_vscrollbar.UpdateInput());

			m_vscrollbar.GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
				return(m_vscrollbar.UpdateInput());
		}
		{
			int scroll=kGUI::GetMouseWheelDelta();
			kGUI::ClearMouseWheelDelta();
			if(scroll)
				MoveRow(-scroll*80);	/* magic number?, change to user var */
		}
		/* pass input down to my children */
		return(UpdateInputC(0));
	}
	/* not over me */
	return(false);
}

/********************************************/

kGUIScrollControl::kGUIScrollControl(kGUIContainerObj *obj,int clicksize)
{
	m_obj=obj;
	m_maxwidth=0;
	m_maxheight=0;
	m_usehs=true;
	m_usevs=true;

	m_vscrollbar.SetClickSize(clicksize);
	m_vscrollbar.SetParent(obj);
	m_vscrollbar.SetVert();
	m_vscrollbar.SetEventHandler(this,CALLBACKNAME(ScrollMoveRow));

	m_hscrollbar.SetClickSize(clicksize);
	m_hscrollbar.SetParent(obj);
	m_hscrollbar.SetHorz();
	m_hscrollbar.SetEventHandler(this,CALLBACKNAME(ScrollMoveCol));

	m_scroll.SetEventHandler(this,CALLBACKNAME(ScrollEvent));
}

void kGUIScrollControl::SetInsideSize(int w,int h)
{
	w+=kGUI::GetSkin()->GetScrollbarWidth();
	h+=kGUI::GetSkin()->GetScrollbarHeight();
	m_obj->SetSize(w,h);
}

void kGUIScrollControl::MoveRow(int delta)
{
	int y=m_scroll.GetDestY();
	int vh=m_obj->GetZoneH();

	if(m_usehs)
		vh-=kGUI::GetSkin()->GetScrollbarWidth();

	y+=delta;
	if(y>(m_maxheight-vh))
		y=m_maxheight-vh;
	if(y<0)
		y=0;

    m_scroll.SetDestY(y);
	UpdateScrollBars();
}

void kGUIScrollControl::MoveCol(int delta)
{
	int x=m_scroll.GetDestX();
	int vw=m_obj->GetZoneW();

	if(m_usevs)
		vw-=kGUI::GetSkin()->GetScrollbarWidth();

	x+=delta;
	if(x>(m_maxwidth-vw))
		x=m_maxwidth-vw;
	if(x<0)
		x=0;

    m_scroll.SetDestX(x);
	UpdateScrollBars();
}

void kGUIScrollControl::Scrolled(void)
{
	kGUIZone cz;

	m_obj->SetChildScroll(m_scroll.GetCurrentX(),m_scroll.GetCurrentY());
	UpdateScrollBars();

	/* set the position of the row scrollbar */
	if(m_usevs)
	{
		m_obj->CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(m_scroll.GetCurrentX()+(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth()));
		cz.SetZoneY(m_scroll.GetCurrentY());
		cz.SetZoneW(kGUI::GetSkin()->GetScrollbarWidth());
		m_vscrollbar.MoveZone(&cz);
	}

	/* set the position of the column scrollbar */
	if(m_usehs)
	{
		m_obj->CopyChildZone(&cz);		/* get the child zone */
		cz.SetZoneX(m_scroll.GetCurrentX());
		cz.SetZoneY(m_scroll.GetCurrentY()+(cz.GetZoneH()-kGUI::GetSkin()->GetScrollbarHeight()));
		cz.SetZoneW(cz.GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth());
		cz.SetZoneH(kGUI::GetSkin()->GetScrollbarHeight());
		m_hscrollbar.SetZone(&cz);
	}

	m_obj->Dirty();
}

void kGUIScrollControl::DrawScroll(kGUICorners *c)
{
	if(m_usehs)
	{
		c->by-=kGUI::GetSkin()->GetScrollbarHeight();
		m_hscrollbar.Draw();
	}
	if(m_usevs)
	{
		c->rx-=kGUI::GetSkin()->GetScrollbarWidth();
		m_vscrollbar.Draw();
	}
}

void kGUIScrollControl::UpdateScrollBars(void)
{
	int vh=m_obj->GetZoneH();
	int vw=m_obj->GetZoneW();

	if(m_usehs)
		vh-=kGUI::GetSkin()->GetScrollbarWidth();

	if(m_usevs)
		vw-=kGUI::GetSkin()->GetScrollbarWidth();

	if(m_usevs==true)
	{
		int below;

		below=m_maxheight-vh-m_scroll.GetDestY();
		if(below<0)
			below=0;
		m_vscrollbar.SetValues(m_scroll.GetDestY(),vh,below);
	}
	if(m_usehs==true)
	{
		int right;

		right=m_maxwidth-vw-m_scroll.GetDestX();
		if(right<0)
			right=0;
		m_hscrollbar.SetValues(m_scroll.GetDestX(),vw,right);
	}
}

bool kGUIScrollControl::UpdateScrollInput(kGUICorners *c)
{
	bool over;
	kGUICorners cs;

	over=kGUI::MouseOver(c);
	if(over)
	{
		/* is the horizontal scroll bar on? */
		if(m_usehs==true)
		{
			if(m_hscrollbar.IsActive()==true)
				return(m_hscrollbar.UpdateInput());

			m_hscrollbar.GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
				return(m_hscrollbar.UpdateInput());
		}

		/* is the vertical scroll bar on? */
		if(m_usevs==true)
		{
			if(m_vscrollbar.IsActive()==true)
				return(m_vscrollbar.UpdateInput());

			m_vscrollbar.GetCorners(&cs);
			if(kGUI::MouseOver(&cs))
				return(m_vscrollbar.UpdateInput());
		}
		{
			int scroll=kGUI::GetMouseWheelDelta();
			kGUI::ClearMouseWheelDelta();
			if(scroll)
				MoveRow(-scroll*80);	/* magic number?, change to user var */
		}
	}
	/* not over me */
	return(false);
}

/* the top window is ALWAYS current */

void kGUIRootObj::SetCurrentChild(kGUIObj *cobj)
{
	SetCurrentChild((int)0);
}

void kGUIRootObj::SetCurrentChild(int num)
{
	kGUIContainerObj::SetCurrentChild(GetNumChildren(0)-1);
}
/***************** end **********************/
