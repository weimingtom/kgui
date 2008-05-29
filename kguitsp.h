#ifndef __KGUITSP__
#define __KGUITSP__

/* The Node structure is used to represent nodes (cities) of the problem */

typedef struct Node {
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
    struct Node *Pred, *Suc; /* The predecessor and successor node in the two-way list 
                                of nodes */
    struct Node *OldPred, 	
                *OldSuc;     /* Previous values of Pred and Suc */  	
    struct Node *BestSuc, 
                *NextBestSuc;/* The best and next best successor node in the
                                currently best tour */
    struct Node *Dad;        /* The father of the node in the minimum 1-tree */
    struct Node *Next;       /* An auxiliary pointer, usually to the next node in
                                a list of nodes (e.g., the list of "active" nodes) */
    struct Node *FixedTo1,	
                *FixedTo2;   /* Pointers to the opposite end nodes of fixed edges.
                                A maximum of two fixed edges can be incident to a node */
    struct Node *OptimumSuc; /* The successor node as given in the TOUR_SECTION */
    struct Node *InitialSuc; /* The successor node as given in the INITIAL_TOUR file */
    struct Node *MergeSuc[2];/* The successor nodes as given in the MERGE_TOUR files */     
    struct Candidate *CandidateSet; /* The candidate array associated with the node */
    struct Segment *Parent;  /* The parent segment of a node when the two-level tree
                                representation is used */ 
    double X, Y, Z;          /* Coordinates of the node */
    int OldPredExcluded, 
        OldSucExcluded;      /* Booleans used for indicating that one (or both) of 
                                the adjoining edges on the tour has been excluded */	
} Node;

/* The SwapRecord structure is used to record 2-opt moves (swaps) */ 
	
typedef struct SwapRecord {
    Node *t1, *t2, *t3, *t4;    /* The 4 nodes involved in a 2-opt move */
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
	int GetIndex(int n) {return m_bestpath.GetEntry(n);}
private:
	/* heap stuff */
	void MakeHeap(const long Size);
	void SiftUp(Node *N);
	Node *DeleteMin(void);
	void Insert(Node * N);

	void CreateCandidateSet(void);
	void CreateNodes(void);
	double FindTour(void);
	double LinKernighan(void);
	void RecordBestTour(void);
	void RecordBetterTour(void);
	Node *RemoveFirstActive(void);
	double Minimum1TreeCost(const int Sparse);
	void MinimumSpanningTree(const int Sparse);
	void NormalizeNodeList(void);
	void ResetCandidateSet(void);
	void ChooseInitialTour(void);
	void RestoreTour(void);
	void StoreTour(void);
	double Ascent(void);
	long Gain23(void);
	void Activate(Node *t);
	void AdjustCandidateSet(void);
	int Between(const Node *t2, const Node *t1, const Node *t3);
	int Between_SL(const Node *t2, const Node *t1, const Node *t3);
	long BridgeGain(Node *s1, Node *s2, Node *s3, Node *s4, Node *s5, Node *s6, Node *s7, Node *s8, int Case6, long G);
	void Connect(Node * N1, const long Max, const int Sparse);
	int Excludable(const Node *Na, const Node *Nb);
	void Exclude(Node *Na, Node *Nb);
	void Flip(Node *t1, Node *t2, Node *t3);
	void Flip_SL(Node *t1, Node *t2, Node *t3);
	void SplitSegment(Node * t1, Node * t2);
	int Forbidden(const Node * ta, const Node * tb);
	void FreeStructures(void);
	void GenerateCandidates(const long MaxCandidates, const long MaxAlpha, const int Symmetric);
	long C_EXPLICIT(Node *Na, Node *Nb);
	long C_FUNCTION(Node *Na, Node *Nb);
	long D_EXPLICIT(Node *Na, Node *Nb);
	long D_FUNCTION(Node *Na, Node *Nb);
	long c_CEIL_2D(Node *Na, Node *Nb);
	long c_CEIL_3D(Node *Na, Node *Nb);
	long c_EUC_2D(Node *Na, Node *Nb);
	long c_EUC_3D(Node *Na, Node *Nb);
	long c_GEO(Node *Na, Node *Nb);
	long c_GEOM(Node *Na, Node *Nb);

	Node *Backtrack2OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Backtrack3OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Backtrack4OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Backtrack5OptMove(Node *t1, Node *t2, long *G0, long *Gain);

	Node *Best2OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Best3OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Best4OptMove(Node *t1, Node *t2, long *G0, long *Gain);
	Node *Best5OptMove(Node *t1, Node *t2, long *G0, long *Gain);

	void Make2OptMove(Node *t1, Node *t2, Node *t3, Node *t4);
	void Make3OptMove(Node *t1, Node *t2, Node *t3, Node *t4,Node *t5, Node *t6, int Case);
	void Make4OptMove(Node *t1, Node *t2, Node *t3, Node *t4,Node *t5, Node *t6, Node *t7, Node *t8, int Case);
	void Make5OptMove(Node *t1, Node *t2, Node *t3, Node *t4, Node *t5, Node *t6, Node *t7, Node *t8, Node *t9, Node *t10, int Case);

	long Distance_1(Node *Na, Node *Nb);
	long Distance_ATSP(Node *Na, Node *Nb);
	long Distance_ATT(Node *Na, Node *Nb);
	long Distance_CEIL_2D(Node *Na, Node *Nb);
	long Distance_CEIL_3D(Node *Na, Node *Nb);
	long Distance_EXPLICIT(Node *Na, Node *Nb);
	long Distance_EUC_2D(Node *Na, Node *Nb);
	long Distance_EUC_3D(Node *Na, Node *Nb);
	long Distance_GEO(Node *Na, Node *Nb);
	long Distance_GEOM(Node *Na, Node *Nb);
	long Distance_MAN_2D(Node *Na, Node *Nb);
	long Distance_MAN_3D(Node *Na, Node *Nb);
	long Distance_MAX_2D(Node *Na, Node *Nb);
	long Distance_MAX_3D(Node *Na, Node *Nb);
	long Distance_XRAY1(Node *Na, Node *Nb);
	long Distance_XRAY2(Node *Na, Node *Nb);

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

	Node *NodeSet, *FirstNode, *FirstActive, *LastActive, **Heap;
	SwapRecord *SwapStack;
	long Swaps, m_Norm, M, GroupSize, Groups, Trial, *BetterTour, *CacheVal,
		*CacheSig, *CostMatrix;
	double BetterCost, LowerBound;
	unsigned long Hash;
	int *Rand, Reversed;
	Segment *FirstSegment;
	HashTable *HTable;

	Node *(kGUITSP::*BestMove) (Node *t1, Node *t2, long *G0, long *Gain);
	Node *(kGUITSP::*BacktrackMove) (Node * t1, Node * t2, long *G0, long *Gain);
	long (kGUITSP::*Distance) (Node * Na, Node * Nb);
	long (kGUITSP::*C) (Node * Na, Node * Nb);
	long (kGUITSP::*D) (Node * Na, Node * Nb);
	long (kGUITSP::*c) (Node * Na, Node * Nb);

	long HeapCount;          /* Its current number of elements */

	/***********************************/

	int m_runs;
	int m_numcoords;
	Array<int>m_bestpath;
	Array<double>m_x;
	Array<double>m_y;
};

#endif
