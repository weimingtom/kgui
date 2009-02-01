/**********************************************************************************/
/* kGUI - kguidatereq.cpp                                                             */
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
#include "_text.h"
#include "kguireq.h"

typedef struct
{
	int month;	/* 0 - 11 */
	int mday;	/* 1 - 31 or -1 if floating */
	int mweek;	/* if mday is floating, then week 0-4 */
	int wday;	/* if mday is floating then day of week 0-6 */
	const char *name;
	const char *picname;
}DAYLIST_DEF;

DAYLIST_DEF daylist[]={
{ 0,0,-1,-1,"Easter Sunday","easter"},			/* Calculated and filled in */
{ 0,0,-1,-1,"Good Friday (Canada)","goodf"},		/* Calculated and filled in */
{ 0,0,-1,-1,"Easter Monday (Canada)","easter"},		/* Calculated and filled in */
{ 0,0,-1,-1,"Chinese New Year","cnewyear"},		/* Calculated and filled in */
{ 1-1, 1,-1,-1,"New Years Day","newyear"},		/* Jan 1 */
{ 1-1,-1, 2, 1,"Martin Luther King Jr. Day (USA)","king"},	/* 3rd Monday in January */
{ 2-1,-1, 2, 1,"President's Day (USA)","presidents"},		/* 3rd Monday in February */
{ 2-1, 2,-1,-1,"Groundhog Day","groundhog"},		/* Feb 2 */
{ 2-1,14,-1,-1,"Valentine's Day","valentines"},		/* Feb 14 */
{ 2-1,22,-1,-1,"Washington's Birthday (USA)","washington"},	/* Feb 22*/
{ 3-1,17,-1,-1,"St Patrick's Day","stpat"},		/* March 17 */
{ 4-1, 1,-1,-1,"April Fools Day","aprilfools"},		/* Apr 1 */
{ 5-1, 5,-1,-1,"Cinco de Mayo","cinco"},		/* May 5th */
{ 5-1,-2, 0, 1,"Memorial Day (USA)","memorial"},	/* Last Monday in May */
{ 5-1,-2, 1, 1,"Victoria Day (Canada)","victoria"},	/* 2nd to last Monday in May */
{ 5-1,-1, 1, 0,"Mother's Day","mothers"},			/* 2nd Sunday in May */
{ 6-1,-1, 2, 0,"Fathers's Day","fathers"},		/* 3rd Sunday in June */
{ 7-1, 1,-1,-1,"Canada Day","canada"},			/* July 1st */
{ 7-1, 4,-1,-1,"Independence Day (USA)","usa"},		/* July 4th */
{ 9-1,-1, 0, 1,"Labour Day","labour"},			/* 1st Monday in September */
{10-1,31,-1,-1,"Halloween","halloween"},		/* Oct 31 */
{10-1,-1, 1, 1,"Thanksgiving (Canada) / Columbus Day (USA)","thanksgiving"}, /* 2nd Monday in October */
{11-1,-1, 3, 4,"Thanksgiving (USA)","thanksgiving"}, 	/* 4th Thursday in November */
{11-1,11,-1,-1,"Rememberance Day / Veteran's Day","rememberance"},	/* November 11th */
{12-1,24,-1,-1,"Christmas Eve","cheve"},			/* Dec 24 */
{12-1,25,-1,-1,"Christmas Day","tree"},			/* Dec 25 */
{12-1,26,-1,-1,"Boxing Day","boxday"},			/* Dec 26 */
{12-1,31,-1,-1,"New Years Eve","newyearseve"}};		/* Dec 31 */

typedef struct
{
	int month,day,year;
	const char *animal;
}DCONST_DEF;

DCONST_DEF chinesenewyears[]={
	{1,29,2006,"Dog"},
	{2,18,2007,"Pig"},
	{2,7,2008,"Rat"},
	{1,26,2009,"Ox"},
	{2,14,2010,"Tiger"},
	{2,3,2011,"Rabbit"},
	{1,23,2012,"Dragon"},
	{2,10,2013,"Snake"},
	{1,31,2014,"Horse"},
	{2,19,2015,"Goat"},
	{2,8,2016,"Monkey"},
	{1,28,2017,"Rooster"},
	{2,16,2018,"Dog"},
	{2,5,2019,"Pig"},
	{1,25,2020,"Rat"}};


