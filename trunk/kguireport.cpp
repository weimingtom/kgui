/**********************************************************************************/
/* kGUI - kguireport.cpp                                                          */
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

/*! @file kguireport.cpp 
   @brief This is the class that handles printing reports.
 The report class uses the kGUIReportObj class for all iof the objects that     
 are printed. kGUI has report objects verions for all the regular gui objects   
 and for printing your own custom items like generated graphs or maps you can   
 make your own report objects                                                   
                                                                                
 Reports can be printed in both regular and landscape mode                      
 Reports can be printed at a different % size for smaller or larger printing    
 Reports can be printed with 2 or 4 report pages to be printed on a printer     
 page.                                                                          
                                                                                
 Also, the report class handles print-preview so you can see what the report    
 will look like with different settings, and applications can add their own     
 settings ( like tick boxes, comboboxs, inputboxes etc. ) to the print preview  
 panel so the user can have many customization options for printing reports */

#include "kgui.h"
#include "_text.h"
#include "kguireport.h"

void kGUIReportAreaObj::AddObj(kGUIReportObj *obj)
{
	unsigned int i;

	assert(obj->GetZoneH()>0,"Height is negative!");
	assert(obj->GetZoneW()>0,"Width is negative!");

	/* insert into the list sorted by Y position */

	i=m_numchildren;
	while(i)
	{
		if(obj->GetZoneY()>=m_children.GetEntry(i-1)->GetZoneY())
			break;
		--i;
	}
	if(i<m_numchildren)
		m_children.InsertEntry(m_numchildren,i,1);
	m_children.SetEntry(i,obj);
	++m_numchildren;
}

/* get height of the objects in this group */

int kGUIReportAreaObj::GetHeight(int pagenum,int start,int end)
{
	int i;
	kGUIReportObj *robj;
	kGUICorners c;
	int h=0;

	for(i=start;i<end;++i)
	{
		if(i==this->GetNumChildren())
			break;

		robj=m_children.GetEntry(i);
		if(robj->GetEnabled()==true)
		{
			if((robj->GetPage()==0) || (robj->GetPage()==pagenum))
			{
				robj->GetCorners(&c);
				if(c.by>h)
					h=c.by;
			}
		}
	}
	return(h);
}

int kGUIReportAreaObj::GetWidth(int pagenum,int start,int end)
{
	int i;
	kGUIReportObj *robj;
	kGUICorners c;
	int w=0;

	for(i=start;i<end;++i)
	{
		if(i==this->GetNumChildren())
			break;

		robj=m_children.GetEntry(i);
		if(robj->GetEnabled()==true)
		{
			if((robj->GetPage()==0) || (robj->GetPage()==pagenum))
			{
				robj->GetCorners(&c);
				if(c.rx>w)
					w=c.rx;
			}
		}
	}
	return(w);
}

bool kGUIReportAreaObj::AssignToPage(int pagenum,int availheight,int start,int *end)
{
	unsigned int i;
	int yoff=-1;
	kGUICorners c;
	kGUIReportObj *robj;

	/* get position of highest unassigned object */
	/* assume objects are sorted by 'y' already? */
	*(end)=start;
	for(i=start;i<m_numchildren;++i)
	{
		robj=m_children.GetEntry(i);
		if(robj->GetEnabled()==true)
		{
			if(robj->GetPage()==0)
			{
				/* get corners of this object */
				robj->GetCorners(&c);
		
				if(yoff==-1)
					yoff=c.ty;
				else if(c.by-yoff>availheight)
					return(false);	/* need another page */

				/* fits */
				robj->SetPos(c.lx,c.ty-yoff);
				robj->SetPage(pagenum);
				*(end)=i;
			}
		}
	}
	return(true);	/* all objects assigned */
}

/* purge all objects */
void kGUIReportAreaObj::PurgeObjs(void)
{
	m_numchildren=0;
}


void kGUIReportAreaObj::Draw(int pagenum)
{
	unsigned int i;
	kGUIReportObj *robj;
	kGUICorners c;
	kGUIDrawSurface *surface=kGUI::GetCurrentSurface();

	/* if draw offsets are in place, then shrink the right and bottom edges */
	/* to stop from drawing off of the surface */
	c.lx=0;
	c.ty=0;
	c.rx=surface->GetWidth()-surface->GetOffsetX();
	c.by=surface->GetHeight()-surface->GetOffsetY();

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);
	for(i=0;i<m_numchildren;++i)
	{
		robj=m_children.GetEntry(i);
		if(robj->GetEnabled()==true)
		{
			if((robj->GetPage()==0) || (robj->GetPage()==pagenum))
				robj->Draw();
		}
	}
	kGUI::PopClip();
}

void kGUIReportRectObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(m_framepix>0)
	{
		kGUI::DrawRect(c.lx,c.ty,c.rx+1,c.ty+m_framepix,m_framecolor);			/* top */
		kGUI::DrawRect(c.lx,(c.by+1)-m_framepix,c.rx+1,c.by+1,m_framecolor);	/* bottom */
		kGUI::DrawRect(c.lx,c.ty+1,c.lx+m_framepix,c.by,m_framecolor);			/* left */
		kGUI::DrawRect((c.rx+1)-m_framepix,c.ty+1,c.rx+1,c.by,m_framecolor);	/* right */
		c.lx+=m_framepix;
		c.rx+=-m_framepix;
		c.ty+=m_framepix;
		c.by+=-m_framepix;
	}
	kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,m_color);
}

