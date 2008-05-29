/* all gui skin shapes need an index in this table */
enum
{
SKINCOLOR_BACKGROUND,
SKINCOLOR_WINDOWBACKGROUND,
SKINCOLOR_CONTAINERBACKGROUND,
SKINCOLOR_MAX
};

enum
{
SKIN_WINDOWTOPLEFT,		/* shapes for window frame pieces */
SKIN_WINDOWTOPMIDDLE,
SKIN_WINDOWTOPRIGHT,
SKIN_WINDOWLEFTSIDE,
SKIN_WINDOWRIGHTSIDE,
SKIN_WINDOWBOTTOMLEFT,
SKIN_WINDOWBOTTOMMIDDLE,
SKIN_WINDOWBOTTOMRIGHT,
SKIN_MINIMIZEDWINDOW,

SKIN_WINDOWCLOSEBUTTON,
SKIN_WINDOWFULLBUTTON,
SKIN_WINDOWMINIMIZEBUTTON,

SKIN_WINDOWCLOSEBUTTONOVER,
SKIN_WINDOWFULLBUTTONOVER,
SKIN_WINDOWMINIMIZEBUTTONOVER,

SKIN_TICK,					/* tick for tickbox object */

SKIN_TABLEFT,
SKIN_TABCENTER,
SKIN_TABRIGHT,

SKIN_SCROLLBARVERTTOP,
SKIN_SCROLLBARVERTCENTER,
SKIN_SCROLLBARVERTBOTTOM,

SKIN_SCROLLBARVERTSLIDERTOP,
SKIN_SCROLLBARVERTSLIDERCENTER,
SKIN_SCROLLBARVERTSLIDERLINE,
SKIN_SCROLLBARVERTSLIDERBOTTOM,

SKIN_SCROLLBARHORIZLEFT,
SKIN_SCROLLBARHORIZCENTER,
SKIN_SCROLLBARHORIZRIGHT,

SKIN_SCROLLBARHORIZSLIDERLEFT,
SKIN_SCROLLBARHORIZSLIDERCENTER,
SKIN_SCROLLBARHORIZSLIDERLINE,
SKIN_SCROLLBARHORIZSLIDERRIGHT,

SKIN_TABSELECTEDLEFT,
SKIN_TABSELECTEDCENTER,
SKIN_TABSELECTEDRIGHT,

SKIN_TABLEROWMARKER,
SKIN_TABLEROWMARKERSELECTED,
SKIN_TABLEROWNEW,

SKIN_COMBODOWNARROW,

SKIN_RADIOUNSELECTED,
SKIN_RADIOSELECTED,

SKIN_WINDOWICON,	/* user set shape */

SKIN_NUMSHAPES};

/* the default skin constructor */
class DefSkin : public kGUISkin
{
public:
	DefSkin();
	~DefSkin();
	/* window */
	void SetWindowIcon(const char *fn) {SetShape(SKIN_WINDOWICON,fn);}
	void GetWindowEdges(kGUICorners *c);
	void GetMinimizedWindowSize(int *w,int *h);
	void GetWindowButtonPositions(int allow,kGUICorners *c,kGUICorners *wclose,kGUICorners *wfull,kGUICorners *wminimize);
	void DrawWindow(kGUIWindowObj *obj,kGUICorners *c,int allow,int over);
	void DrawWindowNoFrame(kGUIWindowObj *obj,kGUICorners *c);
	void DrawBusy(kGUICorners *c);
	/* tab */
	void GetTabSize(int *expand,int *left,int *right,int *height);
	void DrawTab(kGUIText *text,int x,int y,bool current,bool over);
	/* Scrollbars */
	void GetScrollbarSize(bool isvert,int *lt,int *br,int *wh);
	void DrawScrollbar(bool isvert,kGUICorners *c,kGUICorners *sc,bool showends);
	int GetScrollbarWidth(void);
	int GetScrollbarHeight(void);
	/* table/menu */
	int GetMenuRowHeaderWidth(void);
	int GetTableRowHeaderWidth(void);
//	int GetTableColHeaderHeight(void);

	void DrawTableRowHeader(kGUICorners *c,bool selected,bool cursor,bool add);
	void DrawMenuRowHeader(kGUICorners *c);
	/* tickbox */
	void DrawTickbox(kGUICorners *c,bool scale,bool selected,bool current);
	void GetTickboxSize(int *w,int *h);
	/* radio */
	void DrawRadio(kGUICorners *c,bool scale,bool selected,bool current);
	void GetRadioSize(int *w,int *h);
	/* combo box */
	int GetComboArrowWidth(void);
	void DrawComboArrow(kGUICorners *c);

	/* draw left button, gap, right button */
	int GetScrollHorizButtonWidths(void);
	void DrawScrollHoriz(kGUICorners *c);

	/* allow user to override the default shapes */
	void SetShape(int index,const char *fn) {m_skinshapes[index].SetFilename(fn);}
	void SetColor(int index,kGUIColor c) {m_skincolors[index]=c;}
private:
	kGUIImage *GetShape(int index) {return m_skinshapes+index;}
	kGUIColor GetColor(int index) {return m_skincolors[index];} 

	BigFile m_bf;
	kGUIImage m_skinshapes[SKIN_NUMSHAPES];
	kGUIColor m_skincolors[SKINCOLOR_MAX];
};
