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

#include "kgui.h"
/* this is used for the loadconfig / saveconfig for saving users table layout */
#include "kguixml.h"


/* load user settings for this table from the XML object supplied */

void kGUITableObj::LoadConfig(kGUIXMLItem *root)
{
	unsigned int i;
	kGUIXMLItem *col;
	int num,xcol,width;
	bool found;
	const char *from;

	for(i=0;i<root->GetNumChildren();++i)
	{
		col=root->GetChild(i);
		num=col->Locate("num")->GetValueInt();
		from=col->Locate("from")->GetValueString();		

		assert(num<GetNumCols(),"Column number is too large!");

		/* find original column with this name */
		found=false;
		for(xcol=0;xcol<GetNumCols();++xcol)
		{
			if(!strcmp(GetColTitle(xcol),from))
			{
				found=true;
				break;
			}
		}

		if(found)
		{
			width=col->Locate("width")->GetValueInt();
			SetColWidth(num,width);
			if(col->Locate("hidden"))
				SetColShow(num,false);
			SetColOrder(num,xcol);
		}
	}
}

/* save settings for this table using the name supplied to the XML root object */
void kGUITableObj::SaveConfig(kGUIXMLItem *root,const char *name)
{
	int i;
	int xcol;
	kGUIXMLItem *table=root->AddChild(name);
	kGUIXMLItem *col;

	for(i=0;i<GetNumCols();++i)
	{
		xcol=GetColOrder(i);					/* colum number in this slot */
		col=table->AddChild("col");
		col->AddParm("num",(int)i);
		col->AddParm("from",GetColTitle(xcol));
		col->AddParm("width",GetColWidth(i));
		if(GetColShow(i)==false)
			col->AddParm("hidden","1");
	}
}
