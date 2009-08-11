/*********************************************************************************/
/* kGUI - kguimcontrol.cpp                                                       */
/*                                                                               */
/* Initially Designed and Programmed by Kevin Pickell                            */
/*                                                                               */
/* http://code.google.com/p/kgui/				                                 */
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


/*********************************************************************************/
/*                                                                               */
/* Movie Controls, play, pause, rewing/ff scrollbar and loop tickbox             */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguimovie.h"

kGUIMovieControlObj::kGUIMovieControlObj()
{
	SetNumGroups(1);

	/* load button shapes */
	m_ipause.SetFilename("_pause.gif");
	m_iplay.SetFilename("_play.gif");

	m_playpause.SetSize(m_iplay.GetImageWidth(),m_iplay.GetImageHeight());
	m_playpause.SetImage(&m_iplay);
	m_playpause.SetEventHandler(this,CALLBACKNAME(PressPlayPause));
	m_playpause.SetFrame(false);	/* no frame since image takes up whole button */
	m_playpause.SetShowCurrent(false);	/* don't draw current box on it when it is the current button */

	m_slider.SetHorz();
	m_slider.SetFixedThumb(24);
	m_slider.SetShowEnds(false);
	m_slider.SetSize(10,kGUI::GetSkin()->GetScrollbarHeight());
	m_slider.SetEventHandler(this,CALLBACKNAME(Move));

	m_loop.SetHint("Loop Movie");
	m_loop.SetEventHandler(this,CALLBACKNAME(LoopChanged));

	SetSize(320,kGUI::GetSkin()->GetScrollbarHeight()+m_playpause.GetZoneH());
	AddObject(&m_playpause);
	AddObject(&m_slider);
	AddObject(&m_loop);
	m_movie=0;
	m_lasttime=0;
	m_lastduration=0;
	m_lastplaying=false;
	m_eventactive=false;
}

/* position the elements */
void kGUIMovieControlObj::Position(void)
{
	int x,w;
	int sh;

	sh=m_playpause.GetZoneH();
	m_playpause.SetPos(0,0);
	x=m_playpause.GetZoneW();
	w=GetZoneW()-m_loop.GetZoneW();
	m_loop.SetPos(w,0);

	m_slider.SetPos(x,0);
	m_slider.SetSize(w-x,sh);
}

void kGUIMovieControlObj::CalcChildZone(void)
{
	SetChildZone(0,0,GetZoneW(),GetZoneH());
	Position();
}

kGUIMovieControlObj::~kGUIMovieControlObj()
{
	if(m_eventactive)
		SetMovie(0);
}

void kGUIMovieControlObj::SetMovie(kGUIMovie *movie)
{
	bool wantevent;

	if(m_movie==movie)
		return;
	m_movie=movie;
	if(m_movie)
	{
		m_loop.SetSelected(m_movie->GetLoop());
		UpdateButton();
		wantevent=true;
	}
	else
		wantevent=false;

	if(wantevent==m_eventactive)
		return;
	m_eventactive=wantevent;
	if(!wantevent)
		kGUI::DelEvent(this,CALLBACKNAME(Event));
	else
		kGUI::AddEvent(this,CALLBACKNAME(Event));
}

void kGUIMovieControlObj::UpdateButton(void)
{
	bool playing;

	if(m_movie)
		playing=m_movie->GetPlaying();
	else
		playing=false;

	if(playing==m_lastplaying)
		return;

	m_lastplaying=playing;
	if(playing)
		m_playpause.SetImage(&m_ipause);
	else
		m_playpause.SetImage(&m_iplay);
}

void kGUIMovieControlObj::Event(void)
{
	int time,duration,bar;

	UpdateButton();
	time=m_movie->GetTime();
	duration=m_movie->GetDuration();
	if(time==m_lasttime && duration==m_lastduration)
		return;

	m_lasttime=time;
	m_lastduration=duration;
	bar=duration>>4;
	m_slider.SetValues(time,bar,((duration-1)-time));
	m_slider.Dirty();
}

/* returning true means I've used the input, false means pass input to someone else */

bool kGUIMovieControlObj::UpdateInput(void)
{
	bool used;

	used=UpdateInputC(0);
	if(m_slider.IsActive()==true && kGUI::GetActiveObj()!=&m_slider)
		kGUI::PushActiveObj(&m_slider);
	return(used);
}

void kGUIMovieControlObj::Draw(void)
{
	DrawC(0);
}

/* slider callback */
void kGUIMovieControlObj::Move(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		int time,duration;

		m_movie->SetPlaying(false);
		time=m_movie->GetTime();
		duration=m_movie->GetDuration();

		time+=event->m_value[0].i;
		if(time<0)
			time=0;
		else if(time>=duration)
			time=duration-1;

		m_movie->SetPlayAudio(false);
		m_movie->Seek(time);
	}
}

void kGUIMovieControlObj::PressPlayPause(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_PRESSED)
	{
		if(m_movie->GetPlaying()==false)
		{
			if(m_movie->GetDone()==true)
				m_movie->Seek(0);
			m_movie->SetPlayAudio(true);
			m_movie->SetPlaying(true);
		}
		else
			m_movie->SetPlaying(false);
		UpdateButton();
	}
}

void kGUIMovieControlObj::LoopChanged(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		if(m_movie)
			m_movie->SetLoop(m_loop.GetSelected());
	}
}

/*************************************************************************/

kGUIMovieObj::kGUIMovieObj()
{
	SetNumGroups(1);
	m_image.SetPos(0,0);
	AddObject(&m_image);
	AddObject(&m_moviecontrols);
	m_moviecontrols.SetMovie(this);
}

kGUIMovieObj::~kGUIMovieObj()
{
}

void kGUIMovieObj::CalcChildZone(void)
{
	int mch=m_moviecontrols.GetZoneH();

	SetChildZone(0,0,GetZoneW(),GetZoneH());

	m_image.SetPos(0,0);
	m_image.SetSize(GetZoneW(),GetZoneH()-mch);

	m_moviecontrols.SetPos(0,GetZoneH()-mch);
	m_moviecontrols.SetZoneW(GetZoneW());

	/* allocate output image area */
	m_image.SetMemImageCopy(0,GUISHAPE_SURFACE,m_image.GetZoneW(),m_image.GetZoneH(),4,0);
	SetOutputImage(&m_image,GetQuality());
}