double kGUIReportImageObj::CalcScaleToFit(void)
{
	float ws,hs,s;
	float imageh=(float)GetImageHeight();
	float imagew=(float)GetImageWidth();
	float objh=(float)(GetZoneH());
	float objw=(float)(GetZoneW());

	ws=objw/imagew;
	hs=objh/imageh;
	if(ws>hs)
		s=hs;
	else
		s=ws;
	return(s);
}

void kGUIReportImageObj::ShrinkToFit(void)
{
	double s=MIN(CalcScaleToFit(),1.0f);
	SetScale(s,s);
}

void kGUIReportImageObj::ExpandToFit(void)
{
	double s=MAX(CalcScaleToFit(),1.0f);
	SetScale(s,s);
}

/* expand or shrink attached image to fit in the imageobj area */
void kGUIReportImageObj::ScaleToFit(void)
{
	double s=CalcScaleToFit();
	SetScale(s,s);
}

void kGUIReportImageObj::CenterImage(void)
{
	int objh=GetZoneH();
	int objw=GetZoneW();
	int imageh=(int)GetScaledImageHeight();
	int imagew=(int)GetScaledImageWidth();

	if(imagew>objw)
		m_leftoff=0;
	else
		m_leftoff=-((objw-imagew)>>1);
	if(imageh>objh)
		m_topoff=0;
	else
		m_topoff=-((objh-imageh)>>1);
}

void kGUIReportImageObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUIImage::Draw(m_currentframe,c.lx-m_leftoff,c.ty-m_topoff);
}

void kGUIReportImageRefObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);

	m_image->SetScale(m_scalex,m_scaley);
	m_image->Draw(0,c.lx,c.ty);
}

kGUIReportTextObj::kGUIReportTextObj()
{
	SetFontSize(kGUI::GetDefReportFontSize());
	SetFontID(kGUI::GetDefReportFontID());
	SetFrame(false);
	m_framecolor=DrawColor(0,0,0);
	m_usebgcolor=false;
	m_bgcolor=DrawColor(255,255,255);
	m_shaded=false;
	SetColor(DrawColor(0,0,0));
	m_iscurrency=false;
}

void kGUIReportTextObj::Changed(void)
{
	/* only expand to fit, don't shrink */
	SetSize(MAX(GetWidth()+3+3,GetZoneW()),MAX((int)GetLineHeight()+3+3,GetZoneH()));
}

void kGUIReportTextObj::SetFrame(bool df)
{
	m_drawframe=df;
	Changed();
}

int kGUIReportTextObj::Height(int w)
{
	int totalheight;
	int line;
	int lineheight=kGUIText::GetLineHeight()+2;
	CalcLineList(w-6);
	line=GetNumLines();
	totalheight=(lineheight*line)+6;
	return(totalheight);
}

void kGUIReportTextObj::Draw(void)
{
	int w,h,f;
	kGUICorners c;

	GetCorners(&c);
	w=c.rx-c.lx;
	h=c.by-c.ty;

	if(m_drawframe==true)
	{
		kGUI::DrawRect(c.lx,c.ty,c.rx+1,c.ty+1,m_framecolor);	/* top */
		kGUI::DrawRect(c.lx,c.by,c.rx+1,c.by+1,m_framecolor);	/* bottom */
		kGUI::DrawRect(c.lx,c.ty+1,c.lx+1,c.by,m_framecolor);	/* left */
		kGUI::DrawRect(c.rx,c.ty+1,c.rx+1,c.by,m_framecolor);	/* right */
		f=1;
	}
	else
		f=0;

	if(m_usebgcolor)
		kGUI::DrawRect(c.lx+f,c.ty+f,c.rx-f,c.by-f,m_bgcolor);		/* background */
	else
	{
		if(m_shaded==true)
			kGUI::DrawRect(c.lx+f,c.ty+f,c.rx-f,c.by-f,DrawColor(232,232,232));		/* background */
	}

	if(GetLen())	/* check for no string as if no string then don't bother to prepend '$' */
	{
		if(m_iscurrency==true)
			Insert(0,"$");

		CalcLineList(w-6);

		kGUI::PushClip();
		kGUI::ShrinkClip(&c);
		kGUIText::Draw(c.lx+3,c.ty+3,w-6,h-6);
		if(m_iscurrency==true)
			Delete(0,1);
		kGUI::PopClip();
	}
}

void kGUIReportTickboxObj::Draw(void)
{
	kGUICorners c;

	GetCorners(&c);
	kGUI::DrawRectBevelIn(c.lx+2,c.ty+2,c.lx+15,c.ty+15);
	c.lx+=2;
	c.ty+=2;
	kGUI::GetSkin()->DrawTickbox(&c,m_scale,m_selected,false);
}

/*******************************************************************************/

kGUIReportRowHeaderObj::~kGUIReportRowHeaderObj()
{
	int i;
	for(i=0;i<m_numcolumns;++i)
	{
		delete m_colnames.GetEntry(i);
	}
}