/* called by set whenever the month changes */
void kGUIDateGridObj::GetHolidayString(kGUIDate *date,kGUIString *s)
{
	unsigned int i;
	int c,y,n,k,j,l,m,w;
	DAYLIST_DEF *dl;
	int istoday;	
	kGUIDate d;
	time_t ntime;
	struct tm *tinfo;
	struct tm ti;

	s->Clear();
	d.SetToday();
	if((date->GetDay()==d.GetDay()) && (date->GetMonth()==d.GetMonth()) && (date->GetYear()==d.GetYear()))
		s->SetString(kGUI::GetString(KGUISTRING_TODAY));	/* todo, add to button string list for translation */

	/* convert d,m,y to numeric value */
	memset(&ti,0,sizeof(ti));
	ti.tm_mday=date->GetDay();
	ti.tm_mon=date->GetMonth()-1;
	ti.tm_year=date->GetYear()-1900;
	ntime=mktime(&ti);

	tinfo=localtime( &ntime );	/* convert back to d/m/y */
	ti=*(tinfo);

	y=date->GetYear();
	c = y / 100;
	n = y - 19 * ( y / 19 );
	k = ( c - 17 ) / 25;
	i = c - c / 4 - ( c - k ) / 3 + 19 * n + 15;
	i = i - 30 * ( i / 30 );
	i = i - ( i / 28 ) * ( 1 - ( i / 28 ) * ( 29 / ( i + 1 ) ) * ( ( 21 - n ) / 11 ) );
   	j = y + y / 4 + i + 2 - c + c / 4;
   	j = j - 7 * ( j / 7 );
   	l = i - j;

   	m = (3 + ( l + 40 ) / 44);
   	daylist[0].month = m-1;		/* Easter Sunday */
   	daylist[0].mday=l + 28 - 31 * ( m / 4 );

   	d.SetMonth(m);
	d.SetYear(date->GetYear());
	d.SetDay(daylist[0].mday);

	/* good friday */
	d.AddDays(-2);
	daylist[1].month = d.GetMonth()-1;
	daylist[1].mday= d.GetDay();

	/* easter monday */
	d.AddDays(3);
	daylist[2].month = d.GetMonth()-1;
	daylist[2].mday= d.GetDay();

	/* calculate chinese new year for this year */

	for(i=0;i<sizeof(chinesenewyears)/sizeof(DCONST_DEF );++i)
	{
		if(chinesenewyears[i].year==y)
		{
			daylist[3].month=chinesenewyears[i].month-1;
			daylist[3].mday=chinesenewyears[i].day;
			m_ccuryear.Sprintf("Chinese New Year, Year of the %s",chinesenewyears[i].animal);
			daylist[3].name=m_ccuryear.GetString();
			break;
		}
	}

	dl=daylist;
	for(i=0;i<sizeof(daylist)/sizeof(DAYLIST_DEF);++i)
	{
		istoday=0;
		if(dl->month==ti.tm_mon)
		{
			if(dl->mday>=0)	/* fixed day not floating */
			{
				if(dl->mday==ti.tm_mday)
					istoday=1;
			}
			else if(dl->mday==-1)	/* is floating, weekday day of nth week */
			{
				if((dl->wday==ti.tm_wday) )
				{
					w=(ti.tm_mday-1)/7;
					if(dl->mweek==w)
						istoday=1;
				}
			}
			else /* is floating, weekday day of nth week ( from end of month back ) */
			{
				if((dl->wday==ti.tm_wday) && (dl->month==ti.tm_mon) )
				{
					w=(31-ti.tm_mday)/7;
					if(dl->mweek==w)
						istoday=1;
				}
			}
		}
		
		if(istoday)
		{
			if(s->GetLen())
				s->Append(", ");
			s->Append(dl->name);
		}
		++dl;
	}
}

void kGUIDateGridObj::Set(int vm,int vy,int d,int m,int y)
{
	kGUIDate dd;
	int c;
	int w;

	m_curday=d;
	m_curmonth=m;
	m_curyear=y;

	m_viewmonth=vm;

	dd.SetDay(1);
	dd.SetMonth(vm);
	dd.SetYear(vy);

	w=dd.GetDayofWeek();
	if(!w)
		w=7;
	dd.AddDays(-w);
	for(c=0;c<(6*7);++c)
	{
		m_day[c]=dd.GetDay();
		m_month[c]=dd.GetMonth();
		m_year[c]=dd.GetYear();
		GetHolidayString(&dd,&m_holidays[c]);
		if(m_validdaycallback.IsValid())
			m_validdaycallback.Call(&dd,&m_enable[c]);
		else
			m_enable[c]=true;

		dd.AddDays(1);
	}
	Dirty();
}

