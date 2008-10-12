#ifndef __KGUIREPORT__
#define __KGUIREPORT__

/*! @defgroup kGUIReportObjects kGUIReportObjects */ 

/*! This is the base class for all report objects 
    @ingroup kGUIReportObjects */
class kGUIReportObj: public kGUIZone
{
public:
	kGUIReportObj() {m_enabled=true;m_page=0;}
	virtual ~kGUIReportObj() {};
	virtual void Draw(void)=0;
	inline void SetEnabled(bool e) {m_enabled=e;}
	inline bool GetEnabled(void) {return m_enabled;}
	inline void GetCorners(kGUICorners *gc) {gc->lx=GetZoneX();gc->ty=GetZoneY();gc->rx=GetZoneRX();gc->by=GetZoneBY();} 
	inline void SetName(const char *n) {m_name.SetString(n);}
	inline const char *GetName(void) {return m_name.GetString();}
	inline void SetPage(int p) {m_page=p;}
	inline int GetPage(void) {return m_page;}

private:
	bool m_enabled:1;
	unsigned int m_page:16;
	kGUIString m_name;
};

/*! this is the tickbox report object
    @ingroup kGUIReportObjects */
class kGUIReportTickboxObj : public kGUIReportObj
{
public:
	kGUIReportTickboxObj() {m_selected=false;m_scale=false;SetSize(13,13);};
	void SetSelected(bool s) {m_selected=s;}
	void SetScale(bool s) {m_scale=s;}	/* draw 13x13 or expand to fit */
	void Draw(void);
private:
	bool m_selected:1;
	bool m_scale:1;
};

/*! this is the image report object
    @ingroup kGUIReportObjects */
class kGUIReportImageObj : public kGUIReportObj, public kGUIImage
{
public:
	kGUIReportImageObj() {m_currentframe=0;m_leftoff=0;m_topoff=0;}
	double CalcScaleToFit(void);
	void ShrinkToFit(void);		/* only will make smaller, not bigger */
	void ExpandToFit(void);		/* only will make bigger, not smaller */
	void ScaleToFit(void);		/* will make either bigger or smaller */
	void CenterImage(void);
	void Draw(void);
	void SetCurrentFrame(unsigned int f) {m_currentframe=f;}
	unsigned int GetCurrentFrame(void) {return  m_currentframe;}
private:
	int m_currentframe;
	int m_leftoff,m_topoff;
};

/*! this is the image reference report object. It allows multiple referecnes to shared images.
    @ingroup kGUIReportObjects */
class kGUIReportImageRefObj : public kGUIReportObj
{
public:
	kGUIReportImageRefObj() {m_image=0;m_scalex=1.0f;m_scaley=1.0f;}
	void Draw(void);
	void SetImage(kGUIImage *image) {m_image=image;}
	void SetScale(double xscale,double yscale) {m_scalex=xscale;m_scaley=yscale;}
	double GetScaleX(void) {return m_scalex;}
	double GetScaleY(void) {return m_scaley;}
private:
	kGUIImage *m_image;
	double m_scalex,m_scaley;
};

/*! this is the rectangle report object
    @ingroup kGUIReportObjects */
class kGUIReportRectObj : public kGUIReportObj
{
public:
	kGUIReportRectObj() {m_color=0;m_framepix=0;}
	void SetFramePixels(int p) {m_framepix=p;}
	void SetColor(kGUIColor c) {m_color=c;}
	void SetFrameColor(kGUIColor c) {m_framecolor=c;}
	void Draw(void);
private:
	int m_framepix;
	kGUIColor m_color;
	kGUIColor m_framecolor;
};

/*! this is the static text report object
    @ingroup kGUIReportObjects */
class kGUIReportTextObj : public kGUIReportObj, public kGUIText
{
public:
	kGUIReportTextObj();
	void SetFrameColor(kGUIColor c) {m_framecolor=c;}
	void SetTextColor(kGUIColor c) {SetColor(c);}
	void SetBGColor(kGUIColor c) {m_bgcolor=c;m_usebgcolor=true;}
	void SetBGShade(bool b) {m_shaded=b;}
	void SetCurrency(bool c) {m_iscurrency=c;}
	int GetWidth(void) {return (kGUIText::GetWidth()+6);}
	int Height(int width);
	void Draw(void);
	void SetFrame(bool df);
	void SetInt(int v) {Sprintf("%d",v);}
	void SetDouble(const char *f,double v) {Sprintf(f,v);}
	void StringChanged(void) {Changed();}
	void FontChanged(void) {Changed();}
private:
	void Changed(void);
	bool m_drawframe;
	kGUIColor m_framecolor;
	kGUIColor m_bgcolor;
	bool m_shaded;
	bool m_usebgcolor;
	bool m_iscurrency;
};

/*! this is the table row header report object
    @ingroup kGUIReportObjects */