void kGUIReportRowHeaderObj::SetNumColumns(int n)
{
	int i;
	kGUIString *s;

	/* delete any existing ones */
	for(i=0;i<m_numcolumns;++i)
		delete m_colnames.GetEntry(i);

	m_numcolumns=n;
	m_colnames.Alloc(n);
	m_colxs.Alloc(n);
	m_colws.Alloc(n);
	for(i=0;i<m_numcolumns;++i)
	{
		s=new kGUIString();
		m_colnames.SetEntry(i,s);
	}
}

void kGUIReportRowHeaderObj::UpdateSize(void)
{
	int i,x;
	int maxx;

	maxx=0;
	for(i=0;i<m_numcolumns;++i)
	{
		x=m_colxs.GetEntry(i)+m_colws.GetEntry(i);
		if(x>maxx)
			maxx=x;
	}
	SetZoneW(maxx);
}

void kGUIReportRowHeaderObj::Draw(void)
{
	int i,x,y,w,h;
	int nc=GetNumColumns();
	kGUIReportTextObj ro;

	y=GetZoneY();
	h=GetZoneH();
	ro.SetFontID(1);	/* bold */
	for(i=0;i<nc;++i)
	{
		w=GetColWidth(i);
		if(w>0)
		{
			x=GetColX(i);
			ro.SetZone(x,y,w,h);
			ro.SetString(GetColName(i));
			ro.Draw();
		}
	}
}

void kGUIReportRowObj::SetHeader(kGUIReportRowHeaderObj *parent)
{
	int i,nc,x,w;
	kGUIReportObj **olist;
	kGUIReportObj *obj;

	m_parent=parent;
	nc=parent->GetNumColumns();
	olist=GetObjectList();
	m_colspan.Alloc(nc);
	for(i=0;i<nc;++i)
	{
		m_colspan.SetEntry(i,1);
		w=m_parent->GetColWidth(i);
		if(w>0)
		{
			x=m_parent->GetColX(i);

			obj=olist[i];
			obj->SetZoneX(x);
			obj->SetZoneW(w);
		}
	}
	SetZoneW(parent->GetZoneW());
}

void kGUIReportRowObj::Draw(void)
{
	int i,x,y,w,h;
	int c,c2;
	int nc=m_parent->GetNumColumns();
	kGUIReportTextObj ro;
	kGUIReportObj **olist;
	kGUIReportObj *obj;
	kGUICorners cc;

	cc.lx=GetZoneX();
	cc.rx=cc.lx+GetZoneW()+1;
	cc.ty=GetZoneY();
	cc.by=cc.ty+GetZoneH()+1;
	kGUI::PushClip();
	kGUI::ShrinkClip(&cc);

	y=GetZoneY();
	h=GetZoneH();

	ro.SetFrame(GetFrame());
	ro.SetBGShade(m_shaded);
	olist=GetObjectList();
	for(i=0;i<nc;i+=m_colspan.GetEntry(i))
	{
		w=0;
		c2=i+m_colspan.GetEntry(i);
		for(c=i;c<c2;++c)
			w+=m_parent->GetColWidth(c);
		if(w>0)
		{
			x=m_parent->GetColX(i);
			ro.SetZone(x,y,w,h);
			ro.Draw();

			obj=olist[i];
			obj->SetZone(x,y,w,h);
			obj->Draw();
		}
	}
	kGUI::PopClip();
}

/************************************************************************/


kGUIReport::kGUIReport()
{
	m_defsize=kGUI::GetDefReportFontSize();
	Init();
}

kGUIReport::kGUIReport(int defsize)
{
	m_defsize=defsize;
	kGUI::SetDefReportFontSize(defsize);
	Init();
}

void kGUIReport::Init(void)
{
	m_printjob=kGUI::GetSystem()->AllocPrintJob();
	m_pid=0;
	m_numcopies=1;
	m_numgrouppages=0;
	m_numwide=0;
	m_numpages=0;
	m_landscape=false;
	m_okadd=false;
	m_bitmapmode=false;
	m_drawscale=1.0f;
	m_multipage=1;
	m_allowmultipages=true;
	m_numusercontrols=0;
	m_usercontrollist.Init(32,16);
	m_scale.Sprintf("100");

	m_numpurgeobjects=0;
	m_purgeobjects.Init(128,64);
}


kGUIReport::~kGUIReport()
{
	if(m_closecallback.IsValid())
		m_closecallback.Call(this);
	PurgeObjs();
	delete m_printjob;
}

void kGUIReport::PurgeObjs(void)
{
	unsigned int i;
	kGUIReportObj *obj;

	/* purge all objects that the user code asked to be purged */
	for(i=0;i<m_numpurgeobjects;++i)
	{
		obj=m_purgeobjects.GetEntry(i);
		delete obj;
	}
	m_numpurgeobjects=0;

	/* set number of objects in each of these sections to zero */
	m_pageheader.PurgeObjs();
	m_pagefooter.PurgeObjs();
	m_header.PurgeObjs();
	m_footer.PurgeObjs();
	m_body.PurgeObjs();
}

