/*********************************************************************************/
/* kGUI - kguimovieplugin.cpp                                                    */
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

/*********************************************************************************/
/*                                                                               */
/* This is a plugin class to attach the movie player to the browser object.      */
/* This is to allow the browser to be used with or without the movie player      */
/* and all the extra code that then gets pulled in.                              */
/*                                                                               */
/*********************************************************************************/

#include "kgui.h"
#include "kguihtml.h"
#include "kguimovie.h"
#include "kguimovieplugin.h"

//todo: add right click event
//	movie->GetImageObj()->SetEventHandler(this,CALLBACKNAME(RightClickEvent));

kGUIHTMLMoviePluginObj::kGUIHTMLMoviePluginObj()
{
}

kGUIHTMLMoviePluginObj::~kGUIHTMLMoviePluginObj()
{
}

kGUIHTMLPluginObj *kGUIHTMLMoviePluginObj::New(void)
{
	kGUIHTMLPluginObj *po;

	po=new kGUIHTMLMoviePluginObj();
	return(po);
}

DataHandle *kGUIHTMLMoviePluginObj::GetDH(void)
{
	return &m_movieobj;
}

bool kGUIHTMLMoviePluginObj::Open(void)
{
	int w,h;

	m_movieobj.OpenMovie();

	/* is this a movie? */
	if(m_movieobj.GetIsValid()==false)
		return(false);

	/* yes it is, ok */
	w=m_movieobj.GetMovieWidth();
	h=m_movieobj.GetMovieHeight();
	m_movieobj.SetPos(0,0);
	m_movieobj.SetSize(w,h);

	SetNumGroups(1);
	AddObject(&m_movieobj);
	SetPos(0,0);
	SetSize(w,h);
	m_movieobj.SetPlaying(true);
	return(true);
}

/* object used to attach event handle to */
kGUIObj *kGUIHTMLMoviePluginObj::GetObj(void)
{
	return m_movieobj.GetImageObj();
}
