class kGUIHTMLMoviePluginObj : public kGUIHTMLPluginObj
{
public:
	kGUIHTMLMoviePluginObj();
	~kGUIHTMLMoviePluginObj();
	kGUIHTMLPluginObj *New(void);	/* generate a new one */
	DataHandle *GetDH(void);		/* used to attach data source */
	bool Open(void);				/* called after attaching datahandle, true=valid, false=invalid */
	kGUIObj *GetObj(void);		/* object used to attach event handle to */
	void Draw(void) {DrawC(0);}
	bool UpdateInput(void) {return UpdateInputC(0);}
private:
	void CalcChildZone(void) {SetChildZone(0,0,GetZoneW(),GetZoneH());}
	kGUIMovieObj m_movieobj;
};