void kGUIReport::AddObjToSection(int section,kGUIReportObj *obj,bool purge)
{
	assert(m_okadd,"Not allowed to add objects at this time!");
	
	/* set to page zero to flag that this object is not assigned to a particular page yet */
	obj->SetPage(0);
	/* if the user wants us to purge this for them add it to the purge list */
	if(purge)
		m_purgeobjects.SetEntry(m_numpurgeobjects++,obj);

	switch(section)
	{
	case REPORTSECTION_PAGEHEADER:
		m_pageheader.AddObj(obj);
	break;
	case REPORTSECTION_PAGEFOOTER:
		m_pagefooter.AddObj(obj);
	break;
	case REPORTSECTION_HEADER:
		m_header.AddObj(obj);
	break;
	case REPORTSECTION_BODY:
		m_body.AddObj(obj);
	break;
	case REPORTSECTION_FOOTER:
		m_footer.AddObj(obj);
	break;
	}
}

void kGUIReport::ChangePage(int move)
{
	m_curpage+=move;
	if(m_curpage<1)
		m_curpage=1;
	if(m_curpage>m_numgrouppages)
		m_curpage=m_numgrouppages;

	m_pagerangebox.Sprintf("1-%d",m_numgrouppages);
	m_pagenumbox.Sprintf("%d of %d",m_curpage,m_numgrouppages);
	m_pagescrollbar.SetValues(m_curpage-1,1,m_numgrouppages-m_curpage);
	m_pagescrollbar.Dirty();
	DrawPage(m_curpage);
	m_imageobj.Dirty();
}

void kGUIReport::ChangeScale(int move)
{
	double scale;

	m_scalepercent+=move;
	if(m_scalepercent<0)
		m_scalepercent=0;
	if(m_scalepercent>100)
		m_scalepercent=100;

	m_scalescrollbar.SetValues(m_scalepercent,1,100-m_scalepercent);
	m_scalescrollbar.Dirty();

	scale=1.0f/(1.0f+((m_scalepercent*(m_maxscale-1.0f)/100.0f)));
	m_imageobj.SetScale(scale,scale);
	m_imageobj.Dirty();
}

void kGUIReport::ChangeFormatEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
		ChangeFormat();
}


void kGUIReport::ChangeFormat(void)
{
	m_landscape=m_format.GetSelection()==1?true:false;
	if(m_scale.GetInt()<33)
		m_scale.SetInt(33);
	m_drawscale=100.0f/m_scale.GetDouble();
	m_multipage=m_multi.GetSelection();

	CalcPages();
	ChangePage(0);
	m_curpage=1;
	CalcPageSize(kGUI::GetSurfaceWidth()-100,kGUI::GetSurfaceHeight()-140);
	PositionPage();
	ChangeScale(0);
}

void kGUIReport::Printer_Changed(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		m_pid=m_printerlist.GetSelection();
		ChangeFormat();	/* since page size can change, this needs to be called */
	}
}

void kGUIReport::ClickPrint(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		int startpage,endpage;
		const char *rtext;

		/* print to the user selected printer */
		if(m_pagerangebox.IsNull())
			return;	/* nothing to print */

		/* get number of pages incase user changed it */
		SetNumCopies(this->m_pagecopiesbox.GetInt());

		rtext=m_pagerangebox.GetString();
		if(rtext[0]=='-')
		{
			startpage=1;
			endpage=atoi(rtext+1);
		}
		else
		{
			startpage=atoi(rtext);
			while((rtext[0]>='0') && (rtext[0]<='9'))
				++rtext;
			if(rtext[0]=='-')
				endpage=atoi(rtext+1);
			else
				endpage=startpage;
		}
		if(startpage<1)
			startpage=1;
		if(endpage>m_numgrouppages)
			endpage=m_numgrouppages;

		if(startpage>endpage)
			return;	/* invalid selection */


		m_startpage=startpage;
		m_endpage=endpage;

		/* if printing more than 4 pages then ask to make sure */
		if(endpage-startpage>4)
		{
			kGUIMsgBoxReq *msg;

			/* ask, are you sure */
			msg=new kGUIMsgBoxReq(MSGBOX_OK|MSGBOX_CANCEL,this,CALLBACKNAME(ClickPrint2),true,"Print %d pages?",(endpage-startpage)+1);
			msg->GetTitle()->SetString("Are you sure?");
		}
		else
			ClickPrint2(MSGBOX_OK);
	}
}

void kGUIReport::ClickPrint2(int pressed)
{
	if(pressed==MSGBOX_OK)
	{
		if(Print(m_startpage,m_endpage)==false)
		{
			kGUIMsgBoxReq *emsg;

			/* error trying to print */
			emsg=new kGUIMsgBoxReq(MSGBOX_OK,false,"Error: Could not print document.");
		}
	}
}

