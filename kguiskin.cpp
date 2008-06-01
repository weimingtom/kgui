/**********************************************************************************/
/* kGUI - kguiskin.cpp                                                            */
/*                                                                                */
/* Programmed by Kevin Pickell                                                    */
/*                                                                                */
/* http://code.google.com/p/kgui/	                              */
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
#include "defskin.h"

/* this file contains the default gui art, it is in a big file and then converted */
/* into a char array so it can be included directly into the library */
/* to add shapes just place them in the 'big' directory and the make process will */
/* automatically add them */

#include "_data.cpp"

kGUISkin *AllocDefSkin(void)
{
	kGUISkin *defskin;

	defskin=new DefSkin();
	return(defskin);
}

DefSkin::DefSkin()
{
	m_bf.SetMemory(bin__data,sizeof(bin__data));
	m_bf.Load();
	DataHandle::AddBig(&m_bf);

	/* colors */
	m_skincolors[SKINCOLOR_BACKGROUND]=DrawColor(206,233,236);
	m_skincolors[SKINCOLOR_WINDOWBACKGROUND]=DrawColor(236,233,216);
	m_skincolors[SKINCOLOR_CONTAINERBACKGROUND]=DrawColor(236,233,255);

	m_skinshapes[SKIN_WINDOWTOPLEFT].SetFilename("_tl.gif");
	m_skinshapes[SKIN_WINDOWTOPMIDDLE].SetFilename("_tc.gif");
	m_skinshapes[SKIN_WINDOWTOPRIGHT].SetFilename("_tr.gif");

	m_skinshapes[SKIN_WINDOWLEFTSIDE].SetFilename("_ls.gif");
	m_skinshapes[SKIN_WINDOWRIGHTSIDE].SetFilename("_rs.gif");

	m_skinshapes[SKIN_WINDOWBOTTOMLEFT].SetFilename("_bl.gif");
	m_skinshapes[SKIN_WINDOWBOTTOMMIDDLE].SetFilename("_bc.gif");
	m_skinshapes[SKIN_WINDOWBOTTOMRIGHT].SetFilename("_br.gif");

	m_skinshapes[SKIN_WINDOWCLOSEBUTTON].SetFilename("_wclose.gif");
	m_skinshapes[SKIN_WINDOWFULLBUTTON].SetFilename("_wfull.gif");
	m_skinshapes[SKIN_WINDOWMINIMIZEBUTTON].SetFilename("_wmin.gif");

	m_skinshapes[SKIN_WINDOWCLOSEBUTTONOVER].SetFilename("_wclose2.gif");
	m_skinshapes[SKIN_WINDOWFULLBUTTONOVER].SetFilename("_wfull2.gif");
	m_skinshapes[SKIN_WINDOWMINIMIZEBUTTONOVER].SetFilename("_wmin2.gif");

	m_skinshapes[SKIN_MINIMIZEDWINDOW].SetFilename("_mini.gif");

	m_skinshapes[SKIN_TICK].SetFilename("_tick.gif");

	m_skinshapes[SKIN_TABLEFT].SetFilename("_tabl.gif");
	m_skinshapes[SKIN_TABCENTER].SetFilename("_tabc.gif");
	m_skinshapes[SKIN_TABRIGHT].SetFilename("_tabr.gif");

	m_skinshapes[SKIN_TABSELECTEDLEFT].SetFilename("_tabsl.gif");
	m_skinshapes[SKIN_TABSELECTEDCENTER].SetFilename("_tabsc.gif");
	m_skinshapes[SKIN_TABSELECTEDRIGHT].SetFilename("_tabsr.gif");

	m_skinshapes[SKIN_SCROLLBARVERTTOP].SetFilename("_scrvt.gif");
	m_skinshapes[SKIN_SCROLLBARVERTCENTER].SetFilename("_scrvc.gif");
	m_skinshapes[SKIN_SCROLLBARVERTBOTTOM].SetFilename("_scrvb.gif");

	m_skinshapes[SKIN_SCROLLBARVERTSLIDERTOP].SetFilename("_scrvbt.gif");
	m_skinshapes[SKIN_SCROLLBARVERTSLIDERCENTER].SetFilename("_scrvbc.gif");
	m_skinshapes[SKIN_SCROLLBARVERTSLIDERLINE].SetFilename("_scrvbl.gif");
	m_skinshapes[SKIN_SCROLLBARVERTSLIDERBOTTOM].SetFilename("_scrvbb.gif");

	m_skinshapes[SKIN_SCROLLBARHORIZLEFT].SetFilename("_scrhl.gif");
	m_skinshapes[SKIN_SCROLLBARHORIZCENTER].SetFilename("_scrhc.gif");
	m_skinshapes[SKIN_SCROLLBARHORIZRIGHT].SetFilename("_scrhr.gif");

	m_skinshapes[SKIN_SCROLLBARHORIZSLIDERLEFT].SetFilename("_scrhbl.gif");
	m_skinshapes[SKIN_SCROLLBARHORIZSLIDERCENTER].SetFilename("_scrhbc.gif");
	m_skinshapes[SKIN_SCROLLBARHORIZSLIDERLINE].SetFilename("_scrhbf.gif");
	m_skinshapes[SKIN_SCROLLBARHORIZSLIDERRIGHT].SetFilename("_scrhbr.gif");

	m_skinshapes[SKIN_TABLEROWMARKER].SetFilename("_tabler.gif");
	m_skinshapes[SKIN_TABLEROWMARKERSELECTED].SetFilename("_tablers.gif");
	m_skinshapes[SKIN_TABLEROWNEW].SetFilename("_tabnew.gif");

	m_skinshapes[SKIN_COMBODOWNARROW].SetFilename("_cdown.gif");

	m_skinshapes[SKIN_RADIOUNSELECTED].SetFilename("_radu.gif");
	m_skinshapes[SKIN_RADIOSELECTED].SetFilename("_rads.gif");
}

