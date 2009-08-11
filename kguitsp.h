#ifndef __KGUITSP__
#define __KGUITSP__

#include "kguithread.h"

/* The TSPNode structure is used to represent nodes (cities) of the problem */

typedef struct TSPNode {
    long Id;        /* The number of the node (1...Dimension) */ 
    long Loc;       /* The location of the node in the heap 
                       (zero, if the node is not in the heap) */ 
    long Rank;      /* During the ascent, the priority of the node.
                       Otherwise, the ordinal number of the node in the tour */
    long V;         /* During the ascent the degree of the node minus 2.
                       Otherwise, the variable is used to mark nodes */ 			   
    long LastV;     /* The last value of V during the ascent */
    long Cost;      /* The "best" cost of an edge emanating from the node */
    long NextCost;  /* During the ascent, the "next best" cost of an edge
                       emanating from the node. When the candidate set is
                       determined it denotes the associated beta-value */ 
    long Pi;        /* The pi-value of the node */
    long BestPi;    /* The currently best pi-value found during the ascent */
    long *C;        /* A row in the cost matrix */
    struct TSPNode *Pred, *Suc; /* The predecessor and successor node in the two-way list 
                                of nodes */
    struct TSPNode *OldPred, 	
                *OldSuc;     /* Previous values of Pred and Suc */  	
    struct TSPNode *BestSuc, 
                *NextBestSuc;/* The best and next best successor node in the
                                currently best tour */
    struct TSPNode *Dad;        /* The father of the node in the minimum 1-tree */
    struct TSPNode *Next;       /* An auxiliary pointer, usually to the next node in
                                a list of nodes (e.g., the list of "active" nodes) */
    struct TSPNode *FixedTo1,	
                *FixedTo2;   /* Pointers to the opposite end nodes of fixed edges.
                                A maximum of two fixed edges can be incident to a node */
    struct TSPNode *OptimumSuc; /* The successor node as given in the TOUR_SECTION */
    struct TSPNode *InitialSuc; /* The successor node as given in the INITIAL_TOUR file */
    struct TSPNode *MergeSuc[2];/* The successor nodes as given in the MERGE_TOUR files */     
    struct Candidate *CandidateSet; /* The candidate array associated with the node */
    struct Segment *Parent;  /* The parent segment of a node when the two-level tree
                                representation is used */ 
    double X, Y, Z;          /* Coordinates of the node */
    int OldPredExcluded, 
        OldSucExcluded;      /* Booleans used for indicating that one (or both) of 
                                the adjoining edges on the tour has been excluded */	
} TSPNode;

/* The SwapRecord structure is used to record 2-opt moves (swaps) */ 
	
typedef struct SwapRecord {
    TSPNode *t1, *t2, *t3, *t4;    /* The 4 nodes involved in a 2-opt move */
} SwapRecord;

#define HashTableSize 65521    /* The largest prime less than INT_MAX */
#define MaxLoadFactor 0.75

typedef struct HashTableEntry {
    unsigned long Hash;
    double Cost;
} HashTableEntry;

typedef struct HashTable {
    HashTableEntry Entry[HashTableSize];
    long Count;                /* Number occupied entries in the table */
} HashTable;

class kGUITSP
{
public:
	kGUITSP();
	~kGUITSP();
	void Init(int numcoords);
	void SetCoord(int index,double x,double y) {m_x.SetEntry(index,x);m_y.SetEntry(index,y);}
	void Calc(void);
	void AsyncCalc(void);
	void Stop(void) {m_stop=true;}			/* used to stop search */
	int GetIndex(int n) {return m_bestpath.GetEntry(n);}

	/* these are to be called to query the current results while it is in async mode */
	bool GetActive(void) {return m_thread.GetActive();}
	unsigned int GetCount(void) {return m_count;}
	int GetNewBest(void) {return m_newbest;}
	int *GetCurList(void) {return m_curpath.GetArrayPtr();}
private:
	CALLBACKGLUE(kGUITSP,Calc);
	/* heap stuff */
	void MakeHeap(const long Size);
	void SiftUp(TSPNode *N);
	TSPNode *DeleteMin(void);
	void Insert(TSPNode * N);

	void CreateCandidateSet(void);
	void CreateNodes(void);
	double FindTour(void);
	double LinKernighan(void);
	void RecordBestTour(void);
	void RecordBetterTour(void);
	TSPNode *RemoveFirstActive(void);
	double Minimum1TreeCost(const int Sparse);
	void MinimumSpanningTree(const int Sparse);
	void NormalizeNodeList(void);
	void ResetCandidateSet(void);
	void ChooseInitialTour(void);
	void RestoreTour(void);
	void StoreTour(void);
	double Ascent(void);
	long Gain23(void);
	void Activate(TSPNode *t);
	void AdjustCandidateSet(void);
	int Between(const TSPNode *t2, const TSPNode *t1, const TSPNode *t3);
	int Between_SL(const TSPNode *t2, const TSPNode *t1, const TSPNode *t3);
	long BridgeGain(TSPNode *s1, TSPNode *s2, TSPNode *s3, TSPNode *s4, TSPNode *s5, TSPNode *s6, TSPNode *s7, TSPNode *s8, int Case6, long G);
	void Connect(TSPNode * N1, const long Max, const int Sparse);
	int Excludable(const TSPNode *Na, const TSPNode *Nb);
	void Exclude(TSPNode *Na, TSPNode *Nb);
	void Flip(TSPNode *t1, TSPNode *t2, TSPNode *t3);
	void Flip_SL(TSPNode *t1, TSPNode *t2, TSPNode *t3);
	void SplitSegment(TSPNode * t1, TSPNode * t2);
	int Forbidden(const TSPNode * ta, const TSPNode * tb);
	void FreeStructures(void);
	void GenerateCandidates(const long MaxCandidates, const long MaxAlpha, const int Symmetric);
	long C_EXPLICIT(TSPNode *Na, TSPNode *Nb);
	long C_FUNCTION(TSPNode *Na, TSPNode *Nb);
	long D_EXPLICIT(TSPNode *Na, TSPNode *Nb);
	long D_FUNCTION(TSPNode *Na, TSPNode *Nb);
	long c_CEIL_2D(TSPNode *Na, TSPNode *Nb);
	long c_CEIL_3D(TSPNode *Na, TSPNode *Nb);
	long c_EUC_2D(TSPNode *Na, TSPNode *Nb);
	long c_EUC_3D(TSPNode *Na, TSPNode *Nb);
	long c_GEO(TSPNode *Na, TSPNode *Nb);
	long c_GEOM(TSPNode *Na, TSPNode *Nb);