void kGUIReport::CalcPageSize(int sw,int sh)
{
	double scale,ow,oh;
	int rw,rh;
	
	rw=m_drawsurface.GetWidth();
	rh=m_drawsurface.GetHeight();

	if((rw<=sw) && (rh<=sh))
	{
#if 1
		ow=((double)rw/(double)sw)*1.1f;		
		oh=((double)rh/(double)sh)*1.1f;		
		if(ow>oh)
			scale=ow;
		else
			scale=oh;
		m_iw=(int)(rw/scale)+kGUI::GetSkin()->GetScrollbarWidth();
		m_ih=(int)(rh/scale)+kGUI::GetSkin()->GetScrollbarHeight();
#else
		m_iw=rw+kGUI::GetSkin()->GetScrollbarWidth();
		m_ih=rh+kGUI::GetSkin()->GetScrollbarHeight();
		scale=1.0f;
#endif
	}
	else
	{
		ow=((double)rw/(double)sw)*1.1f;		
		oh=((double)rh/(double)sh)*1.1f;		
		if(ow>oh)
			scale=ow;
		else
			scale=oh;
		m_iw=(int)(rw/scale)+kGUI::GetSkin()->GetScrollbarWidth();
		m_ih=(int)(rh/scale)+kGUI::GetSkin()->GetScrollbarHeight();
	}
	m_maxscale=scale;
}

void kGUIReport::PositionPage(void)
{
	m_imageobj.SetPos((m_ww-(m_iw+kGUI::GetSkin()->GetScrollbarWidth()))>>1,m_wy);
	m_imageobj.SetSize(m_iw,m_ih);
	m_imageobj.SetScale(1.0f/m_maxscale,1.0f/m_maxscale);
	m_drawsurface.SetOffsets(0,0);
	m_imageobj.SetMemImage(0,GUISHAPE_SURFACE,m_drawsurface.GetWidth(),m_drawsurface.GetHeight(),m_drawsurface.GetBPP(),(unsigned char *)(m_drawsurface.GetSurfacePtr(0,0)));
	m_imageobj.SetShowScrollBars(true);
}

/* decrement scale till page fits */
void kGUIReport::Fit(unsigned int maxwide,unsigned int maxtall)
{
	unsigned int scale;

	scale=m_scale.GetInt();
	do
	{
		CalcPages();
		if((unsigned int)m_numpages<=maxtall && (unsigned int)m_numwide<=maxwide)
			return;		/* fits */
		--scale;
		SetScale(scale);
	}while(1);
}