void kGUIDateGridObj::Draw(void)
{
	int r,y,i;
	int x1,x2,y1,y2;
	double px,py;
	kGUICorners cc;
	kGUIColor c;

	GetCorners(&cc);
	px=(cc.rx-cc.lx)/7.0f;
	py=(cc.by-cc.ty)/7.0f;

	m_text.SetHAlign(FT_CENTER);
	m_text.SetVAlign(FT_MIDDLE);
	m_text.SetColor(DrawColor(0,0,0));

	/* draw header */
	for(r=0;r<7;++r)
	{
		x1=cc.lx+(int)(px*r);
		y1=cc.ty;
		x2=cc.lx+(int)(px*(r+1));
		y2=cc.ty+(int)py;
		kGUI::DrawRectFrame(x1,y1,x2,y2,DrawColor(150,150,255),DrawColor(0,0,0));
		m_text.SetString(kGUIDate::GetWeekDay3(r));
		m_text.Draw(x1,y1,x2-x1,y2-y1);
	}

	/* draw days */
	i=0;
	for(y=0;y<6;++y)
	{
		y1=cc.ty+(int)((y+1)*py);
		y2=cc.ty+(int)((y+2)*py);
		for(r=0;r<7;++r)
		{
			x1=cc.lx+(int)(px*r);
			x2=cc.lx+(int)(px*(r+1));
			if((m_month[i]==m_curmonth) && (m_day[i]==m_curday) && (m_year[i]==m_curyear))
				c=DrawColor(192,150,150);
			else if(m_month[i]==m_viewmonth)
			{
				if(m_holidays[i].GetLen())
					c=DrawColor(220,255,220);
				else
					c=DrawColor(220,220,220);
			}
			else
				c=DrawColor(96,96,96);

			kGUI::DrawRectFrame(x1,y1,x2,y2,c,DrawColor(0,0,0));
			m_text.Sprintf("%d",m_day[i]);
			m_text.Draw(x1,y1,x2-x1,y2-y1);
			if(m_enable[i]==false)
			{
				kGUI::DrawLine(x1,y1,x2,y2,DrawColor(255,0,0));
				kGUI::DrawLine(x2,y1,x1,y2,DrawColor(255,0,0));
			}
			++i;
		}
	}
}

bool kGUIDateGridObj::UpdateInput(void)
{
	kGUICorners c;
	double px,py;
	int d;

	GetCorners(&c);
	if(kGUI::MouseOver(&c)==true)
	{
		px=(c.rx-c.lx)/7.0f;
		py=(c.by-c.ty)/7.0f;

		d=(int)((kGUI::GetMouseY()-c.ty)/py);
		if(d>0)
		{
			d=(d-1)*7;
			d+=(int)((kGUI::GetMouseX()-c.lx)/px);
			if(d>=0 && d<(6*7))
			{
				if(m_enable[d]==true)
				{
					if(kGUI::GetMouseDoubleClickLeft()==true || kGUI::GetMouseClickLeft()==true)
					{
						kGUIDate cd;
						bool click;

						click=kGUI::GetMouseDoubleClickLeft();

						cd.Set(m_day[d],m_month[d],m_year[d]);
						m_clickcallback.Call(&cd,&click);
						return(true);
					}
				}
				kGUI::SetHintString(kGUI::GetMouseX(),kGUI::GetMouseY()+20,m_holidays[d].GetString());
			}
		}
		return(true);
	}
	return(false);
}

/**************************************************************************/

/* todo: allow limits, like no past dates or no future dates etc */