	TSPNode *Backtrack2OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Backtrack3OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Backtrack4OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Backtrack5OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);

	TSPNode *Best2OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Best3OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Best4OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *Best5OptMove(TSPNode *t1, TSPNode *t2, long *G0, long *Gain);

	void Make2OptMove(TSPNode *t1, TSPNode *t2, TSPNode *t3, TSPNode *t4);
	void Make3OptMove(TSPNode *t1, TSPNode *t2, TSPNode *t3, TSPNode *t4,TSPNode *t5, TSPNode *t6, int Case);
	void Make4OptMove(TSPNode *t1, TSPNode *t2, TSPNode *t3, TSPNode *t4,TSPNode *t5, TSPNode *t6, TSPNode *t7, TSPNode *t8, int Case);
	void Make5OptMove(TSPNode *t1, TSPNode *t2, TSPNode *t3, TSPNode *t4, TSPNode *t5, TSPNode *t6, TSPNode *t7, TSPNode *t8, TSPNode *t9, TSPNode *t10, int Case);

	long Distance_1(TSPNode *Na, TSPNode *Nb);
	long Distance_ATSP(TSPNode *Na, TSPNode *Nb);
	long Distance_ATT(TSPNode *Na, TSPNode *Nb);
	long Distance_CEIL_2D(TSPNode *Na, TSPNode *Nb);
	long Distance_CEIL_3D(TSPNode *Na, TSPNode *Nb);
	long Distance_EXPLICIT(TSPNode *Na, TSPNode *Nb);
	long Distance_EUC_2D(TSPNode *Na, TSPNode *Nb);
	long Distance_EUC_3D(TSPNode *Na, TSPNode *Nb);
	long Distance_GEO(TSPNode *Na, TSPNode *Nb);
	long Distance_GEOM(TSPNode *Na, TSPNode *Nb);
	long Distance_MAN_2D(TSPNode *Na, TSPNode *Nb);
	long Distance_MAN_3D(TSPNode *Na, TSPNode *Nb);
	long Distance_MAX_2D(TSPNode *Na, TSPNode *Nb);
	long Distance_MAX_3D(TSPNode *Na, TSPNode *Nb);
	long Distance_XRAY1(TSPNode *Na, TSPNode *Nb);
	long Distance_XRAY2(TSPNode *Na, TSPNode *Nb);

	/* variables */

	int m_problemtype;
	int m_weighttype;
	int m_weightformat;
	int m_coordtype;
	int m_candidatesetsymmetric;

	long *m_besttour;
	long m_dimension;
	long m_maxcandidates;

	long m_ascentcandidates;
	long m_initialperiod;
	long InitialStepSize;
	long Precision;
	long MaxTrials;
	long MaxSwaps;
	double BestCost, WorstCost, Excess, Optimum;
	unsigned int Seed;
	int Subgradient, MoveType, BacktrackMoveType, RestrictedSearch;

	TSPNode *NodeSet, *FirstNode, *FirstActive, *LastActive, **Heap;
	SwapRecord *SwapStack;
	long Swaps, m_Norm, M, GroupSize, Groups, Trial, *BetterTour, *CacheVal,
		*CacheSig, *CostMatrix;
	double BetterCost, LowerBound;
	unsigned long Hash;
	int *Rand, Reversed;
	Segment *FirstSegment;
	HashTable *HTable;

	TSPNode *(kGUITSP::*BestMove) (TSPNode *t1, TSPNode *t2, long *G0, long *Gain);
	TSPNode *(kGUITSP::*BacktrackMove) (TSPNode * t1, TSPNode * t2, long *G0, long *Gain);
	long (kGUITSP::*Distance) (TSPNode * Na, TSPNode * Nb);
	long (kGUITSP::*C) (TSPNode * Na, TSPNode * Nb);
	long (kGUITSP::*D) (TSPNode * Na, TSPNode * Nb);
	long (kGUITSP::*c) (TSPNode * Na, TSPNode * Nb);

	long HeapCount;          /* Its current number of elements */

	/***********************************/

	unsigned int m_count;
	kGUIThread m_thread;	/* only used for async */

	bool m_stop;
	int m_newbest;
	int m_runs;
	int m_numcoords;
	Array<int>m_curpath;
	Array<int>m_bestpath;
	Array<double>m_x;
	Array<double>m_y;
};

#endif