void kGUIReport::Preview(void)
{
	unsigned int i;
	kGUIString title;

	m_scalepercent=100;
	CalcPages();
//	DrawPage(1,true);
	m_curpage=1;
	/* calculate size for the window */
	CalcPageSize(kGUI::GetBackground()->GetChildZoneW()-100,kGUI::GetBackground()->GetChildZoneH()-140);
	m_controls.SetMaxWidth(kGUI::GetSurfaceWidth()-100);

	m_previewwindow.SetAllowButtons(WINDOWBUTTON_CLOSE);

	/* make the print preview window */
	title.SetString(kGUI::GetString(KGUISTRING_PRINTPREVIEW));
	title.Append(GetName());
	m_previewwindow.GetTitle()->SetString(&title);

	/* show current page number */
	m_pagenumcaption.SetPos(0,0);
	m_pagenumcaption.SetString(kGUI::GetString(KGUISTRING_CURRENT));
	m_pagenumbox.SetPos(0,15);
	m_pagenumbox.SetSize(80,20);
	m_pagenumbox.SetLocked(true);
	m_controls.AddObjects(2,&m_pagenumcaption,&m_pagenumbox);

	/* scroll bar for changing pages */

	m_pagescrollcaption.SetPos(0,0);
	m_pagescrollcaption.SetString(kGUI::GetString(KGUISTRING_PAGE));

	m_pagescrollbar.SetPos(0,15);
	m_pagescrollbar.SetHorz();
	m_pagescrollbar.SetSize(100,kGUI::GetSkin()->GetScrollbarHeight());
	m_pagescrollbar.SetEventHandler(this,& CALLBACKNAME(ScrollChangePage));
	m_controls.AddObjects(2,&m_pagescrollcaption,&m_pagescrollbar);

	/* scroll bar for changing view scale */
	m_scalescrollcaption.SetPos(0,0);
	m_scalescrollcaption.SetString(kGUI::GetString(KGUISTRING_ZOOM));

	m_scalescrollbar.SetPos(0,15);
	m_scalescrollbar.SetHorz();
	m_scalescrollbar.SetSize(100,kGUI::GetSkin()->GetScrollbarHeight());
	m_scalescrollbar.SetEventHandler(this,CALLBACKNAME(ScrollChangeScale));
	m_controls.AddObjects(2,&m_scalescrollcaption,&m_scalescrollbar);

	/* combo box for selecting printer */
	m_printerlist.SetPos(0,15);
	m_printerlist.SetNumEntries(kGUI::GetNumPrinters());
	for(i=0;i<kGUI::GetNumPrinters();++i)
		m_printerlist.SetEntry(i,kGUI::GetPrinterObj(i)->GetName(),i);
	m_printerlist.SetSelection(m_pid);
	i=MIN(350,m_printerlist.GetWidest());
	m_printerlist.SetSize(i,20);
	m_printerlist.SetEventHandler(this,CALLBACKNAME(Printer_Changed));
	m_controls.AddObjects(1,&m_printerlist);

	/* combo box for portrait/landscape */
	m_format.SetPos(0,15);
	m_format.SetNumEntries(2);
	m_format.SetEntry(0,kGUI::GetString(KGUISTRING_PORTRAIT),0);
	m_format.SetEntry(1,kGUI::GetString(KGUISTRING_LANDSCAPE),1);
	m_format.SetSelection(m_landscape==true?1:0);
	m_format.SetSize(m_format.GetWidest(),20);
	m_format.SetEventHandler(this,& CALLBACKNAME(ChangeFormatEvent));
	m_controls.AddObjects(1,&m_format);

	/* scale up/down */
	m_scalecaption.SetPos(0,0);
	m_scalecaption.SetString(kGUI::GetString(KGUISTRING_SCALEPERCENT));
	m_scale.SetPos(0,15);
	m_scale.SetSize(50,20);
	m_scale.SetEventHandler(this,& CALLBACKNAME(ChangeFormatEvent));
	m_controls.AddObjects(2,&m_scalecaption,&m_scale);

	/* print more than 1 page per page */

	m_multicaption.SetPos(0,0);
	m_multicaption.SetString(kGUI::GetString(KGUISTRING_MULTIPAGE));
	m_multi.SetPos(0,15);
	m_multi.SetSize(50,20);
	m_multi.SetNumEntries(3);
	m_multi.SetEntry(0,"1",1);
	m_multi.SetEntry(1,"2",2);
	m_multi.SetEntry(2,"4",4);
	m_multi.SetEventHandler(this,CALLBACKNAME(ChangeFormatEvent));
	if(m_allowmultipages)
		m_controls.AddObjects(2,&m_multicaption,&m_multi);

	/* input box for range of pages to print */
	m_printrangecaption.SetPos(0,0);
	m_printrangecaption.SetString(kGUI::GetString(KGUISTRING_PRINTRANGE));
	m_pagerangebox.SetPos(0,15);
	m_pagerangebox.SetSize(60,20);
	m_pagerangebox.Sprintf("1-%d",m_numgrouppages);
	m_controls.AddObjects(2,&m_printrangecaption,&m_pagerangebox);

	/* input box for range of pages to print */
	m_printcopiescaption.SetPos(0,0);
	m_printcopiescaption.SetString(kGUI::GetString(KGUISTRING_COPIES));
	m_pagecopiesbox.SetPos(0,15);
	m_pagecopiesbox.SetSize(40,20);
	m_pagecopiesbox.Sprintf("%d",m_numcopies);
	m_controls.AddObjects(2,&m_printcopiescaption,&m_pagecopiesbox);

	/* print button to print selected range to selected printer */
	m_printbutton.SetString(kGUI::GetString(KGUISTRING_PRINT));
	m_printbutton.SetPos(0,10);
	m_printbutton.SetSize(75,25);
	m_printbutton.SetEventHandler(this,& CALLBACKNAME(ClickPrint));
	m_controls.AddObjects(1,&m_printbutton);

	m_controls.SetPos(0,5);
	m_previewwindow.AddObject(&m_controls);

	/* calc window width */
	m_previewwindow.SetSize(m_iw,m_ih);
	m_previewwindow.ExpandToFit();		/* expand window to hold all children */
	m_wy=m_controls.GetZoneY()+m_controls.GetZoneH()+3;
	m_ww=m_previewwindow.GetZoneW();

	if(m_numusercontrols)
	{
		m_usercontrols.SetPos(0,m_wy);
		m_usercontrols.SetMaxWidth(m_previewwindow.GetChildZoneW());

		for(i=0;i<m_numusercontrols;++i)
			m_usercontrols.AddObject(m_usercontrollist.GetEntry(i));

		m_previewwindow.AddObject(&m_usercontrols);
		/* each user control can have it's own callback and then this one would be called next */
		m_usercontrols.SetEventHandler(this,CALLBACKNAME(ChangeFormatEvent));
		m_wy+=m_usercontrols.GetZoneH();
	}
	m_wy+=10;	/* small gap */
	m_wh=m_wy+m_ih+kGUI::GetSkin()->GetScrollbarHeight();

	m_previewwindow.SetTop(true);	/* force window to stay on top till closed */
	kGUI::AddWindow(&m_previewwindow);
	m_previewwindow.SetEventHandler(this,CALLBACKNAME(WindowEvent));

	/* width is initially set to the maximum size for viewing */
	/* the page, but if page is narrow then it might need to */
	/* be made wider to accomidate all the top buttons */

	/* is window wide enough for all the header items? */

	PositionPage();
	m_previewwindow.AddObject(&m_imageobj);
	m_previewwindow.ExpandToFit();
	m_wh=m_previewwindow.GetZoneH();
	/* center window on screen */
	m_previewwindow.SetPos(MAX(0,((kGUI::GetScreenWidth())-m_ww)/2),MAX(0,((kGUI::GetScreenHeight())-m_wh)/2));
	ChangeScale(0);
	kGUI::ReDraw();
	ChangePage(0);
}

void kGUIReport::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		delete this;
	break;
	}
}

/* if the report returns <0 for either width or height, then get max size from printer */