/* return the window edge sizes as offsets */
void DefSkin::GetWindowEdges(kGUICorners *c)
{
	c->ty=m_skinshapes[SKIN_WINDOWTOPLEFT].GetImageHeight();
	c->lx=m_skinshapes[SKIN_WINDOWLEFTSIDE].GetImageWidth();
	c->rx=m_skinshapes[SKIN_WINDOWRIGHTSIDE].GetImageWidth();
	c->by=m_skinshapes[SKIN_WINDOWBOTTOMLEFT].GetImageHeight();
}

/* given the 4 corners for the window, return the corner positions */
/* for the three window buttons (close/full/minimize) */

void DefSkin::GetWindowButtonPositions(int allow,kGUICorners *c,kGUICorners *wclose,kGUICorners *wfull,kGUICorners *wminimize)
{
	int x,y;
	kGUIImage *tr;
	kGUIImage *tm;
	kGUIImage *wc;
	kGUIImage *wf;
	kGUIImage *wm;

	tr=GetShape(SKIN_WINDOWTOPRIGHT);
	tm=GetShape(SKIN_WINDOWTOPMIDDLE);

	wc=GetShape(SKIN_WINDOWCLOSEBUTTON);
	wf=GetShape(SKIN_WINDOWFULLBUTTON);
	wm=GetShape(SKIN_WINDOWMINIMIZEBUTTON);

	/* top right shape */
	y=c->ty+(tm->GetImageHeight()-wc->GetImageHeight())/2;
	x=c->rx-tr->GetImageWidth();

	if(allow&WINDOWBUTTON_CLOSE)
	{
		x-=wc->GetImageWidth()+3;

		/* position of the close button */
		wclose->lx=x;
		wclose->rx=x+wc->GetImageWidth();
		wclose->ty=y;
		wclose->by=y+wc->GetImageHeight();
	}
	else
	{
		wclose->lx=0;
		wclose->rx=0;
		wclose->ty=0;
		wclose->by=0;

	}

	/* position of the full button */
	if(allow&WINDOWBUTTON_FULL)
	{
		x-=wf->GetImageWidth()+3;
		wfull->lx=x;
		wfull->rx=x+wf->GetImageWidth();
		wfull->ty=y;
		wfull->by=y+wf->GetImageHeight();
	}
	else
	{
		wfull->lx=0;
		wfull->rx=0;
		wfull->ty=0;
		wfull->by=0;
	}

	/* position of the full button */
	if(allow&WINDOWBUTTON_MINIMIZE)
	{
		x-=wm->GetImageWidth()+3;
		wminimize->lx=x;
		wminimize->rx=x+wm->GetImageWidth();
		wminimize->ty=y;
		wminimize->by=y+wm->GetImageHeight();
	}
	else
	{
		wminimize->lx=0;
		wminimize->rx=0;
		wminimize->ty=0;
		wminimize->by=0;

	}
}

