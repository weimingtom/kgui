#ifndef __KGUIGRID__
#define __KGUIGRID__

/* this class is for a scrolling shape grid for maps etc */

class kGUICellObj
{
public:
	kGUICellObj(class kGUIGridObj *p,int w,int h);
	void Draw(int sx,int sy);
	void Update(int cx,int cy);
	int GetX(void) {return m_cellx;}
	int GetY(void) {return m_celly;}
//	void Flush(void) {m_cellx=99999;m_celly=99999;}	/* force a redraw */
	void Flush(void) {m_dirty=true;}	/* force a redraw */
	void SetDirty(bool d) {m_dirty=d;}
	bool GetDirty(void) {return m_dirty;}
private:
	kGUIDrawSurface m_surface;
	kGUIImage m_image;
	class kGUIGridObj *m_parent;
	bool m_onscreen;
	bool m_dirty;
	int m_cellx;
	int m_celly;
};

class kGUIGridObj : public kGUIObj
{
public:
	kGUIGridObj();
	~kGUIGridObj();
	void Init(int gridwidth,int gridheight,int cellwidth,int cellheight);
	void ReSetCellSize(int cellwidth,int cellheight);
	void ReSize(int gridwidth,int gridheight);
	void SetSize(int w,int h) {kGUIObj::SetSize(w,h);ReSize(w,h);}
	kGUICellObj *GetCell(int cx,int cy);
	void FlushCell(int cx,int cy);		/* user call code to force re-draw of a particular cell */
	void ClipScrollX(void);
	void ClipScrollY(void);
	void GetScrollCorner(int *x,int *y);
	void GetScrollCenter(int *x,int *y);
	void SetScrollCorner(int x,int y);
	void SetScrollCenter(int x,int y);
	void SetBounds(int lx,int ty,int rx,int by) {m_bounds=true;m_minx=lx;m_maxx=rx;m_miny=ty;m_maxy=by;}
	void Draw(void);
	bool UpdateInput(void);
	void SetOverCallBack(void *codeobj,void (*code)(void *,int x,int y)) {m_overcallback.Set(codeobj,code);}
	void SetPreDrawCallBack(void *codeobj,void (*code)(void *)) {m_predrawcallback.Set(codeobj,code);}
	void SetPostDrawCallBack(void *codeobj,void (*code)(void *)) {m_postdrawcallback.Set(codeobj,code);}
	void SetDrawCallBack(void *codeobj,void (*code)(void *,kGUICellObj *)) {m_drawcallback.Set(codeobj,code);}
	void CallDraw(kGUICellObj *c) {m_drawcallback.Call(c);}
	void Flush(void);	/* for a total redraw */
	int GetCellWidth(void) {return m_cellwidth;}
	int GetCellHeight(void) {return m_cellheight;}
	void SetPrintMode(bool mode) {m_printmode=mode;}
private:
	bool m_printmode;
	int m_gridwidth;
	int m_gridheight;
	int m_cellwidth;
	int m_cellheight;
	int m_scrollx;
	int m_scrolly;
	int m_scrollxfrac;
	int m_scrollyfrac;
	bool m_bounds;
	int m_minx,m_maxx,m_miny,m_maxy;

	int m_numcells;
	Array<kGUICellObj *>m_cells;

	int m_numdraw;
	int m_numdrawx;
	int m_numdrawy;
	Array<kGUICellObj *>m_drawcells;

	kGUICallBack m_predrawcallback;	/* called once before drawing */
	kGUICallBack m_postdrawcallback;	/* called once after drawing */
	kGUICallBackPtr<kGUICellObj> m_drawcallback;	/* called for each cell that needs to be redrawn */
	kGUICallBackIntInt m_overcallback;	/* continously called with releative mouse poustion under the pointer */
};

#endif