class kGUIReportRowHeaderObj : public kGUIReportObj
{
public:
	kGUIReportRowHeaderObj() {m_numcolumns=0;}
	~kGUIReportRowHeaderObj();
	/* set functions */
	void SetNumColumns(int n);
	void SetColName(int c,const char *name) {m_colnames.GetEntry(c)->SetString(name);}
	void SetColX(int c,int x) {m_colxs.SetEntry(c,x);UpdateSize();}
	void SetColWidth(int c,int w) {m_colws.SetEntry(c,w);UpdateSize();}
	/* calc col positions based on set widths */
	void CalcColPositions(int startx) {int c,x;x=startx;for(c=0;c<m_numcolumns;++c){m_colxs.SetEntry(c,x);x+=m_colws.GetEntry(c);}UpdateSize();}
	/* get functions */
	int GetNumColumns(void) {return m_numcolumns;}
	int GetColX(int c) {return m_colxs.GetEntry(c);}
	int GetColWidth(int c) {return m_colws.GetEntry(c);}
	const char *GetColName(int c) {return m_colnames.GetEntry(c)->GetString();}
private:
	void UpdateSize(void);
	void Draw(void);
	int m_numcolumns;
	Array<kGUIString *>m_colnames;
	Array<int>m_colxs;
	Array<int>m_colws;
};

/*! this is the table row report object
    @ingroup kGUIReportObjects */
class kGUIReportRowObj : public kGUIReportObj
{
public:
	kGUIReportRowObj() {m_parent=0;m_shaded=false;m_frame=true;}
	void SetHeader(kGUIReportRowHeaderObj *parent);
	kGUIReportRowHeaderObj *GetHeader(void) {return m_parent;}
	virtual kGUIReportObj **GetObjectList(void)=0;
	inline void SetBGShade(bool b) {m_shaded=b;}
	inline bool GetBGShade(void) {return m_shaded;}
	inline void SetFrame(bool f) {m_frame=f;}
	inline bool GetFrame(void) {return m_frame;}
	void SetColSpan(int col,int span) {m_colspan.SetEntry(col,span);}
	int GetColSpan(int col) {return m_colspan.GetEntry(col);}
private:
	void Draw(void);
	bool m_shaded;
	bool m_frame;
	Array<int>m_colspan;
	kGUIReportRowHeaderObj *m_parent;
};

class kGUIReportAreaObj
{
public:
	kGUIReportAreaObj() {m_numchildren=0;m_children.Alloc(64);m_children.SetGrow(true);m_children.SetGrowSize(32);}
	void AddObj(kGUIReportObj *obj);
	int GetNumChildren(void) {return m_numchildren;}
	void PurgeObjs(void);
	bool AssignToPage(int pagenum,int availheight,int start,int *end);
	void Draw(int pagenum);
	int GetHeight(int pagenum,int start,int end);
	int GetWidth(int pagenum,int start,int end);
private:
	unsigned int m_numchildren;
	Array<kGUIReportObj *>m_children;
};

enum
{
	REPORTSECTION_PAGEHEADER,
	REPORTSECTION_PAGEFOOTER,
	REPORTSECTION_HEADER,
	REPORTSECTION_BODY,
	REPORTSECTION_FOOTER
};

/* how to print a report */
/* 1) add reportobjects to page header */
/* 2) add reportobjects to page footer */
/* 3) add reportobjects to report header */
/* 4) add reportobjects to report footer */
/* 5) add reportobjects to body */


class kGUIReport
{
public:
	kGUIReport();
	kGUIReport(int defsize);
	virtual ~kGUIReport();
	void Init(void);
	void SetPID(int pid) {m_pid=pid;}
	int GetPID(void) {return m_pid;}
	void Fit(unsigned int maxwide,unsigned int maxtall);
	void Preview(void);
	bool Print(int startpage=0,int endpage=0);
	void SetAllowMultiPages(bool a) {m_allowmultipages=a;}
	void SetScale(int s) {m_scale.Sprintf("%d",s);m_drawscale=100.0f/m_scale.GetDouble();}
	void SetLandscape(bool l) {m_landscape=l;}
	bool GetLandscape(void) {return m_landscape;}
	void AddObjToSection(int section,kGUIReportObj *obj,bool purge=false);
	void AddUserControl(kGUIObj *obj) {m_usercontrollist.SetEntry(m_numusercontrols++,obj);}
	void SetNumCopies(int num) {m_numcopies=num;}
	int GetNumCopies(void) {return m_numcopies;}
	void SetBitmapMode(bool b) {m_bitmapmode=b;}

	inline int GetNumPages(void) {return m_numgrouppages;}

	void GetSubPageInfo(int subpage,int *pw,int *ph,int *poffx,int *poffy);

	void SetCloseCallBack(void *codeobj,void (*code)(void *,kGUIReport *)) {m_closecallback.Set(codeobj,code);}
	/* used in script generated reports with fields in the body that are replaced */
	/* by code generated objects */
	void PurgeBody(void) {m_body.PurgeObjs();}
	void ChangeFormat(void);

