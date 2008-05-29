#ifndef __KGUIREQ__
#define __KGUIREQ__

enum
{
FILEREQ_OPEN,
FILEREQ_SAVE};

class kGUIFileReq
{
public:
	kGUIFileReq(int type,const char *inname,const char *ext,void *codeobj,void (*code)(void *,kGUIFileReq *,int));
	~kGUIFileReq() {m_table.DeleteChildren();}
	const char *GetPath(void) {return m_path.GetString();}
	const char *GetFilename(void) {return m_longfn.GetString();}
	kGUIText *GetTitle(void) {return m_window.GetTitle();}
	void SetFilename(const char *fn);
private:
	CALLBACKGLUEPTR(kGUIFileReq,PressCancel,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,PressBack,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,PressNewFolder,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,PressDone,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,PathChangedEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,ShortFnEdited,kGUIEvent)
	CALLBACKGLUE(kGUIFileReq,Click)
	CALLBACKGLUE(kGUIFileReq,DoubleClick)
	CALLBACKGLUEPTR(kGUIFileReq,CopyFilename,kGUIEvent)
	CALLBACKGLUEPTR(kGUIFileReq,WindowEvent,kGUIEvent)
	void GoBack(void);
	void PressBack(kGUIEvent *event);
	void PressCancel(kGUIEvent *event);
	void PressNewFolder(kGUIEvent *event);
	void PressDone(kGUIEvent *event);
	void PathChangedEvent(kGUIEvent *event);
	void ShortFnEdited(kGUIEvent *event);

	void Click(void);
	void DoubleClick(void);
	void CopyFilename(kGUIEvent *event);
	void PathChanged(void);
	void WindowEvent(kGUIEvent *event);

	kGUIString m_ext;
	kGUIControlBoxObj m_controls;
	kGUITableObj m_table;
	kGUIWindowObj m_window;
	kGUITextObj m_pathlabel;
	kGUITextObj m_fnlabel;
	kGUIInputBoxObj m_path;			/* path part of filename */
	kGUIInputBoxObj m_shortfn;		/* short part of filename */
	kGUIInputBoxObj m_longfn;		/* generated after closing for returning to caller */

	kGUIImageObj m_backimage;
	kGUIImageObj m_newimage;
	kGUIButtonObj m_back;
	kGUIButtonObj m_newfolder;
	kGUIButtonObj m_cancel;
	kGUIButtonObj m_done;
	kGUICallBackPtrInt<kGUIFileReq> m_donecallback;
	int m_type;
	int m_pressed;	/* button that was pressed to close */
	int m_origwidth;
	int m_origheight;
	int m_lastwidth;
	int m_lastheight;
};

class kGUIDateGridObj : public kGUIObj
{
public:
	void Set(int vm,int vy,int d,int m,int y);
	void Draw(void);
	bool UpdateInput(void);
	void GetHolidayString(kGUIDate *date,kGUIString *s);	/* called by set whenever the month changes */
	void SetValidDayCallback(void *codeobj,void (*code)(void *,kGUIDate *date, bool *e)) {m_validdaycallback.Set(codeobj,code);}
	void SetClickCallback(void *codeobj,void (*code)(void *,kGUIDate *date, bool *dc)) {m_clickcallback.Set(codeobj,code);}
private:
	int m_viewmonth;
	int m_curmonth;
	int m_curyear;
	int m_curday;

	int m_day[6*7];
	int m_month[6*7];
	int m_year[6*7];
	kGUIString m_holidays[6*7];
	bool m_enable[6*7];
	kGUIText m_text;
	kGUIText m_ccuryear;	/* current chinese year of the xxx */
	kGUICallBackPtrPtr<kGUIDate,bool> m_validdaycallback;
	kGUICallBackPtrPtr<kGUIDate,bool> m_clickcallback;
};

class kGUIDateSelectorObj : public kGUIContainerObj
{
public:
	kGUIDateSelectorObj();

	void Draw(void) {DrawC(0);}
	bool UpdateInput(void) {return UpdateInputC(0);}
	void CalcChildZone(void) {SetChildZone(0,0,GetZoneW(),GetZoneH());}
	void SetDate(kGUIDate *date) {m_date=*(date);DayChanged();}
	void GetDate(kGUIDate *date) {date->Copy(&m_date);}
	void SetValidDayCallback(void *codeobj,void (*code)(void *,kGUIDate *date, bool *e)) {m_grid.SetValidDayCallback(codeobj,code);}
	void SetClickCallback(void *codeobj,void (*code)(void *,kGUIDate *date, bool *dc)) {m_clickcallback.Set(codeobj,code);}
private:
	CALLBACKGLUEPTR(kGUIDateSelectorObj,ChangeMonth,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDateSelectorObj,ChangeYear,kGUIEvent)
	CALLBACKGLUE(kGUIDateSelectorObj,DayChanged)
	CALLBACKGLUEPTRPTR(kGUIDateSelectorObj,Clicked,kGUIDate,bool)
	void Clicked(kGUIDate *date,bool *doubleclicked);
	void ChangeMonth(kGUIEvent *event);
	void ChangeYear(kGUIEvent *event);
	void DayChanged(void);

	kGUIDate m_date;

	kGUIScrollTextObj m_vmonth;
	kGUIScrollTextObj m_vyear;
	int m_viewmonth;
	int m_viewyear;

	kGUIDateGridObj m_grid;

	kGUICallBackPtrPtr<kGUIDate,bool> m_clickcallback;
//	kGUICallBack m_donecallback;
};


class kGUIDateReq
{
public:
	kGUIDateReq(kGUIDate *date,void *codeobj,void (*code)(void *,kGUIDate *date));
	kGUIText *GetTitle(void) {return m_window.GetTitle();}
	void SetValidDayCallback(void *codeobj,void (*code)(void *,kGUIDate *date, bool *e)) {m_selector.SetValidDayCallback(codeobj,code);}
	void Clicked(kGUIDate *date,bool *doubleclicked);
private:
	CALLBACKGLUEPTR(kGUIDateReq,WindowEvent,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDateReq,PressDone,kGUIEvent)
	CALLBACKGLUEPTR(kGUIDateReq,PressCancel,kGUIEvent)
	CALLBACKGLUEPTRPTR(kGUIDateReq,Clicked,kGUIDate,bool)
	void WindowEvent(kGUIEvent *event);
	void PressDone(kGUIEvent *event);
	void PressCancel(kGUIEvent *event);

	kGUIDate m_date;

	kGUIDateSelectorObj m_selector;

	kGUIButtonObj m_cancel;
	kGUIButtonObj m_done;
	kGUIWindowObj m_window;

	kGUICallBackPtr<kGUIDate> m_donecallback;
};


#endif