kGUIDateReq::kGUIDateReq(kGUIDate *date,void *codeobj,void (*code)(void *,kGUIDate *date))
{
	int y;

	/* save pointer to user supplied date so it can be updated when done */

	/* get default date from user supplied date */
	m_date.SetDay(date->GetDay());
	m_date.SetMonth(date->GetMonth());
	m_date.SetYear(date->GetYear());

	m_selector.SetClickCallback(this,CALLBACKNAME(Clicked));
	m_donecallback.Set(codeobj,code);

	m_selector.SetPos(0,0);
	m_window.AddObject(&m_selector);

	y=m_selector.GetZoneY()+m_selector.GetZoneH()+10;
	m_cancel.SetString(kGUI::GetString(KGUISTRING_CANCEL));
	m_cancel.SetSize(60,20);
	m_cancel.SetPos(260,y);
	m_cancel.SetEventHandler(this,CALLBACKNAME(PressCancel));
	m_window.AddObject(&m_cancel);

	m_done.SetString(kGUI::GetString(KGUISTRING_OK));
	m_done.SetSize(60,20);
	m_done.SetPos(260+20+60,y);
	m_done.SetEventHandler(this,CALLBACKNAME(PressDone));
	m_window.AddObject(&m_done);

	m_window.SetEventHandler(this,CALLBACKNAME(WindowEvent));
	m_window.SetSize(100,100);
	m_window.ExpandToFit();
	m_window.Center();
	m_window.SetTop(true);
	kGUI::AddWindow(&m_window);
}

void kGUIDateReq::WindowEvent(kGUIEvent *event)
{
	switch(event->GetEvent())
	{
	case EVENT_CLOSE:
		kGUI::ReDraw();
		m_donecallback.Call(&m_date);
		delete this;
	}
}

void kGUIDateReq::Clicked(kGUIDate *date,bool *doubleclicked)
{
	m_date.Copy(date);
	m_date.LongDate(m_window.GetTitle());

	if(doubleclicked[0]==true)
		m_window.Close();
}

void kGUIDateReq::PressCancel(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		m_window.Close();
}

void kGUIDateReq::PressDone(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
		m_window.Close();
}

/******************************************************************************************/

kGUIDateSelectorObj::kGUIDateSelectorObj()
{
	kGUIDate today;

	SetSize(400,300);
	SetNumGroups(1);
	today.SetToday();

	/* get default date from user supplied date */
	m_date.SetDay(today.GetDay());
	m_date.SetMonth(today.GetMonth());
	m_date.SetYear(today.GetYear());

	m_viewmonth=today.GetMonth();
	m_viewyear=today.GetYear();

	m_vmonth.SetPos((GetZoneW()-130)>>1,10);
	m_vmonth.SetSize(130,20);
	m_vmonth.SetEventHandler(this,CALLBACKNAME(ChangeMonth));

	m_vyear.SetPos((GetZoneW()-130)>>1,40);
	m_vyear.SetSize(130,20);
	m_vyear.SetEventHandler(this,CALLBACKNAME(ChangeYear));

	AddObject(&m_vmonth);
	AddObject(&m_vyear);

	/* position day buttons */
	m_grid.SetPos(0,80);
	m_grid.SetSize(GetChildZoneW(),GetChildZoneH()-80);
	AddObject(&m_grid);
	m_grid.SetClickCallback(this,CALLBACKNAME(Clicked));
	DayChanged();
}

void kGUIDateSelectorObj::Clicked(kGUIDate *date,bool *doubleclicked)
{
	m_date.Copy(date);

	m_viewmonth=date->GetMonth();
	m_viewyear=date->GetYear();
	DayChanged();
	m_clickcallback.Call(date,doubleclicked);
}


void kGUIDateSelectorObj::ChangeMonth(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_viewmonth+=event->m_value[0].i;
		if(m_viewmonth==13)
		{
			m_viewmonth=1;
			++m_viewyear;
		}
		else if(m_viewmonth==0)
		{
			m_viewmonth=12;
			--m_viewyear;
		}
		DayChanged();
	}
}

void kGUIDateSelectorObj::ChangeYear(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		m_viewyear+=event->m_value[0].i;
		DayChanged();
	}
}

void kGUIDateSelectorObj::DayChanged(void)
{
	kGUIString title;

	m_grid.Set(m_viewmonth,m_viewyear,m_date.GetDay(),m_date.GetMonth(),m_date.GetYear());

	m_vmonth.SetString(kGUIDate::GetMonthName(m_viewmonth));
	m_vyear.Sprintf("%d",m_viewyear);

	m_date.LongDate(&title);
//	m_window.SetTitle(title.GetString());
}