void DefSkin::GetMinimizedWindowSize(int *w,int *h)
{

	kGUIImage *tl;
	kGUIImage *bl;

	tl=GetShape(SKIN_WINDOWTOPLEFT);
	bl=GetShape(SKIN_WINDOWBOTTOMLEFT);
	w[0]=175;
	h[0]=tl->GetImageHeight()+bl->GetImageHeight();

//	w[0]=m_skinshapes[SKIN_MINIMIZEDWINDOW].GetImageWidth();
//	h[0]=m_skinshapes[SKIN_MINIMIZEDWINDOW].GetImageHeight();
}

void DefSkin::DrawWindowNoFrame(kGUIWindowObj *obj,kGUICorners *c)
{
	/* background */
	kGUI::DrawRect(c->lx,c->ty,c->rx,c->by,GetColor(SKINCOLOR_WINDOWBACKGROUND));
}

void DefSkin::DrawWindow(kGUIWindowObj *obj,kGUICorners *c,int allow,int over)
{
	int tx,x;

	kGUIImage *tl;
	kGUIImage *tm;
	kGUIImage *tr;
	kGUIImage *ls;
	kGUIImage *rs;
	kGUIImage *bl;
	kGUIImage *bm;
	kGUIImage *br;
	kGUIImage *wclose;
	kGUIImage *wfull;
	kGUIImage *wmin;
	kGUIImage *wicon;

	kGUICorners wcpos;
	kGUICorners wfpos;
	kGUICorners wmpos;

	tl=GetShape(SKIN_WINDOWTOPLEFT);
	tm=GetShape(SKIN_WINDOWTOPMIDDLE);
	tr=GetShape(SKIN_WINDOWTOPRIGHT);
	ls=GetShape(SKIN_WINDOWLEFTSIDE);
	rs=GetShape(SKIN_WINDOWRIGHTSIDE);
	bl=GetShape(SKIN_WINDOWBOTTOMLEFT);
	bm=GetShape(SKIN_WINDOWBOTTOMMIDDLE);
	br=GetShape(SKIN_WINDOWBOTTOMRIGHT);

	if(over==WINDOWBUTTON_CLOSE)
		wclose=GetShape(SKIN_WINDOWCLOSEBUTTONOVER);
	else
		wclose=GetShape(SKIN_WINDOWCLOSEBUTTON);

	if(over==WINDOWBUTTON_FULL)
		wfull=GetShape(SKIN_WINDOWFULLBUTTONOVER);
	else
		wfull=GetShape(SKIN_WINDOWFULLBUTTON);

	if(over==WINDOWBUTTON_MINIMIZE)
		wmin=GetShape(SKIN_WINDOWMINIMIZEBUTTONOVER);
	else
		wmin=GetShape(SKIN_WINDOWMINIMIZEBUTTON);

	/* top left shape */
	tl->Draw(0,c->lx,c->ty);

	/* draw the top header bar in between */
	tm->DrawLineRect(0,c->lx+tl->GetImageWidth(),c->ty,c->rx-tr->GetImageWidth(),c->ty+tl->GetImageHeight(),true);

	/* top right shape */
	x=c->rx-tr->GetImageWidth();
	tr->Draw(0,x,c->ty);

	/* draw close/full/minimize buttons */
	GetWindowButtonPositions(allow,c,&wcpos,&wfpos,&wmpos);
#if 1
	if(allow&WINDOWBUTTON_CLOSE)
		wclose->Draw(0,wcpos.lx,wcpos.ty);
	if(allow&WINDOWBUTTON_FULL)
		wfull->Draw(0,wfpos.lx,wfpos.ty);
	if(allow&WINDOWBUTTON_MINIMIZE)
		wmin->Draw(0,wmpos.lx,wmpos.ty);

#else
	yoff=(tm->GetImageHeight()-wclose->GetImageHeight())/2;
	x-=wclose->GetImageWidth()+3;
	wclose->Draw(x,c->ty+yoff);

	x-=wfull->GetImageWidth()+3;
	wfull->Draw(x,c->ty+yoff);

	x-=wmin->GetImageWidth()+3;
	wmin->Draw(x,c->ty+yoff);
#endif
	/* draw the left side */
	ls->DrawLineRect(0,c->lx,c->ty+tl->GetImageHeight(),c->lx+ls->GetImageWidth(),c->by-bl->GetImageHeight(),false);
	/* draw the right side */
	rs->DrawLineRect(0,c->rx-rs->GetImageWidth(),c->ty+tl->GetImageHeight(),c->rx,c->by-br->GetImageHeight(),false);

	/* bottom left shape */
	bl->Draw(0,c->lx,c->by-bl->GetImageHeight());

	/* bottom right shape */
	br->Draw(0,c->rx-br->GetImageWidth(),c->by-br->GetImageHeight());

	/* draw the bottom center edge */
	bm->DrawLineRect(0,c->lx+bl->GetImageWidth(),c->by-GetShape(SKIN_WINDOWBOTTOMMIDDLE)->GetImageHeight(),c->rx-br->GetImageWidth(),c->by,true);

	/* background */
	kGUI::DrawRect(c->lx+ls->GetImageWidth(),c->ty+tl->GetImageHeight(),c->rx-rs->GetImageWidth(),c->by-bm->GetImageHeight(),GetColor(SKINCOLOR_WINDOWBACKGROUND));

	wicon=GetShape(SKIN_WINDOWICON);
	tx=c->lx+tl->GetImageWidth();
	if(wicon->IsValid())
	{
		wicon->Draw(0,tx,c->ty+((tm->GetImageHeight()-wicon->GetImageHeight())/2));
		tx+=wicon->GetImageWidth()+3;
	}

	/* title */
	kGUI::PushClip();
	kGUI::ShrinkClip(tx,c->ty,x-3,c->ty+tl->GetImageHeight());
	obj->GetTitle()->Draw(tx,c->ty,0,tl->GetImageHeight());
	kGUI::PopClip();
}