void kGUIReport::GetSurfaceSize(double *ppw,double *pph)
{
	kGUIPrinter *cp;
	int printermaxw,printermaxh,printerppih,printerppiv;
	int printerppimin;
	double pw,ph;

	cp=kGUI::GetPrinterObj(m_pid);
	cp->GetInfo(&printermaxw,&printermaxh,&printerppih,&printerppiv);
	
	/* if the priniter has different ppi for horiz and vert then */
	/* use the lower one to keep the picture from distorting */

	/* todo: this code is not complete yet: */
	/* this need to keep track of the scale change and pass the scale */
	/* expand value down to the lower printer driver */

	printerppimin=printerppih;
	if(printerppiv<printerppimin)
		printerppimin=printerppiv;

	pw=GetPageWidth();
	if(pw<0)
		pw=(double)printermaxw/(double)printerppih;

	ph=GetPageHeight();
	if(ph<0)
		ph=(double)printermaxh/(double)printerppiv;

	/* remove margins from area */
	pw-=GetLeftMargin();
	pw-=GetRightMargin();
	
	ph-=GetTopMargin();
	ph-=GetBottomMargin();

	*(ppw)=pw;
	*(pph)=ph;
}

/* return the area for a subpage using the current mode */
void kGUIReport::GetSubHistoryRecord(int subpage,int *pw,int *ph,int *poffx,int *poffy)
{
	int	rw=GetSurfaceWidthPix();
	int	rh=GetSurfaceHeightPix();
	int gap=GetPPI()/4;	/* 1/4 inch gap */

	switch(m_multipage)
	{
	case 1:
		pw[0]=rw;
		ph[0]=rh;
		poffx[0]=0;
		poffy[0]=0;
	break;
	case 2:
		if(m_landscape==true)
		{
			rw>>=1;
			pw[0]=rw-gap;
			ph[0]=rh;
			if(!subpage)
			{
				poffx[0]=0;
				poffy[0]=0;
			}
			else
			{
				poffx[0]=rw+gap;
				poffy[0]=0;
			}
		}
		else
		{
			rh>>=1;
			pw[0]=rw;
			ph[0]=rh-gap;
			if(!subpage)
			{
				poffx[0]=0;
				poffy[0]=0;
			}
			else
			{
				poffx[0]=0;
				poffy[0]=rh+gap;
			}
		}
	break;
	case 4:
		rw>>=1;
		rh>>=1;
		pw[0]=rw-gap;
		ph[0]=rh-gap;
		poffx[0]=(!(subpage&1))?0:rw+gap;
		poffy[0]=(!(subpage&2))?0:rh+gap;
	break;
	}
}

void kGUIReport::CalcPages(void)
{
	int rw,rh;
	bool bodydone;
	int pagenum;
	double pw,ph;
	int entry,lastentry;
	double scale=m_drawscale;
	int prw,prh,w,maxw;

	switch(m_multipage)
	{
	case 1:
		scale=m_drawscale;
	break;
	case 2:
		scale=m_drawscale*2.0f;
	break;
	case 4:
		scale=m_drawscale*2.0f;
	break;
	}

	GetSurfaceSize(&pw,&ph);
	if(!m_landscape)
		rw=(int)(pw*GetPPI()*scale);
	else
		rw=(int)(ph*GetPPI()*scale);

	while(rw&3)
		++rw;	/* convert to multiple of 4 */

	if(!m_landscape)
		rh=(int)(ph*GetPPI()*scale);
	else
		rh=(int)(pw*GetPPI()*scale);

	m_drawsurface.Init(rw,rh);
	m_drawsurface.Clear(DrawColor(255,255,255));

	/* calculate the number of pages, so the user can print, page x of y */

	PurgeObjs();
	m_okadd=true;
	kGUI::SetDefReportFontSize(m_defsize);
	Setup();
	m_okadd=false;
	pagenum=0;

	/* calculate heights of the headers/footers */
	m_phh=m_pageheader.GetHeight(0,0,m_pageheader.GetNumChildren());
	m_pfh=m_pagefooter.GetHeight(0,0,m_pagefooter.GetNumChildren());
	m_bhh=m_header.GetHeight(0,0,m_header.GetNumChildren());
	m_bfh=m_footer.GetHeight(0,0,m_footer.GetNumChildren());

	maxw=m_pageheader.GetWidth(0,0,m_pageheader.GetNumChildren());
	w=m_pagefooter.GetWidth(0,0,m_pagefooter.GetNumChildren());
	if(w>maxw)
		maxw=w;
	w=m_header.GetWidth(0,0,m_header.GetNumChildren());
	if(w>maxw)
		maxw=w;
	w=m_footer.GetWidth(0,0,m_footer.GetNumChildren());
	if(w>maxw)
		maxw=w;

	pagenum=0;
	entry=0;

	prw=rw;
	prh=rh;

	switch(m_multipage)
	{
	case 1:
		prw=rw;
		prh=rh;
	break;
	case 2:
		if(m_landscape==true)
		{
			prh=rh;
			prw=rw/2;
		}
		else
		{
			prh=rh/2;
			prw=rw;
		}
	break;
	case 4:
		prw=rw/2;
		prh=rh/2;
	break;
	}

	do
	{
		++pagenum;

		m_availheight=prh-m_phh-m_pfh;
		if(pagenum==1)
			m_availheight-=m_bhh;		/* body header goes on page 1 */

		bodydone=m_body.AssignToPage(pagenum,m_availheight,entry,&lastentry);
		
		/* todo, save first and last index assigned to this page */		

		m_availheight-=m_body.GetHeight(pagenum,entry,lastentry+1);		

		w=m_body.GetWidth(pagenum,entry,lastentry+1);		
		if(w>maxw)
			maxw=w;

		/* stop when body sets done flag and there is available space */
		/* for the body footer */
		if(bodydone==true && (m_availheight>=m_bfh))
			break;
		entry=lastentry+1;
	}while(1);
	m_numpages=pagenum;
	m_numgrouppages=(pagenum+(m_multipage-1))/m_multipage;

	if(maxw>prw)
		m_numwide=2;
	else
		m_numwide=1;
}

