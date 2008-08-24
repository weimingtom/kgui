
class kGUIBSPRectEntry
{
public:
	kGUICorners m_c;
};

class kGUIBSPCornerEntry
{
public:
	int m_c[2];
	kGUIBSPRectEntry *m_rp;
};

class kGUIBSPZoneEntry
{
public:
	int m_minc[2];
	int m_maxc[2];
	int m_start;
	int m_end;
	kGUIBSPZoneEntry *m_left;
	kGUIBSPZoneEntry *m_right;
};

/* BSP for adding rectangles to */

class kGUIBSPRect
{
public:
	kGUIBSPRect();
	~kGUIBSPRect();
	void Alloc(int n,int c);
	void AddEntry(kGUIBSPRectEntry *re);
	kGUIBSPZoneEntry *CutAxis(int start,int end,int depth);
	void Cut(void) {m_curzones=m_zones;CutAxis(0,m_numentries,m_numcuts-1);}
	void SelectZone(kGUICorners *c,kGUIBSPZoneEntry *zone);
	void Select(kGUICorners *c);
	kGUIBSPRectEntry *GetEntry(void);
	static int Sort0(const void *v1,const void *v2);
	static int Sort1(const void *v1,const void *v2);

private:
//	static int m_sortaxis;
	int m_max[2];
	int m_numentries;		/* number allocated */
	int m_numalloc;
	int m_numalloc4;
	int m_numzones;			/* number allocated */
	int m_numcuts;
	int m_numdrawzones;
	/* these point to the allocated tables */
	kGUIBSPCornerEntry *m_rc;
	kGUIBSPCornerEntry **m_prc;
	kGUIBSPZoneEntry *m_zones;
	kGUIBSPZoneEntry **m_drawzones;
	/* these pointers increment as items are added */
	kGUIBSPCornerEntry *m_currc;
	kGUIBSPCornerEntry **m_curprc;
	/* this is incremented as the rects are cut */
	kGUIBSPZoneEntry *m_curzones;
	kGUIBSPZoneEntry **m_wpdrawzones;
	kGUIBSPZoneEntry **m_rpdrawzones;
	int m_readcurrent;
};

class kGUIBSPPointEntry
{
public:
	int m_c[2];
};

/* BSP for adding since points to */
class kGUIBSPPoint
{
public:
	kGUIBSPPoint();
	~kGUIBSPPoint();
	void Alloc(int n,int c);
	void AddEntry(kGUIBSPPointEntry *re);
	kGUIBSPZoneEntry *CutAxis(int start,int end,int depth);
	void Cut(void) {m_curzones=m_zones;CutAxis(0,m_numentries,m_numcuts-1);}
	void SelectZone(kGUICorners *c,kGUIBSPZoneEntry *zone);
	void Select(kGUICorners *c);
	kGUIBSPPointEntry *GetEntry(void);
	static int Sort0(const void *v1,const void *v2);
	static int Sort1(const void *v1,const void *v2);
private:
//	static int m_sortaxis;
	int m_numentries;		/* number allocated */
	int m_numalloc;
	int m_numzones;			/* number allocated */
	int m_numcuts;
	int m_numdrawzones;
	/* these point to the allocated tables */
	kGUIBSPPointEntry **m_prc;
	kGUIBSPZoneEntry *m_zones;
	kGUIBSPZoneEntry **m_drawzones;
	/* these pointers increment as items are added */
	kGUIBSPPointEntry **m_curprc;
	/* this is incremented as the rects are cut */
	kGUIBSPZoneEntry *m_curzones;
	kGUIBSPZoneEntry **m_wpdrawzones;
	kGUIBSPZoneEntry **m_rpdrawzones;
	int m_readcurrent;
};