#define BUSYBLOCKWIDTH 10
#define BUSYBLOCKGAP 2

void DefSkin::DrawBusy(kGUICorners *c)
{
	int x;
	kGUIImage *tm;

	tm=GetShape(SKIN_WINDOWTOPMIDDLE);

	for(x=c->lx;x<c->rx;x+=BUSYBLOCKWIDTH+BUSYBLOCKGAP)
	{
		/* draw the top header bar in between */
		tm->DrawLineRect(0,x,c->ty,min(x+BUSYBLOCKWIDTH,c->rx),c->by,true);
	}
}


#define TABBIGGER 3
#define TABH 18

void DefSkin::GetTabSize(int *expand,int *left,int *right,int *height)
{
	expand[0]=TABBIGGER;
	left[0]=GetShape(SKIN_TABLEFT)->GetImageWidth();
	right[0]=GetShape(SKIN_TABRIGHT)->GetImageWidth();
	height[0]=TABH;
}

void DefSkin::DrawTab(kGUIText *text,int x,int y,bool current,bool over)
{
	int textpixels;
	int bigger;
	kGUIImage *tl;
	kGUIImage *tm;
	kGUIImage *tr;

	tl=GetShape(SKIN_TABLEFT);
	tm=GetShape(SKIN_TABCENTER);
	tr=GetShape(SKIN_TABRIGHT);

	textpixels=text->GetWidth()+8;
	if(current==true)
	{
		tl->Draw(0,x-TABBIGGER,y);
		tm->DrawLineRect(0,x-TABBIGGER+tl->GetImageWidth(),y,x-TABBIGGER+textpixels+(TABBIGGER<<1),y+tm->GetImageHeight(),true);
		tr->Draw(0,x-TABBIGGER+textpixels+(TABBIGGER<<1),y);
		bigger=TABBIGGER;
	}
	else
		bigger=0;

	tl->Draw(0,x-bigger,y-bigger);
	tm->DrawLineRect(0,x-bigger+tl->GetImageWidth(),y-bigger,x-bigger+textpixels+(bigger<<1),y-bigger+tm->GetImageHeight(),true);
	tr->Draw(0,x-bigger+textpixels+(bigger<<1),y-bigger);

	if((current==true) || (over==true))
	{
		/* draw orange selected bar over tab */
		GetShape(SKIN_TABSELECTEDLEFT)->Draw(0,x-bigger,y-bigger);
		GetShape(SKIN_TABSELECTEDCENTER)->DrawLineRect(0,x-bigger+GetShape(SKIN_TABSELECTEDLEFT)->GetImageWidth(),y-bigger,x-bigger+textpixels+(bigger<<1),y-bigger+GetShape(SKIN_TABSELECTEDCENTER)->GetImageHeight(),true);
		GetShape(SKIN_TABSELECTEDRIGHT)->Draw(0,x-bigger+textpixels+(bigger<<1),y-bigger);
	}

	text->Draw(x+6,y+3,0,0);
}

