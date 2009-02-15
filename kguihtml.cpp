/**********************************************************************************/
/* kGUI - kguihtml.cpp                                                            */
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

/*! @file kguihtml.cpp 
    @brief This is my attempt at a HTML renderer, yes it is all in one source-file. :-)   
    The parsing engine is very nice, and fast, if I do say so myself, the layout part on the  
    other hand needs a fair bit of re-work to get it compliant. */

/*! @todo Some webpages refer to gzipped css file so we need to ungzip it before processing */
/*! @todo Optimization: In the position pass there should be NO calls to FindAttrib, these should only be done on the minmax pass */
/*! @todo Need to add handling of media queries http://www.w3.org/TR/css3-mediaqueries/ */


#include "kgui.h"
#include "kguixml.h"
#include "kguihtml.h"
#include "kguireport.h"

#define MAXDOWNLOADS 10

#define BLINKDELAY ((int)(TICKSPERSEC*0.66f))

bool g_trace=false;
static double xfontsize[7]={9.0f,11.0f,13.0f,16.0f,19.0f,23.0f,28.0f};

TAGLIST_DEF kGUIHTMLPageObj::m_roottag=		{false,false,false,"root",			HTMLTAG_ROOT		,DISPLAY_BLOCK};
TAGLIST_DEF kGUIHTMLPageObj::m_unknowntag=		{false,false,true,"unknown",			HTMLTAG_UNKNOWN		,DISPLAY_INLINE};
TAGLIST_DEF kGUIHTMLPageObj::m_texttag=			{false,false,false,"text",			HTMLTAG_IMBEDTEXT	,DISPLAY_INLINE};
TAGLIST_DEF kGUIHTMLPageObj::m_singleobjtag=	{false,false,false,"xxx",			HTMLTAG_SINGLEOBJ	,DISPLAY_INLINE};
TAGLIST_DEF kGUIHTMLPageObj::m_textgrouptag=	{false,false,false,"textgroup",			HTMLTAG_IMBEDTEXTGROUP	,DISPLAY_ANONYMOUS};
TAGLIST_DEF kGUIHTMLPageObj::m_imgtag=		{false,false,false,"img",			HTMLTAG_LIIMG		,DISPLAY_INLINE};
TAGLIST_DEF kGUIHTMLPageObj::m_contenttag=		{false,false,false,":content",			HTMLTAG_CONTENTGROUP		,DISPLAY_INLINE};

TAGLIST_DEF kGUIHTMLPageObj::m_lishapetag=	{false,false,false,"lishape",	HTMLTAG_LISHAPE		,DISPLAY_MARKER};
TAGLIST_DEF kGUIHTMLPageObj::m_litexttag=	{false,false,false,"text",		HTMLTAG_IMBEDTEXT	,DISPLAY_MARKER};

/* since we reference this table using the tagindex this table MUST be in order */
/* of the HTMLTAG_ enum list, we do have an assert in the init code that double checks */

TAGLIST_DEF kGUIHTMLPageObj::m_taglist[]={
	{false,false,true,"a",			HTMLTAG_A			,DISPLAY_INLINE},
	{false,false,false,"abbr",		HTMLTAG_ABBR		,DISPLAY_INLINE},
	{false,false,false,"acronym",		HTMLTAG_ACRONYM		,DISPLAY_INLINE},
	{false,false,false,"address",		HTMLTAG_ADDRESS		,DISPLAY_BLOCK},
	{false,false,false,"applet",		HTMLTAG_APPLET		,DISPLAY_BLOCK},
	{true,false,false,"area",			HTMLTAG_AREA		,DISPLAY_INLINE},
	{false,false,false,"b",			HTMLTAG_B			,DISPLAY_INLINE},
	{true,false,false,"base",			HTMLTAG_BASE		,DISPLAY_INLINE},
	{false,false,false,"bdo",			HTMLTAG_BDO			,DISPLAY_INLINE},
	{false,false,false,"big",			HTMLTAG_BIG			,DISPLAY_INLINE},
	{false,false,false,"blink",		HTMLTAG_BLINK		,DISPLAY_INLINE},	/* not standard */
	{false,false,false,"blockquote",	HTMLTAG_BLOCKQUOTE	,DISPLAY_BLOCK},
	{false,true,false,"body",		HTMLTAG_BODY		,DISPLAY_BLOCK},
	{true,false,true,"br",			HTMLTAG_BR			,DISPLAY_INLINE},
	{false,false,false,"button",		HTMLTAG_BUTTON		,DISPLAY_INLINE_BLOCK},
	{false,false,false,"caption",		HTMLTAG_CAPTION		,DISPLAY_INLINE},
	{false,false,false,"center",		HTMLTAG_CENTER		,DISPLAY_BLOCK},
	{false,false,false,"cite",		HTMLTAG_CITE		,DISPLAY_INLINE},
	{false,false,false,"code",		HTMLTAG_CODE		,DISPLAY_INLINE},
	{true ,false,false,"col",			HTMLTAG_COL			,DISPLAY_INLINE},
	{false,true,false ,"colgroup",	HTMLTAG_COLGROUP	,DISPLAY_INLINE},
	{false,true,false ,"dd",			HTMLTAG_DD			,DISPLAY_BLOCK},
	{false,false,false,"del",			HTMLTAG_DEL			,DISPLAY_INLINE},
	{false,false,false,"dfn",			HTMLTAG_DFN			,DISPLAY_INLINE},
	{false,false,false,"div",			HTMLTAG_DIV			,DISPLAY_BLOCK},
	{false,true,false ,"dl",			HTMLTAG_DL			,DISPLAY_BLOCK},
	{true,false,true,"!doctype",		HTMLTAG_DOCTYPE		,DISPLAY_NONE},
	{false,true,false,"dt",			HTMLTAG_DT			,DISPLAY_BLOCK},
	{false,false,false,"em",			HTMLTAG_EM			,DISPLAY_INLINE},
	{false,false,false,"fieldset",	HTMLTAG_FIELDSET	,DISPLAY_INLINE},
	{false,false,false,"font",		HTMLTAG_FONT		,DISPLAY_INLINE},
	{false,false,false,"form",		HTMLTAG_FORM		,DISPLAY_INLINE},
	{true,false,false,"frame",		HTMLTAG_FRAME		,DISPLAY_BLOCK},
	{false,false,false,"frameset",	HTMLTAG_FRAMESET	,DISPLAY_BLOCK},
	{false,false,false,"h1",			HTMLTAG_H1			,DISPLAY_BLOCK},
	{false,false,false,"h2",			HTMLTAG_H2			,DISPLAY_BLOCK},
	{false,false,false,"h3",			HTMLTAG_H3			,DISPLAY_BLOCK},
	{false,false,false,"h4",			HTMLTAG_H4			,DISPLAY_BLOCK},
	{false,false,false,"h5",			HTMLTAG_H5			,DISPLAY_BLOCK},
	{false,false,false,"h6",			HTMLTAG_H6			,DISPLAY_BLOCK},
	{false,true,false,"head",		HTMLTAG_HEAD		,DISPLAY_INLINE},
	{true ,false,true,"hr",			HTMLTAG_HR			,DISPLAY_BLOCK},
	{false,true,false,"html",		HTMLTAG_HTML		,DISPLAY_BLOCK},
	{false,false,false,"i",			HTMLTAG_I			,DISPLAY_INLINE},
	{false,false,true,"iframe",		HTMLTAG_IFRAME		,DISPLAY_BLOCK},
	{true,false,true,"img",			HTMLTAG_IMG			,DISPLAY_INLINE_BLOCK},
	{true,false,true,"input",		HTMLTAG_INPUT		,DISPLAY_INLINE},
	{false,false,false,"ins",			HTMLTAG_INS			,DISPLAY_INLINE},
	{false,false,false,"kbd",			HTMLTAG_KBD			,DISPLAY_BLOCK},
	{false,false,false,"label",		HTMLTAG_LABEL		,DISPLAY_INLINE},
	{false,false,false,"legend",		HTMLTAG_LEGEND		,DISPLAY_INLINE},
	{false,true,false ,"li",			HTMLTAG_LI			,DISPLAY_LIST_ITEM},
	{true,false,true,"link",			HTMLTAG_LINK		,DISPLAY_INLINE},
	{false,false,false,"map",			HTMLTAG_MAP			,DISPLAY_INLINE},
	{true ,false,true,"meta",			HTMLTAG_META		,DISPLAY_INLINE},
	{false,false,false,"nobr",		HTMLTAG_NOBR		,DISPLAY_INLINE},	/* not standard */
	{false,false,false,"noframes",	HTMLTAG_NOFRAMES	,DISPLAY_BLOCK},
	{false,false,false,"noscript",	HTMLTAG_NOSCRIPT	,DISPLAY_BLOCK},
	{false,false,false,"object",		HTMLTAG_OBJECT		,DISPLAY_BLOCK},
	{false,true,false,"ol",			HTMLTAG_OL			,DISPLAY_BLOCK},
	{false,false,false,"optgroup",	HTMLTAG_OPTGROUP	,DISPLAY_INLINE},
	{false,true,false,"option",		HTMLTAG_OPTION		,DISPLAY_INLINE},
	{false,true,false,"p",			HTMLTAG_P			,DISPLAY_BLOCK},
	{false,false,false,"param",		HTMLTAG_PARAM		,DISPLAY_INLINE},
	{false,false,false,"pre",			HTMLTAG_PRE			,DISPLAY_INLINE},
	{false,false,false,"q",			HTMLTAG_Q			,DISPLAY_INLINE},
	{false,false,false,"s",			HTMLTAG_S			,DISPLAY_INLINE},	
	{false,false,false,"samp",		HTMLTAG_SAMP		,DISPLAY_BLOCK},
	{false,false,false,"script",		HTMLTAG_SCRIPT		,DISPLAY_INLINE},
	{false,false,false,"select",		HTMLTAG_SELECT		,DISPLAY_INLINE},
	{false,false,false,"small",		HTMLTAG_SMALL		,DISPLAY_INLINE},
	{false,false,false,"span",		HTMLTAG_SPAN		,DISPLAY_INLINE},
	{false,false,false,"strike",		HTMLTAG_STRIKE		,DISPLAY_INLINE},
	{false,false,false,"strong",		HTMLTAG_STRONG		,DISPLAY_INLINE},
	{false,false,false,"style",		HTMLTAG_STYLE		,DISPLAY_INLINE},
	{false,false,false,"sub",			HTMLTAG_SUB			,DISPLAY_INLINE},
	{false,false,false,"sup",			HTMLTAG_SUP			,DISPLAY_INLINE},
	{false,false,false,"table",		HTMLTAG_TABLE		,DISPLAY_TABLE},
	{false,true,false,"tbody",		HTMLTAG_TBODY		,DISPLAY_BLOCK},
	{false,true,false,"thead",		HTMLTAG_THEAD		,DISPLAY_BLOCK},
	{false,true,false,"tfoot",		HTMLTAG_TFOOT		,DISPLAY_BLOCK},
	{false,true,false,"td",			HTMLTAG_TD			,DISPLAY_TABLE_CELL},
	{false,false,false,"textarea",	HTMLTAG_TEXTAREA	,DISPLAY_INLINE},
	{false,true,false,"th",			HTMLTAG_TH			,DISPLAY_TABLE_CELL},
	{false,false,false,"title",		HTMLTAG_TITLE		,DISPLAY_NONE},
	{false,true,false,"tr",			HTMLTAG_TR			,DISPLAY_TABLE_ROW},
	{false,false,false,"tt",			HTMLTAG_TT		,DISPLAY_BLOCK},
	{false,false,false,"u",			HTMLTAG_U			,DISPLAY_INLINE},
	{false,true,false,"ul",			HTMLTAG_UL			,DISPLAY_BLOCK},
	{false,false,false,"var",			HTMLTAG_VAR			,DISPLAY_INLINE},
	{false,false,false,"wbr",			HTMLTAG_WBR			,DISPLAY_INLINE},	/* not standard */
	{true,false,false,"?xml",			HTMLTAG_XML			,DISPLAY_NONE},
	{false,false,true,"?xml-stylesheet",		HTMLTAG_XMLSTYLESHEET	,DISPLAY_INLINE}};

/* if the child is attempted to be put inside the parent then close the parent first */
/* if the child is listed as optional close object then don't print out a warning */

/* parent tag first, child tag next */	
POPLIST_DEF kGUIHTMLPageObj::m_poplist[]={
	{HTMLTAG_P,HTMLTAG_P},
	{HTMLTAG_P,HTMLTAG_PRE},
	{HTMLTAG_P,HTMLTAG_OL},
	{HTMLTAG_P,HTMLTAG_UL},
	{HTMLTAG_P,HTMLTAG_LI},
	{HTMLTAG_P,HTMLTAG_FORM},
	{HTMLTAG_P,HTMLTAG_SELECT},
	{HTMLTAG_P,HTMLTAG_DIV},
	//{HTMLTAG_P,HTMLTAG_INPUT},
	{HTMLTAG_P,HTMLTAG_H1},
	{HTMLTAG_P,HTMLTAG_H2},
	{HTMLTAG_P,HTMLTAG_H3},
	{HTMLTAG_P,HTMLTAG_H4},
	{HTMLTAG_P,HTMLTAG_H5},
	{HTMLTAG_P,HTMLTAG_H6},
	{HTMLTAG_P,HTMLTAG_DT},
	{HTMLTAG_P,HTMLTAG_DD},
	{HTMLTAG_P,HTMLTAG_DL},

	{HTMLTAG_OPTION,HTMLTAG_OPTION},
	{HTMLTAG_LI,HTMLTAG_LI},

	{HTMLTAG_DT,HTMLTAG_DD},
	{HTMLTAG_DD,HTMLTAG_DT},
	{HTMLTAG_DD,HTMLTAG_DL},
	{HTMLTAG_DT,HTMLTAG_DL},

	{HTMLTAG_TH,HTMLTAG_TD},
	{HTMLTAG_TH,HTMLTAG_TH},
	{HTMLTAG_TH,HTMLTAG_TR},

	{HTMLTAG_TR,HTMLTAG_TR},
	{HTMLTAG_TR,HTMLTAG_TBODY},
	{HTMLTAG_TD,HTMLTAG_TBODY},
	{HTMLTAG_TH,HTMLTAG_TBODY},
	{HTMLTAG_THEAD,HTMLTAG_TR},

	{HTMLTAG_TD,HTMLTAG_TD},
	{HTMLTAG_TD,HTMLTAG_TH},
	{HTMLTAG_TD,HTMLTAG_TR}};


bool kGUIHTMLPageObj::m_hashinit=false;
Hash kGUIHTMLPageObj::m_taghash;		/* hash list for tags */
Hash kGUIHTMLPageObj::m_atthash;		/* hash list for attributes */
Hash kGUIHTMLPageObj::m_consthash;	/* hash list for constants */
Hash kGUIHTMLPageObj::m_pophash;	/* hash list pop object */
Hash kGUIHTMLPageObj::m_colorhash;	/* hash list for colors */

enum
{
FLOAT_NONE,
FLOAT_LEFT,
FLOAT_RIGHT};

enum
{
POSITION_ABSOLUTE,
POSITION_FIXED,
POSITION_RELATIVE,
POSITION_STATIC,
POSITION_INHERIT};

#define CLEAR_NONE 0
#define CLEAR_LEFT 1
#define CLEAR_RIGHT 2
#define CLEAR_ALL (CLEAR_LEFT|CLEAR_RIGHT)

enum
{
CSSSELECTOR_DESCENDANT,
CSSSELECTOR_CHILD,
CSSSELECTOR_SIBLING,
CSSSELECTOR_ADJACENT,
CSSSELECTOR_UNIVERSAL,
CSSSELECTOR_FIRSTCHILD,
CSSSELECTOR_LASTCHILD,
CSSSELECTOR_NTHCHILD,
CSSSELECTOR_NTHLASTCHILD,
CSSSELECTOR_NTHOFTYPE,
CSSSELECTOR_NTHLASTOFTYPE,
CSSSELECTOR_FIRSTOFTYPE,
CSSSELECTOR_ONLYOFTYPE,
CSSSELECTOR_LASTOFTYPE,
CSSSELECTOR_ONLYCHILD,
CSSSELECTOR_FIRSTLINE,
CSSSELECTOR_FIRSTLETTER,
CSSSELECTOR_EMPTY,

CSSSELECTOR_TAG,
CSSSELECTOR_CLASS,
CSSSELECTOR_ID,

CSSSELECTOR_ATTEXISTS,
CSSSELECTOR_ATTVALUE,
CSSSELECTOR_ATTLIST,
CSSSELECTOR_ATTHYPHENLIST,
CSSSELECTOR_ATTBEGINVALUE,
CSSSELECTOR_ATTCONTAINSVALUE,
CSSSELECTOR_ATTENDVALUE,

CSSSELECTOR_LANG,
CSSSELECTOR_LINK,
CSSSELECTOR_VISITED,
CSSSELECTOR_ACTIVE,
CSSSELECTOR_HOVER,
CSSSELECTOR_FOCUS,
CSSSELECTOR_ROOT,
CSSSELECTOR_BEFORE,
CSSSELECTOR_AFTER,
CSSSELECTOR_TARGET,

CSSSELECTOR_ENABLED,
CSSSELECTOR_DISABLED,
CSSSELECTOR_CHECKED};

kGUIString kGUIHTMLPageObj::m_attstrings[HTMLATT_NUM];

ATTLIST_DEF kGUIHTMLPageObj::m_attlist[]={
	{"accesskey",			HTMLATT_ACCESSKEY		},
	{"action",				HTMLATT_ACTION			},
	{"align",				HTMLATT_TEXT_ALIGN		},
	{"alink",				HTMLATT_ALINK			},
	{"alt",					HTMLATT_ALT				},
	{"background",			HTMLATT_BACKGROUND		},
	{"background-image",	HTMLATT_BACKGROUND_IMAGE		},
	{"background-attachment",HTMLATT_BACKGROUND_ATTACHMENT	},
	{"background-repeat",	HTMLATT_BACKGROUND_REPEAT	},
	{"background-position",	HTMLATTGROUP_BACKGROUND_POSITION	},
	{"background-color",	HTMLATT_BACKGROUND_COLOR	},
	{"bgcolor",				HTMLATT_BACKGROUND_COLOR	},
	{"border",				HTMLATTGROUP_BORDER			},
	{"border-color",		HTMLATTGROUP_BORDER_COLOR	},
	{"border-style",		HTMLATTGROUP_BORDER_STYLE	},
	{"border-width",		HTMLATTGROUP_BORDER_WIDTH	},
	{"border-bottom",		HTMLATTGROUP_BORDER_BOTTOM	},
	{"border-left",			HTMLATTGROUP_BORDER_LEFT	},
	{"border-right",		HTMLATTGROUP_BORDER_RIGHT	},
	{"border-top",			HTMLATTGROUP_BORDER_TOP		},

	{"border-color-bottom",	HTMLATT_BORDER_COLOR_BOTTOM	},
	{"border-bottom-color",	HTMLATT_BORDER_COLOR_BOTTOM	},
	{"border-color-left",	HTMLATT_BORDER_COLOR_LEFT	},
	{"border-left-color",	HTMLATT_BORDER_COLOR_LEFT	},

	{"border-color-right",	HTMLATT_BORDER_COLOR_RIGHT	},
	{"border-right-color",	HTMLATT_BORDER_COLOR_RIGHT	},
	{"border-color-top",	HTMLATT_BORDER_COLOR_TOP	},
	{"border-top-color",	HTMLATT_BORDER_COLOR_TOP	},

	{"border-style-bottom",	HTMLATT_BORDER_STYLE_BOTTOM	},
	{"border-style-left",	HTMLATT_BORDER_STYLE_LEFT	},
	{"border-style-right",	HTMLATT_BORDER_STYLE_RIGHT	},
	{"border-style-top",	HTMLATT_BORDER_STYLE_TOP	},

	{"border-bottom-style",	HTMLATT_BORDER_STYLE_BOTTOM	},
	{"border-left-style",	HTMLATT_BORDER_STYLE_LEFT	},
	{"border-right-style",	HTMLATT_BORDER_STYLE_RIGHT	},
	{"border-top-style",	HTMLATT_BORDER_STYLE_TOP	},

	{"border-width-bottom",	HTMLATT_BORDER_WIDTH_BOTTOM	},
	{"border-width-left",	HTMLATT_BORDER_WIDTH_LEFT	},
	{"border-width-right",	HTMLATT_BORDER_WIDTH_RIGHT	},
	{"border-width-top",	HTMLATT_BORDER_WIDTH_TOP	},

	{"border-bottom-width",	HTMLATT_BORDER_WIDTH_BOTTOM	},
	{"border-left-width",	HTMLATT_BORDER_WIDTH_LEFT	},
	{"border-right-width",	HTMLATT_BORDER_WIDTH_RIGHT	},
	{"border-top-width",	HTMLATT_BORDER_WIDTH_TOP	},

	{"border-spacing",	HTMLATTGROUP_BORDER_SPACING		},
	{"border-collapse",	HTMLATT_BORDER_COLLAPSE	},
	{"bottom",			HTMLATT_BOTTOM			},
	{"bordercolor",		HTMLATT_BORDERCOLOR		},
	{"cellpadding",		HTMLATT_CELLPADDING		},
	{"cellspacing",		HTMLATT_CELLSPACING		},
	{"checked",			HTMLATT_CHECKED			},
	{"cite",			HTMLATT_CITE		},
	{"class",			HTMLATT_CLASS			},
	{"clear",			HTMLATT_CLEAR			},
	{"color",			HTMLATT_COLOR			},
	{"text",			HTMLATT_COLOR			},
	{"cols",			HTMLATT_COLS			},
	{"colspan",			HTMLATT_COLSPAN			},
	{"content",			HTMLATT_CONTENT			},
	{"coords",			HTMLATT_COORDS			},
	{"counter-reset",		HTMLATT_COUNTER_RESET			},
	{"counter-increment",	HTMLATT_COUNTER_INCREMENT			},

	{"cursor",			HTMLATT_CURSOR			},
	{"data",			HTMLATT_DATA			},
	{"defer",			HTMLATT_DEFER			},
	{"disabled",		HTMLATT_DISABLED		},
	{"display",			HTMLATT_DISPLAY			},
	{"face",			HTMLATT_FACE			},
	{"for",				HTMLATT_FOR			},
	{"float",			HTMLATT_FLOAT			},
	{"font",			HTMLATTGROUP_FONT		},
	{"font-family",		HTMLATT_FONT_FAMILY		},
	{"font-style",		HTMLATT_FONT_STYLE		},
	{"font-variant",	HTMLATT_FONT_VARIANT	},
	{"font-weight",		HTMLATT_FONT_WEIGHT		},
	{"font-size",		HTMLATT_FONT_SIZE		},
	{"font-size-adjust",	HTMLATT_FONT_SIZE_ADJUST		},
	{"frameborder",		HTMLATT_FRAMEBORDER		},
	{"height",			HTMLATT_HEIGHT			},
	{"href",			HTMLATT_HREF			},
	{"hreflang",		HTMLATT_HREFLANG		},
	{"hspace",			HTMLATT_HSPACE		},
	{"http-equiv",		HTMLATT_HTTP_EQUIV		},
	{"id",				HTMLATT_ID				},
	{"charset",			HTMLATT_CHARSET			},
	{"dir",				HTMLATT_DIRECTION		},		/* spelling used when attached to html tag */
	{"direction",		HTMLATT_DIRECTION		},		/* spelling used as css style */
	{"lang",			HTMLATT_LANG			},
	{"xml:lang",		HTMLATT_LANG			},
	{"label",			HTMLATT_LABEL			},
	{"language",		HTMLATT_LANGUAGE		},
	{"left",			HTMLATT_LEFT			},
	{"line-height",		HTMLATT_LINE_HEIGHT		},
	{"link",			HTMLATT_LINK			},
	{"list-style",		HTMLATTGROUP_LIST_STYLE		},
	{"list-style-image",HTMLATT_LIST_STYLE_IMAGE	},
	{"list-style-position",HTMLATT_LIST_STYLE_POSITION	},
	{"list-style-type",	HTMLATT_LIST_STYLE_TYPE	},
	{"name",			HTMLATT_NAME			},
	{"no-wrap",			HTMLATT_NOWRAP			},	//non-conforming alternate spelling
	{"nowrap",			HTMLATT_NOWRAP			},
	{"margin",			HTMLATT_MARGIN			},
	{"margin-bottom",	HTMLATT_MARGIN_BOTTOM	},
	{"margin-left",		HTMLATT_MARGIN_LEFT		},
	{"margin-right",	HTMLATT_MARGIN_RIGHT	},
	{"margin-top",		HTMLATT_MARGIN_TOP		},
	{"topmargin",		HTMLATT_MARGIN_TOP		},
	{"leftmargin",		HTMLATT_MARGIN_LEFT		},
	{"marginheight",	HTMLATT_MARGIN_HEIGHT	},
	{"marginwidth",		HTMLATT_MARGIN_WIDTH	},
	{"maxlength",		HTMLATT_MAXLENGTH		},
	{"min-height",		HTMLATT_MINHEIGHT		},
	{"max-height",		HTMLATT_MAXHEIGHT		},
	{"min-width",		HTMLATT_MINWIDTH		},
	{"max-width",		HTMLATT_MAXWIDTH		},
	{"method",			HTMLATT_METHOD			},
	{"media",			HTMLATT_MEDIA			},
	{"multiple",		HTMLATT_MULTIPLE		},
	{"nohref",			HTMLATT_NOHREF			},
	{"opacity",			HTMLATT_OPACITY			},
	{"onblur",			HTMLATT_ONBLUR			},
	{"onchange",		HTMLATT_ONCHANGE		},
	{"onclick",			HTMLATT_ONCLICK			},
	{"onfocus",			HTMLATT_ONFOCUS			},
	{"onsubmit",		HTMLATT_ONSUBMIT		},
	{"onerror",			HTMLATT_ONERROR			},
	{"onload",			HTMLATT_ONLOAD			},
	{"onmousedown",		HTMLATT_ONMOUSEDOWN		},
	{"onmouseup",		HTMLATT_ONMOUSEUP		},
	{"onmouseover",		HTMLATT_ONMOUSEOVER		},
	{"onmouseout",		HTMLATT_ONMOUSEOUT		},
	{"onkeypress",		HTMLATT_ONKEYPRESS		},
	{"outline",			HTMLATTGROUP_OUTLINE	},
	{"outline-width",	HTMLATT_OUTLINE_WIDTH	},
	{"outline-style",	HTMLATT_OUTLINE_STYLE	},
	{"outline-color",	HTMLATT_OUTLINE_COLOR	},
	{"overflow",		HTMLATTGROUP_OVERFLOW	},
	{"overflow-x",		HTMLATT_OVERFLOW_X		},
	{"overflow-y",		HTMLATT_OVERFLOW_Y		},
	{"padding",			HTMLATTGROUP_PADDING	},
	{"padding-left",	HTMLATT_PADDING_LEFT	},
	{"padding-right",	HTMLATT_PADDING_RIGHT	},
	{"padding-top",		HTMLATT_PADDING_TOP		},
	{"padding-bottom",	HTMLATT_PADDING_BOTTOM	},
	{"page-break-after",	HTMLATT_PAGE_BREAK_AFTER	},
	{"page-break-inside",	HTMLATT_PAGE_BREAK_INSIDE	},
	{"position",		HTMLATT_POSITION		},
	{"profile",			HTMLATT_PROFILE		},
	{"rel",				HTMLATT_REL				},
	{"right",			HTMLATT_RIGHT			},
	{"rows",			HTMLATT_ROWS			},
	{"rowspan",			HTMLATT_ROWSPAN			},
	{"rules",			HTMLATT_RULES			},
	{"selected",		HTMLATT_SELECTED		},
	{"scrolling",		HTMLATT_SCROLLING		},
	{"scope",			HTMLATT_SCOPE			},
	{"longdesc",		HTMLATT_LONGDESC		},
	{"shape",			HTMLATT_SHAPE			},
	{"size",			HTMLATT_SIZE			},
	{"src",				HTMLATT_SRC				},
	{"style",			HTMLATT_STYLE			},
	{"summary",			HTMLATT_SUMMARY			},
	{"tabindex",		HTMLATT_TABINDEX		},
	{"target",			HTMLATT_TARGET			},
	{"text-align",		HTMLATT_TEXT_ALIGN		},
	{"text-indent",		HTMLATT_TEXT_INDENT		},
	{"text-decoration",	HTMLATT_TEXT_DECORATION	},
	{"text-overflow",	HTMLATT_TEXT_OVERFLOW	},
	{"text-shadow",		HTMLATTGROUP_TEXT_SHADOW		},
	{"text-transform",	HTMLATT_TEXT_TRANSFORM	},
	{"table-layout",	HTMLATT_TABLE_LAYOUT	},
	{"letter-spacing",	HTMLATT_LETTER_SPACING	},
	{"word-spacing",	HTMLATT_WORD_SPACING	},
	{"title",			HTMLATT_TITLE			},
	{"start",			HTMLATT_START			},
	{"top",				HTMLATT_TOP				},
	{"type",			HTMLATT_TYPE			},
	{"usemap",			HTMLATT_USEMAP			},
	{"valign",			HTMLATT_VALIGN			},
	{"value",			HTMLATT_VALUE			},
	{"vertical-align",	HTMLATT_VERTICAL_ALIGN	},
	{"visibility",		HTMLATT_VISIBILITY	},
	{"vlink",			HTMLATT_VLINK			},
	{"voice-family",	HTMLATT_VOICE_FAMILY	},
	{"vspace",			HTMLATT_VSPACE			},
	{"white-space",		HTMLATT_WHITE_SPACE		},
	{"wrap",			HTMLATT_WRAP			},
	{"width",			HTMLATT_WIDTH			},
	{"xmlns",			HTMLATT_XMLNS			},
	{"xmlns:v",			HTMLATT_XMLNS_V			},
	{"z-index",			HTMLATT_Z_INDEX			},
	{"zoom",			HTMLATT_ZOOM			}};

/* these are internal but can be enabled/disabled */

ATTLIST_DEF kGUIHTMLPageObj::m_attlist2[]={
	{"background-x",			HTMLATT_BACKGROUND_POSITIONX	},
	{"background-y",			HTMLATT_BACKGROUND_POSITIONY	},
	{"before",					HTMLATT_CONTENT_BEFORE	},
	{"after",					HTMLATT_CONTENT_AFTER	},
	{"border-spacing-horiz",	HTMLATT_BORDER_SPACING_HORIZ	},
	{"border-spacing-vert",		HTMLATT_BORDER_SPACING_VERT		},
	{"border-width",			HTMLATT_BORDER_WIDTH			},
	{"text-shadow-x",			HTMLATT_TEXT_SHADOW_X			},
	{"text-shadow-y",			HTMLATT_TEXT_SHADOW_Y			},
	{"text-shadow-radius",		HTMLATT_TEXT_SHADOW_R			},
	{"text-shadow-color",		HTMLATT_TEXT_SHADOW_COLOR		}};

/* this is used by the browser settings enable/disable code */
const char *kGUIHTMLPageObj::GetCSSAttributeName(unsigned int n)
{
	unsigned int i;
	
	for(i=0;i<(sizeof(m_attlist)/sizeof(ATTLIST_DEF));++i)
	{
		if(m_attlist[i].attid==n)
			return (m_attlist[i].name);
	}

	for(i=0;i<(sizeof(m_attlist2)/sizeof(ATTLIST_DEF));++i)
	{
		if(m_attlist2[i].attid==n)
			return (m_attlist2[i].name);
	}
	
	assert(false,"Attribute not in lists!");
	return(0);	/* never hit because of assert */
}

	/* constants */

enum
{
HTMLCONST_ALICEBLUE,
HTMLCONST_ANTIQUEWHITE,
HTMLCONST_AQUA,
HTMLCONST_AQUAMARINE,
HTMLCONST_AZURE,
HTMLCONST_BEIGE,
HTMLCONST_BISQUE,
HTMLCONST_BLACK,
HTMLCONST_BLANCHEDALMOND,
HTMLCONST_BLUE,
HTMLCONST_BLUEVIOLET,
HTMLCONST_BROWN,
HTMLCONST_BURLYWOOD,
HTMLCONST_CADETBLUE,
HTMLCONST_CHARTREUSE,
HTMLCONST_CHOCOLATE,
HTMLCONST_CORAL,
HTMLCONST_CORNFLOWERBLUE,
HTMLCONST_CORNSILK,
HTMLCONST_CRIMSON,
HTMLCONST_CYAN,
HTMLCONST_DARKBLUE,
HTMLCONST_DARKCYAN,
HTMLCONST_DARKGOLDENROD,
HTMLCONST_DARKGRAY,
HTMLCONST_DARKGREEN,
HTMLCONST_DARKKHAKI,
HTMLCONST_DARKMAGENTA,
HTMLCONST_DARKOLIVEGREEN,
HTMLCONST_DARKORANGE,
HTMLCONST_DARKORCHID,
HTMLCONST_DARKRED,
HTMLCONST_DARKSALMON,
HTMLCONST_DARKSEAGREEN,
HTMLCONST_DARKSLATEBLUE,
HTMLCONST_DARKSLATEGRAY,
HTMLCONST_DARKSLATEGREY,
HTMLCONST_DARKTURQUOISE,
HTMLCONST_DARKVIOLET,
HTMLCONST_DEEPPINK,
HTMLCONST_DEEPSKYBLUE,
HTMLCONST_DIMGRAY,
HTMLCONST_DIMGREY,
HTMLCONST_DODGERBLUE,
HTMLCONST_FIREBRICK,
HTMLCONST_FLORALWHITE,
HTMLCONST_FORESTGREEN,
HTMLCONST_FUCHSIA,
HTMLCONST_GAINSBORO,
HTMLCONST_GHOSTWHITE,
HTMLCONST_GOLD,
HTMLCONST_GOLDENROD,
HTMLCONST_GRAY,
HTMLCONST_GREEN,
HTMLCONST_GREENYELLOW,
HTMLCONST_GREY,
HTMLCONST_HONEYDEW,
HTMLCONST_HOTPINK,
HTMLCONST_INDIANRED,
HTMLCONST_INDIGO,
HTMLCONST_IVORY,
HTMLCONST_KHAKI,
HTMLCONST_LAVENDER,
HTMLCONST_LAVENDERBLUSH,
HTMLCONST_LAWNGREEN,
HTMLCONST_LEMONCHIFFON,
HTMLCONST_LIGHTBLUE,
HTMLCONST_LIGHTCORAL,
HTMLCONST_LIGHTCYAN,
HTMLCONST_LIGHTGOLDENRODYELLOW,
HTMLCONST_LIGHTGRAY,
HTMLCONST_LIGHTGREEN,
HTMLCONST_LIGHTPINK,
HTMLCONST_LIGHTSALMON,
HTMLCONST_LIGHTSEAGREEN,
HTMLCONST_LIGHTSKYBLUE,
HTMLCONST_LIGHTSLATEGRAY,
HTMLCONST_LIGHTSLATEGREY,
HTMLCONST_LIGHTSTEELBLUE,
HTMLCONST_LIGHTYELLOW,
HTMLCONST_LIME,
HTMLCONST_LIMEGREEN,
HTMLCONST_LINEN,
HTMLCONST_MAGENTA,
HTMLCONST_MAROON,
HTMLCONST_MEDIUMAQUAMARINE,
HTMLCONST_MEDIUMBLUE,
HTMLCONST_MEDIUMORCHID,
HTMLCONST_MEDIUMPURPLE,
HTMLCONST_MEDIUMSEAGREEN,
HTMLCONST_MEDIUMSLATEBLUE,
HTMLCONST_MEDIUMSPRINGGREEN,
HTMLCONST_MEDIUMTURQUOISE,
HTMLCONST_MEDIUMVIOLETRED,
HTMLCONST_MIDNIGHTBLUE,
HTMLCONST_MINTCREAM,
HTMLCONST_MISTYROSE,
HTMLCONST_MOCCASIN,
HTMLCONST_NAVAJOWHITE,
HTMLCONST_NAVY,
HTMLCONST_OLDLACE,
HTMLCONST_OLIVE,
HTMLCONST_OLIVEDRAB,
HTMLCONST_ORANGE,
HTMLCONST_ORANGERED,
HTMLCONST_ORCHID,
HTMLCONST_PALEGOLDENROD,
HTMLCONST_PALEGREEN,
HTMLCONST_PALETURQUOISE,
HTMLCONST_PALEVIOLETRED,
HTMLCONST_PAPAYAWHIP,
HTMLCONST_PEACHPUFF,
HTMLCONST_PERU,
HTMLCONST_PINK,
HTMLCONST_PLUM,
HTMLCONST_POWDERBLUE,
HTMLCONST_PURPLE,
HTMLCONST_RED,
HTMLCONST_ROSYBROWN,
HTMLCONST_ROYALBLUE,
HTMLCONST_SADDLEBROWN,
HTMLCONST_SALMON,
HTMLCONST_SANDYBROWN,
HTMLCONST_SEAGREEN,
HTMLCONST_SEASHELL,
HTMLCONST_SIENNA,
HTMLCONST_SILVER,
HTMLCONST_SKYBLUE,
HTMLCONST_SLATEBLUE,
HTMLCONST_SLATEGRAY,
HTMLCONST_SLATEGREY,
HTMLCONST_SNOW,
HTMLCONST_SPRINGGREEN,
HTMLCONST_STEELBLUE,
HTMLCONST_TAN,
HTMLCONST_TEAL,
HTMLCONST_THISTLE,
HTMLCONST_TOMATO,
HTMLCONST_TURQUOISE,
HTMLCONST_VIOLET,
HTMLCONST_WHEAT,
HTMLCONST_WHITE,
HTMLCONST_WHITESMOKE,
HTMLCONST_YELLOW,
HTMLCONST_YELLOWGREEN,

HTMLCONST_TRANSPARENT,
HTMLCONST_INVERT,

HTMLCONST_THIN,		/* border thickness */
HTMLCONST_MEDIUM,
HTMLCONST_THICK,
HTMLCONST_LENGTH,

HTMLCONST_NONE,		/* border styles */
HTMLCONST_HIDDEN,
HTMLCONST_DOTTED,
HTMLCONST_DASHED,
HTMLCONST_SOLID,
HTMLCONST_DOUBLE,
HTMLCONST_GROOVE,
HTMLCONST_RIDGE,
HTMLCONST_INSET,
HTMLCONST_OUTSET,

HTMLCONST_ABSOLUTE,
HTMLCONST_FIXED,
HTMLCONST_RELATIVE,
HTMLCONST_STATIC,
HTMLCONST_INHERIT,

HTMLCONST_ALL,
HTMLCONST_BOTH,
HTMLCONST_LEFT,
HTMLCONST_RIGHT,
HTMLCONST_CENTER,
HTMLCONST_JUSTIFY,
HTMLCONST_TOP,
HTMLCONST_MIDDLE,
HTMLCONST_BOTTOM,
HTMLCONST_ABSCENTER,
HTMLCONST_ABSBOTTOM,
HTMLCONST_ABSMIDDLE,
HTMLCONST_BASELINE,
HTMLCONST_SUB,
HTMLCONST_SUPER,

HTMLCONST_BUTTON,
HTMLCONST_RESET,
HTMLCONST_SUBMIT,
HTMLCONST_CHECKBOX,
HTMLCONST_RADIO,
HTMLCONST_TEXT,
HTMLCONST_FILE,
HTMLCONST_PASSWORD,
HTMLCONST_IMAGE,

HTMLCONST_REFRESH,
HTMLCONST_IMPORT,
HTMLCONST_CHARSET,
HTMLCONST_MEDIA,
HTMLCONST_PAGE,

HTMLCONST_NOT,
HTMLCONST_LINK,
HTMLCONST_VISITED,
HTMLCONST_ACTIVE,
HTMLCONST_HOVER,
HTMLCONST_FOCUS,
HTMLCONST_LASTCHILD,
HTMLCONST_NTHCHILD,
HTMLCONST_NTHLASTCHILD,
HTMLCONST_NTHOFTYPE,
HTMLCONST_NTHLASTOFTYPE,
HTMLCONST_FIRSTCHILD,
HTMLCONST_ONLYCHILD,
HTMLCONST_FIRSTOFTYPE,
HTMLCONST_LASTOFTYPE,
HTMLCONST_ONLYOFTYPE,
HTMLCONST_EMPTY,
HTMLCONST_BEFORE,
HTMLCONST_AFTER,
HTMLCONST_ROOT,
HTMLCONST_LANG,
HTMLCONST_TARGET,
HTMLCONST_ENABLED,
HTMLCONST_DISABLED,
HTMLCONST_CHECKED,

HTMLCONST_XXSMALL,
HTMLCONST_XSMALL,
HTMLCONST_SMALL,
HTMLCONST_LARGE,
HTMLCONST_XLARGE,
HTMLCONST_XXLARGE,
HTMLCONST_SMALLER,
HTMLCONST_LARGER,

HTMLCONST_NORMAL,
HTMLCONST_NOWRAP,
HTMLCONST_PRE,
HTMLCONST_PREWRAP,
HTMLCONST_PRELINE,

HTMLCONST_BLOCK,
HTMLCONST_INLINE,
HTMLCONST_INLINE_BLOCK,
HTMLCONST_INLINE_TABLE,
HTMLCONST_TABLE,
HTMLCONST_TABLE_CELL,
HTMLCONST_LIST_ITEM,
HTMLCONST_COMPACT,
HTMLCONST_RUN_IN,
HTMLCONST_MARKER,

HTMLCONST_BOLD,
HTMLCONST_BOLDER,
HTMLCONST_LIGHTER,
HTMLCONST_UNDERLINE,
HTMLCONST_OVERLINE,
HTMLCONST_LINETHROUGH,
HTMLCONST_BLINK,
HTMLCONST_ITALIC,
HTMLCONST_OBLIQUE,
HTMLCONST_SMALL_CAPS,

HTMLCONST_CAPTION,
HTMLCONST_ICON,
HTMLCONST_MENU,
HTMLCONST_MESSAGE_BOX,
HTMLCONST_SMALL_CAPTION,
HTMLCONST_STATUS_BAR,

HTMLCONST_NOREPEAT,
HTMLCONST_REPEAT,
HTMLCONST_REPEATX,
HTMLCONST_REPEATY,
HTMLCONST_SCROLL,

HTMLCONST_DEFAULT,
HTMLCONST_RECT,
HTMLCONST_RECTANGLE,
HTMLCONST_CIRCLE,
HTMLCONST_CIRC,
HTMLCONST_POLYGON,
HTMLCONST_POLY,

HTMLCONST_INSIDE,
HTMLCONST_OUTSIDE,

HTMLCONST_SQUARE,
HTMLCONST_DECIMAL,
HTMLCONST_DECIMAL_LEADING_ZERO,
HTMLCONST_LOWER_ROMAN,
HTMLCONST_UPPER_ROMAN,
HTMLCONST_LOWER_ALPHA,
HTMLCONST_UPPER_ALPHA,
HTMLCONST_LOWER_LATIN,
HTMLCONST_UPPER_LATIN,
HTMLCONST_LOWER_GREEK,

HTMLCONST_DEFAULT_STYLE,
HTMLCONST_VISIBLE,
HTMLCONST_COLLAPSE,
HTMLCONST_SEPARATE,

HTMLCONST_CAPITALIZE,
HTMLCONST_UPPERCASE,
HTMLCONST_LOWERCASE,

HTMLCONST_AUTO,

HTMLCONST_POINTER,
HTMLCONST_HAND,
HTMLCONST_HELP,

HTMLCONST_DISC,

HTMLCONST_LTR,
HTMLCONST_RTL,

HTMLCONST_VIRTUAL,
HTMLCONST_HARD,
HTMLCONST_SOFT,
HTMLCONST_OFF,

HTMLCONST_ATTR,
HTMLCONST_URL,
HTMLCONST_OPEN_QUOTE,
HTMLCONST_CLOSE_QUOTE,

HTMLCONST_CURRENTCOLOR,

HTMLCONST_ODD,
HTMLCONST_EVEN,

HTMLCONST_CLIP,
HTMLCONST_ELLIPSIS,
HTMLCONST_ELLIPSIS_WORD

};

/* the order of these correspond to the colors in the const enum list above */
kGUIColor kGUIHTMLPageObj::m_colors[]={
	DrawColorA(0xf0,0xf8,0xff,255), /* aliceblue*/
	DrawColorA(0xfa,0xeb,0xd7,255), /* antiquewhite*/
	DrawColorA(0x00,0xff,0xff,255), /* aqua*/
	DrawColorA(0x7f,0xff,0xd4,255), /* aquamarine*/
	DrawColorA(0xf0,0xff,0xff,255), /* azure*/
	DrawColorA(0xf5,0xf5,0xdc,255), /* beige*/
	DrawColorA(0xff,0xe4,0xc4,255), /* bisque*/
	DrawColorA(0x00,0x00,0x00,255), /* black*/
	DrawColorA(0xff,0xeb,0xcd,255), /* blanchedalmond*/
	DrawColorA(0x00,0x00,0xff,255), /* blue*/
	DrawColorA(0x8a,0x2b,0xe2,255), /* blueviolet*/
	DrawColorA(0xa5,0x2a,0x2a,255), /* brown*/
	DrawColorA(0xde,0xb8,0x87,255), /* burlywood*/
	DrawColorA(0x5f,0x9e,0xa0,255), /* cadetblue*/
	DrawColorA(0x7f,0xff,0x00,255), /* chartreuse*/
	DrawColorA(0xd2,0x69,0x1e,255), /* chocolate*/
	DrawColorA(0xff,0x7f,0x50,255), /* coral*/
	DrawColorA(0x64,0x95,0xed,255), /* cornflowerblue*/
	DrawColorA(0xff,0xf8,0xdc,255), /* cornsilk*/
	DrawColorA(0xdc,0x14,0x3c,255), /* crimson*/
	DrawColorA(0x00,0xff,0xff,255), /* cyan*/
	DrawColorA(0x00,0x00,0x8b,255), /* darkblue*/
	DrawColorA(0x00,0x8b,0x8b,255), /* darkcyan*/
	DrawColorA(0xb8,0x86,0x0b,255), /* darkgoldenrod*/
	DrawColorA(0xa9,0xa9,0xa9,255), /* darkgray*/
	DrawColorA(0x00,0x64,0x00,255), /* darkgreen*/
	DrawColorA(0xbd,0xb7,0x6b,255), /* darkkhaki*/
	DrawColorA(0x8b,0x00,0x8b,255), /* darkmagenta*/
	DrawColorA(0x55,0x6b,0x2f,255), /* darkolivegreen*/
	DrawColorA(0xff,0x8c,0x00,255), /* darkorange*/
	DrawColorA(0x99,0x32,0xcc,255), /* darkorchid*/
	DrawColorA(0x8b,0x00,0x00,255), /* darkred*/
	DrawColorA(0xe9,0x96,0x7a,255), /* darksalmon*/
	DrawColorA(0x8f,0xbc,0x8f,255), /* darkseagreen*/
	DrawColorA(0x48,0x3d,0x8b,255), /* darkslateblue*/
	DrawColorA(0x2f,0x4f,0x4f,255), /* darkslategray*/
	DrawColorA(0x2f,0x4f,0x4f,255), /* darkslategrey*/
	DrawColorA(0x00,0xce,0xd1,255), /* darkturquoise*/
	DrawColorA(0x94,0x00,0xd3,255), /* darkviolet*/
	DrawColorA(0xff,0x14,0x93,255), /* deeppink*/
	DrawColorA(0x00,0xbf,0xff,255), /* deepskyblue*/
	DrawColorA(0x69,0x69,0x69,255), /* dimgray*/
	DrawColorA(0x69,0x69,0x69,255), /* dimgrey*/
	DrawColorA(0x1e,0x90,0xff,255), /* dodgerblue*/
	DrawColorA(0xb2,0x22,0x22,255), /* firebrick*/
	DrawColorA(0xff,0xfa,0xf0,255), /* floralwhite*/
	DrawColorA(0x22,0x8b,0x22,255), /* forestgreen*/
	DrawColorA(0xff,0x00,0xff,255), /* fuchsia*/
	DrawColorA(0xdc,0xdc,0xdc,255), /* gainsboro*/
	DrawColorA(0xf8,0xf8,0xff,255), /* ghostwhite*/
	DrawColorA(0xff,0xd7,0x00,255), /* gold*/
	DrawColorA(0xda,0xa5,0x20,255), /* goldenrod*/
	DrawColorA(0x80,0x80,0x80,255), /* gray*/
	DrawColorA(0x00,0x80,0x00,255), /* green*/
	DrawColorA(0xad,0xff,0x2f,255), /* greenyellow*/
	DrawColorA(0x80,0x80,0x80,255), /* grey*/
	DrawColorA(0xf0,0xff,0xf0,255), /* honeydew*/
	DrawColorA(0xff,0x69,0xb4,255), /* hotpink*/
	DrawColorA(0xcd,0x5c,0x5c,255), /* indianred*/
	DrawColorA(0x4b,0x00,0x82,255), /* indigo*/
	DrawColorA(0xff,0xff,0xf0,255), /* ivory*/
	DrawColorA(0xf0,0xe6,0x8c,255), /* khaki*/
	DrawColorA(0xe6,0xe6,0xfa,255), /* lavender*/
	DrawColorA(0xff,0xf0,0xf5,255), /* lavenderblush*/
	DrawColorA(0x7c,0xfc,0x00,255), /* lawngreen*/
	DrawColorA(0xff,0xfa,0xcd,255), /* lemonchiffon*/
	DrawColorA(0xad,0xd8,0xe6,255), /* lightblue*/
	DrawColorA(0xf0,0x80,0x80,255), /* lightcoral*/
	DrawColorA(0xe0,0xff,0xff,255), /* lightcyan*/
	DrawColorA(0xfa,0xfa,0xd2,255), /* lightgoldenrodyellow*/
	DrawColorA(0xd3,0xd3,0xd3,255), /* lightgray*/
	DrawColorA(0x90,0xee,0x90,255), /* lightgreen*/
	DrawColorA(0xff,0xb6,0xc1,255), /* lightpink*/
	DrawColorA(0xff,0xa0,0x7a,255), /* lightsalmon*/
	DrawColorA(0x20,0xb2,0xaa,255), /* lightseagreen*/
	DrawColorA(0x87,0xce,0xfa,255), /* lightskyblue*/
	DrawColorA(0x77,0x88,0x99,255), /* lightslategray*/
	DrawColorA(0x77,0x88,0x99,255), /* lightslategrey*/
	DrawColorA(0xb0,0xc4,0xde,255), /* lightsteelblue*/
	DrawColorA(0xff,0xff,0xe0,255), /* lightyellow*/
	DrawColorA(0x00,0xff,0x00,255), /* lime*/
	DrawColorA(0x32,0xcd,0x32,255), /* limegreen*/
	DrawColorA(0xfa,0xf0,0xe6,255), /* linen*/
	DrawColorA(0xff,0x00,0xff,255), /* magenta*/
	DrawColorA(0x80,0x00,0x00,255), /* maroon*/
	DrawColorA(0x66,0xcd,0xaa,255), /* mediumaquamarine*/
	DrawColorA(0x00,0x00,0xcd,255), /* mediumblue*/
	DrawColorA(0xba,0x55,0xd3,255), /* mediumorchid*/
	DrawColorA(0x93,0x70,0xdb,255), /* mediumpurple*/
	DrawColorA(0x3c,0xb3,0x71,255), /* mediumseagreen*/
	DrawColorA(0x7b,0x68,0xee,255), /* mediumslateblue*/
	DrawColorA(0x00,0xfa,0x9a,255), /* mediumspringgreen*/
	DrawColorA(0x48,0xd1,0xcc,255), /* mediumturquoise*/
	DrawColorA(0xc7,0x15,0x85,255), /* mediumvioletred*/
	DrawColorA(0x19,0x19,0x70,255), /* midnightblue*/
	DrawColorA(0xf5,0xff,0xfa,255), /* mintcream*/
	DrawColorA(0xff,0xe4,0xe1,255), /* mistyrose*/
	DrawColorA(0xff,0xe4,0xb5,255), /* moccasin*/
	DrawColorA(0xff,0xde,0xad,255), /* navajowhite*/
	DrawColorA(0x00,0x00,0x80,255), /* navy*/
	DrawColorA(0xfd,0xf5,0xe6,255), /* oldlace*/
	DrawColorA(0x80,0x80,0x00,255), /* olive*/
	DrawColorA(0x6b,0x8e,0x23,255), /* olivedrab*/
	DrawColorA(0xff,0xa5,0x00,255), /* orange*/
	DrawColorA(0xff,0x45,0x00,255), /* orangered*/
	DrawColorA(0xda,0x70,0xd6,255), /* orchid*/
	DrawColorA(0xee,0xe8,0xaa,255), /* palegoldenrod*/
	DrawColorA(0x98,0xfb,0x98,255), /* palegreen*/
	DrawColorA(0xaf,0xee,0xee,255), /* paleturquoise*/
	DrawColorA(0xdb,0x70,0x93,255), /* palevioletred*/
	DrawColorA(0xff,0xef,0xd5,255), /* papayawhip*/
	DrawColorA(0xff,0xda,0xb9,255), /* peachpuff*/
	DrawColorA(0xcd,0x85,0x3f,255), /* peru*/
	DrawColorA(0xff,0xc0,0xcb,255), /* pink*/
	DrawColorA(0xdd,0xa0,0xdd,255), /* plum*/
	DrawColorA(0xb0,0xe0,0xe6,255), /* powderblue*/
	DrawColorA(0x80,0x00,0x80,255), /* purple*/
	DrawColorA(0xff,0x00,0x00,255), /* red*/
	DrawColorA(0xbc,0x8f,0x8f,255), /* rosybrown*/
	DrawColorA(0x41,0x69,0xe1,255), /* royalblue*/
	DrawColorA(0x8b,0x45,0x13,255), /* saddlebrown*/
	DrawColorA(0xfa,0x80,0x72,255), /* salmon*/
	DrawColorA(0xf4,0xa4,0x60,255), /* sandybrown*/
	DrawColorA(0x2e,0x8b,0x57,255), /* seagreen*/
	DrawColorA(0xff,0xf5,0xee,255), /* seashell*/
	DrawColorA(0xa0,0x52,0x2d,255), /* sienna*/
	DrawColorA(0xc0,0xc0,0xc0,255), /* silver*/
	DrawColorA(0x87,0xce,0xeb,255), /* skyblue*/
	DrawColorA(0x6a,0x5a,0xcd,255), /* slateblue*/
	DrawColorA(0x70,0x80,0x90,255), /* slategray*/
	DrawColorA(0x70,0x80,0x90,255), /* slategrey*/
	DrawColorA(0xff,0xfa,0xfa,255), /* snow*/
	DrawColorA(0x00,0xff,0x7f,255), /* springgreen*/
	DrawColorA(0x46,0x82,0xb4,255), /* steelblue*/
	DrawColorA(0xd2,0xb4,0x8c,255), /* tan*/
	DrawColorA(0x00,0x80,0x80,255), /* teal*/
	DrawColorA(0xd8,0xbf,0xd8,255), /* thistle*/
	DrawColorA(0xff,0x63,0x47,255), /* tomato*/
	DrawColorA(0x40,0xe0,0xd0,255), /* turquoise*/
	DrawColorA(0xee,0x82,0xee,255), /* violet*/
	DrawColorA(0xf5,0xde,0xb3,255), /* wheat*/
	DrawColorA(0xff,0xff,0xff,255), /* white*/
	DrawColorA(0xf5,0xf5,0xf5,255), /* whitesmoke*/
	DrawColorA(0xff,0xff,0x00,255), /* yellow*/
	DrawColorA(0x9a,0xcd,0x32,255) /* yellowgreen*/
};

CONSTLIST_DEF kGUIHTMLPageObj::m_constlist[]={
	{"aliceblue",		HTMLCONST_ALICEBLUE},
	{"antiquewhite",	HTMLCONST_ANTIQUEWHITE},
	{"aqua",			HTMLCONST_AQUA},
	{"aquamarine",		HTMLCONST_AQUAMARINE},
	{"azure", HTMLCONST_AZURE},
	{"beige", HTMLCONST_BEIGE},
	{"bisque", HTMLCONST_BISQUE},
	{"black", HTMLCONST_BLACK},
	{"blanchedalmond", HTMLCONST_BLANCHEDALMOND},
	{"blue", HTMLCONST_BLUE},
	{"blueviolet", HTMLCONST_BLUEVIOLET},
	{"brown", HTMLCONST_BROWN},
	{"burlywood", HTMLCONST_BURLYWOOD},
	{"cadetblue", HTMLCONST_CADETBLUE},
	{"chartreuse", HTMLCONST_CHARTREUSE},
	{"chocolate", HTMLCONST_CHOCOLATE},
	{"coral", HTMLCONST_CORAL},
	{"cornflowerblue", HTMLCONST_CORNFLOWERBLUE},
	{"cornsilk", HTMLCONST_CORNSILK},
	{"crimson", HTMLCONST_CRIMSON},
	{"cyan", HTMLCONST_CYAN},
	{"darkblue", HTMLCONST_DARKBLUE},
	{"darkcyan", HTMLCONST_DARKCYAN},
	{"darkgoldenrod", HTMLCONST_DARKGOLDENROD},
	{"darkgray", HTMLCONST_DARKGRAY},
	{"darkgreen", HTMLCONST_DARKGREEN},
	{"darkgrey", HTMLCONST_DARKGRAY	},
	{"darkkhaki", HTMLCONST_DARKKHAKI},
	{"darkmagenta", HTMLCONST_DARKMAGENTA},
	{"darkolivegreen", HTMLCONST_DARKOLIVEGREEN},
	{"darkorange", HTMLCONST_DARKORANGE},
	{"darkorchid", HTMLCONST_DARKORCHID},
	{"darkred", HTMLCONST_DARKRED},
	{"darksalmon", HTMLCONST_DARKSALMON},
	{"darkseagreen", HTMLCONST_DARKSEAGREEN},
	{"darkslateblue", HTMLCONST_DARKSLATEBLUE},
	{"darkslategray", HTMLCONST_DARKSLATEGRAY},
	{"darkslategrey", HTMLCONST_DARKSLATEGREY},
	{"darkturquoise", HTMLCONST_DARKTURQUOISE},
	{"darkviolet", HTMLCONST_DARKVIOLET},
	{"deeppink", HTMLCONST_DEEPPINK},
	{"deepskyblue", HTMLCONST_DEEPSKYBLUE},
	{"dimgray", HTMLCONST_DIMGRAY},
	{"dimgrey", HTMLCONST_DIMGREY},
	{"dodgerblue", HTMLCONST_DODGERBLUE},
	{"firebrick", HTMLCONST_FIREBRICK},
	{"floralwhite", HTMLCONST_FLORALWHITE},
	{"forestgreen", HTMLCONST_FORESTGREEN},
	{"fuchsia", HTMLCONST_FUCHSIA},
	{"gainsboro", HTMLCONST_GAINSBORO},
	{"ghostwhite", HTMLCONST_GHOSTWHITE},
	{"gold", HTMLCONST_GOLD},
	{"goldenrod", HTMLCONST_GOLDENROD},
	{"gray", HTMLCONST_GRAY},
	{"grey", HTMLCONST_GRAY},
	{"green", HTMLCONST_GREEN},
	{"greenyellow", HTMLCONST_GREENYELLOW},
	{"grey", HTMLCONST_GREY},
	{"honeydew", HTMLCONST_HONEYDEW},
	{"hotpink", HTMLCONST_HOTPINK},
	{"indianred", HTMLCONST_INDIANRED},
	{"indigo", HTMLCONST_INDIGO},
	{"ivory", HTMLCONST_IVORY},
	{"khaki", HTMLCONST_KHAKI},
	{"lavender", HTMLCONST_LAVENDER},
	{"lavenderblush", HTMLCONST_LAVENDERBLUSH},
	{"lawngreen", HTMLCONST_LAWNGREEN},
	{"lemonchiffon", HTMLCONST_LEMONCHIFFON},
	{"lightblue", HTMLCONST_LIGHTBLUE},
	{"lightcoral", HTMLCONST_LIGHTCORAL},
	{"lightcyan", HTMLCONST_LIGHTCYAN},
	{"lightgoldenrodyellow", HTMLCONST_LIGHTGOLDENRODYELLOW},
	{"lightgray",		HTMLCONST_LIGHTGRAY	},
	{"lightgreen", HTMLCONST_LIGHTGREEN},
	{"lightgrey",	HTMLCONST_LIGHTGRAY},
	{"lightpink", HTMLCONST_LIGHTPINK},
	{"lightsalmon", HTMLCONST_LIGHTSALMON},
	{"lightseagreen", HTMLCONST_LIGHTSEAGREEN},
	{"lightskyblue", HTMLCONST_LIGHTSKYBLUE},
	{"lightslategray", HTMLCONST_LIGHTSLATEGRAY},
	{"lightslategrey", HTMLCONST_LIGHTSLATEGREY},
	{"lightsteelblue", HTMLCONST_LIGHTSTEELBLUE},
	{"lightyellow", HTMLCONST_LIGHTYELLOW},
	{"lime", HTMLCONST_LIME},
	{"limegreen", HTMLCONST_LIMEGREEN},
	{"linen", HTMLCONST_LINEN},
	{"magenta", HTMLCONST_MAGENTA},
	{"maroon", HTMLCONST_MAROON},
	{"mediumaquamarine", HTMLCONST_MEDIUMAQUAMARINE},
	{"mediumblue", HTMLCONST_MEDIUMBLUE},
	{"mediumorchid", HTMLCONST_MEDIUMORCHID},
	{"mediumpurple", HTMLCONST_MEDIUMPURPLE},
	{"mediumseagreen", HTMLCONST_MEDIUMSEAGREEN},
	{"mediumslateblue", HTMLCONST_MEDIUMSLATEBLUE},
	{"mediumspringgreen", HTMLCONST_MEDIUMSPRINGGREEN},
	{"mediumturquoise", HTMLCONST_MEDIUMTURQUOISE},
	{"mediumvioletred", HTMLCONST_MEDIUMVIOLETRED},
	{"midnightblue", HTMLCONST_MIDNIGHTBLUE},
	{"mintcream", HTMLCONST_MINTCREAM},
	{"mistyrose", HTMLCONST_MISTYROSE},
	{"moccasin", HTMLCONST_MOCCASIN},
	{"navajowhite", HTMLCONST_NAVAJOWHITE},
	{"navy", HTMLCONST_NAVY},
	{"oldlace", HTMLCONST_OLDLACE},
	{"olive", HTMLCONST_OLIVE},
	{"olivedrab", HTMLCONST_OLIVEDRAB},
	{"orange", HTMLCONST_ORANGE},
	{"orangered", HTMLCONST_ORANGERED},
	{"orchid", HTMLCONST_ORCHID},
	{"palegoldenrod", HTMLCONST_PALEGOLDENROD},
	{"palegreen", HTMLCONST_PALEGREEN},
	{"paleturquoise", HTMLCONST_PALETURQUOISE},
	{"palevioletred", HTMLCONST_PALEVIOLETRED},
	{"papayawhip", HTMLCONST_PAPAYAWHIP},
	{"peachpuff", HTMLCONST_PEACHPUFF},
	{"peru", HTMLCONST_PERU},
	{"pink", HTMLCONST_PINK},
	{"plum", HTMLCONST_PLUM},
	{"powderblue", HTMLCONST_POWDERBLUE},
	{"purple", HTMLCONST_PURPLE},
	{"red", HTMLCONST_RED},
	{"rosybrown", HTMLCONST_ROSYBROWN},
	{"royalblue", HTMLCONST_ROYALBLUE},
	{"saddlebrown", HTMLCONST_SADDLEBROWN},
	{"salmon", HTMLCONST_SALMON},
	{"sandybrown", HTMLCONST_SANDYBROWN},
	{"seagreen", HTMLCONST_SEAGREEN},
	{"seashell", HTMLCONST_SEASHELL},
	{"sienna", HTMLCONST_SIENNA},
	{"silver", HTMLCONST_SILVER},
	{"skyblue", HTMLCONST_SKYBLUE},
	{"slateblue", HTMLCONST_SLATEBLUE},
	{"slategray", HTMLCONST_SLATEGRAY},
	{"slategrey", HTMLCONST_SLATEGREY},
	{"snow", HTMLCONST_SNOW},
	{"springgreen", HTMLCONST_SPRINGGREEN},
	{"steelblue", HTMLCONST_STEELBLUE},
	{"tan", HTMLCONST_TAN},
	{"teal", HTMLCONST_TEAL},
	{"thistle", HTMLCONST_THISTLE},
	{"tomato", HTMLCONST_TOMATO},
	{"turquoise", HTMLCONST_TURQUOISE},
	{"violet", HTMLCONST_VIOLET},
	{"wheat", HTMLCONST_WHEAT},
	{"white", HTMLCONST_WHITE},
	{"whitesmoke", HTMLCONST_WHITESMOKE},
	{"yellow", HTMLCONST_YELLOW},
	{"yellowgreen", HTMLCONST_YELLOWGREEN},
	{"transparent",		HTMLCONST_TRANSPARENT		},
	{"invert",		HTMLCONST_INVERT		},

	{"thin",			HTMLCONST_THIN		},
	{"medium",			HTMLCONST_MEDIUM	},
	{"thick",			HTMLCONST_THICK		},
	{"length",			HTMLCONST_LENGTH	},

	{"all",				HTMLCONST_ALL		},
	{"both",			HTMLCONST_BOTH		},
	{"left",			HTMLCONST_LEFT		},
	{"right",			HTMLCONST_RIGHT		},
	{"center",			HTMLCONST_CENTER	},
	{"justify",			HTMLCONST_JUSTIFY	},
	{"top",				HTMLCONST_TOP		},
	{"middle",			HTMLCONST_MIDDLE	},
	{"bottom",			HTMLCONST_BOTTOM	},
	{"abscenter",		HTMLCONST_ABSCENTER	},
	{"absbottom",		HTMLCONST_ABSBOTTOM	},
	{"absmiddle",		HTMLCONST_ABSMIDDLE	},
	{"baseline",		HTMLCONST_BASELINE	},
	{"sub",				HTMLCONST_SUB	},
	{"super",			HTMLCONST_SUPER	},

	{"none",			HTMLCONST_NONE		},
	{"hidden",			HTMLCONST_HIDDEN	},
	{"dotted",			HTMLCONST_DOTTED	},
	{"dashed",			HTMLCONST_DASHED	},
	{"solid",			HTMLCONST_SOLID		},
	{"double",			HTMLCONST_DOUBLE	},
	{"groove",			HTMLCONST_GROOVE	},
	{"ridge",			HTMLCONST_RIDGE		},
	{"inset",			HTMLCONST_INSET		},
	{"outset",			HTMLCONST_OUTSET	},

	{"absolute",		HTMLCONST_ABSOLUTE	},
	{"fixed",			HTMLCONST_FIXED		},
	{"relative",		HTMLCONST_RELATIVE	},
	{"static",			HTMLCONST_STATIC	},
	{"inherit",			HTMLCONST_INHERIT	},

	{"button",			HTMLCONST_BUTTON	},
	{"reset",			HTMLCONST_RESET	},
	{"submit",			HTMLCONST_SUBMIT	},
	{"checkbox",		HTMLCONST_CHECKBOX	},
	{"radio",			HTMLCONST_RADIO		},
	{"text",			HTMLCONST_TEXT		},
	{"file",			HTMLCONST_FILE		},
	{"password",		HTMLCONST_PASSWORD	},
	{"image",			HTMLCONST_IMAGE		},
	{"refresh",			HTMLCONST_REFRESH	},
	{"import",			HTMLCONST_IMPORT	},
	{"charset",			HTMLCONST_CHARSET	},
	{"media",			HTMLCONST_MEDIA	},
	{"page",			HTMLCONST_PAGE	},

	{"link",			HTMLCONST_LINK	},
	{"visited",			HTMLCONST_VISITED	},
	{"active",			HTMLCONST_ACTIVE	},
	{"hover",			HTMLCONST_HOVER		},
	{"focus",			HTMLCONST_FOCUS		},
	{"first-child",		HTMLCONST_FIRSTCHILD },
	{"last-child",		HTMLCONST_LASTCHILD	},
	{"nth-child",		HTMLCONST_NTHCHILD	},
	{"nth-last-child",	HTMLCONST_NTHLASTCHILD	},
	{"nth-of-type",		HTMLCONST_NTHOFTYPE	},
	{"nth-last-of-type",HTMLCONST_NTHLASTOFTYPE	},
	{"only-child",		HTMLCONST_ONLYCHILD	},
	{"first-of-type",	HTMLCONST_FIRSTOFTYPE	},
	{"last-of-type",	HTMLCONST_LASTOFTYPE	},
	{"only-of-type",	HTMLCONST_ONLYOFTYPE	},
	{"empty",			HTMLCONST_EMPTY		},
	{"not",				HTMLCONST_NOT	},
	{"before",			HTMLCONST_BEFORE	},
	{"after",			HTMLCONST_AFTER		},
	{"root",			HTMLCONST_ROOT		},
	{"lang",			HTMLCONST_LANG		},
	{"target",			HTMLCONST_TARGET	},
	{"enabled",			HTMLCONST_ENABLED	},
	{"disabled",		HTMLCONST_DISABLED	},
	{"checked",			HTMLCONST_CHECKED	},

	{"xx-small",		HTMLCONST_XXSMALL	},
	{"x-small",			HTMLCONST_XSMALL	},
	{"small",			HTMLCONST_SMALL		},
	{"large",			HTMLCONST_LARGE		},
	{"x-large",			HTMLCONST_XLARGE	},
	{"xx-large",		HTMLCONST_XXLARGE	},
	{"smaller",			HTMLCONST_SMALLER	},
	{"larger",			HTMLCONST_LARGER	},

	{"normal",			HTMLCONST_NORMAL	},
	{"nowrap",			HTMLCONST_NOWRAP	},
	{"pre",				HTMLCONST_PRE		},
	{"pre-wrap",		HTMLCONST_PREWRAP	},
	{"pre-line",		HTMLCONST_PRELINE	},

	{"block",			HTMLCONST_BLOCK		},
	{"inline",			HTMLCONST_INLINE	},
	{"inline-block",	HTMLCONST_INLINE_BLOCK	},
	{"inline-table",	HTMLCONST_INLINE_TABLE	},
	{"table",			HTMLCONST_TABLE		},
	{"table-cell",		HTMLCONST_TABLE_CELL	},
	{"list-item",		HTMLCONST_LIST_ITEM	},
	{"compact",			HTMLCONST_COMPACT	},
	{"run-in",			HTMLCONST_RUN_IN	},
	{"marker",			HTMLCONST_MARKER	},

	{"bold",			HTMLCONST_BOLD	},
	{"bolder",			HTMLCONST_BOLDER	},
	{"lighter",			HTMLCONST_LIGHTER	},

	{"underline",		HTMLCONST_UNDERLINE	},
	{"overline",		HTMLCONST_OVERLINE	},
	{"line-through",	HTMLCONST_LINETHROUGH	},
	{"blink",			HTMLCONST_BLINK	},
	{"italic",			HTMLCONST_ITALIC	},
	{"oblique",			HTMLCONST_OBLIQUE	},
	{"small-caps",		HTMLCONST_SMALL_CAPS	},

	{"caption",			HTMLCONST_CAPTION	},
	{"icon",			HTMLCONST_ICON	},
	{"menu",			HTMLCONST_MENU	},
	{"message-box",		HTMLCONST_MESSAGE_BOX	},
	{"small-caption",	HTMLCONST_SMALL_CAPTION	},
	{"status-bar",		HTMLCONST_STATUS_BAR	},

	{"no-repeat",		HTMLCONST_NOREPEAT	},
	{"repeat",			HTMLCONST_REPEAT	},
	{"repeat-x",		HTMLCONST_REPEATX	},
	{"repeat-y",		HTMLCONST_REPEATY	},
	{"x-repeat",		HTMLCONST_REPEATX	},	/* alternate spelling */
	{"y-repeat",		HTMLCONST_REPEATY	},	/* alternate spelling */
	{"scroll",			HTMLCONST_SCROLL	},

	{"default",			HTMLCONST_DEFAULT	},
	{"rect",			HTMLCONST_RECT	},
	{"rectangle",		HTMLCONST_RECTANGLE	},
	{"circle",			HTMLCONST_CIRCLE	},
	{"circ",			HTMLCONST_CIRC	},
	{"polygon",			HTMLCONST_POLYGON	},
	{"poly",			HTMLCONST_POLY	},

	{"inside",			HTMLCONST_INSIDE	},
	{"outside",			HTMLCONST_OUTSIDE	},

	{"square",			HTMLCONST_SQUARE	},
	{"decimal",			HTMLCONST_DECIMAL	},
	{"decimal-leading-zero",HTMLCONST_DECIMAL_LEADING_ZERO	},
	{"lower-roman",		HTMLCONST_LOWER_ROMAN	},
	{"upper-roman",		HTMLCONST_UPPER_ROMAN	},
	{"lower-alpha",		HTMLCONST_LOWER_ALPHA	},
	{"upper-alpha",		HTMLCONST_UPPER_ALPHA	},
	{"lower-latin",		HTMLCONST_LOWER_LATIN	},
	{"upper-latin",		HTMLCONST_UPPER_LATIN	},
	{"lower-greek",		HTMLCONST_LOWER_GREEK	},

	{"default-style",	HTMLCONST_DEFAULT_STYLE	},
	{"visible",			HTMLCONST_VISIBLE	},
	{"collapse",		HTMLCONST_COLLAPSE	},
	{"separate",		HTMLCONST_SEPARATE	},

	{"capitalize",		HTMLCONST_CAPITALIZE },
	{"uppercase",		HTMLCONST_UPPERCASE	},
	{"lowercase",		HTMLCONST_LOWERCASE	},

	{"auto",		HTMLCONST_AUTO	},

	{"pointer",		HTMLCONST_POINTER	},
	{"hand",		HTMLCONST_HAND	},
	{"help",		HTMLCONST_HELP	},

	{"disc",		HTMLCONST_DISC	},

	{"ltr",		HTMLCONST_LTR	},
	{"rtl",		HTMLCONST_RTL	},

	{"virtual",		HTMLCONST_VIRTUAL	},
	{"hard",		HTMLCONST_HARD	},
	{"soft",		HTMLCONST_SOFT	},
	{"off",			HTMLCONST_OFF	},

	{"attr",		HTMLCONST_ATTR	},
	{"url",			HTMLCONST_URL	},
	{"open-quote",		HTMLCONST_OPEN_QUOTE	},
	{"close-quote",		HTMLCONST_CLOSE_QUOTE	},

	{"currentcolor",	HTMLCONST_CURRENTCOLOR	},

	{"odd",				HTMLCONST_ODD	},
	{"even",			HTMLCONST_EVEN	},

	{"clip",			HTMLCONST_CLIP	},
	{"ellipsis",		HTMLCONST_ELLIPSIS	},
	{"ellipsis-word",	HTMLCONST_ELLIPSIS_WORD	},
};

kGUIHTMLPageObj::kGUIHTMLPageObj()
{
	unsigned int i;
	TAGLIST_DEF *tl;
	ATTLIST_DEF *al;
	CONSTLIST_DEF *cl;
	POPLIST_DEF *pl;
	kGUIString pops;

	m_settings=0;	/* pointer to settings */
	m_plugins=0;	/* pointer to plugins group */

	/* if it find's strict in the header then this is enabled */
	m_strict=true;

	m_copytdc=false;

	m_fp=0;
	/* this is a pointer to the item cache class, it needs to be set by the users code */
	m_itemcache=0;
	m_visitedcache=0;
	m_ah=0;
	/* max pending object loads */
	m_shutdown=false;
	m_loadreset=false;
	m_hiloadlist.Init(100);
	m_loadlist.Init(1000);
	m_loadthread.Start(this,CALLBACKNAME(LoadListThread));

	/* debugging */
	m_trace=false;

	m_tcicache.Init(16,sizeof(kGUIHTMLRuleCache));

	m_numblinks=0;
	m_blinkdelay=BLINKDELAY;
	m_blinks.Init(32,4);

	m_fontscale=1;
	m_refreshdelay=0;
	m_pid=0;
	m_linkhover=0;
	m_numloading=0;
	m_drawlinkunder=0;
	m_status=0;
	m_reposition=false;
	SetNumGroups(1);	/* only 1 group */

	m_rootobject=new kGUIHTMLObj();
	m_rootobject->SetID(HTMLTAG_ROOT);
	m_rootobject->m_tag=&m_roottag;
	AddObject(m_rootobject);
	m_rootobject->m_page=this;

	m_fixedfgobject=new kGUIHTMLObj();
	m_fixedfgobject->SetID(HTMLTAG_FIXEDROOT);
	AddObject(m_fixedfgobject);
	m_fixedfgobject->m_page=this;

	m_posinfo.Init(256,32);

	m_numrules=0;
	m_rules.Init(128,32);
	m_rulehash.Init(10,sizeof(kGUIHTMLRule *));
	m_numownerrules=0;
	m_ownerrulesbuilt=false;
	m_ownerrules.Init(128*OWNER_NUM,32);
	m_numclasses=0;
	m_numids=0;

	m_unitscache.Init(10,sizeof(kGUIUnits));

	m_locallinks.Init(32,4);

	m_usehs=true;
	m_usevs=true;
	m_vscrollbar.SetClickSize(10);
	m_hscrollbar.SetClickSize(10);
	m_vscrollbar.SetParent(this);
	m_vscrollbar.SetVert();
	m_hscrollbar.SetParent(this);
	m_hscrollbar.SetHorz();
	m_hscrollbar.SetEventHandler(this,& CALLBACKNAME(ScrollMoveCol));
	m_vscrollbar.SetEventHandler(this,& CALLBACKNAME(ScrollMoveRow));
	m_scroll.SetEventHandler(this,CALLBACKNAME(ScrollEvent));

	/* these are static so only need to be inited once */
	if(m_hashinit==false)
	{
		m_hashinit=true;
		m_taghash.Init(16,sizeof(TAGLIST_DEF *));
		m_atthash.Init(16,sizeof(ATTLIST_DEF *));
		m_consthash.Init(16,sizeof(CONSTLIST_DEF *));
		m_pophash.Init(16,0);				/* no storing necessary */
		m_colorhash.Init(16,sizeof(kGUIColor));

		/* add commands to the hashtable */

		tl=m_taglist;
		for(i=0;i<sizeof(m_taglist)/sizeof(TAGLIST_DEF);++i)
		{
			assert(tl->tokenid==i,"Error, tag order is not correct!");
			m_taghash.Add(tl->name,&tl);
			++tl;
		}

		al=m_attlist;
		for(i=0;i<sizeof(m_attlist)/sizeof(ATTLIST_DEF);++i)
		{
			m_attstrings[al->attid].SetString(al->name);
			m_atthash.Add(al->name,&al);
			++al;
		}

		cl=m_constlist;
		for(i=0;i<sizeof(m_constlist)/sizeof(CONSTLIST_DEF);++i)
		{
			m_consthash.Add(cl->name,&cl);
			++cl;
		}

		/* this list contains invalid parent/children pairs */
		/* if an entry is found when building the DOM then the parent is popped */
		pl=m_poplist;
		for(i=0;i<sizeof(m_poplist)/sizeof(POPLIST_DEF);++i)
		{
			pops.Sprintf("%s:%s",m_taglist[pl->parenttagid].name,m_taglist[pl->childtagid].name);			
			m_pophash.Add(pops.GetString(),0);
			++pl;
		}
	}

	m_srnum=0;
	m_srlist.Init(512,512);
	m_srdataindex=0;
	m_srdata.Init(65536,32768);

	/* init image cache */
	m_numimages=0;
	m_images.Init(256,256);
	m_imagehash.Init(12,sizeof(kGUIOnlineImage *));

	/* init linked item cache */
	m_numlinks=0;
	m_stylepriority=0;	/* priority is in order encountered in source, not load time order */
	m_linkhash.SetCaseSensitive(true);
	m_linkhash.Init(12,sizeof(kGUIOnlineLink *));
	m_links.Init(256,-1);

	m_popmenu.SetEventHandler(this,CALLBACKNAME(DoPopMenu));

	m_lastmousex=0;
	m_lastmousey=0;
	m_hovertoggle=0;
	m_hoverlistsize[0]=0;
	m_hoverlistsize[1]=0;
	m_hoverlist[0].Init(256,32);
	m_hoverlist[1].Init(256,32);

	kGUI::AddEvent(this,CALLBACKNAME(TimerEvent));
}

/* em is the font size NOT the pixel height of the font */
void kGUIHTMLPageObj::CalcEM(void)
{
	assert(m_mode==MODE_MINMAX,"Page EM is not valid on position pass, only on minmax pass");

	PushStyle(sizeof(m_pageem),&m_pageem);
	m_pageem=(unsigned int)(m_fontsize*m_fontscale);
}

/* must be positive, so clip to zero */
void kGUIHTMLPageObj::CalcLH(void)
{
	assert(m_mode==MODE_MINMAX,"Page LH is not valid on position pass, only on minmax pass");

	PushStyle(sizeof(m_pagelineheightpix),&m_pagelineheightpix);
	m_pagelineheightpix=m_lineheightratio.CalcUnitValue((int)m_pageem,(int)m_pageem);
	if(m_pagelineheightpix<0)
		m_pagelineheightpix=0;
}

/* return the current PageEM, only valid on minmax pass */
unsigned int kGUIHTMLPageObj::GetEM(void)
{
	assert(m_mode==MODE_MINMAX,"Page EM is not valid on position pass, only on minmax pass");
	return m_pageem;
}

/* return the current PageLH, only valid on minmax pass */
unsigned int kGUIHTMLPageObj::GetLH(void)
{
	assert(m_mode==MODE_MINMAX,"Page LH is not valid on position pass, only on minmax pass");
	return m_pagelineheightpix;
}


int kGUIHTMLPageObj::GetConstID(const char *string)
{
	CONSTLIST_DEF **clptr;
	CONSTLIST_DEF *cl;

	clptr=(CONSTLIST_DEF **)m_consthash.Find(string);
	if(!clptr)
		return(-1);
	cl=clptr[0];
	return(cl->id);
}

/* invalidate the tci cache entries */
void kGUIHTMLPageObj::InvalidateTCICache(void)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLRuleCache **rcp;

	/* rebuild the owner rules list */
	m_ownerrulesbuilt=false;

	n=m_tcicache.GetNum();
	if(n)
	{
		he=m_tcicache.GetFirst();
		for(i=0;i<n;++i)
		{
			rcp=(kGUIHTMLRuleCache **)he->m_data;
			(*rcp)->SetIsValid(false);
			he=he->GetNext();
		}
	}
}

/* purge the tci cache entries */
void kGUIHTMLPageObj::PurgeTCICache(void)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLRuleCache **rcp;

	/* rebuild the owner rules list */
	m_ownerrulesbuilt=false;

	n=m_tcicache.GetNum();
	if(n)
	{
		he=m_tcicache.GetFirst();
		for(i=0;i<n;++i)
		{
			rcp=(kGUIHTMLRuleCache **)he->m_data;
			delete *(rcp);
			he=he->GetNext();
		}

		/* re-init the cache */
		m_tcicache.Reset();
	}
}

kGUIOnlineImage *kGUIHTMLPageObj::LocateImage(kGUIString *url,kGUIString *referrer)
{
	kGUIOnlineImage *image;
	kGUIOnlineImage **ip;

	/* look through list of images and return pointer to match, if not found, then add it */

	ip=(kGUIOnlineImage **)m_imagehash.Find(url->GetString());
	if(ip)
	{
		image=*(ip);
		/* don't trigger a new load since if it is not in memory then it is already loading */
		if(!image->GetLoadTriggered())
			image->SetURL(url,referrer);
		else
			image->LoadPixels();
		return(image);
	}
	/* make a new image and add it to the list of valid images */
	image=new kGUIOnlineImage();
	image->SetPage(this);
	m_imagehash.Add(url->GetString(),&image);
	m_images.SetEntry(m_numimages++,image);
	image->SetURL(url,referrer);
	return(image);
}

void kGUIHTMLPageObj::ImageLoaded(kGUIOnlineImage *image)
{
	m_reposition=true;
	image->LoadPixels();
	Dirty();
}

kGUIOnlineLink *kGUIHTMLPageObj::LocateLink(kGUIString *url,kGUIString *referrer,unsigned int type,kGUIString *media)
{
	kGUIOnlineLink **linkp;
	kGUIOnlineLink *link;

	/* if we already have one with this URL then just return pointer to it */

	linkp=(kGUIOnlineLink **)m_linkhash.Find(url->GetString());
	if(linkp)
		return(*(linkp));

	/* not found, is it cached on the drive? */
	if(GetItemCache())
	{
		kGUIString fn;
		kGUIString header;

		if(GetItemCache()->Find(url,&fn,&header))
		{
			link=new kGUIOnlineLink(this);

			/* load from cache instead of downloading */
			if(link->CacheURL(url,type,&fn)==true)
			{
				m_linkhash.Add(url->GetString(),&link);
				m_links.SetEntry(m_numlinks++,link);
				link->SetMedia(media);
				link->SetHeader(&header);
				return(link);
			}
			else
				delete link;	/* error loading from cache, re-download it */
		}
	}

	/* make a new Link and add it to the list of valid Links */
	link=new kGUIOnlineLink(this);
	link->SetLoadPending(true);
	link->SetLoadedCallback(this,CALLBACKNAME(LinkLoaded));
	link->SetMedia(media);
	m_linkhash.Add(url->GetString(),&link);
	m_links.SetEntry(m_numlinks++,link);
	link->SetURL(url,referrer,type);
	return(link);
}

unsigned int kGUIHTMLPageObj::GetStylePriority(unsigned int after)
{
	unsigned int i,j,p;
	kGUIHTMLRule *rule;
	kGUIHTMLAttrib *cur;
	kGUIOnlineLink *link;

	if(after==m_stylepriority)
		return (++m_stylepriority);
	else
	{
		/* we are inserting a priority, so we need to increment any greater ones */
		for(i=0;i<m_numlinks;++i)
		{
			link=m_links.GetEntry(i);
			p=link->GetPriority();
			if(p>after)
				link->SetPriority(p+1);
		}

		for(i=0;i<m_numrules;++i)
		{
			rule=m_rules.GetEntry(i);
			for(j=0;j<rule->GetNumAttributes();++j)
			{
				cur=rule->GetAttrib(j);
				p=cur->GetPriority();
				if(p>after)
					cur->SetPriority(p+1);
			}
		}
	}
	++m_stylepriority;
	return(after+1);
}

void kGUIHTMLPageObj::LinkLoaded(kGUIOnlineLink *link)
{
	link->SetLoadPending(false);

	/* if loaded then add file to item cache */
	if(link->GetSize())
	{
		if(GetItemCache())
			GetItemCache()->Add(link->GetURL(),link->GetDL()->GetExpiry(),link->GetDL()->GetLastModified(),link);

		AttachLink(link);
		m_reposition=true;
		Dirty();
	}
}

bool kGUIHTMLPageObj::ValidMedia(kGUIString *m)
{
	unsigned int i,num,j,jl;
	kGUIStringSplit ss;
	kGUIString *w;

	if(strstr(m->GetString(),","))
		num=ss.Split(m,",");
	else
		num=ss.Split(m," ");
	if(!num)
	{
		/* if no media types are specified then use it */
		return(true);
	}

	for(i=0;i<num;++i)
	{
		w=ss.GetWord(i);
		w->Lower();

		/* check for 'All' */
		if(GetConstID(w->GetString())==HTMLCONST_ALL)
			return(true);

		/* only compare against alpha chars so 'screen=screen1' or 'screen=screen%%%' etc */
		jl=w->GetLen();
		for(j=0;j<jl;++j)
		{
			if((w->GetChar(j)<'a') || (w->GetChar(j)>'z'))
			{
				w->Clip(j);
				break;
			}
		}
		if(!strcmp(w->GetString(),m_curmedia.GetString()))
			return(true);
	}
	/* there were media strings present but none matched */
	return(false);
}

void kGUIHTMLPageObj::AttachLink(kGUIOnlineLink *link)
{
	AddMedia(link->GetURL());
	switch(link->GetType())
	{
	case LINKTYPE_CSS:
	{
		kGUIString s;

		/* was it loaded sucessfully? */
		if(link->Open()==true)
		{
			/* convert datahandle to string */
			link->Read(&s,link->GetSize());
			link->Close();

			/* does this stylesheet have a pre-pended Byte-Order-Marker */
			s.CheckBOM();
			s.Trim(TRIM_NULL);		/* remove any trailing nulls */

			/* is this media current? */
			if(s.GetLen())
			{
				if(ValidMedia(link->GetMedia()))
					AddClassStyles(OWNER_AUTHOR,link->GetPriority(),link->GetURL(),&s);
			}
		}
	}
	break;
	case LINKTYPE_ICON:
		m_iconcallback.Call(this,link);
	break;
	}
}

/* generates URLbase and URLroot for relative addressing */
void kGUIHTMLPageObj::SetURL(kGUIString *url)
{
	m_url.SetString(url);
	kGUI::ExtractURL(&m_url,&m_urlbase,&m_urlroot);
}

void kGUIHTMLPageObj::SetBaseURL(kGUIString *url)
{
	kGUI::ExtractURL(url,&m_urlbase,&m_urlroot);
}

void kGUIHTMLPageObj::SetSource(kGUIString *url,kGUIString *source,kGUIString *type,kGUIString *header)
{
	if(m_visitedcache)
		m_visitedcache->Add(url);
	SetURL(url);
	m_type.SetString(type);	/* 'text/html', 'img/jpeg' etc */
	m_header.SetString(header);
	m_fp=source->GetString();
	m_len=source->GetLen();

	Parse(false);
	Position();

	if(m_iconlinked==false)
	{
		kGUIOnlineLink *oll;
		kGUIString local,full;
		kGUIString media;

		//manual link to icon was not found so we will try
		//http://xxx/favicon.ico

		local.SetString("/favicon.ico");
		MakeURL(&m_url,&local,&full);

		oll=LocateLink(&full,&m_url,LINKTYPE_ICON,&media);
		if(oll->GetLoadPending()==false)
			AttachLink(oll);
	}
}

/* save all user input ( from form items ) to one string */
void kGUIHTMLPageObj::SaveInput(Hash *input)
{
	input->Init(8,0);
	SaveInput(input,0,0,m_rootobject);
}

void kGUIHTMLPageObj::SaveInput(Hash *input,kGUIHTMLFormObj *form,int childnum,kGUIHTMLObj *obj)
{
	unsigned int i;
	kGUIHTMLAttrib *att;

	if(obj->GetID()==HTMLTAG_FORM)
	{
		form=obj->m_obj.m_formobj;

		/* iterate through all form child objects */
		for(i=0;i<form->GetNumChildren();++i)
			SaveInput(input,form,i,form->GetChild(i));
	}
	else
	{
		/* traverse the tree all the way down */
		for(i=0;i<obj->m_numstylechildren;++i)
			SaveInput(input,form,i,obj->m_stylechildren.GetEntry(i));

		/* we can only save NAMED items, no name, no save */
		att=obj->FindAttrib(HTMLATT_NAME);
		if(att)
		{
			kGUIString name;
			kGUIString value;

			/* if we have a valid form then use the combined form/input name as it will be more unique */
			if(!form)
				name.SetString(att->GetValue());
			else
				name.Sprintf("%S:%S",form->GetName(),att->GetValue());
			name.ASprintf(".%d",childnum);
			value.Clear();

			switch(obj->GetID())
			{
			case HTMLTAG_INPUT:
				switch(obj->GetSubID())
				{
				case HTMLSUBTAG_INPUTCHECKBOX:
					if(obj->m_obj.m_tickobj->GetSelected()==true)
						value.SetString("1");
					else
						value.SetString("0");
				break;
				case HTMLSUBTAG_INPUTRADIO:
					if(obj->m_obj.m_radioobj->GetSelected()==true)
						value.SetString("1");
					else
						value.SetString("0");
				break;
				case HTMLSUBTAG_INPUTTEXTBOX:
				case HTMLSUBTAG_INPUTFILE:
					value.SetString(obj->m_obj.m_inputobj->GetString());
				break;
				}
			break;
			case HTMLTAG_SELECT:
				switch(obj->GetSubID())
				{
				case HTMLSUBTAG_INPUTLISTBOX:
				{
					/* build a tab seperated string of all selected values */
					unsigned int sel;
					unsigned int index;
					unsigned int numsel;
					Array<unsigned int>selected;

					if(!obj->m_obj.m_listboxobj->GetNumEntries())
						name.Clear();
					else
					{
						selected.Init(16,4);
						numsel=obj->m_obj.m_listboxobj->GetSelections(&selected);
						for(sel=0;sel<numsel;++sel)
						{
							index=selected.GetEntry(sel);
							if(sel)
								value.Append("\t");
							value.Append(obj->m_obj.m_listboxobj->GetSelectionStringObj(index));
						}
					}
				}
				break;
				default:
					if(!obj->m_obj.m_comboboxobj->GetNumEntries())
						name.Clear();
					else
						value.SetString(obj->m_obj.m_comboboxobj->GetSelectionString());
				break;
				}
			break;
			case HTMLTAG_TEXTAREA:
				value.SetString(obj->m_obj.m_inputobj->GetString());
			break;
			default:
				name.Clear();
			break;
			}
			/* append name / value to input string */
			if(name.GetLen())
			{
				/* make sure name doesn't already exist */
				if(!input->Find(name.GetString()))
				{
					/*! @todo Need to Investigate what encoding is to be done on posted form input? Do we default to the page encoding? */

					input->SetDataLen(value.GetLen()+1);
					input->Add(name.GetString(),value.GetString());
				}
				else
				{
					/* error, name already exists! */
				}
			}
		}
	}
}

/* this is called if the user uses to go forward or go backward buttons */
/* it is used to reload the forms on the page with the previous user input */
void kGUIHTMLPageObj::LoadInput(Hash *input)
{
	LoadInput(input,0,0,m_rootobject);
}

void kGUIHTMLPageObj::LoadInput(Hash *input,kGUIHTMLFormObj *form,int childnum,kGUIHTMLObj *obj)
{
	unsigned int i;
	kGUIHTMLAttrib *att;

	if(obj->GetID()==HTMLTAG_FORM)
	{
		form=obj->m_obj.m_formobj;
		for(i=0;i<form->GetNumChildren();++i)
			LoadInput(input,form,i,form->GetChild(i));
	}
	else
	{
		/* traverse the tree all the way down */
		for(i=0;i<obj->m_numstylechildren;++i)
			LoadInput(input,form,i,obj->m_stylechildren.GetEntry(i));

		/* we can only load NAMED items, no name, no load */
		att=obj->FindAttrib(HTMLATT_NAME);
		if(att)
		{
			kGUIString name;
			const char *value;

			/* if we have a valid form then use the combined form/input name as it will be more unique */
			if(!form)
				name.SetString(att->GetValue());
			else
				name.Sprintf("%S:%S",form->GetName(),att->GetValue());
			name.ASprintf(".%d",childnum);

			value=(const char *)input->Find(name.GetString());
			if(value)
			{
				switch(obj->GetID())
				{
				case HTMLTAG_INPUT:
					switch(obj->GetSubID())
					{
					case HTMLSUBTAG_INPUTCHECKBOX:
						obj->m_obj.m_tickobj->SetSelected(value[0]=='1'?true:false);
					break;
					case HTMLSUBTAG_INPUTRADIO:
						obj->m_obj.m_radioobj->SetSelected(value[0]=='1'?true:false);
					break;
					case HTMLSUBTAG_INPUTTEXTBOX:
					case HTMLSUBTAG_INPUTFILE:
						obj->m_obj.m_inputobj->SetString(value);
					break;
					}
				break;
				case HTMLTAG_TEXTAREA:
					obj->m_obj.m_inputobj->SetString(value);
				break;
				case HTMLTAG_SELECT:
					switch(obj->GetSubID())
					{
					case HTMLSUBTAG_INPUTLISTBOX:
						/* since multiple entries can be selected this is a tab seperated list */
						if(obj->m_obj.m_listboxobj->GetNumEntries())
						{
							kGUIString vs;
							kGUIStringSplit ss;
							unsigned int v;
							unsigned int numvalues;

							/* this clears the selected entries */
							obj->m_obj.m_listboxobj->UnSelectRows();
						
							vs.SetString(value);
							numvalues=ss.Split(&vs,"\t");
							for(v=0;v<numvalues;++v)
								obj->m_obj.m_listboxobj->SetSelectionStringz(ss.GetWord(v)->GetString(),true);
						}
					break;
					default:
						obj->m_obj.m_comboboxobj->SetSelectionStringz(value);
					break;
					}
				break;
				}
			}
		}
	}
}

/* this is used to generate a unique URL for each call to @IMPORT since it is */
/* relative not to the page URL but to each subsequent @IMPORT URL */

void kGUIHTMLPageObj::MakeURL(kGUIString *parent,kGUIString *in,kGUIString *out)
{
	kGUIString temp;
	const char *cp;

	assert(parent!=0,"Unknown parent URL!");
	if(in->GetChar(0)=='#' || in->GetChar(0)=='?')
	{
		temp.SetString(parent);
		cp=strstr(temp.GetString(),"#");
		if(cp)
			temp.Clip((unsigned int)(cp-temp.GetString()));
		cp=strstr(temp.GetString(),"?");
		if(cp)
			temp.Clip((unsigned int)(cp-temp.GetString()));

		temp.Append(in);
	}
	else if(!strcmpin(in->GetString(),"mailto:",7))
		temp.SetString(in);
	else if(!strcmpin(in->GetString(),"javascript:",11))
		temp.SetString(in);
	else
	{
		kGUIString base,root;

		/* if the parent is the page URL then use then use this as it handles the BASE command too */
		if(!strcmp(m_url.GetString(),parent->GetString()))
			kGUI::MakeURL(&m_urlbase,&m_urlroot,in,&temp);
		else
		{
			kGUI::ExtractURL(parent,&base,&root);
			kGUI::MakeURL(&base,&root,in,&temp);
		}
	}
	temp.Replace(" ","%20");
	kGUIXMLCODES::Shrink(&temp,out);
}

void kGUIHTMLPageObj::Click(kGUIString *url,kGUIString *referrer)
{
	kGUIHTMLClickInfo info;

	m_loadreset=true;
	/* wait for load event to notice */
	while(m_loadreset)
		kGUI::Sleep(1);
	m_numloading-=m_numskipped;

	/* do we have an attached visited url cache? */
	if(m_visitedcache)
		m_visitedcache->Add(url);

	info.m_newtab=false;
	info.m_post=0;
	info.m_url=url;
	info.m_referrer=referrer;
	m_clickcallback.Call(&info);
}

/* click and open in a new tab */
void kGUIHTMLPageObj::ClickNewTab(kGUIString *url,kGUIString *referrer)
{
	kGUIHTMLClickInfo info;

	/* do we have an attached visited url cache? */
	if(m_visitedcache)
		m_visitedcache->Add(url);

	info.m_newtab=true;
	info.m_post=0;
	info.m_url=url;
	info.m_referrer=referrer;
	m_clickcallback.Call(&info);
}

void kGUIHTMLPageObj::PushStyle(int numbytes,void *data)
{
	STYLERESTORE_DEF sr;

	sr.numbytes=numbytes;
	sr.place=data;
	m_srlist.SetEntry(m_srnum++,sr);
	m_srdata.Alloc(m_srdataindex+numbytes,true);	/* make sure there is enough space */
	memcpy(m_srdata.GetArrayPtr()+m_srdataindex,data,numbytes);
	m_srdataindex+=numbytes;
}

void kGUIHTMLPageObj::PopStyles(int index)
{
	STYLERESTORE_DEF sr;

	while(m_srnum!=index)
	{
		sr=m_srlist.GetEntry(--m_srnum);
		
		m_srdataindex-=sr.numbytes;
		memcpy(sr.place,m_srdata.GetArrayPtr()+m_srdataindex,sr.numbytes);
	}
}

/* expand the rules list into one that only has valid rules in it for each owner level */
void kGUIHTMLPageObj::BuildOwnerRules(void)
{
	unsigned int i;
	unsigned int owner;
	kGUIHTMLRule *rule;
	RO_DEF ro;

	ro.m_block=false;
	m_numownerrules=0;
	owner=OWNER_NUM;
	do
	{
		--owner;
		/* insert null rule pointer to flag the point to insert the tag styles */
		if((owner==OWNER_AUTHOR) || (owner==OWNER_AUTHORIMPORTANT))
		{
			ro.m_rule=0;
			ro.m_owner=owner;
			m_ownerrules.SetEntry(m_numownerrules++,ro);
		}

		for(i=0;i<m_numrules;++i)
		{
			rule=m_rules.GetEntry(i);

			/* does this rule have any attributes at this level? */
			if(rule->GetNumOwnerStyles(owner))
			{
				ro.m_rule=rule;
				ro.m_owner=owner;
				m_ownerrules.SetEntry(m_numownerrules++,ro);
			}
		}
	}while(owner);
}

/* apply the style rules applicable to this object */
void kGUIHTMLPageObj::ApplyStyleRules(kGUIHTMLObj *ho,unsigned int pseudotype)
{
	unsigned int i;
	bool apply=false;
	kGUIHTMLRule *rule;
	kGUIHTMLRuleCache *rulecache;
	RCE_DEF *rcep;
	unsigned int owner;
	unsigned int olddisplay;
//	unsigned int oldtextalign=m_textalign;
	kGUIStyleInfo si;

	ho->PreStyle(&si);
	m_beforecontent=0;
	m_aftercontent=0;

	//todo: reset bgx,bgy etc.

	/* put display to default for tag */
	olddisplay=ho->m_display;
	if(ho->m_tag)
	{
		ho->m_display=ho->m_tag->defdisp;
		if(m_trace)
		{
			unsigned int i;
			CID_DEF *cid;
//			bool found=false;

			kGUIString ts;
			
			ts.Sprintf("Apply Style Rules: <%s",ho->m_tag->name);

			cid=ho->m_cids.GetArrayPtr();
			for(i=0;i<ho->m_numcids;++i)
			{
				if(cid->m_type==CID_CLASS)
					ts.ASprintf(" class=\"%S\"",IDToString(cid->m_id));
				else
					ts.ASprintf(" id=\"%S\"",IDToString(cid->m_id));

#if 0
				//to trace a particular rule put a string compare here and break on it
				if(!stricmp(IDToString(cid->m_id)->GetString(),"Home_ArticleSummaryContainer"))
					found=true;
#endif
			}
			ts.Append(">");
			kGUI::Trace("Rule applied: %S\n",&ts);
		}
	}

	if(ho->m_insert==false)
	{
		/* is this a code added object? */
		++m_appliedlevel;

		if(m_trackpossiblerules)
		{
			AddPossibleRules(ho);
			m_poppossrules=true;
		}
		else
			m_poppossrules=false;

		/* block non-allowed styles */
		switch(ho->m_id)
		{
		case HTMLTAG_TD:
		case HTMLTAG_TR:
		case HTMLTAG_TH:
			//technically since these can be any tags we should so this type of blocking
			//in the poststyle
			m_applied[HTMLATT_MARGIN_LEFT]=m_appliedlevel;
			m_applied[HTMLATT_MARGIN_RIGHT]=m_appliedlevel;
			m_applied[HTMLATT_MARGIN_TOP]=m_appliedlevel;
			m_applied[HTMLATT_MARGIN_BOTTOM]=m_appliedlevel;

			m_applied[HTMLATT_PADDING_LEFT]=m_appliedlevel;
			m_applied[HTMLATT_PADDING_RIGHT]=m_appliedlevel;
			m_applied[HTMLATT_PADDING_TOP]=m_appliedlevel;
			m_applied[HTMLATT_PADDING_BOTTOM]=m_appliedlevel;
		break;
		}

		/* used mainly for user and viewer debugging */
		if( GetSettings()->GetUseCSS()==false)
		{
			ho->SetAttributes(ho,OWNER_AUTHORIMPORTANT,&si);
			ho->SetAttributes(ho,OWNER_AUTHOR,&si);
		}
		else
		{
			/* is this situation already in the cache? */

			rulecache=ho->m_rulecache;
			if(rulecache->GetIsValid())
			{
				/* yes this is in the cache */
				rcep=rulecache->GetRuleListPtr();

				for(i=0;i<rulecache->GetNum();++i)
				{
					rule=rcep->m_rule;
					owner=rcep->m_owner;

					/* null pointer is a flag for when to apply the tag's style */	
					if(!rule)
						ho->SetAttributes(ho,owner,&si);
					else
					{
						switch(rcep->m_status)
						{
						case RULE_TRUE:
							apply=true;
						break;
						case RULE_MAYBE:
							apply=rule->Evaluate(ho,m_appliedlevel);
						break;
						}

						if(apply==true)
						{
							if(m_trace)
							{
								kGUIString rs;

								rule->GetString(&rs);
								kGUI::Trace("Rule applied: %S\n",&rs);
							}
							ho->SetAttributes(rule,owner,&si);
						}
					}
					++rcep;
				}
			}
			else
			{
				/* we don't have this in the cache, so do a proper evaluation and then */
				/* add the results to the rule cache */
				unsigned int numtrue,nummaybe;
				unsigned int j;
				RCE_DEF rce;
				RO_DEF ro;
				bool block;

				if(m_trackpossiblerules==false)
				{
					/* this is a simple array with a index for each tag currently in the heiarchy */
					/* it is a quick way to know if a rule is possible or not */
					AddPossibleRules();
					m_trackpossiblerules=true;
				}

				numtrue=0;
				nummaybe=0;

				if(m_ownerrulesbuilt==false)
				{
					m_ownerrulesbuilt=true;

					/* build the m_ownerrules array from m_rules array and only include valid rules at each level */
					BuildOwnerRules();
				}

				/* each rule can have attributes from each owner level so we need to do multiple passes */
				/* todo: have a seperate rule list which has only rules with attributes from each valid level */

				for(i=0;i<m_numownerrules;++i)
				{
					ro=m_ownerrules.GetEntry(i);
					rule=ro.m_rule;
					owner=ro.m_owner;
					(m_ownerrules.GetEntryPtr(i))->m_block=false;

					if(!rule)
					{
						/* null means apply tag style now */
						ho->SetAttributes(ho,owner,&si);
						//since these can be diffferent for each instance of this tag these
						//are NOT to be applied to the block list!
						//SetTrueApplied(ho,owner);
						++numtrue;
						if(m_trace)
							kGUI::Trace("Applying inline style owner=%d\n",owner);
					}
					else if(rule->GetPseudo()==pseudotype)
					{
						if(rule->GetPossible()==true)
						{
							if(rule->GetSimple()==true && GetTrueBlocked(rule,owner))
							{
								(m_ownerrules.GetEntryPtr(i))->m_block=true;
#if 0
								if(m_trace)
								{
									kGUIString rs;

									rule->GetString(&rs);
									kGUI::Trace("Rule not applied (atts all blocked): %S\n",&rs);
								}
#endif
							}
							else if(rule->Evaluate(ho,m_appliedlevel)==true)
							{
								if(rule->GetSimple()==true)
								{
									++numtrue;
									SetTrueApplied(rule,owner);
								}
								else
									++nummaybe;
								if(m_trace)
								{
									kGUIString rs;

									rule->GetString(&rs);
									kGUI::Trace("Rule applied: %S\n",&rs);
								}
								ho->SetAttributes(rule,owner,&si);
							}
							else
							{
#if 0
								if(m_trace)
								{
									kGUIString rs;

									rule->GetString(&rs);
									kGUI::Trace("Rule not applied (evaluated): %S\n",&rs);
								}
#endif
								if((rule->GetSimple()==false) && (rule->GetHitComplex()==true))
									++nummaybe;
							}
						}
						else
						{
#if 0
							if(m_trace)
							{
								kGUIString rs;

								rule->GetString(&rs);
								kGUI::Trace("Rule not applied ( not possible): %S\n",&rs);
							}
#endif
						}
					}
				}

				/* since this was not in the cache, let's add it now */
				rulecache->Alloc(numtrue+nummaybe);
				rulecache->SetIsValid(true);
				j=0;
				for(i=0;i<m_numownerrules;++i)
				{
					ro=m_ownerrules.GetEntry(i);
					rule=ro.m_rule;
					owner=ro.m_owner;
					block=ro.m_block;

					if(!rule)
					{
						/* special case for when to apply tag's style */
						rce.m_rule=rule;
						rce.m_owner=owner;
						rce.m_status=RULE_TRUE;
						rulecache->SetEntry(j++,rce);
					}
					else if(rule->GetPseudo()==pseudotype)
					{
						if(rule->GetPossible()==true && block==false)
						{
							if(rule->GetSimple()==false)
							{
								/* this is a COMPLEX rule that can contain things like hover etc. */
								/* so we can't just dismiss it, but maybe we can? */
								/* the HitComplex flag is set during the last rule evaluation and is */
								/* true once the rule his a complex selector, so if it is false and the */
								/* rule was not valid then that means it failed on only Simple selectors */
								/* before it got to any complex ones, so we can dismiss it after all */

								if((rule->GetHitComplex()==true) || (rule->GetLast()==true))
								{
									rce.m_rule=rule;
									rce.m_owner=owner;
									rce.m_status=RULE_MAYBE;
									rulecache->SetEntry(j++,rce);
								}
							}
							else if(rule->GetLast()==true)
							{
								rce.m_rule=rule;
								rce.m_owner=owner;
								rce.m_status=RULE_TRUE;
								rulecache->SetEntry(j++,rce);
							}
						}
					}
				}
				assert((numtrue+nummaybe)==j,"Alloc count error!");
			}
		}

//		/* if this is a box object and is being floated then change it to an inline object */
//		if((ho->m_float!=FLOAT_NONE) && (ho->m_display==DISPLAY_BLOCK))
//			ho->m_display=DISPLAY_INLINE;

		/* if text-decoration mode changed then grab text-color and use for text-decoration color */
		if(m_copytdc)
		{
			m_copytdc=false;
			PushStyle(sizeof(m_textdecorationcolor),&m_textdecorationcolor);
			m_textdecorationcolor=m_fontcolor.GetColor();
		}
	}

	ho->PostStyle(&si);


#if 0
	/* special cases */
	if(ho->m_id!=HTMLTAG_SPAN)
	{
		//	if(ho->m_float!=FLOAT_NONE && (ho->m_display==DISPLAY_BLOCK || ho->m_display==DISPLAY_INLINE))
		if(ho->m_float!=FLOAT_NONE && (ho->m_display==DISPLAY_INLINE))
			ho->m_display=DISPLAY_INLINE_BLOCK;
		else if(ho->m_display==DISPLAY_INLINE)
		{
			/* defined width,height,left,right etc */
	//		if(ho->m_valign!=VALIGN_UNDEFINED)
	//			ho->m_display=DISPLAY_INLINE_BLOCK;
			if(ho->m_height.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_width.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_left.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_right.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_top.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_bottom.GetUnitType()!=UNITS_UNDEFINED)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_position!=POSITION_STATIC)
				ho->m_display=DISPLAY_INLINE_BLOCK;
			else if(ho->m_box!=0)
			{
				if(ho->m_box->GetValid())
					ho->m_display=DISPLAY_INLINE_BLOCK;
			}
		}
	}
#endif
	if(ho->m_display!=olddisplay)
		ho->ChangeDisplay(olddisplay);
}

/* the URL is only passed since nested @IMPORT commands are releative to each parents URL */

void kGUIHTMLPageObj::AddClassStyles(unsigned int baseowner,unsigned int priority,kGUIString *url,kGUIString *string)
{
	kGUIString name,value;
	char c,q;
	int j,l;
	kGUIStringSplit ss;
	int inmedia;
	int priorityafter=priority;
	bool validmedia;
	bool gotrule;
	bool error=false;
	kGUIString cstring;
	unsigned int nb;
	int ch;

	m_css.Append(string);

	/* is this valid ascii? Check for illegal characters */
	l=string->GetLen()-1;
	j=0;
	while(j<l)
	{
		ch=string->GetChar(j,&nb);
		if(ch<9)
			return;
		j+=nb;
	}

	/* change 'other' whitespace to spaces to simplify parsing */
	cstring.SetString(string);
	cstring.Replace("\n","");
	cstring.Replace("\r","");
	cstring.Replace("\t","");
	cstring.Trim();

	name.SetEncoding(cstring.GetEncoding());
	value.SetEncoding(cstring.GetEncoding());

	gotrule=false;
	inmedia=0;
	validmedia=true;
	j=0;
	l=cstring.GetLen();
	while(j<l)
	{
		/* get name */
		name.Clear();
		value.Clear();

		do
		{
again:		c=cstring.GetChar(j++);
			if(c=='/')
			{
				if(cstring.GetChar(j)=='*')
				{
					/* yes we found a comment */
					++j;
					do
					{
						if(cstring.GetChar(j)=='*' && cstring.GetChar(j+1)=='/')
						{
							j+=2;
							goto again;
						}
					}while(++j<l);
					return;				/* unclosed comment */
				}
			}

			if(c=='{' || c==';')
				break;
			else if(c=='}')
			{
				name.Trim();
				if(!name.GetLen())
				{
					if(inmedia)
					{
						--inmedia;
						validmedia=true;
						goto again;
					}
				}

			}
			else if( c==' ' && name.GetChar(0)=='@')
				break;
			else if( (c=='\"') && name.GetChar(0)=='@')
			{
				--j;
				break;
			}	
			name.Append(c);
			if(c=='@')
				name.Trim();	/* eat any leading whitespace */

		}while(j<l);
		name.Trim();

		if(name.GetLen())
		{
			if(name.GetChar(0)=='@')
			{
				name.Delete(0,1);
				switch(GetConstID(name.GetString()))
				{
				case HTMLCONST_CHARSET:	/* '@charset' */
					/* note: todo: don't override if defined in html header */
					/* also only valid if very first thing in the css file */

					/* get value */
					q=0;
					do
					{
						c=cstring.GetChar(j++);
						if(!q)
						{
							if(c=='\"' || c=='\'')
								q=c;
							else if(c==' ')
							{
								if(value.GetLen())
									break;
								else
									c=0;
							}
							else if(c==';')
								break;
						}
						else if(c==q)
							q=0;
						if(c)
							value.Append(c);
					}while(j<l);
	
					/* set encoding */
					value.Trim(TRIM_QUOTES);
					cstring.SetEncoding(kGUIString::GetEncoding(value.GetString()));
					name.SetEncoding(cstring.GetEncoding());
					value.SetEncoding(cstring.GetEncoding());
				break;
				case HTMLCONST_IMPORT:	/* '@import' */
				{
					kGUIString newurl;
					kGUIOnlineLink *link;
					kGUIString media;

					/* get value */
					q=0;
					do
					{
						c=cstring.GetChar(j++);
						if(!q)
						{
							if(c=='\"' || c=='\'')
								q=c;
							else if(c==' ')
							{
								if(value.GetLen())
									break;
								else
									c=0;
							}
							else if(c==';')
								break;
						}
						else if(c==q)
							q=0;
						if(c)
							value.Append(c);
					}while(j<l);

					/* is there media too? */
					if(j<l)
					{
						if(c!=';')
						{
							do
							{
								c=cstring.GetChar(j++);
								if(c==';')
									break;
								media.Append(c);
							}while(j<l);
						}
						media.Trim();
					}

					if(value.Replace("url(","",0,1))
						value.Replace(")","");

					MakeURL(url,&value,&newurl);

					if((gotrule==true) || (inmedia!=0))
					{
						/* error, all @imports must be before any rules are defined */
						/* or @import is inside an @media */
					}
					else
					{
						link=LocateLink(&newurl,&m_url,LINKTYPE_CSS,&media);
						priorityafter=GetStylePriority(priorityafter);
						link->SetPriority(priorityafter);

						if(link->GetLoadPending()==false)
							AttachLink(link);
					}
				}
				break;
				case HTMLCONST_MEDIA:	/* '@media' */

					/* get media strings up until '{' */
					do
					{
						c=cstring.GetChar(j++);
						if(c=='{')
							break;
						value.Append(c);
					}while(j<l);

					validmedia=ValidMedia(&value);
					++inmedia;
				break;
				case HTMLCONST_PAGE:	/* '@page' */
					/* todo: handle page attributes */
					/* for now we will just ignore it */
					validmedia=false;
					++inmedia;
					do
					{
						c=cstring.GetChar(j++);
						if(c=='{')
							break;
					}while(j<l);
				break;
				default:
				{
					int bracelevel=0;

					/* we don't understand this so perhaps it is a newer command?? */
					/* skip ahead until open brace */
					--j;
					do
					{
						c=cstring.GetChar(j++);
						if(c=='{')
							++bracelevel;
						else if(c=='}')
						{
							--bracelevel;
							if(!bracelevel)
								break;
						}
						else if(c==';')
							break;
					}while(j<l);
				}
				break;
				}
			}
			else
			{
				/* get value */
				q=0;
				do
				{
					c=cstring.GetChar(j++);
					if(!q)
					{
						if(c=='\"')
							q=c;
						if(c=='}')
							break;
					}
					else if(c==q)
						q=0;

					if(c=='\\')
					{
						value.Append(c);
						c=cstring.GetChar(j++);
					}
					value.Append(c);
				}while(j<l);

				if(validmedia)
				{
					if(!strstr(name.GetString(),","))
					{
						/* add name/value to style sheet settings */
						AddClassStyle(baseowner,priority,url,&name,&value);
					}
					else
					{
						unsigned int n;
						unsigned int numnames;
						bool allok;

						/* add style settings to multiple names */
						ss.SetIgnoreEmpty(false);
						numnames=ss.Split(&name,",");

						/* error if last chat is a comma */
						if(name.GetChar(name.GetLen()-1)==',')
							allok=false;
						else
							allok=true;
						for(n=0;n<numnames;++n)
						{
							kGUIHTMLRule *rule;
							kGUIString *rulename;

							rulename=ss.GetWord(n);
							if(!rulename->GetLen())
							{
								allok=false;
								break;
							}
							rule=LocateRule(rulename);
							if(!rule)
								allok=false;
						}

						if(allok==true)
						{
							for(n=0;n<numnames;++n)
							{
								/* add name/value to style sheet settings */
								AddClassStyle(baseowner,priority,url,ss.GetWord(n),&value);
							}
						}
					}
				}
				gotrule=true;
			}
		}
	}
}

kGUIHTMLRule::kGUIHTMLRule(kGUIHTMLPageObj *page)
{
	unsigned int i;

	m_page=page;
	m_lasttime=0;
	m_pseudo=PSEUDO_NONE;

	for(i=0;i<OWNER_NUM;++i)
		m_numownerstyles[i]=0;
}

/* count the number of styles assigned to each owner */
void kGUIHTMLRule::UpdateNumOwnerStyles(void)
{
	unsigned int i;
	unsigned int na;

	for(i=0;i<OWNER_NUM;++i)
		m_numownerstyles[i]=0;

	na=GetNumAttributes();
	for(i=0;i<na;++i)
		++m_numownerstyles[GetAttrib(i)->GetOwner()];
}

bool kGUIHTMLRule::ValidateName(kGUIString *s)
{
	unsigned int index;
	unsigned int l;
	unsigned int c,c2;
	unsigned int nb;

	/* todo: build a hash table of validated class / id names and add them to it */

	l=s->GetLen();
	assert(l>0,"Empty name should have been trapped earlier");

	c=s->GetChar(0,&nb);
	if(c=='-' && l>1)
	{
		/* make sure next char is not a number */
		c2=s->GetChar(1,&nb);
		if(c2>='0' && c2<='9')
			return(false);
	}

	/* > 0xa0 = ok! */
	if((c>='a' && c<='z') || (c>='A' && c<='Z') || c=='-' || c=='_' || c>=0xa0 || c=='\\')
	{
		index=nb;
		while(index<l)
		{
			c=s->GetChar(index,&nb);
			if(!( (c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='-' || c=='_' || c==':' || c=='.' || c>=0xa0 || c=='\\'))
				return(false);
			index+=nb;
		}
		return(true);
	}
	return(false);
}

bool kGUIHTMLRule::Parse(kGUIString *string)
{
	int c;
	unsigned int tokenid;
	kGUIReadString rs;
	kGUIString tag;
	kGUIHTMLSelector s;
	kGUIHTMLSelector s2;
	bool ignorewhite=false;
	bool hadwhite;
	bool trybefore;
	bool innot=false;

	/* simple is a rule that only checks tags & or class & or id */
	/* simple rules can be cached using a tree string that contains Tags/class/ids */
	/* complex rules need to be evaluated if "possible" ( ie: all refd tag/class/ids exist ) */
	m_simple=true;

	m_pseudo=PSEUDO_NONE;

	/* copy to readable string */
	rs.SetString(string);

	m_entries.Init(4,4);
	m_sorder.Init(4,4);		/* order for regenerating the string */
	m_numentries=0;
	m_addtcistart=0;
	s.m_not=false;

	do
	{
restart:;
		/* read white space */
		hadwhite=ReadString(&rs,&tag,false);
		if(ignorewhite)
		{
			hadwhite=false;
			ignorewhite=false;
		}

		if(tag.GetLen())
		{
			trybefore=false;
			c=tag.GetChar(0);
			switch(c)
			{
			case ')':	/* end of not? */
				if(innot==false)
					return(false);		/* error! */
				innot=false;
				ignorewhite=true;
				goto restart;
			break;
			case '.':	/* class */
				s.m_selector=CSSSELECTOR_CLASS;
				tag.Delete(0,1);

				if(!tag.GetLen())
					return(false);

				if(ValidateName(&tag)==false)
					return(false);

				m_page->FixCodes(&tag);
				tag.Replace("\\","");

				if(m_page->ClassUseCase())
					s.m_value=m_page->StringToIDcase(&tag);
				else
					s.m_value=m_page->StringToID(&tag);
				s.m_compare=0; /* not used, set so compiler stops complaining */
				m_page->ExpandClassList(s.m_value);
			break;
			case '#':	/* ID */
				s.m_selector=CSSSELECTOR_ID;
				tag.Delete(0,1);

				if(!tag.GetLen())
					return(false);

				if(ValidateName(&tag)==false)
					return(false);

				m_page->FixCodes(&tag);
				tag.Replace("\\","");

				if(m_page->ClassUseCase())
					s.m_value=m_page->StringToIDcase(&tag);
				else
					s.m_value=m_page->StringToID(&tag);
				s.m_compare=0; //not used, set so compiler stops complaining
				m_page->ExpandIDList(s.m_value);
			break;
			case '*':	/* Universal Selector */
				s.m_selector=CSSSELECTOR_UNIVERSAL;
				s.m_value=0; //not used, set so compiler stops complaining
				s.m_compare=0; //not used, set so compiler stops complaining
			break;
			case '>':	/* Child */
				s.m_selector=CSSSELECTOR_CHILD;
				s.m_value=0; //not used, set so compiler stops complaining
				s.m_compare=0; //not used, set so compiler stops complaining
				hadwhite=false;	/* ignore white space before and after */
				ignorewhite=true;
			break;
			case '~':	/* Sibling */
				s.m_selector=CSSSELECTOR_SIBLING;
				s.m_value=0; //not used, set so compiler stops complaining
				s.m_compare=0; //not used, set so compiler stops complaining
				hadwhite=false;	/* ignore white space before and after */
				ignorewhite=true;
				m_addtcistart=m_numentries+1;
			break;
			case '+':	/* Adjacent Sibling */
				s.m_selector=CSSSELECTOR_ADJACENT;
				s.m_value=0; //not used, set so compiler stops complaining
				s.m_compare=0; //not used, set so compiler stops complaining
				hadwhite=false;	/* ignore white space before and after */
				ignorewhite=true;
				m_addtcistart=m_numentries+1;
			break;
			case ':':
				tag.Delete(0,1);

				m_page->FixCodes(&tag);

				tokenid=m_page->GetConstID(tag.GetString());
				if(tokenid==HTMLCONST_NOT)
				{
					ReadString(&rs,&tag);
					if(strcmp(tag.GetString(),"("))
						return(false);	/* not a valid pseudo class tag */

					/* if white space was found before the item then insert a descendant selector */
					if(hadwhite)
					{
						s2.m_selector=CSSSELECTOR_DESCENDANT;
						s2.m_compare=0;	//not used, set so compiler stops complaining
						s2.m_value=0;	//not used, set so compiler stops complaining
						s2.m_not=false;	//not used, set so compiler stops complaining
						m_entries.SetEntry(m_numentries,s2);
						m_sorder.SetEntry(m_numentries,m_numentries);
						m_numentries++;
					}

					/* negate the next result */
					s.m_not=true;
					innot=true;		/* flag so we know to skip extra open brace */
					ignorewhite=true;
					goto restart;
				}

				/* pseudo class type */
				s.m_selector=GetPseudoClass(tokenid);
				if(!s.m_selector)
				{
					m_page->m_parseerrors.ASprintf("Unknown pseudo class tag '%s'\n",tag.GetString());
					return(false);	/* not a valid pseudo class tag */
				}
				s.m_value=0; //not used, set so compiler stops complaining
				s.m_compare=0; //not used, set so compiler stops complaining
				switch(s.m_selector)
				{
				case CSSSELECTOR_LANG:
					ReadString(&rs,&tag);	//(
					ReadString(&rs,&tag);
					s.m_value=m_page->StringToID(&tag);
					ReadString(&rs,&tag);	//)
				break;
				case CSSSELECTOR_BEFORE:
					if(m_pseudo==PSEUDO_NONE)
						m_pseudo=PSEUDO_BEFORE;
					else
					{
						m_page->m_errors.ASprintf("rule has more than one pseudo selector='%s'\n",tag.GetString());
						return(false);
					}
				break;
				case CSSSELECTOR_AFTER:
					if(m_pseudo==PSEUDO_NONE)
						m_pseudo=PSEUDO_AFTER;
					else
					{
						m_page->m_errors.ASprintf("rule has more than one pseudo selector='%s'\n",tag.GetString());
						return(false);
					}
				break;
				case CSSSELECTOR_NTHCHILD:
				case CSSSELECTOR_NTHLASTCHILD:
				case CSSSELECTOR_NTHOFTYPE:
				case CSSSELECTOR_NTHLASTOFTYPE:
				{
					kGUIString nn;

					ReadString(&rs,&tag);	//(
					if(strcmp(tag.GetString(),"("))
					{
						/* unknown comparator */
						m_page->m_errors.ASprintf("expected '(' n-th child found in rule, but found='/%s'\n",tag.GetString());
						return(false);
					}
					ReadString(&rs,&nn);
					do
					{
						ReadString(&rs,&tag);	//)
						if(!strcmp(tag.GetString(),")"))
							break;
						nn.Append(&tag);
					}while(1);
					
					switch(m_page->GetConstID(nn.GetString()))
					{
					case HTMLCONST_ODD:
						s.m_value=2;
						s.m_compare=1;
					break;
					case HTMLCONST_EVEN:
						s.m_value=2;
						s.m_compare=2;
					break;
					default:
					{
						unsigned int index=0;
						bool neg=false;

						if(nn.GetChar(0)=='-')
						{
							neg=true;
							++index;
						}
						if(nn.GetChar(index)=='n')
						{
							s.m_value=1;
							++index;
						}
						else
						{
							s.m_value=0;
							while(nn.GetChar(index)>='0' && nn.GetChar(index)<='9')
							{
								s.m_value=s.m_value*10+(nn.GetChar(index)-'0');
								if(++index==nn.GetLen())
									break;
							}
							if(nn.GetChar(index)=='n')
								++index;
						}
						if(neg)
							s.m_value=-s.m_value;
						s.m_compare=0;
						if(index<nn.GetLen())
						{
							neg=false;
							if(nn.GetChar(index)=='-')
							{
								neg=false;
								++index;
							}
							else if(nn.GetChar(index)=='+')
								++index;

							while(nn.GetChar(index)>='0' && nn.GetChar(index)<='9')
							{
								s.m_compare=s.m_compare*10+(nn.GetChar(index)-'0');
								if(++index==nn.GetLen())
									break;
							}
							if(neg)
								s.m_compare=-s.m_compare;
						}
					}
					break;
					}
				}
			}
			break;
			case '[':
				/* read name of comparator */
				ReadString(&rs,&tag);
				s.m_value=m_page->StringToID(&tag);
				ReadString(&rs,&tag);
				if(!strcmp(tag.GetString(),"]"))
				{
					s.m_selector=CSSSELECTOR_ATTEXISTS;
					s.m_compare=0;
				}
				else
				{
					kGUIString comparelist;

					if(!strcmp(tag.GetString(),"="))
						s.m_selector=CSSSELECTOR_ATTVALUE;
					else if(!strcmp(tag.GetString(),"~"))
					{
						ReadString(&rs,&tag);
						if(!strcmp(tag.GetString(),"="))
							s.m_selector=CSSSELECTOR_ATTLIST;
						else
							goto comperr;
					}
					else if(!strcmp(tag.GetString(),"|"))
					{
						ReadString(&rs,&tag);
						if(!strcmp(tag.GetString(),"="))
							s.m_selector=CSSSELECTOR_ATTHYPHENLIST;
						else
							goto comperr;
					}
					else if(!strcmp(tag.GetString(),"^"))
					{
						ReadString(&rs,&tag);
						if(!strcmp(tag.GetString(),"="))
							s.m_selector=CSSSELECTOR_ATTBEGINVALUE;
						else
							goto comperr;
					}
					else if(!strcmp(tag.GetString(),"*"))
					{
						ReadString(&rs,&tag);
						if(!strcmp(tag.GetString(),"="))
							s.m_selector=CSSSELECTOR_ATTCONTAINSVALUE;
						else
							goto comperr;
					}
					else if(!strcmp(tag.GetString(),"$"))
					{
						ReadString(&rs,&tag);
						if(!strcmp(tag.GetString(),"="))
							s.m_selector=CSSSELECTOR_ATTENDVALUE;
						else
							goto comperr;
					}
					else
					{
comperr:;
						/* unknown comparator */
						m_page->m_errors.ASprintf("unknown comparator found in rule='/%s'\n",tag.GetString());
						return(false);
					}

					/* ok, so we got a attribute and a comparator, so now we get a value(s) */
					do
					{
						ReadString(&rs,&tag);
						if(!tag.GetLen())
						{
							/* missing close bracket */
							m_page->m_errors.ASprintf("missing close bracket='/%s'\n",tag.GetString());
							return(false);
						}
						if(!strcmp(tag.GetString(),"]"))
							break;

						tag.RemoveQuotes();	/* if it has any */
						if(comparelist.GetLen())
							comparelist.Append(" ");
						comparelist.Append(&tag);
					}while(1);

					/* convert string list into an ID */
					s.m_compare=m_page->StringToID(&comparelist);
				}
			break;
			default:
			{
				TAGLIST_DEF **tagptr;
				TAGLIST_DEF *tl;

				m_page->FixCodes(&tag);

				tagptr=m_page->FindTag(&tag);
				if(tagptr)
				{
					tl=*(tagptr);
					s.m_selector=CSSSELECTOR_TAG;
					s.m_value=tl->tokenid;
					s.m_compare=0; //not used, set so compiler stops complaining
				}
				else
				{
					m_page->m_errors.ASprintf("unknown tag name found in rule='%s'\n",tag.GetString());
					return(false);
				}
			}
			break;
			}

			/* if white space was found before the item then insert a descendant selector */
			if(hadwhite)
			{
				s2.m_selector=CSSSELECTOR_DESCENDANT;
				s2.m_compare=0;	//not used, set so compiler stops complaining
				s2.m_value=0;	//not used, set so compiler stops complaining
				s2.m_not=false;	//not used, set so compiler stops complaining
				m_entries.SetEntry(m_numentries,s2);
				m_sorder.SetEntry(m_numentries,m_numentries);
				m_numentries++;
			}

			/* re-order these to reduce the number of possible hover matches */
			if(trybefore)
			{
				/* can we insert this before a ID or CLASS or TAG?, if so move it back */

				kGUIHTMLSelector *ps;
				unsigned int p=m_numentries;
				bool goback=true;

				do
				{
					if(!p)
						break;
					ps=m_entries.GetEntryPtr(p-1);
		
					switch(ps->m_selector)
					{
					case CSSSELECTOR_CLASS:
					case CSSSELECTOR_ID:
					case CSSSELECTOR_TAG:
						--p;
					break;
					default:
						goback=false;
					break;
					}
				}while(goback);
				if(p<m_numentries)
				{
					unsigned int x;
					unsigned int o;
					m_entries.InsertEntry(m_numentries,p,1);

					for(x=0;x<m_numentries;++x)
					{
						o=m_sorder.GetEntry(x);
						if(o>=p)
							m_sorder.SetEntry(x,o+1);
					}
				}
				m_entries.SetEntry(p,s);
				m_sorder.SetEntry(m_numentries,p);
				m_numentries++;
			}
			else
			{
				m_entries.SetEntry(m_numentries,s);
				m_sorder.SetEntry(m_numentries,m_numentries);
				m_numentries++;
			}
			s.m_not=false;
		}
	}while(rs.AtEnd()==false);
	return(true);
}

int kGUIHTMLRule::Compare(kGUIHTMLRule *r2)
{
	int delta;

	/* num ids */
	delta=m_numids-r2->m_numids;
	if(delta)
		return(delta);

	/* num classes or psuedoclasses */
	delta=m_numclasses-r2->m_numclasses;
	if(delta)
		return(delta);

	/* num tags */
	delta=m_numtags-r2->m_numtags;
	return(delta);
}

void kGUIHTMLRule::CalcScore(void)
{
	unsigned int i,ni,nc;
	kGUIHTMLSelector *sp;
	bool tagrefs[HTMLTAG_NUMTAGS];
	Array<bool> classrefs;
	Array<bool> idrefs;

	/* score variables */
	m_numids=0;
	m_numclasses=0;
	m_numtags=0;

	/* build the tag refs table too */
	m_possible=false;
	m_numrefs=0;
	m_numcurrefs=0;
	for(i=0;i<HTMLTAG_NUMTAGS;++i)
		tagrefs[i]=false;

	nc=m_page->GetNumClasses();
	classrefs.Alloc(nc);
	for(i=0;i<nc;++i)
		classrefs.SetEntry(i,false);

	ni=m_page->GetNumIDs();
	idrefs.Alloc(ni);
	for(i=0;i<ni;++i)
		idrefs.SetEntry(i,false);

	/* calc score for this rule */
	for(i=0;i<m_numentries;++i)
	{
		sp=m_entries.GetEntryPtr(i);
		switch(sp->m_selector)
		{
		case CSSSELECTOR_ADJACENT:
		case CSSSELECTOR_SIBLING:
			m_simple=false;
		break;
		case CSSSELECTOR_UNIVERSAL:
		case CSSSELECTOR_DESCENDANT:
		case CSSSELECTOR_CHILD:
		case CSSSELECTOR_BEFORE:
		case CSSSELECTOR_AFTER:
			/* zero! */
		break;
		case CSSSELECTOR_TAG:
			if(i>=m_addtcistart && sp->m_not==false)
			{
				if(tagrefs[sp->m_value]==false)
				{
					++m_numrefs;
					tagrefs[sp->m_value]=true;
					
					/* add me to the list of rules that use this TAG */
					m_page->AttachRuleToTag(sp->m_value,this);
				}
			}
			++m_numtags;
		break;
		case CSSSELECTOR_ID:
			if(i>=m_addtcistart && sp->m_not==false)
			{
				if(idrefs.GetEntry(sp->m_value)==false)
				{
					++m_numrefs;
					idrefs.SetEntry(sp->m_value,true);
					
					/* add me to the list of rules that use this id */
					m_page->AttachRuleToID(sp->m_value,this);
				}
			}
			++m_numids;
		break;
		case CSSSELECTOR_CLASS:
			if(i>=m_addtcistart && sp->m_not==false)
			{
				if(classrefs.GetEntry(sp->m_value)==false)
				{
					++m_numrefs;
					classrefs.SetEntry(sp->m_value,true);
					
					/* add me to the list of rules that use this class */
					m_page->AttachRuleToClass(sp->m_value,this);
				}
			}
			++m_numclasses;
		break;
		case CSSSELECTOR_ATTEXISTS:
		case CSSSELECTOR_ATTVALUE:
		case CSSSELECTOR_ATTLIST:
		case CSSSELECTOR_ATTHYPHENLIST:
		case CSSSELECTOR_ATTBEGINVALUE:
		case CSSSELECTOR_ATTCONTAINSVALUE:
		case CSSSELECTOR_ATTENDVALUE:
			++m_numclasses;
			if(sp->m_value!=HTMLATT_CLASS && sp->m_value!=HTMLATT_ID)
				m_simple=false;
		break;
		case CSSSELECTOR_LANG:
		case CSSSELECTOR_FIRSTCHILD:
		case CSSSELECTOR_LASTCHILD:
		case CSSSELECTOR_NTHCHILD:
		case CSSSELECTOR_NTHLASTCHILD:
		case CSSSELECTOR_NTHOFTYPE:
		case CSSSELECTOR_NTHLASTOFTYPE:
		case CSSSELECTOR_ONLYCHILD:
		case CSSSELECTOR_FIRSTOFTYPE:
		case CSSSELECTOR_LASTOFTYPE:
		case CSSSELECTOR_ONLYOFTYPE:
		case CSSSELECTOR_EMPTY:
		case CSSSELECTOR_LINK:
		case CSSSELECTOR_VISITED:
		case CSSSELECTOR_ACTIVE:
		case CSSSELECTOR_HOVER:
		case CSSSELECTOR_FOCUS:
		case CSSSELECTOR_ENABLED:
		case CSSSELECTOR_DISABLED:
		case CSSSELECTOR_CHECKED:
			m_simple=false;
			++m_numclasses;
		break;
		case CSSSELECTOR_TARGET:
		case CSSSELECTOR_ROOT:
			++m_numclasses;
		break;
		default:
			assert(false,"Unscored thingy");
		break;
        }
	}
}

/* read a keyword from the current position in the readstring 'rs' and return */
/* the keyword in the string 's' */

bool kGUIHTMLRule::ReadString(kGUIReadString *rs,kGUIString *s,bool fixcodes)
{
	unsigned int c,q;
	bool done;
	bool hadwhite=false;

	s->Clear();
	s->SetEncoding(rs->GetEncoding());

	/* skip white space */
	done=false;
	do
	{
		c=rs->PeekChar();
		switch(c)
		{
		case ' ':
			hadwhite=true;
		case '\n':
		case '\r':
		case '\t':
			rs->ReadChar();
		break;
		default:
			done=true;
		break;
		}
	}while((rs->AtEnd()==false) && (done==false));

	/* read a string */
	q=0;
	do
	{
		c=rs->PeekChar();
		/* handle quotes first */
		if(q && c==q)
			q=0;
		else if(!q && (c=='\"' || c=='\''))
			q=c;
		if(!q)
		{
			switch(c)
			{
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				goto done;
			break;
			case '(':
			case ')':
			case '[':
			case ']':
			case '<':
			case '>':
			case '+':
			case '*':
			case '=':
			case '~':
			case '|':
			case '^':
			case '$':
				/* these are only valid as single character strings */
				if(!s->GetLen())
				{
					s->Append(c);
					rs->ReadChar();
				}
				goto done;
			break;
			case '\\':
				s->Append(c);
				rs->ReadChar();
				c=rs->PeekChar();
			break;
			case ':':
			case '#':
			case '.':
				/* these are only valid at the beginning of a string, stop here if already in a string */
				if(s->GetLen())
					goto done;
			break;
			}
		}
		s->Append(c);
		rs->ReadChar();
	}while(rs->AtEnd()==false);
done:;
	if(fixcodes)
		m_page->FixCodes(s);
	return(hadwhite);
}

/* return pseudo class id along with type that this pseudo class can be attached to */
/* true=ok, false=error */

unsigned int kGUIHTMLRule::GetPseudoClass(unsigned int tokenid)
{
	switch(tokenid)
	{
	case HTMLCONST_LINK:
		return(CSSSELECTOR_LINK);
	break;
	case HTMLCONST_VISITED:
		return(CSSSELECTOR_VISITED);
	break;
	case HTMLCONST_ACTIVE:
		return(CSSSELECTOR_ACTIVE);
	break;
	case HTMLCONST_HOVER:
		return(CSSSELECTOR_HOVER);
	break;
	case HTMLCONST_FOCUS:
		return(CSSSELECTOR_FOCUS);
	break;
	case HTMLCONST_LASTCHILD:
		return(CSSSELECTOR_LASTCHILD);
	break;
	case HTMLCONST_NTHCHILD:
		return(CSSSELECTOR_NTHCHILD);
	break;
	case HTMLCONST_NTHLASTCHILD:
		return(CSSSELECTOR_NTHLASTCHILD);
	break;
	case HTMLCONST_NTHOFTYPE:
		return(CSSSELECTOR_NTHOFTYPE);
	break;
	case HTMLCONST_NTHLASTOFTYPE:
		return(CSSSELECTOR_NTHLASTOFTYPE);
	break;
	case HTMLCONST_ONLYCHILD:
		return(CSSSELECTOR_ONLYCHILD);
	break;
	case HTMLCONST_FIRSTOFTYPE:
		return(CSSSELECTOR_FIRSTOFTYPE);
	break;
	case HTMLCONST_LASTOFTYPE:
		return(CSSSELECTOR_LASTOFTYPE);
	break;
	case HTMLCONST_ONLYOFTYPE:
		return(CSSSELECTOR_ONLYOFTYPE);
	break;
	case HTMLCONST_EMPTY:
		return(CSSSELECTOR_EMPTY);
	break;
	case HTMLCONST_FIRSTCHILD:
		return(CSSSELECTOR_FIRSTCHILD);
	break;
	case HTMLCONST_BEFORE:
		return(CSSSELECTOR_BEFORE);
	break;
	case HTMLCONST_AFTER:
		return(CSSSELECTOR_AFTER);
	break;
	case HTMLCONST_ROOT:
		return(CSSSELECTOR_ROOT);
	break;
	case HTMLCONST_LANG:
		return(CSSSELECTOR_LANG);
	break;
	case HTMLCONST_TARGET:
		return(CSSSELECTOR_TARGET);
	break;
	case HTMLCONST_ENABLED:
		return(CSSSELECTOR_ENABLED);
	break;
	case HTMLCONST_DISABLED:
		return(CSSSELECTOR_DISABLED);
	break;
	case HTMLCONST_CHECKED:
		return(CSSSELECTOR_CHECKED);
	break;
	}
	return(0);
}

/* this is used to check and see if a just defined rule already exists */
/* if it is, then it is deleted and the previous one is used */

void kGUIHTMLRule::GetString(kGUIString *s)
{
	unsigned int i;
	kGUIHTMLSelector *sp;

	/* clear it just incase it already has stuff in it */
	s->Clear();

	/* since the entries have been re-ordered for optimization we use the */
	/* sorder array for printing as that is the source order */

	for(i=0;i<m_numentries;++i)
	{
		sp=m_entries.GetEntryPtr(m_sorder.GetEntry(i));
		/* print tag.class or tag#id or :xxxx [expr] */
	
		if(sp->m_not)
			s->Append(":not(");

		switch(sp->m_selector)
		{
		case CSSSELECTOR_TAG:
		{
			assert(sp->m_value<(sizeof(kGUIHTMLPageObj::m_taglist)/sizeof(TAGLIST_DEF)),"TagID is off of end of list!");

			s->Append(kGUIHTMLPageObj::m_taglist[sp->m_value].name);
		}
		break;
		case CSSSELECTOR_ATTEXISTS:
			s->ASprintf("[%s]",m_page->IDToString(sp->m_value)->GetString());
		break;
		case CSSSELECTOR_ATTVALUE:
			s->ASprintf("[%s=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_ATTLIST:
			s->ASprintf("[%s~=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_ATTHYPHENLIST:
			s->ASprintf("[%s|=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_ATTBEGINVALUE:
			s->ASprintf("[%s^=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_ATTCONTAINSVALUE:
			s->ASprintf("[%s*=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_ATTENDVALUE:
			s->ASprintf("[%s$=%s]",m_page->IDToString(sp->m_value)->GetString(),m_page->IDToString(sp->m_compare)->GetString());
		break;
		case CSSSELECTOR_UNIVERSAL:
			s->Append("*");
		break;
		case CSSSELECTOR_DESCENDANT:
			s->Append(" ");
		break;
		case CSSSELECTOR_CHILD:
			s->Append(">");
		break;
		case CSSSELECTOR_SIBLING:
			s->Append("~");
		break;
		case CSSSELECTOR_ADJACENT:
			s->Append("+");
		break;
		case CSSSELECTOR_CLASS:
			s->Append(".");
			s->Append(m_page->IDToString(sp->m_value));
		break;
		case CSSSELECTOR_ID:
			s->Append("#");
			s->Append(m_page->IDToString(sp->m_value));
		break;
		case CSSSELECTOR_VISITED:
			s->Append(":visited");
		break;
		case CSSSELECTOR_ACTIVE:
			s->Append(":active");
		break;
		case CSSSELECTOR_HOVER:
			s->Append(":hover");
		break;
		case CSSSELECTOR_FOCUS:
			s->Append(":focus");
		break;
		case CSSSELECTOR_BEFORE:
			s->Append(":before");
		break;
		case CSSSELECTOR_AFTER:
			s->Append(":after");
		break;
		case CSSSELECTOR_ROOT:
			s->Append(":root");
		break;
		case CSSSELECTOR_LINK:
			s->Append(":link");
		break;
		case CSSSELECTOR_FIRSTCHILD:
			s->Append(":first-child");
		break;
		case CSSSELECTOR_LASTCHILD:
			s->Append(":last-child");
		break;
		case CSSSELECTOR_NTHCHILD:
			if(sp->m_value==2 && sp->m_compare==1)
				s->Append(":nth-child(odd)");
			else if(sp->m_value==2 && sp->m_compare==2)
				s->Append(":nth-child(even)");
			else
			{
				if(!sp->m_compare)
					s->ASprintf(":nth-child(%dn)",sp->m_value);
				else if(sp->m_compare<0)
					s->ASprintf(":nth-child(%dn%d)",sp->m_value,sp->m_compare);
				else
					s->ASprintf(":nth-child(%dn+%d)",sp->m_value,sp->m_compare);
			}
		break;
		case CSSSELECTOR_NTHLASTCHILD:
			if(sp->m_value==2 && sp->m_compare==1)
				s->Append(":nth-last-child(odd)");
			else if(sp->m_value==2 && sp->m_compare==2)
				s->Append(":nth-last-child(even)");
			else
			{
				if(!sp->m_compare)
					s->ASprintf(":nth-last-child(%dn)",sp->m_value);
				else if(sp->m_compare<0)
					s->ASprintf(":nth-last-child(%dn%d)",sp->m_value,sp->m_compare);
				else
					s->ASprintf(":nth-last-child(%dn+%d)",sp->m_value,sp->m_compare);
			}
		break;
		case CSSSELECTOR_NTHOFTYPE:
			if(sp->m_value==2 && sp->m_compare==1)
				s->Append(":nth-of-type(odd)");
			else if(sp->m_value==2 && sp->m_compare==2)
				s->Append(":nth-of-type(even)");
			else
			{
				if(!sp->m_compare)
					s->ASprintf(":nth-of-type(%dn)",sp->m_value);
				else if(sp->m_compare<0)
					s->ASprintf(":nth-of-type(%dn%d)",sp->m_value,sp->m_compare);
				else
					s->ASprintf(":nth-of-type(%dn+%d)",sp->m_value,sp->m_compare);
			}
		break;
		case CSSSELECTOR_NTHLASTOFTYPE:
			if(sp->m_value==2 && sp->m_compare==1)
				s->Append(":nth-last-of-type(odd)");
			else if(sp->m_value==2 && sp->m_compare==2)
				s->Append(":nth-last-of-type(even)");
			else
			{
				if(!sp->m_compare)
					s->ASprintf(":nth-last-of-type(%dn)",sp->m_value);
				else if(sp->m_compare<0)
					s->ASprintf(":nth-last-of-type(%dn%d)",sp->m_value,sp->m_compare);
				else
					s->ASprintf(":nth-last-of-type(%dn+%d)",sp->m_value,sp->m_compare);
			}
		break;
		case CSSSELECTOR_ONLYCHILD:
			s->Append(":only-child");
		break;
		case CSSSELECTOR_FIRSTOFTYPE:
			s->Append(":first-of-type");
		break;
		case CSSSELECTOR_LASTOFTYPE:
			s->Append(":last-of-type");
		break;
		case CSSSELECTOR_ONLYOFTYPE:
			s->Append(":only-of-type");
		break;
		case CSSSELECTOR_FIRSTLINE:
			s->Append(":first-line");
		break;
		case CSSSELECTOR_FIRSTLETTER:
			s->Append(":first-letter");
		break;
		case CSSSELECTOR_EMPTY:
			s->Append(":empty");
		break;
		case CSSSELECTOR_LANG:
			s->ASprintf(":lang(%s)",m_page->IDToString(sp->m_value)->GetString());
		break;
		case CSSSELECTOR_TARGET:
			s->Append(":target");
		break;
		case CSSSELECTOR_ENABLED:
			s->Append(":enabled");
		break;
		case CSSSELECTOR_DISABLED:
			s->Append(":disabled");
		break;
		case CSSSELECTOR_CHECKED:
			s->Append(":checked");
		break;
		default:
			assert(false,"Unhandled selector");
		break;
		}
		if(sp->m_not)
			s->Append(")");
	}
}

/* it can then recursively call itself for descenders etc. */

bool kGUIHTMLRule::Evaluate(int sindex,kGUIHTMLObj *ho)
{
	kGUIHTMLSelector *sel;
	bool res;

	while(sindex>=0)
	{
		sel=m_entries.GetEntryPtr(sindex--);
		res=true;
		switch(sel->m_selector)
		{
		case CSSSELECTOR_TAG:
			if(ho->m_id!=sel->m_value)
				res=false;
		break;
		case CSSSELECTOR_UNIVERSAL:
		case CSSSELECTOR_BEFORE:
		case CSSSELECTOR_AFTER:
			/* do nothing */
		break;
		case CSSSELECTOR_DESCENDANT:
		{
			kGUIHTMLObj *hop=ho->m_styleparent;

			/* cannot go deeper than HTML */
			if(ho->m_id==HTMLTAG_HTML)
				res=false;
			else
			{
				/* loop through the parents until we find one that matches */
				while(hop)
				{
					if(Evaluate(sindex,hop)==true)
					{
						/* ?? do we need to check for NOT??? */
						return(true);
					}
					hop=hop->m_styleparent;
				}
				res=false;	/* no parents match! */
			}
		}
		break;
		case CSSSELECTOR_CHILD:
		{
			kGUIHTMLObj *hop=ho->m_styleparent;
			if(hop)
			{
				if(Evaluate(sindex,hop)==true)
					return(true);
			}
			res=false;	/* no match! */
		}
		break;
		case CSSSELECTOR_SIBLING:
		{
			unsigned int i;
			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			/* I think this should only match previous siblings */

			m_hitcomplex=true;
			if(hop)
			{
				for(i=0;i<hop->m_numstylechildren;++i)
				{
					sib=hop->m_stylechildren.GetEntry(i);
					if(sib==ho)
						break;
					if(sib->GetID()!=HTMLTAG_IMBEDTEXTGROUP)
					{
						/* if no more entries to check then OK! */
						if(sindex<0)
							return(true);
						/* try this sibling */
						if(Evaluate(sindex,sib)==true)
							return(true);
					}
				}
			}
			res=false;	/* no match! */
		}
		break;
		case CSSSELECTOR_ADJACENT:
		{
			unsigned int i;
			unsigned int s;
			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(hop)
			{
				/* calc my index into my parents child list */
				for(i=0;i<hop->m_numstylechildren;++i)
				{
					if(hop->m_stylechildren.GetEntry(i)==ho)
						break;
				}
				/* find index to prev entry, skipping embeded textgroups */
				s=i;
				do
				{
					if(!s)
					{
						sib=0;
						break;
					}
					sib=hop->m_stylechildren.GetEntry(--s);
				}while(sib->GetID()==HTMLTAG_IMBEDTEXTGROUP);
				if(sib)
				{
					/* if no more entries to check then OK! */
					if(sindex<0)
						return(true);

					/* try my previous sibling first */
					if(Evaluate(sindex,sib)==true)
						return(true);
				}
			}
			res=false;	/* no match! */
		}
		break;
		case CSSSELECTOR_FIRSTCHILD:
		{
			unsigned int e;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *child;

			m_hitcomplex=true;
			/* don't count inserted content ( text is inserted too ) */
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				e=0;
				do
				{
					child=hop->m_stylechildren.GetEntry(e++);
				}while( child->m_insert==true && e<hop->m_numstylechildren);

				if(child!=ho)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_LASTCHILD:
		{
			unsigned int index;
			bool found=false;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				index=hop->m_numstylechildren-1;
				while(index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==HTMLTAG_IMBEDTEXTGROUP)
					{
						if(!index)
							break;
						--index;
					}
					else
					{
						if(sib==ho)
							found=true;
						break;
					}
				}
			}
			if(!found)
				res=false;
		}
		break;
		case CSSSELECTOR_NTHCHILD:
		{
			unsigned int index;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				for(index=0;index<hop->m_numstylechildren;++index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()!=HTMLTAG_IMBEDTEXTGROUP)
					{
						if(sib==ho)
						{
							if(!sel->m_compare)
							{
								if(sel->m_value!=rem)
									res=false;
							}
							else if(sel->m_value<0)
							{
								if(rem>sel->m_compare)
									res=false;
							}
							else
							{
								if(sel->m_compare!=rem)
									res=false;
							}
							break;
						}
						else
						{
							if(sel->m_value<0 || !sel->m_compare)
								++rem;
							else
							{
								if(++rem>sel->m_value)
									rem=1;
							}
						}
					}
				}
			}
		}
		break;
		case CSSSELECTOR_NTHLASTCHILD:
		{
			unsigned int index;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				index=hop->m_numstylechildren;
				while(index)
				{
					--index;
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()!=HTMLTAG_IMBEDTEXTGROUP)
					{
						if(sib==ho)
						{
							if(!sel->m_compare)
							{
								if(sel->m_value!=rem)
									res=false;
							}
							else if(sel->m_value<0)
							{
								if(rem>sel->m_compare)
									res=false;
							}
							else
							{
								if(sel->m_compare!=rem)
									res=false;
							}
							break;
						}
						else
						{
							if(sel->m_value<0 || !sel->m_compare)
								++rem;
							else
							{
								if(++rem>sel->m_value)
									rem=1;
							}
						}
					}
				}
			}
		}
		break;
		case CSSSELECTOR_NTHOFTYPE:
		{
			unsigned int index;
			unsigned int type=ho->m_id;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				for(index=0;index<hop->m_numstylechildren;++index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==type)
					{
						if(sib==ho)
						{
							if(!sel->m_compare)
							{
								if(sel->m_value!=rem)
									res=false;
							}
							else if(sel->m_value<0)
							{
								if(rem>sel->m_compare)
									res=false;
							}
							else
							{
								if(sel->m_compare!=rem)
									res=false;
							}
							break;
						}
						else
						{
							if(sel->m_value<0 || !sel->m_compare)
								++rem;
							else
							{
								if(++rem>sel->m_value)
									rem=1;
							}
						}
					}
				}
			}
		}
		break;
		case CSSSELECTOR_NTHLASTOFTYPE:
		{
			unsigned int index;
			unsigned int type=ho->m_id;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				index=hop->m_numstylechildren;
				while(index)
				{
					--index;
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==type)
					{
						if(sib==ho)
						{
							if(!sel->m_compare)
							{
								if(sel->m_value!=rem)
									res=false;
							}
							else if(sel->m_value<0)
							{
								if(rem>sel->m_compare)
									res=false;
							}
							else
							{
								if(sel->m_compare!=rem)
									res=false;
							}
							break;
						}
						else
						{
							if(sel->m_value<0 || !sel->m_compare)
								++rem;
							else
							{
								if(++rem>sel->m_value)
									rem=1;
							}
						}
					}
				}
			}
		}
		break;
		case CSSSELECTOR_ONLYCHILD:
		{
			unsigned int index;
			unsigned int nc;
			bool found=false;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				nc=hop->m_numstylechildren;
				for(index=0;index<nc;++index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()!=HTMLTAG_IMBEDTEXTGROUP)
					{
						if(sib==ho)
							found=true;
						else
							found=false;
					}
				}
				if(!found)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_FIRSTOFTYPE:
		{
			unsigned int index;
			unsigned int type=ho->m_id;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				for(index=0;index<hop->m_numstylechildren;++index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==type)
					{
						if(sib==ho)
						{
							res=(rem==1);
							break;
						}
						else
							++rem;
					}
				}
			}
		}
		break;
		case CSSSELECTOR_LASTOFTYPE:
		{
			unsigned int index;
			unsigned int type=ho->m_id;
			int rem;

			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=1;
				index=hop->m_numstylechildren;
				while(index)
				{
					--index;
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==type)
					{
						if(sib==ho)
						{
							res=(rem==1);
							break;
						}
						else
							++rem;
					}
				}
			}
		}
		break;
		case CSSSELECTOR_ONLYOFTYPE:
		{
			unsigned int index;
			unsigned int type=ho->m_id;
			int rem;
			kGUIHTMLObj *hop=ho->m_styleparent;
			kGUIHTMLObj *sib;

			m_hitcomplex=true;
			if(ho->m_id==HTMLTAG_HTML || !hop)
				res=false;
			else
			{
				rem=0;
				for(index=0;index<hop->m_numstylechildren;++index)
				{
					sib=hop->m_stylechildren.GetEntry(index);
					if(sib->GetID()==type)
						++rem;
				}
				res=(rem==1);
			}
		}
		break;
		case CSSSELECTOR_CLASS:
		{
			unsigned int i;
			bool found;
			CID_DEF *cid;

			/* since a class string can be space seperated list of class names, */
			/* we pre-split them into individual strings and convert them */
			/* each to a unique number so we can just do a quick compare */

			found=false;
			cid=ho->m_cids.GetArrayPtr();
			for(i=0;(i<ho->m_numcids) && (found==false);++i)
			{
				if(cid->m_type==CID_CLASS && cid->m_id==sel->m_value)
					found=true;
				++cid;
			}
			if(found==false)
				res=false;
		}
		break;
		case CSSSELECTOR_ID:
		{
			unsigned int i;
			bool found;
			CID_DEF *cid;

			/* the ID string was converted to a number and added to the array below */

			found=false;
			cid=ho->m_cids.GetArrayPtr();
			for(i=0;(i<ho->m_numcids) && (found==false);++i)
			{
				if(cid->m_type==CID_ID && cid->m_id==sel->m_value)
					found=true;
				++cid;
			}
			if(found==false)
				res=false;
		}
		break;
		case CSSSELECTOR_EMPTY:
			m_hitcomplex=true;
			if(ho->GetNumStyleChildren())
				res=false;
		break;
		case CSSSELECTOR_ATTEXISTS:
			m_hitcomplex=true;
			if(!ho->FindAttrib(sel->m_value))
				res=false;
		break;
		case CSSSELECTOR_ATTVALUE:
		{
			kGUIHTMLAttrib *att;

			m_hitcomplex=true;
			att=ho->FindAttrib(sel->m_value);
			if(!att)
				res=false;
			else
			{
				/* should this be case sensative or not?? */
				if(m_page->StringToID(att->GetValue())!=sel->m_compare)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_ATTLIST:
		{
			kGUIHTMLAttrib *att;
			const char *compare;
			int cl;
			const char *clist;
			bool found;
			const char *ew;

			m_hitcomplex=true;
			att=ho->FindAttrib(sel->m_value);
			if(!att)
				res=false;
			else
			{
				/* we are looking for this string */
				compare=m_page->IDToString(sel->m_compare)->GetString();
				cl=(int)strlen(compare);
				/* in this space seperated list */
				clist=att->GetValue()->GetString();
				found=false;
				do
				{
					ew=strstr(clist," ");
					if(!ew)
						ew=clist+strlen(clist);

					if(cl==(int)(ew-clist))
					{
						/* todo: check if should be case sensitive or not? */
						if(!strncmp(clist,compare,ew-clist))
						{
							found=true;
							break;
						}
					}
					clist=ew+1;
				}while(ew[0]);
				if(found==false)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_LANG:
			m_hitcomplex=true;
			if(strnicmp(m_page->IDToString(m_page->GetLangID())->GetString(),m_page->IDToString(sel->m_value)->GetString(),m_page->IDToString(sel->m_value)->GetLen()))
				res=false;
		break;
		case CSSSELECTOR_ATTHYPHENLIST:
		{
			kGUIHTMLAttrib *att;
			const char *compare;
			const char *clist;
			bool found;
			const char *ew;
			const char *hs;	/* hyphen spot */

			m_hitcomplex=true;
			att=ho->FindAttrib(sel->m_value);
			if(!att)
				res=false;
			else
			{
				/* we are looking for this string */
				compare=m_page->IDToString(sel->m_compare)->GetString();
				/* in this space seperated list */
				clist=att->GetValue()->GetString();
				found=false;
				do
				{
					ew=strstr(clist," ");
					if(!ew)
						ew=clist+strlen(clist);

					/* get hyphen spot */
					hs=strstr(clist,"-");
					if(!hs || hs>ew)		/* if hyphen not found or if it is past the word, then we will use the end */
						hs=ew;

					/* case sensitive or not? */
					if(!strncmp(clist,compare,hs-clist))
					{
						found=true;
						break;
					}
					clist=ew+1;
				}while(ew[0]);
				if(found==false)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_ATTBEGINVALUE:
		case CSSSELECTOR_ATTCONTAINSVALUE:
		case CSSSELECTOR_ATTENDVALUE:
		{
			kGUIHTMLAttrib *att;
			kGUIString *attvalue;
			kGUIString *compare;

			m_hitcomplex=true;
			att=ho->FindAttrib(sel->m_value);
			if(!att)
				res=false;
			else
			{
				attvalue=att->GetValue();
				compare=m_page->IDToString(sel->m_compare);

				/* should this be case sensative or not?? */
				switch(sel->m_selector)
				{
				case CSSSELECTOR_ATTBEGINVALUE:
					if(strnicmp(attvalue->GetString(),compare->GetString(),compare->GetLen()))
						res=false;
				break;
				case CSSSELECTOR_ATTCONTAINSVALUE:
					if(!strstri(attvalue->GetString(),compare->GetString()))
						res=false;
				break;
				case CSSSELECTOR_ATTENDVALUE:
					if(attvalue->GetLen()<compare->GetLen())
						res=false;	/* can't match! */
					else if(strnicmp(attvalue->GetString()+attvalue->GetLen()-compare->GetLen(),compare->GetString(),compare->GetLen()))
						res=false;
				break;
				}
			}
		}
		break;
		case CSSSELECTOR_LINK:
			m_hitcomplex=true;
			if(ho->m_id!=HTMLTAG_A || !ho->m_obj.m_linkobj)
				res=false;
			else if(ho->m_obj.m_linkobj->GetVisited()==true)
				res=false;
		break;
		case CSSSELECTOR_VISITED:
			m_hitcomplex=true;
			if(ho->m_id!=HTMLTAG_A || !ho->m_obj.m_linkobj)
				res=false;
			else if(ho->m_obj.m_linkobj->GetVisited()==false)
				res=false;
		break;
		case CSSSELECTOR_ACTIVE:
			m_hitcomplex=true;
			if(ho->m_active==false)
				res=false;			/* means just clicked on */
		break;
		case CSSSELECTOR_HOVER:
			m_hitcomplex=true;
			/* this tells the page event code to trigger a refresh when this objects hover status changes */
			/* that way we don't bother doing refreshes on objects that have no hover css styles attached */
			ho->m_useshover=true;

			/* if mouse is over any of my children */
			if(ho->GetHover()==false)
				res=false;
		break;
		case CSSSELECTOR_FOCUS:
			m_hitcomplex=true;		/* todo */
			res=false;
		break;
		case CSSSELECTOR_ROOT:
			if(ho->GetID()!=HTMLTAG_HTML)
				res=false;
		break;
		case CSSSELECTOR_FIRSTLINE:
			res=false;			/* todo */
		break;
		case CSSSELECTOR_FIRSTLETTER:
			res=false;			/* todo */
		break;
		case CSSSELECTOR_TARGET:
			res=(m_page->GetTargetObj()==ho);
		break;
		case CSSSELECTOR_ENABLED:
		{
			kGUIHTMLAttrib *att;

			m_hitcomplex=true;
			if(ho->m_id!=HTMLTAG_INPUT && ho->m_id!=HTMLTAG_BUTTON)
				res=false;	/* only valid for input type objects */
			else
			{
				att=ho->FindAttrib(HTMLATT_DISABLED);
				if(!att)
					res=true;
				else if(att->GetVID()==HTMLCONST_DISABLED)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_DISABLED:
		{
			kGUIHTMLAttrib *att;

			m_hitcomplex=true;
			if(ho->m_id!=HTMLTAG_INPUT && ho->m_id!=HTMLTAG_BUTTON)
				res=false;	/* only valid for input type objects */
			else
			{
				att=ho->FindAttrib(HTMLATT_DISABLED);
				if(!att)
					res=true;
				else if(att->GetVID()!=HTMLCONST_DISABLED)
					res=false;
			}
		}
		break;
		case CSSSELECTOR_CHECKED:
		{
			m_hitcomplex=true;

			if(ho->GetSubID()!=HTMLSUBTAG_INPUTCHECKBOX)
				res=false;
			else
				res=ho->m_obj.m_tickobj->GetSelected();
		}
		break;
		default:
			assert(false,"Unhandled selector");
		break;
		}
		/* handle NOT */
		if(res==sel->m_not)
			return(false);
	}
	return(true);
}

void kGUIHTMLPageObj::GetCorrectedCSS(kGUIString *dest)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	kGUIHTMLRule *rule;
	kGUIString rs;
	kGUIHTMLAttrib *att;

	dest->Clear();
	for(i=0;i<m_numrules;++i)
	{
		rule=m_rules.GetEntry(i);
		rule->GetString(&rs);
		dest->ASprintf("%s {\n",rs.GetString());
		for(j=0;j<rule->GetNumAttributes();++j)
		{
			ATTLIST_DEF *al;

			att=rule->GetAttrib(j);
			al=kGUIHTMLPageObj::m_attlist;
			for(k=0;k<(sizeof(kGUIHTMLPageObj::m_attlist)/sizeof(ATTLIST_DEF))-1;++k)
			{
				if(att->GetID()==al->attid)
					break;
				++al;
			}

			dest->ASprintf("%s: %s;",al->name,att->GetValue()->GetString());
		}

		dest->ASprintf("}\n\n");
	}
}

/* convert rule string to internal representation and if already exists then */
/* return pointer to previous one */

kGUIHTMLRule *kGUIHTMLPageObj::LocateRule(kGUIString *string)
{
	unsigned int i;
	kGUIHTMLRule *newrule;
	kGUIHTMLRule **rpp;
	kGUIString rs;

	newrule= new kGUIHTMLRule(this);
	if(newrule->Parse(string)==false)
	{
		/* error parsing rule */
		m_errors.ASprintf("Error parsing rule! '%S'\n",string);		
		delete newrule;
		return(0);
	}
	if(!newrule->GetNumSelectors())
	{
		m_errors.ASprintf("Error parsing rule! '%S'\n",string);		
		delete newrule;
		return(0);
	}
	
	/* since the owner levels for each attribute can change, we need to rebuild the cache */
	/* or since a new rules have been added we need to flush the tci cache */
	InvalidateTCICache();

	newrule->GetString(&rs);
	rpp=(kGUIHTMLRule **)m_rulehash.Find(rs.GetString());
	if(rpp)
	{
		/* already exists, return pointer to it */
		delete newrule;
		return(rpp[0]);
	}

	/* debugging, compare in rule to out rule */
	if(stricmp(string->GetString(),rs.GetString()))
	{
		/* ok, so maybe it is just a spacing issue, remove spaces, quotes, double colons etc and try again */
		kGUIString sn;
		kGUIString rsn;

		/* see if it looks the same with spaces and quotes removed before complaining */
		sn.SetString(string);
		sn.Replace(" ","");
		sn.Replace("\"","");
		rsn.SetString(&rs);
		rsn.Replace(" ","");
		rsn.Replace("\"","");
	
		if(stricmp(sn.GetString(),rsn.GetString()))
			m_parseerrors.ASprintf("Rule in='%s', out='%s'\n",string->GetString(),rs.GetString());
	}

	newrule->CalcScore();

	for(i=0;i<m_numrules;++i)
	{
		if(newrule->Compare(m_rules.GetEntry(i))>=0)
			break;
	}
	if(i<m_numrules)
	{
		m_rules.InsertEntry(m_numrules,i,1);
		m_rules.SetEntry(i,newrule);
	}
	else
		m_rules.SetEntry(m_numrules,newrule);
	++m_numrules;
	m_rulehash.Add(rs.GetString(),&newrule);

	return(newrule);
}

/* add all unadded rules that apply with the current state */
void kGUIHTMLPageObj::AddPossibleRules(void)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int n;
	kGUIHTMLRule *r;
	kGUIHTMLRule **rp;

	for(i=0;i<m_numrules;++i)
	{
		r=m_rules.GetEntry(i);
		if(r->GetPossible()==false)
		{
			r->SetCurNumRefs(0);

			/* count the number of tag matches */
			for(j=0;j<HTMLTAG_NUMTAGS;++j)
			{
				if(m_curtagrefs[j]>0)
				{
					n=m_tagrules[j].m_num;
					rp=m_tagrules[j].m_rules.GetArrayPtr();
					for(k=0;k<n;++k)
					{
						if(*(rp++)==r)
							r->AddRefQuick();
					}
				}
			}
			/* count the number of class matches */
			for(j=0;j<m_numclasses;++j)
			{
				if(m_curclassrefs.GetEntry(j)>0)
				{
					n=m_classrules.GetEntryPtr(j)->m_num;
					rp=m_classrules.GetEntryPtr(j)->m_rules.GetArrayPtr();
					for(k=0;k<n;++k)
					{
						if(*(rp++)==r)
							r->AddRefQuick();
					}
				}
			}
			/* count the number of id matches */
			for(j=0;j<m_numids;++j)
			{
				if(m_curidrefs.GetEntry(j)>0)
				{
					n=m_idrules.GetEntryPtr(j)->m_num;
					rp=m_idrules.GetEntryPtr(j)->m_rules.GetArrayPtr();
					for(k=0;k<n;++k)
					{
						if(*(rp++)==r)
							r->AddRefQuick();
					}
				}
			}

			/* is this rule possible? */
			if(r->GetNumRefs()==r->GetCurNumRefs())
				r->SetPossible(true);
		}
	}
}

void kGUIHTMLPageObj::AddPossibleRules(kGUIHTMLObj *o)
{
	unsigned int i;
	unsigned int n;
	kGUIHTMLRule *r;
	kGUIHTMLRule **rp;
	unsigned int tag=o->m_id;
	CID_DEF *cid;
	unsigned int ci;
	unsigned int c;
	kGUIHTMLRuleList *rulelist;

	/* if this is the first instance of this tag in the style tree then */
	/* increment the enable count for rules that reference this tag */

	if(!m_curtagrefs[tag])
	{
		/* this list is sorted by priority already so when we add entries */
		/* into the possible list we need to insert them to keep the sorting */

		n=m_tagrules[tag].m_num;
		rp=m_tagrules[tag].m_rules.GetArrayPtr();
		for(i=0;i<n;++i)
		{
			r=*(rp++);
			/* are all tags used in this rule now in the style list? */
			if(r->GetPossible()==false)
				r->AddRef();
		}
	}
	++m_curtagrefs[tag];

	/* cid is an array of class and id indexes associated with this tag */
	for(ci=0;ci<o->m_numcids;++ci)
	{
		cid=o->m_cids.GetEntryPtr(ci);
		switch(cid->m_type)
		{
		case CID_CLASS:
			c=m_curclassrefs.GetEntry(cid->m_id);
			if(!c)
			{
				rulelist=m_classrules.GetEntryPtr(cid->m_id);
				n=rulelist->m_num;
				rp=rulelist->m_rules.GetArrayPtr();
				for(i=0;i<n;++i)
				{
					r=*(rp++);
					/* are all tags used in this rule now in the style list? */
					if(r->GetPossible()==false)
						r->AddRef();
				}
			}
			m_curclassrefs.SetEntry(cid->m_id,++c);
		break;
		case CID_ID:
			c=m_curidrefs.GetEntry(cid->m_id);
			if(!c)
			{
				rulelist=m_idrules.GetEntryPtr(cid->m_id);
				n=rulelist->m_num;
				rp=rulelist->m_rules.GetArrayPtr();
				for(i=0;i<n;++i)
				{
					r=*(rp++);
					/* are all tags used in this rule now in the style list? */
					if(r->GetPossible()==false)
						r->AddRef();
				}
			}
			m_curidrefs.SetEntry(cid->m_id,++c);
		break;
		}
	}
}

void kGUIHTMLPageObj::RemovePossibleRules(kGUIHTMLObj *o)
{
	unsigned int i;
	unsigned int n;
	unsigned int c;
	unsigned int ci;
	kGUIHTMLRule *r;
	kGUIHTMLRule **rp;
	unsigned int tag=o->m_id;
	CID_DEF *cid;
	kGUIHTMLRuleList *rulelist;
		
	if(m_poppossrules==false)
		return;
	
	m_poppossrules=false;

	/* was this the last reference to this tag? */
	if(--m_curtagrefs[tag]==0)
	{
		/* remove all rules that are no longer possible since this tag will be popped */
		/* from the style tree */
		n=m_tagrules[tag].m_num;
		rp=m_tagrules[tag].m_rules.GetArrayPtr();
		for(i=0;i<n;++i)
		{
			r=*(rp++);
			r->SubRef();
		}
	}

	/* was this the last reference to this class/id? */
	for(ci=0;ci<o->m_numcids;++ci)
	{
		cid=o->m_cids.GetEntryPtr(ci);
		switch(cid->m_type)
		{
		case CID_CLASS:
			c=m_curclassrefs.GetEntry(cid->m_id)-1;
			if(!c)
			{
				rulelist=m_classrules.GetEntryPtr(cid->m_id);
				n=rulelist->m_num;
				rp=rulelist->m_rules.GetArrayPtr();
				for(i=0;i<n;++i)
				{
					r=*(rp++);
					r->SubRef();
				}
			}
			m_curclassrefs.SetEntry(cid->m_id,c);
		break;
		case CID_ID:
			c=m_curidrefs.GetEntry(cid->m_id)-1;
			if(!c)
			{
				rulelist=m_idrules.GetEntryPtr(cid->m_id);
				n=rulelist->m_num;
				rp=rulelist->m_rules.GetArrayPtr();
				for(i=0;i<n;++i)
				{
					r=*(rp++);
					r->SubRef();
				}
			}
			m_curidrefs.SetEntry(cid->m_id,c);
		break;
		}
	}
}

/* add list of styles to a HTMLTag or UserClass */
void kGUIHTMLPageObj::AddClassStyle(unsigned int owner,unsigned int priority,kGUIString *url,kGUIString *name,kGUIString *value)
{
	kGUIHTMLRule *rule;
	unsigned int content;

	rule=LocateRule(name);
	
	/* if there were any errors when parsing the rule then we will get null back */
	if(rule)
	{

#if 1
		/* some places have !imp and others ! imp */
		while(value->Replace("! ","!"));

		if(value->Replace("!important","",0,true))
		{
			/* this is important */
			switch(owner)
			{
			case OWNER_USER:
				owner=OWNER_USERIMPORTANT;
			break;
			case OWNER_AUTHOR:
				owner=OWNER_AUTHORIMPORTANT;
			break;
			}
		}
#endif

		/* todo: fix by expanding the URL as it is added */
		/* possible bug, rule ".aaa" is created in css file a.css and background-image is applied to rule ".aaa" in css file b.css */
		/* solution is to keep track of referer for the background-image and each and any other styles that can have a URL */

		if(url)
			rule->SetOwnerURL(StringToIDcase(url));
		else
			rule->SetOwnerURL(0);

		switch(rule->GetPseudo())
		{
		default:
		case PSEUDO_NONE:
			content=HTMLATT_ERROR;
		break;
		case PSEUDO_BEFORE:
			content=HTMLATT_CONTENT_BEFORE;
		break;
		case PSEUDO_AFTER:
			content=HTMLATT_CONTENT_AFTER;
		break;
		}

		rule->AddAttributes(this,content,owner,priority,value);
		rule->UpdateNumOwnerStyles();
	}
}

/* use the whole thing! */
void kGUIHTMLPageObj::CalcChildZone(void)
{
	SetChildZone(0,0,GetZoneW(),GetZoneH());
	Scrolled();
}

void kGUIHTMLPageObj::PurgeRules(void)
{
	unsigned int i;
	kGUIHTMLRule *rule;

	for(i=0;i<m_numrules;++i)
	{
		rule=m_rules.GetEntry(i);
		delete rule;
	}
	m_numrules=0;
	m_numownerrules=0;
	m_ownerrulesbuilt=false;
	m_rulehash.Reset();
}

kGUIHTMLPageObj::~kGUIHTMLPageObj()
{
	unsigned int i;
	kGUIOnlineImage *image;
	kGUIOnlineLink *link;

	kGUI::DelEvent(this,CALLBACKNAME(TimerEvent));

	/* shutdown Async thread */
	m_shutdown=true;
	while(m_shutdown)
		kGUI::Sleep(1);

	PurgeRules();
	PurgeTCICache();

	m_rootobject->DeleteChildren(true);
	delete m_rootobject;

	m_fixedfgobject->DeleteChildren(true);
	delete m_fixedfgobject;

	/* abort all pending downloads */

	for(i=0;i<m_numimages;++i)
	{
		image=m_images.GetEntry(i);
		image->Abort();					/* abort downloading */
	}

	for(i=0;i<m_numlinks;++i)
	{
		link=m_links.GetEntry(i);
		link->Abort();					/* abort downloading */
	}

	for(i=0;i<m_numimages;++i)
	{
		image=m_images.GetEntry(i);
		delete image;
	}

	for(i=0;i<m_numlinks;++i)
	{
		link=m_links.GetEntry(i);
		delete link;
	}
}

int kGUIHTMLPageObj::GetScrollY(void)
{
	return m_scroll.GetDestY();
}

void kGUIHTMLPageObj::SetScrollY(int y)
{
	m_scroll.Goto(0,y);

	/* since we won't get a "moved" callback we will call it ourselves */
	Scrolled();
	UpdateScrollBars();

	if(m_usevs)
		m_vscrollbar.Dirty();

	if(m_usehs)
		m_hscrollbar.Dirty();
}

void kGUIHTMLPageObj::MoveRow(int delta)
{
	int y=m_scroll.GetDestY();

	y+=delta;
	if(y>(m_rootobject->GetZoneH()-m_pageh))
		y=m_rootobject->GetZoneH()-m_pageh;
	if(y<0)
		y=0;

    m_scroll.SetDestY(y);
}

void kGUIHTMLPageObj::MoveCol(int delta)
{
	int x=m_scroll.GetDestX();

	x+=delta;
	if(x>(m_rootobject->GetZoneW()-m_pagew))
		x=m_rootobject->GetZoneW()-m_pagew;
	if(x<0)
		x=0;

    m_scroll.SetDestX(x);
}

void kGUIHTMLPageObj::Scrolled(void)
{
	kGUIZone cz;

	SetChildScroll(m_scroll.GetCurrentX(),m_scroll.GetCurrentY());

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

void kGUIHTMLPageObj::Draw(void)
{
	kGUICorners c;
	int sx=GetChildScrollX();
	int sy=GetChildScrollY();

	g_trace=m_trace;
	if(m_trace)
		kGUI::Trace("DrawPage Trace - start\n");

	kGUI::SetFastDraw(true);
	if(m_reposition==true)
	{
		m_trace=false;
		m_reposition=false;
		Position();
		m_trace=g_trace;	/* put back since position zeros it */
	}

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

	/* fixed to viewport forground objects don't scroll */
	m_fixedfgobject->MoveZone(sx,sy,c.rx-c.lx,c.by-c.ty);
	m_fixedfgobject->CalcChildZone();

	kGUI::PushClip();
	kGUI::ShrinkClip(&c);

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,m_pagebgcolor);

		DrawC(0);
	}
	kGUI::PopClip();
	kGUI::SetFastDraw(false);

	m_trace=false;
	g_trace=false;
}



void kGUIHTMLObj::DrawBG(kGUICorners *c,bool fixed,int offlx,int offty,int offrx,int offby)
{
	int tw,th;
	kGUICorners bgc;
	double alpha;

#if 0
	if(m_error)
	{
		/* draw red BG on error objects */
		kGUI::DrawRect(c->lx,c->ty,c->rx,c->by,DrawColor(255,0,0));
		return;
	}
#endif

	kGUI::PushClip();
	kGUI::ShrinkClip(c);
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(m_bgcolor.GetTransparent()==false && fixed==false)
		{
			alpha=m_bgcolor.GetAlpha();

			if(alpha==1.0f)
				kGUI::DrawRect(c->lx,c->ty,c->rx,c->by,m_bgcolor.GetColor());
			else if(alpha>0.0f)
				kGUI::DrawRect(c->lx,c->ty,c->rx,c->by,m_bgcolor.GetColor(),alpha);
		}
		/* might be "valid", but not loaded yet, so we check width & height as well to see if it is ready */

		if(m_bgfixed==true)
			m_page->GetCorners(&bgc);
		else
			bgc=*(c);

		if(m_bgimage.GetIsValid() /*&& m_bgfixed==fixed*/)
		{
			tw=m_bgimage.GetImageWidth();
			th=m_bgimage.GetImageHeight();
			if(tw && th)
			{
				int px,py;
				int nw,nh;
				int bgx,bgy;

				/* get x position */
				if(m_bgx.GetUnitType()==UNITS_UNDEFINED)
					bgx=0;
				else
					bgx=m_bgx.CalcUnitValue(((bgc.rx-offrx)-(bgc.lx+offlx))-tw,m_em);

				bgx+=offlx;
				if(m_bgrepeatx==false)
					nw=1;
				else
				{
					while(bgx>0)
						bgx-=tw;

					nw=((c->rx-(bgc.lx+bgx))/tw)+1;
				}

				/* get y position */
				if(m_bgy.GetUnitType()==UNITS_UNDEFINED)
					bgy=0;
				else
					bgy=m_bgy.CalcUnitValue(((bgc.by-offby)-(bgc.ty+offty))-th,m_em);

				bgy+=offty;

				if(m_bgrepeaty==false)
					nh=1;
				else
				{
					while(bgy>0)
						bgy-=th;

					nh=((bgc.by-(bgc.ty+bgy))/th)+1;
				}

				px=bgc.lx+bgx;
				py=bgc.ty+bgy;

				m_bgimage.MoveZoneX(px);
				m_bgimage.MoveZoneY(py);
				m_bgimage.MoveZoneW(tw*nw);
				m_bgimage.MoveZoneH(th*nh);
				m_bgimage.TileDraw();
			}
		}
	}
	kGUI::PopClip();
}


/*************************************************************************************/

void kGUIHTMLObj::CalcChildZone(void)
{
	int x,y,w,h;

	x=0;
	y=0;
	w=GetZoneW();
	h=GetZoneH();
	if(m_box)
	{
		x+=m_box->GetBoxLeftWidth();
		y+=m_box->GetBoxTopWidth();
		w-=(m_box->GetBoxLeftWidth()+m_box->GetBoxRightWidth());
		h-=(m_box->GetBoxTopWidth()+m_box->GetBoxBottomWidth());
	}
	if(m_id==HTMLTAG_TD || m_id==HTMLTAG_TH)
	{
		if(m_ti)
		{
			if(m_ti->m_cellpadding)
			{
				x+=m_ti->m_cellpadding;
				y+=m_ti->m_cellpadding;
				w-=(m_ti->m_cellpadding<<1);
				h-=(m_ti->m_cellpadding<<1);
			}
		}
	}

	SetChildZone(x,y,w,h);
}

/* only used for main page, no need for borders etc? */
void kGUIHTMLObj::CalcChildZone(int yoff)
{
	int x,y,w,h;

	x=0;
	y=0;
	w=GetZoneW();
	h=GetZoneH();
	if(m_box)
	{
		x+=m_box->GetBoxLeftWidth();
		y+=m_box->GetBoxTopWidth();
		w-=(m_box->GetBoxLeftWidth()+m_box->GetBoxRightWidth());
		h-=(m_box->GetBoxTopWidth()+m_box->GetBoxBottomWidth());
	}
	if(m_id==HTMLTAG_TD || m_id==HTMLTAG_TH)
	{
		if(m_ti)
		{
			if(m_ti->m_cellpadding)
			{
				x+=m_ti->m_cellpadding;
				y+=m_ti->m_cellpadding;
				w-=(m_ti->m_cellpadding<<1);
				h-=(m_ti->m_cellpadding<<1);
			}
		}
	}

	SetChildZone(x,y+yoff,w,h-yoff);
}

class HandleOpacity
{
public:
	HandleOpacity() {m_w=0;}
	void Save(const kGUICorners *c,double opacity);
	void Blend(void);
private:
	double m_opacity;
	int m_x;
	int m_y;
	int m_w;
	int m_h;
	kGUIColor *m_buffer;
};

/* save screen area under opacitay are for mixing later, clear to black */
void HandleOpacity::Save(const kGUICorners *c,double opacity)
{
	int x,y;
	kGUIColor *sp;
	kGUIColor *bp;

	m_x=c->lx;
	m_y=c->ty;
	m_w=c->rx-c->lx;
	m_h=c->by-c->ty;

	m_opacity=opacity;
	m_buffer=new kGUIColor[m_w*m_h];
	bp=m_buffer;

	for(y=0;y<m_h;++y)
	{
		sp=kGUI::GetSurfacePtr(m_x,y+m_y);
		for(x=0;x<m_w;++x)
		{
			*(bp++)=*(sp++);
			//*(sp++)=0xffffffff;
		}
	}
}

void HandleOpacity::Blend(void)
{
	int x,y;
	kGUIColor *sp;
	kGUIColor *bp;
	kGUIColor b,s;
	int sr,sg,sb,br,bg,bb;
	int newr,newg,newb;
	double salpha=m_opacity;
	double balpha=1.0f-salpha;

	if(!m_w)
		return;

	bp=m_buffer;
	for(y=0;y<m_h;++y)
	{
		sp=kGUI::GetSurfacePtr(m_x,y+m_y);
		for(x=0;x<m_w;++x)
		{
			s=*(sp);
			if(!s)
			{
				*(sp++)=*(bp++);
			}
			else
			{
				b=*(bp++);
			
				/* extract rgb from current screen contents */
				DrawColorToRGB(s,sr,sg,sb);
				/* extract rgb from saved buffer contents */
				DrawColorToRGB(b,br,bg,bb);

				/* blend them */
				newr=(int)(sr*salpha)+(int)(br*balpha);
				newg=(int)(sg*salpha)+(int)(bg*balpha);
				newb=(int)(sb*salpha)+(int)(bb*balpha);

				*(sp++)=DrawColor(newr,newg,newb);
			}
		}
	}
	delete []m_buffer;
}

void kGUIHTMLObj::Draw(void)
{
	kGUICorners c;
	kGUICorners mc;
	HandleOpacity opacity;

	if((m_display==DISPLAY_NONE) || m_opacity==0.0f|| (m_visible==VISIBLE_HIDDEN))
	{
		if(m_page->GetTrace())
			kGUI::Trace("Container Hidden so draw aborted \n");
		return;
	}
	kGUI::PushClip();
	GetCorners(&c);

	if(m_opacity!=1.0)
	{
		kGUI::PushClip();
		kGUI::ShrinkClip(&c);
		if(kGUI::ValidClip())
			opacity.Save(kGUI::GetClipCorners(),m_opacity);
		kGUI::PopClip();
	}

	if(m_page->GetTrace())
	{
		kGUIString ts;

		m_page->GetTagDesc(this,&ts);
		kGUI::Trace("Draw Container %s lx=%d,rx=%d,ty=%d,by=%d\n",ts.GetString(),c.lx,c.rx,c.ty,c.by);
	}

	/* handle overflow clipping modes */
	switch(m_overflowx)
	{
	case OVERFLOW_VISIBLE:
		/* let content overflow */
	break;
	case OVERFLOW_HIDDEN:
	case OVERFLOW_SCROLL:	/* scroll bar allows viewing by scrolling */
	case OVERFLOW_AUTO:
		kGUI::ShrinkClipLX(c.lx);
		kGUI::ShrinkClipRX(c.rx);
	break;
	}

	switch(m_overflowy)
	{
	case OVERFLOW_VISIBLE:
		/* let content overflow */
	break;
	case OVERFLOW_HIDDEN:
	case OVERFLOW_SCROLL:	/* scroll bar allows viewing by scrolling */
	case OVERFLOW_AUTO:
		kGUI::ShrinkClipTY(c.ty);
		kGUI::ShrinkClipBY(c.by);
	break;
	}

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(m_id==HTMLTAG_FIXEDROOT)
		{
			/* no background, since it is already drawn */
		}
		else if(!m_box || m_display==DISPLAY_INLINE)
			DrawBG(&c,false,0,0,0,0);
		else
		{
			mc.lx=c.lx+m_box->GetBoxPosLeftMargin();
			mc.rx=c.rx-m_box->GetBoxPosRightMargin();
			mc.ty=c.ty+m_box->GetBoxPosTopMargin();
			mc.by=c.by-m_box->GetBoxPosBottomMargin();
			//kGUI::ShrinkClip(&mc);

			DrawBG(&mc,false,m_box->GetBoxLeftBorder(),m_box->GetBoxTopBorder(),m_box->GetBoxRightBorder(),m_box->GetBoxBottomBorder());
		}
		if(m_display==DISPLAY_TABLE_CELL)
		{
			if(m_box)
			{
				m_box->Draw(&c);
				c.lx+=m_box->GetBoxLeftWidth();
				c.ty+=m_box->GetBoxTopWidth();
				c.rx-=m_box->GetBoxRightWidth();
				c.by-=m_box->GetBoxBottomWidth();
			}
			if(m_ti)
			{
				m_ti->m_cellborder->Draw(&c);
				c.lx+=m_ti->m_cellborder->GetBoxLeftWidth();
				c.ty+=m_ti->m_cellborder->GetBoxTopWidth();
				c.rx-=m_ti->m_cellborder->GetBoxRightWidth();
				c.by-=m_ti->m_cellborder->GetBoxBottomWidth();
				//kGUI::ShrinkClip(&c);
			}
		}
		else if(m_box)
		{
			m_box->Draw(&c);
			c.lx+=m_box->GetBoxLeftWidth();
			c.ty+=m_box->GetBoxTopWidth();
			c.rx-=m_box->GetBoxRightWidth();
			c.by-=m_box->GetBoxBottomWidth();
			/* are there scroll bars attached to this object? */
			if(m_scroll)
				m_scroll->DrawScroll(&c);

		}
	}
	else
	{
		if(m_page->GetTrace())
		{
			kGUI::Trace("Draw BG Skipped since no valid area to draw in\n");
		}
	}
	/* table and rows are a slight special case since cells with a rowspan larger than one */
	/* can cause problem when the next row overdraws the cell above that hangs down */
	/* so, we instead draw all rows first, then go and draw all cells second */
	if(m_display==DISPLAY_TABLE || m_display==DISPLAY_INLINE_TABLE)
	{
		unsigned int r;
		unsigned int c;
		unsigned int j;
		kGUIHTMLObj *o;

		/* draw rows */
		for(r=0;r<m_ti->m_numrows;++r)
		{
			o=m_ti->m_rowptrs.GetEntry(r);
			o->Draw();
		}

		/* draw cells */
		j=0;
		for(r=0;r<m_ti->m_numrows;++r)
		{
			for(c=0;c<m_ti->m_numcols;++c)
			{
				o=m_ti->m_cellptrs.GetEntry(j++);
				if(o && o!=this)
					o->Draw();		/* entries covered by colspan/rowspan point to table */
			}
		}
	}
	else if(m_display!=DISPLAY_TABLE_ROW)
	{
#if 1
		int e,nc;
		kGUIObj *gobj;
		kGUICorners c;

		/* get the corners of the childarea for this containier */
		GetChildCorners(&c);

		kGUI::PushClip();
		//kGUI::ShrinkClip(&c);
		
		/* is there anywhere to draw? */
		if(kGUI::ValidClip())
		{
			/* draw all child objects in this group */
			nc=GetNumChildren(0);
			for(e=0;e<nc;++e)
			{
				gobj=GetChild(0,e);
				gobj->Draw();
			}
		}
		else if(m_page->GetTrace())
		{
			if(c.ty<0)
				kGUI::Trace("Draw Aborted since no valid area to draw in\n");
			
		}

		kGUI::PopClip();
#else
		DrawC(0);
#endif
	}

#if 1
	/* draw a box */
	if( m_page->GetSettings()->GetDrawBoxes())
	{
		kGUICorners c2;

		GetCorners(&c);
		GetChildCorners(&c2);

		kGUI::DrawCurrentFrame(c.lx,c.ty,c.rx,c.by);
		kGUI::DrawCurrentFrame(c2.lx,c2.ty,c2.rx,c2.by);
	}
#endif
	if(m_opacity!=1.0f)
		opacity.Blend();
	kGUI::PopClip();
}

void kGUIHTMLPageObj::InitPosition(void)
{
	unsigned int i;
	kGUIHTMLRule *r;

	/* this is used for the rule optimizer so only rules that can */
	/* be possible are added to the possible list */

	/* optimize: change these to memfill? */
	for(i=0;i<HTMLTAG_NUMTAGS;++i)
		m_curtagrefs[i]=0;

	for(i=0;i<m_numclasses;++i)
		m_curclassrefs.SetEntry(i,0);
	
	for(i=0;i<m_numids;++i)
		m_curidrefs.SetEntry(i,0);
	
	for(i=0;i<m_numrules;++i)
	{
		r=m_rules.GetEntry(i);
		r->SetCurNumRefs(0);
		r->SetLastTime(0);

		/* special case for rule with no tag/class/id references but possibly other types */
		r->SetPossible(r->GetCurNumRefs()==r->GetNumRefs());
	}
	m_poppossrules=false;

	m_errors.SetString(m_parseerrors.GetString());
	m_errors.Append(m_csserrors.GetString());
	m_debug.ASprintf("******* Start Position ********\n");

	/* default font color to black */
	m_fontcolor.Set((kGUIColor)DrawColorA(0,0,0,255));
	m_textdecorationcolor=DrawColorA(0,0,0,255);
	m_pagebgcolor=DrawColorA(255,255,255,255);

	m_reposition=false;
	m_link=0;
	m_form=0;
	m_fontweight=400;									/* normal */
	m_fontsize=xfontsize[4-1];
	m_fontscale=1;
	m_pageem=(unsigned int)(m_fontsize*m_fontscale);
	m_dir=TEXTDIR_LTR;									/* default textdirection is left to right */

	m_whitespace=WHITESPACE_NORMAL;
	m_textdecoration=TEXTDECORATION_NONE;
	m_texttransform=TEXTTRANSFORM_NONE;
//	m_textalign=ALIGN_LEFT;

	/* lineheight as a ratio */
	m_lineheightratio.SetUnitType(UNITS_PERCENT);
	m_lineheightratio.SetUnitValue(120);
	/* lineheight as a pixel value */
	m_pagelineheightpix=m_lineheightratio.CalcUnitValue((int)m_pageem,(int)m_pageem);

	m_textshadowr=-1;
	m_textshadowx=0;
	m_textshadowy=0;
	m_textshadowcolor.Set((kGUIColor)DrawColor(255,255,255));
	m_liststyle=LISTSTYLE_DECIMAL;
	m_liststyleposition=LISTSTYLE_OUTSIDE;
	m_liststyleurl=0;
	m_liststyleurlreferer=0;

//	m_patttextindent=0;
//	m_textindent.Zero();
	m_letterspacing.Zero();
	m_wordspacing.Zero();

	m_fontid=0;
	m_cellspacing=1;
	m_cellpadding=1;

	/* reset the attribute applied table */
	m_appliedlevel=0;
	for(i=0;i<HTMLATT_UNKNOWN;++i)
	{
		m_applied[i]=0;
		m_blocked[i]=false;
		m_trueapplied[i]=0;
	}
}

int kGUIHTMLPageObj::PositionPrint(int width)
{
	InitPosition();

	m_rootobject->SetZone(0,0,GetZoneW(),GetZoneH());
	m_rootobject->SetPage(this);
	m_rootobject->SetID(HTMLTAG_ROOT);
	m_rootobject->m_tag=&m_roottag;
	m_mode=MODE_MINMAX;

	/* set all containers to minimum size, calc max size too */
	m_posindex=0;
	m_rootobject->Position();
	m_rootobject->Contain();

	/* if width is less than pagewidth then expand root to pagewidth */
	if(m_rootobject->GetOutsideW()<width)
		m_rootobject->SetOutsideW(width);

	/* expand all tables as large as they can go */
	m_mode=MODE_POSITION;
	m_posindex=0;
	m_rootobject->Position();
	m_maxpagew=0;
	m_maxpageh=0;
	m_rootobject->Contain();
	return(m_rootobject->GetZoneH());
}

void kGUIHTMLPageObj::Position(void)
{
	/* screen width */
	int width=GetZoneW()-kGUI::GetSkin()->GetScrollbarWidth();
	int height=GetZoneH();

	m_ownerrulesbuilt=false;

	kGUI::SetMouseCursor(MOUSECURSOR_BUSY);

	m_mode=MODE_MINMAX;
	InitPosition();

	m_rootobject->SetZone(0,0,width,height);
	m_rootobject->CalcChildZone();
	m_rootobject->SetPage(this);
	m_rootobject->SetID(HTMLTAG_ROOT);
	m_rootobject->m_tag=&m_roottag;
	m_absobject=m_rootobject;

	m_fixedfgobject->SetZone(0,0,width,height);
	m_fixedfgobject->CalcChildZone();

	/* set all containers to minimum size, calc max size too */
//	kGUI::Trace("Pass 1 Root size=%d\n",GetZoneW());
	m_posindex=0;
	m_rootobject->Position();
	m_maxpagew=0;
	m_maxpageh=0;
	m_rootobject->Contain(true);

	/* if width is less than pagewidth then expand root to pagewidth */
	if(m_rootobject->GetZoneW()<=width)
	{
		m_usehs=false;
		m_pagew=width;
		m_rootobject->SetOutsideW(width);
		m_rootobject->CalcChildZone();
		m_scroll.SetDestX(0);
	}
	else
	{
		m_usehs=true;
		m_pagew=width;
	}
	/* expand all tables as large as they can go */
	m_debug.ASprintf("*********** Start Expand *************\n");
	m_debug.ASprintf("*********** End Expand *************\n");
//	kGUI::Trace("Pass 2 Root size=%d\n",GetZoneW());
	m_mode=MODE_POSITION;
	m_posindex=0;
	m_rootobject->Position();
	m_maxpagew=0;
	m_maxpageh=0;
	m_rootobject->Contain();

	if(m_rootobject->GetOutsideH()>height)
	{
		m_usevs=true;
		m_pageh=height-kGUI::GetSkin()->GetScrollbarHeight();
	}
	else
	{
		m_usevs=false;
		m_pageh=height;
		m_scroll.Goto(0,0);
		if(!m_usehs)
		{
			/* expand width to full width as whole page fits in window without scrolling */
			m_pagew=GetZoneW();
			m_rootobject->SetOutsideW(m_pagew);
			m_rootobject->CalcChildZone();

			m_mode=MODE_POSITION;
			m_posindex=0;
			m_rootobject->Position();
			m_maxpagew=0;
			m_maxpageh=0;
			m_rootobject->Contain();
		}
	}

	UpdateScrollBars();
	m_debug.ASprintf("******* End Position ********\n");

	if(m_status)
	{
		if(m_numloading)
			m_status->Sprintf("Loading %d items...",m_numloading);
		else
			m_status->Sprintf("Done");
	}

	kGUI::SetMouseCursor(MOUSECURSOR_DEFAULT);
	m_trace=false;
}

void kGUIHTMLPageObj::AddMedia(kGUIString *url)
{
	int x=0;

	if(m_mediahash.Find(url->GetString()))
		return;

	m_mediahash.Add(url->GetString(),&x);
	m_media.ASprintf("%s\n",url->GetString());
}

void kGUIHTMLPageObj::FlushCurrentMedia(void)
{
	int h,hh;
	HashEntry *he;
	const char *fn;
	kGUIOnlineImage **ip;
	kGUIOnlineLink **linkp;

	he=m_mediahash.GetFirst();
	hh=m_mediahash.GetNum();
	for(h=0;h<hh;++h)
	{
		fn=he->m_string;

		if(GetItemCache())
		{
			kGUIString sfn;

			sfn.SetString(fn);
			GetItemCache()->Purge(&sfn);
		}

		/* look for this image in the image cache */
		ip=(kGUIOnlineImage **)m_imagehash.Find(fn);
		if(ip)
			ip[0]->Purge();
		else
		{
			linkp=(kGUIOnlineLink **)m_linkhash.Find(fn);
			if(linkp)
				linkp[0]->Flush();
		}

		he=he->GetNext();
	}
}

void kGUIHTMLPageObj::UpdateScrollBars(void)
{
	if(m_usevs==true)
	{
		int below;

		below=m_rootobject->GetZoneH()-m_pageh-m_scroll.GetDestY();
		if(below<0)
			below=0;
		m_vscrollbar.SetValues(m_scroll.GetDestY(),m_pageh,below);
	}
	if(m_usehs==true)
	{
		int right;

		right=m_rootobject->GetZoneW()-m_pagew-m_scroll.GetDestX();
		if(right<0)
			right=0;
		m_hscrollbar.SetValues(m_scroll.GetDestX(),m_pagew,right);
	}
}

void kGUIHTMLPageObj::TimerEvent(void)
{
	if(m_refreshdelay)
	{
		m_refreshdelay-=kGUI::GetET();
		if(m_refreshdelay<=0)
		{
			m_refreshdelay=0;
			/* load the refresh url */
			Click(&m_refreshurl,&m_url);
		}
	}

	/* if any blinked text is active on the page then handle flashing it */
	if(m_numblinks)
	{
		unsigned int i;

		m_blinkdelay-=kGUI::GetET();
		if(m_blinkdelay<0)
		{
			m_blinkdelay=BLINKDELAY;

			/* dirty all blink objects */
			for(i=0;i<m_numblinks;++i)
				m_blinks.GetEntry(i)->Blink();
		}
	}
}

bool kGUIHTMLPageObj::UpdateInput(void)
{
	kGUICorners c;
	kGUICorners cs;
	bool over;
	bool used;
	bool changed=false;

	/* if i'm current but not in the top window then unactivate me */
	if(this==kGUI::GetActiveObj() && ImCurrent()==false)
	{
		kGUI::PopActiveObj();

		Dirty();
		return(false);
	}

	switch(kGUI::GetKey())
	{
	case GUIKEY_DOWN:
		kGUI::EatKey();
		MoveRow(40);
	break;
	case GUIKEY_UP:
		kGUI::EatKey();
		MoveRow(-40);
	break;
	case GUIKEY_LEFT:
		kGUI::EatKey();
		MoveCol(-40);
	break;
	case GUIKEY_RIGHT:
		kGUI::EatKey();
		MoveCol(40);
	break;
	case GUIKEY_PGDOWN:
		kGUI::EatKey();
		MoveRow(400);
	break;
	case GUIKEY_PGUP:
		kGUI::EatKey();
		MoveRow(-400);
	break;
	}

	m_isover=false;
	GetCorners(&c);
	m_linkunder.Clear();

	UpdateScrollBars();
	over=kGUI::MouseOver(&c);
	if(this!=kGUI::GetActiveObj())
	{
		if(over==true && (kGUI::GetMouseClick()==true || kGUI::GetMouseWheelDelta()))
			kGUI::PushActiveObj(this);
	}

	if(this==kGUI::GetActiveObj())
	{
		if(over==false && (kGUI::GetMouseClick()==true || kGUI::GetMouseWheelDelta()))
		{
			kGUI::PopActiveObj();

			Dirty();
			return(false);
		}

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
				MoveRow(-scroll*80);
		}
	}

	/* update the hover info */
	{
		unsigned int i;
		unsigned int ls;
		int mx,my;
		int oldindex;
		int newindex;
		bool diff;
		kGUIHTMLObj *ho;

		/* has mouse moved? */
		mx=kGUI::GetMouseX();
		my=kGUI::GetMouseY();
		if( (mx!=m_lastmousex) || (my!=m_lastmousey))
		{
			oldindex=m_hovertoggle;
			m_hovertoggle=m_hovertoggle^1;
			newindex=m_hovertoggle;

			/* build the new hover object list */
			m_hoverlistsize[newindex]=0;
			m_rootobject->UpdateHover();
		
			/* compare to the previous one */
			if(m_hoverlistsize[0]!=m_hoverlistsize[1])
				diff=true;
			if(memcmp(m_hoverlist[0].GetArrayPtr(),m_hoverlist[1].GetArrayPtr(),sizeof(kGUIHTMLObj *)*m_hoverlistsize[0]))
				diff=true;
			else
				diff=false;

			/* set all prev "washover"=true */
			ls=m_hoverlistsize[oldindex];
			for(i=0;i<ls;++i)
			{
				ho=m_hoverlist[oldindex].GetEntry(i);
				ho->m_washover=true;
			}

			/* set all now hovered to m_hover=true */
			ls=m_hoverlistsize[newindex];
			for(i=0;i<ls;++i)
			{
				ho=m_hoverlist[newindex].GetEntry(i);
				if(ho->m_washover==false)
				{
					/* this is a newly hovered object */
					ho->m_hover=true;
					if(ho->m_useshover)
						changed=true;
				}
				ho->m_washover=false;	/* clear it */
			}

			/* any old ones with "washover" still set are now unhovered */
			ls=m_hoverlistsize[oldindex];
			for(i=0;i<ls;++i)
			{
				ho=m_hoverlist[oldindex].GetEntry(i);
				if(ho->m_washover==true)
				{
					/* this is a newly un-hovered object */
					ho->m_hover=false;
					ho->m_washover=false;
					if(ho->m_useshover)
						changed=true;
				}
			}
		}

		if(changed==true && m_reposition==false)
		{
			m_reposition=true;
			Dirty();
		}
		m_lastmousex=mx;
		m_lastmousey=my;
	}

	used=UpdateInputC(0);

	/* update the link under the cursor? */
	if(m_drawlinkunder)
	{
		if(strcmp(m_linkunder.GetString(),m_drawlinkunder->GetString()))
			m_drawlinkunder->SetString(&m_linkunder);
	}

	/* nobody under cursor? */
	if(m_isover==false && m_linkhover)
		m_linkhover=0;
	return(used);
}

typedef struct
{
	const char *tagname;
	unsigned int tagnamelen;
	unsigned int tagid;
	kGUIHTMLObj *renderparent;
	kGUIHTMLObj *tagparent;
	bool skip:1;
	bool optend:1;
	bool inserted:1;
}TAGSTACK_DEF;

#define PopTag() \
{\
	TAGSTACK_DEF poptag;\
	popped=true;\
	poptag=tagstack.GetEntry(--numtags);\
	renderparent=poptag.renderparent;\
	tagparent=poptag.tagparent;\
	attparent=tagparent;\
}

#define SkipTag(name) \
{\
	TAGSTACK_DEF missingtag;\
	missingtag.tagname=name;\
	missingtag.tagnamelen=(int)strlen(name);\
	missingtag.renderparent=renderparent;\
	missingtag.tagparent=tagparent;\
	missingtag.tagid=tag->tokenid;\
	missingtag.skip=true;\
	missingtag.inserted=false;\
	missingtag.optend=false;\
	tagstack.SetEntry(numtags++,missingtag);\
	tag=0;\
}

#define InsertTag(name) \
{\
	kGUIHTMLObj *newobj;\
	TAGLIST_DEF	**tagptr2=(TAGLIST_DEF **)m_taghash.Find(name);\
	TAGSTACK_DEF missingtag;\
	newobj=new kGUIHTMLObj(renderparent,this,*(tagptr2));\
	renderparent->AddObject(newobj);\
	tagparent->AddStyleChild(newobj);\
	missingtag.tagname=name;\
	missingtag.tagnamelen=(int)strlen(name);\
	missingtag.renderparent=renderparent;\
	missingtag.tagparent=tagparent;\
	missingtag.tagid=(*(tagptr2))->tokenid;\
	missingtag.inserted=true;\
	missingtag.skip=false;\
	missingtag.optend=true;\
	renderparent=newobj;\
	tagparent=newobj;\
	attparent=newobj;\
	tagstack.SetEntry(numtags++,missingtag);\
}

void kGUIHTMLPageObj::RemoveComments(kGUIString *s)
{
	const char *sc;
	const char *ec;

	/* remove comments from string */
	do
	{
		/* start of comment */
		sc=strstr(s->GetString(),"/*");
		if(!sc)
			return;
		/* end of comment */
		ec=strstr(sc+2,"*/");
		if(!ec)
			return;
		
		/* remove comment */
		s->Delete((int)(sc-s->GetString()),(int)((ec+2)-sc));
	}while(1);
}

/* lookup string to see if it exists and if so return the ID, if not then add */
unsigned int kGUIHTMLPageObj::StringToID(kGUIString *s)
{
	unsigned int *idptr;
	ATTLIST_DEF **attptr;

	/* first lets see if this ID matches a known attribute name */
	attptr=(ATTLIST_DEF **)m_atthash.Find(s->GetString());
	if(attptr)
		return((*attptr)->attid);

	idptr=(unsigned int *)m_stoidhash.Find(s->GetString());
	if(idptr)
		return(idptr[0]);

	/* add it */
	m_stoidhash.Add(s->GetString(),&m_nextstoid);
	/* also save it in indexed list for a reverse lookup */
	m_idtos.GetEntryPtr(m_nextstoid-HTMLATT_NUM)->SetString(s);
	return(m_nextstoid++);
}

/* lookup string to see if it exists and if so return the ID, if not then add */
/* this function is for case sensative lookups */
unsigned int kGUIHTMLPageObj::StringToIDcase(kGUIString *s)
{
	unsigned int *idptr;
	ATTLIST_DEF **attptr;

	/* first lets see if this ID matches a known attribute name */
	m_atthash.SetCaseSensitive(true);
	attptr=(ATTLIST_DEF **)m_atthash.Find(s->GetString());
	m_atthash.SetCaseSensitive(false);
	if(attptr)
		return((*attptr)->attid);

	m_stoidhash.SetCaseSensitive(true);
	idptr=(unsigned int *)m_stoidhash.Find(s->GetString());
	m_stoidhash.SetCaseSensitive(false);
	if(idptr)
		return(idptr[0]);

	/* add it */
	m_stoidhash.SetCaseSensitive(true);
	m_stoidhash.Add(s->GetString(),&m_nextstoid);
	m_stoidhash.SetCaseSensitive(false);

	/* also save it in indexed list for a reverse lookup */
	m_idtos.GetEntryPtr(m_nextstoid-HTMLATT_NUM)->SetString(s);
	return(m_nextstoid++);
}


kGUIString *kGUIHTMLPageObj::IDToString(unsigned int num)
{
	if(num<HTMLATT_NUM)
		return(&m_attstrings[num]);
	else
		return(m_idtos.GetEntryPtr(num-HTMLATT_NUM));
}

#if 0
//example default rules from HTML 4 spec
//http://www.w3.org/TR/REC-CSS2/sample.html

ADDRESS,
BLOCKQUOTE, 
BODY, DD, DIV, 
DL, DT, 
FIELDSET, FORM,
FRAME, FRAMESET,
H1, H2, H3, H4, 
H5, H6, IFRAME, 
NOFRAMES, 
OBJECT, OL, P, 
UL, APPLET, 
CENTER, DIR, 
HR, MENU, PRE   { display: block }
LI              { display: list-item }
HEAD            { display: none }
TABLE           { display: table }
TR              { display: table-row }
THEAD           { display: table-header-group }
TBODY           { display: table-row-group }
TFOOT           { display: table-footer-group }
COL             { display: table-column }
COLGROUP        { display: table-column-group }
TD, TH          { display: table-cell }
CAPTION         { display: table-caption }
TH              { font-weight: bolder; text-align: center }
CAPTION         { text-align: center }
BODY            { padding: 8px; line-height: 1.33 }
H1              { font-size: 2em; margin: .67em 0 }
H2              { font-size: 1.5em; margin: .83em 0 }
H3              { font-size: 1.17em; margin: 1em 0 }
H4, P,
BLOCKQUOTE, UL,
FIELDSET, FORM,
OL, DL, DIR,
MENU            { margin: 1.33em 0 }
H5              { font-size: .83em; line-height: 1.17em; margin: 1.67em 0 }
H6              { font-size: .67em; margin: 2.33em 0 }
H1, H2, H3, H4,
H5, H6, B,
STRONG          { font-weight: bolder }
BLOCKQUOTE      { margin-left: 40px; margin-right: 40px }
I, CITE, EM,
VAR, ADDRESS    { font-style: italic }
PRE, TT, CODE,
KBD, SAMP       { font-family: monospace }
PRE             { white-space: pre }
BIG             { font-size: 1.17em }
SMALL, SUB, SUP { font-size: .83em }
SUB             { vertical-align: sub }
SUP             { vertical-align: super }
S, STRIKE, DEL  { text-decoration: line-through }
HR              { border: 1px inset }
OL, UL, DIR,
MENU, DD        { margin-left: 40px }
OL              { list-style-type: decimal }
OL UL, UL OL,
UL UL, OL OL    { margin-top: 0; margin-bottom: 0 }
U, INS          { text-decoration: underline }
CENTER          { text-align: center }
BR:before       { content: "\A" }

/* An example of style for HTML 4.0's ABBR/ACRONYM elements */

ABBR, ACRONYM   { font-variant: small-caps; letter-spacing: 0.1em }
A[href]         { text-decoration: underline }
:focus          { outline: thin dotted invert }

/* Begin bidirectionality settings (do not change) */
BDO[DIR="ltr"]  { direction: ltr; unicode-bidi: bidi-override }
BDO[DIR="rtl"]  { direction: rtl; unicode-bidi: bidi-override }

*[DIR="ltr"]    { direction: ltr; unicode-bidi: embed }
*[DIR="rtl"]    { direction: rtl; unicode-bidi: embed }

/* Elements that are block-level in HTML4 */
ADDRESS, BLOCKQUOTE, BODY, DD, DIV, DL, DT, FIELDSET, 
FORM, FRAME, FRAMESET, H1, H2, H3, H4, H5, H6, IFRAME,
NOSCRIPT, NOFRAMES, OBJECT, OL, P, UL, APPLET, CENTER, 
DIR, HR, MENU, PRE, LI, TABLE, TR, THEAD, TBODY, TFOOT, 
COL, COLGROUP, TD, TH, CAPTION 
                { unicode-bidi: embed }
/* End bidi settings */


@media print {
  @page         { margin: 10% }
  H1, H2, H3,
  H4, H5, H6    { page-break-after: avoid; page-break-inside: avoid }
  BLOCKQUOTE, 
  PRE           { page-break-inside: avoid }
  UL, OL, DL    { page-break-before: avoid }
}

@media speech {
  H1, H2, H3, 
  H4, H5, H6    { voice-family: paul, male; stress: 20; richness: 90 }
  H1            { pitch: x-low; pitch-range: 90 }
  H2            { pitch: x-low; pitch-range: 80 }
  H3            { pitch: low; pitch-range: 70 }
  H4            { pitch: medium; pitch-range: 60 }
  H5            { pitch: medium; pitch-range: 50 }
  H6            { pitch: medium; pitch-range: 40 }
  LI, DT, DD    { pitch: medium; richness: 60 }
  DT            { stress: 80 }
  PRE, CODE, TT { pitch: medium; pitch-range: 0; stress: 0; richness: 80 }
  EM            { pitch: medium; pitch-range: 60; stress: 60; richness: 50 }
  STRONG        { pitch: medium; pitch-range: 60; stress: 90; richness: 90 }
  DFN           { pitch: high; pitch-range: 60; stress: 60 }
  S, STRIKE     { richness: 0 }
  I             { pitch: medium; pitch-range: 60; stress: 60; richness: 50 }
  B             { pitch: medium; pitch-range: 60; stress: 90; richness: 90 }
  U             { richness: 0 }
  A:link        { voice-family: harry, male }
  A:visited     { voice-family: betty, female }
  A:active      { voice-family: betty, female; pitch-range: 80; pitch: x-high }
}
#endif

typedef struct
{
	const char *rule;
	const char *value;
}DEFAULTRULES_DEF;

DEFAULTRULES_DEF defrules[]={
	{"a:visited","color: purple"},
	{"a:link","color: blue"},
	{"ul ul ul","list-style-type: square"},
	{"ul ul","list-style-type: circle"},
	{"ul","list-style-type: disc"},
	{"h1","font-size: 2em;margin: .67em 0"},
	{"h2","font-size: 1.5em;margin: .83em 0"},
	{"h3","font-size: 1.17em;margin: 1em 0"},
	{"h4","margin: 1.33em 0"},
	{"h5","font-size: .83em; line-height: 1.17em; margin: 1.67em 0"},
	{"h6","font-size: .67em; margin: 2.33em 0"},
	{"p","margin: 1.33em 0"},
	{"hr","margin-left: auto;margin-right:auto;margin-top:0.5em;margin-bottom:0.5em;border: 1px inset" },
	{"noframes","display: none"},
	{"a[href]","text-decoration: underline"},
	{"a[href]:visited img","border: 3px solid purple"},
	{"a[href]:link img","border: 3px solid blue"},
	{"strong","font-weight: 700"},
	{"b","font-weight: 700"},
	{"em","font-weight: 700"},
	{"sub","font-size: .85em;vertical-align: sub"},
	{"sup","font-size: .85em;vertical-align: super"},
	{"big","font-size: 1.5em;"},
	{"small","font-size: .75em;"},
	{"html","background-color: white"},
	{"strike","text-decoration: line-through"},
	{"u","text-decoration: underline"},
	{"center","text-align: center"},
	{"dd","margin-left: 1.33em"},
	{"ol","margin-left: 2em"},
	{"ul","margin-left: 2em"},
	{"nobr","white-space: nowrap"},
	{"wbr","white-space: normal"},
	{"pre","white-space: pre"}};

/* add a default set of rules */
void kGUIHTMLPageObj::AddDefaultRules(void)
{
	unsigned int i;
	DEFAULTRULES_DEF *dr;
	kGUIString rulename;
	kGUIString rulevalue;

	dr=defrules;
	for(i=0;i<sizeof(defrules)/sizeof(DEFAULTRULES_DEF);++i)
	{
		rulename.SetString(dr->rule);
		rulevalue.SetString(dr->value);
		AddClassStyle(OWNER_SYS,GetStylePriority(),0,&rulename,&rulevalue);
		++dr;
	}
	/* add user rules */
	if( GetSettings()->GetUseUserCSS()==true)
		AddClassStyles(OWNER_USER,GetStylePriority(),0,GetSettings()->GetUserCSS());
}

/* new and improved parse */
void kGUIHTMLPageObj::Parse(bool doprint)
{
	unsigned int i;
	kGUIHTMLObj *renderparent;
	kGUIString tci;

	assert(GetSettings()!=0,"Need to Set Settings!");

	for(i=0;i<HTMLATT_UNKNOWN;++i)
	{
		m_blocked[i]=GetSettings()->GetCSSBlock(i);
		m_traceatt[i]=GetSettings()->GetCSSTrace(i);
	}
	m_scripts.Clear();

	m_defstyle.Clear();
	m_targetobj=0;
	m_iconlinked=false;

	/* purge old rule list, start new one */
	
	PurgeRules();

	/* clear the list back to zero */
	for(i=0;i<HTMLTAG_NUMTAGS;++i)
		m_tagrules[i].m_num=0;
	m_numclasses=0;
	m_classrules.Init(512,64);
	m_curclassrefs.Init(512,64);

	m_numids=0;
	m_idrules.Init(512,64);
	m_curidrefs.Init(512,64);

	m_numlocallinks=0;

	m_nummaps=0;
	m_maps.Init(4,4);

	m_numblinks=0;
	m_blinkdelay=BLINKDELAY;

	m_trackpossiblerules=false;

	PurgeTCICache();

	/* all ID and Class strings get converted to ID numbers, this hash table */
	/* contain the strings and numbers assigned to each ID/Class or other string thingy */
	m_nextstoid=HTMLATT_NUM;
	m_stoidhash.Init(12,sizeof(unsigned int));
	m_idtos.Init(1024,256);

	AddDefaultRules();

	/* reset the hoverlist since new objects will be at new places */
	m_hoverlistsize[0]=0;
	m_hoverlistsize[1]=0;

	delete m_rootobject;
    m_rootobject=new kGUIHTMLObj();
	m_rootobject->SetID(HTMLTAG_ROOT);
	m_rootobject->m_tag=&m_roottag;

	delete m_fixedfgobject;
    m_fixedfgobject=new kGUIHTMLObj();
	m_fixedfgobject->SetID(HTMLTAG_FIXEDROOT);
	m_fixedfgobject->m_page=this;

	if(doprint==false)
	{
		AddObject(m_rootobject);
		AddObject(m_fixedfgobject);
	}
	renderparent=m_rootobject;

	m_errors.Clear();
	m_csserrors.Clear();
	m_css.Clear();
	m_scripts.Clear();
	m_media.Clear();
	m_debug.Clear();
	m_mediahash.Init(12,sizeof(int));

	/* clear the loadpendig flag for any aborted images */
	for(i=0;i<m_numimages;++i)
	{
		kGUIOnlineImage *image;

		image=m_images.GetEntry(i);
		if(image->GetLoadAborted()==true)
		{
			image->SetLoadAborted(false);
			image->SetLoadTriggered(false);
		}
	}

	/* default to english please */
	{
		kGUIString en;

		en.SetString("en");
		m_lang=StringToID(&en);
	}

	if(!m_fp)
		return;

	/* Parse returns false if this is NOT a html page, so it must be an image or */
	/* movie or other special object */
	m_singlemode=false;
	if(Parse(renderparent,m_fp,m_len,&m_header)==false)
	{
		/* can this be an image? */
		kGUIImageObj *image;

		image=new kGUIImageObj();
		image->SetMemory((const unsigned char *)m_fp,m_len);
		if(image->IsValid())
		{
			kGUIHTMLObj *newobj;

			/* it is an image, not a html page */
			m_singlemode=true;
			image->SetPos(0,0);
			image->SetSize(image->GetImageWidth(),image->GetImageHeight());
			image->SetEventHandler(this,CALLBACKNAME(RightClickEvent));
			image->SetAnimate(true);

			newobj=new kGUIHTMLObj(renderparent,this,&m_singleobjtag);
			newobj->SetOwnerURL(StringToIDcase(&m_url));
			renderparent->AddStyleChild(newobj);
			newobj->SetOwnerURL(StringToIDcase(&m_url));
			newobj->m_obj.m_singleobj=image;
			newobj->AddRenderObject(image);

			/* make icon be the same as the image */
			m_iconcallback.Call(this,image);
		}
		else
			delete image;

		/* try using one of the plugins */
		if(m_plugins)
		{
			for(i=0;(i<m_plugins->GetNumPlugins() && m_singlemode==false);++i)
			{
				kGUIHTMLPluginObj *po;
				kGUIHTMLObj *newobj;

				po=m_plugins->GetPlugin(i)->New();
				po->GetDH()->SetMemory((const unsigned char *)m_fp,m_len);
				if(po->Open()==true)
				{
					/* yes it was valid, so let's show it */
					m_singlemode=true;

					/* get an object to add the right-click to */
					po->GetObj()->SetEventHandler(this,CALLBACKNAME(RightClickEvent));

					newobj=new kGUIHTMLObj(renderparent,this,&m_singleobjtag);
					newobj->SetOwnerURL(StringToIDcase(&m_url));
					newobj->SetAlign(ALIGN_CENTER);
					newobj->SetVAlign(VALIGN_BOTTOM);
					renderparent->AddStyleChild(newobj);
					newobj->m_obj.m_singleobj=po;
					newobj->AddRenderObject(po);
				}
				else
					delete po;
			}
		}
		if(m_singlemode==false)
		{
			/* is it plain text? */
			if(!strcmpin(m_type.GetString(),"text",4))
			{
				kGUITextObj *text;
				kGUIHTMLObj *newobj;

				/* it is an image, not a html page */
				m_singlemode=true;
				text=new kGUITextObj();
				text->SetPos(0,0);
				text->SetSize(100,500);
				/* todo: get encoding from http header */
				text->SetString((const char *)m_fp,m_len);

				newobj=new kGUIHTMLObj(renderparent,this,&m_singleobjtag);
				newobj->SetOwnerURL(StringToIDcase(&m_url));
				renderparent->AddStyleChild(newobj);
				newobj->SetOwnerURL(StringToIDcase(&m_url));
				newobj->m_obj.m_singleobj=text;
				newobj->AddRenderObject(text);
			}
			else
			{
				/* put up i don't know page */
			}
		}
	}

	/* ok, now scan for meta tags */
	m_refreshurl.Clear();
	m_refreshdelay=0;

	PreProcess(&tci,m_rootobject,false);

	/* save parse errors */
	m_parseerrors.SetString(&m_errors);
}

bool kGUIHTMLPageObj::ExtractFromHeader(kGUIString *header,const char *prefix,kGUIString *s,unsigned int *poffset)
{
	char *l;
	char *le;
	unsigned int offset;

	if(poffset)
		offset=*(poffset);
	else
		offset=0;

	l=strstri ( header->GetString()+offset, prefix );
	if(!l)
	{
		s->Clear();
		return(false);
	}
	/* skip 'prefix:' */
	l+=strlen(prefix);
	le=l;
	while(le[0]>=' ')
		++le;

	s->SetString(l,(int)(le-l));

	/* return offset to just past end of command */
	if(poffset)
		*(poffset)=(unsigned int)(le-header->GetString());
	return(true);
}

void kGUIHTMLPageObj::Link(kGUIString *linkline)
{
	unsigned int g,numgroups;
	unsigned int a,num;
	kGUIStringSplit sg;
	kGUIStringSplit ss;
	kGUIString url;
	kGUIString media;
	kGUIString *w;
	kGUIOnlineLink *lo;

	numgroups=sg.Split(linkline,",");
	for(g=0;g<numgroups;++g)
	{
		kGUIString fullurl;
		kGUIString rel;
		kGUIString type;

		num=ss.Split(sg.GetWord(g),";");
		for(a=0;a<num;++a)
		{
			w=ss.GetWord(a);
			if(w->GetChar(0)=='<')
			{
				/* got a URL */

				url.SetString(ss.GetWord(0));
				url.Replace("<","");
				url.Replace(">","");
				MakeURL(&m_url,&url,&fullurl);
			}
			else if(strnicmp(w->GetString(),"rel=",4))
			{
				rel.SetString(w);
				rel.Replace("rel=","");
				rel.RemoveQuotes();
			}
			else if(strnicmp(w->GetString(),"type=",5))
			{
				/* we don't need this do we? */
				type.SetString(w);
				type.Replace("type=","");
				type.RemoveQuotes();
			}
			else
			{
				m_errors.ASprintf("unknown META LINK attribute ('%s')!\n",w->GetString());
			}
		}

		/* ok, start loading */
		if(fullurl.GetLen() && rel.GetLen())
		{
			/* todo: make sure rel=stylesheet */
			lo=LocateLink(&fullurl,&m_url,LINKTYPE_CSS,&media);
			lo->SetPriority(GetStylePriority());
			if(lo->GetLoadPending()==false)
				AttachLink(lo);
		}
		else
		{
			m_errors.ASprintf("META LINK ( or HEADER ) not enough info to load ('%s')!\n",sg.GetWord(g)->GetString());
		}
	}
}

void kGUIHTMLPageObj::ExpandClassList(unsigned int num)
{
	/* is the table getting bigger? */
	while(num>=m_numclasses)
	{
		m_curclassrefs.SetEntry(m_numclasses,0);
		m_classrules.GetEntryPtr(m_numclasses)->m_num=0;
		++m_numclasses;
	}
}

void kGUIHTMLPageObj::ExpandIDList(unsigned int num)
{
	/* is the table getting bigger? */
	while(num>=m_numids)
	{
		m_curidrefs.SetEntry(m_numids,0);
		m_idrules.GetEntryPtr(m_numids)->m_num=0;
		++m_numids;
	}
}

/* this is a one-time preprocess that is called after the tree has been parsed */
/* it is also called each time a frame has been loaded and is just run on that */
/* newly loaded frame */

void kGUIHTMLPageObj::PreProcess(kGUIString *tci,kGUIHTMLObj *obj,bool inframe)
{
	unsigned int i;
	unsigned int j;
	kGUIHTMLAttrib *att;
	CID_DEF cid;
	kGUIStringSplit ss;
	kGUIString *word;
	int oldtcilen;
	kGUIHTMLRuleCache **cacheptr;

	/* scan attributes for class and ID and build the cid array */
	obj->m_numcids=0;
	obj->m_cids.Init(2,2);
	for(i=0;i<obj->GetNumAttributes();++i)
	{

		/* todo, force class/id to be in alphabetical order so that we get better matching */
		/* tag class="a,b,c" id="d" will then match */
		/* tag id="d" class="c,b,a" */

		att=obj->GetAttrib(i);
		switch(att->GetID())
		{
		case HTMLATT_CLASS:
			if(!strstr(att->GetValue()->GetString()," "))
			{
				if(kGUIHTMLRule::ValidateName(att->GetValue()))
				{
					cid.m_type=CID_CLASS;
					if(ClassUseCase())
						cid.m_id=StringToIDcase(att->GetValue());
					else
						cid.m_id=StringToID(att->GetValue());
					ExpandClassList(cid.m_id);
					obj->m_cids.SetEntry(obj->m_numcids++,cid);
				}
				else
				{
					//fuck
				}
			}
			else
			{
				/* split the space seperated class list */
				ss.Split(att->GetValue()," ");
				for(j=0;j<ss.GetNumWords();++j)
				{
					word=ss.GetWord(j);
					if(kGUIHTMLRule::ValidateName(word))
					{
						cid.m_type=CID_CLASS;
						if(ClassUseCase())
							cid.m_id=StringToIDcase(word);
						else
							cid.m_id=StringToID(word);
						ExpandClassList(cid.m_id);
						obj->m_cids.SetEntry(obj->m_numcids++,cid);
					}
					else
					{
						/* error */
					}
				}
			}
		break;
		case HTMLATT_ID:
			if(kGUIHTMLRule::ValidateName(att->GetValue()))
			{
				cid.m_type=CID_ID;
//				cid.m_id=StringToIDcase(att->GetValue());
				cid.m_id=StringToID(att->GetValue());
				ExpandIDList(cid.m_id);
				obj->m_cids.SetEntry(obj->m_numcids++,cid);

				/* add to list of local links */
				AddLocalLink(att->GetValue(),obj);
	
				/* keep track of target object for css :target: rule pseudo selector */
				if(m_target.GetLen())
				{
					if(!strcmp(att->GetValue()->GetString(),m_target.GetString()))
						m_targetobj=obj;
				}
			}
			else
			{
				/* error */
			}
		break;
		}
	}

	/* build the TCI string */
	oldtcilen=AddTCI(tci,obj);

	/* is this TCI situation already in the cache? */
	cacheptr=(kGUIHTMLRuleCache **)m_tcicache.Find(tci->GetString());
	if(cacheptr)
		obj->m_rulecache=*(cacheptr);
	else
	{
		/* allocate one and add it */
		obj->m_rulecache=new kGUIHTMLRuleCache();
		m_tcicache.Add(tci->GetString(),&obj->m_rulecache);
	}

	/* look for SCRIPTS and META TAGS etc */
	switch(obj->GetID())
	{
	case HTMLTAG_HTML:
		att=obj->FindAttrib(HTMLATT_LANG);
		if(att)
			m_lang=StringToID(att->GetValue());
	break;
	case HTMLTAG_XMLSTYLESHEET:
	case HTMLTAG_LINK:
	{
		kGUIString url;
		kGUIString type;
		kGUIString rel;
		kGUIString media;
		kGUIString title;
		unsigned int linktype;

		for(i=0;i<obj->GetNumAttributes();++i)
		{
			att=obj->GetAttrib(i);
			switch(att->GetID())
			{
			case HTMLATT_HREF:
				MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&url);
			break;
			case HTMLATT_TYPE:
				type.SetString(att->GetValue());
			break;
			case HTMLATT_REL:
				rel.SetString(att->GetValue());
			break;
			case HTMLATT_MEDIA:
				media.SetString(att->GetValue());
			break;
			case HTMLATT_TITLE:
				title.SetString(att->GetValue());
			break;
			}
		}

		/* todo, change rel and type to use CONST IDS instead of strcmp */

		/* examine rel first if defined, if not defined then look at type */
		linktype=LINKTYPE_UNKNOWN;
		if(rel.GetLen())
		{
			if(!stricmp(rel.GetString(),"stylesheet") || !stricmp(rel.GetString(),"stylesheet alternate"))
				linktype=LINKTYPE_CSS;
			else if( !stricmp(rel.GetString(),"shortcut icon") || !stricmp(rel.GetString(),"icon") )
				linktype=LINKTYPE_ICON;
		}
		else if(type.GetLen())
		{
			if(!stricmp(type.GetString(),"text/css"))
				linktype=LINKTYPE_CSS;
			else if( !stricmp(type.GetString(),"image/x-icon") || !stricmp(type.GetString(),"image/x-icon") )
				linktype=LINKTYPE_ICON;
		}

		/* if a defaultstyle is defined then we will only use it and ignore any others */
		if((linktype==LINKTYPE_CSS) && (m_defstyle.GetLen()) && (title.GetLen()))
		{
			if(stricmp(m_defstyle.GetString(),title.GetString()))
				linktype=LINKTYPE_UNKNOWN;	
		}

		if(linktype!=LINKTYPE_UNKNOWN)
		{
			kGUIOnlineLink *lo;

			if(linktype==LINKTYPE_ICON)
			{
				/* icon is manually linked so no need to automagically look at /favicon.ico */
				m_iconlinked=true;
			}

			lo=LocateLink(&url,&m_url,linktype,&media);
			if(linktype==LINKTYPE_CSS)
				lo->SetPriority(GetStylePriority());
			if(lo->GetLoadPending()==false)
				AttachLink(lo);
		}
	}
	break;
	case HTMLTAG_META:
		att=obj->FindAttrib(HTMLATT_HTTP_EQUIV);
		if(att)
		{
			switch(att->GetVID())
			{
			case HTMLCONST_REFRESH:
				if(inframe==false)
				{
					att=obj->FindAttrib(HTMLATT_CONTENT);
					if(att)
					{
						char *u;
						kGUIString url;

						u=strstri(att->GetValue()->GetString(),"url=");
						if(u)
						{
							url.SetString(u+4);
							url.RemoveQuotes();
							MakeURL(&m_url,&url,&m_refreshurl);

							/* todo, add an option to disable this or ASK permission */
							m_refreshdelay=(int)max(1,(atof(att->GetValue()->GetString())*TICKSPERSEC));
							
						}
					}
				}
			break;
			case HTMLCONST_DEFAULT_STYLE:
				att=obj->FindAttrib(HTMLATT_CONTENT);
				if(att)
					m_defstyle.SetString(att->GetValue());
			break;
			case HTMLCONST_LINK:	/*<meta http-equiv="link" content="<http.css>; rel=stylesheet">*/
				att=obj->FindAttrib(HTMLATT_CONTENT);
				if(att)
					Link(att->GetValue());
			break;
			}
		}
	break;
	case HTMLTAG_STYLE:
	{
		kGUIString media;
		kGUIHTMLAttrib *att;

		att=obj->FindAttrib(HTMLATT_MEDIA);
		if(att)
		{
			media.SetString(att->GetValue());
			media.Replace("\t"," ");
		}

		/* todo: check to make sure type is text/css? */
		att=obj->FindAttrib(HTMLATT_CONTENT);
		if(att)
		{
			if(ValidMedia(&media))
				AddClassStyles(OWNER_AUTHOR,GetStylePriority(),&m_url,att->GetValue());
		}
	}
	break;
	case HTMLTAG_FRAME:
	case HTMLTAG_IFRAME:
		/* this will be called again when the object is loaded, so skip the 2nd time */
		if(obj->m_obj.m_linked==0)
		{
			kGUIString url;
			kGUIOnlineLink *link;
			kGUIString media;		/* todo: is a media type possible here? */

			att=obj->FindAttrib(HTMLATT_SRC);
			if(att)
			{
				MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&url);
				AddMedia(&url);
				link=LocateLink(&url,&m_url,LINKTYPE_HTML,&media);
				obj->m_obj.m_linked=link;
			}
			else
			{
				m_errors.ASprintf("Frame/IFrame has no SRC attribute!!\n");
			}
		}
	break;
	case HTMLTAG_OBJECT:
	{
		kGUIString url;
		kGUIString type;
		kGUIString media;

		for(i=0;i<obj->GetNumAttributes();++i)
		{
			att=obj->GetAttrib(i);
			switch(att->GetID())
			{
			case HTMLATT_DATA:
				MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&url);
			break;
			case HTMLATT_TYPE:
				type.SetString(att->GetValue());
			break;
			case HTMLATT_MEDIA:
				media.SetString(att->GetValue());
			break;
			}
		}
		/* todo: error if no url found? */

		if(url.GetLen())
		{
			obj->m_obj.m_linked=LocateLink(&url,&m_url,LINKTYPE_UNKNOWN,&media);
			if(obj->m_obj.m_linked->GetLoadPending()==false)
				obj->DetectObject();
		}
		else
		{
			/*error, no URL for object*/
		}
	}
	break;
	case HTMLTAG_A:
	{
		kGUIString url;

		att=obj->FindAttrib(HTMLATT_HREF);
		if(att)
		{
			assert(obj->m_obj.m_linkobj==0,"Error!");
			obj->m_obj.m_linkobj=new kGUIHTMLLinkObj(this,obj);
			MakeURL(&m_url,att->GetValue(),&url);
//			kGUI::Trace("Making url (in=%s,out=%s) page='%s'\n",att->GetValue()->GetString(),url.GetString(),m_url.GetString());
			obj->m_obj.m_linkobj->SetURL(&url);
			obj->m_obj.m_linkobj->SetReferrer(&m_url);
		}
		else
		{
			/* local link ? '#xxx' */
			att=obj->FindAttrib(HTMLATT_NAME);
			if(att)
			{
				/* add to list of local links */
				AddLocalLink(att->GetValue(),obj);
	
				/* keep track of target object for css :target: rule pseudo selector */
				if(m_target.GetLen())
				{
					if(!strcmp(att->GetValue()->GetString(),m_target.GetString()))
						m_targetobj=obj;
				}
			}
		}
	}
	break;
	case HTMLTAG_BUTTON:
		att=obj->FindAttrib(HTMLATT_CONTENT);
		if(att)
			obj->m_obj.m_buttontextobj->SetString(att->GetValue());
		else
			obj->m_obj.m_buttontextobj->SetString(" ");	/* default text for button */
	break;
	case HTMLTAG_IMG:
	{
		kGUIString url;

		att=obj->FindAttrib(HTMLATT_SRC);
		if(att)
		{
			MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&url);
			obj->m_obj.m_imageobj->SetURL(this,&url,IDToString(obj->GetOwnerURL()));
		}
	}
	break;
	case HTMLTAG_MAP:
	{
		kGUIHTMLMap *map;
		kGUIHTMLArea *area;
		kGUIHTMLObj *child;
		unsigned int shape;
		kGUIHTMLAttrib *catt;
		kGUIPoint2 p;

		/* allocate a map object */
		map=m_maps.GetEntryPtr(m_nummaps++);
		map->Init();
		map->SetTag(obj);

		/* collect the areas and add them to the map */
		for(i=0;i<obj->m_numstylechildren;++i)
		{
			child=obj->m_stylechildren.GetEntry(i);
			if(child->GetID()==HTMLTAG_AREA)
			{
				att=child->FindAttrib(HTMLATT_SHAPE);
				if(!att)
					shape=HTMLCONST_RECT;	/* if absent then rect is assumed */
				else
					shape=att->GetVID();

				switch(shape)
				{
				case HTMLCONST_CIRCLE:
				case HTMLCONST_CIRC:
				case HTMLCONST_RECT:
				case HTMLCONST_RECTANGLE:
					catt=child->FindAttrib(HTMLATT_COORDS);
					if(catt)
					{
						/* split the coords into x1,y1,x2,y2 */
						ss.Split(catt->GetValue(),",");

						area=map->AddArea(child,shape);
						for(j=0;j<ss.GetNumWords();++j)
						{
							word=ss.GetWord(j);
							p.x=word->GetInt();
							p.y=0;	/*unused but set to stop the compiler from complaining*/
							area->AddPoint(p);
						}
					}
					else
					{
						/* error, rect area has no coords! */
						m_errors.ASprintf("Rect Area has no coords tag!\n");
					}
				break;
				case HTMLCONST_POLY:
				case HTMLCONST_POLYGON:
					catt=child->FindAttrib(HTMLATT_COORDS);
					if(catt)
					{
						/* split the coords into x1,y1,x2,y2 */
						ss.Split(catt->GetValue(),",");

						area=map->AddArea(child,shape);
						area->AllocPoints(ss.GetNumWords()>>1);
						for(j=0;j<ss.GetNumWords();j+=2)
						{
							p.x=ss.GetWord(j)->GetInt();
							p.y=ss.GetWord(j+1)->GetInt();
							area->AddPoint(p);
						}
					}
					else
					{
						/* error, rect area has no coords! */
						m_errors.ASprintf("Rect Area has no coords tag!\n");
					}
				break;
				case HTMLCONST_DEFAULT:
					area=map->AddArea(child,shape);
				break;
				default:
					/* unknown area shape */
				break;
				}
			}
			else
			{
				/* huh? what is this thing doing here? */
			}
		}
	}
	break;
	case HTMLTAG_INPUT:
		/* this tag is only used when the type has not been decided yet */

		att=obj->FindAttrib(HTMLATT_TYPE);
		if(!att)
			goto assumetext;
		else
		{
			switch(att->GetVID())
			{
			case HTMLCONST_HIDDEN:
				obj->SetSubID(HTMLSUBTAG_INPUTHIDDEN);
			break;
			case HTMLCONST_RADIO:
				obj->SetSubID(HTMLSUBTAG_INPUTRADIO);
				obj->m_obj.m_radioobj=new kGUIRadioObj();
				obj->AddRenderObject(obj->m_obj.m_radioobj);
				obj->m_obj.m_radioobj->SetEventHandler(obj,CALLBACKCLASSNAME(kGUIHTMLObj,RadioChanged));
				obj->m_obj.m_radioobj->SetSelected(obj->FindAttrib(HTMLATT_CHECKED)?true:false);
			break;
			case HTMLCONST_SUBMIT:
				obj->SetSubID(HTMLSUBTAG_INPUTBUTTONSUBMIT);
				obj->m_obj.m_buttontextobj=new kGUIHTMLButtonTextObj(obj,HTMLCONST_SUBMIT);
				obj->AddRenderObject(obj->m_obj.m_buttontextobj);
				obj->m_obj.m_buttontextobj->SetString(" Submit Query ");	/* default text for button */
getbuttonval:;
				att=obj->FindAttrib(HTMLATT_VALUE);
				if(att)
				{
					kGUIString s,s2;

					/* todo: is this needed, should it not be already done at this point? */
					s.SetString(att->GetValue());
					kGUIXMLCODES::Shrink(&s,&s2);
					obj->m_obj.m_buttontextobj->SetString(s2.GetString());
				}
			break;
			case HTMLCONST_RESET:
				obj->SetSubID(HTMLSUBTAG_INPUTBUTTONRESET);
				obj->m_obj.m_buttontextobj=new kGUIHTMLButtonTextObj(obj,HTMLCONST_RESET);
				obj->AddRenderObject(obj->m_obj.m_buttontextobj);
				obj->m_obj.m_buttontextobj->SetString(" Reset ");	/* default text for button */
				goto getbuttonval;
			break;
			case HTMLCONST_BUTTON:
				obj->SetSubID(HTMLSUBTAG_INPUTBUTTON);
				obj->m_obj.m_buttontextobj=new kGUIHTMLButtonTextObj(obj,HTMLCONST_BUTTON);
				obj->AddRenderObject(obj->m_obj.m_buttontextobj);
				obj->m_obj.m_buttontextobj->SetString(" ");	/* default text for button */
				goto getbuttonval;
			break;
			case HTMLCONST_CHECKBOX:
				obj->SetSubID(HTMLSUBTAG_INPUTCHECKBOX);
				obj->m_obj.m_tickobj=new kGUITickBoxObj();
				obj->AddRenderObject(obj->m_obj.m_tickobj);
				obj->m_obj.m_tickobj->SetSelected(obj->FindAttrib(HTMLATT_CHECKED)?true:false);
			break;
			case HTMLCONST_TEXT:
assumetext:;
				obj->SetSubID(HTMLSUBTAG_INPUTTEXTBOX);
				obj->m_obj.m_inputobj=new kGUIHTMLInputBoxObj();
				obj->m_obj.m_inputobj->SetEventHandler(this,CALLBACKNAME(CheckSubmit));

				obj->AddRenderObject(obj->m_obj.m_inputobj);
settextboxsize:;
				att=obj->FindAttrib(HTMLATT_MAXLENGTH);
				if(att)
					obj->m_obj.m_inputobj->SetMaxLen(att->GetValue()->GetInt());
				/* default string value? */
				att=obj->FindAttrib(HTMLATT_VALUE);
				if(att)
					obj->m_obj.m_inputobj->SetString(att->GetValue());
				else
					obj->m_obj.m_inputobj->Clear();
			break;
			case HTMLCONST_FILE:
				obj->SetSubID(HTMLSUBTAG_INPUTFILE);
				obj->m_obj.m_inputobj=new kGUIHTMLInputBoxObj();
				obj->AddRenderObject(obj->m_obj.m_inputobj);
				goto settextboxsize;
			break;
			case HTMLCONST_PASSWORD:
				obj->SetSubID(HTMLSUBTAG_INPUTTEXTBOX);
				obj->m_obj.m_inputobj=new kGUIHTMLInputBoxObj();
				obj->m_obj.m_inputobj->SetPassword(true);	/* show chars as '*' */
				obj->AddRenderObject(obj->m_obj.m_inputobj);
			break;
			case HTMLCONST_IMAGE:
				obj->SetSubID(HTMLSUBTAG_INPUTBUTTONIMAGE);
				obj->m_obj.m_buttonimageobj=new kGUIHTMLButtonImageObj(obj);
				obj->AddRenderObject(obj->m_obj.m_buttonimageobj);
				att=obj->FindAttrib(HTMLATT_SRC);
				if(att)
				{
					kGUIString url;

					MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&url);
					obj->m_obj.m_buttonimageobj->SetURL(this,&url,&m_url);
				}
			break;
			default:
				m_errors.ASprintf("unknown input type ('%s')!\n",att->GetValue()->GetString());
				obj->SetID(HTMLTAG_UNKNOWN);
			break;
			}
		}
	break;
	case HTMLTAG_SELECT:
	{
		unsigned int i,e;
		int numshow=1;
		int numentries;
		kGUIHTMLObj *option;
		kGUIString text;

		/* count number of option tags */
		numentries=0;
		for(i=0;i<obj->m_numstylechildren;++i)
		{
			option=obj->m_stylechildren.GetEntry(i);
			if(option->GetID()==HTMLTAG_OPTION)
				++numentries;
			else if(option->GetID()!=HTMLTAG_OPTGROUP)
			{
				kGUIString ts;

				GetTagDesc(option,&ts);
				m_errors.ASprintf("Tag inside <SELECT> is not <OPTION> or <OPTGROUP> '%s'!\n",ts.GetString());
			}
		}

		/* if size is 1 or undefined then it is a combo (popup) box */
		/* if size is >1 then it is a listbox and we show 'that' many lines */
		att=obj->FindAttrib(HTMLATT_SIZE);
		if(att)
			numshow=att->GetValue()->GetInt();
		if(numshow>numentries)
			numshow=numentries;
		if(numshow<1)
			numshow=1;

		if(numshow==1)
		{
			kGUIComboBoxObj *comboobj;

			comboobj=new kGUIComboBoxObj();

			obj->m_obj.m_comboboxobj=comboobj;
			comboobj->SetPos(0,0);
			comboobj->SetNumEntries(numentries);
			e=0;
			for(i=0;i<obj->m_numstylechildren;++i)
			{
				option=obj->m_stylechildren.GetEntry(i);
				if(option->GetID()==HTMLTAG_OPTION)
				{
					/* set option text pointer to combo text entry so it can set the text style to the combo box */
					option->m_obj.m_text=comboobj->GetEntryTextPtr(e);

					/* get option text */
					att=option->FindAttrib(HTMLATT_CONTENT);
					if(att)
						text.SetString(att->GetValue());
					else
						text.SetString("xxx");

					/* get option text */
					att=option->FindAttrib(HTMLATT_VALUE);

					/* add entry to combobox */
					comboobj->SetEntry(e,&text,att!=0?att->GetValue():&text);

					/* select this one? */
					att=option->FindAttrib(HTMLATT_SELECTED);
					if(att)
						comboobj->SetSelection(e);

					++e;
				}
			}
			obj->AddRenderObject(comboobj);
		}
		else
		{
			kGUIListboxObj *listobj;

			/* if we are showing more than 1 row then we use a listbox */
			obj->SetSubID(HTMLSUBTAG_INPUTLISTBOX);
			listobj=new kGUIListboxObj();
			obj->m_obj.m_listboxobj=listobj;
			listobj->SetPos(0,0);
			listobj->SetAllowMultiple(obj->FindAttrib(HTMLATT_MULTIPLE)!=0);
			listobj->SetNumEntries(numentries);

			e=0;
			for(i=0;i<obj->m_numstylechildren;++i)
			{
				option=obj->m_stylechildren.GetEntry(i);
				if(option->GetID()==HTMLTAG_OPTION)
				{
					/* set option text pointer to combo text entry so it can set the text style to the combo box */
					option->m_obj.m_text=listobj->GetEntryTextPtr(e);

					/* get option text */
					att=option->FindAttrib(HTMLATT_CONTENT);
					if(att)
						text.SetString(att->GetValue());
					else
						text.SetString("xxx");

					/* get option text */
					att=option->FindAttrib(HTMLATT_VALUE);

					/* add entry to listbox */
					listobj->SetEntry(e,&text,att!=0?att->GetValue():&text);

					/* select this one? */
					att=option->FindAttrib(HTMLATT_SELECTED);
					if(att)
					{
						listobj->SelectRow(e);
						listobj->GotoRow(e,false);		/* false=don't clear current selections */
					}
					++e;
				}
			}
			obj->AddRenderObject(listobj);
		}
	}
	break;
	case HTMLTAG_FORM:
	{
		kGUIHTMLFormObj *form;
		int mode=FORMMODE_GET;
		kGUIString actionurl;

		form=obj->m_obj.m_formobj;
		form->SetReferrer(&m_url);

		att=obj->FindAttrib(HTMLATT_ACTION);
		if(att)
		{
			/* generate relative URL */
			MakeURL(IDToString(obj->GetOwnerURL()),att->GetValue(),&actionurl);
		}
		else
			actionurl.SetString(IDToString(obj->GetOwnerURL()));

		form->SetAction(&actionurl);

		att=obj->FindAttrib(HTMLATT_NAME);
		if(att)
			form->SetName(att->GetValue());

		att=obj->FindAttrib(HTMLATT_METHOD);
		if(att)
		{
			if(!stricmp(att->GetValue()->GetString(),"post"))
				mode=FORMMODE_POST;
			else if(!stricmp(att->GetValue()->GetString(),"get"))
				mode=FORMMODE_GET;
			else
			{
				//error!, unknown mode
			}
			form->SetMode(mode);
		}
	}
	break;
	case HTMLTAG_TEXTAREA:
	{
		kGUIHTMLInputBoxObj *inputobj;

		inputobj=obj->m_obj.m_inputobj;

		inputobj->SetAllowEnter(true);
		inputobj->SetAllowTab(true);
		att=obj->FindAttrib(HTMLATT_CONTENT);	/* inline text is added as a comment */
		if(att)
			inputobj->SetString(att->GetValue());

		att=obj->FindAttrib(HTMLATT_WRAP);
		if(att)
		{
			switch(att->GetVID())
			{
			case HTMLCONST_VIRTUAL:
				inputobj->SetWrap(true);
			break;
			case HTMLCONST_HARD:
				inputobj->SetWrap(true);
			break;
			case HTMLCONST_SOFT:
				inputobj->SetWrap(true);
			break;
			case HTMLCONST_OFF:
				inputobj->SetWrap(false);
			break;
			default:
				m_errors.ASprintf("unknown attribute for WRAP='/%s'\n",att->GetValue());
			break;
			}
		}
	}
	break;
	}

	for(i=0;i<obj->m_numstylechildren;++i)
		PreProcess(tci,obj->m_stylechildren.GetEntry(i),inframe);

	tci->Clip(oldtcilen);
}

kGUIHTMLObj *kGUIHTMLPageObj::LocateLocalLink(kGUIString *name)
{
	unsigned int i;
	unsigned int nameid;
	kGUIHTMLLocalLinkObj *ll;

	nameid=StringToID(name);
	for(i=0;i<m_numlocallinks;++i)
	{
		ll=m_locallinks.GetEntryPtr(i);
		if(ll->GetNameID()==nameid)
			return(ll->GetObj());
	}
	return(0);	/* named object wasn't found on this page */
}

enum
{
POPMENU_VIEW,
POPMENU_VIEWTAB,
POPMENU_SAVE,
POPMENU_SAVELINK,
POPMENU_OPENLINKTAB
};

/* user right clicked on something */

void kGUIHTMLPageObj::RightClickEvent(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_RIGHTCLICK)
		RightClick(0,HTMLTAG_SINGLEOBJ);
}

void kGUIHTMLPageObj::RightClick(void *obj,int tag)
{
	kGUIString name;

	m_clickobj=obj;
	m_clicktag=tag;

	switch(tag)
	{
	case HTMLTAG_SINGLEOBJ:
		name.SetString(m_url.GetString()+m_urlroot.GetLen());
		kGUI::MakeFilename(&m_savedir,&name,&m_savefn);

		m_popmenu.SetNumEntries(1);
		m_popmenu.SetEntry(0,"Save Item As",POPMENU_SAVE);
	break;
	case HTMLTAG_LINK:
		m_popmenu.SetNumEntries(1);
		m_popmenu.SetEntry(0,"Open Link in New tab",POPMENU_OPENLINKTAB);
	break;
	case HTMLTAG_IMG:
		kGUIOnlineImageObj *image=static_cast<kGUIOnlineImageObj *>(obj);
		kGUIString urlroot,urlbase;
	
		kGUI::ExtractURL(image->GetURL(),&urlbase,&urlroot);
		name.SetString(image->GetURL()->GetString()+urlroot.GetLen());
		kGUI::MakeFilename(&m_savedir,&name,&m_savefn);

		if(image->GetLink())
			m_popmenu.SetNumEntries(3+2);
		else
			m_popmenu.SetNumEntries(3);
		m_popmenu.SetEntry(0,"View Image",POPMENU_VIEW);
		m_popmenu.SetEntry(1,"View Image in New Tab",POPMENU_VIEWTAB);
		m_popmenu.SetEntry(2,"Save Image As",POPMENU_SAVE);
		if(image->GetLink())
		{
			m_popmenu.SetEntry(3,"Open Link in New Tab",POPMENU_OPENLINKTAB);
			m_popmenu.SetEntry(4,"Save Link As",POPMENU_SAVELINK);
		}
	break;
	}
	m_popmenu.Activate(kGUI::GetMouseX(),kGUI::GetMouseY());
}

void kGUIHTMLPageObj::DoPopMenu(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_SELECTED)
	{
		kGUIFileReq *req;

		switch(m_popmenu.GetSelection())
		{
		case POPMENU_SAVE:
			req=new kGUIFileReq(FILEREQ_SAVE,m_savefn.GetString(),0,this,CALLBACKNAME(SaveAs));
		break;
		case POPMENU_SAVELINK:
			/* todo */
		break;
		case POPMENU_VIEW:
			if(m_clicktag==HTMLTAG_IMG)
			{
				kGUIOnlineImageObj *img;

				img=static_cast<kGUIOnlineImageObj *>(m_clickobj);
				Click(img->GetURL(),GetURL());
			}
		break;
		case POPMENU_VIEWTAB:
			if(m_clicktag==HTMLTAG_IMG)
			{
				kGUIOnlineImageObj *img;

				img=static_cast<kGUIOnlineImageObj *>(m_clickobj);
				ClickNewTab(img->GetURL(),GetURL());
			}
		break;
		case POPMENU_OPENLINKTAB:
		{
			kGUIHTMLLinkObj *link;
			if(m_clicktag==HTMLTAG_LINK)
				link=static_cast<kGUIHTMLLinkObj *>(m_clickobj);
			else if(m_clicktag==HTMLTAG_IMG)
				link=static_cast<kGUIOnlineImageObj *>(m_clickobj)->GetLink();
			else
			{
				assert(false,"Unhandled TAG \n");
				link=0;
			}
			ClickNewTab(link->GetURL(),link->GetReferrer());
		}
		break;
		}
	}
}

void kGUIHTMLPageObj::SaveAs(kGUIFileReq *req,int closebutton)
{
	if(closebutton==MSGBOX_OK)
	{
		m_savefn.SetString(req->GetFilename());
		if(kGUI::FileExists(m_savefn.GetString())==true)
		{
			/* replace, are you sure? */
			kGUIMsgBoxReq *box;
			box=new kGUIMsgBoxReq(MSGBOX_YES|MSGBOX_NO,this,CALLBACKNAME(AskOverwrite),true,"File '%s' Exists, Overwrite?!",m_savefn.GetString());
		}
		else
			DoSaveAs();
	}
}

void kGUIHTMLPageObj::AskOverwrite(int closebutton)
{
	if(closebutton==MSGBOX_YES)
		DoSaveAs();
}

void kGUIHTMLPageObj::DoSaveAs(void)
{
	DataHandle dh;
	kGUIString longfn;
	kGUIString path;

	/* save directory for next time */
	longfn.SetString(m_savefn.GetString());
	kGUI::ExtractPath(&longfn,&path);
	m_savedir.SetString(&path);

	dh.SetFilename(m_savefn.GetString());

	if(m_clicktag==HTMLTAG_SINGLEOBJ)
	{
		dh.OpenWrite("wb");
		dh.Write((const unsigned char *)m_fp,m_len);
		dh.Close();
	}
	else if(m_clicktag==HTMLTAG_IMG)
	{
		DataHandle *idh;
		Array<unsigned char>buf;
		unsigned long size;

		/* get datahandle from image */
		kGUIOnlineImageObj *image=static_cast<kGUIOnlineImageObj *>(m_clickobj);

		idh=image->GetDataHandle();
		size=(unsigned long)idh->GetSize();

		buf.Alloc(size);
		idh->Open();
		idh->Read(buf.GetArrayPtr(),size);
		idh->Close();

		dh.OpenWrite("wb");
		dh.Write(buf.GetArrayPtr(),size);
		dh.Close();
	}
}



#define CheckTags() \
{\
	int t;\
	TAGSTACK_DEF tt;\
	kGUIHTMLObj *tp;\
	tp=tagparent;\
	t=numtags;\
	while(t>0)\
	{\
		tt=tagstack.GetEntry(t-1);\
		if(tt.skip==false)\
		{\
			assert(tp->GetID()==tt.tagid,"Tag ID Error!");\
			tp=tt.tagparent;\
		}\
		--t;\
	}\
}\

/* given a byte offset, calc a line and column value into the file */
void kGUIHTMLPageObj::CalcPlace(const char *start,const char *place,int *pline,int *pcol)
{
	int line;
	int col;
	const char *cur;

	line=1;
	col=1;
	cur=start;
	do
	{
		if(cur==place)
		{
			*(pline)=line;
			*(pcol)=col;
			return;
		}
		if(*(cur)==0x0a)
		{
			++line;
			col=1;
		}
		else
			++col;
		++cur;
	}while(1);
}

/* change \r\n to \n, also change \r to \n, then remove any leading or trailing returns */
bool kGUIHTMLPageObj::TrimCR(kGUIString *s)
{
	int l;

	s->Replace("\r\n","\n");
	s->Replace("\r","\n");
	if(!s->GetLen())
		return(false);

	/* remove leading return */
	if(s->GetChar(0)=='\n')
		s->Delete(0,1);
	l=s->GetLen();
	if(!l)
		return(false);

	/* remove trailing return */
	if(s->GetChar(l-1)=='\n')
	{
		s->Clip(l-1);
		--l;
	}
	return(l>0);
}

/* new and improved parse */
bool kGUIHTMLPageObj::Parse(kGUIHTMLObj *parent,const char *htmlstart,int htmllen,kGUIString *header)
{
	kGUIHTMLObj *renderparent;
	kGUIHTMLObj *tagparent;
	kGUIHTMLObj *attparent;
	kGUIString name;
	kGUIString attname;
	kGUIString attvalue;
	kGUIString popcheck;
	kGUIStringSplit ss;
	int nl;
	const char *sfp;
	char q;
	bool incomment;
	TAGLIST_DEF **tagptr;
	int numtags;
	Array<TAGSTACK_DEF>tagstack;
	TAGSTACK_DEF curtag;
	TAGSTACK_DEF closetag;
	TAGLIST_DEF *tag=0;
	kGUIHTMLObj *body=0;
	const char *fp=htmlstart;
	const char *htmlend=htmlstart+htmllen;
	unsigned int encoding=ENCODING_8BIT;
	unsigned int attid;
	bool popped=false;

	numtags=0;
	tagstack.Init(32,32);
	name.Alloc(8192);
	attname.Alloc(8192);
	attvalue.Alloc(8192);
	popcheck.Alloc(32);

	renderparent=parent;
	tagparent=parent;
	attparent=parent;

	/* ok, first scan the HTTP header */

	{
		unsigned int offset;
		kGUIString line;

		/* is the page encoding (charset) defined in the header? */
		if(ExtractFromHeader(header,"Content-Type:",&line))
		{
			const char *cp;

			/* check for encoding */
			cp=strstr(line.GetString(),"charset=");
			if(cp)
			{
				cp+=8;	/* skip charset= */
				encoding=kGUIString::GetEncoding(cp);
			}
		}

		/* handle more than 1 link */
		offset=0;
		do
		{
			if(ExtractFromHeader(header,"Link:",&line,&offset))
				Link(&line);
			else
				break;
		}while(1);
	}

	/* is there a BOM at the beginning? */

	if( (unsigned char)fp[0]==0xef &&
		(unsigned char)fp[1]==0xbb &&
		(unsigned char)fp[2]==0xbf )
	{
		encoding=ENCODING_UTF8;
		fp+=3;
	}

	attvalue.SetEncoding(encoding);

	if(fp[0]!='<')
	{
		if(!strnicmp(fp,"Content-Type:",13))
		{
			/* just skip it for now */
			while(fp[0]!='<' && fp<htmlend)
				++fp;
		}
	}

	/* check to see if this is indeed html, if the first none space character */
	/* is not a '<' then it is not valid html! */
	while(fp[0]!='<')
	{
		switch(fp[0])
		{
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			++fp;
		break;
		default:
			return(false);
		break;
		}
	};

	do
	{
		CheckTags();

		if(!strnicmp(fp,"<![CDATA[",9))
		{
			sfp=fp+9;
			/* skip ahead till ']]>' */
			fp=strstri(fp,"]]>");
			if(fp)
			{
				attvalue.SetString(sfp,(int)(fp-sfp));
				fp+=3;	/* skip ']]>' */
				goto gotblock2;
			}
			else
				fp=sfp;
		}
		else if(tagparent->m_id==HTMLTAG_TEXTAREA)
		{
			/* special case for textarea, ignore valid html between open and close tag */
			sfp=fp;
			fp=strstri(fp,"</textarea>");
			if(fp)
				goto gotblock;
			else
				fp=sfp;
		}
		else if(tagparent->m_id==HTMLTAG_SCRIPT)
		{
			/* special case for script, ignore valid html between open and close tag */
			sfp=fp;
			fp=strstri(fp,"</script>");
			if(fp)
				goto gotblock;
			else
				fp=sfp;
		}

		/* any inline stuff? */
		sfp=fp;
		q=0;
		incomment=false;
		while((fp[0]!='<') || (q!=0) || (incomment==true))
		{
			if(tagparent->m_id==HTMLTAG_STYLE || tagparent->m_id==HTMLTAG_SCRIPT)
			{
				if(!q)
				{
					if(fp[0]=='\"' || fp[0]=='\'')
						q=fp[0];
					else if(incomment==false && fp[0]=='/' && fp[1]=='*')
						incomment=true;
					else if(incomment==true && fp[0]=='*' && fp[1]=='/')
						incomment=false;
				}
				else if(fp[0]==q)
					q=0;
				else if(fp[0]==0x0a || fp[0]==0x0d)	/* if open quote is left across a c/r or l/f then close it */
					q=0;
			}
			if(fp[0]=='\\')
				++fp;

			if(++fp>=htmlend)
				goto done;
		}

		if(sfp!=fp)
		{
gotblock:;
			attvalue.SetString(sfp,(int)(fp-sfp));
gotblock2:;
			switch(tagparent->m_id)
			{
			case HTMLTAG_TITLE:
				kGUIXMLCODES::Shrink(&attvalue,&attname);
				TrimCR(&attname);
				attname.Trim();
				SetTitle(&attname);
				{
					kGUIHTMLObj *textobj;

					textobj=new kGUIHTMLObj(renderparent,this,&m_textgrouptag);
					textobj->SetString(&attname,false,false);
					tagparent->AddStyleChild(textobj);
				}
			break;
			case HTMLTAG_STYLE:
				RemoveComments(&attvalue);
				tagparent->AppendAttribute(HTMLATT_CONTENT,&attvalue);
			break;
			case HTMLTAG_SCRIPT:
				RemoveComments(&attvalue);
				tagparent->AppendAttribute(HTMLATT_CONTENT,&attvalue);
				m_scripts.Append(attvalue.GetString());
			break;
			case HTMLTAG_TEXTAREA:
			case HTMLTAG_BUTTON:
			case HTMLTAG_OPTION:
				RemoveComments(&attvalue);
				kGUIXMLCODES::Shrink(&attvalue,&attname);
				TrimCR(&attname);
				attname.Trim();
				tagparent->AppendAttribute(HTMLATT_CONTENT,&attname);
			break;
			default:
			{
				kGUIHTMLObj *textobj;

				kGUIXMLCODES::Shrink(&attvalue,&attname);

				if(TrimCR(&attname))
				{
					switch(renderparent->m_id)
					{
					case HTMLTAG_TABLE:
						if(AllWhite(&attname))
							goto ignoretext;

						InsertTag("tr");
						CheckTags();
					break;
					case HTMLTAG_TR:
						if(AllWhite(&attname))
							goto ignoretext;
						InsertTag("td");
						CheckTags();
					break;
					case HTMLTAG_SELECT:
						if(AllWhite(&attname))
							goto ignoretext;
						/* should not be text within select! */
						m_errors.ASprintf("Should not be text inside of a <SELECT> '/%s'\n",attname.GetString());
					break;
					}

					textobj=new kGUIHTMLObj(renderparent,this,&m_textgrouptag);
					textobj->SetString(&attname,false,false);
					tagparent->AddStyleChild(textobj);
				}
ignoretext:;
			}
			break;
			}
		}

		while( (fp[0]==10) || (fp[0]==13) || (fp[0]==' ') || (fp[0]==9) )
		{
			if(++fp>=htmlend)
				continue;
		}

		assert(fp[0]=='<',"Open bracket not found!");
		++fp;

		/* found a close tag */
		if(fp[0]=='/')
		{
			++fp;
			sfp=fp;
			while(fp[0]!='>' && fp<htmlend)
				++fp;
			name.SetString(sfp,(int)(fp-sfp));
			name.Trim();	/* remove tabs c/r spaces etc. */
			++fp;

			CheckTags();

			if(!numtags)
				m_errors.ASprintf("unneeded close tag='/%s'\n",name.GetString());
			else
			{
				curtag=tagstack.GetEntry(numtags-1);
				if(!strcmpin(curtag.tagname,name.GetString(),curtag.tagnamelen) && name.GetLen()==curtag.tagnamelen)
				{
					/* the top tag is a MATCH, pop it and continue */
					PopTag();
					CheckTags();
				}
				else
				{
					int n=numtags;
					int maxskip=0;
					int okskip=0;
					int line,col;

					/* look down stack to see if missing close tag */
					/* look for close tag  */
					while(n>1)
					{
						closetag=tagstack.GetEntry(n-1);
						if(!strcmpin(closetag.tagname,name.GetString(),closetag.tagnamelen) && name.GetLen()==closetag.tagnamelen)
						{
							/* found it, print out skipped unclosed tags */
							while(numtags>(n-1))
							{
								kGUIString u;
								bool printclose=false;

								closetag=tagstack.GetEntry(numtags-1);
								PopTag();
								CheckTags();
								if(numtags!=(n-1))
								{
									if(closetag.optend==false && closetag.inserted==false && closetag.skip==false)
									{
										if(printclose==false)
										{
											printclose=true;
											CalcPlace(htmlstart,sfp,&line,&col);
											m_errors.ASprintf("Trying to close</%s>! (line=%d,col=%d)\n",name.GetString(),line,col);
										}	
										u.SetString(closetag.tagname,closetag.tagnamelen);
										CalcPlace(htmlstart,closetag.tagname,&line,&col);
										m_errors.ASprintf("\tskipping unclosed tag <%s>! (opened at line=%d,col=%d)\n",u.GetString(),line,col);
									}
								}
							}
							break;
						}
						else
						{
							/* if this tag is "end optional" then don't count as skipped */
							if(closetag.optend==true)
								++okskip;

							/* if close tag is td/th then don't skip tr or table */
							if(!stricmp(name.GetString(),"td") || !stricmp(name.GetString(),"th"))
							{
								if(closetag.tagid==HTMLTAG_TR || closetag.tagid==HTMLTAG_TABLE)
									goto abortclose;
							}
							else if(!stricmp(name.GetString(),"tr"))
							{
								if(closetag.tagid==HTMLTAG_TABLE)
									goto abortclose;
							}
							if((++maxskip-okskip)>=3)
							{
								int line,col;
abortclose:;
								CalcPlace(htmlstart,sfp,&line,&col);
								m_errors.ASprintf("Could not find matching open tag for </%s> (line=%d,col=%d), aborting search!\n",name.GetString(),line,col);
								break;
							}
							--n;
						}
					}
				}
			}
			CheckTags();
			continue;
		}

		/* html comment? */
		if(fp[0]=='!' && fp[1]=='-' && fp[2]=='-')
		{
			bool isend=false;
			const char *efp=0;

			fp+=3;
			sfp=fp;

			do
			{
				if(fp[0]=='-' && fp[1]=='-')
				{
					isend=!isend;
					efp=fp;
					fp+=2;
				}
				else
				{
					if(++fp>=htmlend)
						break;
				}
				if(isend==true &&  fp[0]=='>')
				{
					attvalue.SetString(sfp,(int)(efp-sfp));
					++fp;
					RemoveComments(&attvalue);
					if(tagparent->m_id==HTMLTAG_STYLE)
					{
						kGUIString media;
						kGUIHTMLAttrib *att;

						att=tagparent->FindAttrib(HTMLATT_MEDIA);
						if(att)
						{
							media.SetString(att->GetValue());
							media.Replace("\t"," ");
						}
						if(ValidMedia(&media))
							AddClassStyles(OWNER_AUTHOR,GetStylePriority(),&m_url,&attvalue);
					}
					else
						tagparent->AppendAttribute(HTMLATT_CONTENT,&attvalue);
					break;
				}
			}while(1);
			continue;
		}

		sfp=fp;
		while(fp[0]!=0x0a && fp[0]!=0x0d && fp[0]!=' ' && fp[0]!=9 && fp[0]!='/' && fp[0]!='>')
			++fp;
		name.SetString(sfp,(int)(fp-sfp));
		nl=(int)(fp-sfp);
		curtag.tagname=sfp;
		curtag.tagnamelen=nl;
		curtag.renderparent=renderparent;
		curtag.tagparent=tagparent;
		curtag.skip=false;
		curtag.inserted=false;
		curtag.optend=false;

		SetNoCloseNeeded(false);
		tagptr=(TAGLIST_DEF **)m_taghash.Find(name.GetString());
		if(tagptr)
		{
			tag=*(tagptr);
			curtag.optend=tag->endoptional;
			m_debug.ASprintf("ok tag, name = <%s>\n",name.GetString());

			if(tag->noclose==true)
				SetNoCloseNeeded(true);

#if 1
			/* duplicate bodies? */
			if(tag->tokenid==HTMLTAG_BODY && body)
			{
				attparent=body;
				CheckTags();
				SkipTag("body");
				CheckTags();
				goto ignoretag;
			}
#endif

			/* check to make sure child tag is valid inside parent */

			CheckTags();

			/* loop until we don't do a pop */
			do
			{
				popped=false;

				if(!tagparent->m_tag)
					break;

				/* look in the pop hashtable */
				popcheck.Sprintf("%s:%s",tagparent->m_tag->name,tag->name);
				if(m_pophash.Find(popcheck.GetString()))
				{
					if(tagparent->m_tag->endoptional==false)
						m_errors.ASprintf("tag '%s' was missing it's end block </%s>\n",tagparent->m_tag->name,tagparent->m_tag->name);

					PopTag();
					CheckTags();
				}
				else
				{
					switch(tagparent->GetID())
					{
					case HTMLTAG_ROOT:	/* internal base object, next NEEDS to be "html" (!doctype is ok, ?xml is ok ?xml-stylesheet is ok) */
						if(tag->tokenid!=HTMLTAG_HTML && tag->tokenid!=HTMLTAG_DOCTYPE && tag->tokenid!=HTMLTAG_XML && tag->tokenid!=HTMLTAG_XMLSTYLESHEET)
						{
							InsertTag("html");
							CheckTags();
							m_errors.ASprintf("tag '%s' found as root tag, inserting <html>\n",name.GetString());
						}
					break;
					case HTMLTAG_HTML:	/* HEAD and BODY are the only valid tags here */
						if(tag->tokenid!=HTMLTAG_HEAD && tag->tokenid!=HTMLTAG_BODY)
						{
							m_errors.ASprintf("tag '%s' not allowed inside <html>\n",name.GetString());
						}
					break;
					case HTMLTAG_TABLE:
						if(tag->tokenid!=HTMLTAG_TR)
						{
							if(tag->tokenid==HTMLTAG_TBODY)
							{
								SkipTag("tbody");
								goto ignoretag;
							}
							else if(tag->tokenid==HTMLTAG_THEAD)
							{
								SkipTag("thead");
								goto ignoretag;
							}
							if(tag->tokenid==HTMLTAG_TD || tag->tokenid==HTMLTAG_TH)
							{
								InsertTag("tr");
								CheckTags();
							}
							else if(tag->tokenid!=HTMLTAG_CAPTION && tag->tokenid!=HTMLTAG_COLGROUP)
							{
								m_errors.ASprintf("tag '%s' not allowed inside <table>\n",name.GetString());
							}
						}
					break;
					case HTMLTAG_TR:
						if(tag->tokenid!=HTMLTAG_TD && tag->tokenid!=HTMLTAG_TH)
						{
							m_errors.ASprintf("tag '%s' not allowed inside <tr>\n",name.GetString());
						}
					break;
					}
				}
				/* if we did a pop then check again */
			}while(popped==true);

			CheckTags();

			if(tag)
			{
				kGUIHTMLObj *newobj;

				curtag.renderparent=renderparent;
				curtag.tagparent=tagparent;

				newobj=new kGUIHTMLObj(renderparent,this,tag);
				newobj->SetOwnerURL(StringToIDcase(&m_url));
				renderparent->AddObject(newobj);
				tagparent->AddStyleChild(newobj);
				renderparent=newobj;			/* new render parent */
				tagparent=newobj;				/* new style parent */
				attparent=newobj;

				switch(tag->tokenid)
				{
				case HTMLTAG_BODY:
					body=newobj;
				break;
				case HTMLTAG_SELECT:
				case HTMLTAG_INPUT:
				case HTMLTAG_TEXTAREA:
				{
					kGUIHTMLObj *p=tagparent;

					do
					{
						if(p->m_id==HTMLTAG_FORM)
							break;
						p=p->m_styleparent;
					}while(p);

					/* is there a form back in the tree? */
					if(p)
						p->m_obj.m_formobj->AddObject(newobj);
#if 0
					//since javascript can use these as well we should not report this as an error
					else
					{
						int line,col;

						CalcPlace(htmlstart,sfp,&line,&col);
						m_errors.ASprintf("object '%s' only allowed inside a <form> tag (line=%d,col=%d)\n",name.GetString(),line,col);
					}
#endif
				}
				break;
				}
				curtag.tagid=tag->tokenid;
				tagstack.SetEntry(numtags++,curtag);
				CheckTags();
			}
		}
		else
		{
			kGUIHTMLObj *newobj;
			int line,col;

			CalcPlace(htmlstart,sfp,&line,&col);
			m_errors.ASprintf("unknown tag, name = <%s> (line=%d,col=%d)\n",name.GetString(),line,col);

			newobj=new kGUIHTMLObj(renderparent,this,&m_unknowntag);
			newobj->SetOwnerURL(StringToIDcase(&m_url));
			tagparent->AddStyleChild(newobj);
			tagparent=newobj;				/* new style parent */
			curtag.tagid=HTMLTAG_UNKNOWN;
			attparent=tagparent;
			tagstack.SetEntry(numtags++,curtag);
			CheckTags();
		}
ignoretag:;
		CheckTags();

		while(fp[0]==0x0a || fp[0]==0x0d)
			++fp;
		if(fp[0]!='>')
		{
			if(fp[0]==' ')
				++fp;
			if(fp[0]=='/' && fp[1]=='>')
			{
				++fp;
				if(tag->endearlyok==false)
					m_errors.ASprintf("This tag ('%s') should not be closed right away, ignoring!\n",name.GetString());
				else
					SetNoCloseNeeded(true);
			}
			else
			{
				/* this command has values in the header */
				while(fp[0]!='>')
				{
					bool skipeq=false;

					if(fp[0]=='<')
					{
						/* tag not properly closed */
						m_errors.ASprintf("tag not properly closed, name = <%s>\n",name.GetString());
						goto endtag;
					}
					while(fp[0]=='\n' || fp[0]=='\r')
						++fp;
					sfp=fp;
					while(fp[0]!='=' && fp[0]!=' ' &&  fp[0]!='>')
						++fp;

					attname.SetString(sfp,(int)(fp-sfp));
					attname.Trim();

					while(fp[0]==' ' || fp[0]=='\n' || fp[0]=='\r')
						++fp;

					if(attname.GetLen())
					{
						if(fp[0]=='=')
						{
							++fp;

							/* skip white space */
							while(fp[0]==' ' || fp[0]=='\n' || fp[0]=='\r')
								++fp;

							q=fp[0];
							if(q=='\"' || q=='\'')
							{
								++fp;
								sfp=fp;
								while(fp[0]!=q)
								{
									/* allow \quote to exist inside */
									if(fp[0]=='\\')
										++fp;
									++fp;
								}
								skipeq=true;
							}
							else	/* no quotes, so grab until space/tab or '>' */
							{
								sfp=fp;
								while(fp[0]!=' ' && fp[0]!=10 && fp[0]!=13 && fp[0]!=9 && fp[0]!='>')
									++fp;
							}
							attvalue.SetString(sfp,(int)(fp-sfp));
							attvalue.RemoveQuotes();
						}
						else
							attvalue.Clear();	/* parm without '=' for example <input type="radio" checked> */

						if(attparent->m_id==HTMLTAG_DOCTYPE)
						{
							/* check for strict */
							if(strstri(attname.GetString(),"strict.dtd"))
								m_strict=true;
						}
						else if(name.GetChar(0)!='!')
						{
							attid=StringToID(&attname);
							if(attid>HTMLATT_NUM)
							{
								/* not a known attribute,so print a warning */
								m_errors.ASprintf("unknown html tag parm <%s> '%s'='%s'\n",name.GetString(),attname.GetString(),attvalue.GetString());
							}

							/* todo, handle, <?xml encoding="utf-8"> */

							if(attparent->m_id==HTMLTAG_META)
							{
								/* what is the encoding of this page? */
								if(attid==HTMLATT_CONTENT)
								{
									const char *cp;

									/* check for encoding */
									cp=strstr(attvalue.GetString(),"charset=");
									if(cp)
									{
										cp+=8;	/* skip charset= */
										encoding=kGUIString::GetEncoding(cp);
										attvalue.SetEncoding(encoding);
									}
								}
							}

							switch(attid)
							{
							case HTMLATT_CLASS:
								kGUIXMLCODES::Shrink(&attvalue,&attname);
								if(attname.GetLen())
									attparent->AddAndSplitAttribute(this,HTMLATT_CONTENT,OWNER_AUTHOR,0,attid,&attname,true);
							break;
							case HTMLATT_ID:
								/* no &encoding allowed for ID or Class values */
								if(attvalue.GetLen())
									attparent->AddAndSplitAttribute(this,HTMLATT_CONTENT,OWNER_AUTHOR,0,attid,&attvalue,true);
							break;
							case HTMLATT_STYLE:
								if(GetSettings()->GetUseCSS())
								{
									kGUIXMLCODES::Shrink(&attvalue,&attname);
									attparent->AddAttributes(this,HTMLATT_CONTENT,OWNER_AUTHOR,0,&attname);
								}
							break;
							default:
								kGUIXMLCODES::Shrink(&attvalue,&attname);
								attparent->AddAndSplitAttribute(this,HTMLATT_CONTENT,OWNER_AUTHOR,0,attid,&attname,true);
							break;
							}
						}
					}
					if(skipeq)
						++fp;

					while(fp[0]==0x0a || fp[0]==0x0d || fp[0]==' ' || fp[0]==9)
						++fp;
					if((fp[0]=='?' || fp[0]=='/') && fp[1]=='>')
					{
						++fp;
						if(tag)
						{
							if(tag->endearlyok==false)
								m_errors.ASprintf("This tag ('%s') should not be closed right away, ignoring!\n",name.GetString());
							else
                                SetNoCloseNeeded(true);
						}
						else
                                SetNoCloseNeeded(true);
						break;
					}
				}
			}
			++fp;	/* skip '>' */
			while( (fp[0]==10) || (fp[0]==13) || (fp[0]==9) )
				++fp;
		}
		else
			++fp;

endtag:	/* no end tag needed */
		if((GetNoCloseNeeded()==true) || (name.GetChar(0)=='!'))
		{
			PopTag();
		}

		CheckTags();
	}while(fp<=htmlend);
done:;
	return(true);
}

bool kGUIHTMLPageObj::AllWhite(kGUIString *s)
{
	char c;
	const char *cp;

	cp=s->GetString();
	while((c=*(cp++)))
	{
		switch(c)
		{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		break;
		default:
			return(false);
		break;
		}
	}
	return(true);
}

/* regenerate page source from traversing the HTMLObj tree */

void kGUIHTMLPageObj::GetCorrectedSource(kGUIString *cs)
{
	cs->Clear();
	m_rootobject->GenerateSource(cs,0);
}

void kGUIHTMLObj::GenerateSource(kGUIString *cs,unsigned int depth)
{
	unsigned int i;
	unsigned int j;
	kGUIHTMLAttrib *att;
	TAGLIST_DEF *t;

	/* calc sizes of all children by traversing the style tree*/

	switch(m_id)
	{
	case HTMLTAG_UNKNOWN:
		t=&kGUIHTMLPageObj::m_unknowntag;
	break;
	case HTMLTAG_IMBEDTEXTGROUP:
		t=0;
		cs->Append(m_obj.m_textgroup->GetString());
	break;
	case HTMLTAG_IMBEDTEXT:
		t=0;
	break;
	default:
		t=kGUIHTMLPageObj::m_taglist;
		for(i=0;i<(sizeof(kGUIHTMLPageObj::m_taglist)/sizeof(TAGLIST_DEF));++i)
		{
			if(m_id==t->tokenid)
				break;
			++t;
		}
		if(m_id!=t->tokenid)
			t=0;
	break;
	}
	if(t)
	{
		if(t->defdisp!=DISPLAY_INLINE)
		{
			cs->Append("\n");
			for(i=0;i<depth;++i)
				cs->Append("\t");
		}
		cs->ASprintf("<%s",t->name);

		for(i=0;i<GetNumAttributes();++i)
		{
			ATTLIST_DEF *al;

			att=GetAttrib(i);
			al=kGUIHTMLPageObj::m_attlist;
			for(j=0;j<(sizeof(kGUIHTMLPageObj::m_attlist)/sizeof(ATTLIST_DEF))-1;++j)
			{
				if(att->GetID()==al->attid)
					break;
				++al;
			}
			cs->ASprintf(" %s=\"%s\"",al->name,att->GetValue()->GetString());
		}
		if(t->noclose==true)
			cs->Append("/");
		cs->Append(">");
	}

	for(i=0;i<m_numstylechildren;++i)
		m_stylechildren.GetEntry(i)->GenerateSource(cs,depth+1);

	if(t)
	{
		if(t->noclose==false)
		{
			if(t->defdisp!=DISPLAY_INLINE)
			{
				cs->Append("\n");
				for(i=0;i<depth;++i)
					cs->Append("\t");
			}
			cs->ASprintf("</%s>",t->name);
		}
	}
}

/***************************************************************************/

kGUIHTMLObj::kGUIHTMLObj()
{
	Init();
}

kGUIHTMLObj::kGUIHTMLObj(kGUIHTMLObj *renderparent,kGUIHTMLPageObj *page,TAGLIST_DEF *tag)
{
	Init();
	m_renderparent=renderparent;
	m_page=page;
	m_tag=tag;
	SetID(tag->tokenid);
	if(m_id==HTMLTAG_TR && renderparent->GetID()!=HTMLTAG_TABLE)
		m_rowspan=1;
}


void kGUIHTMLObj::Init(void)
{
	SetNumGroups(1);
	m_styleparent=0;
	m_renderparent=0;
	m_page=0;
	m_tag=0;
	m_id=0;
	m_subid=0;
	m_box=0;
	m_scroll=0;
	m_beforeobj=0;
	m_afterobj=0;
	m_ti=0;	/* pointer to table info, only allocated for table objects */
	m_bordercolor1=DrawColor(0xaf,0xaf,0xaf);
	m_bordercolor2=DrawColor(0x40,0x40,0x40);
	m_colspan=1;
	m_rowspan=1;
	m_display=DISPLAY_BLOCK;
	m_overflowx=OVERFLOW_VISIBLE;
	m_overflowy=OVERFLOW_VISIBLE;
	m_textoverflow=TEXTOVERFLOW_CLIP;
	m_opacity=1.0f;

	m_insert=false;
	m_rulecache=0;
	m_error=false;
	m_hover=false;
	m_active=false;
	m_washover=false;
	m_useshover=false;
	m_visible=VISIBLE_VISIBLE;
	m_page=0;
	m_tag=0;
	m_id=0;
	m_numcids=0;
	SetAlign(ALIGN_UNDEFINED);
	SetVAlign(VALIGN_BASELINE);
	m_string=0;
	m_renderparent=0;
	m_objinit=false;
	m_obj.m_textobj=0;
	m_numstylechildren=0;
	m_stylechildren.Init(16,16);
	m_bgrepeatx=true;
	m_bgrepeaty=true;
	m_bgfixed=false;

	m_width.Reset();
	m_height.Reset();
	m_minwidth.Reset();
	m_minheight.Reset();
	m_maxwidth.Reset();
	m_maxheight.Reset();
	m_hashover=false;
	m_position=POSITION_STATIC;
	m_float=FLOAT_NONE;
	m_clear=CLEAR_NONE;

	m_fixedw=false;
	m_fixedh=false;
	m_relw=false;
	m_relh=false;
	m_fixedpos=false;
	m_abspos=false;
	m_relpos=false;
	m_skipbr=false;
	m_maxchildy=0;
	m_potextalign=ALIGN_UNDEFINED;
}

void kGUIHTMLObj::SetID(int id)
{
	m_id=id;

	switch(id)
	{
	case HTMLTAG_ROOT:	/* internal base object */
		m_insert=true;
	break;
	case HTMLTAG_IMG:
	case HTMLTAG_LIIMG:
		m_obj.m_imageobj=new kGUIOnlineImageObj();
		m_obj.m_imageobj->SetPage(m_page);
		m_obj.m_imageobj->MovePos(0,0);
		AddObject(m_obj.m_imageobj);
	break;
	case HTMLTAG_BUTTON:
		m_obj.m_buttontextobj=new kGUIHTMLButtonTextObj(this,HTMLCONST_BUTTON);
		AddRenderObject(m_obj.m_buttontextobj);
	break;
	case HTMLTAG_LISHAPE:
		m_obj.m_shapeobj=new kGUIHTMLShapeObj();
		m_obj.m_shapeobj->MovePos(0,0);
		AddObject(m_obj.m_shapeobj);
	break;
	case HTMLTAG_FORM:
		m_obj.m_formobj=new kGUIHTMLFormObj();
		m_obj.m_formobj->SetPage(m_page);
	break;
	case HTMLTAG_IMBEDTEXTGROUP:
		m_insert=true;
		m_obj.m_textgroup=new kGUIHTMLTextGroup(m_page);
	break;
	case HTMLTAG_IMBEDTEXT:
		m_insert=true;
		m_obj.m_textobj=new kGUIHTMLTextObj();
		AddRenderObject(m_obj.m_textobj);
	break;
	case HTMLTAG_CONTENTGROUP:
		m_insert=true;
		m_obj.m_contentgroup=new kGUIHTMLContentGroup(m_page);
	break;
	case HTMLTAG_TEXTAREA:
		m_obj.m_inputobj=new kGUIHTMLInputBoxObj();
		AddRenderObject(m_obj.m_inputobj);
	break;
	case HTMLTAG_FRAME:
	case HTMLTAG_IFRAME:
		m_obj.m_linked=0;	/* null until object is linked in */
	break;
	}
}

kGUIString *kGUIHTMLObj::GetURL(void)
{
	switch(m_id)
	{
	case HTMLTAG_A:
		if(m_obj.m_linkobj)
			return(m_obj.m_linkobj->GetURL());
	break;
	}
	return(0);
}

kGUIString *kGUIHTMLObj::GetReferrer(void)
{
	switch(m_id)
	{
	case HTMLTAG_A:
		if(m_obj.m_linkobj)
			return(m_obj.m_linkobj->GetReferrer());
	break;
	}
	return(0);
}

/* called on the min-max pass before applying css style to the object */
void kGUIHTMLObj::PreStyle(kGUIStyleInfo *si)
{
	si->m_cc=0;
	si->m_oldposition=m_position;

	MoveZoneW(0);
	MoveZoneH(0);

	if(m_box)
		m_box->Reset();
	m_fixedw=false;
	m_relw=false;
	m_fixedh=false;
	m_relh=false;
	m_minw=0;
	m_maxw=0;
	m_maxy=0;
	SetOutsideW(0);
	SetOutsideH(0);

	/* reset these styles */
	m_em=m_page->GetEM();
	m_bgcolor.SetTransparent(true);
	m_bgimage.SetIsValid(false);
	m_bgrepeatx=true;
	m_bgrepeaty=true;
	m_bgfixed=false;
	m_outlinewidth=0;
	m_outlinecolor.Reset();
	m_outlinecolor.SetInverse(true);
	m_outlinestyle=BORDERSTYLE_NONE;
	m_opacity=1.0f;

	m_valign=VALIGN_BASELINE;

	m_width.Reset();
	m_height.Reset();
	m_left.Reset();
	m_right.Reset();
	m_top.Reset();
	m_bottom.Reset();

	m_textalign=ALIGN_UNDEFINED;
	m_patttextindent=0;

	m_pattmarginleft=0;
	m_pattmarginright=0;
	m_pattmargintop=0;
	m_pattmarginbottom=0;

	m_pattborderleft=0;
	m_pattborderright=0;
	m_pattbordertop=0;
	m_pattborderbottom=0;

	m_pattpaddingleft=0;
	m_pattpaddingright=0;
	m_pattpaddingtop=0;
	m_pattpaddingbottom=0;
}

/* called on the min-max pass after applying css style to the object */
void kGUIHTMLObj::PostStyle(kGUIStyleInfo *si)
{
	/* check for styles to inherit */
	if(m_styleparent)
	{
		if(!m_patttextindent)
			m_patttextindent=m_styleparent->m_patttextindent;

		/* inherit from parent if undefined or inline or anonymous */
		if(m_textalign==ALIGN_UNDEFINED)
			m_textalign=m_styleparent->m_textalign;
	}
	else
	{
		m_textalign=ALIGN_LEFT;
	}

	if(m_display==DISPLAY_TABLE || m_display==DISPLAY_INLINE_TABLE)
	{
		if(!m_ti)
			m_ti=new kGUIHTMLTableInfo();
	}
	else
	{
		//if(m_ti)
		//{
		//	delete m_ti;
		//	m_ti=0;
		//}
	}

	/* if we are floating an inline object then switch to an inline block */
	if(m_display==DISPLAY_INLINE && m_float!=FLOAT_NONE)
		m_display=DISPLAY_INLINE_BLOCK;

	if(m_display==DISPLAY_ANONYMOUS)
	{
		SetVAlign(m_renderparent->GetVAlign());
	}

	if(si->m_oldposition!=m_position)
	{
		if(si->m_oldposition==POSITION_ABSOLUTE || si->m_oldposition==POSITION_FIXED)
		{
			/* put back into the render tree in it's previous position */
			m_renderparent->DelObject(this);
			m_oldrenderparent->AddObject(this);
			m_renderparent=m_oldrenderparent;
		}
	}

	if(m_position!=POSITION_STATIC)
	{
		if(m_position==POSITION_ABSOLUTE)
		{
			/* remove me from my regular render parent and attach me to my new render parent */
			/* leave me in the style tree at the same position though so any css rules will still match! */
			if(m_renderparent!=m_page->m_absobject)
			{
				m_oldrenderparent=m_renderparent;
				m_renderparent->DelObject(this);
				m_page->m_absobject->AddObject(this);
				m_page->m_absobject->SetAllowOverlappingChildren(true);
				m_renderparent=m_page->m_absobject;
			}
		}
		else if(m_position==POSITION_FIXED)
		{
			if(m_renderparent!=m_page->m_fixedfgobject)
			{
				m_oldrenderparent=m_renderparent;
				m_renderparent->DelObject(this);
				m_page->m_fixedfgobject->AddObject(this);
				m_renderparent=m_page->m_fixedfgobject;
			}
		}
	}

	//8.5.2
	//If an element's border color is not specified with a border property,
	//user agents must use the value of the element's 'color' property as the
	//computed value for the border color. 

	if(m_box)
		m_box->SetUndefinedBorderColors(m_page->m_fontcolor.GetColor(),false);

//	if(m_overflowx==OVERFLOW_VISIBLE || m_overflowy==OVERFLOW_VISIBLE)
		SetAllowOverlappingChildren(true);

	/* do we need to allocate a scroll controller for this container? */
	if(m_overflowx==OVERFLOW_SCROLL || m_overflowy==OVERFLOW_SCROLL)
	{
		if(!m_scroll)
			m_scroll=new kGUIScrollControl(this,10);
	}
	else
	{
		if(m_scroll)
		{
			delete m_scroll;
			m_scroll=0;
		}
	}

	/* set current color for anything?? */
	if(si->m_cc&CURRENTCOLOR_BACKGROUND)
		m_bgcolor.CopyFrom(&m_page->m_fontcolor);

	if(si->m_cc&CURRENTCOLOR_BORDER_TOP)
		GetBox()->SetBorderColor(BORDER_TOP,&m_page->m_fontcolor);

	if(si->m_cc&CURRENTCOLOR_BORDER_BOTTOM)
		GetBox()->SetBorderColor(BORDER_BOTTOM,&m_page->m_fontcolor);

	if(si->m_cc&CURRENTCOLOR_BORDER_LEFT)
		GetBox()->SetBorderColor(BORDER_LEFT,&m_page->m_fontcolor);

	if(si->m_cc&CURRENTCOLOR_BORDER_RIGHT)
		GetBox()->SetBorderColor(BORDER_RIGHT,&m_page->m_fontcolor);

	if(m_id==HTMLTAG_HTML)
	{
		/* save color from HTML to clear whole page to */
		if(m_bgcolor.GetTransparent()==false && m_bgcolor.GetUndefined()==false)
			m_page->m_pagebgcolor=m_bgcolor.GetColor();
	}


	m_dir=m_page->m_dir;
	//m_textindent.CopyFrom(&m_page->m_textindent);
	m_lh=m_page->GetLH();

	ApplyRelative();
}

void kGUIHTMLObj::ApplyRelative(void)
{
	if(m_pattmarginleft)
		GetBox()->SetMarginWidth(MARGIN_LEFT,m_pattmarginleft->GetValue(),m_renderparent);
	if(m_pattmarginright)
		GetBox()->SetMarginWidth(MARGIN_RIGHT,m_pattmarginright->GetValue(),m_renderparent);
	if(m_pattmargintop)
		GetBox()->SetMarginWidth(MARGIN_TOP,m_pattmargintop->GetValue(),m_renderparent);
	if(m_pattmarginbottom)
		GetBox()->SetMarginWidth(MARGIN_BOTTOM,m_pattmarginbottom->GetValue(),m_renderparent);

	if(m_pattborderleft)
		GetBox()->SetBorderWidth(BORDER_LEFT,m_pattborderleft->GetValue(),m_renderparent);
	if(m_pattborderright)
		GetBox()->SetBorderWidth(BORDER_RIGHT,m_pattborderright->GetValue(),m_renderparent);
	if(m_pattbordertop)
		GetBox()->SetBorderWidth(BORDER_TOP,m_pattbordertop->GetValue(),m_renderparent);
	if(m_pattborderbottom)
		GetBox()->SetBorderWidth(BORDER_BOTTOM,m_pattborderbottom->GetValue(),m_renderparent);

	if(m_pattpaddingleft)
		GetBox()->SetPaddingWidth(PADDING_LEFT,m_pattpaddingleft->GetValue(),m_renderparent);
	if(m_pattpaddingright)
		GetBox()->SetPaddingWidth(PADDING_RIGHT,m_pattpaddingright->GetValue(),m_renderparent);
	if(m_pattpaddingtop)
		GetBox()->SetPaddingWidth(PADDING_TOP,m_pattpaddingtop->GetValue(),m_renderparent);
	if(m_pattpaddingbottom)
		GetBox()->SetPaddingWidth(PADDING_BOTTOM,m_pattpaddingbottom->GetValue(),m_renderparent);

}

/* called on the position pass before positioning the object */
void kGUIHTMLObj::PrePosition(void)
{
	/* re-apply incase they use parent size % */
	ApplyRelative();
}

/* allocate or deallocate and special objects */
void kGUIHTMLObj::ChangeDisplay(unsigned int olddisplay)
{
	/* free any items associated with old display type */
	switch(olddisplay)
	{
	case DISPLAY_LIST_ITEM:
		m_obj.m_liprefix->Purge();
		delete m_obj.m_liprefix;
		m_obj.m_liprefix=0;
	break;
	}

	/* init any items needed for new display type */
	switch(m_display)
	{
	case DISPLAY_LIST_ITEM:
	{
		unsigned int i;
		unsigned int j;
		unsigned int index;
		kGUIHTMLLIPrefix *prefix;
		kGUIHTMLObj *sibling;
		kGUIHTMLAttrib *att;

		/* calc list index */
		prefix=new kGUIHTMLLIPrefix(m_page);
		m_obj.m_liprefix=prefix;

		/* loop through my styleparent and calc my index */
		j=m_styleparent->GetNumStyleChildren();
		att=m_styleparent->FindAttrib(HTMLATT_START);
		if(att)
			index=att->GetValue()->GetInt();
		else
			index=1;
		for(i=0;i<j;++i)
		{
			sibling=m_styleparent->m_stylechildren.GetEntry(i);
			if(sibling->m_display==DISPLAY_LIST_ITEM)
			{
				att=sibling->FindAttrib(HTMLATT_START);
				if(att)
					index=att->GetValue()->GetInt();
				if(sibling==this)
					break;
				++index;
			}
		}
		prefix->SetIndex(index);
	}
	break;
	}
}

kGUIHTMLBox *kGUIHTMLObj::GetBox(void)
{
	if(!m_box)
		m_box=new kGUIHTMLBox(m_page);
	return(m_box);
}

void kGUIHTMLLinkObj::Click(void)
{
	m_page->Click(GetURL(),GetReferrer());

	/* flag re-draw since active flag has changed, we can add "usesactive" if we want */
	if(m_a)
		m_a->SetActive(true);
	m_page->RePosition(false);
}

bool kGUIHTMLLinkObj::GetVisited(void)
{
	if(m_page->GetVisitedCache())
		return(m_page->GetVisitedCache()->Find(&m_url));
	else
		return(false);
}

/* mouse is over me */
void kGUIHTMLLinkObj::SetOver(void)
{
	m_page->SetIsOver(true);
	m_page->SetLinkUnder(GetURL());
	m_page->SetLinkHover(this);
}

void kGUIHTMLObj::SetString(kGUIString *string,bool ps,bool pcr)
{
	switch(m_id)
	{
	case HTMLTAG_IMBEDTEXT:
		m_obj.m_textobj->SetString(string);
		m_obj.m_textobj->SetPlusSpace(ps);
		m_obj.m_textobj->SetPlusCR(pcr);
	break;
	case HTMLTAG_IMBEDTEXTGROUP:
		m_obj.m_textgroup->SetString(string);
	break;
	case HTMLTAG_CONTENTGROUP:
		m_obj.m_contentgroup->SetChanged(true);
		m_obj.m_contentgroup->SetString(string);
	break;
	default:
		assert(false,"Error!");
	break;
	}
}

void kGUIHTMLObj::Purge(void)
{
	unsigned int i;

	if(m_id==HTMLTAG_FORM && m_obj.m_formobj)
	{
		delete m_obj.m_formobj;
		m_obj.m_formobj=0;
	}
	else if(m_id==HTMLTAG_IMBEDTEXTGROUP && m_obj.m_textgroup)
	{
		delete m_obj.m_textgroup;
		m_obj.m_textgroup=0;
	}
	else if(m_id==HTMLTAG_CONTENTGROUP && m_obj.m_contentgroup)
	{
		delete m_obj.m_contentgroup;
		m_obj.m_contentgroup=0;
	}
	else if(m_id==HTMLTAG_A)
		delete m_obj.m_linkobj;
	else if(m_id==HTMLTAG_LI)
		delete m_obj.m_liprefix;

	for(i=0;i<m_numstylechildren;++i)
	{
		kGUIHTMLObj *child;

		child=m_stylechildren.GetEntry(i);
		child->m_styleparent=0;
		delete child;
	}
	PurgeStyles();

	m_numstylechildren=0;
}

kGUIHTMLObj::~kGUIHTMLObj()
{
	kGUIHTMLObj *sp=m_styleparent;

	if(m_ti && m_id==HTMLTAG_TABLE)
		delete m_ti;
	if(sp)
	{
		m_styleparent=0;
		sp->m_stylechildren.Delete(this);
		--sp->m_numstylechildren;
	}
	DeleteChildren(true);

	if(m_scroll)
		delete m_scroll;
	if(m_box)
		delete m_box;
	if(m_string)
		delete m_string;

	Purge();
}

void kGUIHTMLObj::AddRenderObject(kGUIObj *obj)
{
	m_renderparent->AddObject(obj);
}

/* set true applied flag to block subsequent rules that use previously set attributes */
void kGUIHTMLPageObj::SetTrueApplied(kGUIStyleObj *slist,unsigned int owner)
{
	unsigned int i;
	unsigned int id;
	kGUIHTMLAttrib *att;

	/* this is for container only attributes */
	for(i=0;i<slist->GetNumAttributes();++i)
	{
		att=slist->GetAttrib(i);

		if(att->GetOwner()!=owner)
			continue;
		id=att->GetID();
		if(id>=HTMLATT_UNKNOWN)
			continue;
		m_trueapplied[id]=m_appliedlevel;
	}
}

/* get true applied flag to check if all styles in this rule have already been applied */
bool kGUIHTMLPageObj::GetTrueBlocked(kGUIStyleObj *slist,unsigned int owner)
{
	unsigned int i;
	unsigned int id;
	kGUIHTMLAttrib *att;

	for(i=0;i<slist->GetNumAttributes();++i)
	{
		att=slist->GetAttrib(i);

		if(att->GetOwner()!=owner)
			continue;
		id=att->GetID();
		if(id>=HTMLATT_UNKNOWN)
			continue;
		if(m_trueapplied[id]!=m_appliedlevel)
			return(false);
	}
	/* all attributes are blocked! */
	return(true);
}


void kGUIHTMLObj::SetAttributes(kGUIStyleObj *slist,unsigned int owner,kGUIStyleInfo *si)
{
	unsigned int i;
	unsigned int id;
	unsigned int code;
	kGUIHTMLAttrib *att;
	kGUIStringSplit ss;
	unsigned int *applied=m_page->m_applied;
	unsigned int appliedlevel=m_page->m_appliedlevel;

	for(i=0;i<slist->GetNumAttributes();++i)
	{
		att=slist->GetAttrib(i);

		/* todo, use linked list */
		if(att->GetOwner()!=owner)
			continue;

		id=att->GetID();
		if(id>=HTMLATT_UNKNOWN)
			continue;

		if(applied[id]==appliedlevel && m_page->GetTrace())
		{
			if(m_page->m_traceatt[att->GetID()])
				kGUI::Trace("Attribute NOT! applied: %S %S\n",m_page->IDToString(att->GetID()),att->GetValue());
		}

		if(applied[id]!=appliedlevel && m_page->GetSettings()->GetCSSBlock(id)==false)
		{
			if(m_page->GetTrace())
			{
				if(m_page->m_traceatt[att->GetID()])
					kGUI::Trace("Attribute applied: %S %S\n",m_page->IDToString(att->GetID()),att->GetValue());
			}
			applied[id]=appliedlevel;
			switch(id)
			{
			case HTMLATT_LEFT:
				m_left.Set(m_page,att->GetValue());
			break;
			case HTMLATT_RIGHT:
				m_right.Set(m_page,att->GetValue());
			break;
			case HTMLATT_TOP:
				m_top.Set(m_page,att->GetValue());
			break;
			case HTMLATT_BOTTOM:
				m_bottom.Set(m_page,att->GetValue());
			break;
			case HTMLATT_DIRECTION:
				m_page->PushStyle(sizeof(m_page->m_dir),&m_page->m_dir);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_LTR:
					m_page->m_dir=TEXTDIR_LTR;
				break;
				case HTMLCONST_RTL:
					m_page->m_dir=TEXTDIR_RTL;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for DIR\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_VISIBILITY:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_HIDDEN:
					m_visible=VISIBLE_HIDDEN;
				break;
				case HTMLCONST_VISIBLE:
					m_visible=VISIBLE_VISIBLE;
				break;
				case HTMLCONST_COLLAPSE:
					m_visible=VISIBLE_COLLAPSE;
				break;
				}
			break;
			case HTMLATT_DISPLAY:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NONE:
					m_display=DISPLAY_NONE;
				break;
				case HTMLCONST_BLOCK:
					m_display=DISPLAY_BLOCK;
				break;
				case HTMLCONST_INLINE:
					if(m_display==DISPLAY_TABLE)
						m_display=DISPLAY_INLINE_TABLE;
					else
						m_display=DISPLAY_INLINE;
				break;
				case HTMLCONST_INLINE_BLOCK:
					m_display=DISPLAY_INLINE_BLOCK;
				break;
				case HTMLCONST_INLINE_TABLE:
					m_display=DISPLAY_INLINE_TABLE;
				break;
				case HTMLCONST_TABLE:
					m_display=DISPLAY_TABLE;
				break;
				case HTMLCONST_TABLE_CELL:
					m_display=DISPLAY_TABLE_CELL;
				break;
				case HTMLCONST_LIST_ITEM:
					m_display=DISPLAY_LIST_ITEM;
				break;
				case HTMLCONST_INHERIT:
					m_display=m_renderparent->m_display;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for DISPLAY\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_OVERFLOW_X:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_VISIBLE:
					m_overflowx=OVERFLOW_VISIBLE;
				break;
				case HTMLCONST_HIDDEN:
					m_overflowx=OVERFLOW_HIDDEN;
				break;
				case HTMLCONST_SCROLL:
					m_overflowx=OVERFLOW_SCROLL;
				break;
				case HTMLCONST_AUTO:
					m_overflowx=OVERFLOW_AUTO;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for OVERFLOWX\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_OVERFLOW_Y:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_VISIBLE:
					m_overflowy=OVERFLOW_VISIBLE;
				break;
				case HTMLCONST_HIDDEN:
					m_overflowy=OVERFLOW_HIDDEN;
				break;
				case HTMLCONST_SCROLL:
					m_overflowy=OVERFLOW_SCROLL;
				break;
				case HTMLCONST_AUTO:
					m_overflowy=OVERFLOW_AUTO;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for OVERFLOWY\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_TEXT_OVERFLOW:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_CLIP:
					m_textoverflow=TEXTOVERFLOW_CLIP;
				break;
				case HTMLCONST_ELLIPSIS:
					m_textoverflow=TEXTOVERFLOW_ELLIPSIS;
				break;
				case HTMLCONST_ELLIPSIS_WORD:
					m_textoverflow=TEXTOVERFLOW_ELLIPSIS_WORD;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for TEXTOVERFLOW\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_POSITION:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_ABSOLUTE:
					m_position=POSITION_ABSOLUTE;
				break;
				case HTMLCONST_FIXED:
					m_position=POSITION_FIXED;
				break;
				case HTMLCONST_RELATIVE:
					m_position=POSITION_RELATIVE;
				break;
				case HTMLCONST_STATIC:
					m_position=POSITION_STATIC;
				break;
				case HTMLCONST_INHERIT:
					m_position=m_renderparent->m_position;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for POSITION\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_FLOAT:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_LEFT:
					m_float=FLOAT_LEFT;
				break;
				case HTMLCONST_RIGHT:
					m_float=FLOAT_RIGHT;
				break;
				case HTMLCONST_NONE:
					m_float=FLOAT_NONE;
				break;
				case HTMLCONST_INHERIT:
					m_float=m_renderparent->m_float;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for FLOAT\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_CLEAR:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_LEFT:
					m_clear=CLEAR_LEFT;
				break;
				case HTMLCONST_RIGHT:
					m_clear=CLEAR_RIGHT;
				break;
				case HTMLCONST_NONE:
					m_clear=CLEAR_NONE;
				break;
				case HTMLCONST_ALL:
				case HTMLCONST_BOTH:
					m_clear=CLEAR_ALL;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for CLEAR\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_BACKGROUND:
			case HTMLATT_BACKGROUND_IMAGE:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NONE:
					m_bgimage.SetIsValid(false);
				break;
				case HTMLCONST_INHERIT:
					m_bgimage.SetIsValid(m_renderparent->m_bgimage.GetIsValid());
					if(m_bgimage.GetIsValid())
					{
						kGUIString *parenturl;

						if(slist->GetOwnerURL())
							parenturl=m_page->IDToString(slist->GetOwnerURL());
						else
							parenturl=&m_page->m_url;
						m_bgimage.SetURL(m_page,m_renderparent->m_bgimage.GetURL(),parenturl);
					}
				break;
				default:
				{
					kGUIString word;
					kGUIString url;
					kGUIString *parenturl;

					word.SetString(att->GetValue());
					word.Replace("url(","",0,1);
					word.Replace(")","");
					
					/* this is only defined on rules, not on inline tag styles so those refer to the page url */
					if(slist->GetOwnerURL())
						parenturl=m_page->IDToString(slist->GetOwnerURL());
					else
						parenturl=&m_page->m_url;

					m_page->MakeURL(parenturl,&word,&url);
					m_bgimage.SetURL(m_page,&url,parenturl);
					m_bgimage.SetIsValid(true);
				}
				break;
				}
			break;
			case HTMLATT_BACKGROUND_ATTACHMENT:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_INHERIT:
					m_bgfixed=m_renderparent->m_bgfixed;
				break;
				case HTMLCONST_SCROLL:
					m_bgfixed=false;
				break;
				case HTMLCONST_FIXED:
					m_bgfixed=true;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for BACKGROUND_ATTACHMENT\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_BACKGROUND_REPEAT:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_INHERIT:
					m_bgrepeatx=m_renderparent->m_bgrepeatx;
					m_bgrepeaty=m_renderparent->m_bgrepeatx;
				break;
				case HTMLCONST_REPEAT:
					m_bgrepeatx=true;
					m_bgrepeaty=true;
				break;
				case HTMLCONST_REPEATX:
					m_bgrepeatx=true;
					m_bgrepeaty=false;
				break;
				case HTMLCONST_REPEATY:
					m_bgrepeatx=false;
					m_bgrepeaty=true;
				break;
				case HTMLCONST_NOREPEAT:
					m_bgrepeatx=false;
					m_bgrepeaty=false;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for BACKGROUND_REPEAT\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_BACKGROUND_POSITIONX:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_INHERIT:
					m_bgx.CopyFrom(&m_renderparent->m_bgx);
				break;
				case HTMLCONST_LEFT:
					m_bgx.SetUnitType(UNITS_PERCENT);
					m_bgx.SetUnitValue(0);
				break;
				case HTMLCONST_CENTER:
					m_bgx.SetUnitType(UNITS_PERCENT);
					m_bgx.SetUnitValue(50);
				break;
				case HTMLCONST_RIGHT:
					m_bgx.SetUnitType(UNITS_PERCENT);
					m_bgx.SetUnitValue(100);
				break;
				default:
					m_bgx.Set(m_page,att->GetValue());
				break;
				}
			break;
			case HTMLATT_BACKGROUND_POSITIONY:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_INHERIT:
					m_bgy.CopyFrom(&m_renderparent->m_bgy);
				break;
				case HTMLCONST_TOP:
					m_bgy.SetUnitType(UNITS_PERCENT);
					m_bgy.SetUnitValue(0);
				break;
				case HTMLCONST_CENTER:
					m_bgy.SetUnitType(UNITS_PERCENT);
					m_bgy.SetUnitValue(50);
				break;
				case HTMLCONST_BOTTOM:
					m_bgy.SetUnitType(UNITS_PERCENT);
					m_bgy.SetUnitValue(100);
				break;
				default:
					m_bgy.Set(m_page,att->GetValue());
				break;
				}
			break;
			case HTMLATT_BACKGROUND_COLOR:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_CURRENTCOLOR:
					si->m_cc|=CURRENTCOLOR_BACKGROUND;
				break;
				case HTMLCONST_INHERIT:
					m_bgcolor.CopyFrom(&m_renderparent->m_bgcolor);
				break;
				default:
					m_page->GetColor(att->GetValue(),&m_bgcolor);
				break;
				}
			break;
			case HTMLATT_TEXT_ALIGN:
//				m_page->PushStyle(sizeof(m_page->m_textalign),&m_page->m_textalign);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_LEFT:
					m_textalign=ALIGN_LEFT;
				break;
				case HTMLCONST_RIGHT:
					m_textalign=ALIGN_RIGHT;
				break;
				case HTMLCONST_CENTER:
					m_textalign=ALIGN_CENTER;
				break;
				case HTMLCONST_JUSTIFY:
					m_textalign=ALIGN_JUSTIFY;
				break;
				case HTMLCONST_INHERIT:
					m_textalign=m_styleparent->m_textalign;
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for TEXT_ALIGN\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_TEXT_INDENT:
				/* inherited by all children elements */
				m_patttextindent=att;
//				m_page->PushStyle(sizeof(m_page->m_textindent),&m_page->m_textindent);
//				m_page->m_textindent.Set(m_page,att->GetValue());
			break;
			case HTMLATT_LETTER_SPACING:
				/* inherited by all children elements */
				m_page->PushStyle(sizeof(m_page->m_letterspacing),&m_page->m_letterspacing);
				m_page->m_letterspacing.Set(m_page,att->GetValue());
			break;
			case HTMLATT_WORD_SPACING:
				/* inherited by all children elements */
				m_page->PushStyle(sizeof(m_page->m_wordspacing),&m_page->m_wordspacing);
				m_page->m_wordspacing.Set(m_page,att->GetValue());
			break;
			case HTMLATT_TEXT_DECORATION:
				m_page->PushStyle(sizeof(m_page->m_textdecoration),&m_page->m_textdecoration);
				m_page->SetTextDecoration(att->GetValue()->GetInt());

				/* if the text-decoration was off but is now on, then we set a flag to copy the */
				/* text-color to the text-decoration-color. We don't copy it right now as at this */
				/* time we don't know what the text-color will be until all css has been applied */
				m_page->m_copytdc=true;
			break;
			case HTMLATT_TEXT_SHADOW_R:
				m_page->PushStyle(sizeof(m_page->m_textshadowr),&m_page->m_textshadowr);
				m_page->m_textshadowr=att->GetValue()->GetInt();
			break;
			case HTMLATT_TEXT_SHADOW_X:
			{
				kGUIUnits unit;

				m_page->PushStyle(sizeof(m_page->m_textshadowx),&m_page->m_textshadowx);
				unit.Set(m_page,att->GetValue());
				if(unit.GetUnitType()!=UNITS_UNDEFINED)
					m_page->m_textshadowx=unit.CalcUnitValue((int)m_em,(int)m_em);
				/* todo, else print error */
			}
			break;
			case HTMLATT_TEXT_SHADOW_Y:
			{
				kGUIUnits unit;
				m_page->PushStyle(sizeof(m_page->m_textshadowy),&m_page->m_textshadowy);
				unit.Set(m_page,att->GetValue());
				if(unit.GetUnitType()!=UNITS_UNDEFINED)
					m_page->m_textshadowy=unit.CalcUnitValue((int)m_em,(int)m_em);
				/* todo, else print error */
			}
			break;
			case HTMLATT_TEXT_SHADOW_COLOR:
				m_page->PushStyle(sizeof(m_page->m_textshadowcolor),&m_page->m_textshadowcolor);
				m_page->GetColor(att->GetValue(),&m_page->m_textshadowcolor);
			break;
			case HTMLATT_TEXT_TRANSFORM:
				m_page->PushStyle(sizeof(m_page->m_texttransform),&m_page->m_texttransform);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NONE:
					m_page->m_texttransform=TEXTTRANSFORM_NONE;
				break;
				case HTMLCONST_CAPITALIZE:
					m_page->m_texttransform=TEXTTRANSFORM_CAPITALIZE;
				break;
				case HTMLCONST_UPPERCASE:
					m_page->m_texttransform=TEXTTRANSFORM_UPPERCASE;
				break;
				case HTMLCONST_LOWERCASE:
					m_page->m_texttransform=TEXTTRANSFORM_LOWERCASE;
				break;
				}
			break;
#if 0
			case HTMLATT_ALIGN:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_LEFT:
					SetAlign(ALIGN_LEFT);
				break;
				case HTMLCONST_RIGHT:
					SetAlign(ALIGN_RIGHT);
				break;
				case HTMLCONST_JUSTIFY:
					SetAlign(ALIGN_JUSTIFY);
				break;
				case HTMLCONST_CENTER:
					SetAlign(ALIGN_CENTER);
				break;
				case HTMLCONST_ABSCENTER:
					SetAlign(ALIGN_ABSCENTER);
				break;
				case HTMLCONST_TOP:
					SetVAlign(VALIGN_TOP);
				break;
				case HTMLCONST_MIDDLE:
					SetVAlign(VALIGN_MIDDLE);
				break;
				case HTMLCONST_BOTTOM:
					SetVAlign(VALIGN_BOTTOM);
				break;
				case HTMLCONST_ABSBOTTOM:
					SetVAlign(VALIGN_ABSBOTTOM);
				break;
				case HTMLCONST_ABSMIDDLE:
					SetVAlign(VALIGN_ABSMIDDLE);
				break;
				case HTMLCONST_BASELINE:
					SetVAlign(VALIGN_BASELINE);
				break;
				default:
					m_page->m_errors.ASprintf("Unknown tag parm '%s' for ALIGN\n",att->GetValue()->GetString());
				break;
				}
			break;
#endif
			case HTMLATT_VALIGN:
			case HTMLATT_VERTICAL_ALIGN:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_TOP:
					SetVAlign(VALIGN_TOP);
				break;
				case HTMLCONST_MIDDLE:
					SetVAlign(VALIGN_MIDDLE);
				break;
				case HTMLCONST_BOTTOM:
					SetVAlign(VALIGN_BOTTOM);
				break;
				case HTMLCONST_CENTER:
					if(m_page->m_strict==true)
						goto valignerr;
					SetVAlign(VALIGN_MIDDLE);
				break;
				case HTMLCONST_ABSBOTTOM:
					SetVAlign(VALIGN_ABSBOTTOM);
				break;
				case HTMLCONST_ABSMIDDLE:
					SetVAlign(VALIGN_ABSMIDDLE);
				break;
				case HTMLCONST_SUB:
					SetVAlign(VALIGN_SUB);
				break;
				case HTMLCONST_SUPER:
					SetVAlign(VALIGN_SUPER);
				break;
				case HTMLCONST_BASELINE:
					SetVAlign(VALIGN_BASELINE);
				break;
				default:
					/* is it a value? */
					m_valignoffset.Set(m_page,att->GetValue());
					if(m_valignoffset.GetUnitType()!=UNITS_UNDEFINED)
						SetVAlign(VALIGN_OFFSET);
					else
valignerr:				m_page->m_errors.ASprintf("Unknown tag parm '%s' for VALIGN\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_BORDER_WIDTH:
				GetBox()->SetBorder(att->GetValue()->GetInt());
				applied[HTMLATT_BORDER_WIDTH_TOP]=appliedlevel;
				applied[HTMLATT_BORDER_WIDTH_BOTTOM]=appliedlevel;
				applied[HTMLATT_BORDER_WIDTH_LEFT]=appliedlevel;
				applied[HTMLATT_BORDER_WIDTH_RIGHT]=appliedlevel;
			break;
			case HTMLATT_BORDER_WIDTH_TOP:
				m_pattbordertop=att;
			break;
			case HTMLATT_BORDER_WIDTH_BOTTOM:
				m_pattborderbottom=att;
			break;
			case HTMLATT_BORDER_WIDTH_LEFT:
				m_pattborderleft=att;
			break;
			case HTMLATT_BORDER_WIDTH_RIGHT:
				m_pattborderright=att;
			break;
			case HTMLATT_BORDER_STYLE_TOP:
				GetBox()->SetBorderStyle(BORDER_TOP,att->GetValue());
			break;
			case HTMLATT_BORDER_STYLE_BOTTOM:
				GetBox()->SetBorderStyle(BORDER_BOTTOM,att->GetValue());
			break;
			case HTMLATT_BORDER_STYLE_LEFT:
				GetBox()->SetBorderStyle(BORDER_LEFT,att->GetValue());
			break;
			case HTMLATT_BORDER_STYLE_RIGHT:
				GetBox()->SetBorderStyle(BORDER_RIGHT,att->GetValue());
			break;
			case HTMLATT_BORDER_COLOR_TOP:
				if(att->GetVID()==HTMLCONST_CURRENTCOLOR)
					si->m_cc|=CURRENTCOLOR_BORDER_TOP;
				else
					GetBox()->SetBorderColor(BORDER_TOP,att->GetValue());
			break;
			case HTMLATT_BORDER_COLOR_BOTTOM:
				if(att->GetVID()==HTMLCONST_CURRENTCOLOR)
					si->m_cc|=CURRENTCOLOR_BORDER_BOTTOM;
				else
					GetBox()->SetBorderColor(BORDER_BOTTOM,att->GetValue());
			break;
			case HTMLATT_BORDER_COLOR_LEFT:
				if(att->GetVID()==HTMLCONST_CURRENTCOLOR)
					si->m_cc|=CURRENTCOLOR_BORDER_LEFT;
				else
					GetBox()->SetBorderColor(BORDER_LEFT,att->GetValue());
			break;
			case HTMLATT_BORDER_COLOR_RIGHT:
				if(att->GetVID()==HTMLCONST_CURRENTCOLOR)
					si->m_cc|=CURRENTCOLOR_BORDER_RIGHT;
				else
					GetBox()->SetBorderColor(BORDER_RIGHT,att->GetValue());
			break;
			case HTMLATT_MARGIN_LEFT:
				m_pattmarginleft=att;
			break;
			case HTMLATT_MARGIN_RIGHT:
				m_pattmarginright=att;
			break;
			case HTMLATT_MARGIN_TOP:
				m_pattmargintop=att;
			break;
			case HTMLATT_MARGIN_BOTTOM:
				m_pattmarginbottom=att;
			break;
			case HTMLATT_PADDING_LEFT:
				/* todo: if NEG or malformed then don't set */
				m_pattpaddingleft=att;
			break;
			case HTMLATT_PADDING_RIGHT:
				/* todo: if NEG or malformed then don't set */
				m_pattpaddingright=att;
			break;
			case HTMLATT_PADDING_TOP:
				/* todo: if NEG or malformed then don't set */
				m_pattpaddingtop=att;
			break;
			case HTMLATT_PADDING_BOTTOM:
				/* todo: if NEG or malformed then don't set */
				m_pattpaddingbottom=att;
			break;
			case HTMLATT_BORDER_COLLAPSE:
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_COLLAPSE:
				break;
				case HTMLCONST_SEPARATE:
				break;
				case HTMLCONST_INHERIT:
				break;
				default:
					m_page->m_errors.ASprintf("unknown type ('%s') for BORDER_COLLAPSE!\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_BORDER_SPACING_HORIZ:
				//fuck
			break;
			case HTMLATT_BORDER_SPACING_VERT:
				//fuck
			break;
			case HTMLATT_OUTLINE_WIDTH:
			{
				kGUIUnits unit;
				int wpix;

				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_THIN:
					wpix=1;
				break;
				case HTMLCONST_MEDIUM:
					wpix=3;
				break;
				case HTMLCONST_THICK:
					wpix=5;
				break;
				case HTMLCONST_LENGTH:
					wpix=14;
				break;
				case -1:
					/* todo, get width as units may be a percent?? */
					unit.Set(m_page,att->GetValue());
					wpix=unit.CalcUnitValue(0,m_renderparent->GetEM());
				break;
				default:
					m_page->m_errors.ASprintf("unknown outline-width '%s'\n",att->GetValue()->GetString());
					return;
				break;
				}
				m_outlinewidth=wpix;
			}
			break;
			case HTMLATT_OUTLINE_STYLE:
				//fuck
			break;
			case HTMLATT_OUTLINE_COLOR:
				m_page->GetColor(att->GetValue(),&m_outlinecolor);
			break;
			case HTMLATT_CELLSPACING:
				m_page->PushStyle(sizeof(m_page->m_cellspacing),&m_page->m_cellspacing);
				m_page->m_cellspacing=att->GetValue()->GetInt();
			break;
			case HTMLATT_CELLPADDING:
				m_page->PushStyle(sizeof(m_page->m_cellpadding),&m_page->m_cellpadding);
				m_page->m_cellpadding=att->GetValue()->GetInt();
			break;
			case HTMLATT_COLOR:
				m_page->PushStyle(sizeof(m_page->m_fontcolor),&m_page->m_fontcolor);
				m_page->GetColor(att->GetValue(),&m_page->m_fontcolor);
			break;
			case HTMLATT_OPACITY:
			{
				double opacity;

				opacity=att->GetValue()->GetDouble();
				if(opacity<0.0f)
					opacity=0.0f;
				else if(opacity>1.0f)
					opacity=1.0f;
				m_opacity=opacity;
			}
			break;
			case HTMLATT_LANG:
				m_page->PushStyle(sizeof(m_page->m_lang),&m_page->m_lang);
				m_page->m_lang=m_page->StringToID(att->GetValue());
			break;
			case HTMLATT_FONT_FAMILY:
			break;
			case HTMLATT_FONT_STYLE:
			break;
			case HTMLATT_FONT_VARIANT:
			break;
			case HTMLATT_FONT_WEIGHT:
				m_page->PushStyle(sizeof(m_page->m_fontweight),&m_page->m_fontweight);

				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NORMAL:
					m_page->m_fontweight=400;
				break;
				case HTMLCONST_BOLD:
					m_page->m_fontweight=700;
				break;
				case HTMLCONST_BOLDER:
					m_page->m_fontweight=900;
				break;
				case HTMLCONST_LIGHTER:
					m_page->m_fontweight=100;
				break;
				default:
				{
					int w;

					w=att->GetValue()->GetInt();
					if(w<100)
						w=100;
					else if(w>900)
						w=900;
					m_page->m_fontweight=w;
				}
				break;
				}
			break;
			case HTMLATT_FONT_SIZE:
			{
				kGUIUnits fs;

				m_page->PushStyle(sizeof(m_page->m_fontsize),&m_page->m_fontsize);

				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_XXSMALL:
					m_page->m_fontsize=xfontsize[0];
				break;
				case HTMLCONST_XSMALL:
					m_page->m_fontsize=xfontsize[1];
				break;
				case HTMLCONST_SMALL:
					m_page->m_fontsize=xfontsize[2];
				break;
				case HTMLCONST_MEDIUM:
				case HTMLCONST_NORMAL:
					m_page->m_fontsize=xfontsize[3];
				break;
				case HTMLCONST_LARGE:
					m_page->m_fontsize=xfontsize[4];
				break;
				case HTMLCONST_XLARGE:
					m_page->m_fontsize=xfontsize[5];
				break;
				case HTMLCONST_XXLARGE:
					m_page->m_fontsize=xfontsize[6];
				break;
				case HTMLCONST_SMALLER:
					m_page->m_fontsize=max(xfontsize[0],m_page->m_fontsize/1.2f);
				break;
				case HTMLCONST_LARGER:
					m_page->m_fontsize=min(xfontsize[6],m_page->m_fontsize*1.2f);
				break;
				case HTMLCONST_INHERIT:
				break;
				default:
					fs.Set(m_page,att->GetValue());
					if(fs.GetUnitType()!=UNITS_UNDEFINED)
					{
						m_page->m_fontsize=fs.CalcUnitValue(m_page->m_fontsize,m_page->m_fontsize);

						if(m_page->m_fontsize<1.0f)
							m_page->m_fontsize=1.0f;
					}
					else
						kGUI::Trace("Unknown attribute value for font-size '%s'\n",att->GetValue());
				break;
				}
				m_page->CalcEM();
				m_page->CalcLH();
			}
			break;
			case HTMLATT_FONT_SIZE_ADJUST:
			{
				kGUIUnits fsa;

				m_page->PushStyle(sizeof(m_page->m_fontscale),&m_page->m_fontscale);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NONE:
					m_page->m_fontscale=1.0f;
				break;
				default:
					fsa.Set(m_page,att->GetValue());
					m_page->m_fontscale=fsa.CalcUnitValue(m_page->m_fontscale,m_page->m_fontscale);
				break;
				}
	
				m_page->CalcEM();
				m_page->CalcLH();
			}
			break;
			case HTMLATT_LINE_HEIGHT:
				m_page->PushStyle(sizeof(m_page->m_lineheightratio),&m_page->m_lineheightratio);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NORMAL:
					m_page->m_lineheightratio.SetUnitType(UNITS_PERCENT);
					m_page->m_lineheightratio.SetUnitValue(120);
				break;
				default:
					m_page->m_lineheightratio.Set(m_page,att->GetValue());
				break;
				}
				m_page->CalcLH();
			break;
			case HTMLATT_WHITE_SPACE:
				m_page->PushStyle(sizeof(m_page->m_whitespace),&m_page->m_whitespace);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NORMAL:
					m_page->m_whitespace=WHITESPACE_NORMAL;
				break;
				case HTMLCONST_NOWRAP:
					m_page->m_whitespace=WHITESPACE_NOWRAP;
				break;
				case HTMLCONST_PRE:
					m_page->m_whitespace=WHITESPACE_PRE;
				break;
				case HTMLCONST_PREWRAP:
					m_page->m_whitespace=WHITESPACE_PREWRAP;
				break;
				case HTMLCONST_PRELINE:
					m_page->m_whitespace=WHITESPACE_PRELINE;
				break;
//				case HTMLCONST_INHERIT:
//					m_whitespace=m_styleparent->m_whitespace;
//				break;
				default:
					m_page->m_errors.ASprintf("unknown type ('%s') for WHITE-SPACE!\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_HREF:
				if(m_id==HTMLTAG_BASE)
				{
					/* hmmm, are we actually using this? should we push/pop it? */				
					/* set the base URL */
					m_page->SetBaseURL(att->GetValue());
				}
			break;
			case HTMLATT_REL:
			break;
			case HTMLATT_HEIGHT:
				m_height.Set(m_page,att->GetValue());
			break;
			case HTMLATT_WIDTH:
				m_width.Set(m_page,att->GetValue());
			break;
			case HTMLATT_MINWIDTH:
				m_minwidth.Set(m_page,att->GetValue());
			break;
			case HTMLATT_MAXWIDTH:
				m_maxwidth.Set(m_page,att->GetValue());
			break;
			case HTMLATT_MINHEIGHT:
				m_minheight.Set(m_page,att->GetValue());
			break;
			case HTMLATT_MAXHEIGHT:
				m_maxheight.Set(m_page,att->GetValue());
			break;
			case HTMLATT_LIST_STYLE_IMAGE:
			{
				kGUIString word;
				kGUIString url;
				kGUIString *parenturl;

				m_page->PushStyle(sizeof(m_page->m_liststyleurl),&m_page->m_liststyleurl);
				m_page->PushStyle(sizeof(m_page->m_liststyleurlreferer),&m_page->m_liststyleurlreferer);
				m_page->PushStyle(sizeof(m_page->m_liststyle),&m_page->m_liststyle);

				//block lower pri list-style-type
				applied[HTMLATT_LIST_STYLE_TYPE]=appliedlevel;

				if(att->GetVID()==HTMLCONST_NONE)
				{
					m_page->m_liststyleurl=0;
					m_page->m_liststyleurlreferer=0;
					m_page->m_liststyle=LISTSTYLE_NONE;
				}
				else
				{
					word.SetString(att->GetValue());
					word.Replace("url(","",0,1);
					word.Replace(")","");
					
					/* this is only defined on rules, not on inline tag styles so those refer to the page url */
					if(slist->GetOwnerURL())
						parenturl=m_page->IDToString(slist->GetOwnerURL());
					else
						parenturl=&m_page->m_url;

					m_page->MakeURL(parenturl,&word,&url);
				
					m_page->m_liststyle=LISTSTYLE_IMAGE;
					m_page->m_liststyleurl=m_page->StringToID(&url);
					m_page->m_liststyleurlreferer=m_page->StringToID(parenturl);
				}
			}
			break;
			case HTMLATT_LIST_STYLE_POSITION:
				m_page->PushStyle(sizeof(m_page->m_liststyleposition),&m_page->m_liststyleposition);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_INSIDE:
					m_page->m_liststyleposition=LISTSTYLE_INSIDE;
				break;
				case HTMLCONST_OUTSIDE:
					m_page->m_liststyleposition=LISTSTYLE_OUTSIDE;
				break;
				default:
					m_page->m_errors.ASprintf("unknown type ('%s') for LIST-STYLE-POSITION!\n",att->GetValue()->GetString());
				break;
				}
			break;
			case HTMLATT_LIST_STYLE_TYPE:
				m_page->PushStyle(sizeof(m_page->m_liststyle),&m_page->m_liststyle);
				code=att->GetVID();
				switch(code)
				{
				case HTMLCONST_NONE:	//none  	No marker
					m_page->m_liststyle=LISTSTYLE_NONE;
				break;
				case HTMLCONST_DISC:	//disc 	Default. The marker is a filled circle
					m_page->m_liststyle=LISTSTYLE_DISC;
				break;
				case HTMLCONST_CIRCLE:	//circle 	The marker is a circle
					m_page->m_liststyle=LISTSTYLE_CIRCLE;
				break;
				case HTMLCONST_SQUARE:	//square 	The marker is a square
					m_page->m_liststyle=LISTSTYLE_SQUARE;
				break;
				case HTMLCONST_DECIMAL:	//decimal 	The marker is a number
					m_page->m_liststyle=LISTSTYLE_DECIMAL;
				break;
				case HTMLCONST_DECIMAL_LEADING_ZERO:	//decimal-leading-zero 	The marker is a number padded by initial zeros (01, 02, 03, etc.)
					m_page->m_liststyle=LISTSTYLE_DECIMAL_LEADING_ZERO;
				break;
				case HTMLCONST_LOWER_ROMAN:	//lower-roman 	The marker is lower-roman (i, ii, iii, iv, v, etc.)
					m_page->m_liststyle=LISTSTYLE_LOWER_ROMAN;
				break;
				case HTMLCONST_UPPER_ROMAN:	//upper-roman 	The marker is upper-roman (I, II, III, IV, V, etc.)
					m_page->m_liststyle=LISTSTYLE_UPPER_ROMAN;
				break;
				case HTMLCONST_LOWER_ALPHA:	//lower-alpha 	The marker is lower-alpha (a, b, c, d, e, etc.)
				case HTMLCONST_LOWER_LATIN:	//lower-latin 	The marker is lower-latin (a, b, c, d, e, etc.)
					m_page->m_liststyle=LISTSTYLE_LOWER_ALPHA;
				break;
				case HTMLCONST_UPPER_ALPHA:	//upper-alpha 	The marker is upper-alpha (A, B, C, D, E, etc.) 
				case HTMLCONST_UPPER_LATIN:	//upper-latin 	The marker is upper-latin (A, B, C, D, E, etc.)
					m_page->m_liststyle=LISTSTYLE_UPPER_ALPHA;
				break;
				case HTMLCONST_LOWER_GREEK:	//lower-greek 	The marker is lower-greek (alpha, beta, gamma, etc.)
					m_page->m_liststyle=LISTSTYLE_LOWER_GREEK;
				break;
				default:
					m_page->m_errors.ASprintf("unknown type ('%s') for LIST-STYLE-TYPE!\n",att->GetValue()->GetString());
				break;
				}
//hebrew 	The marker is traditional Hebrew numbering
//armenian 	The marker is traditional Armenian numbering
//georgian 	The marker is traditional Georgian numbering (an, ban, gan, etc.)
//cjk-ideographic 	The marker is plain ideographic numbers
//hiragana 	The marker is: a, i, u, e, o, ka, ki, etc.
//katakana 	The marker is: A, I, U, E, O, KA, KI, etc.
//hiragana-iroha 	The marker is: i, ro, ha, ni, ho, he, to, etc.
//katakana-iroha
			break;
			case HTMLATT_CONTENT_BEFORE:
				m_page->m_beforecontent=att;
			break;
			case HTMLATT_CONTENT_AFTER:
				m_page->m_aftercontent=att;
			break;
			}
		}
	}
}

kGUIHTMLObj *kGUIHTMLObj::GetParentObj(void)
{
	kGUIHTMLObj *p;

	p=m_renderparent;
	if(p)
	{
		while(p->m_display==DISPLAY_INLINE)
			p=p->m_renderparent;
	}
	return(p);
}

void kGUIHTMLObj::ClipMinMaxWidth(int pw)
{
	int iw,mw;
	bool clipped;

	clipped=false;
	iw=GetInsideW();
	if(m_maxwidth.GetUnitType()!=UNITS_UNDEFINED)
	{
		mw=m_maxwidth.CalcUnitValue(pw,m_em);
		if(iw>mw)
		{
			iw=mw;
			clipped=true;
		}
	}		
	if(m_minwidth.GetUnitType()!=UNITS_UNDEFINED)
	{
		mw=m_minwidth.CalcUnitValue(pw,m_em);
		if(iw<mw)
		{
			iw=mw;
			clipped=true;
		}
	}
	if(clipped)
		SetInsideW(iw);
}

void kGUIHTMLObj::ClipMinMaxHeight(int ph)
{
	int ih,mh;
	bool clipped;

	clipped=false;
	ih=GetInsideH();
	if(m_maxheight.GetUnitType()!=UNITS_UNDEFINED)
	{
		mh=m_maxheight.CalcUnitValue(ph,m_em);
		if(ih>mh)
		{
			ih=mh;
			clipped=true;
		}
	}		
	if(m_minheight.GetUnitType()!=UNITS_UNDEFINED)
	{
		mh=m_minheight.CalcUnitValue(ph,m_em);
		if(ih<mh)
		{
			ih=mh;
			clipped=true;
		}
	}
	if(clipped)
		SetInsideH(ih);
}


/* check for fixed position and or size, this is called on both passes as */
/* positions may be percentages of parents and needs to be calced on both passes */

void kGUIHTMLObj::CheckFixed(void)
{
	int pw;
	int ph;
	int ow;
	int oh;
	int negmarginleft;
	int negmarginright;
	int negmargintop;
	int negmarginbottom;
	kGUIHTMLObj *p=GetParentObj();

	/* on the min/max pass the parent width is undefined so use 0 */
	if(!p || (m_page->m_mode==MODE_MINMAX))
	{
		pw=0;
		ph=0;
	}
	else
	{
		pw=p->GetChildZoneW();		/* width for children to be placed */
		ph=p->GetChildZoneH();		/* height for children to be placed */
	}

	m_fixedw=false;
	m_fixedh=false;
	m_fixedpos=false;
	m_abspos=false;
	m_relw=false;
	m_relh=false;
	m_relpos=false;

	/* there is a defined width */
	if(m_width.GetUnitType()!=UNITS_UNDEFINED && m_width.GetUnitType()!=UNITS_AUTO)
	{
#if 1
		if(m_float!=FLOAT_NONE && m_width.GetUnitType()==UNITS_PERCENT && m_width.GetUnitValue()==100)
		{
			/* width 100% is a special case that means use my max width */
			/* but clip to my parents width */
			ow=m_maxw;
			if(ow>pw)
				ow=pw;

			m_fixedw=true;
		}
		else
#endif
		{
			ow=m_width.CalcUnitValue(pw,m_em);
			if(m_width.GetUnitType()==UNITS_PERCENT)
				m_relw=true;
			else
				m_fixedw=true;
		}

		SetInsideW(ow);
		ClipMinMaxWidth(pw);
	}

	/* there is a defined height */
	if(m_height.GetUnitType()!=UNITS_UNDEFINED && m_height.GetUnitType()!=UNITS_AUTO)
	{
		oh=m_height.CalcUnitValue(ph,m_em);
		SetInsideH(oh);
		ClipMinMaxHeight(ph);

		/* if we reference our parents height then the position needs to do two passes */
		if(m_height.GetUnitType()==UNITS_PERCENT)
		{
			if(m_height.GetUnitValue()!=100)
			{
				p->m_childusesheight=true;
				m_relh=true;
			}
		}
		else
			m_fixedh=true;
	}

	ow=GetOutsideW();	/* object width */
	oh=GetOutsideH();	/* object height */

	//if(m_position!=POSITION_STATIC)
	{
		int xoff=0,yoff=0;

		if(m_left.GetUnitType()!=UNITS_UNDEFINED)
		{
			if(m_left.GetUnitType()==UNITS_AUTO)
			{
				xoff=0;	/* what do I do? */
			}
			else
			{
				xoff=m_left.CalcUnitValue(pw,m_em);
				if(m_right.GetUnitType()==UNITS_AUTO)
				{
					/* huh? */
				}
				else if(m_right.GetUnitType()!=UNITS_UNDEFINED)
				{
					int rx=pw-m_right.CalcUnitValue(pw,m_em);
					ow=rx-xoff;
					SetOutsideW(ow);
					m_fixedw=true;
				}
			}
		}
		else
		{
			if(m_right.GetUnitType()!=UNITS_UNDEFINED)
				xoff=pw-ow-m_right.CalcUnitValue(pw,m_em);
			else
				xoff=0;
		}

		if(m_top.GetUnitType()==UNITS_AUTO)
		{
			/* what to do here? */
			yoff=0;
		}
		else if(m_top.GetUnitType()!=UNITS_UNDEFINED)
			yoff=m_top.CalcUnitValue(ph,m_em);
		else
		{
			if(m_bottom.GetUnitType()!=UNITS_UNDEFINED && m_bottom.GetUnitType()!=UNITS_AUTO)
			{
				yoff=ph-(oh+m_bottom.CalcUnitValue(ph,m_em));
				p->m_childusesheight=true;
			}
			else
				yoff=0;
		}

		/* handle negative margins */
		if(m_box)
		{
			negmarginleft=m_box->GetBoxLeftMargin();
			negmargintop=m_box->GetBoxTopMargin();
			negmarginright=m_box->GetBoxRightMargin();
			negmarginbottom=m_box->GetBoxBottomMargin();
			
			if(negmarginleft<0 && negmarginright<0)
			{
				/* expand width */
				xoff+=negmarginleft;
				SetOutsideW(GetOutsideW()-(negmarginleft+negmarginright));
			}
			else
			{
				if(negmarginleft<0 && m_right.GetUnitType()==UNITS_UNDEFINED)
					xoff+=negmarginleft;
				if(negmarginright<0 && m_left.GetUnitType()==UNITS_UNDEFINED)
					xoff-=negmarginright;
			}

			if(negmargintop<0 && negmarginbottom<0)
			{
				/* expand height */
				yoff+=negmargintop;
				SetOutsideW(GetOutsideW()-(negmarginleft+negmarginright));
			}
			else
			{
				if(negmargintop<0 && m_bottom.GetUnitType()==UNITS_UNDEFINED)
					yoff+=negmargintop;
				if(negmarginbottom<0 && m_top.GetUnitType()==UNITS_UNDEFINED)
					yoff-=negmarginbottom;
			}
		}

		switch(m_position)
		{
		case POSITION_ABSOLUTE:
			/* position the box relative to the containing box */
			MoveZoneX(xoff);
			MoveZoneY(yoff);
			m_abspos=true;
		break;
		case POSITION_FIXED:
			/* position the box relative to the containing box, and don't move if page scrolls */
			/* make this a box object if it is not already */
			if(m_display==DISPLAY_INLINE)
				m_display=DISPLAY_BLOCK;
			MoveZoneX(xoff);
			MoveZoneY(yoff);
			m_fixedpos=true;
		break;
		case POSITION_STATIC:
			if(xoff || yoff)
			{
				MoveZoneX(xoff);
				MoveZoneY(yoff);
				m_relpos=true;
			}
		break;
		case POSITION_RELATIVE:
			/* position the box relative to the default position */
			MoveZoneX(xoff);
			MoveZoneY(yoff);
			m_relpos=true;
//			m_relx=xoff;
//			m_rely=yoff;
		break;
		}
	}
}

kGUIHTMLFormObj *kGUIHTMLObj::GetForm(kGUIHTMLObj *o)
{
	kGUIHTMLObj *p=o->m_styleparent;

	if(p)
	{
		do
		{
			if(p->GetID()==HTMLTAG_FORM)
				return (p->m_obj.m_formobj);

			p=p->m_styleparent;
		}while(p);
	}
	/* this object is not attached to a form */
	return(0);
}

void kGUIHTMLObj::RadioChanged(kGUIEvent *event)
{
	if(event->GetEvent()==EVENT_AFTERUPDATE)
	{
		kGUIHTMLFormObj *f;
		unsigned int i;
		kGUIHTMLObj *o;
		kGUIHTMLAttrib *att;
		kGUIHTMLAttrib *oatt;

		/* turn all other radio buttons with this same name as me to unselected */
		att=FindAttrib(HTMLATT_NAME);
		f=GetForm(this);
		if(f && att)
		{
			/* un-select all other ratio buttons with the same name as me ( but not me! ) */
			for(i=0;i<f->GetNumChildren();++i)
			{
				o=f->GetChild(i);
				if(o->GetSubID()==HTMLSUBTAG_INPUTRADIO && o!=this)
				{
					oatt=o->FindAttrib(HTMLATT_NAME);

					/* make sure names match too */
					if(!stricmp(att->GetValue()->GetString(),oatt->GetValue()->GetString()))
						o->m_obj.m_radioobj->SetSelected(false);
				}
			}
		}
	}
}

unsigned int kGUIHTMLPageObj::AddTCI(kGUIString *tci,kGUIHTMLObj *o)
{
	unsigned int i;
	unsigned int oldlen=tci->GetLen();
	CID_DEF *cid;

	tci->ASprintf("<%d",o->GetID());

	/* hmmm different order will cause same strings to non-match, for example */
	/* is there an easy quick fix? */
	/* class="a b", id="c" */
	/* id="c" class="b a" */

	for(i=0;i<o->m_numcids;++i)
	{
		cid=o->m_cids.GetEntryPtr(i);
		switch(cid->m_type)
		{
		case CID_CLASS:
			tci->ASprintf(".%d",cid->m_id);
		break;
		case CID_ID:
			tci->ASprintf("#%d",cid->m_id);
		break;
		}
	}
	tci->Append(">");
	return(oldlen);
}


void kGUIHTMLObj::PositionHardBreak(int lineheight)
{
	assert(m_display!=DISPLAY_INLINE,"Cannot position inside an inline object!\n");

	if(!m_pos->m_lineasc)
		m_pos->m_lineasc=lineheight;

	PositionBreak();
}

void kGUIHTMLObj::Clear(int c)
{
	int h;

	switch(c)
	{
	case CLEAR_LEFT:
		h=m_pos->m_lefth;
	break;
	case CLEAR_RIGHT:
		h=m_pos->m_righth;
	break;
	case CLEAR_ALL:
		h=max(m_pos->m_righth,m_pos->m_lefth);
	break;
	default:
		return;
	break;
	}
	m_pos->m_lineasc=h;
	PositionBreak();
}

void kGUIHTMLObj::PositionBreak(bool wrap)
{
	int i,y,by,xoff;
	int rx,ry;
	double gap=0.0f;
	PO_DEF *pop;
	kGUIObj *robj;

	assert(m_display!=DISPLAY_INLINE,"Cannot position inside an inline object!\n");

	//if margin-left is Auto and margin-right is Auto then center object
	//else if margin-left is Auto then align=right
	//else if margin-right is Auto then align=left

	switch(m_potextalign)
	{
	case ALIGN_CENTER:
		CalcChildZone();
		xoff=((GetChildZoneW()-m_pos->m_leftw-m_pos->m_rightw)-m_pos->m_curx)/2;
	break;
	case ALIGN_RIGHT:
		CalcChildZone();
		xoff=(GetChildZoneW()-m_pos->m_leftw-m_pos->m_rightw)-m_pos->m_curx;
	break;
	case ALIGN_JUSTIFY:
		/* calc rightmost x, then calc gap */
		if(wrap==false)
			gap=0.0f;
		else
			gap=(double)((GetChildZoneW()-m_pos->m_leftw-m_pos->m_rightw)-m_pos->m_curx);
		xoff=0;
	break;
	default:
		if(m_dir==TEXTDIR_RTL)
		{
			/* text is displayed right to left not left to right */
			CalcChildZone();
			xoff=(GetChildZoneW()-m_pos->m_leftw-m_pos->m_rightw)-m_pos->m_curx;
		}
		else
			xoff=0;
	break;
	}

	//change renderparent to getparentobj
	if(!m_pos->m_line && m_patttextindent)
	{
		kGUIUnits tiu;

		tiu.Set(m_page,m_patttextindent->GetValue());
		m_pos->m_curx=tiu.CalcUnitValue((m_page->m_mode==MODE_MINMAX || !m_renderparent)?0:m_renderparent->GetChildZoneW(),m_em);
	}
	else
	{
		m_pos->m_curx=0;
	}
	by=m_pos->m_cury+m_pos->m_lineasc+m_pos->m_linedesc;
	for(i=0;i<m_pos->m_ponum;++i)
	{
		pop=m_pos->m_polist.GetEntryPtr(i);
		robj=pop->robj;

		/* error checking */
		assert(robj->GetZoneW()==pop->width && robj->GetZoneH()==pop->height,"Size Changed!\n");

		if(pop->obj->m_relpos)
		{
			rx=robj->GetZoneX();
			ry=robj->GetZoneY();
		}
		else
		{
			rx=0;
			ry=0;
		}
		if(m_page->m_mode==MODE_MINMAX)
			robj->MoveZoneX(m_pos->m_leftw+m_pos->m_curx+rx /*+obj->GetZoneX()*/ );
		else
		{
			if(m_potextalign==ALIGN_JUSTIFY && gap!=0.0f)
				xoff=(int)((i*gap)/m_pos->m_ponum);
			robj->MoveZoneX(m_pos->m_leftw+m_pos->m_curx+xoff+rx /*+obj->GetZoneX()*/);
		}
//		obj->MoveZoneY(m_y+(m_lineasc-obj->GetZoneH())+obj->GetZoneY());
		robj->MoveZoneY(m_pos->m_cury+((m_pos->m_lineasc-robj->GetZoneH())+pop->baseline)+ry);

		if(robj->GetZoneY()<0)
			kGUI::Trace("Error!\n");

		/* if this is the last child of an inline tag then call ContainChildren to make the box surround them */

		if(m_page->m_trace)
		{
			if(m_tag)
				kGUI::Trace("Position Child %s (%d,%d,w=%d,h=%d)\n",m_tag->name,robj->GetZoneX(),robj->GetZoneY(),robj->GetZoneW(),robj->GetZoneH());
			else
				kGUI::Trace("Position Child %d (%d,%d,w=%d,h=%d)\n",m_id,robj->GetZoneX(),robj->GetZoneY(),robj->GetZoneW(),robj->GetZoneH());
		}
		y=robj->GetZoneY()+robj->GetZoneH();
		if(y>by)
			by=y;
		m_pos->m_curx+=pop->width;	//robj->GetZoneW();
	}
	m_pos->m_cury=by;
	if(m_pos->m_cury>m_maxy)
		m_maxy=m_pos->m_cury;

	if(m_page->m_trace && m_tag)
		kGUI::Trace("Parent (%s) y=%d,maxy=%d\n",m_tag->name,m_pos->m_cury,m_maxy);

	if(m_pos->m_lefth)
	{
		m_pos->m_lefth-=m_pos->m_lineasc+m_pos->m_linedesc;
		if(m_pos->m_lefth<=0)
		{
			m_pos->m_lefth=0;
			m_pos->m_leftw=0;
		}
	}
	if(m_pos->m_righth)
	{
		m_pos->m_righth-=m_pos->m_lineasc+m_pos->m_linedesc;
		if(m_pos->m_righth<=0)
		{
			m_pos->m_righth=0;
			m_pos->m_rightw=0;
		}
	}

	m_pos->m_lineasc=0;
	m_pos->m_linedesc=0;
	m_pos->m_ponum=0;
	m_pos->m_curx=0;
	m_pos->m_line++;
}

/* position container object within another container object */

void kGUIHTMLObj::PositionChild(kGUIHTMLObj *obj,kGUIObj *robj,int asc,int desc)
{
	int by,down;
	int rx;
	int ow=robj->GetZoneW();		/* object width */
	int oh=robj->GetZoneH();		/* object height */
	int pw=GetChildZoneW();		/* width for children to be placed */
	int above=0,below=0;
	PO_DEF po;
	bool isbox;

	assert(m_display!=DISPLAY_INLINE,"Cannot position inside an inline object!\n");

	/* am I a box object? */
	switch(obj->m_display)
	{
	case DISPLAY_BLOCK:
	case DISPLAY_LIST_ITEM:
	case DISPLAY_TABLE:
		isbox=true;
	break;
	default:
		isbox=false;
	break;
	}

	switch(obj->GetVAlign())
	{
	case VALIGN_MIDDLE:
	case VALIGN_ABSMIDDLE:
		/* 1/2 ascender, 1/2 descender */
		isbox=false;
		desc=asc=oh>>1;
		if(oh&1)			/* if odd height then give asc 1 more */
			++asc;
	break;
	case VALIGN_TOP:
		/* no ascender, all descender */
		isbox=false;
		asc=0;
		desc=oh;
	break;
	case VALIGN_BOTTOM:
//	case VALIGN_BASELINE:
		/* all ascender, no descender */
		isbox=false;
		asc=oh;
		desc=0;
	break;
	case VALIGN_ABSBOTTOM:
		/* all ascender, no descender */
		isbox=false;
		asc=oh;
		desc=0;
	break;
	case VALIGN_OFFSET:
		if(obj->m_valignoffset.GetUnitType()==UNITS_PERCENT)
			m_childusesheight=true;

		/* add offset to position */
		asc+=obj->m_valignoffset.CalcUnitValue(GetChildZoneH(),m_em);
	break;
	case VALIGN_SUB:
		isbox=false;		/* move up descender height */
		asc+=desc;
		desc=0;
	break;
	case VALIGN_SUPER:
		isbox=false;		/* move down descender height */
		asc-=desc;
		desc+=desc;
	break;
	}

	if(m_page->m_trace && m_page->m_mode==MODE_POSITION)
	{
		switch(obj->m_id)
		{
		case HTMLTAG_IMBEDTEXT:
			kGUI::Trace("PositionObj (%s) w=%d,h=%d,asc=%d.desc=%d\n",obj->m_obj.m_textobj->GetString(),ow,oh,asc,desc);
		break;
		case HTMLTAG_IMG:
			kGUI::Trace("PositionObj (%S) w=%d,h=%d,asc=%d.desc=%d\n",obj->m_obj.m_imageobj->GetURL(),ow,oh,asc,desc);
		break;
		default:
			kGUI::Trace("PositionObj (%s) w=%d,h=%d,asc=%d.desc=%d\n",obj->m_tag->name,ow,oh,asc,desc);
		break;
		}
	}

	if(obj->GetID()==HTMLTAG_IMBEDTEXT)
	{
		int extra;

		/* if lineheight is taller the add extra space to top/bottom */
		extra=obj->m_lh-(asc+desc);
		if(extra>0)
		{
			above=extra>>1;
			below=extra-above;
		}
		goto dotext;
	}

	if(obj->m_fixedpos)
	{
		rx=robj->GetZoneX()+ow;
		by=max(robj->GetZoneY()+oh,robj->GetZoneY()+obj->m_maxchildy);
		if(rx>m_minw)
			m_minw=rx;
		if(rx>m_maxw)
			m_maxw=rx;
		if(by>m_maxy)
			m_maxy=by;
		return;
	}
	else if(obj->m_abspos)
	{
		rx=robj->GetZoneX()+ow;
		by=max(robj->GetZoneY()+oh,robj->GetZoneY()+obj->m_maxchildy);
		if(rx>m_minw)
			m_minw=rx;
		if(rx>m_maxw)
			m_maxw=rx;
		if(by>m_maxy)
			m_maxy=by;
		return;
	}


	if(obj->GetAlign()==ALIGN_LEFT || obj->m_float==FLOAT_LEFT)
	{
		if(m_pos->m_ponum)
			PositionBreak();

		if(m_page->m_mode==MODE_POSITION)
		{
			/* is there room for this?, if not, move down until there is */
			while(m_pos->m_leftw || m_pos->m_rightw)
			{
				if(m_pos->m_leftw+ow+m_pos->m_rightw<=pw)
					break;

				/* no room, move down */
				if(m_pos->m_lefth && m_pos->m_righth)
					down=min(m_pos->m_lefth,m_pos->m_righth);
				else
					down=max(m_pos->m_lefth,m_pos->m_righth);
				PositionHardBreak(down);
			}
		}

		robj->MoveZoneX(m_pos->m_leftw);
		robj->MoveZoneY(m_pos->m_cury);
		by=max(m_pos->m_cury+oh,robj->GetZoneY()+obj->m_maxchildy);
		if(by>m_maxy)
			m_maxy=by;

		if(m_page->m_mode==MODE_MINMAX)
		{
			m_pos->m_leftw+=obj->m_maxw;
			if(obj->m_box)
				m_pos->m_leftw+=(obj->m_box->GetBoxLeftWidth()+obj->m_box->GetBoxRightWidth());
			if((m_pos->m_leftw+m_pos->m_rightw)>m_maxw)
				m_maxw=m_pos->m_leftw+m_pos->m_rightw;
		}
		else
			m_pos->m_leftw+=ow;

		if(m_pos->m_leftw)
		{
			m_pos->m_lefth=max(m_pos->m_lefth,oh);
			if(!m_pos->m_lefth)
				m_pos->m_leftw=0;
		}
		return;
	}
	else if(obj->GetAlign()==ALIGN_RIGHT || obj->m_float==FLOAT_RIGHT)
	{
		if(m_pos->m_ponum)
			PositionBreak();

		if(m_page->m_mode==MODE_POSITION)
		{
			/* is there room for this?, if not, move down until there is */
			while(m_pos->m_leftw || m_pos->m_rightw)
			{
				if(m_pos->m_leftw+ow+m_pos->m_rightw<=pw)
					break;

				/* no room, move down */
				if(m_pos->m_lefth && m_pos->m_righth)
					down=min(m_pos->m_lefth,m_pos->m_righth);
				else
					down=max(m_pos->m_lefth,m_pos->m_righth);
				PositionHardBreak(down);
			}
		}

		robj->MoveZoneX(pw-(m_pos->m_rightw+ow));
		robj->MoveZoneY(m_pos->m_cury);
		by=max(m_pos->m_cury+oh,robj->GetZoneY()+obj->m_maxchildy);
		if(by>m_maxy)
			m_maxy=by;

		if(m_page->m_mode==MODE_MINMAX)
		{
			m_pos->m_rightw+=obj->m_maxw;
			if(obj->m_box)
				m_pos->m_rightw+=(obj->m_box->GetBoxLeftWidth()+obj->m_box->GetBoxRightWidth());
			if((m_pos->m_leftw+m_pos->m_rightw)>m_maxw)
				m_maxw=m_pos->m_leftw+m_pos->m_rightw;
		}
		else
			m_pos->m_rightw+=ow;

		if(m_pos->m_rightw)
		{
			m_pos->m_righth=max(m_pos->m_righth,oh);
			if(!m_pos->m_righth)
				m_pos->m_rightw=0;
			else if((m_pos->m_leftw+m_pos->m_rightw)>m_maxw)
				m_maxw=m_pos->m_leftw+m_pos->m_rightw;
		}
		return;
	}
	else
	{
		switch(obj->GetAlign())
		{
		case ALIGN_CENTER:
		case ALIGN_ABSCENTER:
			isbox=false;
			if(m_pos->m_ponum)
				PositionBreak();

			/* is there room for this?, if not, move down until there is */
			while(m_pos->m_leftw || m_pos->m_rightw)
			{
				if(m_pos->m_leftw+ow+m_pos->m_rightw<=pw)
					break;

				/* no room, move down */
				if(m_pos->m_lefth && m_pos->m_righth)
					down=min(m_pos->m_lefth,m_pos->m_righth);
				else
					down=max(m_pos->m_lefth,m_pos->m_righth);
				PositionHardBreak(down);
			}

			robj->MoveZoneX(m_pos->m_leftw+(((GetChildZoneW()-(ow-m_pos->m_leftw-m_pos->m_rightw)))>>1));
			robj->MoveZoneY(m_pos->m_cury+(asc-oh));
			m_pos->m_cury+=(asc+desc);
			if(m_pos->m_cury>m_maxy)
				m_maxy=m_pos->m_cury;
			if((GetZoneX()+ow)>m_maxw)
				m_maxw=GetZoneX()+ow;
			return;
		break;
		}
dotext:;
		if(isbox && m_pos->m_ponum)
			PositionBreak();
		else if(m_pos->m_ponum)
		{
			/* if the text-align mode changes then flush the previous ones */
			if(m_potextalign!=obj->m_textalign)
				PositionBreak(false);
		}
		if(m_page->m_mode==MODE_POSITION)
		{
			/* this is a HACK that should not be needed once I get around to writing code */
			/* for collapsing margins */

			/* HACK: if object has margins that allow it to clear then */
#if 0
			if(!m_pos->m_curx && (m_pos->m_leftw || m_pos->m_rightw) && isbox==true && obj->m_box)
			{
				if((int)obj->m_box->GetBoxLeftMargin()>=m_pos->m_leftw && (ow-((int)obj->m_box->GetBoxRightMargin()+(int)obj->m_box->GetBoxLeftMargin()))<=pw)
				{
					robj->MoveZoneX(0);
					robj->MoveZoneY(m_pos->m_cury);
					m_pos->m_cury+=(asc+desc);
					if(m_pos->m_cury>m_maxy)
						m_maxy=m_pos->m_cury;
					return;
				}
				if((int)obj->m_box->GetBoxLeftMargin()==m_pos->m_leftw && (int)obj->m_box->GetBoxRightMargin()==m_pos->m_rightw)
				{
					ow-=obj->m_box->GetBoxLeftMargin()+obj->m_box->GetBoxRightMargin();
					obj->m_box->SetBoxLeftMargin(0);
					obj->m_box->SetBoxRightMargin(0);
					obj->SetOutsideW(ow);
					obj->CalcChildZone();
					//move all children inside me too
				}
			}
#endif

			/* calc width of left and right border */
			if((m_pos->m_curx+ow)>(pw-(m_pos->m_leftw+m_pos->m_rightw)))
				PositionBreak(true);

			/* move down to clear left/right */
			while(ow>(pw-(m_pos->m_leftw+m_pos->m_rightw)))
			{
				if(!m_pos->m_leftw && !m_pos->m_rightw)
					break;
				if(m_pos->m_leftw && m_pos->m_rightw)
					m_pos->m_lineasc=min(m_pos->m_lefth,m_pos->m_righth);
				else
					m_pos->m_lineasc=max(m_pos->m_lefth,m_pos->m_righth);
				PositionBreak(true);
			}
		}

		/* save align mode for objects */
		m_potextalign=obj->m_textalign;

		if(obj->m_relpos==false)
		{
			robj->MoveZoneX(0);
			robj->MoveZoneY(0);
		}
		po.obj=obj;
		po.robj=robj;
		po.above=above;
		po.baseline=desc;
		po.below=below;
	
		//debugging
		po.width=ow;
		po.height=oh;

		m_pos->m_polist.SetEntry(m_pos->m_ponum++,po);
		asc+=above;
		desc+=below;

		if(asc>m_pos->m_lineasc)
			m_pos->m_lineasc=asc;

		/* actual descender */
		if(desc>m_pos->m_linedesc)
			m_pos->m_linedesc=desc;
		m_pos->m_curx+=ow;
		if((m_pos->m_curx+m_pos->m_leftw+m_pos->m_rightw)>m_maxw)
			m_maxw=m_pos->m_curx+m_pos->m_leftw+m_pos->m_rightw;
		if(ow>m_minw)
			m_minw=ow;

		if(isbox)
			PositionBreak();
	}
}

kGUIHTMLObj *kGUIHTMLObj::FindStyleChild(unsigned int type)
{
	unsigned int r;
	kGUIHTMLObj *obj;

	for(r=0;r<m_numstylechildren;++r)
	{
		obj=m_stylechildren.GetEntry(r);
		if(obj->GetID()==type)
			return(obj);
	}
	return(0);
}

kGUIHTMLTableInfo::kGUIHTMLTableInfo()
{
	/* doesn't need page since it never parses tokens */
	m_numrows=0;
	m_numcols=0;
	m_box=0;
	m_cellborder=new kGUIHTMLBox(0);
}

kGUIHTMLTableInfo::~kGUIHTMLTableInfo()
{
	delete m_cellborder;
}

void kGUIHTMLTableInfo::Get(kGUIHTMLObj *obj,Array<kGUIHTMLObj *>*objectarray,unsigned int *num,unsigned int display)
{
	unsigned int r;
	kGUIHTMLObj *child;

	for(r=0;r<obj->m_numstylechildren;++r)
	{
		child=obj->m_stylechildren.GetEntry(r);
		if(child->m_display==display)
		{
			if(objectarray)
				objectarray->SetEntry(*(num),static_cast<kGUIHTMLObj *>(child));
			*(num)=*(num)+1;
		}
		else if(child->m_display!=DISPLAY_TABLE && child->m_display!=DISPLAY_NONE)
		{
			/* don't continue looking if we encounter another table */
			/* or a hidden object */
			Get(child,objectarray,num,display);
		}
	}
}

void kGUIHTMLTableInfo::CalcMinMax(kGUIHTMLObj *table,int em)
{
	unsigned int r;
	unsigned int i;
	unsigned int j;
	unsigned int c;
	int rc;
	int w;
	kGUIHTMLObj *row;
	kGUIHTMLObj *cell;
	kGUIHTMLObj *crow;
	kGUIHTMLObj *ccell;
	Array<unsigned int>colsperrow;
	kGUIHTMLAttrib *att;

	/* calc number of rows first*/
	m_numrows=0;
	for(i=0;i<table->m_numstylechildren;++i)
	{
		row=table->m_stylechildren.GetEntry(i);
		if(row->m_display==DISPLAY_TABLE_ROW)
			++m_numrows;
		else
			Get(row,0,&m_numrows,DISPLAY_TABLE_ROW);
	}

	/* allocate pointer table for each row and number of columns for each row */
	m_rowptrs.Alloc(m_numrows);
	colsperrow.Alloc(m_numrows);
	for(i=0;i<m_numrows;++i)
		colsperrow.SetEntry(i,0);

	/* build row pointer table */
	r=0;
	for(i=0;i<table->m_numstylechildren;++i)
	{
		row=table->m_stylechildren.GetEntry(i);
		if(row->m_display==DISPLAY_TABLE_ROW)
			m_rowptrs.SetEntry(r++,static_cast<kGUIHTMLObj *>(row));
		else
			Get(row,&m_rowptrs,&r,DISPLAY_TABLE_ROW);
	}

	/* build row pointer table and then count number of cols per row */
	m_numcols=0;
	for(r=0;r<m_numrows;++r)
	{
		crow=m_rowptrs.GetEntry(r);
		crow->m_ti=this;	/* these are NOT orphan rows */
		for(j=0;j<crow->m_numstylechildren;++j)
		{
			cell=crow->m_stylechildren.GetEntry(j);
			if(cell->m_display==DISPLAY_TABLE_CELL)
				ccell=static_cast<kGUIHTMLObj *>(cell);
			else
			{
				/* todo, change this to look at display for DISPLAY_TABLE_CELL */
				cell=cell->FindStyleChild(HTMLTAG_TD);
				if(cell)
					ccell=static_cast<kGUIHTMLObj *>(cell);
				else
					ccell=0;
			}
			if(ccell)
			{
				/* assign rowspan and colspan variables */
				att=ccell->FindAttrib(HTMLATT_COLSPAN);
				if(att)
				{
					ccell->m_colspan=att->GetValue()->GetInt();
					if(ccell->m_colspan<1)
						ccell->m_colspan=1;/* todo: this should be trapped earlier on */
				}
				else
					ccell->m_colspan=1;

				att=ccell->FindAttrib(HTMLATT_ROWSPAN);
				if(att)
				{
					ccell->m_rowspan=att->GetValue()->GetInt();
					if(ccell->m_rowspan<1)
						ccell->m_rowspan=1;	/* todo: this should be trapped earlier on */
				}
				else
					ccell->m_rowspan=1;

				/* trim rowspan if off end of the table */
				while((r+ccell->m_rowspan)>m_numrows)
					--ccell->m_rowspan;

				for(unsigned int y=r;y<(r+ccell->m_rowspan);++y)
					colsperrow.SetEntry(y,colsperrow.GetEntry(y)+ccell->m_colspan);
			}
		}
		if(colsperrow.GetEntry(r)>m_numcols)
			m_numcols=colsperrow.GetEntry(r);
	}
	m_cellptrs.Alloc(m_numcols*m_numrows);
	for(i=0;i<m_numcols*m_numrows;++i)
		m_cellptrs.SetEntry(i,0);

	/* build cell pointers table */
	rc=0;
	for(r=0;r<m_numrows;++r)
	{
		crow=m_rowptrs.GetEntry(r);
		c=0;
		for(j=0;j<crow->m_numstylechildren;++j)
		{
			cell=crow->m_stylechildren.GetEntry(j);
			if(cell->m_display==DISPLAY_TABLE_CELL)
				ccell=static_cast<kGUIHTMLObj *>(cell);
			else
			{
				//todo change to look for display type
				cell=cell->FindStyleChild(HTMLTAG_TD);
				if(cell)
					ccell=static_cast<kGUIHTMLObj *>(cell);
				else
					ccell=0;
			}
			if(ccell)
			{
				/* skip used cells from rowspan above */
				while(m_cellptrs.GetEntry(rc+c))
					++c;
				if(ccell->m_colspan==1 && ccell->m_rowspan==1)
					m_cellptrs.SetEntry(rc+c,ccell);
				else
				{
					int x2,y2,yy;

					yy=rc+c;
					for(y2=0;y2<ccell->m_rowspan;++y2)
					{
                        for(x2=0;x2<ccell->m_colspan;++x2)
						{
							if(!x2 && !y2)
								m_cellptrs.SetEntry(rc+c,ccell);
							else
								m_cellptrs.SetEntry(yy+x2,table);
						}
						yy+=m_numcols;
					}
				}
				c+=ccell->m_colspan;
			}
		}
		rc+=m_numcols;
	}

	/***************************************************************************/

	/* alloc space for calculating column widths and row heights */
	m_colx.Alloc(m_numcols);
	m_colmin.Alloc(m_numcols);
	m_colmax.Alloc(m_numcols);
	m_colwidth.Alloc(m_numcols);
	m_colwidthfixed.Alloc(m_numcols);
	m_colwidthpercent.Alloc(m_numcols);
	m_coly.Alloc(m_numrows);
	m_rowheight.Alloc(m_numrows);

	for(i=0;i<m_numcols;++i)
	{
		m_colwidth.SetEntry(i,0);
		m_colmin.SetEntry(i,0);
		m_colmax.SetEntry(i,0);

		m_colwidthfixed.SetEntry(i,false);
		m_colwidthpercent.SetEntry(i,0);
	}

	/* calc min/max for cells with colspan=1 */

	rc=0;
	for(r=0;r<m_numrows;++r)
	{
		for(c=0;c<m_numcols;++c)
		{
			ccell=m_cellptrs.GetEntry(rc++);
			if(ccell && ccell!=table)
			{
				ccell->m_ti=this;			/* this is NOT an orpan cell */
				if(ccell->m_colspan==1)
				{
					/* does this have a fixed width? */
					if(ccell->m_width.GetIsFixed())
					{
						m_colwidthfixed.SetEntry(c,true);

						/* set to max of fixed width or minimum width, whatever is greater! */
						w=max(ccell->m_width.CalcUnitValue(0,em)+(m_cellpadding<<1),ccell->m_minw+(m_cellpadding<<1));

						if(w>m_colmin.GetEntry(c))
						{
							m_colmin.SetEntry(c,w);
							m_colwidth.SetEntry(c,w);
							m_colmax.SetEntry(c,w);
						}
					}
					else
					{
						if(ccell->m_width.GetUnitType()==UNITS_PERCENT)
						{
							int curp=m_colwidthpercent.GetEntry(c);
							int newp=ccell->m_width.GetUnitValue();

							if(newp>curp)
								m_colwidthpercent.SetEntry(c,newp);
						}

						w=ccell->m_minw+(m_cellpadding<<1);
						if(w>m_colmin.GetEntry(c))
						{
							m_colmin.SetEntry(c,w);
							if(m_colwidthfixed.GetEntry(c)==false)
								m_colwidth.SetEntry(c,w);
						}
						w=ccell->m_maxw+(m_cellpadding<<1);
						if(w>m_colmax.GetEntry(c))
							m_colmax.SetEntry(c,w);
					}
				}
			}
		}
	}

	/* now process cell with colspans greater than one */

	rc=0;
	for(r=0;r<m_numrows;++r)
	{
		for(c=0;c<m_numcols;++c)
		{
			ccell=m_cellptrs.GetEntry(rc++);
			if(ccell && ccell!=table)
			{
				if(ccell->m_colspan>1)
				{
					unsigned int endcol;
					int groupmax;

					endcol=c+ccell->m_colspan;
#if 1
//					kGUI::Trace("Getting Cell Size[%d,%d,span=%d] width=%d\n",r,c,ccell->m_colspan,ccell->m_minw);

					if(ccell->m_width.GetIsFixed())
						w=max(ccell->m_minw,ccell->m_width.CalcUnitValue(0,em)+(m_cellpadding<<1));
					else
						w=ccell->m_minw;

					ExpandCols(c,ccell->m_colspan,w,true,false);
					for(j=c;j<endcol;++j)
						m_colmin.SetEntry(j,m_colwidth.GetEntry(j));

					/* expand colmax also across range */
					groupmax=0;
					for(j=c;j<endcol;++j)
						groupmax+=m_colmax.GetEntry(j);

					w=ccell->m_maxw+(m_cellpadding<<1);
					j=c;
					while(w>groupmax)
					{
						m_colmax.SetEntry(j,m_colmax.GetEntry(j)+1);
						if(++j==endcol)
							j=c;
						++groupmax;
					}

#else
					int groupmin,wtemp;
					
					/* calc current minimum/maximum for this group of columns */
					groupmin=0;
					groupmax=0;
					for(j=c;j<endcol;++j)
					{
						groupmin+=m_colmin.GetEntry(j);
						groupmax+=m_colmax.GetEntry(j);
					}

					/* expand minimum across columns */
					w=ccell->m_minw+(m_cellpadding<<1);
					j=c;
					while(w>groupmin)
					{
						wtemp=m_colmin.GetEntry(j)+1;
						m_colmin.SetEntry(j,wtemp);
						m_colwidth.SetEntry(j,wtemp);
						if(++j==endcol)
							j=c;
						++groupmin;
					}

					/* expand maximum across columns */
					w=ccell->m_maxw+(m_cellpadding<<1);
					j=c;
					while(w>groupmax)
					{
						m_colmax.SetEntry(j,m_colmax.GetEntry(j)+1);
						if(++j==endcol)
							j=c;
						++groupmax;
					}
#endif
				}
			}
		}
	}

	/* calc min and max for table */
	table->m_minw=0;
	table->m_maxw=0;
	for(c=0;c<m_numcols;++c)
	{
		table->m_minw+=m_cellspacing;
		table->m_maxw+=m_cellspacing;
		table->m_minw+=m_colmin.GetEntry(c);
		table->m_maxw+=m_colmax.GetEntry(c);
	}
	if(m_numcols)
	{
		table->m_minw+=m_cellspacing;
		table->m_maxw+=m_cellspacing;
	}

	/* only expand table to fit cells, don't make smaller */
	if(table->m_width.GetIsFixed())
	{
		int minsize=table->m_width.CalcUnitValue(0,em);

		table->m_minw=max(table->m_minw,minsize);
		table->m_maxw=max(table->m_maxw,minsize);
	}
}

int kGUIHTMLTableInfo::GetSpanWidth(int startcol,int endcol)
{
	int lx,rx;

	lx=m_colx.GetEntry(startcol);
	rx=m_colx.GetEntry(endcol-1)+m_colwidth.GetEntry(endcol-1);
	return(rx-lx);
}

int kGUIHTMLTableInfo::GetSpanHeight(int startrow,int endrow)
{
	int ty,by;

	ty=m_coly.GetEntry(startrow);
	by=m_coly.GetEntry(endrow-1)+m_rowheight.GetEntry(endrow-1);
	return(by-ty);
}

/* expand some columns */
void kGUIHTMLTableInfo::ExpandCols(int startcol,int numcols,int width,bool expand,bool applypercents)
{
	int i;
	int curw;
	int maxw;
	int grow;
	int per;
	int w,wx;

	curw=0;
	maxw=0;
	for(i=0;i<numcols;++i)
	{
		curw+=m_colwidth.GetEntry(startcol+i);
		curw+=m_cellspacing;
		if(m_colwidthfixed.GetEntry(startcol+i)==true)
		{
			maxw+=m_colwidth.GetEntry(startcol+i);
			maxw+=m_cellspacing;
		}
		else
		{
			maxw+=m_colmax.GetEntry(startcol+i);
			maxw+=m_cellspacing;
		}
	}
	/* current width is already past this! */
	if(curw>=width)
		return;
	if(maxw<=width)
	{
		/* expand all non-fixed columns to max */
		for(i=0;i<numcols;++i)
		{
			if(m_colwidthfixed.GetEntry(startcol+i)==false)
				m_colwidth.SetEntry(startcol+i,m_colmax.GetEntry(startcol+i));
		}
		curw=maxw;
		if(curw==width)
			return;
	}

	if(curw<width)
	{
		/* expand any 100% columns to max first */
		for(i=0;((i<numcols) && (curw<width));++i)
		{
			if(m_colwidthpercent.GetEntry(startcol+i)==100)
			{
				w=m_colwidth.GetEntry(startcol+i);

				/* expand column up to this amount */
				wx=m_colmax.GetEntry(startcol+i);
				grow=wx-w;
				if(grow>(width-curw))
					grow=width-curw;
				if(grow>0)
				{
					w+=grow;
					m_colwidth.SetEntry(startcol+i,w);
					curw+=grow;
				}
			}
		}
	}

	/* percent columns can grow beyond the max size of the contents */
	if(curw<width)
	{
		int colp;
		int totalp;
		int grew;

		do
		{
			totalp=0;
			/* count total percent used so far */
			for(i=0;(i<numcols);++i)
			{
				if(m_colwidthpercent.GetEntry(startcol+i))
				{
					if(m_colwidthpercent.GetEntry(startcol+i)!=100)
						totalp+=m_colwidthpercent.GetEntry(startcol+i);
					else
						totalp+=(int)(((double)m_colwidth.GetEntry(startcol+i)/curw)*100.0f);
				}
				else
					totalp+=(int)(((double)m_colwidth.GetEntry(startcol+i)/curw)*100.0f);
			}
			if(!totalp)
				break;

			/* expand percent columns first */
			grew=0;
			for(i=0;((i<numcols) && (curw<width));++i)
			{
				if(m_colwidthfixed.GetEntry(startcol+i)==false)
				{
					colp=m_colwidthpercent.GetEntry(startcol+i);
					if(colp)
					{
						/* expand column up to this amount */
						w=m_colwidth.GetEntry(startcol+i);
						wx=(int)((width*colp)/totalp);
						grow=wx-w;
						if(grow>(width-curw))
							grow=width-curw;
						if(grow>0)
						{
							grew=1;
							w+=grow;
							m_colwidth.SetEntry(startcol+i,w);
							curw+=grow;
						}
					}
				}
			}
		}while(grew);
	}

	/* pro-rate expansion across columns that still can grow */
	if(curw<maxw)
	{
		int numgrow;
		int colp;

		while(curw<width)
		{
			/* count number of columns that can grow and get smallest grow amount */
			numgrow=0;
			grow=0;
			for(i=0;i<numcols;++i)
			{
				w=m_colwidth.GetEntry(startcol+i);
				if(m_colwidthfixed.GetEntry(startcol+i)==false)
				{
					colp=m_colwidthpercent.GetEntry(startcol+i);
					wx=m_colmax.GetEntry(startcol+i);

					if(w<wx)
					{
						if(!numgrow)
							grow=wx-w;
						else if((wx-w)<grow)
							grow=wx-w;
						++numgrow;
					}
				}
			}
			if(!numgrow)
				break;

			if(grow>((width-curw)/numgrow))
				grow=(width-curw)/numgrow;
			if(!grow)
				grow=1;

			/* grow all columns by smallest amount */
			for(i=0;i<numcols;++i)
			{
				w=m_colwidth.GetEntry(startcol+i);
				if(m_colwidthfixed.GetEntry(startcol+i)==false)
				{
					wx=m_colmax.GetEntry(startcol+i);
					if(w<wx)
					{
						m_colwidth.SetEntry(startcol+i,w+grow);
						curw+=grow;
					}
				}
			}
		}
	}
	else
	{
		int numnotfixed;

		/* only expand columns whos width is less then their desired percent of the table width */
		if(applypercents)
		{
			int colp;
			int desiredw,currentw;
			int pixavail=width-curw;
			int addw;

			for(i=0;i<numcols;++i)
			{
				colp=m_colwidthpercent.GetEntry(startcol+i);
				if(colp)
				{
					/* if column width=100% then that means use maxw not 100% of the table size */
					currentw=m_colwidth.GetEntry(startcol+i);
					if(colp==100)
						desiredw=m_colmax.GetEntry(startcol+i);
					else
						desiredw=(int)((double)(colp*width)/100.0f);

					if(desiredw>currentw)
					{
						/* expand this column */
						addw=desiredw-currentw;
						if(addw>pixavail)
							addw=pixavail;
					
						currentw+=addw;
						m_colwidth.SetEntry(startcol+i,currentw);
						pixavail-=addw;
						if(!pixavail)
							return;					/* no more space to grow */
						curw+=addw;				/* update total width of columns */
					}
				}
			}
		}

		if(expand==false)
			return;

		numnotfixed=0;

		/* count number of not fixed columns */
		for(i=0;i<numcols;++i)
		{
			if(m_colwidthfixed.GetEntry(startcol+i)==false /* && m_colwidthpercent.GetEntry(startcol+i)==0 */)
				++numnotfixed;
		}
		if(numnotfixed)
		{
			/* all non fixed columns are at max, so expand them all */
			per=(int)((double)(width-curw)/numnotfixed);
			for(i=0;i<numcols;++i)
			{
				if(m_colwidthfixed.GetEntry(startcol+i)==false /* && m_colwidthpercent.GetEntry(startcol+i)==0 */)
				{
					m_colwidth.SetEntry(startcol+i,m_colwidth.GetEntry(startcol+i)+per);
					curw+=per;
				}
			}
		}
		/* if still need more then expand all columns */
#if 0
		if(curw<width)
		{
			/* expand them all */
			per=(int)((double)(width-curw)/numcols);
			if(per)
			{
				for(i=0;i<numcols;++i)
				{
					m_colwidth.SetEntry(startcol+i,m_colwidth.GetEntry(startcol+i)+per);
					curw+=per;
				}
			}
		}
#endif
	}

	/* check for rounding error */
}

void kGUIHTMLTableInfo::ExpandTable(kGUIHTMLObj *table,int em)
{
	unsigned int i;
	unsigned int x;
	int j,w;
	kGUIHTMLObj *ccell;
	kGUIHTMLObj *crow;
	int tw;
	bool expand;

	if(!m_numcols)
		return;			/* table can't expand! */

	/* calc number of pixels that the table can expand by */
	if(table->m_width.GetUnitType()!=UNITS_UNDEFINED && table->m_width.GetUnitType()!=UNITS_AUTO)
	{
		tw=table->m_width.CalcUnitValue(table->GetParentObj()->GetChildZoneW(),em);
		if(m_box)
			tw+=(table->m_box->GetBoxLeftWidth()+table->m_box->GetBoxRightWidth());
		expand=true;
	}
	else
	{
		tw=table->GetChildZoneW();
		expand=false;
	}
	tw-=m_cellpadding<<1;
	tw-=(m_numcols+1)*m_cellspacing;

	ExpandCols(0,m_numcols,tw,expand,true);

	/* recalc column positions */

	w=0;
	for(i=0;i<m_numcols;++i)
	{
		w+=m_cellspacing;
		m_colx.SetEntry(i,w);
		w+=m_colwidth.GetEntry(i);
	}
	if(m_numcols)
		w+=m_cellspacing;

	table->SetOutsideW(w);
	table->CalcChildZone();

	if(table->m_page->GetTrace())
	{
		kGUI::Trace("TableExpand NumCols=%d neww=%d, numrows=%d, newh=%d\n",m_numcols,table->GetZoneW(),m_numrows,table->GetZoneH());
		for(i=0;i<m_numcols;++i)
		{
			kGUI::Trace("  ColWidth[%d]=%d (min=%d,max=%d)",i,m_colwidth.GetEntry(i),m_colmin.GetEntry(i),m_colmax.GetEntry(i));
			if(m_colwidthfixed.GetEntry(i)==true)
				kGUI::Trace(" Fixed width=%d",m_colwidth.GetEntry(i));
			if(m_colwidthpercent.GetEntry(i)!=0)
				kGUI::Trace(" Percent width=%d",m_colwidthpercent.GetEntry(i));

			kGUI::Trace("\n");
		}
		for(i=0;i<m_numrows;++i)
			kGUI::Trace("  RowHeight[%d]=%d\n",i,m_rowheight.GetEntry(i));
	}

	/* expand all cells to new column widths */

	j=0;
	for(i=0;i<m_numrows;++i)
	{
		crow=m_rowptrs.GetEntry(i);
		crow->SetOutsideW(w);
		crow->CalcChildZone();
		for(x=0;x<m_numcols;++x)
		{
			ccell=m_cellptrs.GetEntry(j++);
			if(ccell && ccell!=table)
			{
				if(ccell->m_colspan==1)
					ccell->SetOutsideW(m_colwidth.GetEntry(x));
				else
					ccell->SetOutsideW(GetSpanWidth(x,x+ccell->m_colspan));
				ccell->CalcChildZone();
			}
		}
	}
}

void kGUIHTMLTableInfo::PositionCells(kGUIHTMLObj *table)
{
	unsigned int i;
	unsigned int x;
	int rowx,tablex,cpad,h,h2,j,y,tallest,oldh,yoff;
	kGUIHTMLObj *crow;
	kGUIHTMLObj *ccell;

#if 0
	kGUIString debug;
	for(i=0;i<m_numcols;++i)
	{
		debug.ASprintf("c%d{%d,%d},",i,m_colmin.GetEntry(i),m_colmax.GetEntry(i));
	}
	debug.Append("\n\n");
	kGUI::Trace(debug.GetString());
#endif

	/* calculate positions of columns */
#if 0
	kGUI::Trace("Table Start\n");
#endif
	rowx=0;
	for(i=0;i<m_numcols;++i)
	{
		rowx+=m_cellspacing;
#if 0
		kGUI::Trace("Col %d, lx=%d,rx=%d\n",i,rowx,rowx+m_colwidth.GetEntry(i));
#endif
		m_colx.SetEntry(i,rowx);
		rowx+=m_colwidth.GetEntry(i);
	}
	if(m_numcols)
		rowx+=m_cellspacing;
	tablex=rowx;
	if(table->m_box)
		tablex+=(table->m_box->GetBoxLeftWidth()+table->m_box->GetBoxRightWidth());
#if 0
	kGUI::Trace("Total Table Width=%d\n",rowx);
#endif

	/* calculate row heights */

	for(i=0;i<m_numrows;++i)
		m_rowheight.SetEntry(i,0);

	/* calc row heights for colspan==1 */
	j=0;
	cpad=(m_cellpadding<<1);
	for(i=0;i<m_numrows;++i)
	{
		for(x=0;x<m_numcols;++x)
		{
			ccell=m_cellptrs.GetEntry(j++);
			if(ccell && ccell!=table)
			{
				if(ccell->m_rowspan==1)
				{
					h=ccell->GetZoneH()+cpad;

					/* does this cell have a fixed height? */
					if(ccell->m_height.GetUnitType()==UNITS_PIXELS)
					{
						h2=ccell->m_height.GetUnitValue()+cpad;
						if(h2>h)
							h=h2;
					}

					if(h>m_rowheight.GetEntry(i))
						m_rowheight.SetEntry(i,h);
				}
			}
		}
	}

	/* expand height for rowspans>1 */
	j=0;

	for(i=0;i<m_numrows;++i)
	{
		for(x=0;x<m_numcols;++x)
		{
			ccell=m_cellptrs.GetEntry(j++);
			if(ccell && ccell!=table)
			{
				if(ccell->m_rowspan>1)
				{
					int r2;
					int groupheight,endrow;

					h=ccell->GetZoneH()+cpad;
					endrow=i+ccell->m_rowspan;
					groupheight=0;
					for(r2=i;r2<endrow;++r2)
						groupheight+=m_rowheight.GetEntry(r2);
					
					/* expand rows till big enough */
					r2=i;
					while(h>groupheight)
					{
						/* todo, don't expand rows with a fixed height */
						m_rowheight.SetEntry(r2,m_rowheight.GetEntry(r2)+1);
						if(++r2==endrow)
							r2=i;
						++groupheight;
					}
				}
			}
		}
	}

	/* calculate y positions */
	y=0;
	for(i=0;i<m_numrows;++i)
	{
		y+=m_cellspacing;
		m_coly.SetEntry(i,y);
		y+=m_rowheight.GetEntry(i);
//		kGUI::Trace("Row %d, y=%d\n",i,y);
	}
	if(m_numrows)
		y+=m_cellspacing;

	if(table->m_height.GetUnitType()==UNITS_PIXELS)
		table->m_pos->m_cury=table->m_height.GetUnitValue();
	else
		table->m_pos->m_cury=y;

	table->m_maxy=y;
	table->SetOutsideW(tablex);
	table->SetOutsideH(y);

	/* set positions of all rows and cells */
	j=0;
	for(i=0;i<m_numrows;++i)
	{
		tallest=0;
		for(x=0;x<m_numcols;++x)
		{
			ccell=m_cellptrs.GetEntry(j++);
			if(ccell && ccell!=table)
			{
				ccell->MoveZoneY(0);
				ccell->MoveZoneX(m_colx.GetEntry(x));

				if(ccell->m_colspan==1)
					ccell->SetOutsideW(m_colwidth.GetEntry(x));
				else
				{
					ccell->SetOutsideW(GetSpanWidth(x,x+ccell->m_colspan));
//					kGUI::Trace("Setting Cell Size[%d,%d,span=%d] width=%d\n",x,i,ccell->m_colspan,GetSpanWidth(x,x+ccell->m_colspan));
				}
				if(ccell->m_rowspan==1)
					h=m_rowheight.GetEntry(i);
				else
					h=GetSpanHeight(i,i+ccell->m_rowspan);

				if(ccell->m_childusesheight)
				{
					/* if we have any children that reference our height then we need to re-layout */
					/* the children again with the height defined */
					ccell->SetOutsideH(h);
					ccell->CalcChildZone();
					ccell->Position(false);
				}
				oldh=ccell->GetOutsideH();
				ccell->SetOutsideH(h);
				switch(ccell->GetVAlign())
				{
				case VALIGN_MIDDLE:
					yoff=(h-oldh)/2;
				break;
				case VALIGN_BOTTOM:
					yoff=(h-oldh);
				break;
				default:
					yoff=0;
				break;
				}
				ccell->CalcChildZone(yoff);

				if(h>tallest)
					tallest=h;
			}
		}
		crow=m_rowptrs.GetEntry(i);
		crow->MoveZoneX(0);
		crow->MoveZoneY(m_coly.GetEntry(i));
		crow->SetOutsideH(tallest+m_cellspacing);
		crow->SetOutsideW(rowx);
		crow->CalcChildZone();
	}
}

bool kGUIHTMLObj::UpdateHover(void)
{
	unsigned int i;
	bool over;
	kGUICorners c;

	/* mousecorners is the box that defines the previous and the current mouse position */
	/* it is needed so objects that WERE covered are now cleared and objects that are */
	/* NOW covered are set */

	GetCorners(&c);
	over=kGUI::MouseOver(&c);
	if(m_display==DISPLAY_INLINE && over==true)
	{
		unsigned int nc;

		/* since the mouse is over me but my children are spread out I need to make sure */
		/* that is is actually over one of my children */
		over=false;
		nc=GetNumChildren();
		for(i=0;(i<nc) && (over==false);++i)
		{
			GetChild(i)->GetCorners(&c);
			if(kGUI::MouseOver(&c)==true)
				over=true;
		}
	}

	/* also update my children too */
	for(i=0;i<m_numstylechildren;++i)
		m_stylechildren.GetEntry(i)->UpdateHover();

	if(over)
	{
		int index=m_page->m_hovertoggle;
		m_page->m_hoverlist[index].SetEntry(m_page->m_hoverlistsize[index],this);
		m_page->m_hoverlistsize[index]=m_page->m_hoverlistsize[index]+1;
	}
	return(over);
}

#if 1


void kGUIHTMLObj::SavePosition(void)
{
	unsigned int i;

	m_savew=GetZoneW();
	m_saveh=GetZoneH();
	m_saveminw=m_minw;
	m_savemaxw=m_maxw;

	for(i=0;i<m_numstylechildren;++i)
		m_stylechildren.GetEntry(i)->SavePosition();

}

void kGUIHTMLObj::ComparePosition(void)
{
	unsigned int i;

	for(i=0;i<m_numstylechildren;++i)
		m_stylechildren.GetEntry(i)->ComparePosition();

	if(m_savew!=GetZoneW() || m_saveh!=GetZoneH())
	{
		CID_DEF *cid;
		kGUIString ts;

		if(m_tag)
		{
			ts.Sprintf("<%s",m_tag->name);

			cid=m_cids.GetArrayPtr();
			for(i=0;i<m_numcids;++i)
			{
				if(cid->m_type==CID_CLASS)
					ts.ASprintf(" class=\"%S\"",m_page->IDToString(cid->m_id));
				else
					ts.ASprintf(" id=\"%S\"",m_page->IDToString(cid->m_id));
				++cid;

			}
			ts.Append(">");
		}
		else
			ts.Sprintf("<%04x>",m_id);

		kGUI::Trace("Size Changed %s ow=%d,oh=%d,neww=%d,newh=%d\n",ts.GetString(),m_savew,m_saveh,GetZoneW(),GetZoneH());
	}
}
#endif

/* position all renderable children */
void kGUIHTMLObj::Position(bool placeme)
{
	int popindex=m_page->GetPopIndex();
	unsigned int i;
	bool pass;
	int oldh=GetOutsideH();
	bool addme=false;	/* set to false to stop compiler warning */
	kGUIHTMLAttrib *att;
	kGUIHTMLObj *rp=GetParentObj();
	int w;

	/* this can happen if a table cell is not directly inside the table but enclosed in another tag */
	while(rp && !rp->m_pos)
		rp=rp->GetParentObj();

	/* get me a position class to use for positioning my children */
	m_pos=m_page->GetPos();
	m_pos->m_line=0;
	m_pos->m_ponum=0;

	pass=false;

	if(m_page->m_mode==MODE_POSITION)
	{
		PrePosition();
	}
	else //if(m_page->m_mode==MODE_MINMAX)
	{
		m_page->ApplyStyleRules(this,PSEUDO_NONE);

		/* is there any before or after content to add or update */
		if(m_page->m_beforecontent && m_display!=DISPLAY_NONE)
		{
			if(!m_beforeobj)
			{
				m_beforeobj=new kGUIHTMLObj(this,m_page,&kGUIHTMLPageObj::m_contenttag);
				m_beforeobj->m_insert=true;
				AddObject(m_beforeobj);
				InsertStyleChild(0,m_beforeobj);
			}

			/* has it changed? */
			if(strcmp(m_beforeobj->m_obj.m_contentgroup->GetString(),m_page->m_beforecontent->GetValue()->GetString()))
				m_beforeobj->SetString(m_page->m_beforecontent->GetValue(),false,false);
		}
		else if(m_beforeobj)
		{
			delete m_beforeobj;
			m_beforeobj=0;
		}

		if(m_page->m_aftercontent && m_display!=DISPLAY_NONE)
		{
			if(!m_afterobj)
			{
				m_afterobj=new kGUIHTMLObj(this,m_page,&kGUIHTMLPageObj::m_contenttag);
				m_afterobj->m_insert=true;

				AddObject(m_afterobj);
				AddStyleChild(m_afterobj);
			}
			/* has it changed? */
			if(strcmp(m_afterobj->m_obj.m_contentgroup->GetString(),m_page->m_aftercontent->GetValue()->GetString()))
				m_afterobj->SetString(m_page->m_aftercontent->GetValue(),false,false);
		}
		else if(m_afterobj)
		{
			delete m_afterobj;
			m_afterobj=0;
		}
	}

	if(m_display==DISPLAY_NONE)
		goto done;

	if(m_page->m_mode==MODE_POSITION)
	{
		unsigned int i;
		int oldw=GetZoneW();

		/* do I want to expand? */
		if(rp && rp!=m_page->m_fixedfgobject)
		{
			/* parent width */
			int pw=rp->GetChildZoneW();

			switch(m_display)
			{
			case DISPLAY_COMPACT:
			case DISPLAY_RUN_IN:
			case DISPLAY_BLOCK:
			case DISPLAY_LIST_ITEM:
				if(m_float==FLOAT_NONE && m_left.GetUnitType()==UNITS_UNDEFINED && m_right.GetUnitType()==UNITS_UNDEFINED && m_width.GetUnitType()==UNITS_UNDEFINED)
				{
					if(rp->m_pos->m_leftw || rp->m_pos->m_rightw)
					{
						pw+=rp->m_pos->m_leftw+rp->m_pos->m_rightw;
						if((int)GetBox()->GetBoxLeftMargin()<rp->m_pos->m_leftw)
							GetBox()->SetBoxLeftMargin(rp->m_pos->m_leftw);
						if((int)GetBox()->GetBoxRightMargin()<rp->m_pos->m_rightw)
							GetBox()->SetBoxRightMargin(rp->m_pos->m_rightw);
					}

					SetOutsideW(pw);
				}
				else
				{
					SetInsideW(m_maxw);
					w=GetOutsideW();
					if(w>pw)
						SetOutsideW(pw);
				}
				CheckFixed();
			break;
			case DISPLAY_TABLE:
			case DISPLAY_INLINE_TABLE:
				SetOutsideW(pw);
				CalcChildZone();
				if(m_ti)
					m_ti->ExpandTable(this,m_em);
			break;
			case DISPLAY_INLINE:
			case DISPLAY_MARKER:
			case DISPLAY_ANONYMOUS:
			case DISPLAY_INLINE_BLOCK:
				SetInsideW(m_maxw);
				w=GetOutsideW();
				if(w>pw)
					SetOutsideW(pw);

				CheckFixed();
			break;
			case DISPLAY_TABLE_CELL:
			case DISPLAY_TABLE_ROW:
				/* already expanded by table */
			break;
			default:
				assert(false,"Not supported");
			break;
			}
			CalcChildZone();
		}
		if(m_page->m_trace)
		{
			if(m_tag)
			{
				kGUIString ts;
				CID_DEF *cid;

				ts.Sprintf("<%s",m_tag->name);

				cid=m_cids.GetArrayPtr();
				for(i=0;i<m_numcids;++i)
				{
					if(cid->m_type==CID_CLASS)
						ts.ASprintf(" class=\"%S\"",m_page->IDToString(cid->m_id));
					else
						ts.ASprintf(" id=\"%S\"",m_page->IDToString(cid->m_id));
					++cid;

				}
				ts.Append(">");

				if(rp)
					kGUI::Trace("Expand [type=%d] %s [minw=%d,maxw=%d], oldw=%d,neww=%d (parentwidth=%d)\n",m_display,ts.GetString(),m_minw,m_maxw,oldw,GetZoneW(),rp->GetChildZoneW());
				else
					kGUI::Trace("Expand [type=%d] %s [minw=%d,maxw=%d], oldw=%d,neww=%d (parentwidth=%d)\n",m_display,ts.GetString(),m_minw,m_maxw,oldw,GetZoneW());
			}
		}
	}
	if(m_page->m_mode==MODE_MINMAX)
	{
		m_childusesheight=false;
	}

dopass2:;

	if(!m_pos->m_line && m_patttextindent)
	{
		kGUIUnits tiu;

		tiu.Set(m_page,m_patttextindent->GetValue());

		m_pos->m_curx=tiu.CalcUnitValue((m_page->m_mode==MODE_MINMAX || !m_renderparent)?0:m_renderparent->GetChildZoneW(),m_em);
	}
	else
		m_pos->m_curx=0;

	m_pos->m_cury=0;
	m_maxy=0;
	m_maxchildy=0;
	m_pos->m_leftw=0;
	m_pos->m_rightw=0;
	m_pos->m_lefth=0;
	m_pos->m_righth=0;
	m_pos->m_lineasc=0;
	m_pos->m_linedesc=0;
	m_pos->m_ponum=0;

	if(m_page->m_trace && m_tag)
		kGUI::Trace("PositionContainer (%s)\n",m_tag->name);

	switch(m_id)
	{
	case HTMLTAG_A:
		m_page->PushStyle(sizeof(m_page->m_link),&m_page->m_link);
		m_page->m_link=m_obj.m_linkobj;
	break;
	}

	//this should never happen?? since inline parents are skipped
	if(m_childusesheight==true && m_display==DISPLAY_INLINE)
		ChangeDisplay(DISPLAY_INLINE_BLOCK);

	if(m_position!=POSITION_STATIC)
	{
		/* should this only be done on the style pass??? */

		/* any non-static object becomes the renderparent for abs positioned children */
		/* update the lastest ABS render parent object to me */
		m_page->PushStyle(sizeof(m_page->m_absobject),&m_page->m_absobject);
		m_page->m_absobject=this;
	}

	/* check for fixed position/width */
	if(m_id!=HTMLTAG_TABLE && m_id!=HTMLTAG_TD && m_id!=HTMLTAG_TR && m_id!=HTMLTAG_TH)
		CheckFixed();

	/* clear left/right/both/none */
	if(m_clear!=CLEAR_NONE)
		rp->Clear(m_clear);

	switch(m_display)
	{
	case DISPLAY_LIST_ITEM:
	{
		kGUIHTMLLIPrefix *prefix;
		unsigned int style;

		if(m_page->m_mode==MODE_MINMAX)
		{
			/* has the prefix mode, or URL changed since before? */
			prefix=m_obj.m_liprefix;
			if(m_display==DISPLAY_INLINE)
				style=LISTSTYLE_NONE;
			else
				style=m_page->m_liststyle;

			if(style!=prefix->GetStyle() ||
				m_page->m_liststyleposition!=prefix->GetPosition() ||
				m_page->m_liststyleurl!=prefix->GetUrlID())
			{
				/* delete old imbed text objects */
				/* re-split the text using the new whitespace rule */
				prefix->Set(style,m_page->m_liststyleposition,
					m_page->m_liststyleurl,m_page->m_liststyleurlreferer,this,this);
			}
		}
	}
	break;
	}

typechanged:;
	switch(m_id)
	{
#if 0
	case HTMLTAG_P:
		/* empty P tag collapses into nothing */
		/* http://www.w3.org/TR/REC-html40/struct/text.html#h-9.3.1 */
		if(m_page->m_mode==MODE_MINMAX)
		{
			//used for background-image???
			bool show=false;

			/* are all my children hidden? */
			for(i=0;i<m_numstylechildren;++i)
			{
				if(m_stylechildren.GetEntry(i)->m_display!=DISPLAY_NONE)
					show=true;
			}

			/* are all my children display:none? */
			if(show==false)
			{
				m_display=DISPLAY_NONE;
				goto done;
			}
		}
	break;
#endif
	case HTMLTAG_FONT:
		if(m_page->m_mode==MODE_MINMAX)
		{
			kGUIHTMLAttrib *att;

			att=FindAttrib(HTMLATT_SIZE);
			if(att)
			{
				int size;

				m_page->PushStyle(sizeof(m_page->m_fontsize),&m_page->m_fontsize);
				if(att->GetValue()->GetChar(0)=='+' || att->GetValue()->GetChar(0)=='-')
				{
					/* convert float font size to index size */
					size=1;
					while(m_page->m_fontsize>xfontsize[size-1])
					{
						++size;
						if(size==7)
							break;
					}
					size+=att->GetValue()->GetInt();
				}
				else
					size=att->GetValue()->GetInt();

				if(size<1)
					size=1;
				else if(size>7)
					size=7;
				m_page->m_fontsize=xfontsize[size-1];
				m_page->CalcEM();
				m_page->CalcLH();
			}
		}
	break;
	case HTMLTAG_FORM:
		if(m_page->m_mode==MODE_MINMAX)
		{
			m_page->PushStyle(sizeof(m_page->m_form),&m_page->m_form);
			m_page->m_form=this;
		}
	break;
	case HTMLTAG_SINGLEOBJ:
#if 0
		if(m_objinit==false)
		{
			m_objinit=true;

			/* this cannot be done in the PreProcess stage as maps can be in the page */
			/* after the image so they won't be added to the map list yet */
			att=FindAttrib(HTMLATT_USEMAP);
			if(att)
			{
				/* locate the map */
				map=m_page->LocateMap(att->GetValue());
				if(map)
					m_obj.m_imageobj->SetMap(map);
				else
				{
					/* error, map not found */
				}
			}
		}
#endif
		rp->PositionChild(this,m_obj.m_singleobj,m_obj.m_singleobj->GetZoneH(),0);
	break;
	case HTMLTAG_HR:
		/* size is in pixels */
		att=FindAttrib(HTMLATT_SIZE);
		if(att)
			SetInsideH(att->GetValue()->GetInt());
	break;
	case HTMLTAG_BR:
		if(m_page->m_mode==MODE_MINMAX)
		{
			kGUIHTMLTextObj textobj;

			textobj.SetFontID(m_page->m_fontid);
			textobj.SetFontSize(m_em);
			m_height.SetUnitType(UNITS_PIXELS);
			m_height.SetUnitValue((int)textobj.GetLineHeight());
		}
		rp->PositionHardBreak(m_height.GetUnitValue());
	break;
	case HTMLTAG_IMBEDTEXTGROUP:
	{
		kGUIHTMLTextGroup *textgroup;

		if(m_page->m_mode==MODE_MINMAX)
		{
			/* has the whitespace mode changed? */
			textgroup=m_obj.m_textgroup;

			if(m_page->m_whitespace!=textgroup->GetLastWhitespace() ||
				m_page->m_texttransform!=textgroup->GetLastTransform())
			{
				/* delete old imbed text objects */
				/* re-split the text using the new whitespace and transform rules */
				textgroup->Split(m_page->m_whitespace,m_page->m_texttransform,m_renderparent,this);
			}
			/* if I have no children then don't display me */
			if(!textgroup->GetNumChildren())
			{
				m_display=DISPLAY_NONE;
				goto done;
			}
		}
	}
	break;
	case HTMLTAG_IMBEDTEXT:
	{
		kGUIHTMLTextObj *textobj;
		int w,h,bold;

		textobj=m_obj.m_textobj;
		if(m_page->m_mode==MODE_MINMAX)
		{
			/* inherit valign from parent */
			if(m_renderparent->m_id==HTMLTAG_TD)
				SetVAlign(VALIGN_BASELINE);
			else
				SetVAlign(m_renderparent->GetVAlign());

			if(m_page->m_fontweight>=700)
				bold=1;
			else
				bold=0;
			textobj->SetTextDecoration(m_page->m_textdecoration);
			textobj->SetTextDecorationColor(m_page->m_textdecorationcolor);
			textobj->SetFontID(m_page->m_fontid+bold);

			/* handle text drop shadows */
			/* todo: check if color is transparent */
			textobj->SetShadow(m_page->m_textshadowr,m_page->m_textshadowx,m_page->m_textshadowy,m_page->m_textshadowcolor.GetColor());

			textobj->SetFontSize(m_em);
			textobj->SetColor(m_page->m_fontcolor.GetColor());
			textobj->SetAlpha(m_page->m_fontcolor.GetAlpha());
			textobj->SetLetterSpacing(m_page->m_letterspacing.CalcUnitValue(0,m_em));
			textobj->SetLink(m_page->m_link);

			/* if blink mode is active on this text then add it to the blink list */
			if(m_page->m_textdecoration&TEXTDECORATION_BLINK)
				m_page->m_blinks.SetEntry(m_page->m_numblinks++,textobj);

			w=textobj->GetWidth();
			if(textobj->GetFixedWidth())
				w=max(w,(int)(textobj->GetFixedWidth()*m_em));	/* used for LI prefix as it is right aligned and wider */

			if(textobj->GetPlusSpace())
				w+=(int)(0.33f*m_em)+(m_page->m_wordspacing.CalcUnitValue(0,m_em));
			textobj->MoveZoneW(w);

			h=textobj->GetLineHeight();
			if(m_page->m_textdecoration&TEXTDECORATION_UNDERLINE)
				h=max(h,(int)textobj->GetAscHeight()+4);

			textobj->MoveZoneH(h);
		}

		/* this isn't needed, I just have it for debugging */
		m_minw=m_maxw=textobj->GetZoneW();
		MoveZoneW(m_minw);
		MoveZoneH(textobj->GetZoneH());

//		kGUI::Trace("Text='%s'\n",textobj->GetString());
		rp->PositionChild(this,textobj,textobj->GetAscHeight(),textobj->GetDescHeight()+2);


		if(textobj->GetPlusCR())	/* only set in PRE mode */
			rp->PositionBreak();
	}
	break;
	case HTMLTAG_CONTENTGROUP:		/* this is the :before and :after content */
	{
		kGUIHTMLContentGroup *contentgroup;

		if(m_page->m_mode==MODE_MINMAX)
		{
			/* has the whitespace mode changed? */
			contentgroup=m_obj.m_contentgroup;

			/* has the content changed since last time? */
			if(contentgroup->GetChanged())
			{
				/* delete old content and insert new updated content */
				contentgroup->Split(m_renderparent,this);
			}
		}
	}
	break;
	case HTMLTAG_TEXTAREA:
	{
		kGUIHTMLInputBoxObj *inputobj;
		int bold;

		inputobj=m_obj.m_inputobj;

		if(m_page->m_mode==MODE_MINMAX)
		{
			inputobj->SetTextDecoration(m_page->m_textdecoration);
			inputobj->SetTextDecorationColor(m_page->m_textdecorationcolor);
			if(m_page->m_fontweight>=700)
				bold=1;
			else
				bold=0;
			inputobj->SetFontID(m_page->m_fontid+bold);
			inputobj->SetFontSize(m_em);
			inputobj->SetColor(m_page->m_fontcolor.GetColor());

		}
		if(m_fixedw==true || m_relw==true)
			inputobj->MoveZoneW(GetInsideW());
		else
		{
			att=FindAttrib(HTMLATT_COLS);
			if(att)
				inputobj->MoveZoneW(att->GetValue()->GetInt()*m_em);
		}
		if(m_fixedh==true || m_relh==true)
			inputobj->MoveZoneH(GetInsideH());
		else
		{
			att=FindAttrib(HTMLATT_ROWS);
			if(att)
				inputobj->MoveZoneH((att->GetValue()->GetInt()*m_em+6));
		}

		rp->PositionChild(this,inputobj,inputobj->GetZoneH(),0);
	}
	break;
	case HTMLTAG_SELECT:
		switch(GetSubID())
		{
		case HTMLSUBTAG_INPUTLISTBOX:
		{
			kGUIListboxObj *listobj;
			int childwidth,childheight;

			listobj=m_obj.m_listboxobj;

			if(m_page->m_mode==MODE_MINMAX)
			{
				unsigned int e;
				kGUIText *line;
				int bold;

				if(m_page->m_fontweight>=700)
					bold=1;
				else
					bold=0;
				listobj->SetFontID(m_page->m_fontid+bold);
				listobj->SetFontSize(m_em);
				listobj->SetColor(m_page->m_fontcolor.GetColor());

				for(e=0;e<listobj->GetNumEntries();++e)
				{
					line=listobj->GetEntryTextPtr(e);
					line->SetFontID(m_page->m_fontid+bold);
					line->SetFontSize(m_em);
					line->SetColor(m_page->m_fontcolor.GetColor());
					listobj->SetRowHeight(e,listobj->CalcRowHeight(e));
				}
			}
			if(m_fixedw==true || m_relw==true)
				childwidth=GetChildZoneW();
			else
				childwidth=listobj->GetWidest();
			if(m_fixedh==true || m_relh==true)
				childheight=GetChildZoneH();
			else
				childheight=listobj->CalcHeight(FindAttrib(HTMLATT_SIZE)->GetValue()->GetInt());

			m_minw=m_maxw=childwidth;
			m_maxy=childheight;
			listobj->MoveZone(0,0,childwidth,childheight);
			listobj->CalcChildZone();

			/* if we are not inline then the parent will be added so don't bother here */
			if(m_display==DISPLAY_INLINE)
				rp->PositionChild(this,listobj,childheight,0);
		}
		break;
		default:
		{
			int childwidth,childheight;
			kGUIComboBoxObj *comboobj;

			//todo: for speed this should keep track of last font/size/color etc and only 
			//re-calc the size if it has changed

			comboobj=m_obj.m_comboboxobj;
			if(m_page->m_mode==MODE_MINMAX)
			{
				unsigned int e;
				kGUIText *line;
				int bold;

				if(m_page->m_fontweight>=700)
					bold=1;
				else
					bold=0;
				comboobj->SetFontID(m_page->m_fontid+bold);
				comboobj->SetFontSize(m_em);
				comboobj->SetColor(m_page->m_fontcolor.GetColor());

				for(e=0;e<comboobj->GetNumEntries();++e)
				{
					line=comboobj->GetEntryTextPtr(e);

					line->SetFontID(m_page->m_fontid+bold);
					line->SetFontSize(m_em);
					line->SetColor(m_page->m_fontcolor.GetColor());
				}
			}

			if(m_fixedw==true || m_relw==true)
				childwidth=GetChildZoneW();
			else
				childwidth=comboobj->GetWidest()+6;
			if(m_fixedh==true || m_relh==true)
				childheight=GetChildZoneH();
			else
				childheight=comboobj->GetFontHeight()+8;

			m_minw=m_maxw=childwidth;
			m_maxy=childheight;
			comboobj->MoveZone(0,0,childwidth,childheight);

			/* if we are not inline then the parent will be added so don't bother here */
			if(m_display==DISPLAY_INLINE)
				rp->PositionChild(this,comboobj,childheight,0);
		}
		break;
		}
	break;
	case HTMLTAG_LIIMG:
	{
		int imagewidth,imageheight;
		imagewidth=m_obj.m_imageobj->GetImageWidth();
		imageheight=m_obj.m_imageobj->GetImageHeight();

		/* this is a user image on a list */
		m_obj.m_imageobj->MoveZone(0,0,imagewidth,imageheight);

		//is this needed?
		m_minw=m_maxw=imagewidth;
		m_maxy=imageheight;
		MoveZoneW(imagewidth);
		MoveZoneH(imageheight);

		rp->PositionChild(this,m_obj.m_imageobj,m_obj.m_imageobj->GetZoneH(),0);
	}
	break;
	case HTMLTAG_LISHAPE:
		if(m_page->m_mode==MODE_MINMAX)
		{
			int width,height;

			/* this is one of the little bullet shapes on a list */
			width=m_page->GetEM()*2;
			height=m_page->GetEM();

			m_obj.m_shapeobj->MoveZone(0,0,width,height);

			m_minw=m_maxw=width;
			m_maxy=height;
		}
		rp->PositionChild(this,m_obj.m_shapeobj,m_obj.m_shapeobj->GetZoneH(),0);
	break;
	case HTMLTAG_IMG:
	{
		int childwidth,childheight,imagewidth,imageheight;
		double wscale,hscale;
		kGUIHTMLAttrib *att;
		kGUIHTMLMap *map;

		if(m_objinit==false)
		{
			m_objinit=true;

			/* this cannot be done in the PreProcess stage as maps can be in the page */
			/* after the image so they won't be added to the map list yet */
			att=FindAttrib(HTMLATT_USEMAP);
			if(att)
			{
				/* locate the map */
				map=m_page->LocateMap(att->GetValue());
				if(map)
					m_obj.m_imageobj->SetMap(map);
				else
				{
					/* error, map not found */
				}
			}
		}
		
		if(m_page->m_mode==MODE_MINMAX)
			m_obj.m_imageobj->SetLink(m_page->m_link);

		imagewidth=m_obj.m_imageobj->GetImageWidth();
		imageheight=m_obj.m_imageobj->GetImageHeight();

		/* calc width/height of box and then width/height of image */
		if(m_fixedw || m_relw)
			childwidth=GetInsideW();
		else
		{
			if(m_fixedh || m_relh)
			{
				if(imageheight)
					childwidth=(int)(imagewidth*((double)GetInsideH()/(double)imageheight));
				else
				{
					imagewidth=0;
					childwidth=0;
				}
			}
			else
				childwidth=imagewidth;
		}

		if(imagewidth)
			wscale=(double)childwidth/(double)imagewidth;
		else
			wscale=0.0f;

		if(m_fixedh==false && m_relh==false)
		{
			/* example: image= 500x400, style="width=150px" */
			if(m_fixedw==true || m_relw==true)
				childheight=(int)(wscale*imageheight);
			else
				childheight=imageheight;
		}
		else
			childheight=GetInsideH();

		if(imageheight)
			hscale=(double)childheight/(double)imageheight;
		else
			hscale=0.0f;

		m_minw=m_maxw=childwidth;
		m_maxy=childheight;
		m_obj.m_imageobj->MoveZone(0,0,childwidth,childheight);
		m_obj.m_imageobj->SetScale(wscale,hscale);
		m_obj.m_imageobj->SetAnimate(true);

		/* if we are not inline then the parent will be added so don't bother here */
		if(m_display==DISPLAY_INLINE)
			rp->PositionChild(this,m_obj.m_imageobj,childheight,0);
	}
	break;
	case HTMLTAG_OBJECT:
		/* it stays as an "object" until it is loaded and recognized */
		/* once it is recognized then it is changed to the proper object type */
		/* if we don't know how to render it then it stays an object and it not drawn */
		/* only check on min-max pass since if it arrives between passes it will mess up the layout */

		if(m_page->m_mode==MODE_MINMAX)
		{
			if(m_objinit==false && m_obj.m_linked)
			{
				if(m_obj.m_linked->GetLoadPending()==false)
				{
					m_objinit=true;
					if(DetectObject()==true)
						goto typechanged;
				}
			}
		}
	break;
	}
	/* recalc child area since attributes have updated the margins etc. */
	CalcChildZone();

	switch(m_id)
	{
	case HTMLTAG_BUTTON:
		goto isbutton;
	break;
	case HTMLTAG_INPUT:
		switch(m_subid)
		{
		case HTMLSUBTAG_INPUTBUTTON:
		case HTMLSUBTAG_INPUTBUTTONSUBMIT:
		case HTMLSUBTAG_INPUTBUTTONRESET:
		{
isbutton:	kGUIHTMLButtonTextObj *buttonobj;
			int bold;

			buttonobj=m_obj.m_buttontextobj;

			if(m_fixedw==true || m_relw==true)
				buttonobj->MoveZoneW(GetInsideW());

			if(m_fixedh==true || m_relh==true)
				buttonobj->MoveZoneH(GetInsideH());

			if(m_page->m_mode==MODE_MINMAX)
			{
				if(m_page->m_fontweight>=700)
					bold=1;
				else
					bold=0;
				buttonobj->SetTextDecoration(m_page->m_textdecoration);
				buttonobj->SetTextDecorationColor(m_page->m_textdecorationcolor);
				buttonobj->SetFontID(m_page->m_fontid+bold);
				buttonobj->SetFontSize(m_em);
				buttonobj->SetColor(m_page->m_fontcolor.GetColor());
			}
			rp->PositionChild(this,buttonobj,buttonobj->GetZoneH(),0);
		}
		break;
		case HTMLSUBTAG_INPUTBUTTONIMAGE:
		{
			int width,height,imagewidth,imageheight;
			double wscale,hscale;

			if(m_page->m_mode==MODE_MINMAX)
				m_obj.m_buttonimageobj->SetLink(m_page->m_link);

			imagewidth=m_obj.m_buttonimageobj->GetImageWidth();
			imageheight=m_obj.m_buttonimageobj->GetImageHeight();

	//		kGUI::Trace("Image='%s'\n",m_obj.m_imageobj->GetURL()->GetString());

			if(m_fixedw==false && m_relw==false)
			{
				width=imagewidth;
				wscale=1.0f;
				m_obj.m_buttonimageobj->MoveZoneW(width);
			}
			else
			{
				width=GetInsideW();
				if(imagewidth)
					wscale=(double)width/(double)imagewidth;
				else
					wscale=0.0f;
			}

			if(m_fixedh==false && m_relh==false)
			{
				height=imageheight;
				hscale=1.0f;
				m_obj.m_buttonimageobj->MoveZoneH(height);
			}
			else
			{
				height=GetInsideH();
				if(imageheight)
					hscale=(double)height/(double)imageheight;
				else
					hscale=0.0f;
			}

			m_obj.m_buttonimageobj->SetScale(wscale,hscale);
			m_obj.m_buttonimageobj->SetAnimate(true);

			rp->PositionChild(this,m_obj.m_buttonimageobj,m_obj.m_buttonimageobj->GetZoneH(),0);
		}
		break;
		case HTMLSUBTAG_INPUTCHECKBOX:
			if(m_page->m_mode==MODE_MINMAX)
			{
				int size=(int)(m_em*0.63f);

				if(size<13)
					size=13;
				m_obj.m_tickobj->SetScale(true);
				m_obj.m_tickobj->MoveSize(size,size);
			}
			rp->PositionChild(this,m_obj.m_tickobj,m_obj.m_tickobj->GetZoneH(),0);
		break;
		case HTMLSUBTAG_INPUTRADIO:
			if(m_page->m_mode==MODE_MINMAX)
			{
				int size=(int)(m_em*0.63f);

				if(size<13)
					size=13;
				m_obj.m_radioobj->SetScale(true);
				m_obj.m_radioobj->MoveSize(size,size);
			}
			rp->PositionChild(this,m_obj.m_radioobj,m_obj.m_radioobj->GetZoneH(),0);
		break;
		case HTMLSUBTAG_INPUTTEXTBOX:
		case HTMLSUBTAG_INPUTFILE:
		{
			kGUIHTMLInputBoxObj *inputboxobj;

			inputboxobj=m_obj.m_inputobj;
			if(m_page->m_mode==MODE_MINMAX)
			{
				int w,bold;

				att=FindAttrib(HTMLATT_SIZE);
				if(att)
					w=(int)(att->GetValue()->GetInt()*m_em*0.6f);
				else
					w=(int)(6*m_em*0.6f);	/* default size if not defined */

				if(m_page->m_fontweight>=700)
					bold=1;
				else
					bold=0;

				inputboxobj->SetTextDecoration(m_page->m_textdecoration);
				inputboxobj->SetTextDecorationColor(m_page->m_textdecorationcolor);
				inputboxobj->SetFontID(m_page->m_fontid+bold);
				inputboxobj->SetFontSize(m_em);
				inputboxobj->SetColor(m_page->m_fontcolor.GetColor());
				inputboxobj->MoveZoneW(w);
				inputboxobj->MoveZoneH(inputboxobj->GetLineHeight()+6);
			}
			rp->PositionChild(this,inputboxobj,inputboxobj->GetZoneH(),0);
		}
		break;
		}
	break;
	case HTMLTAG_OPTION:
	{
		kGUIText *text;
		int bold;

		/* this is connected to a combobox or a listbox */
		/* todo, only flag 'dirty' if it has changed */
		text=m_obj.m_text;
		if(text && m_page->m_mode==MODE_MINMAX)
		{
			if(m_page->m_fontweight>=700)
				bold=1;
			else
				bold=0;

			//	textobj->SetTextDecoration(m_page->m_textdecoration);
			text->SetFontID(m_page->m_fontid+bold);
			text->SetFontSize(m_em);
			text->SetColor(m_page->m_fontcolor.GetColor());
		}
	}
	break;
	case HTMLTAG_FRAME:
	case HTMLTAG_IFRAME:
		if(m_page->m_mode==MODE_MINMAX)
		{
			if(m_objinit==false && m_obj.m_linked)
			{
				if(m_obj.m_linked->GetLoadPending()==false)
					InsertFrame();
			}
		}
	break;
	}

//	kGUI::Trace("Before Container size=%d,%d\n",GetZoneW(),GetZoneH());
	/* calc sizes of all children by traversing the style tree*/

	for(i=0;i<m_numstylechildren;++i)
		m_stylechildren.GetEntry(i)->Position();

	/* if I have any unplaced children, then place them now */
	if(m_pos->m_ponum)
		PositionBreak();

	switch(m_id)
	{
	case HTMLTAG_TABLE:
		if(m_page->m_mode==MODE_MINMAX)
		{
			/* this is to fix a problem caused by rows overlapping each other */
			/* due to cells with rowspan>1 */
			SetAllowOverlappingChildren(true);
			/* calc number of rows and columns and min/max size of table*/
			m_ti->m_box=m_box;
			m_ti->m_cellborder->SetBorder(0);
			if(m_box)
			{
				m_ti->m_cellborder->SetBorder(m_box->GetBoxWidth()>0?1:0);
				m_ti->m_cellborder->SetUndefinedBorderColors(DrawColor(0,0,0),true);
			}
			m_ti->m_cellspacing=m_page->m_cellspacing;
			m_ti->m_cellpadding=m_page->m_cellpadding;
			m_ti->CalcMinMax(this,(int)(m_page->m_fontsize*m_page->m_fontscale));
		}
		else
		{
			/* position all cells and set sizes */
			m_ti->PositionCells(this);
		}
	break;
	}

	/* check for fixed position/width */
	if(m_id!=HTMLTAG_TABLE && m_id!=HTMLTAG_TD && m_id!=HTMLTAG_TR && m_id!=HTMLTAG_TH)
		CheckFixed();

	/* calc size needed to hold all renderable child objects */
	
	if(m_page->m_mode==MODE_MINMAX)
	{
		if(m_id!=HTMLTAG_TR && m_id!=HTMLTAG_TD && m_id!=HTMLTAG_TH)
		{
			if(m_fixedw==false && m_relw==false)
				SetInsideW(m_minw);
		}
	}

	m_maxchildy=max(m_pos->m_cury,m_maxy);
	if(m_fixedh==false && m_relh==false)
		SetInsideH(m_maxchildy);
	else
	{
		if(m_maxchildy>GetInsideH())
		{
			/* todo: handle different overlap modes */
			rp->SetAllowOverlappingChildren(true);
		}
	}

	CalcChildZone();
#if 1
	if(m_page->m_mode==MODE_MINMAX)
	{
		kGUIHTMLObj *pb;
		int bw;

//		pb=GetParentObj();
		pb=m_renderparent;

		if(pb)
		{
			if(!m_box)
				bw=0;
			else
				bw=m_box->GetBoxLeftWidth()+m_box->GetBoxRightWidth();

			/* make sure my parent can fit me */
			if((m_minw+bw)>pb->m_minw)
				pb->m_minw=m_minw+bw;
			if((m_maxw+bw)>pb->m_maxw)
				pb->m_maxw=m_maxw+bw;
		}
	}
#endif
	/* if any direct children reference my height then we need to do this twice */
	if((m_childusesheight==true) && (m_page->m_mode==MODE_POSITION) && pass==false && placeme==true)
	{
		/* only valid if we are a box object */

		/* only if changed */
		if(GetZoneH()!=oldh)
		{
			pass=true;
			MoveZoneH(0);
			goto dopass2;
		}
	}

	/* if I am an inline object then my children have been added dircetly to their */
	/* ancestor and we don't need to reserve space for me */

	if(placeme==true)
	{
		switch(m_display)
		{
		case DISPLAY_INLINE:
		case DISPLAY_MARKER:
		case DISPLAY_ANONYMOUS:
			addme=false;	/* since my children are all added then I don't need to be */
		break;
		case DISPLAY_BLOCK:
		case DISPLAY_LIST_ITEM:
		case DISPLAY_TABLE:
			addme=true;
		break;
		case DISPLAY_INLINE_BLOCK:
		case DISPLAY_INLINE_TABLE:
			addme=true;
		break;
		case DISPLAY_TABLE_ROW:
		case DISPLAY_TABLE_CELL:
	//		if(m_ti)				
				addme=false;		/* is connected to a table so the table will position it */
	//		else
	//			addme=true;			/* not connected to a table, so position it */
		break;
		default:
			assert(false,"unhandled case!");
		break;
		}
	}
	else
		addme=false;

	if(addme==true || m_relpos)
	{
		if(rp && m_position!=POSITION_FIXED && m_position!=POSITION_ABSOLUTE)
		{
#if 0
			if(m_relpos==true)
			{
				int xoff,yoff;

				xoff=GetZoneX();
				yoff=GetZoneY();
				rp->PositionChild(this,this,GetZoneH(),0);
				MoveZoneX(GetZoneX()+xoff);
				MoveZoneY(GetZoneY()+yoff);
			}
			else
#endif
				rp->PositionChild(this,this,GetZoneH(),0);
		}
	}
	else
	{
		/* since this is inline we don't need this position for rendering, but... */
		/* we do need it if this is an local link so we can calc the scroll down position */
		if(placeme==true)
		{
			SetZoneX(rp->m_pos->m_curx);
			SetZoneY(rp->m_pos->m_curx);
		}
	}

	if(m_page->m_mode==MODE_POSITION && m_scroll)
	{
		//since the size may have changed, re postion the scrollbars 
		m_scroll->Scrolled();
	}

done:;
	if(m_pos->m_ponum)
		PositionBreak();

	m_page->PopStyles(popindex);
	m_page->RemovePossibleRules(this);

	m_page->FreePos();
	m_pos=0;
}

/* a frame was just loaded, insert it into the tree and parse it */
void kGUIHTMLObj::InsertFrame(void)
{
	kGUIString s;
	kGUIString tci;
	kGUIString oldurl;
	kGUIHTMLObj *child;
	unsigned int urlid;

	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLRuleCache **rcp;
	kGUIOnlineLink *link;

	m_objinit=true;

	/* find TCI index in cache and use this as the base tci string for the pre-process */
	n=m_page->m_tcicache.GetNum();
	he=m_page->m_tcicache.GetFirst();
	for(i=0;i<n;++i)
	{
		rcp=(kGUIHTMLRuleCache **)he->m_data;
			
		if(m_rulecache==*(rcp))
		{
			tci.SetString(he->m_string);
			break;
		}
		he=he->GetNext();
	}
	assert(tci.GetLen()>0,"Error, couldn't find TCI cache entry!");

	link=m_obj.m_linked;
	/* convert link (DataHandle) to a string and then parse it */
	link->Open();
	link->Read(&s,link->GetSize());
	link->Close();

	/* change base URL to this one, then put back after parse */
	oldurl.SetString(m_page->m_url.GetString());
//	kGUI::Trace("Saving Old URL as %s\n",oldurl.GetString());
	m_page->SetURL(link->GetURL());
//	kGUI::Trace("Setting New URL as %s\n",link->GetURL()->GetString());

	{
		kGUIString header;

		/* todo, put proper page header in here! */
		m_page->Parse(this,s.GetString(),s.GetLen(),&header);
	}

	/* don't call for me again, just call for my children that were just added */
	/* skip any children that were there before the frame was inserted */
	urlid=m_page->StringToIDcase(&m_page->m_url);
	for(i=0;i<m_numstylechildren;++i)
	{
		child=m_stylechildren.GetEntry(i);
		if(urlid==child->GetOwnerURL())
			m_page->PreProcess(&tci,child,true);
	}

	m_page->SetURL(&oldurl);
//	kGUI::Trace("Restoring Old URL as %s\n",oldurl.GetString());
}

/* called after an unknown "object" has been loaded, if recognized then we will */
/* change the "type" to the new type and return true! */
bool kGUIHTMLObj::DetectObject(void)
{
	kGUIOnlineLink *link;
	kGUIImageObj *image;
	kGUIString s;

	link=m_obj.m_linked;

	/* load the data into a string */
	link->Open();
	link->Read(&s,link->GetSize());
	link->Close();

	image=new kGUIImageObj();
	image->SetMemory();
	image->OpenWrite("wb",s.GetLen());
	image->Write(s.GetString(),s.GetLen());
	image->Close();

	if(image->IsValid())
	{
		SetID(HTMLTAG_SINGLEOBJ);
		m_tag=&m_page->m_singleobjtag;
		m_obj.m_singleobj=image;

		AddRenderObject(image);

		image->SetSize(image->GetImageWidth(),image->GetImageHeight());
		/* this needs to be changed! */
//		image->SetEventHandler(this,CALLBACKNAME(RightClickEvent));
		image->SetAnimate(true);
		return(true);
	}
	/* not an image so delete it */
	delete image;

	/* todo: we need to check here for movies or flash etc */

#if 0
		/* is it plain text? */
		if(!strcmp(m_type.GetString(),"text"))
		{
			printf("plain text");
		}

		/* try using one of the plugins */
		for(i=0;(i<m_numplugins && m_singlemode==false);++i)
		{
			kGUIHTMLPluginObj *po;
			kGUIHTMLObj *newobj;

			po=m_plugins.GetEntry(i)->New();
			po->GetDH()->SetMemory((const unsigned char *)m_fp,m_len);
			if(po->Open()==true)
			{
				/* yes it was valid, so let's show it */
				m_singlemode=true;

				/* get an object to add the right-click to */
				po->GetObj()->SetEventHandler(this,CALLBACKNAME(RightClickEvent));

				newobj=new kGUIHTMLObj(renderparent,this,&m_singleobjtag);
				newobj->SetOwnerURL(StringToIDCase(&m_url));
				newobj->SetAlign(ALIGN_CENTER);
				newobj->SetVAlign(VALIGN_BOTTOM);
				renderparent->AddStyleChild(newobj);
				newobj->m_obj.m_singleobj=po;
				newobj->AddRenderObject(po);
			}
			else
				delete po;
		}

#endif
	return(false);
}


/* if this object is an inline object then all children are positioned */
/* inside their next parent that is not inline, so we need to calc a bounding */
/* box for them and contain them all in it */
void kGUIHTMLObj::Contain(bool force)
{
	unsigned int i;
	unsigned int nc;
	int lx=0,rx=0,ty=0,by=0;
	int olx,orx,oty,oby;
	kGUIHTMLObj *sobj;
	kGUIObj *obj;
	kGUICorners c;
	kGUICorners cc;

	if(m_display==DISPLAY_NONE)
		return;

	/* keep track of widest and tallest item in page, and it's children */
	GetCorners(&c);
	nc=GetNumChildren();
	for(i=0;i<nc;++i)
	{
		obj=GetChild(i);
		obj->GetCorners(&cc);
		if(cc.rx>c.rx)
			c.rx=cc.rx;
		if(cc.by>c.by)
			c.by=cc.by;
	}

	if(c.rx>m_page->m_maxpagew)
		m_page->m_maxpagew=c.rx;
	if(c.by>m_page->m_maxpageh)
		m_page->m_maxpageh=c.by;

	for(i=0;i<m_numstylechildren;++i)
	{
		sobj=m_stylechildren.GetEntry(i);
		sobj->Contain();
	}

	/* if I am not inline then my children are already contained to me */
	switch(m_display)
	{
	case DISPLAY_INLINE:
	case DISPLAY_MARKER:
	case DISPLAY_ANONYMOUS:
		/* calc min/max for all children */
		/* then set my position to that - border edges */
		/* then subtract that position from all my children since they are all relative to me */

		m_maxw=0;
		m_minw=0;
		nc=GetNumChildren();
		if(nc)
		{
			for(i=0;i<nc;++i)
			{
				obj=GetChild(i);
				m_maxw+=obj->GetZoneW();
				if(obj->GetZoneW()>m_minw)
					m_minw=obj->GetZoneW();

				if(!i)
				{
					lx=obj->GetZoneX();
					rx=lx+obj->GetZoneW();
					ty=obj->GetZoneY();
					by=ty+obj->GetZoneH();
				}
				else
				{
					olx=obj->GetZoneX();
					orx=olx+obj->GetZoneW();
					oty=obj->GetZoneY();
					oby=oty+obj->GetZoneH();
					lx=min(lx,olx);
					ty=min(ty,oty);
					rx=max(rx,orx);
					by=max(by,oby);
				}
			}
			/* set my position and size to this */
			m_maxy=by-ty;

			MoveZoneX(lx);
			MoveZoneY(ty);
			SetInsideW(rx-lx);
			SetInsideH(by-ty);
			CalcChildZone();
			SetAllowOverlappingChildren(true);

			/* now make all children relative to ME */
			for(i=0;i<nc;++i)
			{
				obj=GetChild(i);
				olx=obj->GetZoneX();
				oty=obj->GetZoneY();
				obj->MovePos(olx-lx,oty-ty);
			}
		}
	break;
	default:
#if 0
		/* double check to make sure I am big enough to hold my children */
		/* since some could have been absolutely positioned outside of me or */
		/* relatively positioned outside of me */
		if(/*(m_page->m_mode==MODE_POSITION) ||*/ (m_id==HTMLTAG_BODY || m_id==HTMLTAG_ROOT) || force)
		{
			int oldw,oldh;
			int neww,newh;
			/* debug, am I big enough to hold all my children? */
			m_error=false;
			neww=oldw=GetZoneW();
			newh=oldh=GetZoneH();
			for(i=0;i<GetNumChildren();++i)
			{
				kGUIZone *z;

				z=GetChild(i);
				if(z->GetZoneW() && z->GetZoneH())
				{
					if(z->GetZoneRX()>GetZoneW() || z->GetZoneBY()>GetZoneH())
					{
						neww=max(neww,z->GetZoneRX());		
						newh=max(newh,z->GetZoneBY());		
						m_error=true;
					}
				}
			}
			if(m_error==true)
			{
			//	if(m_fixedw==false)
					MoveZoneW(neww);
			//	else
			//		SetContainChildren(false);
			//	if(m_fixedh==false)
					MoveZoneH(newh);
			//	else
			//		SetContainChildren(false);
				CalcChildZone();

				/* since I might overlap another object, tell the input handler to not abort processing */
				/* if it can'f find an active object inside me, keep trying.... */
				SetAllowOverlappingChildren(true);
			}
		}
#endif
	break;
	}

	/* this is probably a hack that can go away once some other bug is fixed */
	if(m_page->m_mode==MODE_POSITION)
	{
		if(m_id==HTMLTAG_BODY || m_id==HTMLTAG_ROOT)
		{
			/* adjust for scroll values by subtracting page scroll offsets */
			MoveZoneW(m_page->m_maxpagew-c.lx);
			MoveZoneH(m_page->m_maxpageh-c.ty);
		}
	}
}

/**********************************************************************************/

void kGUIOnlineImage::Purge(void)
{
	/* if image is re-referenced then we need to retrigger the download */
	if(m_dl.GetAsyncActive()==false)
		m_loadtriggered=false;	

	kGUIImage::Purge();
}

static unsigned char ReadURI(unsigned const char **pp)
{
    unsigned const char *p = *pp;
    unsigned char c;

	/* skip all whitespace */
    do{
        c = *(p++); 
    }while ((c==' ') || (c=='\r') || (c=='\n') || (c == '\t'));

    if (c == '%')
	{
        char c1 = *(p++);
        char c2 = *(p++);

		if(c1>='0' && c1<='9')
			c=(c1-'0');
		else if(c1>='A' && c1<='F')
			c=(c1-'A');
		else if(c1>='a' && c1<='f')
			c=(c1-'a');
		else
			return (0);
        c = c << 4;

		if(c2>='0' && c2<='9')
			c+=(c2-'0');
		else if(c2>='A' && c2<='F')
			c+=(c2-'A'+10);
		else if(c2>='a' && c2<='f')
			c+=(c2-'a'+10);
		else return(0);
    }

    *pp = p;

    return c;
}

static int Read6(unsigned const char **pp)
{
	static int xref[256]=
	{ 
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,  /* 0  */
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,  /* 16 */
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, 62, -1, -1, -1, 63,  /* 32 */
    52, 53, 54, 55, 56, 57, 58, 59,   60, 61, -1, -1, -1, -1, -1, -1,  /* 48 */
    -1,  0,  1,  2,  3,  4,  5,  6,    7,  8,  9, 10, 11, 12, 13, 14,  /* 64 */
    15, 16, 17, 18, 19, 20, 21, 22,   23, 24, 25, -1, -1, -1, -1, -1,  /* 80 */
    -1, 26, 27, 28, 29, 30, 31, 32,   33, 34, 35, 36, 37, 38, 39, 40,  /* 96 */
    41, 42, 43, 44, 45, 46, 47, 48,   49, 50, 51, -1, -1, -1, -1, -1   /* 112 */

    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,  /* 128 */
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1   };

    return(xref[ReadURI(pp)]);
}

void kGUIOnlineImage::SetURL(kGUIString *url,kGUIString *referrer)
{
	bool infile;

	infile=false;
	/* is this an inline image imbedded right into the webpage? */
	if(!strcmpin(url->GetString(),"data:",5))
	{
		const unsigned char *cp;

		if(m_loadtriggered)
			return;

		cp=(const unsigned char *)strstr(url->GetString(),"base64,");
		if(cp)
		{
			int index;
			unsigned int in[4];
			unsigned char byte;
		
			SetMemory();	/* write to allocated memory */
			m_loadtriggered=true;
			OpenWrite("wb",(int)strlen((const char *)cp));
			
			index=0;
			cp+=7;	/* skip "base64," */
			while(cp[0])
			{
				byte=Read6(&cp);
				if(byte!=255)		/* Read6 returns -1 if illegal value in string */
				{
					in[index]=byte;
					switch(index++)
					{
					case 0:
						/* not enough bits to write out yet */
					break;
					case 1:
						byte=(in[0]<<2)|(in[1]>>4);
						Write(&byte,1);
					break;
					case 2:
						byte=(in[1]<<4)|(in[2]>>2);
						Write(&byte,1);
					break;
					case 3:
						byte=(in[2]<<6)|in[3];
						Write(&byte,1);
						index=0;
					break;
					}
				}
			}

			Close();
			m_page->ImageLoaded(this);
			return;
		}
	}
	else if(strcmpin(url->GetString(),"http://",7) && strcmpin(url->GetString(),"https://",8))
	{
		m_loadtriggered=true;
		/* must be a local file not a URL */
		SetFilename(url->GetString());
		return;
	}

	/* if the name has changed or the flush flag is set then */
	/* force a reload */
	if(m_url.GetLen())
	{
		if(stricmp(m_url.GetString(),url->GetString()))
			Purge();
	}

	/* if this is true then it is one of: */
	/*	1) currently being downloaded */ 
	/*	2) error while downloading */ 
	/*	3) was downloaded sucessfully! */ 
		
	if(m_loadtriggered==true)
		return;

	/* does this already have a valid image? */
	if(IsValid()==true)
	{
		m_loadtriggered=true;
		return;
	}

	/* check to see if this is in the disk cache? */

	m_url.SetString(url);
	m_dl.SetIfMod(0);

	/* if we are not to load images, then don't bother loading */
	if(m_page->GetSettings()->GetLoadImages()==false)
		return;

	if(m_page->GetItemCache())
	{
		kGUIString fn;
		kGUIString ifmod;
		kGUIString header;

		/* is it in the cache and still valid? */
		if(m_page->GetItemCache()->Find(&m_url,&fn,&header))
		{
			/* load from the disk instead */
			SetFilename(fn.GetString());
			LoadPixels();
			m_loadtriggered=true;
			return;
		}

		/* is it in the cache but expired? if so, let's ask if it is still up to date */
		if(m_page->GetItemCache()->FindExpired(&m_url,&fn,&ifmod))
		{
			/* load it from the cache and then overwrite it if it has changed */
			infile=true;
			SetFilename(fn.GetString());
			LoadPixels();
			m_dl.SetIfMod(&ifmod);
		}
	}

	{
		AsyncLoadInfo ali;

		if(infile==false)
			SetMemory();
		m_loadtriggered=true;
		m_loadaborted=true;			/* this is cleared upon the load being finished */
		m_page->IncLoading();

		m_url.SetString(url);
		m_dl.SetReferer(referrer);

		m_dl.SetAuthHandler(m_page->GetAuthHandler());
		m_dl.SetURL(url);
		CheckWrite();
		kGUI::Trace("Load %08x '%s'\n",this,url->GetString());
		ali.m_dh=this;
		ali.m_dl=&m_dl;
		ali.m_donecallback.Set(this,CALLBACKNAME(LoadFinished));
		m_page->AsyncLoad(&ali,false);
	}
}

void kGUIOnlineImage::LoadFinished(int status)
{
//	kGUI::Trace("Done: %d - %s\n",m_page->m_numloading,m_url.GetString());

	m_page->DecLoading();
	m_loadaborted=false;
	switch(status)
	{
	case DOWNLOAD_OK:
		if(GetSize())	/* if there was an error writing it then this will be zero */
		{
			if(m_page->GetItemCache())
				m_page->GetItemCache()->Add(&m_url,m_dl.GetExpiry(),m_dl.GetLastModified(),this);

			SetMemory((const unsigned char *)GetBufferPtr(),(unsigned long)GetSize());
			/* it get's cleared by the purge in SetMemory so we put it back */
			m_loadtriggered=true;
		}
	break;
	case DOWNLOAD_SAME:
		/* this is trigerred if an if-modified header is sent and the item is the same as before */
		m_page->GetItemCache()->UpdateExpiry(&m_url,m_dl.GetExpiry());
		m_loadtriggered=true;
	break;
	default:
		/* error */
	break;
	}
	m_page->ImageLoaded(this);
}

void kGUIOnlineImageObj::SetURL(kGUIHTMLPageObj *page,kGUIString *url,kGUIString *referrer)
{
	page->AddMedia(url);

	/* already set? */
	if(!strcmp(url->GetString(),m_url.GetString()))
		return;

	m_url.SetString(url);
	/* look for this image in the image cache, if not there then add it */
	SetImage(page->LocateImage(url,referrer));
}

void kGUIUnits::Reset(void)
{
	m_value.i=0;
	m_vint=true;
	m_units=UNITS_UNDEFINED;
}

void kGUIUnits::Set(kGUIHTMLPageObj *page,kGUIString *s)
{
	Hash *uc;
	kGUIUnits *c;

	/* look in the page units hash table to see if this */
	/* string is already there, and if so then just copy the units/value/vint */
	/* and if not then add it for faster processing next time */

	uc=page->GetUnitsCache();
	c=(kGUIUnits *)uc->Find(s->GetString());
	if(c)
	{
		/* copy results from the cache */
		m_units=c->m_units;
		m_value.d=c->m_value.d;
		m_vint=c->m_vint;
		return;
	}

	if(s->GetChar(s->GetLen()-1)=='%')
		m_units=UNITS_PERCENT;
	else if(!strcmpi(s->GetString(),"auto"))
		m_units=UNITS_AUTO;
	else
	{
		/* find the suffix */
		if(strstri(s->GetString(),"em"))
			m_units=UNITS_EM;
		else if(strstri(s->GetString(),"ex"))
			m_units=UNITS_EX;
		else if(strstri(s->GetString(),"px"))
			m_units=UNITS_PIXELS;
		else if(strstri(s->GetString(),"cm"))
			m_units=UNITS_CM;
		else if(strstri(s->GetString(),"mm"))
			m_units=UNITS_MM;
		else if(strstri(s->GetString(),"in"))
			m_units=UNITS_IN;
		else if(strstri(s->GetString(),"pc"))
			m_units=UNITS_PC;
		else if(strstri(s->GetString(),"pt"))
			m_units=UNITS_POINTS;
		else if((s->GetChar(0)>='0' && s->GetChar(0)<='9') || (s->GetChar(0)=='-') || (s->GetChar(0)=='+') || (s->GetChar(0)=='.'))
			m_units=UNITS_PIXELS;
		else
			m_units=UNITS_UNDEFINED;
	}
	/* is the constant an int or a double? */
	if(strstr(s->GetString(),"."))
	{
		m_vint=false;
		m_value.d=s->GetDouble();
	}
	else
	{
		m_vint=true;
		m_value.i=s->GetInt();
	}
	uc->Add(s->GetString(),this);
}

int kGUIUnits::CalcUnitValue(int scale100,int em)
{
	switch(m_units)
	{
	case UNITS_PERCENT:
//		kGUI::Trace("scale=%d,scale100=%d,return=%d\n",m_value,scale100,(int)((m_value*scale100)/100.0f));
		if(m_vint==false)
			return((int)((m_value.d*scale100)/100.0f));
		else
			return((int)((m_value.i*scale100)/100.0f));
	break;
	case UNITS_EM:
		if(m_vint==false)
			return((int)(m_value.d*em));
		else
			return((int)(m_value.i*em));
	break;
	case UNITS_EX:
		if(m_vint==false)
			return((int)(m_value.d*em*0.5f));
		else
			return((int)(m_value.i*em*0.5f));
	break;
	case UNITS_CM:
		return((int)(GetUnitValueD()*(72.0f/2.54f)));
	break;
	case UNITS_MM:
		return((int)(GetUnitValueD()*(72.0f/25.4f)));
	break;
	case UNITS_IN:
		return((int)(GetUnitValueD()*72.0f));
	break;
	case UNITS_PC:
		return((int)(GetUnitValueD()*12.0f));
	break;
	case UNITS_PIXELS:
		return(GetUnitValue());
	break;
	case UNITS_POINTS:
		return(GetUnitValue());
	break;
	case UNITS_AUTO:
		assert(false,"This should be handled externally!");
//		return(scale100);		/* huh? */
	break;
	}

	//todo, add a error print here
//	assert(false,"Unknown type!");
	return(0);
}

double kGUIUnits::CalcUnitValue(double scale100,double em)
{
	switch(m_units)
	{
	case UNITS_PERCENT:
//		kGUI::Trace("scale=%d,scale100=%d,return=%d\n",m_value,scale100,(int)((m_value*scale100)/100.0f));
		return(((GetUnitValueD()*scale100)/100.0f));
	break;
	case UNITS_EM:
		return (GetUnitValueD()*em);
	break;
	case UNITS_EX:
		return (GetUnitValueD()*em*0.5f);
	break;
	case UNITS_PIXELS:
		return(GetUnitValueD());
	break;
	case UNITS_CM:
		return(GetUnitValueD()*(72.0f/2.54f));
	break;
	case UNITS_MM:
		return(GetUnitValueD()*(72.0f/25.4f));
	break;
	case UNITS_IN:
		return(GetUnitValueD()*72.0f);
	break;
	case UNITS_PC:
		return(GetUnitValueD()*12.0f);
	break;
	case UNITS_POINTS:
		return(GetUnitValue());
	break;
	case UNITS_AUTO:
		assert(false,"This should be handled externally!");
//		return(0.0f);		/* huh? */
	break;
	}

	assert(false,"Unknown type!");
	return(0);
}

unsigned int kGUIHTMLObj::GetAlign(void)
{
	int align;
	if(m_box)
	{
		align=m_box->GetMarginAlign();
		if(align!=ALIGN_UNDEFINED)
			return(align);
	}
	return(m_align);
}


/* add style attributes to the style collection */
void kGUIStyleObj::AddAttributes(kGUIHTMLPageObj *page,unsigned int content,unsigned int baseowner,unsigned int priority,kGUIString *s)
{
	kGUIString name,value;
	ATTLIST_DEF **attptr;
	char c;
	int j,l,in;

	j=0;
	l=s->GetLen();
	while(j<l)
	{
		/* get name */
		name.Clear();
		value.Clear();

		do
		{
again:		c=s->GetChar(j++);
			if(c=='/')
			{
				if(s->GetChar(j)=='*')
				{
					/* yes we found a comment */
					++j;
					do
					{
						if(s->GetChar(j)=='*' && s->GetChar(j+1)=='/')
						{
							j+=2;
							goto again;
						}
					}while(++j<l);
					return;				/* unclosed comment */
				}
			}
			if(c==':')
				break;

			/* if we found a ';' before we found a ':' then this must be malformed, skip it */
			if(c==';')
			{
				/* don't report error if only whitespace was found */	
				name.Trim();
				if(name.GetLen())
				{
					page->m_errors.ASprintf("malformed style attribute ('%s')!\n",name.GetString());
					name.Clear();
					--j;
					break;
				}
				else
					goto again;
			}
			name.Append(c);
		}while(j<l);

		/* get value */
		in=0;
		do
		{
vagain:;
			c=s->GetChar(j++);
			if(c=='/')
			{
				if(s->GetChar(j)=='*')
				{
					/* yes we found a comment */
					++j;
					do
					{
						if(s->GetChar(j)=='*' && s->GetChar(j+1)=='/')
						{
							j+=2;
							goto vagain;
						}
					}while(++j<l);
					return;				/* unclosed comment */
				}
			}
			if(c=='(')
				++in;
			else if(c==')')
				--in;
			if(c==';' && !in)
				break;
			value.Append(c);
		}while(j<l);

		/* if any '\xxx ' are inside the string then change them to characters */
		page->FixCodes(&name);
		page->FixCodes(&value);

		/* add name/value to style sheet settings */
		name.Trim();
		value.Trim();
		if(name.GetLen())
		{
			/* get attribute ID */
			attptr=page->FindAtt(&name);
			if(attptr)
				AddAndSplitAttribute(page,content,baseowner,priority,(*attptr)->attid,&value,false);
			else
			{
				/* unknown attribute */
				page->m_errors.ASprintf("unknown style attribute ('%s')!\n",name.GetString());
			}
		}
	}
}

/* collapse space between parms for rgb() and rgba() and hsl() so that splitting on */
/* spaces leaves them grouped together */

void kGUIStyleObj::FixParms(kGUIString *s)
{
	unsigned int index;
	unsigned int nb;
	unsigned int c;
	unsigned int q=0;
	unsigned int l;
	bool inbrackets=false;

	index=0;
	l=s->GetLen();
	while(index<l)
	{
		c=s->GetChar(index,&nb);
		index+=nb;
		if(q && c==q)
			q=0;
		else if(!q && (c=='\"' || c=='\''))
			q=c;
		else if(!q)
		{
			if(!inbrackets && c=='(')
				inbrackets=true;
			else if(inbrackets && c==')')
				inbrackets=false;
			else if(inbrackets && c==' ')
			{
				/* all whitespace has been converted to characters so remove it! */
				s->Delete(index-1,1);
				--l;
			}
		}
	}
}

void kGUIStyleObj::AddAndSplitAttribute(kGUIHTMLPageObj *page,unsigned int content,unsigned int baseowner,unsigned int priority,unsigned int attid,kGUIString *value,bool addempty)
{
	int x,w,numwords,code;
	kGUIStringSplit ss;
	kGUIString *word;
	kGUIString *wt=0;
	kGUIString *wb=0;
	kGUIString *wl=0;
	kGUIString *wr=0;
	kGUIUnits u;
	kGUIHTMLColor c;
	unsigned int owner;
	bool goterr=false;

	ss.SetRemoveQuotes(false);
	/* some places have !imp and others ! imp */
	while(value->Replace("! ","!"));

	if(value->Replace("!important","",0,true))
	{
		/* this is important */
		switch(baseowner)
		{
		case OWNER_USER:
			owner=OWNER_USERIMPORTANT;
		break;
		case OWNER_AUTHOR:
			owner=OWNER_AUTHORIMPORTANT;
		break;
		default:
			owner=baseowner;
			assert(false,"Not a valid owner!");
		break;
		}
	}
	else
		owner=baseowner;

	/* split group commands into their individual commands */
	switch(attid)
	{
	case HTMLATT_COLOR:
	{
		/* check for malformed syntax */
		kGUIHTMLColor color;

		switch(page->GetConstID(value->GetString()))
		{
		case HTMLCONST_INHERIT:
			AddAttribute(attid,owner,priority,value);
		break;
		case HTMLCONST_CURRENTCOLOR:
		{
			kGUIString inherit;

			inherit.SetString("inherit");
			AddAttribute(attid,owner,priority,&inherit);
		}
		break;
		default:
			if(page->GetColor(value,&color))
				AddAttribute(attid,owner,priority,value);
			else
				page->m_errors.ASprintf("Malformed color '%s' for COLOR tag\n",value->GetString());
		break;
		}
	}
	break;
	case HTMLATT_WIDTH:
	case HTMLATT_HEIGHT:
	case HTMLATT_LEFT:
	case HTMLATT_RIGHT:
	case HTMLATT_TOP:
	case HTMLATT_BOTTOM:
	case HTMLATT_MINWIDTH:
	case HTMLATT_MAXWIDTH:
	case HTMLATT_MINHEIGHT:
	case HTMLATT_MAXHEIGHT:
		/* syntax check before adding (so as to not replace a valid one with an invalid one) */
		u.Set(page,value);
		if(u.GetUnitType()==UNITS_UNDEFINED)
			page->m_errors.ASprintf("Malformed units '%s' for ATTID=%d\n",value->GetString(),attid);
		else
			AddAttribute(attid,owner,priority,value);
	break;
	case HTMLATT_CONTENT:
		/* content is a special case, it can only be applied to rules that either have */
		/* before or after in them and not in any other case */
		/* the content variable passed in let's us know what it's status is */
		if(content==HTMLATT_ERROR)
		{
			page->m_errors.ASprintf("Content not allowed to be set in this instance '%s'\n",value->GetString());
		}
		else
		{
			/* content is either HTML_CONTEMT ( for tags ) or HTMLATT_CONTENT_BEFORE or HTMLATT_CONTENT_AFTER for rules */
			AddAttribute(content,owner,priority,value);
		}
	break;
	case HTMLATTGROUP_FONT:
	{
		kGUIString s;
		bool inherit=false;
		bool gotstyle=false;
		bool gotvariant=false;
		bool gotweight=false;
		bool gotsize=false;
		bool gotlineheight=false;
		bool gotfamily=false;;

		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);

			if(strstr(word->GetString(),"/"))
			{
				/* is this fontsize / lineheight?? */
			}
			else
			{
				switch(page->GetConstID(word->GetString()))
				{
				case HTMLCONST_INHERIT:
					/* ignore for now */
				break;
				case HTMLCONST_NORMAL:
					/* ignore for now */
				break;
				case HTMLCONST_ITALIC:
				case HTMLCONST_OBLIQUE:
					if(gotstyle==false)
					{
						AddAttribute(HTMLATT_FONT_STYLE,owner,priority,word);
						gotstyle=true;
					}
					else
					{
						goterr=true;
						page->m_csserrors.ASprintf("Duplicate tag parm '%s' for FONT (style)\n",word->GetString());
					}
				break;
				case HTMLCONST_SMALL_CAPS:
					if(gotvariant==false)
					{
						AddAttribute(HTMLATT_FONT_VARIANT,owner,priority,word);
						gotvariant=true;
					}
					else
					{
						goterr=true;
						page->m_csserrors.ASprintf("Duplicate tag parm '%s' for FONT (variant)\n",word->GetString());
					}
				break;
				case HTMLCONST_BOLD:		/* weight */
				case HTMLCONST_BOLDER:		/* weight */
				case HTMLCONST_LIGHTER:		/* weight */
	isweight:		if(gotweight==false)
					{
						AddAttribute(HTMLATT_FONT_WEIGHT,owner,priority,word);
						gotweight=true;
					}
					else
					{
						goterr=true;
						page->m_csserrors.ASprintf("Duplicate tag parm '%s' for FONT (weight)\n",word->GetString());
					}
				break;

					// 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit

					inherit=true;
				break;
				case HTMLCONST_XXSMALL:
				case HTMLCONST_XSMALL:
				case HTMLCONST_SMALL:
				case HTMLCONST_MEDIUM:
				case HTMLCONST_LARGE:
				case HTMLCONST_XLARGE:
				case HTMLCONST_XXLARGE:
				case HTMLCONST_SMALLER:
				case HTMLCONST_LARGER:
					if(gotsize==false)
					{
						AddAttribute(HTMLATT_FONT_SIZE,owner,priority,word);
						gotsize=true;
					}
					else
					{
						goterr=true;
						page->m_csserrors.ASprintf("Duplicate tag parm '%s' for FONT (size)\n",word->GetString());
					}

				break;
				case HTMLCONST_CAPTION:
				case HTMLCONST_ICON:
				case HTMLCONST_MENU:
				case HTMLCONST_MESSAGE_BOX:
				case HTMLCONST_SMALL_CAPTION:
				case HTMLCONST_STATUS_BAR:
					/* todo */
				break;
				default:
					u.Set(page,word);

					/* 100,200,300,400,500,600,700,800,900 */
					switch(u.GetUnitType())
					{
					case UNITS_PIXELS:
						goto isweight;
					break;
					case UNITS_POINTS:
					break;
					default:
						/* what is this?? */
					break;
					}
				break;
				}
			}
		}

		if(gotstyle==false)
		{
			s.SetString("normal");
			AddAttribute(HTMLATT_FONT_STYLE,owner,priority,&s);
		}

		if(gotvariant==false)
		{
			s.SetString("normal");
			AddAttribute(HTMLATT_FONT_VARIANT,owner,priority,&s);
		}

		if(gotweight==false)
		{
			s.SetString("normal");
			AddAttribute(HTMLATT_FONT_WEIGHT,owner,priority,&s);
		}

		if(gotsize==false)
		{
			s.SetString("medium");
			AddAttribute(HTMLATT_FONT_SIZE,owner,priority,&s);
		}

		if(gotlineheight==false)
		{
			s.SetString("1.2em");
			AddAttribute(HTMLATT_LINE_HEIGHT,owner,priority,&s);
		}

		if(gotfamily==false)
		{
			/* leave */
		}
	}
	break;
	case HTMLATTGROUP_OVERFLOW:
		/* hmmm, can there be more than one value? */
		AddAttribute(HTMLATT_OVERFLOW_X,owner,priority,value);
		AddAttribute(HTMLATT_OVERFLOW_Y,owner,priority,value);
	break;
	case HTMLATT_HSPACE:
		AddAttribute(HTMLATT_MARGIN_LEFT,owner,priority,value);
		AddAttribute(HTMLATT_MARGIN_RIGHT,owner,priority,value);
	break;
	case HTMLATT_VSPACE:
		AddAttribute(HTMLATT_MARGIN_TOP,owner,priority,value);
		AddAttribute(HTMLATT_MARGIN_BOTTOM,owner,priority,value);
	break;
	case HTMLATT_TEXT_DECORATION:
	{
		kGUIString v;

		/* split string and generate a single bitfield value for text-decoration */
		x=TEXTDECORATION_NONE;
		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			switch(page->GetConstID(word->GetString()))
			{
			case HTMLCONST_NONE:
				x=TEXTDECORATION_NONE;
			break;
			case HTMLCONST_UNDERLINE:
				x|=TEXTDECORATION_UNDERLINE;
			break;
			case HTMLCONST_OVERLINE:
				x|=TEXTDECORATION_OVERLINE;
			break;
			case HTMLCONST_LINETHROUGH:
				x|=TEXTDECORATION_LINETHROUGH;
			break;
			case HTMLCONST_BLINK:
				x|=TEXTDECORATION_BLINK;
			break;
			default:
				goterr=true;
				page->m_csserrors.ASprintf("Unknown tag parm '%s' for TEXT_DECORATION\n",value->GetString());
			break;
			}
		}
		if(!goterr)
		{
			v.Sprintf("%d",x);						/* att no longer has "string" now it just has a bitfield value */
			AddAttribute(HTMLATT_TEXT_DECORATION,owner,priority,&v);
		}
	}
	break;
	case HTMLATTGROUP_LIST_STYLE:
	{
		kGUIString type;
		kGUIString position;
		kGUIString url;
		bool goterr=false;

		type.SetString("none");
		position.SetString("outside");

		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			switch(page->GetConstID(word->GetString()))
			{
			case HTMLCONST_NONE:	//none  	No marker
			case HTMLCONST_DISC:	//disc 	Default. The marker is a filled circle
			case HTMLCONST_CIRCLE:	//circle 	The marker is a circle
			case HTMLCONST_SQUARE:	//square 	The marker is a square
			case HTMLCONST_DECIMAL:	//decimal 	The marker is a number
			case HTMLCONST_DECIMAL_LEADING_ZERO:	//decimal-leading-zero 	The marker is a number padded by initial zeros (01, 02, 03, etc.)
			case HTMLCONST_LOWER_ROMAN:	//lower-roman 	The marker is lower-roman (i, ii, iii, iv, v, etc.)
			case HTMLCONST_UPPER_ROMAN:	//upper-roman 	The marker is upper-roman (I, II, III, IV, V, etc.)
			case HTMLCONST_LOWER_ALPHA:	//lower-alpha 	The marker is lower-alpha (a, b, c, d, e, etc.)
			case HTMLCONST_LOWER_LATIN:	//lower-latin 	The marker is lower-latin (a, b, c, d, e, etc.)
			case HTMLCONST_UPPER_ALPHA:	//upper-alpha 	The marker is upper-alpha (A, B, C, D, E, etc.) 
			case HTMLCONST_UPPER_LATIN:	//upper-latin 	The marker is upper-latin (A, B, C, D, E, etc.)
			case HTMLCONST_LOWER_GREEK:	//lower-greek 	The marker is lower-greek (alpha, beta, gamma, etc.)
				type.SetString(word);
			break;
			case HTMLCONST_INSIDE:
			case HTMLCONST_OUTSIDE:
				position.SetString(word);
			break;
			default:
				/* must be URL a? */
				if(!strcmpin(word->GetString(),"url(",4) || strstr(word->GetString(),"."))
					url.SetString(word);
				else
				{
					goterr=true;
					page->m_csserrors.ASprintf("Unknown tag parm '%s' for LIST_STYLE\n",value->GetString());
				}
			break;
			}
		}
		if(!goterr)
		{
			if(url.GetLen())
				AddAttribute(HTMLATT_LIST_STYLE_IMAGE,owner,priority,&url);
			else
				AddAttribute(HTMLATT_LIST_STYLE_TYPE,owner,priority,&type);
			AddAttribute(HTMLATT_LIST_STYLE_POSITION,owner,priority,&position);
		}
	}
	break;
	case HTMLATTGROUP_TEXT_SHADOW:
	{
		kGUIHTMLColor c;
		unsigned int fieldindex;
		int gotcolor;
		bool gotnone=false;
		static int shadowfields[]={HTMLATT_TEXT_SHADOW_X,HTMLATT_TEXT_SHADOW_Y,HTMLATT_TEXT_SHADOW_R};

		goterr=true;
		//text-shadow: #6374AB 20px -12px 2px;
		// [none] color, xoffset, yoffset and blur radius

		fieldindex=0;
		gotcolor=0;
		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			switch(page->GetConstID(word->GetString()))
			{
			case HTMLCONST_NONE:
				gotnone=true;
				word->SetString("-1");						/* we will use a 0 radius to signify "not enabled" */
				AddAttribute(HTMLATT_TEXT_SHADOW_R,owner,priority,word);
			break;
			default:
				if(page->GetColor(word,&c)==true)
				{
					AddAttribute(HTMLATT_TEXT_SHADOW_COLOR,owner,priority,word);
					++gotcolor;
				}
				else
				{
					if(fieldindex<(sizeof(shadowfields)/sizeof(int)))
						AddAttribute(shadowfields[fieldindex++],owner,priority,word);
					else
					{
						//error too many fields
						page->m_csserrors.ASprintf("Too many fields/parms '%s' for text-shadow\n",value->GetString());
					}
				}
			break;
			}
		}
		if(gotnone==false && fieldindex<3)
		{
			kGUIString r;

			r.SetString("1");						/* we will use a 1 radius to signify "enabled" */
			AddAttribute(HTMLATT_TEXT_SHADOW_R,owner,priority,&r);
		}
	}
	break;
	case HTMLATTGROUP_BACKGROUND_POSITION:
	{
		kGUIString center;
		bool gotx=false,goty=false,gotcenter=false;

		center.SetString("Center");

		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			switch(page->GetConstID(word->GetString()))
			{
			case HTMLCONST_LEFT:
			case HTMLCONST_RIGHT:
				if(gotcenter==true)
				{
					AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,&center);
					gotcenter=false;
					goty=true;
				}
				AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,word);
				gotx=true;
			break;
			case HTMLCONST_TOP:
			case HTMLCONST_BOTTOM:
				if(gotcenter==true)
				{
					AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,&center);
					gotcenter=false;
					gotx=true;
				}
				AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,word);
				goty=true;
			break;
			case HTMLCONST_CENTER:
				gotcenter=true;
			break;
			default:
				u.Set(page,word);
				if(u.GetUnitType()!=UNITS_UNDEFINED)
				{
					if(gotx==false)
					{
						AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,word);
						gotx=true;
					}
					else if(goty==false)
					{
						AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,word);
						goty=true;
					}
					else
						page->m_csserrors.ASprintf("Unknown tag parm '%s' for BACKGROUND\n",word->GetString());
				}
				else
					page->m_csserrors.ASprintf("Unknown tag parm '%s' for BACKGROUND\n",word->GetString());
			break;
			}
		}
		if(gotx==false)
			AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,&center);
		if(goty==false)
			AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,&center);
	}
	break;
	case HTMLATT_BACKGROUND:
	{
		kGUIString url;
		kGUIString fn;
		kGUIString center;
		bool inherit=false;
		bool gotx=false,goty=false,gotcenter=false,gotcolor=false,gotattachment=false,gotrepeat=false,gotimage=false;

		center.SetString("Center");

		FixParms(value);
		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			switch(page->GetConstID(word->GetString()))
			{
			case HTMLCONST_INHERIT:
				inherit=true;			/* inherit all undefined values */
			break;
			case HTMLCONST_CURRENTCOLOR:
				gotcolor=true;
				AddAttribute(HTMLATT_BACKGROUND_COLOR,owner,priority,word);
			break;
			case HTMLCONST_NONE:
			{
				//kGUIString w;
				//w.SetString("transparent");
				//AddAttribute(HTMLATT_BACKGROUND_COLOR,owner,priority,&w);
				//gotcolor=true;
				//w.SetString("none");
				gotimage=true;
				AddAttribute(HTMLATT_BACKGROUND_IMAGE,owner,priority,word);
			}
			break;
			case HTMLCONST_FIXED:
			case HTMLCONST_SCROLL:
				gotattachment=true;
				AddAttribute(HTMLATT_BACKGROUND_ATTACHMENT,owner,priority,word);
			break;
			case HTMLCONST_NOREPEAT:
			case HTMLCONST_REPEAT:
			case HTMLCONST_REPEATX:
			case HTMLCONST_REPEATY:
				gotrepeat=true;
				AddAttribute(HTMLATT_BACKGROUND_REPEAT,owner,priority,word);
			break;
			case HTMLCONST_LEFT:
			case HTMLCONST_RIGHT:
				if(gotcenter==true)
				{
					AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,&center);
					gotcenter=false;
					goty=true;
				}
				AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,word);
				gotx=true;
			break;
			case HTMLCONST_TOP:
			case HTMLCONST_BOTTOM:
				if(gotcenter==true)
				{
					AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,&center);
					gotcenter=false;
					gotx=true;
				}
				AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,word);
				goty=true;
			break;
			case HTMLCONST_CENTER:
				gotcenter=true;
			break;
			default:
			{
				kGUIHTMLColor c;

				if(page->GetColor(word,&c)==true)
				{
					AddAttribute(HTMLATT_BACKGROUND_COLOR,owner,priority,word);
					gotcolor=true;
				}
				else
				{
					if(!strcmpin(word->GetString(),"url(",4) || strstr(word->GetString(),"."))
					{
						gotimage=true;
						AddAttribute(HTMLATT_BACKGROUND_IMAGE,owner,priority,word);
					}
					else
					{
						u.Set(page,word);
						if(u.GetUnitType()!=UNITS_UNDEFINED)
						{
							if(gotx==false)
							{
								AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,word);
								gotx=true;
							}
							else if(goty==false)
							{
								AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,word);
								goty=true;
							}
							else
								page->m_csserrors.ASprintf("Unknown tag parm '%s' for BACKGROUND\n",word->GetString());
						}
						else
							page->m_csserrors.ASprintf("Unknown tag parm '%s' for BACKGROUND\n",word->GetString());
					}
				}
			}
			break;
			}
		}

		if(gotcenter==true)
		{
			if(gotx==false)
			{
				AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,&center);
				gotx=true;
			}
			if(goty==false)
			{
				AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,&center);
				goty=true;
			}
		}

		if(inherit)
		{
			kGUIString inherit;

			/* inherit all undefined fields */
			inherit.SetString("inherit");
			if(gotattachment==false)
				AddAttribute(HTMLATT_BACKGROUND_ATTACHMENT,owner,priority,&inherit);
			if(gotrepeat==false)
				AddAttribute(HTMLATT_BACKGROUND_REPEAT,owner,priority,&inherit);
			if(goty==false)
				AddAttribute(HTMLATT_BACKGROUND_POSITIONY,owner,priority,&inherit);
			if(gotx==false)
				AddAttribute(HTMLATT_BACKGROUND_POSITIONX,owner,priority,&inherit);
			if(gotcolor==false)
				AddAttribute(HTMLATT_BACKGROUND_COLOR,owner,priority,&inherit);
			if(gotimage==false)
				AddAttribute(HTMLATT_BACKGROUND_IMAGE,owner,priority,&inherit);
		}
#if 0
		else if(gotcolor==false)
		{
			/* make color transparent */
			kGUIString w;

			w.SetString("transparent");
			AddAttribute(HTMLATT_BACKGROUND_COLOR,owner,priority,&w);
		}
#endif
	}
	break;
	case HTMLATT_MARGIN:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			wt=wb=wl=wr=ss.GetWord(0);
		break;
		case 2:
			wt=wb=ss.GetWord(0);
			wl=wr=ss.GetWord(1);
		break;
		case 3:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wl=wb=ss.GetWord(2);
		break;
		case 4:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=ss.GetWord(2);
			wl=ss.GetWord(3);
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("margin, bad # keywords %d, for MARGIN '%s'\n",numwords,value->GetString());
		break;
		}
		if(wt)
		{
			AddAttribute(HTMLATT_MARGIN_TOP,owner,priority,wt);
			AddAttribute(HTMLATT_MARGIN_BOTTOM,owner,priority,wb);
			AddAttribute(HTMLATT_MARGIN_LEFT,owner,priority,wl);
			AddAttribute(HTMLATT_MARGIN_RIGHT,owner,priority,wr);
		}
	break;
	case HTMLATTGROUP_PADDING:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			wt=wb=wl=wr=ss.GetWord(0);
		break;
		case 2:
			wt=wb=ss.GetWord(0);
			wl=wr=ss.GetWord(1);
		break;
		case 3:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=wl=ss.GetWord(2);
		break;
		case 4:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=ss.GetWord(2);
			wl=ss.GetWord(3);
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("padding, bad # keywords %d, for PADDING '%s'\n",numwords,value->GetString());
		break;
		}
		if(wt)
		{
			AddAttribute(HTMLATT_PADDING_TOP,owner,priority,wt);
			AddAttribute(HTMLATT_PADDING_BOTTOM,owner,priority,wb);
			AddAttribute(HTMLATT_PADDING_LEFT,owner,priority,wl);
			AddAttribute(HTMLATT_PADDING_RIGHT,owner,priority,wr);
		}
	break;
	case HTMLATTGROUP_BORDER_STYLE:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			wt=wb=wl=wr=ss.GetWord(0);
		break;
		case 2:
			wt=wb=ss.GetWord(0);
			wl=wr=ss.GetWord(1);
		break;
		case 3:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wl=wb=ss.GetWord(2);
		break;
		case 4:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=ss.GetWord(2);
			wl=ss.GetWord(3);
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("border-style, bad # keywords %d, '%s'\n",numwords,value->GetString());
		break;
		}
		if(wt)
		{
			AddAttribute(HTMLATT_BORDER_STYLE_TOP,owner,priority,wt);
			AddAttribute(HTMLATT_BORDER_STYLE_LEFT,owner,priority,wl);
			AddAttribute(HTMLATT_BORDER_STYLE_RIGHT,owner,priority,wr);
			AddAttribute(HTMLATT_BORDER_STYLE_BOTTOM,owner,priority,wb);
		}
	break;
	case HTMLATTGROUP_BORDER_COLOR:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			wt=wb=wl=wr=ss.GetWord(0);
		break;
		case 2:
			wt=wb=ss.GetWord(0);
			wl=wr=ss.GetWord(1);
		break;
		case 3:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wl=wb=ss.GetWord(2);
		break;
		case 4:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=ss.GetWord(2);
			wl=ss.GetWord(3);
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("border-color, bad # keywords %d, '%s'\n",numwords,value->GetString());
		break;
		}
		if(wt)
		{
			AddAttribute(HTMLATT_BORDER_COLOR_TOP,owner,priority,wt);
			AddAttribute(HTMLATT_BORDER_COLOR_LEFT,owner,priority,wl);
			AddAttribute(HTMLATT_BORDER_COLOR_RIGHT,owner,priority,wr);
			AddAttribute(HTMLATT_BORDER_COLOR_BOTTOM,owner,priority,wb);
		}
	break;
	case HTMLATTGROUP_BORDER_TOP:
		x=BORDER_TOP;
		goto setborder;
	break;
	case HTMLATTGROUP_BORDER_BOTTOM:
		x=BORDER_BOTTOM;
		goto setborder;
	break;
	case HTMLATTGROUP_BORDER_LEFT:
		x=BORDER_LEFT;
		goto setborder;
	break;
	case HTMLATTGROUP_BORDER_RIGHT:
		x=BORDER_RIGHT;
		goto setborder;
	break;
	case HTMLATTGROUP_BORDER:
		x=BORDER_ALL;
setborder:;
	{
		int gotcolor=0;
		int gotstyle=0;
		int gotsize=0;
		kGUIString color;
		kGUIString style;
		kGUIString size;

		//color.SetString("black");
		//size.SetString("medium");

		numwords=ss.Split(value," ");
		if(numwords==1 && x==BORDER_ALL && page->DigOnly(value))
		{
			/* tbis is the old style for tables "border=1" (etc) */
			AddAttribute(HTMLATT_BORDER_WIDTH,owner,priority,value);
			if(value->GetInt()>0)
			{
				style.SetString("solid");
				AddAttribute(HTMLATT_BORDER_STYLE_LEFT,owner,priority,&style);
				AddAttribute(HTMLATT_BORDER_STYLE_RIGHT,owner,priority,&style);
				AddAttribute(HTMLATT_BORDER_STYLE_TOP,owner,priority,&style);
				AddAttribute(HTMLATT_BORDER_STYLE_BOTTOM,owner,priority,&style);
			}
		}
		else
		{
			for(w=0;w<numwords;++w)
			{
				word=ss.GetWord(w);
				code=page->GetConstID(word->GetString());
				switch(code)
				{
				case HTMLCONST_NONE:
				case HTMLCONST_HIDDEN:
				case HTMLCONST_DOTTED:
				case HTMLCONST_DASHED:
				case HTMLCONST_SOLID:
				case HTMLCONST_DOUBLE:
				case HTMLCONST_GROOVE:
				case HTMLCONST_RIDGE:
				case HTMLCONST_INSET:
				case HTMLCONST_OUTSET:
					++gotstyle;
					style.SetString(word);
				break;
				case HTMLCONST_THIN:
				case HTMLCONST_MEDIUM:
				case HTMLCONST_THICK:
				case HTMLCONST_LENGTH:
					++gotsize;
					size.SetString(word);
				break;
				default:
					/* is this a unit measure? */
					u.Set(page,word);
					if(u.GetUnitType()!=UNITS_UNDEFINED)
					{
						/* must be a size */
						++gotsize;
						size.SetString(word);
					}
					else
					{
						/* must be a color */
						if(page->GetColor(word,&c))
						{
							++gotcolor;
							color.SetString(word);
						}
					}
				break;
				}
			}
			/* was this all valid? */
			if(gotstyle<2 && gotcolor<2 && gotsize<2)
			{
				switch(x)
				{
				case BORDER_LEFT:
					if(gotstyle)
						AddAttribute(HTMLATT_BORDER_STYLE_LEFT,owner,priority,&style);
					if(gotsize)
						AddAttribute(HTMLATT_BORDER_WIDTH_LEFT,owner,priority,&size);
					if(gotcolor)
						AddAttribute(HTMLATT_BORDER_COLOR_LEFT,owner,priority,&color);
				break;
				case BORDER_RIGHT:
					if(gotstyle)
						AddAttribute(HTMLATT_BORDER_STYLE_RIGHT,owner,priority,&style);
					if(gotsize)
						AddAttribute(HTMLATT_BORDER_WIDTH_RIGHT,owner,priority,&size);
					if(gotcolor)
						AddAttribute(HTMLATT_BORDER_COLOR_RIGHT,owner,priority,&color);
				break;
				case BORDER_TOP:
					if(gotstyle)
						AddAttribute(HTMLATT_BORDER_STYLE_TOP,owner,priority,&style);
					if(gotsize)
						AddAttribute(HTMLATT_BORDER_WIDTH_TOP,owner,priority,&size);
					if(gotcolor)
						AddAttribute(HTMLATT_BORDER_COLOR_TOP,owner,priority,&color);
				break;
				case BORDER_BOTTOM:
					if(gotstyle)
						AddAttribute(HTMLATT_BORDER_STYLE_BOTTOM,owner,priority,&style);
					if(gotsize)
						AddAttribute(HTMLATT_BORDER_WIDTH_BOTTOM,owner,priority,&size);
					if(gotcolor)
						AddAttribute(HTMLATT_BORDER_COLOR_BOTTOM,owner,priority,&color);
				break;
				case BORDER_ALL:
					if(gotstyle)
					{
						AddAttribute(HTMLATT_BORDER_STYLE_TOP,owner,priority,&style);
						AddAttribute(HTMLATT_BORDER_STYLE_BOTTOM,owner,priority,&style);
						AddAttribute(HTMLATT_BORDER_STYLE_LEFT,owner,priority,&style);
						AddAttribute(HTMLATT_BORDER_STYLE_RIGHT,owner,priority,&style);
					}
					if(gotsize)
					{
						AddAttribute(HTMLATT_BORDER_WIDTH_TOP,owner,priority,&size);
						AddAttribute(HTMLATT_BORDER_WIDTH_BOTTOM,owner,priority,&size);
						AddAttribute(HTMLATT_BORDER_WIDTH_LEFT,owner,priority,&size);
						AddAttribute(HTMLATT_BORDER_WIDTH_RIGHT,owner,priority,&size);
					}
					if(gotcolor)
					{
						AddAttribute(HTMLATT_BORDER_COLOR_TOP,owner,priority,&color);
						AddAttribute(HTMLATT_BORDER_COLOR_BOTTOM,owner,priority,&color);
						AddAttribute(HTMLATT_BORDER_COLOR_LEFT,owner,priority,&color);
						AddAttribute(HTMLATT_BORDER_COLOR_RIGHT,owner,priority,&color);
					}
				break;
				}
			}
		}
		}
	break;
	case HTMLATTGROUP_BORDER_WIDTH:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			wl=wr=wt=wb=ss.GetWord(0);
		break;
		case 2:
			wt=wb=ss.GetWord(0);
			wl=wr=ss.GetWord(1);
		break;
		case 3:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wl=wb=ss.GetWord(2);
		break;
		case 4:
			wt=ss.GetWord(0);
			wr=ss.GetWord(1);
			wb=ss.GetWord(2);
			wl=ss.GetWord(3);
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("border-width, bad # keywords %d, '%s'\n",numwords,value->GetString());
		break;
		}
		if(wt)
		{
			AddAttribute(HTMLATT_BORDER_WIDTH_TOP,owner,priority,wt);
			AddAttribute(HTMLATT_BORDER_WIDTH_LEFT,owner,priority,wl);
			AddAttribute(HTMLATT_BORDER_WIDTH_RIGHT,owner,priority,wr);
			AddAttribute(HTMLATT_BORDER_WIDTH_BOTTOM,owner,priority,wb);
		}
	break;
	case HTMLATTGROUP_BORDER_SPACING:
		numwords=ss.Split(value," ");
		switch(numwords)
		{
		case 1:
			AddAttribute(HTMLATT_BORDER_SPACING_HORIZ,owner,priority,ss.GetWord(0));
			AddAttribute(HTMLATT_BORDER_SPACING_VERT,owner,priority,ss.GetWord(0));
		break;
		case 2:
			AddAttribute(HTMLATT_BORDER_SPACING_HORIZ,owner,priority,ss.GetWord(0));
			AddAttribute(HTMLATT_BORDER_SPACING_VERT,owner,priority,ss.GetWord(1));
		break;
		default:
			wt=0;
			page->m_csserrors.ASprintf("BORDER_SPACING, bad # keywords %d, '%s'\n",numwords,value->GetString());
		break;
		}
	break;
	case HTMLATTGROUP_OUTLINE:
	{
		int gotcolor=0;
		int gotstyle=0;
		int gotsize=0;
		kGUIString color;
		kGUIString style;
		kGUIString size;

		color.SetString("invert");
		size.SetString("medium");

		numwords=ss.Split(value," ");

		for(w=0;w<numwords;++w)
		{
			word=ss.GetWord(w);
			code=page->GetConstID(word->GetString());
			switch(code)
			{
			case HTMLCONST_NONE:
			case HTMLCONST_HIDDEN:
			case HTMLCONST_DOTTED:
			case HTMLCONST_DASHED:
			case HTMLCONST_SOLID:
			case HTMLCONST_DOUBLE:
			case HTMLCONST_GROOVE:
			case HTMLCONST_RIDGE:
			case HTMLCONST_INSET:
			case HTMLCONST_OUTSET:
				++gotstyle;
				style.SetString(word);
			break;
			case HTMLCONST_THIN:
			case HTMLCONST_MEDIUM:
			case HTMLCONST_THICK:
			case HTMLCONST_LENGTH:
				++gotsize;
				size.SetString(word);
			break;
			default:
				/* is this a unit measure? */
				u.Set(page,word);
				if(u.GetUnitType()!=UNITS_UNDEFINED)
				{
					/* must be a size */
					++gotsize;
					size.SetString(word);
				}
				else
				{
					/* must be a color */
					if(page->GetColor(word,&c))
					{
						++gotcolor;
						color.SetString(word);
					}
				}
			break;
			}
			/* was this all valid? */
			if(gotstyle<2 && gotcolor<2 && gotsize<2)
			{
				if(style.GetLen())
					AddAttribute(HTMLATT_OUTLINE_STYLE,owner,priority,&style);
				if(size.GetLen())
					AddAttribute(HTMLATT_OUTLINE_WIDTH,owner,priority,&size);
				if(color.GetLen())
					AddAttribute(HTMLATT_OUTLINE_COLOR,owner,priority,&color);
				}
			}
		}
	break;
	default:
		/* these are already individual attributes, so add them as is */

		if((attid<HTMLATT_UNKNOWN) && (attid!=HTMLATT_HREF) && (attid!=HTMLATT_FONT_FAMILY))
		{
			/* they should only have 1 attribute, so if they have more than 1 then print an error message */
		
			/* todo: parse text color and convert to standard format for speed */
			if(strnicmp(value->GetString(),"rgb",3))
			{
				numwords=ss.Split(value," ");
				if(numwords>1)
				{
					page->m_csserrors.ASprintf("Attribute should only have 1 word!, bad # keywords %d, '%s'\n",numwords,value->GetString());

					if(addempty==true)
						AddAttributez(attid,owner,priority,ss.GetWord(0));
					else
						AddAttribute(attid,owner,priority,ss.GetWord(0));
					return;
				}
			}
		}
		if(addempty==true)
			AddAttributez(attid,owner,priority,value);
		else
			AddAttribute(attid,owner,priority,value);
	break;
	}
}

/* don't add attributes with no value */
void kGUIStyleObj::AddAttribute(unsigned int attid,unsigned int owner,unsigned int priority,kGUIString *value)
{
	if(value->GetLen())
		AddAttributez(attid,owner,priority,value);
}

/* add style attributes to the style collection, check for duplicate */
/* allow null values */
void kGUIStyleObj::AddAttributez(unsigned int attid,unsigned int owner,unsigned int priority,kGUIString *value)
{
	unsigned int j;
	kGUIHTMLAttrib *cur;

	for(j=0;j<m_numattribs;++j)
	{
		cur=m_attributes.GetEntry(j);
		if(attid==cur->GetID())
		{
			if(owner>cur->GetOwner())
			{
				/* attribute already exists so just replace the value */
				cur->SetOwner(owner);
				cur->SetPriority(priority);
				cur->SetValue(value);
			}
			else if(owner==cur->GetOwner())
			{
				if(priority>=cur->GetPriority())
				{
					cur->SetPriority(priority);
					cur->SetValue(value);
				}
			}
			return;
		}
	}

	/* was not found so generate a new one and add it to the list */
	cur=new kGUIHTMLAttrib();
	cur->SetID(attid);
	cur->SetOwner(owner);
	cur->SetPriority(priority);
	cur->SetValue(value);
	m_attributes.SetEntry(m_numattribs++,cur);
}

kGUIHTMLAttrib *kGUIStyleObj::FindAttrib(unsigned int id)
{
	unsigned int i;
	kGUIHTMLAttrib *att;

	for(i=0;i<m_numattribs;++i)
	{
		att=m_attributes.GetEntry(i);
		if(att->GetID()==id)
			return(att);
	}
	return(0);
}

void kGUIHTMLPageObj::FixCodes(kGUIString *s)
{
	unsigned int sindex;
	unsigned int eindex;
	const char *cp;
	char c;
	int v;
	int numd;

	//remove any ending slash followed by c/r l/f
	s->Replace("\\\r\n","");

	/* change '\xxx' to character, also eat and trailing spaces */

	sindex=0;
	while((cp=strstr(s->GetString()+sindex,"\\")))
	{
		sindex=(int)(cp-s->GetString());
		eindex=sindex+1;
		numd=0;
		v=0;
		while(eindex<s->GetLen())
		{
			c=s->GetChar(eindex);
			if((c>='0' && c<='9') && (v<16))
			{
				++eindex;
				++numd;
				v=(v<<4)|(c-'0');
			}
			else if((c>='a' && c<='f') && (v<16))
			{
				++eindex;
				++numd;
				v=(v<<4)|(10+(c-'a'));
			}
			else if((c>='A' && c<='F') && (v<16))
			{
				++eindex;
				++numd;
				v=(v<<4)|(10+(c-'A'));
			}
			else if(c==' ' && numd)
				++eindex;
			else
				break;
		}
		if(numd)
		{
			s->SetChar(sindex,v);
			s->Delete(sindex+1,eindex-(sindex+1));
			++sindex;
		}
		else
		{
			/* delete the '\' char */
//			s->Delete(sindex,1);
			++sindex;	/* skip the '/' */
		}
	}
}

/* true = only contains digits 0-9 */

bool kGUIHTMLPageObj::DigOnly(kGUIString *s)
{
	unsigned int i,n;
	unsigned int nb;
	int c;

	n=s->GetLen();
	if(!n)
		return(false);
	i=0;
	while(i<n)
	{
		c=s->GetChar(i,&nb);
		if((c<'0') || (c>'9'))
			return(false);
		i+=nb;
	}
	return(true);
}

bool kGUIHTMLPageObj::DigOnlyP(kGUIString *s)
{
	unsigned int i,n;
	unsigned int nb;
	int c;

	n=s->GetLen();
	if(!n)
		return(false);
	i=0;
	while(i<n)
	{
		c=s->GetChar(i,&nb);
		switch(c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
		case '.':
			/* ok */
		break;
		default:
			return(false);
		break;
		}
		i+=nb;
	}
	return(true);
}


/* true=string is only digits, period or percent, false=other char found */
bool kGUIHTMLPageObj::DigOnlyPP(kGUIString *s,bool *percent)
{
	unsigned int i,n;
	unsigned int nb;
	int c;
	bool gotp;

	n=s->GetLen();
	if(!n)
		return(false);
	gotp=false;
	i=0;
	while(i<n)
	{
		c=s->GetChar(i,&nb);

		if(c=='%' && i==n-1)		/* last char can be '%' */
		{
			*(percent)=true;
			return(true);
		}
		if(c=='.')
			gotp=true;
		else if((c<'0' || c>'9') && (c!='-'))
			return(false);
		i+=nb;
	}

	if(gotp)	/* if we got a '.' but no percent at end then error */
		return(false);
	*(percent)=false;
	return(true);
}

bool kGUIHTMLPageObj::GetColor(kGUIString *col,kGUIHTMLColor *c)
{
	int cid;
	const char *cp;
	int r,g,b;
	bool gothash;
	kGUIColor *cc;
	kGUIColor kc;

	cc=(kGUIColor *)m_colorhash.Find(col->GetString());
	if(cc)
	{
		c->Set(*(cc));
		return(true);
	}

	cp=col->GetString();
	if(!strcmpin(cp,"rgb(",4))
	{
		int num;

		kGUIString s;
		kGUIStringSplit ss;
		bool pr=false,pg=false,pb=false;

		s.SetString(col->GetString()+4);
		s.Clip(s.GetLen()-1);
		num=ss.Split(&s,",");
		if(num==3)
		{
			if(DigOnlyPP(ss.GetWord(0),&pr)==false || DigOnlyPP(ss.GetWord(1),&pg)==false || DigOnlyPP(ss.GetWord(2),&pb)==false)
			{
				/* an invalid charcter was found! */
				if(m_strict)
					return(false);
			}
			if(pr!=pg || pr!=pb || pg!=pb)
			{
				/* either all 3 are percent or none are! */
				if(m_strict)
					return(false);
			}

			r=ss.GetWord(0)->GetInt();

			if(pr==true)		/* if this a percent? */
				r=(int)(r*(255.0f/100.0f));
			if(r<0)
				r=0;
			else if(r>255)
				r=255;

			g=ss.GetWord(1)->GetInt();
			if(pg==true)		/* if this a percent? */
				g=(int)(g*(255.0f/100.0f));
			if(g<0)
				g=0;
			else if(g>255)
				g=255;

			b=ss.GetWord(2)->GetInt();
			if(pb==true)		/* if this a percent? */
				b=(int)(b*(255.0f/100.0f));
			if(b<0)
				b=0;
			else if(b>255)
				b=255;

			kc=DrawColorA(r,g,b,255);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
		return(false);
	}

	cp=col->GetString();
	if(!strcmpin(cp,"rgba(",5))
	{
		int num;
		double a;
		kGUIString s;
		kGUIStringSplit ss;
		bool pr=false,pg=false,pb=false;

		s.SetString(col->GetString()+5);
		s.Clip(s.GetLen()-1);
		num=ss.Split(&s,",");
		if(num==4)
		{
			if(DigOnlyPP(ss.GetWord(0),&pr)==false || DigOnlyPP(ss.GetWord(1),&pg)==false || DigOnlyPP(ss.GetWord(2),&pb)==false || DigOnlyP(ss.GetWord(3))==false)
			{
				/* an invalid charcter was found! */
				if(m_strict)
					return(false);
			}
			if(pr!=pg || pr!=pb || pg!=pb)
			{
				/* either all 3 are percent or none are! */
				if(m_strict)
					return(false);
			}

			r=ss.GetWord(0)->GetInt();

			if(pr==true)		/* if this a percent? */
				r=(int)(r*(255.0f/100.0f));
			if(r<0)
				r=0;
			else if(r>255)
				r=255;

			g=ss.GetWord(1)->GetInt();
			if(pg==true)		/* if this a percent? */
				g=(int)(g*(255.0f/100.0f));
			if(g<0)
				g=0;
			else if(g>255)
				g=255;

			b=ss.GetWord(2)->GetInt();
			if(pb==true)		/* if this a percent? */
				b=(int)(b*(255.0f/100.0f));
			if(b<0)
				b=0;
			else if(b>255)
				b=255;

			a=ss.GetWord(3)->GetDouble();
			if(a<0.0f)
				a=0.0f;
			else if(a>1.0f)
				a=1.0f;
			a*=255.0f;

			kc=DrawColorA(r,g,b,(int)a);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
		return(false);
	}

	/* hsl */
	if(!strcmpin(cp,"hsl(",4))
	{
		int num;

		kGUIString s;
		kGUIStringSplit ss;
		bool ph=false,ps=false,pl=false;
		double ch,cs,cl;
		unsigned char ur;
		unsigned char ug;
		unsigned char ub;

		s.SetString(col->GetString()+4);
		s.Clip(s.GetLen()-1);
		num=ss.Split(&s,",");
		if(num==3)
		{
			if(DigOnlyPP(ss.GetWord(0),&ph)==false || DigOnlyPP(ss.GetWord(1),&ps)==false || DigOnlyPP(ss.GetWord(2),&pl)==false)
			{
				/* an invalid charcter was found! */
				if(m_strict)
					return(false);
			}
			/* s & v should have % after them, h should not */

			if(ph!=false || ps!=true || pl!=true)
			{
				/* either all 3 are percent or none are! */
				if(m_strict)
					return(false);
			}

			ch=ss.GetWord(0)->GetDouble();
			if(ch>=360.0f)
			{
				num=(int)(ch/360.0f);
				ch-=num*360.0;
			}
			else if(ch<=0.0f)
			{
				num=(int)(-ch/360.0f);
				ch+=num*360.0;
				if(ch<0.0f)
					ch+=360.0f;
			}
			cs=ss.GetWord(1)->GetDouble();
			if(cs<0.0f)
				cs=0.0f;
			else if(cs>100.0f)
				cs=100.0f;
			cl=ss.GetWord(2)->GetDouble();
			if(cl<0.0f)
				cl=0.0f;
			else if(cl>100.0f)
				cl=100.0f;

			kGUI::HSLToRGB(ch/360.0f,cs/100.0f,cl/100.0f,&ur,&ug,&ub);

			kc=DrawColorA(ur,ug,ub,255);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
		return(false);
	}

	/* hsla */
	if(!strcmpin(cp,"hsla(",5))
	{
		int num;

		kGUIString s;
		kGUIStringSplit ss;
		bool ph=false,ps=false,pl=false;
		double ch,cs,cl,a;
		unsigned char ur;
		unsigned char ug;
		unsigned char ub;

		s.SetString(col->GetString()+5);
		s.Clip(s.GetLen()-1);
		num=ss.Split(&s,",");
		if(num==4)
		{
			if(DigOnlyPP(ss.GetWord(0),&ph)==false || DigOnlyPP(ss.GetWord(1),&ps)==false || DigOnlyPP(ss.GetWord(2),&pl)==false || DigOnlyP(ss.GetWord(3))==false)
			{
				/* an invalid charcter was found! */
				if(m_strict)
					return(false);
			}
			/* s & v should have % after them, h should not */

			if(ph!=false || ps!=true || pl!=true)
			{
				/* either all 3 are percent or none are! */
				if(m_strict)
					return(false);
			}

			ch=ss.GetWord(0)->GetDouble();
			if(ch>=360.0f)
			{
				num=(int)(ch/360.0f);
				ch-=num*360.0;
			}
			else if(ch<=0.0f)
			{
				num=(int)(-ch/360.0f);
				ch+=num*360.0;
				if(ch<0.0f)
					ch+=360.0f;
			}
			cs=ss.GetWord(1)->GetDouble();
			if(cs<0.0f)
				cs=0.0f;
			else if(cs>100.0f)
				cs=100.0f;
			cl=ss.GetWord(2)->GetDouble();
			if(cl<0.0f)
				cl=0.0f;
			else if(cl>100.0f)
				cl=100.0f;

			kGUI::HSLToRGB(ch/360.0f,cs/100.0f,cl/100.0f,&ur,&ug,&ub);

			a=ss.GetWord(3)->GetDouble();
			if(a<0.0f)
				a=0.0f;
			else if(a>1.0f)
				a=1.0f;
			a*=255.0f;

			kc=DrawColorA(ur,ug,ub,a);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
		return(false);
	}

	if(cp[0]=='#')
	{
		gothash=true;
		++cp;
	}
	else
		gothash=false;

	cid=GetConstID(cp);
	switch(cid)
	{
	case -1:
		/* todo, if strict then must have gothash here */
		if(strlen(cp)==6)
		{
			int r,g,b;

			r=GetHexByte(cp);
			if(r<0)
				return(false);
			g=GetHexByte(cp+2);
			if(g<0)
				return(false);
			b=GetHexByte(cp+4);
			if(b<0)
				return(false);

			if(m_strict && gothash==false)
			{
				return(false);
			}

			kc=DrawColorA(r,g,b,255);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
		else if(strlen(cp)==3)
		{
			int r,g,b;

			r=GetHexNibble(cp);
			if(r<0)
				return(false);
			g=GetHexNibble(cp+1);
			if(g<0)
				return(false);
			b=GetHexNibble(cp+2);
			if(b<0)
				return(false);

			if(m_strict && gothash==false)
			{
				return(false);
			}

			kc=DrawColorA(((r<<4)|r),((g<<4)|g),((b<<4)|b),255);
			c->Set(kc);
			m_colorhash.Add(col->GetString(),&kc);
			return(true);
		}
	break;
	case HTMLCONST_TRANSPARENT:
		c->SetTransparent(true);
		return(true);
	break;
	case HTMLCONST_INVERT:
		c->SetInverse(true);
		return(true);
	break;
	default:
		if(cid>=0 && cid<HTMLCONST_TRANSPARENT)
		{
			/* todo: add these to the colorhash table in the init code */
			c->Set(m_colors[cid]);
			m_colorhash.Add(col->GetString(),&m_colors[cid]);
			return(true);
		}
	break;
	}

	return(false);
}

int kGUIHTMLPageObj::GetHexNibble(const char *hex)
{
	if(hex[0]>='0' && hex[0]<='9')
		return(hex[0]-'0');
	else if(hex[0]>='a' && hex[0]<='f')
		return((hex[0]-'a')+10);
	else if(hex[0]>='A' && hex[0]<='F')
		return((hex[0]-'A')+10);
	else
		return(-1);
}

int kGUIHTMLPageObj::GetHexByte(const char *hex)
{
	int n1,n2;

	n1=GetHexNibble(hex);
	if(n1<0)
		return(-1);
	n2=GetHexNibble(hex+1);
	if(n2<0)
		return(-1);
	return((n1<<4)|n2);
}

class HTMLPageReportObj : public kGUIReportObj
{
public:
	HTMLPageReportObj(kGUIHTMLPageObj *po,int offy) {m_po=po;m_offy=offy;}
	~HTMLPageReportObj() {}
	void Draw(void);
private:
	kGUIHTMLPageObj *m_po;
	int m_offy;
};

void HTMLPageReportObj::Draw(void)
{
	m_po->DrawPrint(0,m_offy);
}

class HTMLPageReport : public kGUIReport
{
public:
	HTMLPageReport(kGUIHTMLPageObj *po) {m_po=po;SetPID(po->GetPID());}
	~HTMLPageReport() {m_po->SetPID(GetPID());}
	void Setup(void);
	void Setup(int pagenum) {}
	const char *GetName(void)		{return m_po->GetTitle()->GetString();}
	int GetPPI(void)				{return 72;}	/* pixels per inch */
	double GetPageWidth(void)		{return -1;}	/* inches */
	double GetPageHeight(void)		{return -1;}	/* inches */
	double GetLeftMargin(void)		{return 0.25f;}	/* inches */
	double GetRightMargin(void)		{return 0.25f;}	/* inches */
	double GetTopMargin(void)		{return 0.25f;}	/* inches */
	double GetBottomMargin(void)	{return 0.25f;}	/* inches */

private:
	kGUIHTMLPageObj *m_po;
};

void HTMLPageReport::Setup(void)
{
	int i,height,numpages,y;
	HTMLPageReportObj *ro;
	int pw,ph;	
	
	/* page width and height in pixels */
	GetPageSizePixels(&pw,&ph);

	/* position using pagewidth */
	height=m_po->PositionPrint(pw);
	numpages=(height/ph)+1;
	y=0;
	for(i=0;i<numpages;++i)
	{
		ro=new HTMLPageReportObj(m_po,y);
		ro->SetPos(0,y);
		ro->SetSize(pw,ph);
		AddObjToSection(REPORTSECTION_BODY,ro,true);
		y+=ph;
	}
}

/* print preview */
void kGUIHTMLPageObj::Preview(void)
{
	HTMLPageReport *r;

	Parse(true);	/* copy to print buffer */

	r=new HTMLPageReport(this);
	r->Preview();
}

bool kGUIHTMLButtonTextObj::UpdateInput(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(kGUI::MouseOver(&c))
	{
		kGUI::SetTempMouseCursor(MOUSECURSOR_LINK);

		if(kGUI::GetMouseReleaseLeft())
		{
			kGUIHTMLFormObj *f;
			
			f=kGUIHTMLObj::GetForm(m_styleparent);
			if(f)
			{
				switch(m_type)
				{
				case HTMLCONST_SUBMIT:
					f->Submit(m_styleparent);
				break;
				case HTMLCONST_RESET:
					f->Reset();
				break;
				case HTMLCONST_BUTTON:
				break;
				}
				return(true);
			}
			/* error, no enclosing form */
			return(true);
		}
	}
	return(false);
}

bool kGUIHTMLButtonImageObj::UpdateInput(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(kGUI::MouseOver(&c))
	{
		kGUI::SetTempMouseCursor(MOUSECURSOR_LINK);
		if(kGUI::GetMouseReleaseLeft())
		{
			kGUIHTMLFormObj *f;
			
			f=kGUIHTMLObj::GetForm(m_styleparent);
			if(f)
			{
				f->Submit(m_styleparent);
				return(true);
			}
			/* error, no enclosing form */
			return(true);
		}
	}
	return(false);
}

/* encode a form parameter from plain text to encoded text */
void kGUIHTMLFormObj::Encode(kGUIString *in,kGUIString *out)
{
	int i,nc;
	unsigned int c;

	out->Clear();

	nc=in->GetLen();
	for(i=0;i<nc;++i)
	{
		c=in->GetChar(i);
		if( (c>='0' && c<='9') || (c>='a' && c<='z') || (c>='A' && c<='Z'))
			out->Append(c);
		else if(c==' ')
			out->Append('+');
		else
			out->ASprintf("%%%02x",c);
	}
}
/* this event is attached to all inputboxes, and is called when the user */
/* changes the input, is looks for the user typing enter ( or return ) */
/* and then checks to see if the form only has 1 inputbox and no textareas */
/* if that is the case then it does a submit */

void kGUIHTMLPageObj::CheckSubmit(kGUIEvent *event)
{
	kGUIObj *obj;
	kGUIHTMLObj *hobj;
	kGUIHTMLFormObj *form;

	/* has the user pressed Return on this input box? */
	if(event->GetEvent()==EVENT_PRESSRETURN)
	{
		obj=event->GetObj();
		hobj=static_cast<kGUIHTMLObj *>(obj->GetParent());
		form=kGUIHTMLObj::GetForm(hobj);
		if(form)
			form->CheckSimpleSubmit();
	}
}

void kGUIHTMLFormObj::CheckSimpleSubmit(void)
{
	unsigned int i;
	kGUIHTMLObj *obj;
	bool simple;
	int numinputboxes;

	simple=true;
	numinputboxes=0;
	for(i=0;i<m_numchildren;++i)
	{
		obj=m_children.GetEntry(i);
		switch(obj->GetID())
		{
		case HTMLTAG_TEXTAREA:
			simple=false;
		break;
		case HTMLTAG_INPUT:
			switch(obj->GetSubID())
			{
			case HTMLSUBTAG_INPUTTEXTBOX:
				++numinputboxes;
			break;
			case HTMLSUBTAG_INPUTFILE:
				simple=false;
			break;
			}
		break;
		}
	}
	if(numinputboxes==1 && simple==true)
		Submit(0);
}


void kGUIHTMLFormObj::Submit(kGUIHTMLObj *button)
{
	kGUIString url;
	kGUIString post;
	kGUIHTMLObj *obj;
	kGUIString name;
	kGUIString value;
	kGUIString evalue;
	unsigned int i;
	int np=0;
	kGUIHTMLAttrib *att;
	bool multiple;
	kGUIHTMLClickInfo info;

	url.SetString(&m_action);

	for(i=0;i<m_numchildren;++i)
	{
		obj=m_children.GetEntry(i);

		att=obj->FindAttrib(HTMLATT_NAME);
		if(att)
		{
			name.SetString(att->GetValue());
			value.Clear();
			multiple=false;
			switch(obj->GetID())
			{
			case HTMLTAG_TEXTAREA:
				value.SetString(obj->m_obj.m_inputobj->GetString());
			break;
			case HTMLTAG_INPUT:
				switch(obj->GetSubID())
				{
				case HTMLSUBTAG_INPUTBUTTON:
				case HTMLSUBTAG_INPUTBUTTONSUBMIT:
				case HTMLSUBTAG_INPUTBUTTONRESET:
				case HTMLSUBTAG_INPUTBUTTONIMAGE:
					if(obj==button)						/* only send info for the particular button pressed */
					{
						att=obj->FindAttrib(HTMLATT_VALUE);
						if(att)
							value.SetString(att->GetValue());
						else
							value.Clear();	/* error */
					}
					else
					{
						name.Clear();
						value.Clear();
					}
				break;
				case HTMLSUBTAG_INPUTHIDDEN:
					att=obj->FindAttrib(HTMLATT_VALUE);
					if(att)
						value.SetString(att->GetValue());
					else
						value.Clear();	/* error */
				break;
				case HTMLSUBTAG_INPUTCHECKBOX:
					value.Clear();
					if(obj->m_obj.m_tickobj->GetSelected()==true)
					{
						att=obj->FindAttrib(HTMLATT_VALUE);
						if(att)
							value.SetString(att->GetValue());
						else
							value.SetString("on");
					}
					else
						name.Clear();
				break;
				case HTMLSUBTAG_INPUTRADIO:
					value.Clear();
					if(obj->m_obj.m_radioobj->GetSelected()==true)
					{
						att=obj->FindAttrib(HTMLATT_VALUE);
						if(att)
							value.SetString(att->GetValue());
						else
							value.SetString("on");
					}
					else
						name.Clear();
				break;
				case HTMLSUBTAG_INPUTTEXTBOX:
					value.SetString(obj->m_obj.m_inputobj->GetString());
				break;
				case HTMLSUBTAG_INPUTFILE:
					/* todo, upload selected file */
				break;
				}
			break;
			case HTMLTAG_SELECT:
				switch(obj->GetSubID())
				{
				case HTMLSUBTAG_INPUTLISTBOX:
				{
					/* build a tab seperated string of all selected values */
					unsigned int sel;
					unsigned int index;
					unsigned int numsel;
					Array<unsigned int>selected;

					if(!obj->m_obj.m_listboxobj->GetNumEntries())
						value.Clear();
					else
					{
						selected.Init(16,4);
						numsel=obj->m_obj.m_listboxobj->GetSelections(&selected);
						for(sel=0;sel<numsel;++sel)
						{
							index=selected.GetEntry(sel);
							if(sel)
								value.Append("\t");
							value.Append(obj->m_obj.m_listboxobj->GetSelectionStringObj(index));
						}
						if(numsel>1)
							multiple=true;
					}
				}
				break;
				default:
					if(obj->m_obj.m_comboboxobj->GetNumEntries())
						value.SetString(obj->m_obj.m_comboboxobj->GetSelectionString());
					else
						value.Clear();	/* Error: combo box has no entries attached */
				break;
				}
			break;
			default:
				name.Clear();
			break;
			}

			if(name.GetLen())
			{
				if(multiple==false)
				{
					switch(m_mode)
					{
					case FORMMODE_POST:
						if(np)
							post.Append("&");
						post.Append(name.GetString());
						post.Append("=");

						Encode(&value,&evalue);
						post.Append(evalue.GetString());
						++np;
					break;
					case FORMMODE_GET:
						if(!np)
							url.Append("?");
						else
							url.Append("&");
						url.Append(name.GetString());
						url.Append("=");

						Encode(&value,&evalue);
						url.Append(evalue.GetString());
						++np;
					break;
					}
				}
				else
				{
					unsigned int w,numw;
					kGUIStringSplit ss;
					kGUIString *vs;

					/* value contains multiple tab seperated values */
					numw=ss.Split(&value,"\t");

					for(w=0;w<numw;++w)
					{
						vs=ss.GetWord(w);
						switch(m_mode)
						{
						case FORMMODE_POST:
							if(np)
								post.Append("&");
							post.Append(name.GetString());
							post.Append("=");

							Encode(vs,&evalue);
							post.Append(evalue.GetString());
							++np;
						break;
						case FORMMODE_GET:
							if(!np)
								url.Append("?");
							else
								url.Append("&");
							url.Append(name.GetString());
							url.Append("=");

							Encode(vs,&evalue);
							url.Append(evalue.GetString());
							++np;
						break;
						}
					}
				}
			}
		}
	}

	info.m_newtab=false;
	if(post.GetLen())
		info.m_post=&post;
	else
		info.m_post=0;
	info.m_url=&url;
	info.m_referrer=GetReferrer();

	m_page->CallClickCallback(&info);
}

/* put all inputs back to their defaults */
void kGUIHTMLFormObj::Reset(void)
{
	unsigned int i;
	kGUIHTMLObj *obj;

	/* set the obj_init flag to false for all form objects, this will make them reset to default values */
	for(i=0;i<m_numchildren;++i)
	{
		obj=m_children.GetEntry(i);
		obj->m_objinit=false;
	}

	/* the page needs to be reparsed to reset the form fields */
	m_page->RePosition(false);
	m_page->Dirty();
}

void kGUIHTMLButtonTextObj::Draw(void)
{
	int h;
	kGUICorners c;
	int td=GetTextDecoration();
	kGUIColor tdc=GetTextDecorationColor();

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		kGUI::DrawRectBevel(c.lx,c.ty,c.rx,c.by,false);
		m_styleparent->DrawBG(&c,false,0,0,0,0);
			c.lx+=3;
			c.rx-=3;
			c.ty+=3;
			c.by-=3;
	
		if(kGUI::GetCurrentObj()==this)
			SetRevRange(0,GetLen());
		else
			SetRevRange(0,0);

		if(GetUseBGColor()==true)
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,GetBGColor());

		{
			int offx,offy;
			int tw,th;

			th=kGUIText::GetLineHeight();
			tw=kGUIText::GetWidth();
			offx=((c.rx-c.lx)-tw)>>1;
			if(offx<0)
				offx=0;
			offy=((c.by-c.ty)-th)>>1;
			if(offy<0)
				offy=0;

			c.lx+=offx;
			c.rx-=offx;
			c.ty+=offy;
			c.by-=offy;
		}

		kGUIText::Draw(c.lx,c.ty,0,0);
		if(td&TEXTDECORATION_UNDERLINE)
		{
			h=GetAscHeight();
			kGUI::DrawRect(c.lx,c.ty+h+2,c.rx,c.ty+h+3,tdc);
		}
		if(td&TEXTDECORATION_OVERLINE)
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.ty+1,tdc);
		if(td&TEXTDECORATION_LINETHROUGH)
		{
			h=GetAscHeight()>>1;
			kGUI::DrawRect(c.lx,c.ty+h,c.rx,c.ty+h+1,tdc);
		}
	}
	kGUI::PopClip();
}

void kGUIHTMLTextObj::Draw(void)
{
	int h;
	kGUICorners c;

	if(GetAlpha()==0.0f)
		return;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);

	//we have no reference back to the page so we will use this global hack!
	if(g_trace)
	{
		kGUI::Trace("TextDraw '%s' lx=%d,rx=%d,ty=%d,by=%d, Color(%x)\n",GetString(),c.lx,c.rx,c.ty,c.by,GetColor());
	}

	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		if(kGUI::GetCurrentObj()==this)
			SetRevRange(0,GetLen());
		else
			SetRevRange(0,0);

		if(GetUseBGColor()==true)
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.by,GetBGColor());

		if(m_draw==true)	/* toggled when in blink mode */
		{
			if(m_shadowr>=0)
				kGUIText::Draw(c.lx+m_shadowx,c.ty+m_shadowy,0,0,m_shadowcolor);

			kGUIText::Draw(c.lx,c.ty,GetZoneW(),GetZoneH());
		}

		if(m_textdecoration&TEXTDECORATION_UNDERLINE)
		{
			h=GetAscHeight();
			kGUI::DrawRect(c.lx,c.ty+h+2,c.rx,c.ty+h+3,m_textdecorationcolor);
		}
		if(m_textdecoration&TEXTDECORATION_OVERLINE)
			kGUI::DrawRect(c.lx,c.ty,c.rx,c.ty+1,m_textdecorationcolor);
		if(m_textdecoration&TEXTDECORATION_LINETHROUGH)
		{
			h=GetAscHeight()>>1;
			kGUI::DrawRect(c.lx,c.ty+h,c.rx,c.ty+h+1,m_textdecorationcolor);
		}
		/* debug */
		//	kGUI::DrawCurrentFrame(c.lx,c.ty,c.rx,c.by);
	}
	kGUI::PopClip();
}

bool kGUIHTMLTextObj::UpdateInput(void)
{
	kGUICorners c;

	GetCorners(&c);
	if(kGUI::MouseOver(&c))
	{
		if(m_link)
		{
			kGUI::SetTempMouseCursor(MOUSECURSOR_LINK);
			m_link->SetOver();
			if(kGUI::GetMouseReleaseLeft())
			{
				m_link->Click();
				return(true);
			}
			else if(kGUI::GetMouseReleaseRight())
			{
				m_link->GetPage()->RightClick(m_link,HTMLTAG_LINK);
				return(true);
			}
		}
	}
	return(false);
}

void kGUIOnlineImageObj::Draw(void)
{
	if(1) //GetIsValid()==true)
	{
		/* draw the image first */
		kGUIImageRefObj::Draw();
	}
	else if(GetBad()==true)
	{
		/* this is set when a download was triggered but came back with an error */
		/* or if there is valid image data but not in a understood */
	}
	if(m_map && m_page->GetSettings()->GetDrawAreas())
	{
		unsigned int i;
		unsigned int n;
		kGUIHTMLArea *a;
		kGUICorners c;

		GetCorners(&c);

		/* if map area debugging is on then draw clickable areas */
		n=m_map->GetNumAreas();
		for(i=0;i<n;++i)
		{
			a=m_map->GetArea(i);
			switch(a->GetType())
			{
			case HTMLCONST_CIRC:
			case HTMLCONST_CIRCLE:
				kGUI::DrawCircle(c.lx+a->GetPointX(0),c.ty+a->GetPointX(1),a->GetPointX(2),DrawColor(255,255,255),0.5f);
			break;
			case HTMLCONST_RECT:
			case HTMLCONST_RECTANGLE:
				kGUI::DrawRect(c.lx+a->GetPointX(0),c.ty+a->GetPointX(1),c.lx+a->GetPointX(2),c.ty+a->GetPointX(3),DrawColor(255,255,255),0.5f);
			break;
			case HTMLCONST_POLY:
			case HTMLCONST_POLYGON:
			{
				unsigned int j;
				unsigned int k;
				kGUIPoint2 *p;

				k=a->GetNumPoints();
				p=a->GetArrayPtr();
				for(j=0;j<k;++j)
				{
					p->x+=c.lx;
					p->y+=c.ty;
					++p;
				}

				kGUI::DrawPoly(k,a->GetArrayPtr(),DrawColor(255,255,255),0.5f);

				p=a->GetArrayPtr();
				for(j=0;j<k;++j)
				{
					p->x-=c.lx;
					p->y-=c.ty;
					++p;
				}

			}
			break;
			}
		}
	}
}

bool kGUIOnlineImageObj::UpdateInput(void)
{
	kGUICorners c;
	kGUIHTMLLinkObj *link;

	GetCorners(&c);
	if(kGUI::MouseOver(&c))
	{
		link=m_link;
		/* is there an attached map? */
		if(m_map)
		{
			unsigned int i;
			unsigned int n;
			kGUIHTMLArea *a;
			kGUICorners c2;
			bool over=false;

			/* if map area debugging is on then draw clickable areas */
			n=m_map->GetNumAreas();
			for(i=0;(i<n) && over==false;++i)
			{
				a=m_map->GetArea(i);
				link=a->GetLink();
				switch(a->GetType())
				{
				case HTMLCONST_CIRC:
				case HTMLCONST_CIRCLE:
					/* use hypot */
					c2.lx=c.lx+a->GetPointX(0);
					c2.ty=c.ty+a->GetPointX(1);
					if(kGUI::FastHypot(c2.lx-kGUI::GetMouseX(),c2.ty-kGUI::GetMouseY())<=a->GetPointX(2))
						over=true;
				break;
				case HTMLCONST_RECT:
				case HTMLCONST_RECTANGLE:
					c2.lx=c.lx+a->GetPointX(0);
					c2.ty=c.ty+a->GetPointX(1);
					c2.rx=c.lx+a->GetPointX(2);
					c2.by=c.ty+a->GetPointX(3);
					if(kGUI::MouseOver(&c2))
						over=true;
				break;
				case HTMLCONST_POLY:
				case HTMLCONST_POLYGON:
					if(kGUI::PointInsidePoly((int)(kGUI::GetMouseX()-c.lx),(int)(kGUI::GetMouseY()-c.ty),a->GetNumPoints(),a->GetArrayPtr()))
						over=true;
				break;
				case HTMLCONST_DEFAULT:
					over=true;
				break;
				}
			}
			if(!over)
				link=0;
		}
		if(link)
		{
			kGUI::SetTempMouseCursor(MOUSECURSOR_LINK);
			link->SetOver();
			if(kGUI::GetMouseReleaseLeft())
			{
				link->Click();
				return(true);
			}
		}
		if(kGUI::GetMouseClickRight())
			m_page->RightClick(this,HTMLTAG_IMG);
	}
	return(false);
}

void kGUIOnlineLink::SetURL(kGUIString *url,kGUIString *referrer,unsigned int type)
{
	AsyncLoadInfo ali;

	/* same as before? */
	if(!strcmp(m_url.GetString(),url->GetString()))
		return;

	m_url.SetString(url);
	m_type=type;

	/* is this local? */
	if(!strncmp(url->GetString(),"file://",7))
	{
		SetFilename(url->GetString()+7);
		SetLoadPending(false);				/* it's ready NOW */
	}
	else
	{
		m_page->IncLoading();

		/* online */
		SetMemory();
		
		m_dl.SetReferer(referrer->GetString());
		m_dl.SetAuthHandler(m_page->GetAuthHandler());
		m_dl.SetURL(url);
		CheckWrite();
		kGUI::Trace("Load %08x '%s'\n",this,url->GetString());
		ali.m_dh=this;
		ali.m_dl=&m_dl;
		ali.m_donecallback.Set(this,CALLBACKNAME(LoadFinished));
		m_page->AsyncLoad(&ali,true);
	}
}

void kGUIOnlineLink::LoadFinished(int status)
{
	switch(status)
	{
	case DOWNLOAD_OK:
	case DOWNLOAD_SAME:
		m_header.SetString(m_dl.GetHeader());
	break;
	default:
		/* error */
	break;
	}

	m_page->DecLoading();
	m_loadedcallback.Call(this);
}

/* return true if loaded ok */
bool kGUIOnlineLink::CacheURL(kGUIString *url,unsigned int type,kGUIString *fn)
{
	m_url.SetString(url);
	m_type=type;

	SetFilename(fn->GetString());
	if(Open()==false)
		return(false);		/* couldn't open file! */

	Close();
	return(true);
}

kGUIHTMLBox::kGUIHTMLBox(kGUIHTMLPageObj *page)
{
	m_page=page;
	Reset();
}

void kGUIHTMLBox::Reset(void)
{
	m_bw=0;
	m_leftbw=3;			/* border, default to medium */
	m_rightbw=3;
	m_topbw=3;
	m_bottombw=3;

	m_topcolor.Reset();
	m_leftcolor.Reset();
	m_bottomcolor.Reset();
	m_rightcolor.Reset();

	m_leftstyle=BORDERSTYLE_NONE;
	m_rightstyle=BORDERSTYLE_NONE;
	m_topstyle=BORDERSTYLE_NONE;
	m_bottomstyle=BORDERSTYLE_NONE;

	m_leftmw=0;	/* margin */
	m_rightmw=0;
	m_topmw=0;
	m_bottommw=0;
	m_marginauto=0;

	m_leftpw=0;	/* padding */
	m_rightpw=0;
	m_toppw=0;
	m_bottompw=0;
}

/* false means that there is essentially no border info defined */

bool kGUIHTMLBox::GetValid(void)
{
	if(GetBoxTopWidth())
		return(true);
	if(GetBoxLeftWidth())
		return(true);
	if(GetBoxRightWidth())
		return(true);
	if(GetBoxBottomWidth())
		return(true);

	if(m_bw)
		return(true);

	return(false);
}

unsigned int kGUIHTMLBox::GetBoxTopWidth(void)
{
	if(m_topstyle==BORDERSTYLE_NONE)
		return GetBoxPosTopMargin()+m_toppw;
	else
		return m_topbw+GetBoxPosTopMargin()+m_toppw;
}

unsigned int kGUIHTMLBox::GetBoxLeftWidth(void)
{
	if(m_leftstyle==BORDERSTYLE_NONE)
		return GetBoxPosLeftMargin()+m_leftpw;
	else
		return m_leftbw+GetBoxPosLeftMargin()+m_leftpw;
}

unsigned int kGUIHTMLBox::GetBoxRightWidth(void)
{
	if(m_rightstyle==BORDERSTYLE_NONE)
		return GetBoxPosRightMargin()+m_rightpw;
	else
		return m_rightbw+GetBoxPosRightMargin()+m_rightpw;
}

unsigned int kGUIHTMLBox::GetBoxBottomWidth(void)
{
	if(m_bottomstyle==BORDERSTYLE_NONE)
		return GetBoxPosBottomMargin()+m_bottompw;
	else
		return m_bottombw+GetBoxPosBottomMargin()+m_bottompw;
}

void kGUIHTMLBox::SetBorder(unsigned int w)
{
	m_bw=w;
	m_topbw=w;
	m_leftbw=w;
	m_rightbw=w;
	m_bottombw=w;
	if(w)
		m_topstyle=m_bottomstyle=m_leftstyle=m_rightstyle=BORDERSTYLE_SOLID;
	else
		m_topstyle=m_bottomstyle=m_leftstyle=m_rightstyle=BORDERSTYLE_HIDDEN;
}

void kGUIHTMLBox::SetBorderWidth(unsigned int bb,kGUIString *s,kGUIHTMLObj *parent)
{
	unsigned int code;
	unsigned int wpix;
	kGUIUnits u;

	code=m_page->GetConstID(s->GetString());
	switch(code)
	{
	case HTMLCONST_THIN:
		wpix=1;
	break;
	case HTMLCONST_MEDIUM:
		wpix=3;
	break;
	case HTMLCONST_THICK:
		wpix=5;
	break;
	case HTMLCONST_LENGTH:
		wpix=14;
	break;
	case HTMLCONST_INHERIT:
	{
		kGUIHTMLBox *pbox=parent->GetBox();

		if(bb&BORDER_LEFT)
			m_leftbw=pbox->m_leftbw;
		if(bb&BORDER_RIGHT)
			m_rightbw=pbox->m_rightbw;
		if(bb&BORDER_TOP)
			m_topbw=pbox->m_topbw;
		if(bb&BORDER_BOTTOM)
			m_bottombw=pbox->m_bottombw;
		return;
	}
	break;
	case -1:
		/* todo, get width as units may be a percent?? */
		u.Set(m_page,s);
		wpix=u.CalcUnitValue(0,parent->GetEM());
	break;
	default:
		m_page->m_errors.ASprintf("unknown border-width '%s'\n",s->GetString());
		return;
	break;
	}
	if(bb&BORDER_LEFT)
		m_leftbw=wpix;
	if(bb&BORDER_RIGHT)
		m_rightbw=wpix;
	if(bb&BORDER_TOP)
		m_topbw=wpix;
	if(bb&BORDER_BOTTOM)
		m_bottombw=wpix;
}

void kGUIHTMLBox::SetMarginWidth(unsigned int mm,kGUIString *s,kGUIHTMLObj *parent)
{
	kGUIUnits u;
	unsigned int wpix;

	if(m_page->GetConstID(s->GetString())==HTMLCONST_INHERIT)
	{
		kGUIHTMLBox *pbox=parent->GetBox();

		if(mm&MARGIN_LEFT)
			m_leftmw=pbox->m_leftmw;
		if(mm&MARGIN_RIGHT)
			m_rightmw=pbox->m_rightmw;
		if(mm&MARGIN_TOP)
			m_topmw=pbox->m_topmw;
		if(mm&MARGIN_BOTTOM)
			m_bottommw=pbox->m_bottommw;
		return;
	}

	u.Set(m_page,s);
	switch(u.GetUnitType())
	{
	case UNITS_AUTO:
		m_marginauto|=mm;
		return;
	break;
	default:
		wpix=u.CalcUnitValue(parent->GetOutsideW(),parent->GetEM());
	break;
	}

	if(mm&MARGIN_LEFT)
		m_leftmw=wpix;
	if(mm&MARGIN_RIGHT)
		m_rightmw=wpix;
	if(mm&MARGIN_TOP)
		m_topmw=wpix;
	if(mm&MARGIN_BOTTOM)
		m_bottommw=wpix;
}

void kGUIHTMLBox::SetUndefinedBorderColors(kGUIColor c,bool iscell)
{
	/* if a border color is not defined then we will set it to the */
	/* objects text color, unless it is a table cell then we will set */
	/* it to two shades of gray to make a beveled 3d look */

	if(m_topcolor.GetUndefined())
		m_topcolor.Set(iscell==false?c:DrawColor(0x8d,0x8d,0x8d));

	if(m_leftcolor.GetUndefined())
		m_leftcolor.Set(iscell==false?c:DrawColor(0x8d,0x8d,0x8d));

	if(m_rightcolor.GetUndefined())
		m_rightcolor.Set(iscell==false?c:DrawColor(0xc0,0xc0,0xc0));

	if(m_bottomcolor.GetUndefined())
		m_bottomcolor.Set(iscell==false?c:DrawColor(0xc0,0xc0,0xc0));
}


int kGUIHTMLBox::GetMarginAlign(void)
{
	switch(m_marginauto)
	{
	case 0:
		return(ALIGN_UNDEFINED);
	break;
	case MARGIN_LEFT:
		return(ALIGN_RIGHT);
	break;
	case MARGIN_RIGHT:
		return(ALIGN_LEFT);
	break;
	case MARGIN_LEFT+MARGIN_RIGHT:
		return(ALIGN_CENTER);
	break;
	}
	//huh??
	return(ALIGN_UNDEFINED);
}

void kGUIHTMLBox::SetPaddingWidth(unsigned int pp,kGUIString *s,kGUIHTMLObj *parent)
{
	kGUIUnits u;
	unsigned int wpix;

	switch(m_page->GetConstID(s->GetString()))
	{
	case HTMLCONST_INHERIT:
	{
		kGUIHTMLBox *pbox=parent->GetBox();

		if(pp&PADDING_LEFT)
			m_leftpw=pbox->m_leftpw;
		if(pp&PADDING_RIGHT)
			m_rightpw=pbox->m_rightpw;
		if(pp&PADDING_TOP)
			m_toppw=pbox->m_toppw;
		if(pp&PADDING_BOTTOM)
			m_bottompw=pbox->m_bottompw;
	}
	break;
	case HTMLCONST_AUTO:
		/* not sure what to do here, any ideas?? */
		if(pp&PADDING_LEFT)
			m_leftpw=0;
		if(pp&PADDING_RIGHT)
			m_rightpw=0;
		if(pp&PADDING_TOP)
			m_toppw=0;
		if(pp&PADDING_BOTTOM)
			m_bottompw=0;
	break;
	default:
		u.Set(m_page,s);
		wpix=u.CalcUnitValue(0,parent->GetEM());

		if(pp&PADDING_LEFT)
			m_leftpw=wpix;
		if(pp&PADDING_RIGHT)
			m_rightpw=wpix;
		if(pp&PADDING_TOP)
			m_toppw=wpix;
		if(pp&PADDING_BOTTOM)
			m_bottompw=wpix;
	break;
	}
}

void kGUIHTMLBox::SetBorderColor(unsigned int bb,kGUIString *s)
{
	kGUIHTMLColor c;

	if(m_page->GetColor(s,&c))
	{
		if(bb&BORDER_LEFT)
			m_leftcolor.Set(&c);
		if(bb&BORDER_RIGHT)
			m_rightcolor.Set(&c);
		if(bb&BORDER_TOP)
			m_topcolor.Set(&c);
		if(bb&BORDER_BOTTOM)
			m_bottomcolor.Set(&c);
		return;
	}
	m_page->m_errors.ASprintf("unknown border-color '%s'\n",s->GetString());
}

void kGUIHTMLBox::SetBorderColor(unsigned int bb,kGUIHTMLColor *c)
{
	if(bb&BORDER_LEFT)
		m_leftcolor.Set(c);
	if(bb&BORDER_RIGHT)
		m_rightcolor.Set(c);
	if(bb&BORDER_TOP)
		m_topcolor.Set(c);
	if(bb&BORDER_BOTTOM)
		m_bottomcolor.Set(c);
}


void kGUIHTMLBox::SetBorderStyle(unsigned int bb,kGUIString *s)
{
	unsigned int code;
	unsigned int bstyle;

	code=m_page->GetConstID(s->GetString());
	switch(code)
	{
	case HTMLCONST_NONE:
		bstyle=BORDERSTYLE_NONE;
	break;
	case HTMLCONST_HIDDEN:
		bstyle=BORDERSTYLE_HIDDEN;
	break;
	case HTMLCONST_DOTTED:
		bstyle=BORDERSTYLE_DOTTED;
	break;
	case HTMLCONST_DASHED:
		bstyle=BORDERSTYLE_DASHED;
	break;
	case HTMLCONST_SOLID:
		bstyle=BORDERSTYLE_SOLID;
	break;
	case HTMLCONST_DOUBLE:
		bstyle=BORDERSTYLE_DOUBLE;
	break;
	case HTMLCONST_GROOVE:
		bstyle=BORDERSTYLE_GROOVE;
	break;
	case HTMLCONST_RIDGE:
		bstyle=BORDERSTYLE_RIDGE;
	break;
	case HTMLCONST_INSET:
		bstyle=BORDERSTYLE_INSET;
	break;
	case HTMLCONST_OUTSET:
		bstyle=BORDERSTYLE_OUTSET;
	break;
	default:
		/* if unknown then use groove */
		bstyle=BORDERSTYLE_GROOVE;
		m_page->m_errors.ASprintf("unknown border-style '%s'\n",s->GetString());
	break;
	}
	if(bb&BORDER_LEFT)
		m_leftstyle=bstyle;
	if(bb&BORDER_RIGHT)
		m_rightstyle=bstyle;
	if(bb&BORDER_TOP)
		m_topstyle=bstyle;
	if(bb&BORDER_BOTTOM)
		m_bottomstyle=bstyle;
}

kGUIColor kGUIHTMLBox::Dark(kGUIColor c)
{
	double r,g,b;

	DrawColorToRGB(c,r,g,b);
	r*=0.554f;
	g*=0.554f;
	b*=0.554f;
	return(DrawColor((int)r,(int)g,(int)b));
}

kGUIColor kGUIHTMLBox::Light(kGUIColor c)
{
	double r,g,b;

	DrawColorToRGB(c,r,g,b);
	r*=1.6875f;
	r=min(r,255.0f);
	r=max(r,178.0f);
	g*=1.6875f;
	g=min(g,255.0f);
	g=max(g,178.0f);
	b*=1.6875f;
	b=min(b,255.0f);
	b=max(b,178.0f);
	return(DrawColor((int)r,(int)g,(int)b));
}


/* draw line index/num using style and color */
void kGUIHTMLBox::DrawLine(unsigned int side,int index,int num,int x1,int y1,int x2,int y2,kGUIColor c,int style)
{
	int l;

	switch(style)
	{
	case BORDERSTYLE_NONE:
	case BORDERSTYLE_HIDDEN:
		return;
	break;
	case BORDERSTYLE_DOTTED:
		if(side==BORDER_TOP || side==BORDER_BOTTOM)	/* horiz? */
		{
			while(x1<x2)
			{
				kGUI::DrawLine(x1,y1,x1+num,y2,c);
				x1+=num<<1;
			}
		}
		else
		{
			while(y1<y2)
			{
				kGUI::DrawLine(x1,y1,x2,y1+num,c);
				y1+=num<<1;
			}
		}
	break;
	case BORDERSTYLE_DASHED:
		l=num*3;
		if(side==BORDER_TOP || side==BORDER_BOTTOM)	/* horiz? */
		{
			while(x1<x2)
			{
				kGUI::DrawLine(x1,y1,x1+l,y2,c);
				x1+=l<<1;
			}
		}
		else
		{
			while(y1<y2)
			{
				kGUI::DrawLine(x1,y1,x2,y1+l,c);
				y1+=l<<1;
			}
		}
	break;
	case BORDERSTYLE_SOLID:
solid:;
		kGUI::DrawLine(x1,y1,x2,y2,c);
	break;
	case BORDERSTYLE_DOUBLE:
	{
		double r;

		if(num==1)
			goto solid;

		r=((double)index+0.5f)/num;
		if(r<=0.33f || r>=0.66f)
			goto solid;
	}
	break;
	case BORDERSTYLE_GROOVE:
	{
		double r;

		if(num==1)
			goto solid;

		r=((double)index+0.5f)/num;
		/* swap right & bottom */
		if(side==BORDER_TOP || side==BORDER_LEFT)
			kGUI::DrawLine(x1,y1,x2,y2,r<=0.5f?Dark(c):Light(c));
		else
			kGUI::DrawLine(x1,y1,x2,y2,r>=0.5f?Dark(c):Light(c));
	}
	break;
	case BORDERSTYLE_RIDGE:
	{
		double r;

		if(num==1)
			goto solid;

		r=((double)index+0.5f)/num;
		if(side==BORDER_TOP || side==BORDER_LEFT)
			kGUI::DrawLine(x1,y1,x2,y2,r>=0.5f?Dark(c):Light(c));
		else
			kGUI::DrawLine(x1,y1,x2,y2,r<=0.5f?Dark(c):Light(c));
	}
	break;
	case BORDERSTYLE_INSET:
		if(side==BORDER_TOP || side==BORDER_LEFT)
			kGUI::DrawLine(x1,y1,x2,y2,Dark(c));
		else
			kGUI::DrawLine(x1,y1,x2,y2,Light(c));
	break;
	case BORDERSTYLE_OUTSET:
		if(side==BORDER_TOP || side==BORDER_LEFT)
			kGUI::DrawLine(x1,y1,x2,y2,Light(c));
		else
			kGUI::DrawLine(x1,y1,x2,y2,Dark(c));
	break;
	}
}

void kGUIHTMLBox::Draw(kGUICorners *c)
{
	unsigned int i;
	unsigned int w;
	int lx,rx,ty,by;
	unsigned int tb,lb,rb,bb;	/* border widths */
	kGUICorners cc;

	if(m_topstyle==BORDERSTYLE_NONE)
		tb=0;
	else
		tb=m_topbw;

	if(m_leftstyle==BORDERSTYLE_NONE)
		lb=0;
	else
		lb=m_leftbw;

	if(m_rightstyle==BORDERSTYLE_NONE)
		rb=0;
	else
		rb=m_rightbw;

	if(m_bottomstyle==BORDERSTYLE_NONE)
		bb=0;
	else
		bb=m_bottombw;

	lx=c->lx+GetBoxPosLeftMargin();
	rx=c->rx-GetBoxPosRightMargin();
	ty=c->ty+GetBoxPosTopMargin();
	by=c->by-GetBoxPosBottomMargin();

	cc.lx=lx;
	cc.rx=rx;
	cc.ty=ty;
	cc.by=by;
	kGUI::PushClip();
	kGUI::ShrinkClip(&cc);

	w=max(max(tb,bb),max(lb,rb));
	for(i=0;i<w;++i)
	{
		if(i<tb)
		{
			DrawLine(BORDER_TOP,i,tb,lx,ty+i,rx,ty+i+1,m_topcolor.GetColor(),m_topstyle);
			++cc.ty;
			kGUI::ShrinkClip(&cc);
		}
		if(i<bb)
		{
			DrawLine(BORDER_BOTTOM,i,bb,lx,by-1-i,rx,by-i,m_bottomcolor.GetColor(),m_bottomstyle);
			--cc.by;
			kGUI::ShrinkClip(&cc);
		}
		if(i<lb)
		{
			DrawLine(BORDER_LEFT,i,lb,lx+i,ty,lx+i+1,by,m_leftcolor.GetColor(),m_leftstyle);
			++cc.lx;
			kGUI::ShrinkClip(&cc);
		}
		if(i<rb)
		{
			DrawLine(BORDER_RIGHT,i,rb,rx-i-1,ty,rx-i,by,m_rightcolor.GetColor(),m_rightstyle);
			--cc.rx;
			kGUI::ShrinkClip(&cc);
		}
	}
	kGUI::PopClip();
}

void kGUIHTMLAttrib::SetValue(kGUIString *value)
{
	CONSTLIST_DEF **conptr;
	bool imp;

	m_value.SetString(value);

	//debug code!
#if 1
	/* this code not valid inside things like "script" etc so only check for css attributes */
	if(m_id<HTMLATT_UNKNOWN)
	{
		/* some places have !imp and others ! imp */
		while(m_value.Replace("! ","!"));

		/* !important should already be processed so if we find it here then we have an error! */
		imp=m_value.Replace("!important","",0,true)>0;
		assert(imp==false,"Should have already been removed!");
	}
#endif

	m_value.Trim();

	/* pre-calc attid if applicable */
	conptr=kGUIHTMLPageObj::GetConstPtr(&m_value);
	if(conptr)
		m_vid=(*conptr)->id;
	else
	{
		if(m_id<HTMLATT_UNKNOWN2)
		{
			switch(m_id)
			{
			case HTMLATT_HREF:
			case HTMLATT_FONT_FAMILY:
			case HTMLATT_BACKGROUND_IMAGE:
			case HTMLATT_LANG:
			break;
			default:
				if(m_value.GetLen())
				{
					if(strstr(m_value.GetString()," ") || strstr(m_value.GetString(),","))
						goto show;
					switch(m_value.GetChar(0))
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '-':
					case '+':
					case '#':
					case '.':
					break;
					default:
show:					if(strnicmp(m_value.GetString(),"rgb",3) && strncmp(m_value.GetString(),"url",3))
							kGUI::Trace("Couldn't find const '%s'\n",m_value.GetString());
					break;
					}
				}
			break;
			}
		}
		m_vid=-1;
	}
}

/* this thread is running asynchronously and is used to load images and other objects */

void kGUIHTMLPageObj::LoadListThread(void)
{
	unsigned int i;
	AsyncLoadInfo ali;
	unsigned int numloading;
	Array<AsyncLoadInfo>loadinglist;
	unsigned int numdone;
	Array<AsyncLoadInfo>donelist;

	numloading=0;
	loadinglist.Init(100,25);
	numdone=0;
	donelist.Init(100,25);

	do
	{
		kGUI::Sleep(10);

		/* end task? */
		if(m_shutdown==true)
		{
			m_shutdown=false;
			return;
		}

		/* have any finished? */
		for(i=0;i<numloading;)
		{
			ali=loadinglist.GetEntry(i);
			if(ali.m_dl->GetAsyncActive()==false)
			{
				loadinglist.DeleteEntry(i,1);
				donelist.SetEntry(numdone++,ali);
				--numloading;
			}
			else
				++i;
		}

		/* abort all pending loads? */
		if(m_loadreset==true)
		{
			m_numskipped=0;

			/* throw away all pending hi priority loads */
			while(m_hiloadlist.GetIsEmpty()==false)
			{
				m_hiloadlist.Read(&ali);
				++m_numskipped;
			}

			/* throw away all pending loads */
			while(m_loadlist.GetIsEmpty()==false)
			{
				m_loadlist.Read(&ali);
				++m_numskipped;
			}
			/* don't bother with any pending callbacks */
			m_numskipped+=numdone;
			numdone=0;
			/* reset is finished */
			m_loadreset=false;
		}

		/* can we tell the page that we have loaded new items? */
		if(numdone)
		{
			if(kGUI::TryAccess()==true)
			{
				/* ok, the main thread is stopped so we can talk to it */
				for(i=0;i<numdone;++i)
				{
					ali=donelist.GetEntry(i);
					ali.m_donecallback.Call(ali.m_dl->GetStatus());
				}
				numdone=0;
				kGUI::ReleaseAccess();
			}
		}

		/* try loading more objects if we can */
		while(numloading<=MAXDOWNLOADS)
		{
			/* check hipri list first */		
			if(m_hiloadlist.GetIsEmpty()==false)
			{
				/* get handle to item to load */
				m_hiloadlist.Read(&ali);
				loadinglist.SetEntry(numloading++,ali);
				ali.m_dl->AsyncDownLoad(ali.m_dh,ali.m_dl->GetURL());
			}
			else if(m_loadlist.GetIsEmpty()==false)
			{
				/* get handle to item to load */
				m_loadlist.Read(&ali);
				loadinglist.SetEntry(numloading++,ali);
				ali.m_dl->AsyncDownLoad(ali.m_dh,ali.m_dl->GetURL());
			}
			else
				break;	/* both lists are empty */
		}
	}while(1);
}

/****************************************************************************/

void kGUIHTMLArea::SetTag(kGUIHTMLObj *tag)
{
	kGUIString url;
	kGUIHTMLAttrib *att;

	m_tag=tag;
	att=tag->FindAttrib(HTMLATT_HREF);
	if(att)
	{
		m_haslink=true;
		tag->GetPage()->MakeURL(tag->GetPage()->GetURL(),att->GetValue(),&url);
		m_linkobj.SetPage(tag->GetPage());
		m_linkobj.SetURL(&url);
		m_linkobj.SetReferrer(tag->GetPage()->GetURL());
	}
	else
		m_haslink=false;
}

/* find the map in the list of maps */

kGUIHTMLMap *kGUIHTMLPageObj::LocateMap(kGUIString *name)
{
	unsigned int i;
	unsigned int nametype;
	kGUIHTMLMap *map;
	kGUIHTMLObj *tag;
	kGUIString n;
	kGUIHTMLAttrib *att;

	n.SetString(name);
	if(n.GetChar(0)=='.')
	{
		nametype=HTMLATT_CLASS;
		n.Delete(0,1);
	}
	else if(n.GetChar(0)=='#')
	{
		nametype=HTMLATT_ID;
		n.Delete(0,1);
	}
	else
		nametype=HTMLATT_NAME;

again:;
	for(i=0;i<m_nummaps;++i)
	{
		map=m_maps.GetEntryPtr(i);
		tag=map->GetTag();
		att=tag->FindAttrib(nametype);
		if(att)
		{
			if(!stricmp(n.GetString(),att->GetValue()->GetString()))
				return(map);
		}
	}
	/* map not found! */
	if(nametype!=HTMLATT_NAME)
	{
		nametype=HTMLATT_NAME;
		goto again;
	}

	return(0);
}


/****************************************************************************/

kGUIHTMLItemCache::kGUIHTMLItemCache()
{
	m_itemcache.Init(16,sizeof(kGUIHTMLItemCacheEntry *));
	m_saved=false;
	m_size=0;
	m_mode=CACHEMODE_NOSAVE;
}

kGUIHTMLItemCache::~kGUIHTMLItemCache()
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;

	n=m_itemcache.GetNum();
	he=m_itemcache.GetFirst();
	for(i=0;i<n;++i)
	{
		icep=(kGUIHTMLItemCacheEntry **)he->m_data;
		ice=*(icep);

		/* if reload failed then this can be null */
		if(ice)
		{
			/* if we didn't save the cache info then delete it instead */
			if(!m_saved)
				kGUI::FileDelete(ice->GetFilename()->GetString());
		
			delete ice;
		}
		he=he->GetNext();
	}
}

void kGUIHTMLItemCache::SetDirectory(const char *dir)
{
	m_cachedir.SetString(dir);

	/* make directory if it doesn't already exist */
	kGUI::MakeDirectory(dir);
}

/* iterate through the cache and delete all the files */
void kGUIHTMLItemCache::Delete(void)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;

	n=m_itemcache.GetNum();
	he=m_itemcache.GetFirst();
	for(i=0;i<n;++i)
	{
		icep=(kGUIHTMLItemCacheEntry **)he->m_data;
		ice=*(icep);

		/* delete the file */
		kGUI::FileDelete(ice->GetFilename()->GetString());

		he=he->GetNext();
	}

	m_itemcache.Reset();
}

/* look in the item cache for a given url, if it exists in the cache */
/* then the filename is set and true is returned, else the filename  */
/* is empty and false is returned */
bool kGUIHTMLItemCache::Find(kGUIString *url,kGUIString *fn,kGUIString *header)
{
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();

	icep=(kGUIHTMLItemCacheEntry **)m_itemcache.Find(url->GetString());
	if(icep)
	{
		ice=*(icep);
		if(ice)
		{
			if(ice->GetHasExpired()==false)
			{
				fn->SetString(ice->GetFilename());
				header->SetString(ice->GetHeader());
				m_busymutex.UnLock();
				return(true);
			}
		}
	}
	fn->Clear();
	header->Clear();
	m_busymutex.UnLock();
	return(false);
}

/* this function is called to check for expired items in the cache */
bool kGUIHTMLItemCache::FindExpired(kGUIString *url,kGUIString *fn,kGUIString *ifmod)
{
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;
	kGUIString weekday;
	kGUIDate datetime;

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();

	icep=(kGUIHTMLItemCacheEntry **)m_itemcache.Find(url->GetString());
	if(icep)
	{
		ice=*(icep);
		if(ice)
		{
			fn->SetString(ice->GetFilename());

			/* do we have a 'last-modified' string for this file? */
			if(ice->GetLastModified()->GetLen())
			{
				ifmod->SetString(ice->GetLastModified());
				m_busymutex.UnLock();
				return(true);
			}
			else if(datetime.GetFileDate(ice->GetFilename()->GetString()))
			{
				/* get time from the file and convert to GMT */
				datetime.LocaltoGMT();
		
				weekday.SetString(datetime.GetWeekDay3(datetime.GetDayofWeek()));
				weekday.Append(", ");
				datetime.ShortDateTime(ifmod);
				ifmod->Insert(0,weekday.GetString());
				ifmod->Append(" GMT");
				m_busymutex.UnLock();
				return(true);
			}
		}
	}
	fn->Clear();
	ifmod->Clear();
	m_busymutex.UnLock();
	return(false);
}

/* an expired item is still valid so let's update the expiry date */
void kGUIHTMLItemCache::UpdateExpiry(kGUIString *url,kGUIString *expires)
{
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();
	icep=(kGUIHTMLItemCacheEntry **)m_itemcache.Find(url->GetString());
	if(icep)
	{
		ice=*(icep);
		
		/* update the expiry */
		ice->SetExpires(expires);	
	}
	m_busymutex.UnLock();
}

/* this function takes the memory based datahandle and saves the file to */
/* the current cache directory, it then saves a referece to it in the */
/* item cache hash table */

void kGUIHTMLItemCache::Add(kGUIString *url,kGUIString *expires,kGUIString *lastmod,DataHandle *dh)
{
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;
	int index;
	kGUIString rootfn;
	kGUIString shortfn;
	kGUIString longfn;
	DataHandle write;
	unsigned long size;
	Array<unsigned char>buf;

	if(m_mode==CACHEMODE_NOSAVE)
		return;

	size=(unsigned long)dh->GetSize();
	if(!size)
		return;		/* empty file, don't bother saving */

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();

	/* convert URL to a hashcode for using as a filename */
	rootfn.Sprintf("%08x",m_itemcache.TableIndex(url->GetString()));
	
	/* if this was in the cache but is now newer then we need to check the hashtable */

	icep=(kGUIHTMLItemCacheEntry **)m_itemcache.Find(url->GetString());
	if(icep)
	{
		ice=*(icep);
		if(ice)
		{
			m_size-=ice->GetSize();
			longfn.SetString(ice->GetFilename());
		}
	}
	else
		ice=0;

	if(!ice)
	{
		index=0;
		do
		{
			/* generate a filename and if it exists then increment the extension until it doesn't*/
			shortfn.Sprintf("%S.%d",&rootfn,index++);
			kGUI::MakeFilename(&m_cachedir,&shortfn,&longfn);		
		}while(kGUI::FileExists(longfn.GetString())==true);
	}

	/* generate the cache file */
	write.SetFilename(longfn.GetString());
	write.OpenWrite("wb",size);
	/* assume datahandle is a memory based datahandle */
	write.Write(dh->GetBufferPtr(),size);
	write.Close();

	if(!ice)
	{
		ice=new kGUIHTMLItemCacheEntry();
		if(icep)
			*(icep)=ice;
		else
			m_itemcache.Add(url->GetString(),&ice);
	}
	ice->SetURL(url);
	ice->SetExpires(expires);
	ice->SetFilename(&longfn);	
	ice->SetSize(size);
	ice->SetLastModified(lastmod);

	m_size+=size;

	/* if this will make cache larger than available space then remove oldest until room */
	CheckCacheSize();

	/* unlock */
	m_busymutex.UnLock();
}

/* trigerred when user does a reload all on a page */
void kGUIHTMLItemCache::Purge(kGUIString *url)
{
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();

	icep=(kGUIHTMLItemCacheEntry **)m_itemcache.Find(url->GetString());
	if(icep)
	{
		ice=*(icep);
		if(ice)
		{
			kGUI::FileDelete(ice->GetFilename()->GetString());
			*(icep)=0;
		}
	}
	/* unlock */
	m_busymutex.UnLock();
}

/* load item cache info from the supplied xml file */
/* populate the item cache hash table with the values */

void kGUIHTMLItemCache::Load(kGUIXMLItem *root)
{
	unsigned int i,n,size;
	kGUIXMLItem *group;
	kGUIXMLItem *entry;
	kGUIHTMLItemCacheEntry *ice;
	kGUIString *filename;
	DataHandle dh;

	group=root->Locate("browsercache");
	if(!group)
		return;
	n=group->GetNumChildren();
	for(i=0;i<n;++i)
	{
		entry=group->GetChild(i);

		/* make sure file exists and is correct size, ignore if not there */
		filename=entry->Locate("filename")->GetValue();
		size=entry->Locate("size")->GetValueInt();
		dh.SetFilename(filename);
		if(dh.GetSize()==size)
		{
			ice=new kGUIHTMLItemCacheEntry();
	
			ice->SetFilename(filename);
			ice->SetExpires(entry->Locate("expires")->GetValue());
			if(entry->Locate("lastmod"))
				ice->SetLastModified(entry->Locate("lastmod")->GetValue());
			if(entry->Locate("header"))
				ice->SetHeader(entry->Locate("header")->GetValue());
			ice->SetURL(entry->Locate("url")->GetValue());
			ice->SetSize(size);
			m_itemcache.Add(ice->GetURL()->GetString(),&ice);
			m_size+=size;
		}
		else
		{
			/* if it exists and doesn't match then delete it */
			if(kGUI::FileExists(filename->GetString()))
				kGUI::FileDelete(filename->GetString());
		}
	}
}

int kGUIHTMLItemCache::Sort(const void *v1,const void *v2)
{
	kGUIHTMLItemCacheEntry *i1;
	kGUIHTMLItemCacheEntry *i2;

	i1=*((kGUIHTMLItemCacheEntry **)v1);
	i2=*((kGUIHTMLItemCacheEntry **)v2);

	return(i2->GetExpires()->GetDiffSeconds(i1->GetExpires()));
}


/* if the size is currently too large, then remove items until we are under or */
/* no items are left */
void kGUIHTMLItemCache::CheckCacheSize(void)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;
	Array<kGUIHTMLItemCacheEntry *>list;

	if(m_size<=m_maxsize)
		return;

	n=m_itemcache.GetNum();
	if(!n)
		return;

	/* ok, build an array of ice pointers, then sort based on expiry date and remove oldest first */
	list.Alloc(n);
	he=m_itemcache.GetFirst();
	for(i=0;i<n;++i)
	{
		icep=(kGUIHTMLItemCacheEntry **)he->m_data;
		ice=*(icep);

		list.SetEntry(i,ice);
		he=he->GetNext();
	}
	
	/* sort from oldest to newest */
	list.Sort(n,Sort);

	i=0;
	while(i<n && (m_size>m_maxsize))
	{
		ice=list.GetEntry(i++);
		kGUI::FileDelete(ice->GetFilename()->GetString());
		m_size-=ice->GetSize();
		delete ice;
	}

	/* rebuild the hashtable */
	m_itemcache.Reset();
	for(;i<n;++i)
	{
		ice=list.GetEntry(i);
		m_itemcache.Add(ice->GetURL()->GetString(),&ice);
	}
}

/* save the item cache info from the hash table into the */
/* supplied xml file */

void kGUIHTMLItemCache::Save(kGUIXMLItem *root)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLItemCacheEntry **icep;
	kGUIHTMLItemCacheEntry *ice;
	kGUIXMLItem *group;
	kGUIXMLItem *entry;
	kGUIString e;

	if(m_mode!=CACHEMODE_SAVE)
		return;

	/* this can be called in a threaded callback so we need to stop re-entry */
	m_busymutex.Lock();

	/* tell the destructor that we have saved the cache so don't delete it */
	m_saved=true;

	group=new kGUIXMLItem();
	group->SetName("browsercache");
	root->AddChild(group);

	n=m_itemcache.GetNum();
	he=m_itemcache.GetFirst();
	for(i=0;i<n;++i)
	{
		icep=(kGUIHTMLItemCacheEntry **)he->m_data;
		ice=*(icep);

		/* if user did a "reload" on this item and we never got it again, then */
		/* this can be a null pointer */
		if(ice)
		{
			entry=group->AddChild("entry");
			entry->AddParm("url",ice->GetURL());
			entry->AddParm("filename",ice->GetFilename());
			ice->GetExpires(&e);
			entry->AddParm("expires",&e);
			entry->AddParm("size",(int)ice->GetSize());
			if(ice->GetLastModified()->GetLen())
				entry->AddParm("lastmod",ice->GetLastModified());
			if(ice->GetHeader()->GetLen())
				entry->AddParm("header",ice->GetHeader());
		}

		he=he->GetNext();
	}

	m_busymutex.UnLock();
}

/****************************************************************************/

kGUIHTMLVisitedCache::kGUIHTMLVisitedCache()
{
	/* default to 30 days */
	m_numdays=30;
	m_visitedcache.Init(12,sizeof(kGUIHTMLVisitedCacheEntry *));
}

kGUIHTMLVisitedCache::~kGUIHTMLVisitedCache()
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLVisitedCacheEntry **vcep;
	kGUIHTMLVisitedCacheEntry *vce;

	n=m_visitedcache.GetNum();
	he=m_visitedcache.GetFirst();
	for(i=0;i<n;++i)
	{
		vcep=(kGUIHTMLVisitedCacheEntry **)he->m_data;
		vce=*(vcep);

		delete vce;
		he=he->GetNext();
	}
}

/* look in the item cache for a given url, if it exists in the cache */
/* then return true */
bool kGUIHTMLVisitedCache::Find(kGUIString *url)
{
	kGUIHTMLVisitedCacheEntry **vcep;

	vcep=(kGUIHTMLVisitedCacheEntry **)m_visitedcache.Find(url->GetString());
	if(vcep)
		return(true);
	return(false);
}

/* look in the item cache for a given url, if it exists in the cache */
/* then update the visited time, else add it */
void kGUIHTMLVisitedCache::Add(kGUIString *url)
{
	kGUIHTMLVisitedCacheEntry **vcep;
	kGUIHTMLVisitedCacheEntry *vce;
	kGUIDate d;
	kGUIString ds;

	d.SetToday();
	d.ShortDateTime(&ds);

	vcep=(kGUIHTMLVisitedCacheEntry **)m_visitedcache.Find(url->GetString());
	if(vcep)
	{
		vce=*(vcep);
		vce->SetVisited(&ds);
		return;
	}
	vce=new kGUIHTMLVisitedCacheEntry();
	m_visitedcache.Add(url->GetString(),&vce);
	vce->SetURL(url);
	vce->SetVisited(&ds);
}

/* load item cache info from the supplied xml file */
/* populate the item cache hash table with the values */

void kGUIHTMLVisitedCache::Load(kGUIXMLItem *root)
{
	unsigned int i,n;
	kGUIXMLItem *group;
	kGUIXMLItem *entry;
	kGUIHTMLVisitedCacheEntry *vce;
	kGUIDate stale;
	kGUIDate entrydate;

	/* only load if visited is after the "stale" date */
	stale.SetToday();
	stale.AddDays(-m_numdays);

	group=root->Locate("browservisited");
	if(!group)
		return;
	n=group->GetNumChildren();
	for(i=0;i<n;++i)
	{
		entry=group->GetChild(i);

		entrydate.Set(entry->Locate("visited")->GetValue()->GetString());
		if(entrydate.GetDiffSeconds(&stale)<=0)
		{
			vce=new kGUIHTMLVisitedCacheEntry();
			vce->SetURL(entry->Locate("url")->GetValue());
			vce->SetVisited(entry->Locate("visited")->GetValue());
			m_visitedcache.Add(vce->GetURL()->GetString(),&vce);
		}
	}
}

/* save the item cache info from the hash table into the */
/* supplied xml file */

void kGUIHTMLVisitedCache::Save(kGUIXMLItem *root)
{
	unsigned int i,n;
	HashEntry *he;
	kGUIHTMLVisitedCacheEntry **vcep;
	kGUIHTMLVisitedCacheEntry *vce;
	kGUIXMLItem *group;
	kGUIXMLItem *entry;
	kGUIDate stale;
	kGUIDate entrydate;

	/* only save days if after the "stale" date */
	stale.SetToday();
	stale.AddDays(-m_numdays);

	group=new kGUIXMLItem();
	group->SetName("browservisited");
	root->AddChild(group);

	n=m_visitedcache.GetNum();
	he=m_visitedcache.GetFirst();
	for(i=0;i<n;++i)
	{
		vcep=(kGUIHTMLVisitedCacheEntry **)he->m_data;
		vce=*(vcep);

		entrydate.Set(vce->GetVisited()->GetString());
		if(entrydate.GetDiffSeconds(&stale)<=0)
		{
			entry=group->AddChild("entry");
			entry->AddParm("url",vce->GetURL());
			entry->AddParm("visited",vce->GetVisited());
		}
		he=he->GetNext();
	}
}

void kGUIHTMLTextGroup::Split(unsigned int whitespace,unsigned int transform,kGUIHTMLObj *renderparent,kGUIHTMLObj *parent)
{
	unsigned int i;
//	bool startcr;
	int pluscr;
	bool endspace=false;
	kGUIHTMLObj *textobj;
	int w,numwords;
	kGUIStringSplit ss;

	/* delete any old chunks, the destructor altomatically removes it from it's parents linked lists */
	for(i=0;i<m_numchunks;++i)
	{
		textobj=m_chunks.GetEntry(i);

		delete textobj->m_obj.m_textobj;
		delete textobj;
	}
	m_numchunks=0;

	m_tempstring.SetString(this);

	ss.SetTrim(false);		/* leave leading and trailing spaces/whitespace */
	ss.SetIgnoreEmpty(false);
	if(whitespace==WHITESPACE_PRE || whitespace==WHITESPACE_PREWRAP)
	{
		/* leave white space but break on c/r */
//		m_tempstring.Replace("\r\n","\n");
//		m_tempstring.Replace("\r","\n");
		numwords=ss.Split(&m_tempstring,"\n",0,false);
		pluscr=numwords-1;
		goto addwords;
	}
	else if(whitespace==WHITESPACE_PRELINE)
	{
		/* collapse white space but break on c/r */
		m_tempstring.Replace("\t"," ");
		while(m_tempstring.Replace("  "," ")>0);

//		m_tempstring.Replace("\r\n","\n");
//		m_tempstring.Replace("\r","\n");

		numwords=ss.Split(&m_tempstring,"\n",0,false);
		pluscr=numwords-1;
		goto addwords;
	}
	else
	{
		/* change any inside white space to actual spaces */
		m_tempstring.Replace("\t"," ");
		m_tempstring.Replace("\n"," ");
#if 0
		if(m_tempstring.GetChar(0)==' ')
			startcr=false;
		else
			startcr=true;
		m_tempstring.Replace("\t"," ");
		m_tempstring.Replace("\r\n"," ");
		m_tempstring.Replace("\r"," ");
		m_tempstring.Replace("\n"," ");
		if(m_tempstring.GetChar(0)==' ' && startcr)
			m_tempstring.Delete(0,1);
#endif
		while(m_tempstring.Replace("  "," ")>0);

		if(m_tempstring.GetLen())
		{
			/* ignore single spaces, only trailing spaces are collected! */
			if(!(m_tempstring.GetChar(0)==' ' && m_tempstring.GetLen()==1))
			{
				if(m_tempstring.GetChar(m_tempstring.GetLen()-1)==' ')
					endspace=true;

				if(whitespace==WHITESPACE_NOWRAP)
					numwords=0;
				else
					numwords=ss.Split(&m_tempstring," ",0,false);
				pluscr=0;
addwords:;
				if(!numwords)
				{
					textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
					textobj->SetString(&m_tempstring,false,pluscr>0);
			//		textobj->m_obj.m_textobj->SetShadow(shadowr,shadowx,shadowy,shadowcolor);
					switch(transform)
					{
					case TEXTTRANSFORM_UPPERCASE:
						textobj->m_obj.m_textobj->Upper();
					break;
					case TEXTTRANSFORM_LOWERCASE:
						textobj->m_obj.m_textobj->Lower();
					break;
					case TEXTTRANSFORM_CAPITALIZE:
						textobj->m_obj.m_textobj->Proper(true);
					break;
					}
					parent->AddStyleChild(textobj);
					m_chunks.SetEntry(m_numchunks++,textobj);
				}
				else
				{
					for(w=0;w<numwords;++w)
					{
						textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
						textobj->SetString(ss.GetWord(w),w<numwords-1?true:endspace,pluscr>0);
						--pluscr;
					//	textobj->m_obj.m_textobj->SetShadow(shadowr,shadowx,shadowy,shadowcolor);
						switch(transform)
						{
						case TEXTTRANSFORM_UPPERCASE:
							textobj->m_obj.m_textobj->Upper();
						break;
						case TEXTTRANSFORM_LOWERCASE:
							textobj->m_obj.m_textobj->Lower();
						break;
						case TEXTTRANSFORM_CAPITALIZE:
							textobj->m_obj.m_textobj->Proper(true);
						break;
						}

						parent->AddStyleChild(textobj);
						m_chunks.SetEntry(m_numchunks++,textobj);
					}
				}
			}
		}
	}
	m_lastwhitespace=whitespace;
	m_lasttransform=transform;
}

void kGUIHTMLContentGroup::Split(kGUIHTMLObj *renderparent,kGUIHTMLObj *parent)
{
	unsigned int i;
	kGUIHTMLObj *obj;
	kGUIString word;
	unsigned int c,q;

	/* clear the changed flag */
	m_changed=false;

	/* delete any old chunks, the destructor automatically removes it from it's parents linked lists */
	for(i=0;i<m_numchunks;++i)
	{
		obj=m_chunks.GetEntry(i);

		if(obj->GetID()==HTMLTAG_IMBEDTEXT)
			delete obj->m_obj.m_textobj;
		delete obj;
	}
	m_numchunks=0;

	/* copy encoding for extracted words */
	word.SetEncoding(GetEncoding());

	/* ok now, let's parse the content string */
	Start();
	do
	{
		/* eat whitespace */
		do
		{
			c=PeekChar();
			if(kGUIString::IsWhiteSpace(c))
				ReadChar();
			else
				break;
		}while(AtEnd()==false);
		if(AtEnd()==true)
			return;

		word.Clear();

		/* is this quoted text? */
		if(c=='\"' || c=='\'')
		{
			ReadChar();
			q=c;		/* save quote so we know when to stop */
			do
			{
				c=ReadChar();
				if(c=='\\')
					c=ReadChar();
				else if(c==q || AtEnd()==true)
					break;
				word.Append(c);
			}while(1);

			/* todo: hmm, handle whitespace rules so multiple spaces become 1 space etc. */

			/* insert a text object */
			{
				kGUIHTMLObj *textobj;

				textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
				textobj->SetString(&word,false,false);
				parent->AddStyleChild(textobj);
				m_chunks.SetEntry(m_numchunks++,textobj);
			}
		}
		else
		{
			/* handle url,attr and other things */
			do
			{
				c=ReadChar();
				if(AtEnd()==true || c=='(' || kGUIString::IsWhiteSpace(c))
					break;
				word.Append(c);
			}while(1);
			if(word.GetLen())
			{
				/* todo: add more */
				switch(m_page->GetConstID(word.GetString()))
				{
				case HTMLCONST_ATTR:
					if(c=='(')
					{
						word.Clear();
						do
						{
							c=ReadChar();
							if(AtEnd()==true || c==')')
								break;
							word.Append(c);
						}while(1);
						/* ok, so insert this attributes value */
						if(word.GetLen() && c==')')
						{
							kGUIHTMLAttrib *att;
							att=parent->FindAttrib(m_page->StringToID(&word));
							if(att)
							{
								kGUIHTMLObj *textobj;

								/* insert a text object */
								textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
								textobj->SetString(att->GetValue(),false,false);
								parent->AddStyleChild(textobj);
								m_chunks.SetEntry(m_numchunks++,textobj);
							}
						}
					}
				break;
				case HTMLCONST_OPEN_QUOTE:
				case HTMLCONST_CLOSE_QUOTE:
				{
					kGUIHTMLObj *textobj;
					kGUIString s;

					/* todo: handle user defined quote characters */

					/* insert a text object */
					textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
					s.SetString("\"");
					textobj->SetString(&s,false,false);
					parent->AddStyleChild(textobj);
					m_chunks.SetEntry(m_numchunks++,textobj);
				}
				break;
				case HTMLCONST_URL:
					if(c=='(')
					{
						word.Clear();
						do
						{
							c=ReadChar();
							if(AtEnd()==true || c==')')
								break;
							word.Append(c);
						}while(1);
						/* ok, so insert this attributes value */
						if(word.GetLen() && c==')')
						{
							kGUIHTMLObj *imgobj;
							kGUIString fullurl;

							/* insert an image object */
							imgobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_imgtag);
							imgobj->m_insert=true;
							m_page->MakeURL(m_page->GetURL(),&word,&fullurl);
							imgobj->m_obj.m_imageobj->SetURL(m_page,&fullurl,m_page->GetURL());

							renderparent->AddObject(imgobj);
							parent->AddStyleChild(imgobj);
							m_chunks.SetEntry(m_numchunks++,imgobj);
						}
					}
				break;
				default:
					m_page->m_errors.ASprintf("Unhandled content='%s'\n",word.GetString());
				break;
				}
			}
		}
	}while(AtEnd()==false);
#if 0
	numwords=0;
	if(!numwords)
	{
		textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
		textobj->SetString(this,false,false);
		parent->AddStyleChild(textobj);
		m_chunks.SetEntry(m_numchunks++,textobj);
	}
	else
	{
		for(w=0;w<numwords;++w)
		{
			textobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_texttag);
			textobj->SetString(this,false,false);
			parent->AddStyleChild(textobj);
			m_chunks.SetEntry(m_numchunks++,textobj);
		}
	}
#endif
}

void kGUIHTMLLIPrefix::Purge(void)
{
	unsigned int i;
	kGUIHTMLObj *obj;
	kGUIHTMLObj *sp;

	for(i=0;i<m_numchunks;++i)
	{

		obj=m_chunks.GetEntry(i);
		sp=obj->m_styleparent;

		delete obj;
		//obj->m_styleparent->DelStyleChild(obj);
	}
	m_numchunks=0;
}

void kGUIHTMLLIPrefix::Set(unsigned int style,unsigned int position,unsigned int urlid,unsigned int urlrefererid,kGUIHTMLObj *renderparent,kGUIHTMLObj *parent)
{
	kGUIHTMLObj *obj;
	kGUIString s;

	/* delete any previously added chunks */
	Purge();

	m_renderparent=renderparent;
	m_style=style;
	m_position=position;
	m_urlid=urlid;
	m_urlrefererid=urlrefererid;

	switch(m_style)
	{
	case LISTSTYLE_NONE:
		return;					/* nothing to insert */
	break;
	case LISTSTYLE_IMAGE:
	{
		kGUIHTMLObj *cobj;

		cobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_imgtag);
		cobj->m_insert=true;
		cobj->m_obj.m_imageobj->SetURL(m_page,m_page->IDToString(m_urlid),m_page->IDToString(m_urlrefererid));

		renderparent->AddObject(cobj);
		parent->InsertStyleChild(0,cobj);
		m_chunks.SetEntry(m_numchunks++,cobj);
		return;
	}
	break;
	case LISTSTYLE_DISC:
	case LISTSTYLE_CIRCLE:
	case LISTSTYLE_SQUARE:
	{
		kGUIHTMLObj *cobj;

		cobj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_lishapetag);
		cobj->m_insert=true;
		cobj->m_obj.m_shapeobj->SetType(m_style);

		renderparent->AddObject(cobj);
		parent->InsertStyleChild(0,cobj);
		m_chunks.SetEntry(m_numchunks++,cobj);
		return;
	}
	break;
	case LISTSTYLE_DECIMAL:
		s.Sprintf("%d.",m_index);
	break;
	case LISTSTYLE_DECIMAL_LEADING_ZERO:
		s.Sprintf("%02d.",m_index);
	break;
	case LISTSTYLE_LOWER_ROMAN:
	case LISTSTYLE_UPPER_ROMAN:
	{
		int rvals[]= { 1000,500,100, 50, 10,  5, 1,  0  };
		int rvalsx[]= {   0,  0,  1,  0,  1,  0, 1,  0 };
		char rchars[]={  'm', 'd','c','l','x','v','i','.'};
		int value;
		int offset;

		offset=0;
		value=m_index;
		do
		{
			if(value>=rvals[offset])
			{
				if((value>=rvals[offset+1]*9) && rvalsx[offset+1])
				{
					/* special case for 9 */
					value-=rvals[offset+1]*9;
					s.Append(rchars[offset+1]);
					s.Append(rchars[offset-1]);
				}
				else if((value>=rvals[offset]*4) && rvalsx[offset])
				{
					/* special case for 4 */
					value-=rvals[offset]*4;
					s.Append(rchars[offset]);
					s.Append(rchars[offset-1]);
				}
				else
				{
					value-=rvals[offset];
					s.Append(rchars[offset]);
				}
			}
			else
				++offset;
		}while(value);
		s.Append('.');
	}
	break;
	case LISTSTYLE_LOWER_ALPHA:
	case LISTSTYLE_UPPER_ALPHA:
		if(m_index<=26)
			s.Sprintf("%c.",m_index+('a'-1));
		else
			s.Sprintf("%c%c.",(m_index/26)+('a'-1),(m_index%26)+('a'-1));
	break;
	case LISTSTYLE_LOWER_GREEK:
		s.Sprintf("%02d.",m_index);
	break;
	}

	/* convert lowers to upper */
	switch(m_style)
	{
	case LISTSTYLE_UPPER_ALPHA:
	case LISTSTYLE_UPPER_ROMAN:
		s.Upper();
	break;
	}

	s.Append(' ');
	obj=new kGUIHTMLObj(renderparent,m_page,&kGUIHTMLPageObj::m_litexttag);
	obj->m_insert=true;
	obj->SetString(&s,false,false);
	obj->m_obj.m_textobj->SetFixedWidth(3);	/* in ems */
	obj->m_obj.m_textobj->SetHAlign(FT_RIGHT);

	parent->InsertStyleChild(0,obj);
	m_chunks.SetEntry(m_numchunks++,obj);
}

/* load html settings from the XML file */

void kGUIHTMLSettings::Load(kGUIXMLItem *group)
{
	m_usercss.SetString(group->Locate("usercss")->GetValue());
	m_drawboxes=group->Locate("drawboxes")->GetValueInt()?true:false;
	m_drawareas=group->Locate("drawareas")->GetValueInt()?true:false;
	if(group->Locate("usecss"))
		m_usecss=group->Locate("usecss")->GetValueInt()?true:false;
	else
		m_usecss=true;

	if(group->Locate("useusercss"))
		m_useusercss=group->Locate("useusercss")->GetValueInt()?true:false;
	else
		m_useusercss=true;

	if(group->Locate("loadimages"))
		m_loadimages=group->Locate("loadimages")->GetValueInt()?true:false;
}

/* save html settings to the XML file */

void kGUIHTMLSettings::Save(kGUIXMLItem *group)
{
	group->AddChild("loadimages",m_loadimages==true?1:0);
	group->AddChild("usercss",&m_usercss);
	group->AddChild("drawboxes",m_drawboxes==true?1:0);
	group->AddChild("drawareas",m_drawareas==true?1:0);
	group->AddChild("usecss",m_usecss==true?1:0);
	group->AddChild("useusercss",m_useusercss==true?1:0);
}

void kGUIHTMLShapeObj::Draw(void)
{
	kGUICorners c;
	int size,gap,r;

	kGUI::PushClip();
	GetCorners(&c);
	kGUI::ShrinkClip(&c);
	
	/* is there anywhere to draw? */
	if(kGUI::ValidClip())
	{
		size=c.by-c.ty;
		switch(m_type)
		{
		case LISTSTYLE_DISC:
			gap=(int)(size*0.25f);
			r=(size-(gap<<1))>>1;
			kGUI::DrawCircle(c.lx+gap+r,c.ty+gap+r,r,DrawColor(0,0,0),1.0f);			
		break;
		case LISTSTYLE_CIRCLE:
			gap=(int)(size*0.25f);
			r=(size-(gap<<1))>>1;
			kGUI::DrawCircleOutline(c.lx+gap+r,c.ty+gap+r,r,(int)(size*0.1f),DrawColor(0,0,0),1.0f);			
		break;
		case LISTSTYLE_SQUARE:
			gap=(int)(size*0.35f);
			kGUI::DrawRect(c.lx+gap,c.ty+gap,c.rx-size-gap,c.by-gap,DrawColor(0,0,0));
		break;
		}
	}
	kGUI::PopClip();
}

#if 1

void kGUIHTMLPageObj::GetTagDesc(kGUIHTMLObj *obj,kGUIString *ts)
{
	unsigned int i;
	CID_DEF *cid;

	if(obj->m_tag)
		ts->Sprintf("<%s",obj->m_tag->name);
	else
		ts->Sprintf("<%d",obj->m_id);

	cid=obj->m_cids.GetArrayPtr();
	for(i=0;i<obj->m_numcids;++i)
	{
		if(cid->m_type==CID_CLASS)
			ts->ASprintf(" class=\"%S\"",IDToString(cid->m_id));
		else
			ts->ASprintf(" id=\"%S\"",IDToString(cid->m_id));
		++cid;

	}
	ts->Append(">");
}


void kGUIHTMLPageObj::TraceBack(kGUIHTMLObj *obj)
{
	kGUIString tdesc;
	unsigned int bw;

	GetTagDesc(obj,&tdesc);
	if(!obj->m_box)
		bw=0;
	else
		bw=obj->m_box->GetBoxLeftWidth()+obj->m_box->GetBoxRightWidth();

	kGUI::Trace("Trace %s minw=%d,maxw=%d,borderw=%d\n",tdesc.GetString(),obj->m_minw,obj->m_maxw,bw);

	if(obj->m_renderparent)
		TraceBack(obj->m_renderparent);
}
#endif

/* call updateinput for all children in the specified group */
/* since children can be totally outside of the parent area we will */
/* not do any position culling */

bool kGUIHTMLPageObj::UpdateInputC(int num)
{
	int e,nc;
	bool objret;
	kGUIObj *gobj;

	nc=GetNumChildren(num);
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(num,e);
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
	}
	return(false);	/* not used by and children */
}

/* call updateinput for all children in the specified group */
/* since children can be totally outside of the parent area we will */
/* not do any position culling */

bool kGUIHTMLObj::UpdateInputC(int num)
{
	int e,nc;
	bool objret;
	kGUIObj *gobj;

	nc=GetNumChildren(num);
	for(e=0;e<nc;++e)
	{
		gobj=GetChild(num,e);
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
	}
	return(false);	/* not used by and children */
}
