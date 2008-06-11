/**********************************************************************************/
/* kGUI - kguifont2.cpp                                                           */
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

/*! @file kguifont2.cpp 
    @brief This contains the text rendering function for rotated / anti-aliased 
	text drawing. */

#include "kgui.h"
#include "kguifont.h"

/* draw rotated font */

void kGUIText::DrawSectionRot(int sstart,int slen,double x,double y,double angle,kGUIColor color,double alpha)
{
	kGUIFace *face=kGUIFont::GetFace(GetFontID());
	int glyph_index;
	int font_height,font_above,font_below;
	FT_Face ftface;
	int size;
	FT_Glyph   glyph2;
	FT_Matrix  matrix;
	FT_BitmapGlyph  bit;
	double dx,dy,adv;
	double advsin,advcos;
	double fax,fay;	/* rotated font above */
	unsigned int ch;	/* current character */
	unsigned int nb;	/* number of bytes for current character */

	ftface=face->GetFace();
	size=GetFontSize();
	if(!size)
		return;
	assert(size>0,"Cannot print size 0\n");

	font_height=face->GetPixHeight(size);
	font_above = face->GetPixAscHeight(size);
	font_below = face->GetPixDescHeight(size);
	kGUI::SelectFont(face,size);

	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );

	adv=0;
	advsin=sin(angle);
	advcos=cos(angle);
	fax=sin(angle-(2*PI)*0.025f)*font_above;
	fay=-(cos(angle-(2*PI)*0.025f)*font_above);

	/* todo, handle underline */

	while(slen>0)
	{
		ch=GetChar(sstart,&nb);
		if(!ch)
			return;
		sstart+=nb;
		slen-=nb;

		/* todo, handle tabs, handle encoding  */
		glyph_index = FT_Get_Char_Index( ftface, ch );
		if(glyph_index>0)
		{
			FT_Load_Glyph(ftface,glyph_index,FT_LOAD_DEFAULT);
			FT_Get_Glyph( ftface->glyph, &glyph2 );
			FT_Glyph_Transform( glyph2, &matrix, 0 );
			FT_Glyph_To_Bitmap( &glyph2, ft_render_mode_normal,0, 1);
			
			/* draw to screen using writepixel */
			bit = (FT_BitmapGlyph)glyph2;

			dx=x+((advcos*adv)+fax);
			dy=y-((advsin*adv)+fay);
			DrawChar( (char *)bit->bitmap.buffer,
						dx + bit->left,
						dy -bit->top,
						(double)bit->bitmap.width, (double)bit->bitmap.rows,
						color,alpha);

			adv+=ftface->glyph->advance.x >> 6;
			adv+=m_letterspacing;
			FT_Done_Glyph(glyph2);
		}
	}
	if(m_underline)
		kGUI::DrawLine(x+fax,y-fay,x+((advcos*adv)+fax),y-((advsin*adv)+fay),color,alpha);
}

void kGUIText::DrawChar( char * src, double x,double y,double w,double h,kGUIColor color,double alpha)
{
	double cx,cy;
	double lx,rx,ty,by;
	unsigned char s;
	double weight;

	if(w<=0.0f || h<=0.0f)
		return;

	if(kGUI::OffClip((int)x,(int)y,(int)(x+w),(int)(y+h))==true)
		return;

	lx=x;
	ty=y;
	rx=lx+w;
	by=ty+h;

	kGUI::m_subpixcollector.SetBounds(ty,by);
	kGUI::m_subpixcollector.SetColor(color,alpha);

	for(cy=ty;cy<by;cy+=1.0f)
	{
		for(cx=lx;cx<rx;cx+=1.0f)
		{
			s=*(src++);
			if(s)
			{
				weight=(double)s/255.0f;
				kGUI::m_subpixcollector.AddRect(cx,cy,weight,weight);
			}
		}
	}
	kGUI::m_subpixcollector.Draw();
}