int DefSkin::GetScrollbarWidth(void)
{
	return GetShape(SKIN_SCROLLBARVERTTOP)->GetImageWidth();
}

int DefSkin::GetScrollbarHeight(void)
{
	return GetShape(SKIN_SCROLLBARHORIZLEFT)->GetImageHeight();
}

void DefSkin::GetScrollbarSize(bool isvert,int *lt,int *br,int *wh)
{
	if(isvert==true)
	{
		lt[0]=GetShape(SKIN_SCROLLBARVERTTOP)->GetImageHeight();
		br[0]=GetShape(SKIN_SCROLLBARVERTBOTTOM)->GetImageHeight();
		wh[0]=	GetShape(SKIN_SCROLLBARVERTSLIDERTOP)->GetImageHeight()+
				GetShape(SKIN_SCROLLBARVERTSLIDERCENTER)->GetImageHeight()+
				GetShape(SKIN_SCROLLBARVERTSLIDERBOTTOM)->GetImageHeight();
	}
	else
	{
		lt[0]=GetShape(SKIN_SCROLLBARHORIZLEFT)->GetImageWidth();
		br[0]=GetShape(SKIN_SCROLLBARHORIZRIGHT)->GetImageWidth();
		wh[0]=	GetShape(SKIN_SCROLLBARHORIZSLIDERLEFT)->GetImageWidth()+
				GetShape(SKIN_SCROLLBARHORIZSLIDERCENTER)->GetImageWidth()+
				GetShape(SKIN_SCROLLBARHORIZSLIDERRIGHT)->GetImageWidth();
	}
}