/* true=print ok, false=error printing */
bool kGUIReport::Print(int startpage,int endpage)
{
	int i;
	kGUIDrawSurface m_rotateddrawsurface;
	kGUIPrinter *p;
	bool prerotate;
	bool rc;

	/* this will draw in one of two modes */

	/* bitmap mode just sends the drawn display to the printer */
	/* this is slower but easier */

	/* with bitmap mode off, small primitive objects are sent to the */
	/* printer, this is much quicker then sending a large bitmap */

	p=kGUI::GetPrinterObj(m_pid);

	/* is this a valid printer? */
	if(p->GetValid()==false)
		return(false);

	if(m_landscape==true && m_bitmapmode==true && kGUI::NeedRotatedSurfaceForPrinting()==true)
	{
		int pw=0,ph=0;

		GetPageSizePixels(&pw,&ph);
		while(ph&3)
			++ph;	/* convert to multiple of 4 */

		prerotate=true;
		m_landscape=false;
		m_rotateddrawsurface.Init(ph,pw);
	}
	else
		prerotate=false;

	/* if not already done by print-preview */
	if(m_numpages==0)
		CalcPages();

	/* 0,0 = print whole thing */
	if(!startpage)
		startpage=1;

	if(!endpage)
		endpage=m_numgrouppages;

	rc=m_printjob->Start(p,GetName(),(endpage+1)-startpage,GetNumCopies(),GetPPI(),GetLeftMargin(),GetRightMargin(),GetTopMargin(),GetBottomMargin(),&m_drawsurface,m_landscape);
	if(rc==true)
	{
		for(i=startpage;i<=endpage;++i)
		{
			m_printjob->StartPage();
			if(m_bitmapmode==false)
				m_drawsurface.SetPrintJob(m_printjob);
			DrawPage(i);
			if(m_bitmapmode==true)
			{
				if(prerotate==true)
				{
					m_rotateddrawsurface.UnRotateSurface(&m_drawsurface);
					m_printjob->PrintSurface(&m_rotateddrawsurface);
				}
				else
					m_printjob->PrintSurface(&m_drawsurface);
			}
			m_drawsurface.SetPrintJob(0);
			m_printjob->EndPage();
		}
		rc=m_printjob->End();
	}

	/* if landscape was turned off then put it back on when done */
	if(prerotate)
		m_landscape=true;

	return(rc);
}

void kGUIReport::DrawPage(int grouppage)
{
	int h,rw=0,rh=0;
	int offx=0,offy=0;
	int subpage;
	int page;
	kGUICorners c;

	/* select it as the draw-to bitmap */

	m_drawsurface.SetOffsets(0,0);
	kGUI::SetCurrentSurface(&m_drawsurface);
	m_drawsurface.Clear(DrawColor(255,255,255));	/* clear page to white */

	page=((grouppage-1)*m_multipage+1);
	for(subpage=0;(subpage<m_multipage) && (page<=m_numpages);++subpage)
	{
		kGUI::PushClip();
		kGUI::ResetClip();	/* set clip to full surface on stack */

		GetSubHistoryRecord(subpage,&rw,&rh,&offx,&offy);
		c.lx=0;
		c.ty=0;
		c.rx=rw;
		c.by=rh;
		kGUI::ShrinkClip(&c);

		Setup(page);	/* call users report setup code for this page */

		m_yoff=0;
		m_availheight=rh;

		m_drawsurface.SetOffsets(offx,m_yoff+offy);
		m_pageheader.Draw(page);
		m_yoff+=m_phh;m_availheight-=(m_phh+m_pfh);		

		if(page==1)
		{
			m_drawsurface.SetOffsets(offx,m_yoff+offy);
			m_header.Draw(page);
			m_yoff+=m_bhh;m_availheight-=m_bhh;
		}

		m_drawsurface.SetOffsets(offx,m_yoff+offy);
		m_body.Draw(page);
		h=m_body.GetHeight(page,0,m_body.GetNumChildren());
		m_yoff+=h;m_availheight-=h;
		if(page==m_numpages)
		{
			m_drawsurface.SetOffsets(offx,m_yoff+offy);
			m_footer.Draw(page);
		}
		m_yoff=rh-m_pfh;
		m_availheight=m_pfh;
		m_drawsurface.SetOffsets(offx,m_yoff+offy);
		m_pagefooter.Draw(page);
		kGUI::PopClip();
		++page;
	}
	m_drawsurface.SetOffsets(0,0);
	kGUI::RestoreScreenSurface();
//	if(showbusy)
//		delete b;
}

/* return page width and height in pixels */
void kGUIReport::GetPageSizePixels(int *ppw,int *pph)
{
	int offx,offy;

	GetSubHistoryRecord(0,ppw,pph,&offx,&offy);
}