	void GetPageSizePixels(int *ppw,int *pph);

private:
	CALLBACKGLUEVAL(kGUIReport,ChangePage,int )
	CALLBACKGLUEVAL(kGUIReport,ChangeScale,int )
	CALLBACKGLUEPTR(kGUIReport,ChangeFormatEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIReport,Printer_Changed,kGUIEvent)
	CALLBACKGLUEPTR(kGUIReport,ClickPrint,kGUIEvent)
	CALLBACKGLUEVAL(kGUIReport,ClickPrint2,int)
	CALLBACKGLUEPTR(kGUIReport,WindowEvent,kGUIEvent)

	void ChangeFormatEvent(kGUIEvent *event);
	void WindowEvent(kGUIEvent *event);
	void PurgeObjs(void);

	virtual void Setup(void)=0;
	virtual void Setup(int pagenum)=0;
	virtual const char *GetName(void)=0;
	virtual int GetPPI(void)=0;				/* pixels per inch */
	virtual double GetPageWidth(void)=0;	/* inches */
	virtual double GetPageHeight(void)=0;	/* inches */
	virtual double GetLeftMargin(void)=0;	/* inches */
	virtual double GetRightMargin(void)=0;	/* inches */
	virtual double GetTopMargin(void)=0;	/* inches */
	virtual double GetBottomMargin(void)=0;	/* inches */

	int GetSurfaceWidthPix(void) {return m_drawsurface.GetWidth();}
	int GetSurfaceHeightPix(void) {return m_drawsurface.GetHeight();}

	kGUIReportAreaObj m_pageheader;
	kGUIReportAreaObj m_pagefooter;
	kGUIReportAreaObj m_header;
	kGUIReportAreaObj m_footer;
	kGUIReportAreaObj m_body;

	/* variables needed to process drawing */
	kGUIDrawSurface m_drawsurface;
	kGUIPrintJob *m_printjob;

	/* Calculate the number of pages the report will take */
	void GetSurfaceSize(double *ppw,double *pph);
	void CalcPages(void);
	void DrawPage(int grouppage);
	int m_phh;
	int m_pfh;
	int m_bhh;
	int m_bfh;

	CALLBACKGLUEPTR(kGUIReport,ScrollChangePage,kGUIEvent)
	CALLBACKGLUEPTR(kGUIReport,ScrollChangeScale,kGUIEvent)
	void ScrollChangePage(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)ChangePage(event->m_value[0].i);}
	void ScrollChangeScale(kGUIEvent *event) {if(event->GetEvent()==EVENT_AFTERUPDATE)ChangeScale(event->m_value[0].i);}

	void ChangePage(int move);
	void ChangeScale(int move);

	void ClickPrint(kGUIEvent *event);
	void ClickPrint2(int pressed);
	void Printer_Changed(kGUIEvent *event);
	void CalcPageSize(int sw,int sh);
	void PositionPage(void);
	
	kGUIWindowObj m_previewwindow;
	kGUITextObj m_pagenumcaption;
	kGUIInputBoxObj m_pagenumbox;	/* locked */
	kGUITextObj m_pagescrollcaption;
	kGUIScrollBarObj m_pagescrollbar;
	kGUITextObj m_scalescrollcaption;
	kGUIScrollBarObj m_scalescrollbar;
	kGUITextObj m_printrangecaption;
	kGUIInputBoxObj m_pagerangebox;		/* print range */
	kGUITextObj m_printcopiescaption;
	kGUIInputBoxObj m_pagecopiesbox;	/* number of boxes */
	kGUIButtonObj m_printbutton;
	kGUIComboBoxObj m_printerlist;
	kGUIComboBoxObj m_format;
	kGUITextObj m_scalecaption;
	kGUIInputBoxObj m_scale;	/* scale as a percent */
	kGUITextObj m_multicaption;
	kGUIComboBoxObj m_multi;	/* multi page printing */

	unsigned int m_numpurgeobjects;
	Array<kGUIReportObj *>m_purgeobjects;

	kGUIControlBoxObj m_controls;		/* user control area */
	kGUIControlBoxObj m_usercontrols;	/* user control area */

	kGUIImageObj m_imageobj;
	int m_defsize;
	int m_curpage;
	double m_maxscale;		/* allow user to view between 1.0 and this */
	int m_scalepercent;		/* 0 to 100 = 1.0 -> b_maxscale */ 
	bool m_okadd;			/* true only when users are allowed to add objects */
	double m_drawscale;		/* 1.0 = regular */
	int m_multipage;		/* number of pages to squeeze into a page */
	bool m_allowmultipages;

	int m_pid;
	int m_yoff;
	int m_availheight;
	int m_numpages;
	int m_numwide;
	int m_numgrouppages;
	int m_numcopies;
	bool m_landscape;
	int m_wy;
	int m_ww;
	int m_wh;
	int m_iw;
	int m_ih;
	bool m_bitmapmode;

	/* user controls */
	unsigned int m_numusercontrols;
	Array<kGUIObj *>m_usercontrollist;

	kGUICallBackPtr<kGUIReport> m_closecallback;

	int m_startpage;
	int m_endpage;	/* only used for the are you sure message box */
};

#endif