void DefSkin::DrawScrollbar(bool isvert,kGUICorners *c,kGUICorners *sc,bool showends)
{
	int blen;

	if(isvert==true)		/* vertical scroll bar? */
	{
		int babove,bbelow;
		int vth,vbh;
		kGUIImage *vc=GetShape(SKIN_SCROLLBARVERTCENTER);

		kGUIImage *vst=GetShape(SKIN_SCROLLBARVERTSLIDERTOP);
		kGUIImage *vsc=GetShape(SKIN_SCROLLBARVERTSLIDERCENTER);
		kGUIImage *vsl=GetShape(SKIN_SCROLLBARVERTSLIDERLINE);
		kGUIImage *vsb=GetShape(SKIN_SCROLLBARVERTSLIDERBOTTOM);

		if(showends)
		{
			kGUIImage *vt=GetShape(SKIN_SCROLLBARVERTTOP);
			kGUIImage *vb=GetShape(SKIN_SCROLLBARVERTBOTTOM);

			/* vertical scrollbar top button */
			vt->Draw(0,c->lx,c->ty);
			vth=vt->GetImageHeight();

			/* vertical scrollbar bottom button */
			vb->Draw(0,c->lx,c->by-vb->GetImageHeight());
			vbh=vt->GetImageHeight();
		}
		else
		{
			vth=0;
			vbh=0;
		}
		/* draw the background inbetween the buttons */
		vc->DrawLineRect(0,c->lx,c->ty+vth,c->lx+vc->GetImageWidth(),c->by-vbh,false);

		blen=(sc->by-sc->ty)-(vst->GetImageHeight()+vsc->GetImageHeight()+vsb->GetImageHeight());
		babove=blen/2;
		bbelow=blen-babove;

		/* top of slider shape */
		vst->Draw(0,sc->lx,sc->ty);
		sc->ty+=vst->GetImageHeight();

		if(babove)	/* number of pixels between slider top and slider center shape */
		{
			/* repeating line above slider center shape */
			vsl->DrawLineRect(0,sc->lx,sc->ty,sc->rx,sc->ty+babove,false);
			sc->ty+=babove;
		}
		/* middle of slider shape */
		vsc->Draw(0,sc->lx,sc->ty);
		sc->ty+=vsc->GetImageHeight();

		if(bbelow)	/* number of pixels between slider center and slider bottom shape */
		{
			/* repeating line above slider center shape */
			vsl->DrawLineRect(0,sc->lx,sc->ty,sc->rx,sc->ty+bbelow,false);
			sc->ty+=babove;
		}
		/* bottom of slider shape */
		vsb->Draw(0,sc->lx,sc->ty);
	}
	else	/* horizontal */
	{
		int bleft,bright;
		int hlw,hrw;
		kGUIImage *hc=GetShape(SKIN_SCROLLBARHORIZCENTER);

		kGUIImage *hsl=GetShape(SKIN_SCROLLBARHORIZSLIDERLEFT);
		kGUIImage *hsc=GetShape(SKIN_SCROLLBARHORIZSLIDERCENTER);
		kGUIImage *hsf=GetShape(SKIN_SCROLLBARHORIZSLIDERLINE);
		kGUIImage *hsr=GetShape(SKIN_SCROLLBARHORIZSLIDERRIGHT);

		if(showends)
		{
			kGUIImage *hl=GetShape(SKIN_SCROLLBARHORIZLEFT);
			kGUIImage *hr=GetShape(SKIN_SCROLLBARHORIZRIGHT);

			/* top left shape */
			hl->Draw(0,c->lx,c->ty);
			hlw=hl->GetImageWidth();

			/* bottom right shape */
			hr->Draw(0,c->rx-hr->GetImageWidth(),c->ty);
			hrw=hr->GetImageWidth();
		}
		else
		{
			hlw=0;
			hrw=0;
		}
		/* draw the bar in between */
		hc->DrawLineRect(0,c->lx+hlw,c->ty,c->rx-hrw,c->ty+hc->GetImageHeight(),true);

		blen=(sc->rx-sc->lx)-(hsl->GetImageWidth()+hsc->GetImageWidth()+hsr->GetImageWidth());
		bleft=blen/2;
		bright=blen-bleft;

		/* left of slider shape */
		hsl->Draw(0,sc->lx,sc->ty);

		sc->lx+=hsl->GetImageWidth();
		if(bleft)
		{
			/* repeating line left of slider center shape */
			hsf->DrawLineRect(0,sc->lx,sc->ty,sc->lx+bleft,sc->by,true);
			sc->lx+=bleft;
		}
		/* middle of slider shape */
		hsc->Draw(0,sc->lx,sc->ty);
		sc->lx+=hsc->GetImageWidth();
		if(bright)
		{
			/* repeating line above slider center shape */
			hsf->DrawLineRect(0,sc->lx,sc->ty,sc->lx+bright,sc->by,true);
			sc->lx+=bright;
		}
		/* bottom of slider shape */
		hsr->Draw(0,sc->lx,sc->ty);
	}
}

#define TABLEROWEDGE 25
//#define TABLECOLEDGE 18

int DefSkin::GetTableRowHeaderWidth(void)
{
	return TABLEROWEDGE;
}

//int DefSkin::GetTableColHeaderHeight(void)
//{
//	return TABLECOLEDGE;
//}

void DefSkin::DrawTableRowHeader(kGUICorners *c,bool selected,bool cursor,bool add)
{
	if(add==true)
	{
		kGUI::DrawRectBevel(c->lx,c->ty,c->lx+TABLEROWEDGE,c->by,false);
		GetShape(SKIN_TABLEROWNEW)->Draw(0,c->lx+4,c->ty+3);
	}
	else if(selected==true)
	{
		kGUI::DrawRectBevelIn(c->lx,c->ty,c->lx+TABLEROWEDGE,c->by,DrawColor(0,0,0));
		if(cursor==true)
			GetShape(SKIN_TABLEROWMARKERSELECTED)->Draw(0,c->lx+4,c->ty+3);
	}
	else
	{
		kGUI::DrawRectBevel(c->lx,c->ty,c->lx+TABLEROWEDGE,c->by,false);
		if(cursor==true)
			GetShape(SKIN_TABLEROWMARKER)->Draw(0,c->lx+4,c->ty+3);
	}
}

int DefSkin::GetMenuRowHeaderWidth(void)
{
	return TABLEROWEDGE;
}

void DefSkin::DrawMenuRowHeader(kGUICorners *c)
{
	GetShape(SKIN_WINDOWTOPMIDDLE)->DrawLineRect(0,c->lx,c->ty,c->lx+TABLEROWEDGE,c->by,false);
}

void DefSkin::GetTickboxSize(int *w,int *h)
{
	w[0]=13;
	h[0]=13;
}

void DefSkin::DrawTickbox(kGUICorners *c,bool scale,bool selected,bool current)
{
	kGUIImage *tick=GetShape(SKIN_TICK);

	if(scale==false)
	{
		kGUI::DrawRectBevelIn(c->lx,c->ty,c->lx+13,c->ty+13);
		if(selected)
		{
			tick->SetScale(1.0f,1.0f);
			tick->Draw(0,c->lx+3,c->ty+3);
		}
	}
	else
	{
		kGUI::DrawRectBevelIn(c->lx,c->ty,c->rx,c->by);
		if(selected)
		{
			double sx,sy;

			sx=((c->rx-c->lx)-6)/(double)tick->GetImageWidth();
			sy=((c->by-c->ty)-6)/(double)tick->GetImageHeight();
			tick->SetScale(sx,sy);
			tick->Draw(0,c->lx+3,c->ty+3);
		}
	}

	if(current)
		kGUI::DrawCurrentFrame(c->lx+2,c->ty+2,c->rx-2,c->by-2);
}

void DefSkin::GetRadioSize(int *w,int *h)
{
	w[0]=13;
	h[0]=13;
}

void DefSkin::DrawRadio(kGUICorners *c,bool scale,bool selected,bool current)
{
	kGUIImage *shape=GetShape(selected==true?SKIN_RADIOSELECTED:SKIN_RADIOUNSELECTED);

	if(scale==false)
		shape->SetScale(1.0f,1.0f);
	else
	{
		double sx,sy;

		sx=((c->rx-c->lx)-1)/(double)shape->GetImageWidth();
		sy=((c->by-c->ty)-1)/(double)shape->GetImageHeight();
		shape->SetScale(sx,sy);
	}
	shape->Draw(0,c->lx,c->ty);

	if(current)
		kGUI::DrawCurrentFrame(c->lx,c->ty,c->rx-1,c->by-1);
}


int DefSkin::GetComboArrowWidth(void)
{
	return GetShape(SKIN_COMBODOWNARROW)->GetImageWidth();
}

void DefSkin::DrawComboArrow(kGUICorners *c)
{
	GetShape(SKIN_COMBODOWNARROW)->Draw(0,c->lx-3,c->ty+3);
}

int DefSkin::GetScrollHorizButtonWidths(void)
{
	return GetShape(SKIN_SCROLLBARHORIZLEFT)->GetImageWidth();
}

void DefSkin::DrawScrollHoriz(kGUICorners *c)
{
	kGUIImage *hl=GetShape(SKIN_SCROLLBARHORIZLEFT);
	kGUIImage *hc=GetShape(SKIN_SCROLLBARHORIZCENTER);
	kGUIImage *hr=GetShape(SKIN_SCROLLBARHORIZRIGHT);

	/* left shape */
	hl->Draw(0,c->lx,c->ty);

	/* right shape */
	hr->Draw(0,c->rx-hr->GetImageWidth(),c->ty);
	
	/* draw the bar in between */
	hc->DrawLineRect(0,c->lx+hl->GetImageWidth(),c->ty,c->rx-hr->GetImageWidth(),c->ty+hc->GetImageHeight(),true);
}

DefSkin::~DefSkin()
{
	int i;

	for(i=0;i<SKIN_NUMSHAPES;++i)
	{
		m_skinshapes[i].Purge();
	}
	/* remove bigfile from global list */
	DataHandle::RemoveBig(&m_bf);
}
