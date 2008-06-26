/**********************************************************************************/
/* kGUI - kguitsp.cpp                                                             */
/*                                                                                */
/* Wrapped by Kevin Pickell ( main code by K. Helsgaun, see below )               */
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

/*! @file kguitsp.cpp
    @brief This is a travelling salesman function. I have no idea how it works. :-) */

/*! @todo make an async version that is given an amount of 'time' to process */

#include "kgui.h"
#include "kguitsp.h"
#include <limits.h>
//only needed for DBL_MAX
#include <float.h>

//LKH
//
//http://www.akira.ruc.dk/~keld/
//
//Version 1.3 (July 2002)
//
//LKH is an effective implementation of the Lin-Kernighan heuristic for
// solving the traveling salesman problem.
//
//LKH has been described in the report
//
//    K. Helsgaun,
//    "An Effective Implementation of the Lin-Kernighan Traveling Salesman Heuristic",
//    DATALOGISKE SKRIFTER (Writings on Computer Science), No. 81, 1998,
//    Roskilde University.
//
//and in the paper
//
//    K. Helsgaun,
//    "An Effective Implementation of the Lin-Kernighan Traveling Salesman Heuristic",
//    European Journal of Operational Research 126 (1), 106-130 (2000).

#undef assert
#define assert
/*
   This header specifies the interface for accessing and manipulating a
   tour. 

   If SEGMENT_LIST is defined the two-level tree representation is used
   for a tour. Otherwise the linked list representation is used. 

   Both representations support the following primitive operations:

       (1) find the predecessor of a node in the tour with respect 
           to a chosen orientation (PRED);

       (2) find the successor of a node in the tour with respect to 
           a chosen orientation (SUC); 

       (3) determine whether a given node is between two other nodes 
           in the tour with respect to a chosen orientation (BETWEEN);

       (4) make a 2-opt move (FLIP).
	
   The default representation is the two-level tree representation. 
   In order to use the linked list representation, uncomment the 
   following preprocessor command line.	
*/

/* #undef SEGMENT_LIST */
#define SEGMENT_LIST 1

#ifdef SEGMENT_LIST
#define PRED(a) (Reversed == (a)->Parent->Reversed ? (a)->Pred : (a)->Suc)
#define SUC(a) (Reversed == (a)->Parent->Reversed ? (a)->Suc : (a)->Pred)
#define BETWEEN(a,b,c) Between_SL(a,b,c)
#define FLIP(a,b,c,d) Flip_SL(a,b,c)
#else
#define PRED(a) (Reversed ? (a)->Suc : (a)->Pred)
#define SUC(a) (Reversed ? (a)->Pred : (a)->Suc)
#define BETWEEN(a,b,c) Between(a,b,c)
#define FLIP(a,b,c,d) Flip(a,b,c)
#endif

#define Swap1(a1,a2,a3)\
        FLIP(a1,a2,a3,0)
#define Swap2(a1,a2,a3, b1,b2,b3)\
        (Swap1(a1,a2,a3), Swap1(b1,b2,b3))
#define Swap3(a1,a2,a3, b1,b2,b3, c1,c2,c3)\
        (Swap2(a1,a2,a3, b1,b2,b3), Swap1(c1,c2,c3))
#define Swap4(a1,a2,a3, b1,b2,b3, c1,c2,c3, d1,d2,d3)\
        (Swap3(a1,a2,a3, b1,b2,b3, c1,c2,c3), Swap1(d1,d2,d3))
#define Swap5(a1,a2,a3, b1,b2,b3, c1,c2,c3, d1,d2,d3, e1,e2,e3)\
        (Swap4(a1,a2,a3, b1,b2,b3, c1,c2,c3, d1,d2,d3), Swap1(e1,e2,e3))

/*
   This header specifies the interface for hashing.   
*/
void HashInitialize(HashTable * T);
void HashInsert(HashTable * T, unsigned long Hash, double Cost);
int HashSearch(HashTable * T, unsigned long Hash, double Cost);

/* Macro definitions */

#define Fixed(a,b) ((a)->FixedTo1 == (b) || (a)->FixedTo2 == (b))
#define Follow(b,a) ((a)->Suc != (b) ? Link((b)->Pred,(b)->Suc), Link(b,(a)->Suc), Link(a,b) : 0) 
#define InBestTour(a,b) ((a)->BestSuc == (b) || (b)->BestSuc == (a))
#define InNextBestTour(a,b) ((a)->NextBestSuc == (b) || (b)->NextBestSuc == (a))
#define Link(a,b) ((a)->Suc = (b), (b)->Pred = (a))
#define Near(a,b) ((a)->BestSuc ? InBestTour(a,b) : (a)->Dad == (b) || (b)->Dad == (a))
#define InOptimumTour(a,b) ((a)->OptimumSuc == (b) || (b)->OptimumSuc == (a))
#define IsCommonEdge(a,b) (((a)->MergeSuc[0] == (b) || (b)->MergeSuc[0] == (a)) &&\
                           ((a)->MergeSuc[1] == (b) || (b)->MergeSuc[1] == (a)))
#define Precede(a,b) ((b)->Pred != (a) ? Link((a)->Pred,(a)->Suc), Link((b)->Pred,a), Link(a,b) : 0)

enum Types {TSP, ATSP, SOP, HCP, CVRP, TOUR, HPP};
enum EdgeWeightTypes {EXPLICIT, EUC_2D, EUC_3D, MAX_2D, MAX_3D, MAN_2D, MAN_3D,
                      CEIL_2D, CEIL_3D, GEO, GEOM, ATT, XRAY1, XRAY2, SPECIAL};
enum EdgeWeightFormats {FUNCTION, FULL_MATRIX, UPPER_ROW, LOWER_ROW,
                        UPPER_DIAG_ROW, LOWER_DIAG_ROW, UPPER_COL, LOWER_COL, 			        
                        UPPER_DIAG_COL, LOWER_DIAG_COL};
enum CoordTypes {TWOD_COORDS, THREED_COORDS, NO_COORDS};

/* The Candidate structure is used to represent candidate edges */

typedef struct Candidate
{
    Node *To;           /* A pointer to the end node of the edge */
    long Cost;          /* The cost (distance) of the edge */
    long Alpha;         /* Its alpha-value */
} Candidate; 

/* The Segment strucure is used to represent the segments in the two-level representation
   of tours */
            
typedef struct Segment
{
    int Reversed;               /* The reversal bit */
    Node *First, *Last;         /* The first and last node in the segment */
    struct Segment *Pred, *Suc; /* The predecessor and successor in the two-way list of
                                   segments */ 
    long Rank;                  /* The ordinal number of the segment in the list */						
    long Size;                  /* The number of nodes in the segment */
} Segment;

/**********************************************************************************************/

/*
   If a node is made "active", attempts are made to find an improving 
   move with the node as anchor node, t1.

   The Active function makes a node active by inserting it into a 
   queue of active nodes. FirstActive denotes the front node of 
   the queue. LastActive denotes the rear. 

   The queue is implemented as a circular list in which the Next field 
   of each Node references the successor node. 

   A node is member of the queue iff its Next != 0. The function has no 
   effect if the node is already in the queue. 

   The function is called from the StoreTour function.
*/

void kGUITSP::Activate(Node * t)
{
    if (t->Next != 0)
        return;
    if (FirstActive == 0)
        FirstActive = LastActive = t;
    else {
        LastActive->Next = t;
        LastActive = t;
    }
    LastActive->Next = FirstActive;
}

/*
   When a trial has produced a better tour, the set of candidate edges is
   adjusted as follows:

   (1) The set is extended with edges with tour edges not present in the
   current set;
   (2)  Precedence is given to those edges that are common to the two
   currently best tours.

   The AdjustCandidateSet function adjusts for each node its table of candidate edges.
   A new candidate edge is added by extending the table and inserting the edge as
   its last ordinary element (disregarding the dummy edge). The Alpha field of the
   new candidate edge is set to LONG_MAX. Edges that belong to the best tour as well 
   as the next best tour are moved to the start of the table.                         
*/

void kGUITSP::AdjustCandidateSet(void)
{
    Candidate *NFrom, *NN, Temp;
    Node *From = FirstNode, *To;

    /* Extend candidate sets */
    do {
        for (To = From->Pred; To; To = To == From->Pred ? From->Suc : 0) {
            long Count = 0;
            for (NFrom = From->CandidateSet; NFrom->To && NFrom->To != To;
                 NFrom++)
                Count++;
            if (!NFrom->To) {   /*  Insert edge */
                NFrom->Cost = (this->*C)(From, To);
                NFrom->To = To;
                NFrom->Alpha = LONG_MAX;
                From->CandidateSet =
                       (Candidate *) realloc(From->CandidateSet,
                                             (Count +
                                              2) * sizeof(Candidate));
                From->CandidateSet[Count + 1].To = 0;
            }
        }
    }
    while ((From = From->Suc) != FirstNode);

    /* Reorder candidate sets */
    do {
        for (NFrom = From->CandidateSet + 1; (To = NFrom->To); NFrom++)
            if (InBestTour(From, To) && InNextBestTour(From, To)) {
                /* Move the edge to the start of the candidate table */
                Temp = *NFrom;
                for (NN = NFrom - 1; NN >= From->CandidateSet; NN--)
                    *(NN + 1) = *NN;
                *(NN + 1) = Temp;
            }
    }
    while ((From = From->Suc) != FirstNode);
}

/* 
   The Ascent function computes a lower bound on the optimal tour length using 
   subgradient optimization. The function also transforms the original problem 
   into a problem in which the alpha-values reflect the likelihood of edges being
   optimal.

   The function attempts to find penalties (pi-values) that maximizes the lower
   bound L(T(Pi)) - 2*PiSum, where L(T(Pi)) denotes the length of the minimum
   spanning 1-tree computed from the transformed distances, and PiSum denotes the 
   sum of pi-values. If C(i,j) denotes the length of and edge (i,j) then the 
   transformed distance D(i,j) of an edge is C(i,j) + Pi(i) + Pi(j).

   The Minimum1TreeCost function is used to compute the cost of a minimum 1-tree. 
   The Generatecandidates function is called in order to generate candidate sets. 
   Minimum 1-trees are then computed in the corresponding sparse graph.         
*/

double kGUITSP::Ascent(void)
{
    Node *t;
    double BestW, W, W0;
    long T, Period, P;
    int InitialPhase;

  Start:
    /* Initialize Pi and BestPi */
    t = FirstNode;
    do
        t->BestPi = t->Pi = 0;
    while ((t = t->Suc) != FirstNode);

    /* Compute the cost of a minimum 1-tree */
    W = Minimum1TreeCost(0);

    /* Return this cost 
       if either
       (1) subgradient optimization is not wanted,
       (2) the norm of the tree (its deviation from a tour) is zero
       (in that case the true optimum has been found), or
       (3) the cost equals the specified value for optimum.
     */
    if (!Subgradient || !m_Norm || W / Precision == Optimum)
        return W;

    /* Generate symmetric candididate sets for all nodes */
    GenerateCandidates(m_ascentcandidates, LONG_MAX, 1);

    /* Set LastV of every node to V (the node's degree in the 1-tree) */
    t = FirstNode;
    do
        t->LastV = t->V;
    while ((t = t->Suc) != FirstNode);

    BestW = W0 = W;
    InitialPhase = 1;
    /* Perform subradient optimization with decreasing period length 
       and decreasing step size */
    for (Period = m_initialperiod, T = InitialStepSize * Precision; Period > 0 && T > 0 && m_Norm != 0; Period /= 2, T /= 2) {      /* Period and step size are halved at each iteration */
//        if (TraceLevel >= 2) {
//            printf("  T = %ld, Period = %ld, BestW = %0.2f, Norm = %ld\n",
//                   T, Period, BestW / Precision, Norm);
//            fflush(stdout);
//        }
        for (P = 1; T && P <= Period && m_Norm != 0; P++) {
            /* Adjust the Pi-values */
            t = FirstNode;
            do {
                if (t->V != 0)
                    t->Pi += T * (7 * t->V + 3 * t->LastV) / 10;
                t->LastV = t->V;
            }
            while ((t = t->Suc) != FirstNode);
            /* Compute a minimum 1-tree in the sparse graph */
            W = Minimum1TreeCost(1);
            /* Test if an improvement has been found */
            if (W > BestW) {
                /* If the lower bound becomes greater than twice its
                   initial value it is taken as a sign that the graph is
                   too sparse */
                if (W > 2 * W0 && m_ascentcandidates < m_dimension) {
                    W = Minimum1TreeCost(0);
                    if (W < W0) {
                        /* Double the number of candidate edges 
                           and start all over again */
//                        if (TraceLevel >= 2) {
//                            printf("Warning: m_ascentcandidates doubled\n");
//                            fflush(stdout);
//                        }
                        if ((m_ascentcandidates *= 2) > m_dimension)
                            m_ascentcandidates = m_dimension;
                        goto Start;
                    }
                    W0 = W;
                }
                BestW = W;
                /* Update the BestPi-values */
                t = FirstNode;
                do
                    t->BestPi = t->Pi;
                while ((t = t->Suc) != FirstNode);
//                if (TraceLevel >= 2) {
//                    printf
//                        ("* T = %ld, Period = %ld, P = %ld, BestW = %0.2f, Norm = %ld\n",
//                         T, Period, P, BestW / Precision, Norm);
//                    fflush(stdout);
//                }
                /* If in the initial phase, the step size is doubled */
                if (InitialPhase)
                    T *= 2;
                /* If the improvement was found at the last iteration of the current
                   period, then double the period */
                if (P == Period && (Period *= 2) > m_initialperiod)
                    Period = m_initialperiod;
            } else if (InitialPhase && P > Period / 2) {
                /* Conclude the initial phase */
                InitialPhase = 0;
                P = 0;
                T = 3 * T / 4;
            }
        }
    }

    t = FirstNode;
    do {
        free(t->CandidateSet);
        t->CandidateSet = 0;
        t->Pi = t->BestPi;
    }
    while ((t = t->Suc) != FirstNode);

    /* Compute a minimum 1-tree in the original graph */
    W = Minimum1TreeCost(0);

//    if (TraceLevel >= 2) {
//        printf("Ascent: BestW = %0.2f, Norm = %ld\n", BestW / Precision,
//               Norm);
//        fflush(stdout);
//    }
    return W;
}

/*
   The Backtrack2OptMove function searches for a tour improvement using backtracking
   and initial 2-opt moves.  
   
   In case a 2-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found, and a pointer to the node that
   was connected to t1 (in order to close the tour) is returned. Otherwise, 0 is
   returned.

   The function is called from the LinKernighan function. 
*/

Node *kGUITSP::Backtrack2OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3, *t4, *t;
    Candidate *Nt2;
    long G1, G2, G;

    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 (only one choice gives a closed tour) */
        t4 = PRED(t3);
        if (Fixed(t3, t4))
            continue;
        G2 = G1 + (this->*C)(t3, t4);
        if (!Forbidden(t4, t1) &&
            (!c || G2 - (this->*c)(t4, t1) > 0) && (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
            Make2OptMove(t1, t2, t3, t4);
            return t4;
        }
        if (G2 - t4->Cost <= 0)
            continue;
        Make2OptMove(t1, t2, t3, t4);
        Exclude(t1, t2);
        Exclude(t3, t4);
        G = G2;
        t = t4;
        while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
            if (*Gain > 0)
                return t;
        RestoreTour();
        if (t2 != SUC(t1))
            Reversed ^= 1;
    }
    *Gain = 0;
    return 0;
}

/* 
   The Backtrack3OptMove function searches for a tour improvement using backtracking
   and initial r-opt moves (2 <= r <= 3).  
   
   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found, and a pointer to the node that
   was connected to t1 (in order to close the tour) is returned. Otherwise, 0 is
   returned.

   The function is called from the LinKernighan function.   
*/

Node *kGUITSP::Backtrack3OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3, *t4, *t5, *t6, *t;
    Candidate *Nt2, *Nt4;
    long G1, G2, G3, G4, G;
    int Case6, X4, X6;

    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1) {
                if (!Forbidden(t4, t1) &&
                    (!c || G2 - (this->*c)(t4, t1) > 0) &&
                    (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                    Make2OptMove(t1, t2, t3, t4);
                    return t4;
                }
                if (G2 - t4->Cost <= 0)
                    continue;
                Make2OptMove(t1, t2, t3, t4);
                Exclude(t1, t2);
                Exclude(t3, t4);
                G = G2;
                t = t4;
                while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                    if (*Gain > 0)
                        return t;
                RestoreTour();
                if (t2 != SUC(t1))
                    Reversed ^= 1;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc ||
                    (G3 = G2 - Nt4->Cost) <= 0 ||
                    (X4 == 2 && !BETWEEN(t2, t5, t3)))
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= X4; X6++) {
                    if (X4 == 1) {
                        Case6 = 1 + !BETWEEN(t2, t5, t4);
                        t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                    } else {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if (!Forbidden(t6, t1) &&
                        (!c || G4 - (this->*c)(t6, t1) > 0) &&
                        (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        return t6;
                    }
                    if (G4 - t6->Cost <= 0)
                        continue;
                    Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                    Exclude(t1, t2);
                    Exclude(t3, t4);
                    Exclude(t5, t6);
                    G = G4;
                    t = t6;
                    while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                        if (*Gain > 0)
                            return t;
                    RestoreTour();
                    if (t2 != SUC(t1))
                        Reversed ^= 1;
                }
            }
        }
    }
    *Gain = 0;
    return 0;
}

/*
   The Backtrack4OptMove function searches for a tour improvement using backtracking
   and initial r-opt moves (2 <= r <= 4).  
   
   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found, and a pointer to the node that
   was connected to t1 (in order to close the tour) is returned. Otherwise, 0 is
   returned.

   The function is called from the LinKernighan function.   
*/

Node *kGUITSP::Backtrack4OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Candidate *Nt2, *Nt4, *Nt6;
    Node *t3, *t4, *t5, *t6=0, *t7, *t8=0, *t;
    long G1, G2, G3, G4, G5, G6, G;
    int Case6=0, Case8=0, X4, X6, X8;

    *Gain = 0;
    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1) {
                if (!Forbidden(t4, t1) &&
                    (!c || G2 - (this->*c)(t4, t1) > 0) &&
                    (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                    Make2OptMove(t1, t2, t3, t4);
                    return t4;
                }
                if (G2 - t4->Cost <= 0)
                    continue;
                Make2OptMove(t1, t2, t3, t4);
                Exclude(t1, t2);
                Exclude(t3, t4);
                G = G2;
                t = t4;
                while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                    if (*Gain > 0)
                        return t;
                RestoreTour();
                if (t2 != SUC(t1))
                    Reversed ^= 1;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc ||
                    (G3 = G2 - Nt4->Cost) <= 0)
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= 2; X6++) {
                    if (X4 == 1) {
                        if (X6 == 1) {
                            Case6 = 1 + !BETWEEN(t2, t5, t4);
                            t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                        } else {
                            t6 = t6 == t5->Pred ? t5->Suc : t5->Pred;
                            if ((t5 == t1 && t6 == t2) ||
                                (t5 == t2 && t6 == t1))
                                continue;
                            Case6 += 2;
                        }
                    } else if (BETWEEN(t2, t5, t3)) {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    } else {
                        if (X6 == 2)
                            break;
                        Case6 = 7;
                        t6 = X6 == 1 ? PRED(t5) : SUC(t5);
                        if (t6 == t2)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if (Case6 <= 2 || Case6 == 5 || Case6 == 6) {
                        if (!Forbidden(t6, t1) &&
                            (!c || G4 - (this->*c)(t6, t1) > 0) &&
                            (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                            Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                            return t6;
                        }
                        if (G4 - t6->Cost <= 0)
                            continue;
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        Exclude(t1, t2);
                        Exclude(t3, t4);
                        Exclude(t5, t6);
                        G = G4;
                        t = t6;
                        while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                            if (*Gain > 0)
                                return t;
                        RestoreTour();
                        if (t2 != SUC(t1))
                            Reversed ^= 1;
                    }
                    /* Choose (t6,t7) as a candidate edge emanating from t6 */
                    for (Nt6 = t6->CandidateSet; (t7 = Nt6->To); Nt6++) {
                        if (t7 == t6->Pred || t7 == t6->Suc ||
                            (t6 == t2 && t7 == t3) ||
                            (t6 == t3 && t7 == t2) ||
                            (G5 = G4 - Nt6->Cost) <= 0)
                            continue;
                        /* Choose t8 as one of t7's two neighbors on the tour */
                        for (X8 = 1; X8 <= 2; X8++) {
                            if (X8 == 1) {
                                Case8 = Case6;
                                t8 = 0;
                                switch (Case6) {
                                case 1:
                                    t8 = BETWEEN(t2, t7,
                                                 t5) ? SUC(t7) : PRED(t7);
                                    break;
                                case 2:
                                    t8 = BETWEEN(t3, t7,
                                                 t6) ? SUC(t7) : PRED(t7);
                                    break;
                                case 3:
                                    if (BETWEEN(t5, t7, t4))
                                        t8 = SUC(t7);
                                    break;
                                case 4:
                                    if (BETWEEN(t2, t7, t5))
                                        t8 = BETWEEN(t2, t7,
                                                     t4) ? SUC(t7) :
                                            PRED(t7);
                                    break;
                                case 5:
                                    t8 = PRED(t7);
                                    break;
                                case 6:
                                    t8 = BETWEEN(t2, t7,
                                                 t3) ? SUC(t7) : PRED(t7);
                                    break;
                                case 7:
                                    if (BETWEEN(t2, t7, t3))
                                        t8 = SUC(t7);
                                    break;
                                }
                                if (t8 == 0)
                                    break;
                            } else {
                                if (Case6 != 3 && Case6 != 4 && Case6 != 7)
                                    break;
                                t8 = t8 == t7->Pred ? t7->Suc : t7->Pred;
                                Case8 += 8;
                            }
                            if (t8 == t1 ||
                                (t7 == t3 && t8 == t4) ||
                                (t7 == t4 && t8 == t3))
                                continue;
                            if (Fixed(t7, t8))
                                continue;
                            G6 = G5 + (this->*C)(t7, t8);
                            if (t8 != t1) {
                                if (!Forbidden(t8, t1)
                                    && (!c || G6 - (this->*c)(t8, t1) > 0)
                                    && (*Gain = G6 - (this->*C)(t8, t1)) > 0) {
                                    Make4OptMove(t1, t2, t3, t4, t5, t6,
                                                 t7, t8, Case8);
                                    return t8;
                                }
                                if (G6 - t8->Cost <= 0)
                                    continue;
                                Make4OptMove(t1, t2, t3, t4, t5, t6, t7,
                                             t8, Case8);
                                Exclude(t1, t2);
                                Exclude(t3, t4);
                                Exclude(t5, t6);
                                Exclude(t7, t8);
                                G = G6;
                                t = t8;
                                while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                                    if (*Gain > 0)
                                        return t;
                                RestoreTour();
                                if (t2 != SUC(t1))
                                    Reversed ^= 1;
                            }
                        }
                    }
                }
            }
        }
    }
    *Gain = 0;
    return 0;
}

/*
   The Backtrack5OptMove function searches for a tour improvement using backtracking
   and initial r-opt moves (2 <= r <= 5).  
   
   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found, and a pointer to the node that
   was connected to t1 (in order to close the tour) is returned. Otherwise, 0 is
   returned.

   The function is called from the LinKernighan function. 
*/

Node *kGUITSP::Backtrack5OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3=0, *t4=0, *t5=0, *t6=0, *t7=0, *t8=0, *t9=0, *t10=0, *t=0;
    Candidate *Nt2, *Nt4, *Nt6, *Nt8;
    long G1, G2, G3, G4, G5, G6, G7, G8=0, G;
    int Case6=0, Case8=0, Case10=0, X4, X6, X8, X10, BTW275=0, BTW674=0, BTW571=0,
        BTW376=0, BTW574=0, BTW671=0, BTW471=0, BTW673=0, BTW573=0, BTW273=0;

    if (t2 != SUC(t1))
        Reversed ^= 1;

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1) {
                if (!Forbidden(t4, t1) &&
                    (!c || G2 - (this->*c)(t4, t1) > 0) &&
                    (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                    Make2OptMove(t1, t2, t3, t4);
                    return t4;
                }
                if (G2 - t4->Cost <= 0)
                    continue;
                Make2OptMove(t1, t2, t3, t4);
                Exclude(t1, t2);
                Exclude(t3, t4);
                G = G2;
                t = t4;
                while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                    if (*Gain > 0)
                        return t;
                RestoreTour();
                if (t2 != SUC(t1))
                    Reversed ^= 1;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc
                    || (G3 = G2 - Nt4->Cost) <= 0)
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= 2; X6++) {
                    if (X4 == 1) {
                        if (X6 == 1) {
                            Case6 = 1 + !BETWEEN(t2, t5, t4);
                            t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                        } else {
                            t6 = t6 == t5->Pred ? t5->Suc : t5->Pred;
                            if ((t5 == t1 && t6 == t2)
                                || (t5 == t2 && t6 == t1))
                                continue;
                            Case6 += 2;
                        }
                    } else if (BETWEEN(t2, t5, t3)) {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    } else {
                        Case6 = 6 + X6;
                        t6 = X6 == 1 ? PRED(t5) : SUC(t5);
                        if (t6 == t2)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if (Case6 <= 2 || Case6 == 5 || Case6 == 6) {
                        if (!Forbidden(t6, t1) &&
                            (!c || G4 - (this->*c)(t6, t1) > 0) &&
                            (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                            Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                            return t6;
                        }
                        if (G4 - t6->Cost <= 0)
                            continue;
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        Exclude(t1, t2);
                        Exclude(t3, t4);
                        Exclude(t5, t6);
                        G = G4;
                        t = t6;
                        while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                            if (*Gain > 0)
                                return t;
                        RestoreTour();
                        if (t2 != SUC(t1))
                            Reversed ^= 1;
                    }
                    /* Choose (t6,t7) as a candidate edge emanating from t6 */
                    for (Nt6 = t6->CandidateSet; (t7 = Nt6->To); Nt6++) {
                        if (t7 == t6->Pred || t7 == t6->Suc
                            || (t6 == t2 && t7 == t3) || (t6 == t3
                                                          && t7 == t2)
                            || (G5 = G4 - Nt6->Cost) <= 0)
                            continue;
                        /* Choose t8 as one of t7's two neighbors on the tour */
                        for (X8 = 1; X8 <= 2; X8++) {
                            if (X8 == 1) {
                                Case8 = Case6;
                                switch (Case6) {
                                case 1:
                                    if ((BTW275 = BETWEEN(t2, t7, t5)))
                                        t8 = SUC(t7);
                                    else {
                                        t8 = PRED(t7);
                                        BTW674 = BETWEEN(t6, t7, t4);
                                    }
                                    break;
                                case 2:
                                    if ((BTW376 = BETWEEN(t3, t7, t6)))
                                        t8 = SUC(t7);
                                    else {
                                        t8 = PRED(t7);
                                        BTW571 = BETWEEN(t5, t7, t1);
                                    }
                                    break;
                                case 3:
                                    t8 = SUC(t7);
                                    BTW574 = BETWEEN(t5, t7, t4);
                                    break;
                                case 4:
                                    if ((BTW671 = BETWEEN(t6, t7, t1)))
                                        t8 = PRED(t7);
                                    else
                                        t8 = BETWEEN(t2, t7,
                                                     t4) ? SUC(t7) :
                                            PRED(t7);
                                    break;
                                case 5:
                                    t8 = PRED(t7);
                                    BTW471 = BETWEEN(t4, t7, t1);
                                    if (!BTW471)
                                        BTW673 = BETWEEN(t6, t7, t3);
                                    break;
                                case 6:
                                    if ((BTW471 = BETWEEN(t4, t7, t1)))
                                        t8 = PRED(t7);
                                    else {
                                        t8 = SUC(t7);
                                        BTW573 = BETWEEN(t5, t7, t3);
                                    }
                                    break;
                                case 7:
                                case 8:
                                    t8 = SUC(t7);
                                    BTW273 = BETWEEN(t2, t7, t3);
                                    break;
                                }
                            } else {
                                t8 = t8 == t7->Pred ? t7->Suc : t7->Pred;
                                Case8 += 8;
                            }
                            if ((t7 == t1 && t8 == t2)
                                || (t7 == t2 && t8 == t1)
                                || (t7 == t3 && t8 == t4)
                                || (t7 == t4 && t8 == t3))
                                continue;
                            if (Fixed(t7, t8))
                                continue;
                            if (Case6 == 3 && !BTW574
                                && (X8 == 1) == BETWEEN(t3, t7, t1))
                                continue;
                            if (Case6 == 4 && BTW671 && X8 == 2)
                                break;
                            if (Case6 == 7 && !BTW273
                                && (X8 == 1) == BETWEEN(t5, t7, t1))
                                continue;
                            if (Case6 == 8 && !BTW273
                                && !BETWEEN(t4, t7, t5))
                                break;
                            G6 = G5 + (this->*C)(t7, t8);
                            if (t8 != t1
                                && (Case6 == 3 ? BTW574 : Case6 ==
                                    4 ? !BTW671 : Case6 ==
                                    7 ? BTW273 : Case6 != 8 && X8 == 1)) {
                                if (!Forbidden(t8, t1)
                                    && (!c || G6 - (this->*c)(t8, t1) > 0)
                                    && (*Gain = G6 - (this->*C)(t8, t1)) > 0) {
                                    Make4OptMove(t1, t2, t3, t4, t5, t6,
                                                 t7, t8, Case8);
                                    return t8;
                                }
                                if (G8 - t8->Cost <= 0)
                                    continue;
                                Make4OptMove(t1, t2, t3, t4, t5, t6, t7,
                                             t8, Case8);
                                Exclude(t1, t2);
                                Exclude(t3, t4);
                                Exclude(t5, t6);
                                Exclude(t7, t8);
                                G = G6;
                                t = t8;
                                while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                                    if (*Gain > 0)
                                        return t;
                                RestoreTour();
                                if (t2 != SUC(t1))
                                    Reversed ^= 1;
                            }
                            /* Choose (t8,t9) as a candidate edge emanating from t8 */
                            for (Nt8 = t8->CandidateSet; (t9 = Nt8->To); Nt8++) {
                                if (t9 == t8->Pred || t9 == t8->Suc
                                    || t9 == t1 || (t8 == t2 && t9 == t3)
                                    || (t8 == t3 && t9 == t2)
                                    || (t8 == t4 && t9 == t5)
                                    || (t8 == t5 && t9 == t4)
                                    || (G7 = G6 - Nt8->Cost) <= 0)
                                    continue;
                                /* Choose t10 as one of t9's two neighbors on the tour */
                                for (X10 = 1; X10 <= 2; X10++) {
                                    if (X10 == 1) {
                                        t10 = 0;
                                        switch (Case8) {
                                        case 1:
                                            t10 =
                                                (BTW275 ?
                                                 BETWEEN(t8, t9, t5)
                                                 || BETWEEN(t3, t9,
                                                            t1) : BTW674 ?
                                                 BETWEEN(t7, t9,
                                                         t1) : BETWEEN(t7,
                                                                       t9,
                                                                       t5))
                                                ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 22;
                                            break;
                                        case 2:
                                            t10 =
                                                (BTW376 ?
                                                 BETWEEN(t8, t9,
                                                         t4) : BTW571 ?
                                                 BETWEEN(t7, t9, t1)
                                                 || BETWEEN(t3, t9,
                                                            t6) :
                                                 BETWEEN(t7, t9,
                                                         t1)) ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 23;
                                            break;
                                        case 3:
                                            if (BTW574) {
                                                t10 =
                                                    BETWEEN(t5, t9,
                                                            t1) ? PRED(t9)
                                                    : SUC(t9);
                                                Case10 = 24;
                                                break;
                                            }
                                            if (!BETWEEN(t5, t9, t4))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 1;
                                            break;
                                        case 4:
                                            if (BTW671) {
                                                if (!BETWEEN(t2, t9, t5))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 2;
                                                break;
                                            }
                                            t10 =
                                                BETWEEN(t6, t9,
                                                        t4) ? PRED(t9) :
                                                SUC(t9);
                                            Case10 = 25;
                                            break;
                                        case 5:
                                            t10 =
                                                (BTW471 ?
                                                 BETWEEN(t7, t9,
                                                         t1) : BTW673 ?
                                                 BETWEEN(t7, t9,
                                                         t5) : BETWEEN(t4,
                                                                       t9,
                                                                       t1)
                                                 || BETWEEN(t7, t9,
                                                            t5)) ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 26;
                                            break;
                                        case 6:
                                            t10 =
                                                (BTW471 ?
                                                 BETWEEN(t7, t9,
                                                         t3) : BTW573 ?
                                                 BETWEEN(t8, t9,
                                                         t6) : BETWEEN(t4,
                                                                       t9,
                                                                       t1)
                                                 || BETWEEN(t8, t9,
                                                            t6)) ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 27;
                                            break;
                                        case 7:
                                            if (BTW273) {
                                                t10 =
                                                    BETWEEN(t5, t9,
                                                            t3) ? PRED(t9)
                                                    : SUC(t9);
                                                Case10 = 28;
                                                break;
                                            }
                                            if (!BETWEEN(t2, t9, t3))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 3;
                                            break;
                                        case 8:
                                            if (BTW273) {
                                                if (!BETWEEN(t4, t9, t5))
                                                    break;
                                                Case10 = 4;
                                            } else {
                                                if (!BETWEEN(t2, t9, t3))
                                                    break;
                                                Case10 = 5;
                                            }
                                            t10 = SUC(t9);
                                            break;
                                        case 9:
                                            if (BTW275) {
                                                if (!BETWEEN(t7, t9, t4))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 6;
                                                break;
                                            }
                                            if (!BTW674) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 7;
                                                break;
                                            }
                                            if (!BETWEEN(t6, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 8;
                                            break;
                                        case 10:
                                            if (BTW376) {
                                                if (!BETWEEN(t7, t9, t6))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 9;
                                                break;
                                            }
                                            if (BTW571) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 10;
                                                break;
                                            }
                                            if (!BETWEEN(t3, t9, t6)
                                                && !BETWEEN(t2, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 11;
                                            break;
                                        case 11:
                                            if (BTW574) {
                                                t10 =
                                                    BETWEEN(t3, t9,
                                                            t1) ? PRED(t9)
                                                    : SUC(t9);
                                                Case10 = 29;
                                                break;
                                            }
                                            if (!BETWEEN(t5, t9, t4))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 12;
                                            break;
                                        case 12:
                                            t10 =
                                                BETWEEN(t3, t9,
                                                        t1) ? PRED(t9) :
                                                SUC(t9);
                                            Case10 = 30;
                                            break;
                                        case 13:
                                            if (BTW471) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 13;
                                                break;
                                            }
                                            if (BTW673) {
                                                if (!BETWEEN(t6, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 14;
                                                break;
                                            }
                                            if (!BETWEEN(t6, t9, t3)
                                                && !BETWEEN(t2, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 15;
                                            break;
                                        case 14:
                                            if (BTW471) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 16;
                                                break;
                                            }
                                            if (BTW573) {
                                                if (!BETWEEN(t7, t9, t3)
                                                    && !BETWEEN(t2, t9,
                                                                t6))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 17;
                                                break;
                                            }
                                            if (!BETWEEN(t7, t9, t6))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 18;
                                            break;
                                        case 15:
                                            if (BTW273) {
                                                t10 =
                                                    BETWEEN(t5, t9,
                                                            t1) ? PRED(t9)
                                                    : SUC(t9);
                                                Case10 = 31;
                                                break;
                                            }
                                            if (!BETWEEN(t2, t9, t3))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 19;
                                            break;
                                        case 16:
                                            if (BTW273) {
                                                if (!BETWEEN(t4, t9, t5))
                                                    break;
                                                Case10 = 20;
                                            } else {
                                                if (!BETWEEN(t2, t9, t3))
                                                    break;
                                                Case10 = 21;
                                            }
                                            t10 = SUC(t9);
                                            break;
                                        }
                                        if (!t10)
                                            break;
                                    } else {
                                        if (Case10 >= 22)
                                            continue;
                                        Case10 += 31;
                                        t10 =
                                            t10 ==
                                            t9->Pred ? t9->Suc : t9->Pred;
                                    }
                                    if (t10 == t1
                                        || (t9 == t3 && t10 == t4)
                                        || (t9 == t4 && t10 == t3)
                                        || (t9 == t5 && t10 == t6)
                                        || (t9 == t6 && t10 == t5))
                                        continue;
                                    if (Fixed(t9, t10))
                                        continue;
                                    G8 = G7 + (this->*C)(t9, t10);
                                    if (!Forbidden(t10, t1)
                                        && (!c || G8 - (this->*c)(t10, t1) > 0)
                                        && (*Gain = G8 - (this->*C)(t10, t1)) > 0) {
                                        Make5OptMove(t1, t2, t3, t4, t5,
                                                     t6, t7, t8, t9, t10,
                                                     Case10);
                                        return t10;
                                    }
                                    if (G8 - t10->Cost <= 0)
                                        continue;
                                    Make5OptMove(t1, t2, t3, t4, t5, t6,
                                                 t7, t8, t9, t10, Case10);
                                    Exclude(t1, t2);
                                    Exclude(t3, t4);
                                    Exclude(t5, t6);
                                    Exclude(t7, t8);
                                    Exclude(t9, t10);
                                    G = G8;
                                    t = t10;
                                    while ((t = (this->*(BestMove))(t1, t, &G, Gain)))
                                        if (*Gain > 0)
                                            return t;
                                    RestoreTour();
                                    if (t2 != SUC(t1))
                                        Reversed ^= 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    *Gain = 0;
    return 0;
}
/*
   The Best2OptMove function makes sequential edge exchanges. If possible, it makes a
   2-opt move that improves the tour. Otherwise, it makes the most promising
   2-opt move that fulfils the positive gain criterion. To prevent an infinity chain
   of moves the last edge in a 2-opt move must not previously have been included in
   the chain. 

   The edge (t1,t2) is the first edge to be exchanged. G0 is a pointer to the 
   accumulated gain.

   In case a 2-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found.

   Otherwise, the best 2-opt move is made, and a pointer to the node that was connected 
   to t1 (in order to close the tour) is returned. The new accumulated gain is made 
   available to the caller through the parameter G0. 

   If no move can be made, the function returns 0.

   The function is called from the LinKernighan function. 
*/

Node *kGUITSP::Best2OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3=0, *t4, *T3=0, *T4 = 0;
    Candidate *Nt2;
    long G1, G2, BestG2 = LONG_MIN;

    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* 
       Determine (T3,T4) = (t3,t4)
       such that 

       G4 = *G0 - C(t2,T3) + C(T3,T4)

       is maximum (= BestG2), and (T3,T4) has not previously been included.
       If during this process a legal move with *Gain > 0 is found, then make
       the move and exit Best2OptMove immediately 
    */

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 (only one choice gives a closed tour) */
        t4 = PRED(t3);
        if (Fixed(t3, t4))
            continue;
        G2 = G1 + (this->*C)(t3, t4);
        if (!Forbidden(t4, t1) &&
            (!c || G2 - (this->*c)(t4, t1) > 0) && (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
            Swap1(t1, t2, t3);
            *G0 = G2;
            return t4;
        }
        if (G2 > BestG2 &&
            Swaps < MaxSwaps &&
            G2 - Precision >= t4->Cost &&
            Excludable(t3, t4) && !InOptimumTour(t3, t4)) {
            T3 = t3;
            T4 = t4;
            BestG2 = G2;
        }
    }
    *Gain = 0;
    if (T4) {
        /* Make the best 2-opt move */
        Swap1(t1, t2, T3);
        Exclude(t1, t2);
        Exclude(T3, T4);
        *G0 = BestG2;
    }
    return T4;
}

/*
   Below is shown the use of the variable X4 to discriminate between 
   cases considered by the algorithm. 

   The notation

   ab-

   is used for a subtour that starts with the edge (ta,tb). For example the tour 

   12-43-

   contains the edges (t1,t2) and (t4,t3), in that order. 

   X4 = 1:
       12-43-
   X4 = 2:
       12-34-
*/

/*
   The Best3OptMove function makes sequential edge exchanges. If possible, it makes an
   r-opt move (r <= 3) that improves the tour. Otherwise, it makes the most promising
   3-opt move that fulfils the positive gain criterion. To prevent an infinity chain
   of moves the last edge in a 3-opt move must not previously have been included in
   the chain. 

   The edge (t1,t2) is the first edge to be exchanged. G0 is a pointer to the 
   accumulated gain.

   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found.

   Otherwise, the best 3-opt move is made, and a pointer to the node that was connected 
   to t1 (in order to close the tour) is returned. The new accumulated gain is made 
   available to the caller through the parameter G0. 

   If no move can be made, the function returns 0.

   The function is called from the LinKernighan function. 
*/

/* 
   The algorithm splits the set of possible moves up into a number disjunct subsets
   (called "cases"). When t1, t2, ..., t6 has been chosen, Case6 is used to 
   discriminate between 8 cases.

   A description of the cases is given after the code.   
*/

Node *kGUITSP::Best3OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3=0, *t4=0, *t5=0, *t6, *T3=0, *T4=0, *T5=0, *T6 = 0;
    Candidate *Nt2, *Nt4;
    long G1, G2, G3, G4, BestG4 = LONG_MIN;
    int Case6=0, BestCase6=0, X4, X6;

    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* 
       Determine (T3,T4,T5,T6) = (t3,t4,t5,t6)
       such that 

       G4 = *G0 - C(t2,T3) + C(T3,T4)
       - C(T4,T5) + C(T5,T6)

       is maximum (= BestG4), and (T5,T6) has not previously been included.
       If during this process a legal move with *Gain > 0 is found, then make
       the move and exit Best3OptMove immediately. 
    */

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1 &&
                !Forbidden(t4, t1) &&
                (!c || G2 - (this->*c)(t4, t1) > 0) &&
                (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                Swap1(t1, t2, t3);
                *G0 = G2;
                return t4;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc ||
                    (G3 = G2 - Nt4->Cost) <= 0 ||
                    (X4 == 2 && !BETWEEN(t2, t5, t3)))
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= X4; X6++) {
                    if (X4 == 1) {
                        Case6 = 1 + !BETWEEN(t2, t5, t4);
                        t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                    } else {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if (!Forbidden(t6, t1) &&
                        (!c || G4 - (this->*c)(t6, t1) > 0) &&
                        (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        *G0 = G4;
                        return t6;
                    }
                    if (G4 > BestG4 &&
                        Swaps < MaxSwaps &&
                        G4 - Precision >= t6->Cost &&
                        Excludable(t5, t6) && !InOptimumTour(t5, t6)) {
                        /* Do not make the move if the gain does not vary */
                        if (RestrictedSearch &&
                            m_problemtype != HCP &&
                            m_problemtype != HPP &&
                            G3 + t5->Pi == G1 + t3->Pi)
                            continue;
                        T3 = t3;
                        T4 = t4;
                        T5 = t5;
                        T6 = t6;
                        BestCase6 = Case6;
                        BestG4 = G4;
                    }
                }
            }
        }
    }
    *Gain = 0;
    if (T6) {
        /* Make the best 3-opt move */
        Make3OptMove(t1, t2, T3, T4, T5, T6, BestCase6);
        Exclude(t1, t2);
        Exclude(T3, T4);
        Exclude(T5, T6);
        *G0 = BestG4;
    }
    return T6;
}

/*
   Below is shown the use of the variables X4 and Case6 to discriminate between 
   cases considered by the algorithm. 

   The notation

   ab-

   is used for a subtour that starts with the edge (ta,tb). For example the tour 

   12-43-

   contains the edges (t1,t2) and (t4,t3), in that order. 

   X4 = 1:
       12-43-
       Case6 = 1: 
           12-56-43-
       Case6 = 2:   
           12-43-65-
   X4 = 2:
       12-34-
       Case6 = 5: 
           12-56-34-
       Case6 = 6: 
           12-65-34-
*/


/*
   The Best4OptMove function makes sequential edge exchanges. If possible, it makes an
   r-opt move (r <= 4) that improves the tour. Otherwise, it makes the most promising
   4-opt move that fulfils the positive gain criterion. To prevent an infinity chain
   of moves the last edge in a 4-opt move must not previously have been included in
   the chain. 

   The edge (t1,t2) is the first edge to be exchanged.  G0 is a pointer to the 
   accumulated gain.

   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found.

   Otherwise, the best 4-opt move is made, and a pointer to the node that was connected 
   to t1 (in order to close the tour) is returned. The new accumulated gain is made 
   available to the caller through the parameter G0. 

   If no move can be made, the function returns 0.

   The function is called from the LinKernighan function. 
*/

/* 
   The algorithm splits the set of possible moves up into a number disjunct subsets
   (called "cases"). When t1, t2, ..., t6 has been chosen, Case6 is used to 
   discriminate between 8 cases. When t1, t2, ..., t8 has been chosen, Case8 is used 
   to discriminate between 16 cases. 

   A description of the cases is given after the code.   
*/

Node *kGUITSP::Best4OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Candidate *Nt2=0, *Nt4=0, *Nt6=0;
    Node *t3=0, *t4=0, *t5=0, *t6=0, *t7=0, *t8=0, *T3=0, *T4=0, *T5=0, *T6=0, *T7=0, *T8 = 0;
    long G1=0, G2=0, G3=0, G4=0, G5=0, G6=0, BestG6 = LONG_MIN;
    int Case6=0, Case8=0, BestCase8=0, X4=0, X6=0, X8=0;

    *Gain = 0;
    if (SUC(t1) != t2)
        Reversed ^= 1;

    /* 
       Determine (T3,T4,T5,T6,T7,T8) = (t3,t4,t5,t6,t7,t8)
       such that

       G8 = *G0 - C(t2,T3) + C(T3,T4)
       - C(T4,T5) + C(T5,T6)
       - C(T6,T7) + C(T7,T8)

       is maximum (= BestG6), and (T7,T8) has not previously been included.
       If during this process a legal move with *Gain > 0 is found, then make
       the move and exit Best4OptMove immediately. 
    */

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc || 
            ((G1 = *G0 - Nt2->Cost) <= 0  &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1 &&
                !Forbidden(t4, t1) &&
                (!c || G2 - (this->*c)(t4, t1) > 0) &&
                (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                Swap1(t1, t2, t3);
                *G0 = G2;
                return t4;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc ||
                    (G3 = G2 - Nt4->Cost) <= 0)
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= 2; X6++) {
                    if (X4 == 1) {
                        if (X6 == 1) {
                            Case6 = 1 + !BETWEEN(t2, t5, t4);
                            t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                        } else {
                            t6 = t6 == t5->Pred ? t5->Suc : t5->Pred;
                            if ((t5 == t1 && t6 == t2) ||
                                (t5 == t2 && t6 == t1))
                                continue;
                            Case6 += 2;
                        }
                    } else if (BETWEEN(t2, t5, t3)) {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    } else {
                        if (X6 == 2)
                            break;
                        Case6 = 7;
                        t6 = X6 == 1 ? PRED(t5) : SUC(t5);
                        if (t6 == t2)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if ((Case6 <= 2 || Case6 == 5 || Case6 == 6) &&
                        !Forbidden(t6, t1) &&
                        (!c || G4 - (this->*c)(t6, t1) > 0) &&
                        (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        *G0 = G4;
                        return t6;
                    }
                    /* Choose (t6,t7) as a candidate edge emanating from t6 */
                    for (Nt6 = t6->CandidateSet; (t7 = Nt6->To); Nt6++) {
                        if (t7 == t6->Pred || t7 == t6->Suc ||
                            (t6 == t2 && t7 == t3) ||
                            (t6 == t3 && t7 == t2) ||
                            (G5 = G4 - Nt6->Cost) <= 0)
                            continue;
                        /* Choose t8 as one of t7's two neighbors on the tour */
                        for (X8 = 1; X8 <= 2; X8++) {
                            if (X8 == 1) {
                                Case8 = Case6;
                                t8 = 0;
                                switch (Case6) {
                                case 1:
                                    t8 = BETWEEN(t2, t7,
                                                 t5) ? SUC(t7) : PRED(t7);
                                    break;
                                case 2:
                                    t8 = BETWEEN(t3, t7,
                                                 t6) ? SUC(t7) : PRED(t7);
                                    break;
                                case 3:
                                    if (BETWEEN(t5, t7, t4))
                                        t8 = SUC(t7);
                                    break;
                                case 4:
                                    if (BETWEEN(t2, t7, t5))
                                        t8 = BETWEEN(t2, t7,
                                                     t4) ? SUC(t7) :
                                            PRED(t7);
                                    break;
                                case 5:
                                    t8 = PRED(t7);
                                    break;
                                case 6:
                                    t8 = BETWEEN(t2, t7,
                                                 t3) ? SUC(t7) : PRED(t7);
                                    break;
                                case 7:
                                    if (BETWEEN(t2, t7, t3))
                                        t8 = SUC(t7);
                                    break;
                                }
                                if (t8 == 0)
                                    break;
                            } else {
                                if (Case6 != 3 && Case6 != 4 && Case6 != 7)
                                    break;
                                t8 = t8 == t7->Pred ? t7->Suc : t7->Pred;
                                Case8 += 8;
                            }
                            if (t8 == t1 ||
                                (t7 == t3 && t8 == t4) ||
                                (t7 == t4 && t8 == t3))
                                continue;
                            if (Fixed(t7, t8))
                                continue;
                            G6 = G5 + (this->*C)(t7, t8);
                            if (t8 != t1 &&
                                !Forbidden(t8, t1) &&
                                (!c || G6 - (this->*c)(t8, t1) > 0) &&
                                (*Gain = G6 - (this->*C)(t8, t1)) > 0) {
                                Make4OptMove(t1, t2, t3, t4, t5, t6, t7,
                                             t8, Case8);
                                *G0 = G6;
                                return t8;
                            }
                            if ((G6 > BestG6 ||
                                 (G6 == BestG6 && !Near(t7, t8)
                                  && Near(T7, T8)))
                                && G6 - Precision >= t8->Cost
                                && Swaps < MaxSwaps && Excludable(t7, t8)
                                && !InOptimumTour(t7, t8)) {
                                /* Do not make the move if the gain does not vary */
                                if (RestrictedSearch &&
                                    m_problemtype != HCP &&
                                    m_problemtype != HPP &&
                                    G5 + t7->Pi == G3 + t5->Pi &&
                                    G3 + t5->Pi == G1 + t3->Pi)
                                    continue;
                                T3 = t3;
                                T4 = t4;
                                T5 = t5;
                                T6 = t6;
                                T7 = t7;
                                T8 = t8;
                                BestCase8 = Case8;
                                BestG6 = G6;
                            }
                        }
                    }
                }
            }
        }
    }
    *Gain = 0;
    if (T8) {
        /* Make the best 4-opt move */
        Make4OptMove(t1, t2, T3, T4, T5, T6, T7, T8, BestCase8);
        Exclude(t1, t2), Exclude(T3, T4);
        Exclude(T5, T6);
        Exclude(T7, T8);
        *G0 = BestG6;
    }
    return T8;
}

/*
   Below is shown the use of the variables X4, Case6, Case8 and Case10 to discriminate
   between cases considered by the algorithm. 

   The notation

   ab-

   is used for a subtour that starts with the edge (ta,tb). For example the tour 

   12-43-

   contains the edges (t1,t2) and (t4,t3), in that order. 

   X4 = 1:
       12-43-
       Case6 = 1: 
           12-56-43-
           Case8 = 1: 
               12-78-56-43-, 12-56-87-43-, 12-56-43-87-
       Case6 = 2:   
           12-43-65-
           Case8 = 2: 
               12-87-43-65-, 12-43-78-65-, 12-43-65-87-
       Case6 = 3: 
           12-65-43-
           Case8 = 3:
               12-65-78-43-
           Case8 = 11:
               12-65-87-43-
       Case6 = 4: 
           12-43-56-
           Case8 = 4: 
               12-78-43-56, 12-43-87-56
           Case8 = 12:
               12-87-43-56-
   X4 = 2:
       12-34-
       Case6 = 5: 
           12-56-34-
           Case8 = 5: 
               12-87-56-34-, 12-56-87-34-, 12-56-34-87-
           Case8 = 13:
               12-56-87-34-
        Case6 = 6: 
           12-65-34-
           Case8 = 6: 
               12-78-65-34-, 12-65-34-87-
           Case8 = 14:
               12-65-87-34-        
       Case6 = 7: 
           12-34-65-
           Case8 = 7: 
               12-78-34-65-
           Case8 = 15:
               12-87-34-65-
*/

/*
   The Best5OptMove function makes sequential edge exchanges. If possible, it makes an
   r-opt move (r <= 5) that improves the tour. Otherwise, it makes the most promising
   5-opt move that fulfils the positive gain criterion. To prevent an infinity chain
   of moves the last edge in a 5-opt move must not previously have been included in
   the chain. 

   The edge (t1,t2) is the first edge to be exchanged.  G0 is a pointer to the 
   accumulated gain.

   In case a r-opt move is found that improves the tour, the improvement of the cost
   is made available to the caller through the parameter Gain. If *Gain > 0, an 
   improvement of the current tour has been found.

   Otherwise, the best 5-opt move is made, and a pointer to the node that was connected 
   to t1 (in order to close the tour) is returned. The new accumulated gain is made 
   available to the caller through the parameter G0. 

   If no move can be made, the function returns 0.

   The function is called from the LinKernighan function. 
*/

/* 
   The algorithm splits the set of possible moves up into a number disjunct subsets
   (called "cases"). When t1, t2, ..., t6 has been chosen, Case6 is used to 
   discriminate between 8 cases. When t1, t2, ..., t8 has been chosen, Case8 is used 
   to discriminate between 16 cases. When t1, t2, ..., t10 has been chosen, Case10 is
   used to discriminate between 52 cases.

   A description of the cases is given after the code.   
*/

Node *kGUITSP::Best5OptMove(Node * t1, Node * t2, long *G0, long *Gain)
{
    Node *t3=0, *t4=0, *t5=0, *t6=0, *t7=0, *t8=0, *t9=0, *t10=0;
    Node *T3=0, *T4=0, *T5=0, *T6=0, *T7=0, *T8=0, *T9=0, *T10 = 0;
    Candidate *Nt2, *Nt4, *Nt6, *Nt8;
    long G1, G2, G3, G4, G5, G6, G7, G8, BestG8 = LONG_MIN;
    int Case6=0, Case8=0, Case10=0, BestCase10=0, X4=0, X6=0, X8=0, X10=0, BTW275=0, BTW674=0,
        BTW571=0, BTW376=0, BTW574=0, BTW671=0, BTW471=0, BTW673=0, BTW573=0, BTW273=0;

    if (t2 != SUC(t1))
        Reversed ^= 1;

    /* 
       Determine (T3,T4,T5,T6,T7,T8,T9,T10) = (t3,t4,t5,t6,t7,t8,t9,t10)
       such that

       G8 = *G0 - C(t2,T3) + C(T3,T4)
       - C(T4,T5) + C(T5,T6)
       - C(T6,T7) + C(T7,T8)
       - C(T8,T9) + C(T9,T10)

       is maximum (= BestG8), and (T9,T10) has not previously been included.
       If during this process a legal move with *Gain > 0 is found, then make
       the move and exit Best5OptMove immediately. 
    */

    /* Choose (t2,t3) as a candidate edge emanating from t2 */
    for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
        if (t3 == t2->Pred || t3 == t2->Suc ||
            ((G1 = *G0 - Nt2->Cost) <= 0 &&
             m_problemtype != HCP && m_problemtype != HPP))
            continue;
        /* Choose t4 as one of t3's two neighbors on the tour */
        for (X4 = 1; X4 <= 2; X4++) {
            t4 = X4 == 1 ? PRED(t3) : SUC(t3);
            if (Fixed(t3, t4))
                continue;
            G2 = G1 + (this->*C)(t3, t4);
            if (X4 == 1 &&
                !Forbidden(t4, t1) &&
                (!c || G2 - (this->*c)(t4, t1) > 0) &&
                (*Gain = G2 - (this->*C)(t4, t1)) > 0) {
                Make2OptMove(t1, t2, t3, t4);
                return t4;
            }
            /* Choose (t4,t5) as a candidate edge emanating from t4 */
            for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                if (t5 == t4->Pred || t5 == t4->Suc ||
                    (G3 = G2 - Nt4->Cost) <= 0)
                    continue;
                /* Choose t6 as one of t5's two neighbors on the tour */
                for (X6 = 1; X6 <= 2; X6++) {
                    if (X4 == 1) {
                        if (X6 == 1) {
                            Case6 = 1 + !BETWEEN(t2, t5, t4);
                            t6 = Case6 == 1 ? SUC(t5) : PRED(t5);
                        } else {
                            t6 = t6 == t5->Pred ? t5->Suc : t5->Pred;
                            if ((t5 == t1 && t6 == t2) ||
                                (t5 == t2 && t6 == t1))
                                continue;
                            Case6 += 2;
                        }
                    } else if (BETWEEN(t2, t5, t3)) {
                        Case6 = 4 + X6;
                        t6 = X6 == 1 ? SUC(t5) : PRED(t5);
                        if (t6 == t1)
                            continue;
                    } else {
                        Case6 = 6 + X6;
                        t6 = X6 == 1 ? PRED(t5) : SUC(t5);
                        if (t6 == t2)
                            continue;
                    }
                    if (Fixed(t5, t6))
                        continue;
                    G4 = G3 + (this->*C)(t5, t6);
                    if ((Case6 <= 2 || Case6 == 5 || Case6 == 6) &&
                        !Forbidden(t6, t1) &&
                        (!c || G4 - (this->*c)(t6, t1) > 0) &&
                        (*Gain = G4 - (this->*C)(t6, t1)) > 0) {
                        Make3OptMove(t1, t2, t3, t4, t5, t6, Case6);
                        return t6;
                    }
                    /* Choose (t6,t7) as a candidate edge emanating from t6 */
                    for (Nt6 = t6->CandidateSet; (t7 = Nt6->To); Nt6++) {
                        if (t7 == t6->Pred || t7 == t6->Suc ||
                            (t6 == t2 && t7 == t3) ||
                            (t6 == t3 && t7 == t2) ||
                            (G5 = G4 - Nt6->Cost) <= 0)
                            continue;
                        /* Choose t8 as one of t7's two neighbors on the tour */
                        for (X8 = 1; X8 <= 2; X8++) {
                            if (X8 == 1) {
                                Case8 = Case6;
                                switch (Case6) {
                                case 1:
                                    if ((BTW275 = BETWEEN(t2, t7, t5)))
                                        t8 = SUC(t7);
                                    else {
                                        t8 = PRED(t7);
                                        BTW674 = BETWEEN(t6, t7, t4);
                                    }
                                    break;
                                case 2:
                                    if ((BTW376 = BETWEEN(t3, t7, t6)))
                                        t8 = SUC(t7);
                                    else {
                                        t8 = PRED(t7);
                                        BTW571 = BETWEEN(t5, t7, t1);
                                    }
                                    break;
                                case 3:
                                    t8 = SUC(t7);
                                    BTW574 = BETWEEN(t5, t7, t4);
                                    break;
                                case 4:
                                    if ((BTW671 = BETWEEN(t6, t7, t1)))
                                        t8 = PRED(t7);
                                    else
                                        t8 = BETWEEN(t2, t7,
                                                     t4) ? SUC(t7) :
                                            PRED(t7);
                                    break;
                                case 5:
                                    t8 = PRED(t7);
                                    BTW471 = BETWEEN(t4, t7, t1);
                                    if (!BTW471)
                                        BTW673 = BETWEEN(t6, t7, t3);
                                    break;
                                case 6:
                                    if ((BTW471 = BETWEEN(t4, t7, t1)))
                                        t8 = PRED(t7);
                                    else {
                                        t8 = SUC(t7);
                                        BTW573 = BETWEEN(t5, t7, t3);
                                    }
                                    break;
                                case 7:
                                case 8:
                                    t8 = SUC(t7);
                                    BTW273 = BETWEEN(t2, t7, t3);
                                    break;
                                }
                            } else {
                                t8 = t8 == t7->Pred ? t7->Suc : t7->Pred;
                                Case8 += 8;
                            }
                            if ((t7 == t1 && t8 == t2) ||
                                (t7 == t2 && t8 == t1) ||
                                (t7 == t3 && t8 == t4) ||
                                (t7 == t4 && t8 == t3))
                                continue;
                            if (Fixed(t7, t8))
                                continue;
                            if (Case6 == 3 && !BTW574 &&
                                (X8 == 1) == BETWEEN(t3, t7, t1))
                                continue;
                            if (Case6 == 4 && BTW671 && X8 == 2)
                                break;
                            if (Case6 == 7 && !BTW273 &&
                                (X8 == 1) == BETWEEN(t5, t7, t1))
                                continue;
                            if (Case6 == 8 && !BTW273
                                && !BETWEEN(t4, t7, t5))
                                break;
                            G6 = G5 + (this->*C)(t7, t8);
                            if (t8 != t1 &&
                                (Case6 == 3 ? BTW574 :
                                 Case6 == 4 ? !BTW671 :
                                 Case6 == 7 ? BTW273 :
                                 Case6 != 8 && X8 == 1) &&
                                !Forbidden(t8, t1) &&
                                (!c || G6 - (this->*c)(t8, t1) > 0) &&
                                (*Gain = G6 - (this->*C)(t8, t1)) > 0) {
                                Make4OptMove(t1, t2, t3, t4, t5, t6, t7,
                                             t8, Case8);
                                return t8;
                            }
                            /* Choose (t8,t9) as a candidate edge emanating from t8 */
                            for (Nt8 = t8->CandidateSet; (t9 = Nt8->To);
                                 Nt8++) {
                                if (t9 == t8->Pred || t9 == t8->Suc
                                    || t9 == t1 || (t8 == t2 && t9 == t3)
                                    || (t8 == t3 && t9 == t2) || (t8 == t4
                                                                  && t9 ==
                                                                  t5)
                                    || (t8 == t5 && t9 == t4)
                                    || (G7 = G6 - Nt8->Cost) <= 0)
                                    continue;
                                /* Choose t10 as one of t9's two neighbors on the tour */
                                for (X10 = 1; X10 <= 2; X10++) {
                                    if (X10 == 1) {
                                        t10 = 0;
                                        switch (Case8) {
                                        case 1:
                                            t10 = (BTW275 ?
                                                   BETWEEN(t8, t9, t5)
                                                   || BETWEEN(t3, t9,
                                                              t1) : BTW674
                                                   ? BETWEEN(t7, t9,
                                                             t1) :
                                                   BETWEEN(t7, t9,
                                                           t5)) ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 22;
                                            break;
                                        case 2:
                                            t10 = (BTW376 ?
                                                   BETWEEN(t8, t9, t4) :
                                                   BTW571 ?
                                                   BETWEEN(t7, t9, t1)
                                                   || BETWEEN(t3, t9,
                                                              t6) :
                                                   BETWEEN(t7, t9,
                                                           t1)) ? PRED(t9)
                                                : SUC(t9);
                                            Case10 = 23;
                                            break;
                                        case 3:
                                            if (BTW574) {
                                                t10 = BETWEEN(t5, t9, t1) ?
                                                    PRED(t9) : SUC(t9);
                                                Case10 = 24;
                                                break;
                                            }
                                            if (!BETWEEN(t5, t9, t4))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 1;
                                            break;
                                        case 4:
                                            if (BTW671) {
                                                if (!BETWEEN(t2, t9, t5))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 2;
                                                break;
                                            }
                                            t10 = BETWEEN(t6, t9, t4) ?
                                                PRED(t9) : SUC(t9);
                                            Case10 = 25;
                                            break;
                                        case 5:
                                            t10 = (BTW471 ?
                                                   BETWEEN(t7, t9, t1) :
                                                   BTW673 ?
                                                   BETWEEN(t7, t9, t5) :
                                                   BETWEEN(t4, t9, t1)
                                                   || BETWEEN(t7, t9,
                                                              t5)) ?
                                                PRED(t9) : SUC(t9);
                                            Case10 = 26;
                                            break;
                                        case 6:
                                            t10 = (BTW471 ?
                                                   BETWEEN(t7, t9, t3) :
                                                   BTW573 ?
                                                   BETWEEN(t8, t9, t6) :
                                                   BETWEEN(t4, t9, t1)
                                                   || BETWEEN(t8, t9,
                                                              t6)) ?
                                                PRED(t9) : SUC(t9);
                                            Case10 = 27;
                                            break;
                                        case 7:
                                            if (BTW273) {
                                                t10 = BETWEEN(t5, t9, t3) ?
                                                    PRED(t9) : SUC(t9);
                                                Case10 = 28;
                                                break;
                                            }
                                            if (!BETWEEN(t2, t9, t3))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 3;
                                            break;
                                        case 8:
                                            if (BTW273) {
                                                if (!BETWEEN(t4, t9, t5))
                                                    break;
                                                Case10 = 4;
                                            } else {
                                                if (!BETWEEN(t2, t9, t3))
                                                    break;
                                                Case10 = 5;
                                            }
                                            t10 = SUC(t9);
                                            break;
                                        case 9:
                                            if (BTW275) {
                                                if (!BETWEEN(t7, t9, t4))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 6;
                                                break;
                                            }
                                            if (!BTW674) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 7;
                                                break;
                                            }
                                            if (!BETWEEN(t6, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 8;
                                            break;
                                        case 10:
                                            if (BTW376) {
                                                if (!BETWEEN(t7, t9, t6))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 9;
                                                break;
                                            }
                                            if (BTW571) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 10;
                                                break;
                                            }
                                            if (!BETWEEN(t3, t9, t6) &&
                                                !BETWEEN(t2, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 11;
                                            break;
                                        case 11:
                                            if (BTW574) {
                                                t10 = BETWEEN(t3, t9, t1) ?
                                                    PRED(t9) : SUC(t9);
                                                Case10 = 29;
                                                break;
                                            }
                                            if (!BETWEEN(t5, t9, t4))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 12;
                                            break;
                                        case 12:
                                            t10 = BETWEEN(t3, t9, t1) ?
                                                PRED(t9) : SUC(t9);
                                            Case10 = 30;
                                            break;
                                        case 13:
                                            if (BTW471) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 13;
                                                break;
                                            }
                                            if (BTW673) {
                                                if (!BETWEEN(t6, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 14;
                                                break;
                                            }
                                            if (!BETWEEN(t6, t9, t3) &&
                                                !BETWEEN(t2, t9, t7))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 15;
                                            break;
                                        case 14:
                                            if (BTW471) {
                                                if (!BETWEEN(t2, t9, t7))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 16;
                                                break;
                                            }
                                            if (BTW573) {
                                                if (!BETWEEN(t7, t9, t3) &&
                                                    !BETWEEN(t2, t9, t6))
                                                    break;
                                                t10 = SUC(t9);
                                                Case10 = 17;
                                                break;
                                            }
                                            if (!BETWEEN(t7, t9, t6))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 18;
                                            break;
                                        case 15:
                                            if (BTW273) {
                                                t10 = BETWEEN(t5, t9, t1) ?
                                                    PRED(t9) : SUC(t9);
                                                Case10 = 31;
                                                break;
                                            }
                                            if (!BETWEEN(t2, t9, t3))
                                                break;
                                            t10 = SUC(t9);
                                            Case10 = 19;
                                            break;
                                        case 16:
                                            if (BTW273) {
                                                if (!BETWEEN(t4, t9, t5))
                                                    break;
                                                Case10 = 20;
                                            } else {
                                                if (!BETWEEN(t2, t9, t3))
                                                    break;
                                                Case10 = 21;
                                            }
                                            t10 = SUC(t9);
                                            break;
                                        }
                                        if (!t10)
                                            break;
                                    } else {
                                        if (Case10 >= 22)
                                            continue;
                                        Case10 += 31;
                                        t10 =
                                            t10 ==
                                            t9->Pred ? t9->Suc : t9->Pred;
                                    }
                                    if (t10 == t1 ||
                                        (t9 == t3 && t10 == t4) ||
                                        (t9 == t4 && t10 == t3) ||
                                        (t9 == t5 && t10 == t6) ||
                                        (t9 == t6 && t10 == t5))
                                        continue;
                                    if (Fixed(t9, t10))
                                        continue;
                                    G8 = G7 + (this->*C)(t9, t10);
                                    if (!Forbidden(t10, t1) &&
										(!c || G8 - (this->*c)(t10, t1) > 0) &&
                                        (*Gain = G8 - (this->*C)(t10, t1)) > 0) {
                                        Make5OptMove(t1, t2, t3, t4, t5,
                                                     t6, t7, t8, t9, t10,
                                                     Case10);
                                        return t10;
                                    }
                                    if ((G8 > BestG8 ||
                                         (G8 == BestG8 && Near(T9, T10)
                                          && !Near(t9, t10)))
                                        && G8 - Precision >= t10->Cost
                                        && Swaps < MaxSwaps
                                        && Excludable(t9, t10)
                                        && !InOptimumTour(t9, t10)) {
                                        /* Do not make the move if the gain does not vary */
                                        if (RestrictedSearch &&
                                            m_problemtype != HCP &&
                                            m_problemtype != HPP &&
                                            (G7 + t9->Pi == G5 + t7->Pi &&
                                             G5 + t7->Pi == G3 + t5->Pi &&
                                             G3 + t5->Pi == G1 + t3->Pi))
                                            continue;
                                        T3 = t3;
                                        T4 = t4;
                                        T5 = t5;
                                        T6 = t6;
                                        T7 = t7;
                                        T8 = t8;
                                        T9 = t9;
                                        T10 = t10;
                                        BestCase10 = Case10;
                                        BestG8 = G8;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    *Gain = 0;
    if (T10) {
        /* Make the best 5-opt move */
        Make5OptMove(t1, t2, T3, T4, T5, T6, T7, T8, T9, T10, BestCase10);
        Exclude(t1, t2);
        Exclude(T3, T4);
        Exclude(T5, T6);
        Exclude(T7, T8);
        Exclude(T9, T10);
        *G0 = BestG8;
    }
    return T10;
}

/*
   Below is shown the use of the variables X4, Case6, Case8 and Case10 to discriminate
   between cases considered by the algorithm. 

   The notation

   ab-

   is used for a subtour that starts with the edge (ta,tb). For example the tour 

   12-43-

   contains the edges (t1,t2) and (t4,t3), in that order. 

   X4 = 1:
       12-43-
       Case6 = 1: 
           12-56-43-
           Case8 = 1: 
               12-78-56-43-, 12-56-87-43-, 12-56-43-87-
               Case10 = 22:
                   12-910-78-56-43-, 12-78-109-56-43-, 12-78-56-910-43-, 12-78-56-43-109-
                   12-910-56-87-43-, 12-56-910-87-43-, 12-56-87-109-43-, 12-56-87-43-109- 
                   12-109-56-43-87-, 12-56-910-43-87-, 12-56-43-910-87-, 12-56-43-87-109-
           Case8 = 9:
               12-87-56-43-, 12-56-78-43-, 12-56-43-78-
               Case10 =  6:
                   12-87-910-56-43-, 12-87-56-910-43-
               Case10 =  7:
                   12-910-56-43-78-, 12-56-910-43-78-, 12-56-43-910-78-
               Case10 =  8:
                   12-56-910-78-43-
               Case10 = 37:
                   12-87-109-56-43-, 12-87-56-109-43-
               Case10 = 38:
                   12-109-56-43-78-, 12-56-109-43-78-, 12-56-43-109-78-
               Case10 = 39:
                   12-56-109-78-43-
       Case6 = 2:   
           12-43-65-
           Case8 = 2: 
               12-87-43-65-, 12-43-78-65-, 12-43-65-87-
               Case10 = 2:
                   12-910-87-43-65-, 12-87-910-43-65-, 12-87-43-910-65-, 12-87-43-65-910-
                   12-109-43-78-65-, 12-43-910-78-65-, 12-43-78-109-65-, 12-43-78-65-109-
                   12-910-43-65-87-, 12-43-109-65-87-, 12-43-65-910-87-, 12-43-65-87-109-
           Case8 = 10:
               12-78-43-65-, 12-43-87-65-, 12-43-65-78-
               Case10 = 9:
                   12-43-87-910-65-
               Case10 = 10:
                   12-910-43-65-78-, 12-43-910-65-78-, 12-43-65-910-78-
               Case10 = 11:
                   12-910-78-43-65-, 12-78-43-910-43-
               Case10 = 40:
                   12-43-87-109-65-
               Case10 = 41:
                   12-109-43-65-78-, 12-43-109-65-78-, 12-43-65-109-78-
               Case10 = 42:
                   12-109-78-43-65-, 12-78-43-109-43-
       Case6 = 3: 
           12-65-43-
           Case8 = 3:
               12-78-65-43-, 12-65-78-43-
               Case10 = 24:
                   12-910-65-78-43-, 12-65-109-78-43-, 12-65-78-109-43-, 12-65-78-43-109-
               Case10 = 1:
                   12-78-65-910-43-
               Case10 = 32:
                   12-78-65-109-43
           Case8 = 11:
               12-65-87-43-, 12-65-43-87-
               Case10 = 29:
                   12-910-65-87-43-, 12-65-910-87-43-, 12-65-87-910-43-, 12-65-87-43-109-
               Case10 = 22:
                   12-65-910-43-87-
               Case10 = 43:
                   12-65-109-43-87-
       Case6 = 4: 
           12-43-56-
           Case8 = 4: 
               12-78-43-56, 12-43-87-56, 12-43-56-87-
               Case10 = 2:
                   12-910-43-56-87-, 12-43-910-56-87-
               Case10 = 25:
                   12-109-78-43-56-, 12-78-109-43-56-, 12-67-43-910-56-, 12-67-43-56-109-
                   12-109-43-87-56-, 12-43-910-87-56-, 12-43-87-910-56-, 12-43-87-56-109-
               Case10 = 33:
                   12-109-43-56-87-, 12-43-109-56-87-
           Case8 = 12:
               12-87-43-56-, 12-43-78-56-
           Case10 = 30:
               12-910-87-43-56-, 12-87-910-43-56-, 12-87-43-109-56-, 12-87-43-56-109-
               12-910-43-78-56-, 12-43-109-78-56-, 12-43-78-109-56-, 12-43-78-56-109-
   X4 = 2:
       12-34-
       Case6 = 5: 
           12-56-34-
           Case8 = 5: 
               12-87-56-34-, 12-56-87-34-, 12-56-34-87-
               Case10 = 26:
                   12-910-87-56-34-, 12-87-109-56-34-, 12-87-56-910-34-, 12-87-56-34-109-
                   12-109-56-87-34-, 12-56-910-87-34-, 12-56-87-109-34-, 12-56-87-34-109-
                   12-910-56-34-87-, 12-56-910-34-87-, 12-56-34-910-87-, 12-56-34-87-109-
           Case8 = 13:
               12-78-56-34-, 12-56-78-34-, 12-56-34-78-
               Case10 = 13:
                   12-910-56-34-78-, 12-56-910-34-78-, 12-56-34-910-78-
               Case10 = 14:
                   12-56-910-78-34-
               Case10 = 15:
                   12-910-78-56-34-, 12-78-56-910-34-
               Case10 = 44:
                   12-109-56-34-78-, 12-56-109-34-78-, 12-56-34-109-78-
               Case10 = 45:
                   12-56-109-78-34-
               Case10 = 46:
                   12-109-78-56-34-, 12-78-56-109-34-
       Case6 = 6: 
           12-65-34-
           Case8 = 6: 
               12-87-65-34-, 12-65-78-34-, 12-65-34-87-
               Case10 = 8:
                   12-910-87-65-34-, 12-87-109-65-34-, 12-87-65-910-34-, 12-87-65-34-109-
                   12-109-65-78-34-, 12-65-910-78-34-, 12-65-78-109-34-, 12-65-78-34-109- 
                   12-910-65-34-87-, 12-65-910-34-87-, 12-65-34-910-87-, 12-65-34-87-109-
           Case8 = 14:
               12-87-65-34-, 12-65-87-34-, 12-65-34-78-
               Case10 = 16:
                   12-910-65-34-78-, 12-65-910-34-78-, 12-65-34-910-78-
               Case10 = 17:
                   12-910-65-87-34-, 12-65-87-910-34-
               Case10 = 18:
                   12-87-910-65-34-
               Case10 = 47:
                  12-109-65-34-78-, 12-65-109-34-78-, 12-65-34-109-78-
               Case10 = 48:
                  12-109-65-87-34-, 12-65-87-109-34-
               Case10 = 49:
                  12-87-109-65-34-                             
       Case6 = 7: 
           12-34-65-
            Case8 = 7: 
                12-78-34-65-, 12-34-78-65-
                Case10 = 28:
                    12-109-78-34-65-, 12-78-109-34-65-, 12-78-34-910-34-, 12-78-34-65-109-
                Case10 = 3:
                    12-910-34-78-65-
                Case10 = 34:
                    12-109-34-78-65-
            Case8 = 15:
                12-87-34-65-, 12-34-87-65-
                Case10 = 31:
                    12-910-87-34-65-, 12-87-910-34-65-, 12-87-34-910-65-, 12-87-34-65-109-
                Case10 = 19:
                    12-910-34-87-65-
                Case10 = 50:
                    12-109-34-87-65-
       Case6 = 8: 
           12-34-56-
           Case8 = 8: 
               12-78-34-56-, 12-34-78-56-
               Case10 = 4:
                   12-78-34-910-56-
               Case10 = 5:
                   12-910-34-78-56-
               Case10 = 35:
                   12-78-34-109-56-
               Case10 = 36:
                   12-109-34-78-56-
           Case8 = 16:
               12-87-34-56-, 12-34-87-56-
               Case10 = 20:
                   12-87-34-910-56-
               Case10 = 21:
                   12-910-34-87-56-
               Case10 = 51:
                   12-87-34-109-56-
               Case10 = 52:
                   12-109-34-87-56-
*/

/* 
   The Between function is used to determine whether a node
   is between two other nodes with respect to the current
   orientation. The function is only used if the doubly linked
   list representation is used for a tour; if the two-level
   tree representation is used, the function Between_SL is used
   instead.
   	
   Between(ta,tb,tc) returns 1 if node tb is between node ta and tc.
   Otherwise, 0 is returned.
   	
   The function is called from the functions BestMove, Gain23,
   BridgeGain, Make4OptMove and Make5OptMove.
*/

int kGUITSP::Between(const Node * ta, const Node * tb, const Node * tc)
{
    long a, b = tb->Rank, c;

    if (!Reversed) {
        a = ta->Rank;
        c = tc->Rank;
    } else {
        a = tc->Rank;
        c = ta->Rank;
    }
    return a <= c ? b >= a && b <= c : b >= a || b <= c;
}


/*
   The Between_SL function is used to determine whether a node is 
   between two other nodes with respect to the current orientation. 
   The function is only used if the two-level tree representation 
   is used for a tour; if the doubly linked list representation is 
   used, the function Between is used instead.
   	
   Between_SL(a,b,c) returns 1 if node b is between node a and c.
   Otherwise, 0 is returned.
   	
   The function is called from the functions BestMove, Gain23,
   BridgeGain, Make4OptMove and Make5OptMove.
*/

int kGUITSP::Between_SL(const Node * ta, const Node * tb, const Node * tc)
{
    const Segment *Pa, *Pb, *Pc;

    if (tb == ta || tb == tc)
        return 1;
    if (ta == tc)
        return 0;
    Pa = ta->Parent;
    Pb = tb->Parent;
    Pc = tc->Parent;
    if (Pa == Pc) {
        if (Pb == Pa)
            return (Reversed == Pa->Reversed) ==
                (ta->Rank < tc->Rank ?
                 tb->Rank > ta->Rank && tb->Rank < tc->Rank :
                 tb->Rank > ta->Rank || tb->Rank < tc->Rank);
        return (Reversed == Pa->Reversed) == (ta->Rank > tc->Rank);
    }
    if (Pb == Pc)
        return (Reversed == Pb->Reversed) == (tb->Rank < tc->Rank);
    if (Pa == Pb)
        return (Reversed == Pa->Reversed) == (ta->Rank < tb->Rank);
    return !Reversed ==
        (Pa->Rank < Pc->Rank ?
         Pb->Rank > Pa->Rank && Pb->Rank < Pc->Rank :
         Pb->Rank > Pa->Rank || Pb->Rank < Pc->Rank);
}


/*
   The BridgeGain function attempts to improve the tour by making a nonsequential move. 
	
   The function is called by the Gain23 function. 
	
   For any nonfeasible 2-opt move that would cause the current tour to be split into 
   two separate tours, BridgGain may be called in order to find a (nonfeasible) 2- or
   3-opt move that reconnects the two separate tours into a tour which is shorter than 
   the original one. In some cases, the second move may even be 4-opt.
	
   For any nonfeasible 3-opt move that would cause the current tour to be split into 
   two separate tours, BridgGain may be called in order to find a (nonfeasible) 2-opt 
   move that reconnects the two separate tours into a tour which is shorter than the 
   original one.
	
   The parameters s1, s2, ..., s8 denote the end nodes of edges that are part of the
   nonfeasible move suggested by Gain23. The parameter Case6 is used specify the move 
   type (Case6 = 0 when 2-opt, Case6 = 3, 4 or 7 when 3- or 4-opt). The parameter G  
   contains the gain achieved by making the move.
	
   If the composite move results in a shorter tour, then the move is made, and the 
   function returns the gain achieved.	
*/

long kGUITSP::BridgeGain(Node * s1, Node * s2, Node * s3, Node * s4,
                Node * s5, Node * s6, Node * s7, Node * s8,
                int Case6, long G)
{
    Node *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *u2=0, *u3=0;
    Candidate *Nt2, *Nt4, *Nt6;
    long G0, G1, G2, G3, G4, G5, G6, Gain, i;
    int X4;

    /* From the original tour select a segment (u2 --> u3) which contains as
       few nodes as possible. The number of nodes in a segment is computed from
       the rank (V) of its end points */
    switch (Case6) {
    case 3:
        if ((i = !Reversed ? s4->V - s5->V : s5->V - s4->V) < 0)
            i += m_dimension;
        if (2 * i <= m_dimension) {
            u2 = s5;
            u3 = s4;
        } else {
            u2 = s3;
            u3 = s6;
        }
        break;
    case 4:
        if ((i = !Reversed ? s5->V - s2->V : s2->V - s5->V) < 0)
            i += m_dimension;
        if (2 * i <= m_dimension) {
            u2 = s2;
            u3 = s5;
        } else {
            u2 = s6;
            u3 = s1;
        }
        break;
    case 0:
    case 7:
        if ((i = !Reversed ? s3->V - s2->V : s2->V - s3->V) < 0)
            i += m_dimension;
        if (2 * i <= m_dimension) {
            u2 = s2;
            u3 = s3;
        } else {
            u2 = s4;
            u3 = s1;
        }
    }

    /* Choose t1 between u2 and u3 */
    for (t1 = u2; t1 != u3; t1 = t2) {
        /* Choose t2 as the successor of t1 */
        t2 = SUC(t1);
        if ((t1 == s1 && t2 == s2) ||
            (t1 == s2 && t2 == s1) ||
            (t1 == s3 && t2 == s4) ||
            (t1 == s4 && t2 == s3) ||
            (t1 == s5 && t2 == s6) ||
            (t1 == s6 && t2 == s5) ||
            (t1 == s7 && t2 == s8) ||
            (t1 == s8 && t2 == s7) || Fixed(t1, t2))
            continue;
        G0 = G + (this->*C)(t1, t2);
        /* Choose (t2,t3) as a candidate edge emanating from t2. 
           t3 must not be between u2 and u3 */
        for (Nt2 = t2->CandidateSet; (t3 = Nt2->To); Nt2++) {
            if (t3 == t2->Pred || t3 == t2->Suc || BETWEEN(u2, t3, u3))
                continue;
            G1 = G0 - Nt2->Cost;
            /* Choose t4 as one of t3's two neighbors on the tour */
            for (X4 = 1; X4 <= 2; X4++) {
                t4 = X4 == 1 ? SUC(t3) : PRED(t3);
                if (t4 == t2)
                    continue;
                if ((t3 == s1 && t4 == s2) ||
                    (t3 == s2 && t4 == s1) ||
                    (t3 == s3 && t4 == s4) ||
                    (t3 == s4 && t4 == s3) ||
                    (t3 == s5 && t4 == s6) ||
                    (t3 == s6 && t4 == s5) ||
                    (t3 == s7 && t4 == s8) ||
                    (t3 == s8 && t4 == s7) || Fixed(t3, t4))
                    continue;
                G2 = G1 + (this->*C)(t3, t4);
                /* test if an improvement can be obtained */
                if (!Forbidden(t4, t1) &&
                    (!c || G2 - (this->*c)(t4, t1) > 0) &&
                    (Gain = G2 - (this->*C)(t4, t1)) > 0) {
                    switch (Case6) {
                    case 0:
                        if (X4 == 1)
                            Swap3(s1, s2, s4, t3, t4, t1, s1, s3, s2);
                        else
                            Swap2(t1, t2, t3, s1, s2, s3);
                        return Gain;
                    case 3:
                        if ((X4 == 1) ==
                            (!BETWEEN(s2, t1, s6) && !BETWEEN(s2, t3, s6)))
                            Swap3(s1, s2, s3, t1, t2, t3, s5, s6, s1);
                        else
                            Swap4(s1, s2, s3, t1, t2, t4, s5, s6, s1, t2,
                                  t4, t1);
                        if (s8)
                            Swap1(s7, s8, s1);
                        return Gain;
                    case 4:
                        if ((X4 == 1) ==
                            (!BETWEEN(s3, t1, s5) && !BETWEEN(s3, t3, s5)))
                            Swap3(s1, s2, s3, t1, t2, t3, s5, s6, s1);
                        else
                            Swap4(s1, s2, s3, t1, t2, t4, s5, s6, s1, t2,
                                  t4, t1);
                        if (s8)
                            Swap1(s7, s8, s1);
                        return Gain;
                    case 7:
                        if ((X4 == 1) ==
                            (!BETWEEN(s4, t1, s6) && !BETWEEN(s4, t3, s6)))
                            Swap3(s5, s6, s1, t1, t2, t3, s3, s4, s5);
                        else
                            Swap4(s5, s6, s1, t1, t2, t4, s3, s4, s5, t2,
                                  t4, t1);
                        if (s8)
                            Swap1(s7, s8, s1);
                        return Gain;
                    }
                }
                /* If BridgeGain has been called with a nonfeasible 2-opt move,
                   then try to find a 3-opt or 4-opt move which, when composed with 
                   the 2-opt move, results in an improvement of the tour */
                if (Case6 != 0)
                    continue;
                /* Choose (t4,t5) as a candidate edge emanating from t4 */
                for (Nt4 = t4->CandidateSet; (t5 = Nt4->To); Nt4++) {
                    if (t5 == t4->Pred || t5 == t4->Suc ||
                        t5 == t1 || t5 == t2)
                        continue;
                    /* Choose t6 as one of t5's two neighbors on the tour.
                       Only one choice! */
                    t6 = X4 == 1
                        || BETWEEN(u2, t5, u3) ? PRED(t5) : SUC(t5);
                    if ((t5 == s1 && t6 == s2) || (t5 == s2 && t6 == s1)
                        || (t5 == s3 && t6 == s4) || (t5 == s4 && t6 == s3)
                        || Fixed(t5, t6))
                        continue;
                    G3 = G2 - Nt4->Cost;
                    G4 = G3 + (this->*C)(t5, t6);
                    if (!Forbidden(t6, t1) &&
                        (!c || G4 - (this->*c)(t6, t1) > 0) &&
                        (Gain = G4 - (this->*C)(t6, t1)) > 0) {
                        if (X4 == 1)
                            Swap4(s1, s2, s4, t3, t4, t1, s1, s3, s2, t5,
                                  t6, t1);
                        else
                            Swap3(t1, t2, t3, s1, s2, s3, t5, t6, t1);
                        return Gain;
                    }
                    /* Choose (t7,t8) as a candidate edge emanating from t7.
                       Only one choice! */
                    for (Nt6 = t6->CandidateSet; (t7 = Nt6->To); Nt6++) {
                        if (t7 == t6->Pred || t7 == t6->Suc)
                            continue;
                        /* Choose t8 as one of t7's two neighbors on the tour.
                           Only one choice! */
                        if (X4 == 1)
                            t8 = (BETWEEN(u2, t5, t1) ? BETWEEN(t5, t7, t1)
                                  : BETWEEN(t2, t5, u3) ? BETWEEN(u2, t7,
                                                                  t1)
                                  || BETWEEN(t5, t7, u3) : BETWEEN(SUC(u3),
                                                                   t5,
                                                                   t3) ?
                                  BETWEEN(u2, t7, u3)
                                  || BETWEEN(t5, t7, t3) : !BETWEEN(t4, t7,
                                                                    t6)) ?
                                PRED(t7) : SUC(t7);
                        else
                            t8 = (BETWEEN(u2, t5, t1) ?
                                  !BETWEEN(u2, t7, t6)
                                  && !BETWEEN(t2, t7, u3) : BETWEEN(t2, t5,
                                                                    u3) ?
                                  !BETWEEN(t2, t7, t6) : BETWEEN(SUC(u3),
                                                                 t5,
                                                                 t4) ?
                                  !BETWEEN(SUC(u3), t7, t5)
                                  && !BETWEEN(t3, t7,
                                              PRED(u2)) : !BETWEEN(t3, t7,
                                                                   t5)) ?
                                PRED(t7) : SUC(t7);
                        if (t8 == t1
                            || (t7 == t1 && t8 == t2) || (t7 == t3
                                                          && t8 == t4)
                            || (t7 == t4 && t8 == t3) || (t7 == s1
                                                          && t8 == s2)
                            || (t7 == s2 && t8 == s1) || (t7 == s3
                                                          && t8 == s4)
                            || (t7 == s4 && t8 == s3))
                            continue;
                        if (Fixed(t7, t8) || Forbidden(t8, t1))
                            continue;
                        G5 = G4 - Nt6->Cost;
                        G6 = G5 + (this->*C)(t7, t8);
                        /* Test if an improvement can be achieved  */
                        if ((!c || G6 - (this->*c)(t8, t1) > 0) &&
                            (Gain = G6 - (this->*C)(t8, t1)) > 0) {
                            if (X4 == 1)
                                Swap4(s1, s2, s4, t3, t4, t1, s1, s3, s2,
                                      t5, t6, t1);
                            else
                                Swap3(t1, t2, t3, s1, s2, s3, t5, t6, t1);
                            Swap1(t7, t8, t1);
                            return Gain;
                        }
                    }
                }
            }
        }
    }
    /* No improvement has been found */
    return 0;
}

/* 
   Functions for computing the transformed distance of an edge (Na,Nb). 
*/

/* 
   The C_EXPLICIT function returns the distance by looking it up in a table. 
*/


long kGUITSP::C_EXPLICIT(Node * Na, Node * Nb)
{
    return Na->Id < Nb->Id ? Nb->C[Na->Id] : Na->C[Nb->Id];
}

/*
   The C_FUNCTION function is used when the distance is defined by a
   function (e.g. the Euclidean distance function). In order to speed
   up the computations the following algorithm used:
	
   (1) If the edge (Na,Nb) is a candidate edge incident to Na, then
       its distance is available in the field Cost of the corresponding
       Candidate structure.
	    
   (2) A hash table (CacheVal) is consulted to see if the distance has
       been stored. 
	    
   (3) Otherwise the distance function is called and the distance computed
       is stored in the hash table.
	    
   [ see Bentley (1990): K-d trees for semidynamic point sets. ] 
	      
*/

long kGUITSP::C_FUNCTION(Node * Na, Node * Nb)
{
    Node *Nc;
    Candidate *Cand;
    long Index, i, j;

    if ((Cand = Na->CandidateSet))
        for (; (Nc = Cand->To); Cand++)
            if (Nc == Nb)
                return Cand->Cost;
    if (CacheSig == 0)
        return (this->*D)(Na, Nb);
    i = Na->Id;
    j = Nb->Id;
    Index = i ^ j;
    if (i > j)
        i = j;
    if (CacheSig[Index] == i)
        return CacheVal[Index];
    CacheSig[Index] = i;
    return (CacheVal[Index] = (this->*D)(Na, Nb));
}

long kGUITSP::D_EXPLICIT(Node * Na, Node * Nb)
{
    return (Na->Id <
            Nb->Id ? Nb->C[Na->Id] : Na->C[Nb->Id]) + Na->Pi + Nb->Pi;
}

long kGUITSP::D_FUNCTION(Node * Na, Node * Nb)
{
    return (Fixed(Na, Nb) ? 0 : (this->*Distance)(Na, Nb) * Precision) + Na->Pi +
        Nb->Pi;
}

/* Functions for computing lower bounds for the distance functions */

long kGUITSP::c_CEIL_2D(Node * Na, Node * Nb)
{
    long dx = (long)(ceil(fabs(Na->X - Nb->X)));
	long dy = (long)(ceil(fabs(Na->Y - Nb->Y)));
    return (long)((dx > dy ? dx : dy) * Precision + Na->Pi + Nb->Pi);
}

long kGUITSP::c_CEIL_3D(Node * Na, Node * Nb)
{
    long dx = (long)(ceil(fabs(Na->X - Nb->X)));
	long dy = (long)(ceil(fabs(Na->Y - Nb->Y)));
	long dz = (long)(ceil(fabs(Na->Z - Nb->Z)));
    if (dy > dx)
        dx = dy;
    if (dz > dx)
        dx = dz;
    return (long)(dx * Precision + Na->Pi + Nb->Pi);
}

long kGUITSP::c_EUC_2D(Node * Na, Node * Nb)
{
    long dx = (long)(fabs(Na->X - Nb->X) + 0.5);
	long dy = (long)(fabs(Na->Y - Nb->Y) + 0.5);
    return (long)((dx > dy ? dx : dy) * Precision + Na->Pi + Nb->Pi);
}

long kGUITSP::c_EUC_3D(Node * Na, Node * Nb)
{
    long dx = (long)(fabs(Na->X - Nb->X) + 0.5);
	long dy = (long)(fabs(Na->Y - Nb->Y) + 0.5);
	long dz = (long)(fabs(Na->Z - Nb->Z) + 0.5);
    if (dy > dx)
        dx = dy;
    if (dz > dx)
        dx = dz;
    return (long)(dx * Precision + Na->Pi + Nb->Pi);
}

//#define PI 3.141592
#define RRR 6378.388

long kGUITSP::c_GEO(Node * Na, Node * Nb)
{
    long da = (long)(Na->X);
	long db = (long)(Nb->X);
    double ma = Na->X - da;
	double mb = Nb->X - db;
    long dx = (long)(RRR * PI / 180.0 * fabs(da - db + 5.0 * (ma - mb) / 3.0));
    return (long)(dx * Precision + Na->Pi + Nb->Pi);
}

#undef M_PI
#define M_PI 3.14159265358979323846264

long kGUITSP::c_GEOM(Node * Na, Node * Nb)
{
    long dx = (long)(6378388.0 * M_PI / 180.0 * fabs(Na->X - Nb->X) + 1.0);
    return (long)(dx * Precision + Na->Pi + Nb->Pi);
}

/*
   The ChooseInitialTour function generates a pseudo random initial tour. 
   The algorithm constructs a tour as follows. 

   First, a random node, N, is chosen.

   Then, as long as no all nodes have been chosen, choose the next node to 
   follow N in the tour, NextN, and set N equal to NextN.

   NextN is chosen as follows: 

		(A) If possible, choose NextN so that
			(N,NextN) is a candidate edge,
			the alpha-value of (N,NextN) is zero, and
			(N,NextN) belongs to the current best tour.
		(B) Otherwise, if possible, choose NextN such that 
			(N,NextN) is a candidate edge.
		(C) Otherwise, choose NextN among those nodes not already chosen.

   When more than one node may be chosen, the node is chosen at random 
   among the alternatives (a one-way list of nodes). 

   The sequence of chosen nodes constitutes the initial tour.
*/

void kGUITSP::ChooseInitialTour(void)
{
    Node *N, *NextN, *FirstAlternative, *Last;
    Candidate *NN;
    long i;

    /* Choose a random node N = FirstFirstNode */
    N = FirstNode = &NodeSet[1 + rand() % m_dimension];

    /* Mark all nodes as "not chosen" by setting their V field to zero */
    do
        N->V = 0;
    while ((N = N->Suc) != FirstNode);
    
    /* Choose FirstNode without two incident fixed edges */
    do {
        if (!N->FixedTo2)
            break;
    } while ((N = N->Suc) != FirstNode);
    FirstNode = N;
   
    /* Move nodes with two incident fixed edges before FirstNode */
    for (Last = FirstNode->Pred; N != Last; N = NextN) {
        NextN = N->Suc;
        if (N->FixedTo2)
            Follow(N, Last);
    }

    /* Mark FirstNode as chosen */
    FirstNode->V = 1;
    N = FirstNode;

    /* Loop as long as not all nodes have been chosen */
    while (N->Suc != FirstNode) {
        if (N->InitialSuc && Trial == 1)
            NextN = N->InitialSuc;
        else {
            for (NN = N->CandidateSet; (NextN = NN->To); NN++)
                if (!NextN->V && Fixed(N, NextN))
                    break;
        }
        if (NextN == 0) {
            FirstAlternative = 0;
            i = 0;
            if (m_problemtype != HCP && m_problemtype != HPP) {
                /* Try case A0 */
                for (NN = N->CandidateSet; (NextN = NN->To); NN++) {
                    if (!NextN->V && !NextN->FixedTo2 &&
                        Near(N, NextN) && IsCommonEdge(N, NextN)) {
                        i++;
                        NextN->Next = FirstAlternative;
                        FirstAlternative = NextN;
                    }
                }
            }
            if (i == 0 && m_maxcandidates > 0 &&
                m_problemtype != HCP && m_problemtype != HPP) {
                /* Try case A */
                for (NN = N->CandidateSet; (NextN = NN->To); NN++) {
                    if (!NextN->V && !NextN->FixedTo2 &&
                        NN->Alpha == 0 && InBestTour(N, NextN)) {
                        i++;
                        NextN->Next = FirstAlternative;
                        FirstAlternative = NextN;
                    }
                }
            }
            if (i == 0) {
                /* Try case B */
                for (NN = N->CandidateSet; (NextN = NN->To); NN++) {
                    if (!NextN->V && !NextN->FixedTo2) {
                        i++;
                        NextN->Next = FirstAlternative;
                        FirstAlternative = NextN;
                    }
                }
            }
            if (i == 0) {
                /* Try case C (actually, not really a random choice) */
                NextN = N->Suc;
                while ((NextN->FixedTo2 || Forbidden(N, NextN))
                       && NextN->Suc != FirstNode)
                    NextN = NextN->Suc;
            } else {
                NextN = FirstAlternative;
                if (i > 1) {
                    /* Select NextN at random among the alternatives */
                    i = rand() % i;
                    while (i--)
                        NextN = NextN->Next;
                }
            }
        }
        /* Include NextN as the successor of N */
        Follow(NextN, N);
        N = NextN;
        N->V = 1;
    }
}

/*
   Let T be a minimum spanning tree on the graph, and let N1 be a node of
   degree one in T. The Connect function determines a shortest edge emanating
   from N1, but not in T. At return, the Next field of N1 points to the end node
   of the edge, and its NextCost field contains the cost of the edge. However, 
   the search for the shortest edge is stopped if an edge shorter than a 
   specified threshold (Max) is found.
*/

void kGUITSP::Connect(Node * N1, const long Max, const int Sparse)
{
    Node *N;
    Candidate *NN1;
    long d;

    N1->Next = 0;
    N1->NextCost = LONG_MAX;
    if (!Sparse || N1->CandidateSet == 0) {
        /* Find the requested edge in a dense graph */
        N = FirstNode;
        do {
            if (N == N1 || N == N1->Dad || N1 == N->Dad)
                continue;
            if (Fixed(N1, N)) {
                N1->NextCost = (this->*D)(N1, N);
                N1->Next = N;
                return;
            }
            if (!N1->FixedTo2 && !N->FixedTo2 &&
                !Forbidden(N1, N) &&
                (!c || (this->*c)(N1, N) < N1->NextCost) &&
                (d = (this->*D)(N1, N)) < N1->NextCost) {
                N1->NextCost = d;
                if (d <= Max)
                    return;
                N1->Next = N;
            }
        }
        while ((N = N->Suc) != FirstNode);
    } else {
        /* Find the requested edge in a sparse graph */
        for (NN1 = N1->CandidateSet; (N = NN1->To); NN1++) {
            if (N == N1->Dad || N1 == N->Dad)
                continue;
            if (Fixed(N1, N)) {
                N1->NextCost = NN1->Cost + N1->Pi + N->Pi;
                N1->Next = N;
                return;
            }
            if (!N1->FixedTo2 && !N->FixedTo2 &&
                (d = NN1->Cost + N1->Pi + N->Pi) < N1->NextCost) {
                N1->NextCost = d;
                if (d <= Max)
                    return;
                N1->Next = N;
            }
        }
    }
}

/*
   The CreateCandidateSet function determines for each node its set of incident
   candidate edges.

   The Ascent function is called to determine a lower bound on the optimal tour 
   using subgradient optimization. But only if the penalties (the pi-values) is
   not available on file. In the latter case, the penalties is read from the file,
   and the lower bound is computed from a minimum 1-tree.      

   The function GenerateCandidates is called to compute the alpha-values and to 
   associate to each node a set of incident candidate edges.  

   The CreateCandidateSet function itself is called from LKmain.
*/

void kGUITSP::CreateCandidateSet(void)
{
    double Cost;
    long i,Count;
//	long j, Id, Alpha;
    Node *Na, *Nb;
    Candidate *NNa, *NNb;

    if (m_problemtype == HPP) {
	    m_Norm = 9999;
        return;
    }
	if (C == &kGUITSP::C_EXPLICIT) {
        Na = FirstNode;
        do {
            for (i = 1; i < Na->Id; i++)
                Na->C[i] *= Precision;
        }
        while ((Na = Na->Suc) != FirstNode);
    }
#if 1
    /* No PiFile specified or available */
    Cost = Ascent();
#else
	if (PiFileName == 0 || (PiFile = fopen(PiFileName, "r")) == 0)
	{
        /* No PiFile specified or available */
        Cost = Ascent();
    }
	else
	{
        /* Read the Pi-values from file */
        fscanf(PiFile, "%ld", &Id);
        assert(Id >= 1 && Id <= m_dimension);
        FirstNode = Na = &NodeSet[Id];
        fscanf(PiFile, "%ld", &Na->Pi);
        for (i = 2; i <= m_dimension; i++) {
            fscanf(PiFile, "%ld", &Id);
            assert(Id >= 1 && Id <= m_dimension);
            Nb = &NodeSet[Id];
            fscanf(PiFile, "%ld", &Nb->Pi);
            Nb->Pred = Na;
            Na->Suc = Nb;
            Na = Nb;
        }
        FirstNode->Pred = Nb;
        Nb->Suc = FirstNode;
        fclose(PiFile);
        if (CandidateFileName == 0 ||
            (CandidateFile = fopen(CandidateFileName, "r")) == 0)
            Cost = Minimum1TreeCost(0);
        else {
            if (m_maxcandidates == 0) {
                Na = FirstNode;
                do {
                    Na->CandidateSet =
                        (Candidate *) malloc(sizeof(Candidate));
                    Na->CandidateSet[0].To = 0;
                } while ((Na = Na->Suc) != FirstNode);
            } else {
                for (i = 1; i <= m_dimension; i++) {
                    fscanf(CandidateFile, "%ld", &Id);
                    assert(Id >= 1 && Id <= m_dimension);
                    Na = &NodeSet[Id];
                    fscanf(CandidateFile, "%ld", &Id);
                    assert(Id >= 0 && Id <= m_dimension);
                    Na->Dad = Id ? &NodeSet[Id] : 0;
                    assert(Na != Na->Dad);
                    fscanf(CandidateFile, "%ld", &Count);
                    Na->CandidateSet =
                        (Candidate *) malloc(((Count < m_maxcandidates ? Count : m_maxcandidates) + 1) *
                                             sizeof(Candidate));
                    for (j = 0, NNa = Na->CandidateSet; j < Count; j++) {
                        fscanf(CandidateFile, "%ld", &Id);
                        assert(Id >= 0 && Id <= m_dimension);
                        fscanf(CandidateFile, "%ld", &Alpha);
                        if (j < m_maxcandidates) {
                            NNa->To = &NodeSet[Id];
                            NNa->Cost = (this->*D)(Na, NNa->To);
                            NNa->Alpha = Alpha;
                            NNa++;
                        }
                    }
                    NNa->To = 0;
                }
            }
            fclose(CandidateFile);
            Norm = 9999;
            goto End_CreateCandidateSet;
        }
    }
#endif
	LowerBound = Cost / Precision;
//    printf("\nLower bound = %0.1f, ", LowerBound);
//    if (Optimum != -DBL_MAX && Optimum != 0)
//        printf("Gap = %0.1f%%, ", 100 * (Optimum - LowerBound) / Optimum);
//    printf("Ascent time = %0.0f sec.\n", GetTime() - LastTime);
//    fflush(stdout);
//    if (m_Norm == 0)
  //      return;
    GenerateCandidates(m_maxcandidates, (long)(fabs(Excess * Cost)), m_candidatesetsymmetric);
#if 0
	if (CandidateFileName && (CandidateFile = fopen(CandidateFileName, "w"))) {
        Na = FirstNode;
        do {
            fprintf(CandidateFile, "%ld %ld", Na->Id,
                    Na->Dad ? Na->Dad->Id : 0);
            Count = 0;
            for (NNa = Na->CandidateSet; NNa->To; NNa++)
                Count++;
            fprintf(CandidateFile, " %d ", Count);
            for (NNa = Na->CandidateSet; NNa->To; NNa++)
                fprintf(CandidateFile, " %ld %ld", (NNa->To)->Id,
                        NNa->Alpha);
            fprintf(CandidateFile, "\n");
        } while ((Na = Na->Suc) != FirstNode);
        fprintf(CandidateFile, "-1\nEOF\n");
        fclose(CandidateFile);
    }
End_CreateCandidateSet:
#endif

		if (C == &kGUITSP::C_EXPLICIT) {
        Na = FirstNode;
        do {
            Nb = Na;
            while ((Nb = Nb->Suc) != FirstNode) {
                if (Na->Id > Nb->Id)
                    Na->C[Nb->Id] += Na->Pi + Nb->Pi;
                else
                    Nb->C[Na->Id] += Na->Pi + Nb->Pi;
            }
        }
        while ((Na = Na->Suc) != FirstNode);
    }
    /* Read tours to be merged */
    for (i = 0; i <= 1; i++) {
        if (FirstNode->MergeSuc[i]) {
            Na = FirstNode;
            do {
                Nb = Na->MergeSuc[i];
                Count = 0;
                for (NNa = Na->CandidateSet; NNa->To && NNa->To != Nb;
                     NNa++)
                    Count++;
                if (!NNa->To) {
                    NNa->Cost = (this->*C)(Na, Nb);
                    NNa->To = Nb;
                    NNa->Alpha = 0;
                    assert(Na->CandidateSet =
                           (Candidate *) realloc(Na->CandidateSet,
                                                 (Count +
                                                  2) * sizeof(Candidate)));
                    Na->CandidateSet[Count + 1].To = 0;
                }
                Count = 0;
                for (NNb = Nb->CandidateSet; NNb->To && NNb->To != Na;
                     NNb++)
                    Count++;
                if (!NNb->To) {
                    NNb->Cost = (this->*C)(Na, Nb);
                    NNb->To = Na;
                    NNb->Alpha = 0;
                    assert(Nb->CandidateSet =
                           (Candidate *) realloc(Nb->CandidateSet,
                                                 (Count +
                                                  2) * sizeof(Candidate)));
                    Nb->CandidateSet[Count + 1].To = 0;
                }
            } while ((Na = Nb) != FirstNode);
        }
    }
}

/* 
   Functions for computing distances (see TSPLIB).

   The appropriate function is referenced by the function pointer Distance.
*/

long kGUITSP::Distance_1(Node * Na, Node * Nb)
{
    return 1;
}

long kGUITSP::Distance_ATSP(Node * Na, Node * Nb)
{
    long n = m_dimension / 2;
    if ((Na->Id <= n) == (Nb->Id <= n))
        return M;
    if (labs(Na->Id - Nb->Id) == n)
        return 0;
    return Na->Id < Nb->Id ? Na->C[Nb->Id - n] : Nb->C[Na->Id - n];
}

long kGUITSP::Distance_ATT(Node * Na, Node * Nb)
{
    double xd = Na->X - Nb->X, yd = Na->Y - Nb->Y;
    return (long)(ceil(sqrt((xd * xd + yd * yd) / 10.0)));
}

long kGUITSP::Distance_CEIL_2D(Node * Na, Node * Nb)
{
    double xd = Na->X - Nb->X, yd = Na->Y - Nb->Y;
    return (long)(ceil(sqrt(xd * xd + yd * yd)));
}

long kGUITSP::Distance_CEIL_3D(Node * Na, Node * Nb)
{
    double xd = Na->X - Nb->X, yd = Na->Y - Nb->Y, zd = Na->Z - Nb->Z;
    return (long)(ceil(sqrt(xd * xd + yd * yd + zd * zd)));
}

long kGUITSP::Distance_EXPLICIT(Node * Na, Node * Nb)
{
    return (long)(Na->Id < Nb->Id ? Nb->C[Na->Id] : Na->C[Nb->Id]);
}

long kGUITSP::Distance_EUC_2D(Node * Na, Node * Nb)
{
    double xd = Na->X - Nb->X, yd = Na->Y - Nb->Y;
    return (long)(sqrt(xd * xd + yd * yd) + 0.5);
}

long kGUITSP::Distance_EUC_3D(Node * Na, Node * Nb)
{
    double xd = Na->X - Nb->X, yd = Na->Y - Nb->Y, zd = Na->Z - Nb->Z;
    return (long)(sqrt(xd * xd + yd * yd + zd * zd) + 0.5);
}

//#define PI 3.141592
#define RRR 6378.388

long kGUITSP::Distance_GEO(Node * Na, Node * Nb)
{
    long deg;
    double NaLatitude, NaLongitude, NbLatitude, NbLongitude, min, q1, q2,
        q3;
    deg = (long)(Na->X);
    min = Na->X - deg;
    NaLatitude = PI * (deg + 5.0 * min / 3.0) / 180.0;
    deg = (long)(Na->Y);
    min = Na->Y - deg;
    NaLongitude = PI * (deg + 5.0 * min / 3.0) / 180.0;
    deg = (long)(Nb->X);
    min = Nb->X - deg;
    NbLatitude = PI * (deg + 5.0 * min / 3.0) / 180.0;
    deg = (long)(Nb->Y);
    min = Nb->Y - deg;
    NbLongitude = PI * (deg + 5.0 * min / 3.0) / 180.0;
    q1 = cos(NaLongitude - NbLongitude);
    q2 = cos(NaLatitude - NbLatitude);
    q3 = cos(NaLatitude + NbLatitude);
    return (long)(RRR * acos(0.5 * ((1.0 + q1) * q2 - (1.0 - q1) * q3)) + 1.0);
}

#undef M_PI
#define M_PI 3.14159265358979323846264

long kGUITSP::Distance_GEOM(Node * Na, Node * Nb)
{
    double lati = M_PI * (Na->X / 180.0);
    double latj = M_PI * (Nb->X / 180.0);
    double longi = M_PI * (Na->Y / 180.0);
    double longj = M_PI * (Nb->Y / 180.0);
    double q1 = cos(latj) * sin(longi - longj);
    double q3 = sin((longi - longj) / 2.0);
    double q4 = cos((longi - longj) / 2.0);
    double q2 = sin(lati + latj) * q3 * q3 - sin(lati - latj) * q4 * q4;
    double q5 = cos(lati - latj) * q4 * q4 - cos(lati + latj) * q3 * q3;
    return (long) (6378388.0 * atan2(sqrt(q1 * q1 + q2 * q2), q5) + 1.0);
}

long kGUITSP::Distance_MAN_2D(Node * Na, Node * Nb)
{
    return (long)(fabs(Na->X - Nb->X) + fabs(Na->Y - Nb->Y) + 0.5);
}

long kGUITSP::Distance_MAN_3D(Node * Na, Node * Nb)
{
    return (long)(fabs(Na->X - Nb->X) + fabs(Na->Y - Nb->Y) + fabs(Na->Z - Nb->Z) + 0.5);
}

long kGUITSP::Distance_MAX_2D(Node * Na, Node * Nb)
{
    long dx = (long)(fabs(Na->X - Nb->X) + 0.5);
	long dy = (long)(fabs(Na->Y - Nb->Y) + 0.5);
    return (long)(dx > dy ? dx : dy);
}

long kGUITSP::Distance_MAX_3D(Node * Na, Node * Nb)
{
    long dx = (long)(fabs(Na->X - Nb->X) + 0.5);
	long dy = (long)(fabs(Na->Y - Nb->Y) + 0.5);
	long dz = (long)(fabs(Na->Z - Nb->Z) + 0.5);
    if (dy > dx)
        dx = dy;
    return (long)(dx > dz ? dx : dz);
}

#if 0
/* 
   The eprintf function prints an error message and exits.
*/

void eprintf(char *fmt, ...)
{
    va_list args;

    fflush(stdout);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(2);
}
#endif

/* 
   The Excludable function is used to test if an edge, (ta,tb), 
   of the tour may be excluded (when making a move). An edge is
   excludable if it is on the original tour and has not previously 
   been excluded (and inserted again) in the current series of moves.
   If the edge is excludable, the function returns 1; otherwise 0.

   The function is called from the BestMove function in order to
   test if the last edge to be excluded in a non-gainful r-opt move 
   is excludable.  
*/

int kGUITSP::Excludable(const Node * ta, const Node * tb)
{
    if (ta == tb->OldPred)
        return !tb->OldPredExcluded;
    if (ta == tb->OldSuc)
        return !tb->OldSucExcluded;
    return 0;
}

/* 
   The Exclude function is used to register that an edge, (ta,tb), 
   of the original tour has been excluded in a move. This is done by
   setting the appropriate flag, OldPredExcluded or OldSucExluded, 
   for each of the two end nodes.
*/

void kGUITSP::Exclude(Node * ta, Node * tb)
{
    if (ta == tb->Pred || ta == tb->Suc)
        return;
    if (ta == tb->OldPred)
        tb->OldPredExcluded = 1;
    else if (ta == tb->OldSuc)
        tb->OldSucExcluded = 1;
    if (tb == ta->OldPred)
        ta->OldPredExcluded = 1;
    else if (tb == ta->OldSuc)
        ta->OldSucExcluded = 1;
}

/*
   After the candidate set has been created the FindTour function is called a 
   predetermined number of times (Runs). 

   FindTour performs a number of trials, where in each trial it attempts to 
   improve a chosen initial tour using the modified Lin-Kernighan edge exchange 
   heuristics. 

   Each time a better tour is found, the tour is recorded, and the candidates are 
   reorderded by the AdjustCandidateSet function. Precedence is given to edges that 
   are common to two currently best tours. The candidate set is extended with those
   tour edges that are not present in the current set. The original candidate set
   is re-established at exit from FindTour.  
*/

double kGUITSP::FindTour(void)
{
    double Cost;
    Node *t;
//    double LastTime = GetTime();

    t = FirstNode;
    do
        t->OldPred = t->OldSuc = t->NextBestSuc = t->BestSuc = 0;
    while ((t = t->Suc) != FirstNode);
    HashInitialize(HTable);
    BetterCost = DBL_MAX;
    for (Trial = 1; Trial <= MaxTrials; Trial++) {
        ChooseInitialTour();
        Cost = LinKernighan();
        if (Cost < BetterCost) {
//            if (TraceLevel >= 1) {
//                printf("* %ld: Cost = %0.0f, Time = %0.0f sec.\n",
//                       Trial, Cost, GetTime() - LastTime);
//                fflush(stdout);
//            }
            BetterCost = Cost;
            RecordBetterTour();
            if (BetterCost <= Optimum)
                break;
            AdjustCandidateSet();
            HashInitialize(HTable);
            HashInsert(HTable, Hash, Cost * Precision);
        }
//		else if (TraceLevel >= 2)
//		{
  //          printf("  %ld: Cost = %0.0f, Time = %0.0f sec.\n",
    //               Trial, Cost, GetTime() - LastTime);
      //      fflush(stdout);
     //   }
    }
    if (Trial > MaxTrials)
        Trial = MaxTrials;
    ResetCandidateSet();
    return BetterCost;
}

/*
   The Flip function performs a 2-opt move. Edges (t1,t2) and (t3,t4) 
   are exchanged with edges (t2,t3) and (t4,t1). Node t4 is one of 
   t3's two neighbors on the tour; which one is uniquely determined
   by the orientation of (t1,t2).

   The function is only used if the doubly linked list representation 
   is used for a tour; if the two-level tree representation is used, 
   the function Flip_SL is used instead.

   The 2-opt move is made by swapping Pred and Suc of each node of the
   two segments, and then reconnecting the segments by suitable
   settings of Pred and Suc of t1, t2, t3 and t4. In addition,
   Rank is updated for nodes in the reversed segment (Rank gives the
   ordinal number of a node in the tour).

   Any of two segments defined by the 2-opt move may be reversed. The
   segment with the fewest number of nodes is reversed in order to
   speed up computations. The number of nodes in a segment is found 
   from the Rank-values. 

   The move is pushed onto a stack of 2-opt moves. The stack makes it
   possible to undo moves (by the RestoreTour function).

   Finally, the hash value corresponding to the tour is updated. 
*/

void kGUITSP::Flip(Node * t1, Node * t2, Node * t3)
{
    Node *s1, *s2, *t4;
    long R;

    if (t3 == t2->Pred || t3 == t2->Suc)
        return;
    t4 = t1->Suc == t2 ? t3->Pred : t3->Suc;
    if (t1->Suc != t2) {
        s1 = t1;
        t1 = t2;
        t2 = s1;
        s1 = t3;
        t3 = t4;
        t4 = s1;
    }
    /* Find the segment with the fewest nodes */
    if ((R = t2->Rank - t3->Rank) < 0)
        R += m_dimension;
    if (2 * R >= m_dimension) {
        s1 = t3;
        t3 = t2;
        t2 = s1;
        s1 = t4;
        t4 = t1;
        t1 = s1;
    }
    /* Swap segment (t3 --> t1) */
    R = t1->Rank;
    t1->Suc = 0;
    s2 = t3;
    while ((s1 = s2)) {
        s2 = s1->Suc;
        s1->Suc = s1->Pred;
        s1->Pred = s2;
        s1->Rank = R--;
    }
    t3->Suc = t2;
    t2->Pred = t3;
    t1->Pred = t4;
    t4->Suc = t1;
    SwapStack[Swaps].t1 = t1;
    SwapStack[Swaps].t2 = t2;
    SwapStack[Swaps].t3 = t3;
    SwapStack[Swaps].t4 = t4;
    Swaps++;
    Hash ^= (Rand[t1->Id] * Rand[t2->Id]) ^
        (Rand[t3->Id] * Rand[t4->Id]) ^
        (Rand[t2->Id] * Rand[t3->Id]) ^ (Rand[t4->Id] * Rand[t1->Id]);
}

/*
   The Flip_SL function performs a 2-opt move. Edges (t1,t2) and (t3,t4) 
   are exchanged with edges (t2,t3) and (t4,t1). Node t4 is one of 
   t3's two neighbors on the tour; which one is uniquely determined
   by the orientation of (t1,t2).

   The function is only used if the two-level tree representation is used 
   for a tour; if the doubly linked list representation is used, the function 
   Flip is used instead.

   The worst-case time cost of a 2-op move is O(n) when the doubly linked
   list representation is used. A worst-case cost of O(sqrt(n)) per 2-opt 
   move may be achieved using the two-level tree representation.

   The idea is to divide the tour into roughly sqrt(n) segments. Each segment
   is maintained as a doubly linked list of nodes (using pointers labeled
   Pred and Suc). The segments are connected in a doubly linked list (using
   pointers labeled Pred and Suc). Each segment contains a number, Rank,
   that represents its position in the list, two pointers First and Last that
   references the first and last node of the segment, respectively, and a bit,
   Reversed, that is used to indicate whether the segment should be traversed
   in forward or backward direction. Just switching this bit reverses the 
   orientation of a whole segment. 

   The implementation of Flip_SL closely follows the suggestions given in

   M. L. Fredman, D. S. Johnson & L. A. McGeoch,
   Data Structures for Traveling Salesmen",
   J. Algorithms, 16, 432-479 (1995).

   When a 2-opt move has been made it is pushed onto a stack of 2-opt moves.
   The stack makes it possible to undo moves (by the RestoreTour function).

   Finally, the hash value corresponding to the tour is updated.
*/

//static void SplitSegment(Node * t1, Node * t2);

void kGUITSP::Flip_SL(Node * t1, Node * t2, Node * t3)
{
    Node *t4, *a, *b, *c, *d;
    Segment *P1, *P2, *P3, *P4, *Q1, *Q2;
    Node *s1, *s2;
    long i;

    if (t3 == t2->Pred || t3 == t2->Suc)
        return;
    if (Groups == 1) {
        Flip(t1, t2, t3);
        return;
    }
    t4 = t2 == SUC(t1) ? PRED(t3) : SUC(t3);
    P1 = t1->Parent;
    P2 = t2->Parent;
    P3 = t3->Parent;
    P4 = t4->Parent;
    /* Split segments if needed */
    if (P1 != P3 && P2 != P4) {
        if (P1 == P2) {
            SplitSegment(t1, t2);
            P1 = t1->Parent;
            P2 = t2->Parent;
        }
        if (P3 == P4 && P1 != P3 && P2 != P4) {
            SplitSegment(t3, t4);
            P3 = t3->Parent;
            P4 = t4->Parent;
        }
    } else if ((P1 == P3 && labs(t3->Rank - t1->Rank) > 3 * GroupSize / 4)
               || (P2 == P4
                   && labs(t4->Rank - t2->Rank) > 3 * GroupSize / 4)) {
        if (P1 == P2) {
            SplitSegment(t1, t2);
            P1 = t1->Parent;
            P2 = t2->Parent;
            P3 = t3->Parent;
            P4 = t4->Parent;
        }
        if (P3 == P4) {
            SplitSegment(t3, t4);
            P1 = t1->Parent;
            P2 = t2->Parent;
            P3 = t3->Parent;
            P4 = t4->Parent;
        }
    }
    /* Check if it is possible to flip locally within a segment */
    a = 0;
    if (P1 == P3) {
        /* Either the t1 --> t3 path or the t2 --> t4 path lies 
           within one segment */
        if (t1->Rank < t3->Rank) {
            if (P1 == P2 && P1 == P4 && t2->Rank > t1->Rank) {
                a = t1;
                b = t2;
                c = t3;
                d = t4;
            } else {
                a = t2;
                b = t1;
                c = t4;
                d = t3;
            }
        } else {
            if (P1 == P2 && P1 == P4 && t2->Rank < t1->Rank) {
                a = t3;
                b = t4;
                c = t1;
                d = t2;
            } else {
                a = t4;
                b = t3;
                c = t2;
                d = t1;
            }
        }
    } else if (P2 == P4) {
        /* The t2 --> t4 path lies within one segment */
        if (t4->Rank < t2->Rank) {
            a = t3;
            b = t4;
            c = t1;
            d = t2;
        } else {
            a = t1;
            b = t2;
            c = t3;
            d = t4;
        }
    }
    if (a) {
        /* Flip locally (b --> d) within a segment */
        i = d->Rank;
        d->Suc = 0;
        s2 = b;
        while ((s1 = s2)) {
            s2 = s1->Suc;
            s1->Suc = s1->Pred;
            s1->Pred = s2;
            s1->Rank = i--;
        }
        d->Pred = a;
        b->Suc = c;
        if (a->Suc == b)
            a->Suc = d;
        else
            a->Pred = d;
        if (c->Pred == d)
            c->Pred = b;
        else
            c->Suc = b;
        if (b->Parent->First == b)
            b->Parent->First = d;
        else if (d->Parent->First == d)
            d->Parent->First = b;
        if (b->Parent->Last == b)
            b->Parent->Last = d;
        else if (d->Parent->Last == d)
            d->Parent->Last = b;
    } else {
        /* Reverse a sequence of segments */
        if (P1->Suc != P2) {
            a = t1;
            t1 = t2;
            t2 = a;
            a = t3;
            t3 = t4;
            t4 = a;
            Q1 = P1;
            P1 = P2;
            P2 = Q1;
            Q1 = P3;
            P3 = P4;
            P4 = Q1;
        }
        /* Find the sequence with the fewest segments */
        if ((i = P2->Rank - P3->Rank) <= 0)
            i += Groups;
        if (2 * i > Groups) {
            a = t3;
            t3 = t2;
            t2 = a;
            a = t1;
            t1 = t4;
            t4 = a;
            Q1 = P3;
            P3 = P2;
            P2 = Q1;
            Q1 = P1;
            P1 = P4;
            P4 = Q1;
        }
        /* Reverse the sequence of segments (P3 --> P1). 
           Mirrors the corresponding code in the Flip function */
        i = P1->Rank;
        P1->Suc = 0;
        Q2 = P3;
        while ((Q1 = Q2)) {
            Q2 = Q1->Suc;
            Q1->Suc = Q1->Pred;
            Q1->Pred = Q2;
            Q1->Rank = i--;
            Q1->Reversed ^= 1;
        }
        P3->Suc = P2;
        P2->Pred = P3;
        P1->Pred = P4;
        P4->Suc = P1;
        if (t3->Suc == t4)
            t3->Suc = t2;
        else
            t3->Pred = t2;
        if (t2->Suc == t1)
            t2->Suc = t3;
        else
            t2->Pred = t3;
        if (t1->Pred == t2)
            t1->Pred = t4;
        else
            t1->Suc = t4;
        if (t4->Pred == t3)
            t4->Pred = t1;
        else
            t4->Suc = t1;
    }
    SwapStack[Swaps].t1 = t1;
    SwapStack[Swaps].t2 = t2;
    SwapStack[Swaps].t3 = t3;
    SwapStack[Swaps].t4 = t4;
    Swaps++;
    Hash ^= (Rand[t1->Id] * Rand[t2->Id]) ^
        (Rand[t3->Id] * Rand[t4->Id]) ^
        (Rand[t2->Id] * Rand[t3->Id]) ^ (Rand[t4->Id] * Rand[t1->Id]);
}

/*
   The SplitSegment function is called by the Flip_SL function to split a segment.
   Calling SplitSegment(t1,t2), where t1 and t2 are neighbors in the same segment 
   causes the segment to be split between t1 and t2. The smaller half is merged with
   its neighbouring segment, thus keeping the number of segments fixed.

   The implementation of SplitSegment closely follows the suggestions given in

   M. L. Fredman, D. S. Johnson & L. A. McGeoch,
   Data Structures for Traveling Salesmen",
   J. Algorithms, 16, 432-479 (1995).
 */

void kGUITSP::SplitSegment(Node * t1, Node * t2)
{
    Segment *P = t1->Parent, *Q;
    Node *t, *u;
    long i, Count;

    if (t2->Rank < t1->Rank) {
        t = t1;
        t1 = t2;
        t2 = t;
    }
    Count = t1->Rank - P->First->Rank + 1;
    if (2 * Count < P->Size) {
        /* The left part of P is merged with its neighbouring segment, Q */
        Q = P->Reversed ? P->Suc : P->Pred;
        t = P->Reversed == Q->Reversed ? Q->Last : Q->First;
        i = t->Rank;
        if (t == Q->Last) {
            if (t == Q->First && t->Suc != P->First) {
                u = t->Suc;
                t->Suc = t->Pred;
                t->Pred = u;
                Q->Reversed ^= 1;
            }
            for (t = P->First; t != t2; t = t->Suc) {
                t->Parent = Q;
                t->Rank = ++i;
            }
            Q->Last = t1;
        } else {
            for (t = P->First; t != t2; t = u) {
                t->Parent = Q;
                t->Rank = --i;
                u = t->Suc;
                t->Suc = t->Pred;
                t->Pred = u;
            }
            Q->First = t1;
        }
        P->First = t2;
    } else {
        /* The right part of P is merged with its neighbouring segment, Q */
        Q = P->Reversed ? P->Pred : P->Suc;
        t = P->Reversed == Q->Reversed ? Q->First : Q->Last;
        i = t->Rank;
        if (t == Q->First) {
            if (t == Q->Last && t->Pred != P->Last) {
                u = t->Suc;
                t->Suc = t->Pred;
                t->Pred = u;
                Q->Reversed ^= 1;
            }
            for (t = P->Last; t != t1; t = t->Pred) {
                t->Parent = Q;
                t->Rank = --i;
            }
            Q->First = t2;
        } else {
            for (t = P->Last; t != t1; t = u) {
                t->Parent = Q;
                t->Rank = ++i;
                u = t->Pred;
                t->Pred = t->Suc;
                t->Suc = u;
            }
            Q->Last = t2;
        }
        Count = P->Size - Count;
        P->Last = t1;
    }
    P->Size -= Count;
    Q->Size += Count;
}

/* 
   The Forbidden function is used to test if an edge, (ta,tb), 
   is one of the forbidden edges (C(ta, tb) == M) in a solution of
   an asymmetric traveling saleman problem. 
   If the edge is forbidden, the function returns 1; otherwise 0.
*/

int kGUITSP::Forbidden(const Node * ta, const Node * tb)
{
    return m_problemtype == ATSP &&
        (ta->Id <= m_dimension / 2) == (tb->Id <= m_dimension / 2);
}

/*      
   The FreeStructures function frees all allocated structures.
*/

void kGUITSP::FreeStructures(void)
{
    if (FirstNode) {
        Node *N = FirstNode, *Next;
        do {
            Next = N->Suc;
            free(N->CandidateSet);
        } while ((N = Next) != FirstNode);
        FirstNode = 0;
    }
    if (FirstSegment) {
        Segment *S = FirstSegment, *SPrev;
        do {
            SPrev = S->Pred;
            free(S);
        } while ((S = SPrev) != FirstSegment);
        FirstSegment = 0;

    }
    free(NodeSet);
    NodeSet = 0;
    free(CostMatrix);
    CostMatrix = 0;
    free(m_besttour);
    m_besttour = 0;
    free(BetterTour);
    BetterTour = 0;
    free(SwapStack);
    SwapStack = 0;
    free(HTable);
    HTable = 0;
    free(Rand);
    Rand = 0;
    free(CacheSig);
    CacheSig = 0;
    free(CacheVal);
    CacheVal = 0;
    free(Heap);
    Heap = 0;
//    free(Type);
//    Type = 0;
//    free(EdgeWeightType);
//    EdgeWeightType = 0;
//    free(EdgeWeightFormat);
//    EdgeWeightFormat = 0;
//    free(EdgeDataFormat);
  //  EdgeDataFormat = 0;
 //   free(NodeCoordType);
 //   NodeCoordType = 0;
 //   free(DisplayDataType);
 //   DisplayDataType = 0;
}

/*
   The Gain23 function attempts to improve a tour by making nonsequential
   moves.

   The set of nonsequential moves considered consists of:

   (1) any nonfeasible 2-opt move (producing two cycles) followed by any
   2- or 3-opt move which produces a feasible tour (by joining the
   two cycles);

   (2) any nonfeasible 3-opt move (producing two cycles) followed by any
   2-opt move which produces a feasible tour (by joining the two 
   cycles).

   The first and second move may in some cases be 4-opt in (2) and (1),
   respectively. In (1) this can happen when a possible 3-opt move may be
   extended to a nonfeasible 4-opt move. In (2) it can happen in those cases 
   where a sequential 4-opt extends a possible 3-opt move and produces two 
   cycles.

   The first move must have a positive gain. The second move is determined by 
   the BridgeGain function.
*/

long kGUITSP::Gain23(void)
{
    Candidate *Ns2, *Ns4, *Ns6;
    Node *s1=0, *s2=0, *s3=0, *s4=0, *s5=0, *s6=0, *s7=0, *s8=0;
    long G0, G1, G2, G3, G4, G5, G6=0, Gain=0, Gain6=0, i=0;
    int X2, X4, X6, X8, Case6=0, Case8=0;

/*  
   The algorithm splits the set of possible moves up into a number disjunct subsets
   (called "cases"). When s1, s2, ..., s6 has been chosen, Case6 is used to 
   discriminate between 7 cases. When s1, s2, ..., s8 has been chosen, Case8 is used 
   to discriminate between 11 cases. 

   A detailed description of the different cases can be found after the code.
*/

    Reversed = 0;
    i = 0;
    s1 = FirstNode;
    do
        s1->V = ++i;
    while ((s1 = SUC(s1)) != FirstNode);

    /* Try any nonfeasible 2-opt move folllowed by a 2-, 3- or 4-opt move */
    for (X2 = 1; X2 <= 2; X2++) {
        Reversed ^= 1;
        do {
            s2 = SUC(s1);
            if (Fixed(s1, s2))
                continue;
            G0 = (this->*C)(s1, s2);
            /* Choose (s2,s3) as a candidate edge emanating from s2 */
            for (Ns2 = s2->CandidateSet; (s3 = Ns2->To); Ns2++) {
                if (s3 == s1 || (s4 = SUC(s3)) == s1 || Fixed(s3, s4))
                    continue;
                if ((i = !Reversed ? s3->V - s2->V : s2->V - s3->V) <= 0)
                    i += m_dimension;
                if (2 * i > m_dimension)
                    continue;
                G1 = G0 - Ns2->Cost;
                G2 = G1 + (this->*C)(s3, s4);
                if (!Forbidden(s4, s1) &&
                    (!c || G2 - (this->*c)(s4, s1) > 0) &&
                    (G3 = G2 - (this->*C)(s4, s1)) > 0 &&
                    (Gain =
                     BridgeGain(s1, s2, s3, s4, 0, 0, 0, 0, 0, G3)) > 0)
                    return Gain;
            }
        }
        while ((s1 = s2) != FirstNode);
    }
    /* Try any nonfeasible 2-, 3- or 4-opt move folllowed by a 2-opt move */
    for (X2 = 1; X2 <= 2; X2++) {
        Reversed ^= 1;
        do {
            s2 = SUC(s1);
            if (Fixed(s1, s2))
                continue;
            G0 = (this->*C)(s1, s2);
            /* Choose (s2,s3) as a candidate edge emanating from s2 */
            for (Ns2 = s2->CandidateSet; (s3 = Ns2->To); Ns2++) {
                if (s3 == s2->Pred || s3 == s2->Suc ||
                    (G1 = G0 - Ns2->Cost) <= 0)
                    continue;
                /* Choose s4 as one of s3's two neighbors on the tour */
                for (X4 = 1; X4 <= 2; X4++) {
                    s4 = X4 == 1 ? PRED(s3) : SUC(s3);
                    if (Fixed(s3, s4))
                        continue;
                    G2 = G1 + (this->*C)(s3, s4);
                    if (X4 == 1 &&
                        !Forbidden(s4, s1) &&
                        (!c || G2 - (this->*c)(s4, s1) > 0) &&
                        (Gain = G2 - (this->*C)(s4, s1)) > 0) {
                        Swap1(s1, s2, s3);
                        return Gain;
                    }
                    /* Choose (s4,s5) as a candidate edge emanating from s4 */
                    for (Ns4 = s4->CandidateSet; (s5 = Ns4->To); Ns4++) {
                        if (s5 == s4->Pred || s5 == s4->Suc ||
                            (G3 = G2 - Ns4->Cost) <= 0)
                            continue;
                        /* Choose s6 as one of s5's two neighbors on the tour */
                        for (X6 = 1; X6 <= 2; X6++) {
                            if (X4 == 1) {
                                if (X6 == 1) {
                                    Case6 = 1 + !BETWEEN(s2, s5, s4);
                                    s6 = Case6 == 1 ? SUC(s5) : PRED(s5);
                                } else {
                                    s6 = s6 ==
                                        s5->Pred ? s5->Suc : s5->Pred;
                                    if (s5 == s1 || s6 == s1)
                                        continue;
                                    Case6 += 2;
                                }
                            } else if (BETWEEN(s2, s5, s3)) {
                                Case6 = 4 + X6;
                                s6 = X6 == 1 ? SUC(s5) : PRED(s5);
                                if (s6 == s1)
                                    continue;
                            } else {
                                if (X6 == 2)
                                    break;
                                Case6 = 7;
                                s6 = PRED(s5);
                            }
                            if (Fixed(s5, s6))
                                continue;
                            G4 = G3 + (this->*C)(s5, s6);
                            Gain6 = 0;
                            if (!Forbidden(s6, s1) &&
                                (!c || G4 - (this->*c)(s6, s1) > 0) &&
                                (Gain6 = G4 - (this->*C)(s6, s1)) > 0) {
                                if (Case6 <= 2 || Case6 == 5 || Case6 == 6) {
                                    Make3OptMove(s1, s2, s3, s4, s5, s6,
                                                 Case6);
                                    return Gain6;
                                }
                                if ((Gain =
                                     BridgeGain(s1, s2, s3, s4, s5, s6, 0,
                                                0, Case6, Gain6)) > 0)
                                    return Gain;
                            }
                            /* Choose (s6,s7) as a candidate edge emanating from s6 */
                            for (Ns6 = s6->CandidateSet; (s7 = Ns6->To);
                                 Ns6++) {
                                if (s7 == s6->Pred || s7 == s6->Suc
                                    || (s6 == s2 && s7 == s3) || (s6 == s3
                                                                  && s7 ==
                                                                  s2)
                                    || (G5 = G4 - Ns6->Cost) <= 0)
                                    continue;
                                /* Choose s6 as one of s5's two neighbors on the tour */
                                for (X8 = 1; X8 <= 2; X8++) {
                                    if (X8 == 1) {
                                        Case8 = Case6;
                                        switch (Case6) {
                                        case 1:
                                            s8 = BETWEEN(s2, s7,
                                                         s5) ? SUC(s7) :
                                                PRED(s7);
                                            break;
                                        case 2:
                                            s8 = BETWEEN(s3, s7,
                                                         s6) ? SUC(s7) :
                                                PRED(s7);
                                            break;
                                        case 3:
                                            if (BETWEEN(s5, s7, s4))
                                                s8 = SUC(s7);
                                            else {
                                                s8 = BETWEEN(s3, s7,
                                                             s1) ? PRED(s7)
                                                    : SUC(s7);
                                                Case8 = 17;
                                            }
                                            break;
                                        case 4:
                                            if (BETWEEN(s2, s7, s5))
                                                s8 = BETWEEN(s2, s7,
                                                             s4) ? SUC(s7)
                                                    : PRED(s7);
                                            else {
                                                s8 = PRED(s7);
                                                Case8 = 18;
                                            }
                                            break;
                                        case 5:
                                            s8 = PRED(s7);
                                            break;
                                        case 6:
                                            s8 = BETWEEN(s2, s7,
                                                         s3) ? SUC(s7) :
                                                PRED(s7);
                                            break;
                                        case 7:
                                            if (BETWEEN(s2, s7, s3))
                                                s8 = SUC(s7);
                                            else {
                                                s8 = BETWEEN(s5, s7,
                                                             s1) ? PRED(s7)
                                                    : SUC(s7);
                                                Case8 = 19;
                                            }
                                        }
                                    } else {
                                        if (Case8 >= 17 ||
                                            (Case6 != 3 && Case6 != 4
                                             && Case6 != 7))
                                            break;
                                        s8 = s7->Pred ? s7->Suc : s7->Pred;
                                        Case8 += 8;
                                    }
                                    if (s8 == s1 ||
                                        (s7 == s1 && s8 == s2) ||
                                        (s7 == s3 && s8 == s4) ||
                                        (s7 == s4 && s8 == s3))
                                        continue;
                                    if (Fixed(s7, s8) || Forbidden(s8, s1))
                                        continue;
                                    G6 = G5 + (this->*C)(s7, s8);
                                    if ((!c || G6 - (this->*c)(s8, s1) > 0) &&
                                        (Gain = G6 - (this->*C)(s8, s1)) > 0) {
                                        if (Case8 <= 15) {
                                            Make4OptMove(s1, s2, s3, s4,
                                                         s5, s6, s7, s8,
                                                         Case8);
                                            return Gain;
                                        }
                                        if (Gain > Gain6 &&
                                            (Gain =
                                             BridgeGain(s1, s2, s3, s4, s5,
                                                        s6, s7, s8, Case6,
                                                        Gain)) > 0)
                                            return Gain;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        while ((s1 = s2) != FirstNode);
    }
    return 0;
}

/*

   Below is shown the use of the variables X4, Case6 and Case8 to discriminate between 
   cases considered by the algorithm. 

   The notation

   ab-

   is used for a subtour that starts with the edge (sa,sb). For example the tour 

   12-43-

   contains the edges (s1,s2) and (s4,s3), in that order. A (*) signifies an infeasible
   solution (BridgeGain is called if the accumulated gain is possitive).

   X4 = 1:
       12-43-
       Case6 = 1: 
           12-56-43-
           Case8 = 1: 
               12-78-56-43-, 12-56-87-43-, 12-56-43-87-
       Case6 = 2:   
           12-43-65-
           Case8 = 2: 
              12-87-43-65-, 12-43-78-65-, 12-43-65-87-
       Case6 = 3: 
           12-65-43- (*)
           Case8 = 3:
               12-65-78-43-
           Case8 = 11:
               12-65-87-43- 
       Case6 = 17:
           12-78-65-43- (*), 12-65-43-87- (*) 
       Case6 = 4: 
           12-43-56- (*)
           Case8 = 4: 
               12-43-56-87-
           Case8 = 12:
               12-43-78-56-
       Case6 = 18:
           12-87-43-56- (*), 12-43-87-56 (*)
   X4 = 2:
       12-34-
       Case6 = 5: 
           12-56-34- (*)
           Case8 = 5: 
               12-87-56-34-, 12-56-87-34-, 12-56-34-87-
           Case8 = 13:
               12-78-56-34-, 12-56-78-34-, 12-56-34-78-
       Case6 = 6: 
           12-65-34- (*)
           Case8 = 6: 
               12-87-65-34-, 12-65-78-34-, 12-65-34-87-
           Case8 = 14:
               12-87-65-34-, 12-65-87-34-, 12-65-34-78- 
       Case6 = 7: 
           12-34-65- (*)
           Case8 = 7: 
               12-78-34-65- 
           Case8 = 15:
               12-87-34-65- 
           Case6 = 19:
               12-34-87-65- (*), 12-34-78-65 (*), 

 */

/*
   The GenerateCandidates function associates to each node a set of incident 
   candidate edges. The candidate edges of each node are ordered according to
   their alpha-values.

   The parameter MaxCandidates specifies the maximum number of candidate edges 
   allowed for each node, and MaxAlpha puts an upper limit on their alpha-values.

   A non-zero value of Symmetric specifies that the candidate set is to be
   complemented such that every candidate edge is associated with both its two
   end nodes (in this way MaxCandidates may be exceeded). 

   The candidate edges of each node is kept in an array (CandidatSet) of
   structures. Each structure (Candidate) holds the following information:

   Node *To     : points to the other end node of the edge
   long Alpha   : contains the alpha-value of the edge
   long Cost    : the cost (length) of the edge

   The algorithm for computing alpha-values in time O(n^2) and space O(n) follows 
   the description in

   Keld Helsgaun,
   An Effective Implementation of the Lin-Kernighan Traveling Salesman Heuristic,
   Report, RUC, 1998. 
*/

#define Mark Next
#define Beta NextCost

static long Max(long a, long b)
{
    return a > b ? a : b;
}

void kGUITSP::GenerateCandidates(const long MaxCandidates, const long MaxAlpha,  const int Symmetric)
{
    Node *From, *To;
    Candidate *NFrom, *NN, *NTo;
    long a, d, Count;

    /* Initialize CandidateSet for each node */
    From = FirstNode;
    do {
		if(From->CandidateSet)
	        free(From->CandidateSet);
        From->CandidateSet = 0;
        From->Mark = 0;
    }while ((From = From->Suc) != FirstNode);

    do {
        assert(From->CandidateSet = (Candidate *) malloc((MaxCandidates + 1) * sizeof(Candidate)));
        From->CandidateSet[0].To = 0;        
    }while ((From = From->Suc) != FirstNode);

    if (MaxCandidates <= 0)
        return;

    /* Loop for each node, From */
    do {
        NFrom = From->CandidateSet;
        if (From != FirstNode) {
            From->Beta = LONG_MIN;
            for (To = From; To->Dad != 0; To = To->Dad) {
                To->Dad->Beta =
                    !Fixed(To, To->Dad) ? Max(To->Beta, To->Cost)
                    : To->Beta;
                To->Dad->Mark = From;
            }
        }
        Count = 0;
        /* Loop for each node, To */
        To = FirstNode;
        do {
            if (To == From)
                continue;
            d = c && !Fixed(From, To) ? (this->*c)(From, To) : (this->*D)(From, To);
            if (From == FirstNode)
                a = To == From->Dad ? 0 : d - From->NextCost;
            else if (To == FirstNode)
                a = From == To->Dad ? 0 : d - To->NextCost;
            else {
                if (To->Mark != From)
                    To->Beta =
                        !Fixed(To, To->Dad) ? Max(To->Dad->Beta, To->Cost)
                        : To->Dad->Beta;
                a = d - To->Beta;
            }
            if (Fixed(From, To))
                a = LONG_MIN;
            else {
                if (To->Beta == LONG_MIN || From->FixedTo2 || To->FixedTo2
                    || Forbidden(From, To))
                    continue;
                if (InOptimumTour(From, To)) {
                    a = 0;
                    if (c)
                        d = (this->*D)(From, To);
                } else if (c) {
                    if (a > MaxAlpha ||
                        (Count == MaxCandidates &&
                         (a > (NFrom - 1)->Alpha ||
                          (a == (NFrom - 1)->Alpha
                           && d >= (NFrom - 1)->Cost))))
                        continue;
                    if (To == From->Dad) {
                        d = From->Cost;
                        a = 0;
                    } else if (From == To->Dad) {
                        d = To->Cost;
                        a = 0;
                    } else {
                        a -= d;
                        a += (d = (this->*D)(From, To));
                    }
                }
            }
            if (a <= MaxAlpha) {
                /* Insert new candidate edge in From->CandidateSet */
                NN = NFrom;
                while (--NN >= From->CandidateSet) {
                    if (a > NN->Alpha || (a == NN->Alpha && d >= NN->Cost))
                        break;
                    *(NN + 1) = *NN;
                }
                NN++;
                NN->To = To;
                NN->Cost = d;
                NN->Alpha = a;
                if (Count < MaxCandidates) {
                    Count++;
                    NFrom++;
                }
                NFrom->To = 0;
            }
        }
        while ((To = To->Suc) != FirstNode);
    }
    while ((From = From->Suc) != FirstNode);

    if (!Symmetric)
        return;

    /* Complement the candidate set such that every candidate edge is 
       associated with both its two end nodes */
    To = FirstNode;
    do {
        for (NTo = To->CandidateSet; (From = NTo->To); NTo++) {
            Count = 0;
            for (NN = NFrom = From->CandidateSet; NN->To && NN->To != To;
                 NN++)
                Count++;
            if (!NN->To) {
                a = NTo->Alpha;
                d = NTo->Cost;
                while (--NN >= NFrom) {
                    if (a > NN->Alpha || (a == NN->Alpha && d >= NN->Cost))
                        break;
                    *(NN + 1) = *NN;
                }
                NN++;
                NN->To = To;
                NN->Cost = d;
                NN->Alpha = a;
                assert(From->CandidateSet =
                       (Candidate *) realloc(From->CandidateSet,
                                             (Count +
                                              2) * sizeof(Candidate)));
                From->CandidateSet[Count + 1].To = 0;
            }
        }
    }
    while ((To = To->Suc) != FirstNode);
}

/*
   The functions HashInitialize, HashInsert and HashSearch is used
   to maintain a hash table of tours. 

   A hash function maps tours to locations in a hash table. Each time 
   a tour improvement has been found, the hash table is consulted to 
   see whether the new tour happens to be local optimum found earlier. 
   If this is the case, fruitless checkout time is avoided. 
*/

/*
   HashInitialize(T) empties the hash table T.  
   Empty entries have Cost equal to -DBL_MAX. 
*/

void HashInitialize(HashTable * T)
{
    long i;

    for (i = 0; i < HashTableSize; i++) {
        T->Entry[i].Hash = ULONG_MAX;
        T->Entry[i].Cost = -DBL_MAX;
    }
    T->Count = 0;
}

/*
   HashInsert(T,H,Cost) inserts H and Cost (the cost of the tour) in 
   the table T in a location given by the hash value H. 

   Collisions are handled by double hashing.

   The table size is fixed. If the load factor becomes greater than 
   a specified maximum, MaxLoadFactor, no more insertions will be
   made. However, if the table entry given by H has a cost greater 
   than or equal Cost, then Cost of this entry replaces its pervious 
   value.      
*/

void HashInsert(HashTable * T, unsigned long Hash, double Cost)
{
    long i, p;

    i = Hash % HashTableSize;
    if (T->Count >= MaxLoadFactor * HashTableSize) {
        if (Cost > T->Entry[i].Cost)
            return;
    } else {
        p = 8 - Hash % 8;
        while (T->Entry[i].Cost != -DBL_MAX)
            if ((i -= p) < 0)
                i += HashTableSize;
        T->Count++;
    }
    T->Entry[i].Hash = Hash;
    T->Entry[i].Cost = Cost;
}

/*
   HashSearch(T,H,Cost) returns 1 if table T has an entry containing 
   Cost and H. Otherwise, the function returns 0.
 */

int HashSearch(HashTable * T, unsigned long Hash, double Cost)
{
    long i, p;

    i = Hash % HashTableSize;
    p = 8 - Hash % 8;
    while ((T->Entry[i].Hash != Hash ||
            T->Entry[i].Cost != Cost) && T->Entry[i].Cost != -DBL_MAX)
        if ((i -= p) < 0)
            i += HashTableSize;
    return T->Entry[i].Hash == Hash;
}

/*
   A binary heap is used to implement a priority queue. 

   A heap is useful in order to speed up the computations of minimum 
   spanning trees. The elements of the heap are the nodes, and the
   priorities (ranks) are their associated costs (their minimum distance 
   to the current tree). 
*/


/*      
   MakeHeap creates an empty heap. 
*/

void kGUITSP::MakeHeap(const long Size)
{
    assert(Heap = (Node **) malloc((Size + 1) * sizeof(Node *)));
    HeapCount = 0;
}

/*
   The SiftUp function is called when the rank of a node is decreased.
   The function moves the node forward in the heap (the foremost node
   of the heap has the lowest rank).

   When calling SiftUp(N), node N must belong to the heap.              
*/

void kGUITSP::SiftUp(Node * N)
{
    long Loc = N->Loc, P = Loc / 2;

    while (P && N->Rank < Heap[P]->Rank) {
        Heap[Loc] = Heap[P];
        Heap[Loc]->Loc = Loc;
        Loc = P;
        P /= 2;
    }
    Heap[Loc] = N;
    Heap[Loc]->Loc = Loc;
}

/*       
   The DeleteMin function deletes the foremost node from the heap. 
   The function returns a pointer to the deleted node (0, if the heap
   is empty).
*/

Node *kGUITSP::DeleteMin(void)
{
    Node *Remove, *Item;
    long Ch, Loc;

    if (!HeapCount)
        return 0;
    Remove = Heap[1];
    Item = Heap[HeapCount];
    HeapCount--;
    Loc = 1;
    Ch = 2 * Loc;

    while (Ch <= HeapCount) {
        if (Ch < HeapCount && Heap[Ch + 1]->Rank < Heap[Ch]->Rank)
            Ch++;
        if (Heap[Ch]->Rank >= Item->Rank)
            break;
        Heap[Loc] = Heap[Ch];
        Heap[Loc]->Loc = Loc;
        Loc = Ch;
        Ch *= 2;
    }
    Heap[Loc] = Item;
    Item->Loc = Loc;
    Remove->Loc = 0;
    return Remove;
}

/*       
   The Insert function insert a node into the heap.
   When calling Insert(N), node N must belong to the heap.
*/

void kGUITSP::Insert(Node *N)
{
    long Ch, P;

    Ch = ++HeapCount;
    P = Ch / 2;
    while (P && N->Rank < Heap[P]->Rank) {
        Heap[Ch] = Heap[P];
        Heap[Ch]->Loc = Ch;
        Ch = P;
        P /= 2;
    }
    Heap[Ch] = N;
    N->Loc = Ch;
}

/*
   The LinKenighan function seeks to improve a tour by sequential and nonsequential
   edge exchanges.

   The function returns the cost of the resulting tour. 
*/

double kGUITSP::LinKernighan(void)
{
    Node *t1, *t2, *SUCt1;
    long Gain, G0, i;
    double Cost;
    Candidate *Nt1;
    Segment *S;
    int X2;
//    double LastTime = GetTime();

    Reversed = 0;
    S = FirstSegment;
    i = 0;
    do {
        S->Size = 0;
        S->Rank = ++i;
        S->Reversed = 0;
        S->First = S->Last = 0;
    }
    while ((S = S->Suc) != FirstSegment);
    i = 0;
    Hash = 0;
    Swaps = 0;
    FirstActive = LastActive = 0;

    /* Compute the cost of the initial tour, Cost.
       Compute the corresponding hash value, Hash.
       Initialize the segment list.
       Make all nodes "active" (so that they can be used as t1). */
    Cost = 0;
    t1 = FirstNode;
    do {
        t2 = t1->OldSuc = t1->Next = t1->Suc;
        t1->OldPred = t1->Pred;
        t1->Rank = ++i;
        Cost += (this->*C)(t1, t2) - t1->Pi - t2->Pi;
        Hash ^= Rand[t1->Id] * Rand[t2->Id];
        t1->Cost = LONG_MAX;
        for (Nt1 = t1->CandidateSet; (t2 = Nt1->To); Nt1++)
            if (t2 != t1->Pred && t2 != t1->Suc && Nt1->Cost < t1->Cost)
                t1->Cost = Nt1->Cost;
        t1->Parent = S;
        S->Size++;
        if (S->Size == 1)
            S->First = t1;
        S->Last = t1;
        if (S->Size == GroupSize)
            S = S->Suc;
        t1->OldPredExcluded = t1->OldSucExcluded = 0;
        t1->Next = 0;
        Activate(t1);
    }
    while ((t1 = t1->Suc) != FirstNode);
    if (HashSearch(HTable, Hash, Cost))
        return Cost / Precision;
    /* Loop as long as improvements are found */
    do {
        /* Choose t1 as the first "active" node */
        while ((t1 = RemoveFirstActive())) {
            /* t1 is now "passive" */
            SUCt1 = SUC(t1);
            /* Choose t2 as one of t1's two neighbor nodes on the tour */
            for (X2 = 1; X2 <= 2; X2++) {
                t2 = X2 == 1 ? PRED(t1) : SUCt1;
                if ((RestrictedSearch && Near(t1, t2)) || Fixed(t1, t2))
                    continue;
                G0 = (this->*C)(t1, t2);
                /* Make sequential moves */
                while ((t2 = (BacktrackMove ?
					(this->*(BacktrackMove))(t1, t2, &G0, &Gain) :
                       (this->*(BestMove))(t1, t2, &G0, &Gain))))
					   {
                    if (Gain > 0) {
                        /* An improvement has been found */
                        Cost -= Gain;
#if 0
						if (TraceLevel >= 3 ||
                            (TraceLevel == 2
                             && Cost / Precision < BetterCost)) {
                            printf("Cost = %0.0f, Time = %0.0f sec.\n",
                                   Cost / Precision, GetTime() - LastTime);
                            fflush(stdout);
                        }
#endif
							 StoreTour();
                        if (HashSearch(HTable, Hash, Cost))
                            goto End_LinKernighan;
                        /* Make t1 "active" again */
                        Activate(t1);
                        goto Next_t1;
                    }
                }
                RestoreTour();
            }
          Next_t1:;
        }
        if (!HashSearch(HTable, Hash, Cost))
            HashInsert(HTable, Hash, Cost);
        /* Attempt to find improvements using nonsequential moves (with Gain23) */
        if ((Gain = Gain23()) > 0) {
            /* An improvement has been found */
            Cost -= Gain;
//            if (TraceLevel >= 3 ||
//                (TraceLevel == 2 && Cost / Precision < BetterCost)) {
//                printf("Cost = %0.0f, Time = %0.0f sec.\n",
//                       Cost / Precision, GetTime() - LastTime);
//                fflush(stdout);
//            }
            StoreTour();
            if (HashSearch(HTable, Hash, Cost))
                goto End_LinKernighan;
        }
    }
    while (Gain > 0);

  End_LinKernighan:
    NormalizeNodeList();
    return Cost / Precision;
}

/*
    The Make2OptMove function makes a 2-opt move by calling the macro Swap1 
    (i.e., by calling either Flip of Flip_SL). Edges (t1,t2) and (t3,t4) 
    are exchanged with edges (t2,t3) and (t4,t1). Node t4 is one of t3's two
    neighbors on the tour; which one is uniquely determined by the orientation  
    of (t1,t2).
*/

void kGUITSP::Make2OptMove(Node * t1, Node * t2, Node * t3, Node * t4)
{
    Swap1(t1, t2, t3);
}

/*
    The Make3OptMove function makes a 3-opt move by calling the macro 
    Swap2 or Swap3.
*/

void kGUITSP::Make3OptMove(Node * t1, Node * t2, Node * t3, Node * t4,
                  Node * t5, Node * t6, int Case)
{
    switch (Case) {
    case 1:
    case 2:
        Swap2(t1, t2, t3, t6, t5, t4);
        return;
    case 5:
        Swap3(t1, t2, t4, t6, t5, t4, t6, t2, t3);
        return;
    case 6:
        Swap2(t3, t4, t5, t1, t2, t3);
        return;
    }
}

/*
    The Make4OptMove function makes a 4-opt move by calling the macro Swap3.
*/

void kGUITSP::Make4OptMove(Node * t1, Node * t2, Node * t3, Node * t4,
                  Node * t5, Node * t6, Node * t7, Node * t8, int Case)
{
    if (SUC(t1) != t2)
        Reversed ^= 1;
    switch (Case) {
    case 1:
    case 2:
        Swap3(t1, t2, t3, t6, t5, t4, t7, t8, t1);
        return;
    case 3:
    case 4:
        Swap3(t1, t2, t3, t8, t7, t6, t5, t8, t1);
        return;
    case 5:
        if (!BETWEEN(t2, t7, t3))
            Swap3(t5, t6, t7, t2, t1, t4, t1, t4, t5);
        else if (BETWEEN(t2, t7, t6))
            Swap3(t5, t6, t7, t5, t8, t3, t3, t8, t1);
        else
            Swap3(t1, t2, t7, t7, t2, t3, t4, t7, t6);
        return;
    case 6:
        Swap3(t3, t4, t5, t6, t3, t2, t1, t6, t7);
        return;
    case 7:
        Swap3(t6, t5, t8, t2, t1, t4, t8, t5, t4);
        return;
    case 11:
        Swap3(t1, t2, t7, t3, t4, t5, t3, t6, t7);
        return;
    case 12:
        Swap3(t3, t4, t5, t7, t8, t1, t3, t6, t7);
        return;
    case 15:
        Swap3(t3, t4, t5, t3, t6, t7, t8, t3, t2);
        return;
    }
}

/*
    The Make5OptMove function makes a 5-opt move by calling the macro 
    Swap4 or Swap5.
*/

void kGUITSP::Make5OptMove(Node * t1, Node * t2, Node * t3, Node * t4,
                  Node * t5, Node * t6, Node * t7, Node * t8,
                  Node * t9, Node * t10, int Case)
{
    if (SUC(t1) != t2)
        Reversed ^= 1;
    switch (Case) {
    case 1:
        Swap4(t1, t2, t3, t8, t7, t6, t10, t9, t8, t10, t5, t4);
        return;
    case 2:
        if (BETWEEN(t2, t9, t4))
            Swap4(t1, t2, t3, t5, t6, t7, t10, t9, t8, t5, t10, t1);
        else
            Swap4(t1, t2, t3, t7, t8, t9, t6, t5, t4, t7, t10, t1);
        return;
    case 3:
        Swap4(t3, t4, t5, t7, t8, t9, t1, t2, t3, t7, t10, t1);
        return;
    case 4:
        Swap5(t5, t6, t8, t1, t2, t3, t10, t9, t8, t1, t4, t5, t6, t10,
              t1);
        return;
    case 5:
        Swap5(t5, t6, t10, t1, t2, t3, t6, t10, t1, t8, t7, t6, t8, t4,
              t5);
        return;
    case 6:
        Swap4(t1, t2, t3, t9, t10, t1, t7, t8, t9, t6, t5, t4);
        return;
    case 7:
        if (BETWEEN(t3, t9, t7))
            Swap4(t3, t4, t5, t8, t7, t6, t10, t9, t8, t1, t2, t3);
        else if (BETWEEN(t6, t9, t4))
            Swap4(t3, t4, t5, t8, t7, t6, t9, t10, t1, t9, t2, t3);
        else
            Swap4(t1, t2, t3, t6, t5, t4, t7, t8, t9, t7, t10, t1);
        return;
    case 8:
        Swap4(t3, t4, t5, t9, t10, t1, t8, t7, t6, t8, t3, t2);
        return;
    case 9:
        Swap4(t10, t9, t8, t5, t6, t7, t1, t2, t3, t1, t4, t5);
        return;
    case 10:
        if (BETWEEN(t5, t9, t7))
            Swap4(t5, t6, t7, t9, t10, t1, t4, t3, t2, t4, t9, t8);
        else if (BETWEEN(t3, t9, t6))
            Swap4(t1, t2, t3, t6, t5, t4, t7, t8, t9, t7, t10, t1);
        else
            Swap4(t1, t2, t3, t9, t10, t1, t5, t6, t7, t5, t8, t9);
        return;
    case 11:
        if (BETWEEN(t3, t9, t6))
            Swap4(t1, t2, t3, t6, t5, t4, t9, t10, t1, t7, t8, t9);
        else
            Swap4(t5, t6, t7, t10, t9, t8, t2, t1, t10, t4, t3, t2);
        return;
    case 12:
        Swap4(t1, t2, t3, t8, t7, t6, t10, t9, t8, t5, t10, t1);
        return;
    case 13:
        if (BETWEEN(t4, t9, t7))
            Swap5(t7, t8, t10, t5, t6, t7, t1, t2, t3, t5, t9, t1, t9, t1,
                  t10);
        else if (BETWEEN(t6, t9, t3))
            Swap5(t10, t9, t1, t7, t8, t9, t3, t4, t5, t3, t6, t7, t3, t1,
                  t10);
        else
            Swap5(t10, t9, t1, t4, t3, t2, t5, t6, t7, t5, t8, t10, t9, t1,
                  t10);
        return;
    case 14:
        Swap5(t10, t9, t1, t5, t6, t7, t5, t8, t9, t3, t4, t5, t3, t1,
              t10);
        return;
    case 15:
        if (BETWEEN(t6, t9, t3))
            Swap5(t10, t9, t1, t3, t4, t5, t6, t3, t2, t8, t7, t6, t9, t1,
                  t10);
        else
            Swap5(t1, t2, t6, t3, t4, t5, t8, t7, t6, t10, t9, t8, t2, t10,
                  t1);
        return;
    case 16:
        if (BETWEEN(t4, t9, t7))
            Swap4(t3, t4, t5, t8, t7, t6, t9, t10, t1, t8, t3, t2);
        else if (BETWEEN(t5, t9, t3))
            Swap4(t3, t4, t5, t9, t10, t1, t6, t3, t2, t7, t8, t9);
        else
            Swap4(t3, t4, t5, t1, t2, t3, t7, t8, t9, t7, t10, t1);
        return;
    case 17:
        if (BETWEEN(t7, t9, t3))
            Swap4(t3, t4, t5, t7, t8, t9, t2, t1, t10, t3, t6, t7);
        else
            Swap4(t7, t8, t9, t2, t1, t10, t3, t4, t5, t3, t6, t7);
        return;
    case 18:
        Swap4(t3, t4, t5, t7, t8, t9, t3, t6, t7, t1, t2, t3);
        return;
    case 19:
        Swap4(t7, t8, t9, t1, t2, t3, t6, t5, t4, t7, t10, t1);
        return;
    case 20:
        Swap4(t7, t8, t9, t3, t4, t5, t10, t7, t6, t3, t10, t1);
        return;
    case 21:
        Swap4(t5, t6, t7, t5, t8, t9, t1, t2, t3, t4, t1, t10);
        return;
    case 22:
        Swap4(t1, t2, t3, t6, t5, t4, t7, t8, t1, t9, t10, t1);
        return;
    case 23:
        Swap4(t1, t2, t3, t6, t5, t4, t7, t8, t1, t9, t10, t1);
        return;
    case 24:
        Swap4(t1, t2, t3, t8, t7, t6, t5, t8, t1, t9, t10, t1);
        return;
    case 25:
        Swap4(t1, t2, t3, t8, t7, t6, t5, t8, t1, t9, t10, t1);
        return;
    case 26:
        if (!BETWEEN(t2, t7, t3))
            Swap4(t5, t6, t7, t2, t1, t4, t1, t4, t5, t9, t10, t1);
        else if (BETWEEN(t2, t7, t6))
            Swap4(t5, t6, t7, t5, t8, t3, t3, t8, t1, t9, t10, t1);
        else
            Swap4(t1, t2, t7, t7, t2, t3, t4, t7, t6, t9, t10, t1);
        return;
    case 27:
        Swap4(t3, t4, t5, t6, t3, t2, t1, t6, t7, t9, t10, t1);
        return;
    case 28:
        Swap4(t6, t5, t8, t2, t1, t4, t8, t5, t4, t9, t10, t1);
        return;
    case 29:
        Swap4(t1, t2, t7, t3, t4, t5, t3, t6, t7, t9, t10, t1);
        return;
    case 30:
        if (BETWEEN(t3, t7, t5))
            Swap4(t3, t4, t5, t7, t8, t1, t7, t2, t3, t9, t10, t1);
        else
            Swap4(t3, t4, t5, t3, t6, t7, t1, t2, t3, t9, t10, t1);
        return;
    case 31:
        Swap4(t3, t4, t5, t3, t6, t7, t8, t3, t2, t9, t10, t1);
        return;
    case 32:
        Swap4(t1, t2, t3, t7, t8, t9, t6, t5, t4, t7, t10, t1);
        return;
    case 33:
        if (BETWEEN(t3, t9, t5))
            Swap4(t1, t2, t3, t5, t6, t7, t10, t9, t8, t5, t10, t1);
        else
            Swap4(t1, t2, t3, t7, t8, t9, t7, t10, t1, t5, t6, t7);
        return;
    case 34:
        Swap4(t7, t8, t9, t1, t2, t3, t1, t4, t5, t7, t10, t1);
        return;
    case 35:
        Swap4(t9, t10, t1, t5, t6, t7, t4, t3, t2, t9, t4, t5);
        return;
    case 36:
        Swap4(t9, t10, t1, t7, t8, t9, t3, t4, t5, t6, t3, t2);
        return;
    case 37:
        if (BETWEEN(t6, t9, t4))
            Swap4(t1, t2, t3, t6, t5, t4, t9, t10, t1, t8, t7, t6);
        else
            Swap4(t9, t10, t1, t3, t4, t5, t3, t6, t7, t3, t8, t9);
        return;
    case 38:
        if (BETWEEN(t3, t9, t7))
            Swap4(t1, t2, t3, t7, t8, t9, t6, t5, t4, t6, t1, t10);
        else if (BETWEEN(t6, t9, t4))
            Swap4(t1, t2, t3, t6, t5, t4, t7, t8, t9, t7, t10, t1);
        else
            Swap4(t3, t4, t5, t9, t10, t1, t8, t7, t6, t3, t8, t9);
        return;
    case 39:
        Swap4(t1, t2, t3, t7, t8, t9, t5, t6, t7, t1, t4, t5);
        return;
    case 40:
        Swap4(t9, t10, t1, t4, t3, t2, t5, t6, t7, t5, t8, t9);
        return;
    case 41:
        if (BETWEEN(t5, t9, t7))
            Swap4(t7, t8, t9, t1, t2, t3, t6, t5, t4, t7, t10, t1);
        else if (BETWEEN(t3, t9, t6))
            Swap4(t1, t2, t3, t5, t6, t7, t9, t10, t1, t5, t8, t9);
        else
            Swap4(t5, t6, t7, t9, t10, t1, t2, t9, t8, t3, t4, t5);
        return;
    case 42:
        if (BETWEEN(t3, t9, t6))
            Swap4(t7, t8, t9, t5, t6, t7, t1, t2, t3, t1, t4, t5);
        else
            Swap4(t9, t10, t1, t5, t6, t7, t3, t4, t5, t3, t8, t9);
        return;
    case 43:
        Swap4(t1, t2, t3, t7, t8, t9, t6, t5, t4, t7, t10, t1);
        return;
    case 44:
        if (BETWEEN(t4, t9, t7))
            Swap4(t7, t8, t9, t5, t6, t7, t1, t2, t3, t5, t10, t1);
        else if (BETWEEN(t6, t9, t3))
            Swap4(t9, t10, t1, t5, t6, t7, t3, t4, t5, t3, t8, t9);
        else
            Swap4(t7, t8, t9, t1, t2, t3, t6, t5, t4, t7, t10, t1);
        return;
    case 45:
        Swap4(t9, t10, t1, t3, t4, t5, t7, t8, t9, t3, t6, t7);
        return;
    case 46:
        if (BETWEEN(t6, t9, t3))
            Swap4(t7, t8, t9, t5, t6, t7, t3, t4, t5, t1, t2, t3);
        else
            Swap4(t7, t8, t9, t5, t6, t7, t3, t4, t5, t1, t2, t3);
        return;
    case 47:
        if (BETWEEN(t4, t9, t7))
            Swap4(t5, t6, t7, t1, t2, t3, t9, t10, t1, t5, t8, t9);
        else if (BETWEEN(t5, t9, t3))
            Swap4(t9, t10, t1, t7, t8, t9, t5, t6, t7, t3, t4, t5);
        else
            Swap4(t7, t8, t9, t3, t4, t5, t3, t6, t7, t2, t1, t10);
        return;
    case 48:
        if (BETWEEN(t7, t9, t3))
            Swap4(t3, t4, t5, t8, t7, t6, t2, t1, t10, t8, t3, t2);
        else
            Swap4(t3, t4, t5, t7, t8, t9, t3, t6, t7, t1, t2, t3);
        return;
    case 49:
        Swap4(t9, t10, t1, t5, t6, t7, t3, t4, t5, t3, t8, t9);
        return;
    case 50:
        Swap4(t3, t4, t5, t3, t6, t7, t9, t10, t1, t8, t3, t2);
        return;
    case 51:
        Swap4(t5, t6, t7, t1, t2, t3, t9, t10, t1, t4, t9, t8);
        return;
    case 52:
        Swap4(t5, t6, t7, t3, t4, t5, t9, t10, t1, t3, t8, t9);
        return;
    }
}

/*
   The Minimum1TreeCost function returns the cost of a minimum 1-tree.

   The minimum 1-tre is found by determining the minimum spanning tree and 
   then adding an edge corresponding to the second nearest neighbor of one 
   of the leaves of the tree (any node which has degree 1). The leaf chosen
   is the one that has the longest second nearest neighbor distance.

   The V-value of a node is its degree minus 2. Therefore, Norm being the 
   sum of squares of all V-values, is a measure of a minimum 1-tree/s 
   discrepancy from a tour. If Norm is zero, then the 1-tree constitutes a 
   tour, and an optimal tour has been found.
*/

double kGUITSP::Minimum1TreeCost(const int Sparse)
{
    Node *N, *N1=0;
    double Sum = 0;
    long Max;

    MinimumSpanningTree(Sparse);
    N = FirstNode;
    do {
        N->V = -2;
        Sum += N->Pi;
    }
    while ((N = N->Suc) != FirstNode);
    Sum *= -2;
    while ((N = N->Suc) != FirstNode) {
        N->V++;
        N->Dad->V++;
        Sum += N->Cost;
        N->Next = 0;
    }
    FirstNode->Dad = FirstNode->Suc;
    FirstNode->Cost = FirstNode->Suc->Cost;
    Max = LONG_MIN;
    do {
        if (N->V == -1) {
            Connect(N, Max, Sparse);
            if (N->NextCost > Max) {
                N1 = N;
                Max = N->NextCost;
            }
        }
    }
    while ((N = N->Suc) != FirstNode);
    N1->Next->V++;
    N1->V++;
    Sum += N1->NextCost;
    m_Norm = 0;
    do
        m_Norm += N->V * N->V;
    while ((N = N->Suc) != FirstNode);
    if (N1 == FirstNode)
        N1->Suc->Dad = 0;
    else {
        FirstNode->Dad = 0;
        Precede(N1, FirstNode);
        FirstNode = N1;
    }
    if (m_Norm == 0)
        for (N = FirstNode->Dad; N; N1 = N, N = N->Dad)
            Follow(N, N1);
    return Sum;
}

/*
   The MinimumSpanningTree function determines a minimum spanning tree using 
   Prim's algorithm.

   At return the Dad field of each node contains the father of the node, and the
   Cost field contains cost of the corresponding edge. The nodes are placed in a
   topological ordered list, i.e., for any node its father precedes the node in the
   list. The fields Pred and Suc of a node are pointers to the predecessor and
   successor node in this list.

   The function can be used to determine a minimum spanning tree in a dense graph,
   or in a sparse graph (a graph determined by a candidate set).

   When the graph is sparse a priority queue, implemented as a binary heap, is used 
   to speed up the determination of which edge to include next into the tree.
   The Rank field of a node is used to contain its priority (usually equal to the 
   smallest distance (Cost) to nodes of the tree).        
*/

void kGUITSP::MinimumSpanningTree(const int Sparse)
{
    Node *Blue,                 /* Points to the last node included in the tree */
    *NextBlue=0,                  /* Points to the provisional next node to be included */
    *N=0;
    Candidate *NBlue=0;
    long Min, d;

    Blue = N = FirstNode;
    Blue->Dad = 0;              /* The root of the tree has no father */
    Blue->Loc = 0;              /* A blue node is not in the heap */
    if (Sparse && Blue->CandidateSet) {
        /* The graph is sparse */
        /* Insert all nodes in the heap */
        while ((N = N->Suc) != FirstNode) {
            N->Dad = Blue;
            N->Cost = N->Rank = LONG_MAX;
            Insert(N);
        }
        /* Update all neighbors to the blue node */
        for (NBlue = Blue->CandidateSet; (N = NBlue->To); NBlue++) {
            if (Fixed(N, Blue)) {
                N->Dad = Blue;
                N->Cost = NBlue->Cost + N->Pi + Blue->Pi;
                N->Rank = LONG_MIN;
                SiftUp(N);
            } else if (!Blue->FixedTo2 && !N->FixedTo2) {
                N->Dad = Blue;
                N->Cost = N->Rank = NBlue->Cost + N->Pi + Blue->Pi;
                SiftUp(N);
            }
        }
        /* Loop as long as there a more nodes to include in the tree */
        while ((NextBlue = DeleteMin())) {
            Follow(NextBlue, Blue);
            Blue = NextBlue;
            /* Update all neighbors to the blue node */
            for (NBlue = Blue->CandidateSet; (N = NBlue->To); NBlue++) {
                if (!N->Loc)
                    continue;
                if (Fixed(N, Blue)) {
                    N->Dad = Blue;
                    N->Cost = NBlue->Cost + N->Pi + Blue->Pi;
                    N->Rank = LONG_MIN;
                    SiftUp(N);
                } else if (!Blue->FixedTo2 && !N->FixedTo2 &&
                           (d =
                            NBlue->Cost + N->Pi + Blue->Pi) < N->Cost) {
                    N->Dad = Blue;
                    N->Cost = N->Rank = d;
                    SiftUp(N);
                }
            }
        }
    } else {
        /* The graph is dense */
        while ((N = N->Suc) != FirstNode)
            N->Cost = LONG_MAX;
        /* Loop as long as there a more nodes to include in the tree */
        while ((N = Blue->Suc) != FirstNode) {
            Min = LONG_MAX;
            /* Update all non-blue nodes (the successors of Blue in the list) */
            do {
                if (Fixed(Blue, N)) {
                    N->Dad = Blue;
                    N->Cost = (this->*D)(N, Blue);
                    NextBlue = N;
                    Min = LONG_MIN;
                } else {
                    if (!Blue->FixedTo2 && !N->FixedTo2 &&
                        !Forbidden(N, Blue) &&
                        (!c || (this->*c)(Blue, N) < N->Cost) &&
                        (d = (this->*D)(Blue, N)) < N->Cost) {
                        N->Cost = d;
                        N->Dad = Blue;
                    }
                    if (N->Cost < Min) {
                        Min = N->Cost;
                        NextBlue = N;
                    }
                }
            }
            while ((N = N->Suc) != FirstNode);
            Follow(NextBlue, Blue);
            Blue = NextBlue;
        }
    }
}

/*
   The NormalizeNodeList function is used to swap the Suc and Pred fields 
   of nodes in such a way that the list of nodes constitutes a cyclic 
   two-way list. 

   A call of the function corrupts the segment list representation.   
*/

void kGUITSP::NormalizeNodeList(void)
{
    Node *t1, *t2;

    t1 = FirstNode;
    do {
        t2 = SUC(t1);
        t1->Pred = PRED(t1);
        t1->Suc = t2;
    }
    while ((t1 = t2) != FirstNode);
}

#if 0
/*      
   The ReadLine function reads the next input line from a file. The function
   handles the problem that an input line may be terminated by a carriage
   return, a newline, both, or EOF.
*/

static char *Buffer = 0;
static unsigned long MaxBuffer = 0;

static int EndOfLine(FILE * InputFile, int c)
{
    int EOL = (c == '\r' || c == '\n');
    if (c == '\r') {
        c = fgetc(InputFile);
        if (c != '\n' && c != EOF)
            ungetc(c, InputFile);
    }
    return EOL;
}

char *ReadLine(FILE * InputFile)
{
    int i, c;

    if (Buffer == 0)
        assert(Buffer = (char *) malloc(MaxBuffer = 1));
    for (i = 0; (c = fgetc(InputFile)) != EOF && !EndOfLine(InputFile, c);
         i++) {
        if (i >= MaxBuffer - 1) {
            MaxBuffer *= 2;
            assert(Buffer = (char *) realloc(Buffer, MaxBuffer));
        }
        Buffer[i] = (char) c;
    }
    Buffer[i] = '\0';
    return c == EOF && i == 0 ? 0 : Buffer;
}
#endif

/*
   The ReadParameters function reads the name of a parameter file from standard
   input and reads the problem parameters from this file.

   The order of specifications in the file is arbitrary.

   The format is as follows: 

   PROBLEM_FILE = <string>
   Specifies the name of the problem file.

   Additional control information may be supplied in the following format:

   ASCENT_CANDIDATES = <integer>
   The number of candidate edges to be associated with each node during 
   the ascent. The candidate set is complemented such that every candidate 
   edge is associated with both its two end nodes.
   Default: 50.
   
   BACKTRACK_MOVE_TYPE = <integer>
   Specifies the backtrack move type to be used in local search. A backtrack 
   move allows for backtracking up to a certain level of the local search. 
   A value of 2, 3, 4 or 5 signifies that a backtrack 2-opt, 3-opt, 4-opt or 
   5-opt move is to be used as the first move in the search. The value 0 
   signifies that no backtracking is to be used.
   Default: 0. 

   CANDIDATE_FILE = <string>
   Specifies the name of a file to which the candidate sets are to be written.
   If the file already exists, and the PI_FILE exists, the candidate edges are 
   read from the file. Each line contains a node number, the number of the dad of 
   the node in the minimum spanning tree (0, if the node has no dad), the number 
   of candidate edges emanating from the node, followed by the candidate edges.   
   For each candidate edge its end node number and alpha-value are given.

   COMMENT : <string>
   A comment.

   EOF
   Terminates the input data. The entry is optional.

   EXCESS = <real>
   The maximum alpha-value allowed for any candidate edge is set to EXCESS 
   times the absolute value of the lower bound of a solution tour (determined 
   by the ascent).
   Default: 1.0/DIMENSION.

   INITIAL_PERIOD = <integer>
   The length of the first period in the ascent.
   Default: DIMENSION/2 (but at least 100). 

   INITIAL_STEP_SIZE = <integer>
   The initial step size used in the ascent.
   Default: 1.
   
   INITIAL_TOUR_FILE = <string>
   Specifies the name of a file containing a tour to be used as the initial tour
   in the search. The tour is given by a list of integers giving the sequence 
   in which the nodes are visited in the tour. The tour is terminated by a -1.  

   INPUT_TOUR_FILE = <string>
   Specifies the name of a file containing a tour. The tour is given by a 
   list of integers giving the sequence in which the nodes are visited in 
   the tour. The tour is terminated by a -1. The tour is used to limit the 
   search (the last edge to be removed in a non-gainful move must not belong to 
   the tour). In addition, the Alpha field of its edges is set to zero.

   MAX_CANDIDATES = <integer> { SYMMETRIC }
   The maximum number of candidate edges to be associated with each node.
   The integer may be followed by the keyword SYMMETRIC, signifying that 
   the candidate set is to be complemented such that every candidate edge 
   is associated with both its two end nodes. 
   Default: 5.
   
   MAX_SWAPS = <integer>
   Specifies the maximum number of swaps (flips) allowed in any search for a 
   tour improvement.
   Default: DIMENSION.

   MAX_TRIALS = <integer>
   The maximum number of trials in each run. 
   Default: number of nodes (DIMENSION, given in the problem file).
   
   MERGE_TOUR_FILE_1 = <string>
   Specifies the name of a tour to be merged. The edges of the tour are added
   to the candidate sets with alpha-values equal to 0. 
   
   MERGE_TOUR_FILE_2 = <string>
   Specifies the name of a tour to be merged. The edges of the tour are added
   to the candidate sets with alpha-values equal to 0.               

   MOVE_TYPE = <integer>
   Specifies the move type to be used in local search. The value can be 
   2, 3, 4 or 5 which signifies that a 2-opt, 3-opt, 4-opt or 5-opt move 
   is to be used.
   Default: 5.      

   OPTIMUM = <real>
   Known optimal tour length. A run will be terminated as soon as a tour 
   length less than or equal to optimum is achieved.
   Default: -DBL_MAX.

   PI_FILE = <string>
   Specifies the name of a file to which penalties (pi-values determined 
   by the ascent) are to be written. If the file already exists, the penalties 
   are read from the file, and the ascent is skipped. Each line of the file
   is of the form
       <integer> <integer>
   where the first integer is a node number, and the second integer is the
   Pi-value associated with the node.

   PRECISION = <integer>
   The internal precision in the representation of transformed distances: 
       d[i][j] = PRECISION*c[i][j] + pi[i] + pi[j], 
   where d[i][j], c[i][j], pi[i] and pi[j] are all integral. 
   Default: 100 (which corresponds to 2 decimal places).
   
   RESTRICTED_SEARCH: [ YES | NO ]
   Specifies whether the following search pruning technique is used: 
   The first edge to be broken in a move must not belong to the currently 
   best solution tour. When no solution tour is known, it must not belong to 
   the minimum spanning 1-tree.     
   Default: YES.         

   RUNS = <integer>
   The total number of runs. 
   Default: 10.

   SEED = <integer>
   Specifies the initial seed for random number generation.
   Default: 1.

   SUBGRADIENT: [ YES | NO ]
   Specifies whether the pi-values should be determined by subgradient 
   optimization.
   Default: YES.

   TOUR_FILE = <string>
   Specifies the name of a file to which the best tour is to be written.

   TRACE_LEVEL = <integer>
   Specifies the level of detail of the output given during the solution 
   process. The value 0 signifies a minimum amount of output. The higher 
   the value is the more information is given.
   Default: 1.        
*/

//static char Delimiters[] = " =\n\t\r\f\v";

/*      
   The ReadProblem function reads the problem data in TSPLIB format from the file 
   specified in the parameter file (PROBLEM_FILE).

   The following description of the file format is extracted from the TSPLIB 
   documentation.  

   The file consists of a specification part and a data part. The specification part
   contains information on the file format and on its contents. The data part contains
   explicit data.

   (1) The Specification part

   All entries in this section are of the form <keyword> : <value>, where <keyword> 
   denotes an alphanumerical keyword and <value> denotes alphanumerical or numerical
   data. The terms <string>, <integer> and <real> denote character string, integer 
   or real data, respectively. The order of specification of the keywords in the data
   file is arbitrary (in principle), but must be consistent, i.e., whenever a keyword
   is specified, all necessary information for the correct interpretation of the
   keyword has to be known. Below is given a list of all available keywords.

   NAME : <string>e
   Identifies the data file.

   TYPE : <string>
   Specifies the type of data. Possible types are
   TSP          Data for a symmetric traveling salesman problem
   ATSP         Data for an asymmetric traveling salesman problem
   HCP          Hamiltonian cycle problem data.
   HPP          Hamiltonian path problem data (not available in TSPLIB)

   COMMENT : <string>
   Additional comments (usually the name of the contributor or the creator of the
   problem instance is given here).

   DIMENSION : < integer>
   The number of nodes.

   EDGE_WEIGHT_TYPE : <string>
   Specifies how the edge weights (or distances) are given. The values are:
   EXPLICIT     Weights are listed explicitly in the corresponding section
   EUC_2D       Weights are Euclidean distances in 2-D
   EUC_3D       Weights are Euclidean distances in 3-D
   MAX_2D       Weights are maximum distances in 2-D
   MAX_3D       Weights are maximum distances in 3-D
   MAN_2D       Weights are Manhattan distances in 2-D
   MAN_3D       Weights are Manhattan distances in 3-D
   CEIL_2D      Weights are Euclidean distances in 2-D rounded up
   CEIL_3D      Weights are Euclidean distances in 3-D rounded up
   GEO          Weights are geographical distances (TSPLIB)
   GEOM         Weights are geographical distances (used for the world TSP) 
   ATT          Special distance function for problem att48 and att532

   EDGE-WEIGHT_FORMAT : <string>
   Describes the format of the edge weights if they ar given explicitely. The values
   are
   FULL_MATRIX          Weights are given by a full matrix
   UPPER_ROW            Upper triangular matrix (row-wise without diagonal entries)
   LOWER_ROW            Lower triangular matrix (row-wise without diagonal entries)     
   UPPER_DIAG_ROW       Upper triangular matrix (row-wise including diagonal entries)
   LOWER_DIAG_ROW       Lower triangular matrix (row-wise including diagonal entries)
   UPPER_COL            Upper triangular matrix (column-wise without diagonal entries)
   LOWER_COL            Lower triangular matrix (column-wise without diagonal entries)  
   UPPER_DIAG_COL       Upper triangular matrix (column-wise including diagonal entries)
   LOWER_DIAG_COL       Lower triangular matrix (colum-wise including diagonal entries)

   EDGE_DATA_FORMAT : <string>
   Describes the format in which the edges of a graph are given, if the graph is
   not complete. The values are
   EDGE_LIST    The graph is given by an edge list
   ADJ_LIST     The graph is given by an adjacency list

   NODE_COORD_TYPE : <string>
   Specifies whether the coordinates are associated with each node (which, for
   example may be used for either graphical display or distance computations.
   The values are
   TWOD_COORDS          Nodes are specified by coordinates in 2-D
   THREE_COORDS         Nodes are specified by coordinates in 3-D
   NO_COORDS            The nodes do not have associated coordinates
   The default value is NO_COORDS. In the current implementation, however, the value 
   has no significance.

   DISPLAY_DATA_TYPE : <string>
   Specifies how a graphical display of the nodes can be obtained. The values are
   COORD_DISPLAY        Display is generated from the node coordinates
   TWOD_DISPLAY         Explicit coordinates in 2-D are given
   BO_DISPLAY           No graphical display is possible

   The default value is COORD_DISPLAY if node coordinates are specifies and 
   NO_DISPLAY otherwise. In the current implementation, however, the value has no 
   significance.

   EOF
   Terminates input data. The entry is optional.

   (2) The data part

   Depending on the choice of specifications some additional data may be required. 
   These data are given corresponding data sections following the specification part.
   Each data section begins with the corresponding keyword. The length of the section
   is either explicitly known form the format specification, or the section is
   terminated by an appropriate end-of-section identifier.

   NODE_COORD_SECTION :
   Node coordinates are given in this section. Each line is of the form
   <integer> <real> <real>
   if NODE_COORD_TYPE is TWOD_COORDS, or
   <integer> <real> <real> <real>
   if NODE_COORD_TYPE is THREED_COORDS. The integers give the number of the 
   respective nodes. The real numbers are the associated coordinates.

   EDGE_DATA_SECTION :
   Edges of the graph are specified in either of the two formats allowed in the
   EDGE_DATA_FORAT entry. If a type is EDGE_LIST, then the edges are given as a 
   sequence of lines of the form
   <integer> <integer>
   each entry giving the terminal nodes of some edge. The list is terminated by
   a -1. If the type is ADJ_LIST, the section consists of adjacency list for nodes.
   The adjacency list of a node x is specified as
   <integer> <integer> ... <integer> -1
   where the first integer gives the number of node x and the following integers
   (terminated by -1) the numbers of the nodes adjacent to x. The list of adjacency
   lists are terminated by an additional -1.

   FIXED_EDGES_SECTION :
   In this section, edges are listed that are required to appear in each solution
   to the problem. The edges to be fixed are given in the form (per line)
   <integer> <integer>
   meaning that the edge (arc) from the first node to the second node has to be
   contained in a solution. This section is terminated by a -1.

   DISPLAY_DATA_SECTION :
   If DISPLAY_DATA_TYPE is TWOD_DISPLAY, the 2-dimensional coordinates from which
   a display can be generated are given in the form (per line)
   <integer> <real> <real>
   The integers specify the respective nodes and the real numbers give the 
   associated coordinates. The contents of this section, however, has no 
   significance in the current implementation.

   TOUR_SECTION :
   A tour is specified in this section. The tour is given by a list of integers
   giving the sequence in which the nodes are visited in the tour. The tour is
   terminated by a -1. Note: In contrast to the TSPLIB format, only one tour can 
   be given in this section. The tour is used to limit the search (the last edge 
   to be removed in a non-gainful move must not belong to the tour). In addition, 
   the Alpha field of its edges is set to zero.

   EDGE_WEIGHT_SECTION :
   The edge weights are given in the format specifies by the EDGE_WEIGHT_FORMAT 
   entry. At present, all explicit data are integral and is given in one of the
   (self-explanatory) matrix formats, with explicitly known lengths.
*/

static const int MaxMatrixDimension = 2000;

//static void CheckSpecificationPart();
//static char *Copy(char *S);
//static void CreateNodes();

#if 0
void CheckSpecificationPart()
{
    if (m_problemtype == -1)
        eprintf("TYPE is missing");
    if (m_dimension <= 0)
        eprintf("DIMENSION is not positive (or not specified)");
    if (m_weighttype == -1 && m_problemtype != HCP)
        eprintf("EDGE_WEIGHT_TYPE is missing");
    if (m_weighttype == EXPLICIT && m_weightformat == -1)
        eprintf("EDGE_WEIGHT_FORMAT is missing");
    if (m_weighttype != EXPLICIT && m_weighttype != SPECIAL
        && m_weightformat != -1)
        eprintf("Conflicting EDGE_WEIGHT_TYPE and EDGE_WEIGHT_FORMAT (1)");
    if (m_problemtype == ATSP && m_weighttype != EXPLICIT)
        eprintf("Conflicting TYPE and EDGE_WEIGHT_TYPE");
    if (m_problemtype == ATSP && m_weightformat != FULL_MATRIX)
        eprintf("Conflicting TYPE and EDGE_WEIGHT_FORMAT");
}
#endif

#if 0
static char *Copy(char *S)
{
    char *Buffer;

    if (strlen(S) == 0)
        return 0;
    assert(Buffer = (char *) malloc(strlen(S) + 1));
    strcpy(Buffer, S);
    return Buffer;
}
#endif

void kGUITSP::CreateNodes(void)
{
    Node *Prev=0, *N=0;
    long i;

//    if (m_dimension <= 0)
//        eprintf("DIMENSION is not positive (or not specified)");
    if (m_problemtype == ATSP)
        m_dimension *= 2;
    else if (m_problemtype == HPP) {
        m_dimension++;
      //  if (m_dimension > MaxMatrixDimension)
      //      eprintf("m_dimension too large in HPP problem");
    }
    assert(NodeSet = (Node *) calloc(m_dimension + 1, sizeof(Node)));
    for (i = 1; i <= m_dimension; i++, Prev = N) {
        N = &NodeSet[i];
        if (i == 1)
            FirstNode = N;
        else
            Link(Prev, N);
        N->Id = i;
    }
    Link(N, FirstNode);
}

/*
   The RecordBestTour function records the current best tour in the m_besttour 
   array.

   The function is called by LKmain each time a run has resulted in a
   shorter tour. Thus when the predetermined number of runs have been is
   completed, m_besttour contains an array representation of the best tour
   found.    
*/

void kGUITSP::RecordBestTour(void)
{
    long i;

    for (i = 1; i <= m_dimension; i++)
        m_besttour[i] = BetterTour[i];
}

/*
   The RecordBetterTour function is called by FindTour each time
   the LinKernighan function has returned a better tour.

   The function records the tour in the BetterTour array and in the
   BestSuc field of each node. Furthermore, for each node the previous 
   value of BestSuc is saved in the NextBestSuc field.

   Recording a better tour in the BetterTour array when the problem is 
   asymmetric requires special treatment since the number of nodes has
   been doubled.  
*/

void kGUITSP::RecordBetterTour(void)
{
    Node *N;
    long i, k;

    for (i = 1, N = FirstNode, k = 0; i <= m_dimension; i++, N = N->Suc) {
        if (m_problemtype != ATSP)
            BetterTour[++k] = N->Id;
        else if (N->Id <= m_dimension / 2) {
            k++;
            if (N->Suc->Id != N->Id + m_dimension / 2)
                BetterTour[k] = N->Id;
            else
                BetterTour[m_dimension / 2 - k + 1] = N->Id;
        }
        N->NextBestSuc = N->BestSuc;
        N->BestSuc = N->Suc;
    }
}

/* 
   The RemoveFirstActive function removes the first node in the list 
   of "active" nodes (i.e., nodes to be tried as an anchor node, t1,
   by the LinKernighan algorithm).

   The function returns a pointer to the node removes. 

   The list must not be empty before the call. 
*/

Node *kGUITSP::RemoveFirstActive(void)
{
    Node *t = FirstActive;
    if (FirstActive == LastActive)
        FirstActive = LastActive = 0;
    else
        FirstActive = FirstActive->Next;
    if (t)
        t->Next = 0;
    return t;
}

/*
   Each time a trial has resulted in a shorter tour the candidate set is
   adjusted (by AdjustCandidateSet). The ResetCandidates function resets
   the candidate set. Any included edges are removed and the original
   order is re-established.

   The function is called at the end of the FindTour function.   
*/

void kGUITSP::ResetCandidateSet(void)
{
    Candidate *NFrom, Temp, *NN;
    Node *From;

    From = FirstNode;
    /* Loop for all nodes */
    do {
        /* Reorder the candidate array of From */
        for (NFrom = From->CandidateSet + 1; NFrom->To; NFrom++) {
            if (InOptimumTour(From, NFrom->To))
                NFrom->Alpha = 0;
            Temp = *NFrom;
            for (NN = NFrom - 1;
                 NN >= From->CandidateSet &&
                 (Temp.Alpha < NN->Alpha ||
                  (Temp.Alpha == NN->Alpha && Temp.Cost < NN->Cost)); NN--)
                *(NN + 1) = *NN;
            *(NN + 1) = Temp;
        }
        NFrom--;
        /* Remove any included edges */
        while (NFrom->Alpha == LONG_MAX)
            NFrom--;
        NFrom++;
        NFrom->To = 0;
    }
    while ((From = From->Suc) != FirstNode);
}

/*
   The RestoreTour function is used to undo a series of moves. The function 
   restores the tour from SwapStack, the stack of 2-opt moves. A bad sequence 
   of moves is undone by unstacking the 2-opt moves and making the inverse 
   2-opt moves in this reversed sequence.
*/

void kGUITSP::RestoreTour(void)
{
    Node *t1, *t2, *t3, *t4;

    /* Loop as long as the stack is not empty */
    while (Swaps > 0) {
        /* Undo topmost 2-opt move */
        Swaps--;
        t1 = SwapStack[Swaps].t1;
        t2 = SwapStack[Swaps].t2;
        t3 = SwapStack[Swaps].t3;
        t4 = SwapStack[Swaps].t4;
        Swap1(t3, t2, t1);
        Swaps--;
        /* Make edges (t1,t2) and (t2,t3) excludable again */
        t1->OldPredExcluded = t1->OldSucExcluded = 0;
        t2->OldPredExcluded = t2->OldSucExcluded = 0;
        t3->OldPredExcluded = t3->OldSucExcluded = 0;
        t4->OldPredExcluded = t4->OldSucExcluded = 0;
    }
}

/* 
   The StoreTour function is called each time the tour has been improved by 
   the LinKernighan function.

   The function "activates" all nodes involved in the current sequence of moves.

   It sets OldPred to Pred and OldSuc to Suc for each of these nodes. In this
   way it can always be determined whether an edge belongs to current starting
   tour. This is used by the BestMove function to determine whether an edge is
   excludable.

   Finally, for each of these nodes the function updates their Cost field.
   The Cost field contains for each node its minimum cost of candidate edges 
   not on the tour. This fact is used by the BestMove function to decide whether
   a tentative non-gainful move should be considered. 
*/

void kGUITSP::StoreTour(void)
{
    Node *t, *u;
    Candidate *Nt;
    int i;

    while (Swaps > 0) {
        Swaps--;
        for (i = 1; i <= 4; i++) {
            t = i == 1 ? SwapStack[Swaps].t1 :
                i == 2 ? SwapStack[Swaps].t2 :
                i == 3 ? SwapStack[Swaps].t3 : SwapStack[Swaps].t4;
            Activate(t);
            t->OldPred = t->Pred;
            t->OldSuc = t->Suc;
            t->OldPredExcluded = t->OldSucExcluded = 0;
            t->Cost = LONG_MAX;
            for (Nt = t->CandidateSet; (u = Nt->To); Nt++)
                if ((u != t->Pred) && (u != t->Suc) && (Nt->Cost < t->Cost))
                    t->Cost = Nt->Cost;
        }
    }
}

/***********************************************************************************/

kGUITSP::kGUITSP()
{
	m_problemtype = -1;
	m_weighttype = -1;
	m_weightformat = -1;
	m_coordtype = NO_COORDS;
	m_candidatesetsymmetric = 0;
	
	FirstNode = 0;
    FirstSegment = 0;
    NodeSet = 0;
    CostMatrix = 0;
    m_besttour = 0;
    BetterTour = 0;
    SwapStack = 0;
    HTable = 0;
    Rand = 0;
    CacheSig = 0;
    CacheVal = 0;
    Heap = 0;
	BacktrackMove=0;

	m_count=0;
	m_runs=10;
    Seed = 1;
    MaxTrials = 0;
    MaxSwaps = -1;
    m_maxcandidates = 5;
    m_ascentcandidates = 50;
    Optimum = -DBL_MAX;
    m_initialperiod = 0;
	InitialStepSize = 0;
    Precision = 100;
    Subgradient = 1;
    Excess = 0.0;
    MoveType = 5;
    BacktrackMoveType = 0;
    RestrictedSearch = 1;
	
	m_problemtype = TSP;

    m_weighttype = GEO;
	Distance = &kGUITSP::Distance_GEO;
    c = &kGUITSP::c_GEO;

    m_weightformat = -1;
    m_coordtype = NO_COORDS;
    C = 0;
}

kGUITSP::~kGUITSP()
{
	while(m_thread.GetActive())
		m_stop=true;
}

void kGUITSP::Init(int numcoords)
{
	int i;

	m_dimension=numcoords;

	m_numcoords=numcoords;
	m_curpath.Alloc(m_numcoords);
	m_bestpath.Alloc(m_numcoords);
	m_x.Alloc(m_numcoords);
	m_y.Alloc(m_numcoords);
	for(i=0;i<m_numcoords;++i)
	{
		m_bestpath.SetEntry(i,i);
		m_curpath.SetEntry(i,i);
	}
}

void kGUITSP::AsyncCalc(void)
{
	m_thread.Start(this,CALLBACKNAME(Calc));
}

void kGUITSP::Calc(void)
{
    Node *N;
    long Id, i;
    long TrialSum, MinTrial, Successes, Run;
    double Cost, CostSum;	//, Time, TimeSum, MinTime;
    Segment *S=0, *SPrev;
#if 1
	int MoveTypeInc=0;
	int BacktrackMoveTypeInc=0;
#endif
	m_stop=false;
	/* not enough points? */
	if(m_numcoords<=2)
	{
		for(i=0;i<m_numcoords;++i)
			m_bestpath.SetEntry(i,i);
		goto done;
	}

    if (!FirstNode)
        CreateNodes();
    N = FirstNode;
    if (m_problemtype == HPP)
        m_dimension--;
    for (i = 1; i <= m_dimension; i++)
	{
		Id=i;
        N = &NodeSet[Id];
        N->V = 1;
		N->X=m_x.GetEntry(Id-1);
		N->Y=m_y.GetEntry(Id-1);
        N->Z = 0;
    }
    N = FirstNode;

	do
        if (!N->V && N->Id <= m_dimension)
            break;
    while ((N = N->Suc) != FirstNode);
//    if (!N->V)
  //      eprintf("(NODE_COORD_SECTION) No coordinates given for node %ld",  N->Id);
    if (m_problemtype == HPP)
        m_dimension++;
	
	m_Norm = 9999;
	TrialSum = Successes = 0;
    CostSum = 0.0;	//TimeSum = 0.0;
    MinTrial = LONG_MAX;
//    MinTime = DBL_MAX;

    /* Read the specification of the problem */
//    ReadParameters();
  //  ReadProblem();

    assert(m_besttour = (long *) calloc((m_dimension + 1), sizeof(long)));
    assert(BetterTour = (long *) calloc((m_dimension + 1), sizeof(long)));
    assert(SwapStack =
           (SwapRecord *) malloc((m_dimension + 10) * sizeof(SwapRecord)));
    assert(HTable = (HashTable *) malloc(sizeof(HashTable)));
    assert(Rand = (int *) malloc((m_dimension + 1) * sizeof(int)));
    if (Seed == 0)
        Seed = 1;
    srand(Seed);
    for (i = 1; i <= m_dimension; i++)
        Rand[i] = rand();
    HashInitialize(HTable);
    srand(Seed);
    Swaps = 0;
    MakeHeap(m_dimension);
    if (m_maxcandidates < 0)
        m_maxcandidates = 5;
    else if (m_maxcandidates > m_dimension)
        m_maxcandidates = m_dimension;
    if (m_ascentcandidates > m_dimension)
        m_ascentcandidates = m_dimension;
    if (m_initialperiod == 0) {
        m_initialperiod = m_dimension / 2;
        if (m_initialperiod < 100)
            m_initialperiod = 100;
    }
    if (Precision == 0)
        Precision = 100;
    if (InitialStepSize == 0)
        InitialStepSize = 1;
    if (Excess == 0.0)
        Excess = 1.0 / m_dimension;
    if (MaxTrials == 0)
        MaxTrials = m_dimension;
    if (MaxSwaps < 0)
        MaxSwaps = m_dimension;
    if (CostMatrix == 0 && m_dimension <= MaxMatrixDimension && Distance != 0
		&& Distance != &kGUITSP::Distance_1 && Distance != &kGUITSP::Distance_ATSP) {
        Node *Ni, *Nj;
        assert(CostMatrix =
               (long *) calloc(m_dimension * (m_dimension - 1) / 2,
                               sizeof(long)));
        Ni = FirstNode->Suc;
        do {
            Ni->C = &CostMatrix[(Ni->Id - 1) * (Ni->Id - 2) / 2] - 1;
            if (m_problemtype != HPP || Ni->Id < m_dimension)
                for (Nj = FirstNode; Nj != Ni; Nj = Nj->Suc)
                    Ni->C[Nj->Id] = Fixed(Ni, Nj) ? 0 : (this->*Distance)(Ni, Nj);
            else
                for (Nj = FirstNode; Nj != Ni; Nj = Nj->Suc)
                    Ni->C[Nj->Id] = 0;
        }
        while ((Ni = Ni->Suc) != FirstNode);
        m_weighttype = EXPLICIT;
        c = 0;
    }
    C = m_weighttype == EXPLICIT ? &kGUITSP::C_EXPLICIT : &kGUITSP::C_FUNCTION;
    D = m_weighttype == EXPLICIT ? &kGUITSP::D_EXPLICIT : &kGUITSP::D_FUNCTION;
    if (m_weighttype != EXPLICIT && !FirstNode->CandidateSet)
	{
        for (i = 1; i <= m_dimension; i *= 2);
        assert(CacheSig = (long *) calloc(i, sizeof(long)));
        assert(CacheVal = (long *) calloc(i, sizeof(long)));
    }
    if (MoveType == 0)
        MoveType = 5;

	switch (MoveType) {
    case 2:
        BestMove = &kGUITSP::Best2OptMove;
        break;
    case 3:
        BestMove = &kGUITSP::Best3OptMove;
        break;
    case 4:
        BestMove = &kGUITSP::Best4OptMove;
        break;
    case 5:
        BestMove = &kGUITSP::Best5OptMove;
        break;
    }
    switch (BacktrackMoveType)
	{
    case 2:
        BacktrackMove = &kGUITSP::Backtrack2OptMove;
        break;
    case 3:
        BacktrackMove = &kGUITSP::Backtrack3OptMove;
        break;
    case 4:
        BacktrackMove = &kGUITSP::Backtrack4OptMove;
        break;
    case 5:
        BacktrackMove = &kGUITSP::Backtrack5OptMove;
        break;
    }
    GroupSize = (long)(sqrt(1.0 * m_dimension));
    Groups = 0;
    for (i = m_dimension, SPrev = 0; i > 0; i -= GroupSize, SPrev = S) {
        S = (Segment *) malloc(sizeof(Segment));
        S->Rank = ++Groups;
        if (!SPrev)
            FirstSegment = S;
        else
            Link(SPrev, S);
    }
    Link(S, FirstSegment);

	CreateCandidateSet();
//    printf("Preprocessing time = %0.0f sec.\n", GetTime() - LastTime);
//    fflush(stdout);
    if (m_Norm != 0)
	{
        BestCost = DBL_MAX;
        WorstCost = -DBL_MAX;
        Successes = 0;
    }
	else
	{
        /* The ascent has solved the problem! */
        Successes = 1;
        m_runs = 0;
        RecordBetterTour();
        RecordBestTour();
        BestCost = WorstCost = Cost = CostSum = LowerBound;
      //  PrintBestTour();
    }
//    TimeSum = 0;
    /* Find a specified number (Runs) of local optima */

	if(m_thread.GetActive()==true)
		m_runs=9999999;			/* run till told to stop */
	else if(m_dimension>40)
		m_runs=10;
	else if(m_dimension>25)
		m_runs=20;
	else if(m_dimension>15)
		m_runs=50;
	else
		m_runs=100;

	m_newbest=0;
	for (Run = 1; (Run <= m_runs) && m_stop==false; Run++,++m_count)
	{
//        LastTime = GetTime();

#if 1
		if(++MoveTypeInc==20)
		{
			MoveTypeInc=0;
			switch (++MoveType)
			{
			case 2:
				BestMove = &kGUITSP::Best2OptMove;
			break;
			case 3:
				BestMove = &kGUITSP::Best3OptMove;
			break;
			case 4:
				BestMove = &kGUITSP::Best4OptMove;
			break;
			case 5:
				BestMove = &kGUITSP::Best5OptMove;
				MoveType=0;
			break;
			default:
				BestMove = &kGUITSP::Best5OptMove;
			break;
			}
		}
#if 0
		if(++BacktrackMoveTypeInc==15)
		{
			BacktrackMoveTypeInc=0;
			switch (++BacktrackMoveType)
			{
			case 2:
				BacktrackMove = &kGUITSP::Backtrack2OptMove;
			break;
			case 3:
				BacktrackMove = &kGUITSP::Backtrack3OptMove;
			break;
			case 4:
				BacktrackMove = &kGUITSP::Backtrack4OptMove;
			break;
			case 5:
				BacktrackMove = &kGUITSP::Backtrack5OptMove;
				BacktrackMoveType=0;
			break;
			default:
				BacktrackMove=0;
			break;
			}
		}
#endif
#endif		
		
		Cost = FindTour();      /* using the Lin-Kerninghan heuristics */
        if (Cost < BestCost)
		{
            RecordBestTour();
            BestCost = Cost;

			for(i=0;i<m_numcoords;++i)
				m_curpath.SetEntry(i,m_besttour[i+1]-1);
			++m_newbest;
			//            PrintBestTour();
        }
        /* Update statistics */
        if (Cost > WorstCost)
            WorstCost = Cost;
        if (Cost <= Optimum)
            Successes++;
//        Time = GetTime() - LastTime;
//        if (TraceLevel >= 1)
//		{
  //          printf("Cost = %0.0f, Seed = %u, Time = %0.0f sec.\n",
    //               Cost, Seed, Time);
      //      fflush(stdout);
      //  }
        CostSum += Cost;
        TrialSum += Trial;
        if (Trial < MinTrial)
            MinTrial = Trial;
//        TimeSum += Time;
  //      if (Time < MinTime)
    //        MinTime = Time;
        srand(++Seed);
        if (Cost < Optimum || (Cost == Optimum && Successes == 1))
		{
            if (Cost < Optimum) {
                Node *N;
                N = FirstNode;
                while ((N = N->OptimumSuc = N->Suc) != FirstNode);
//                printf("New optimum = %f, Old optimum = %f\n", Cost,  Optimum);
  //              fflush(stdout);
                Optimum = Cost;
            }
    //        PrintBestTour();
        }

		/* save current */
    }
    /* Report the resuls */
 //   printf("\nSuccesses/Runs = %ld/%ld \n", Successes, m_runs);
    if (m_runs == 0)
	{
        m_runs = 1;
        MinTrial = 0;
//        MinTime = 0;
    }
 //   printf("Cost.min = %0.0f, Cost.avg = %0.1f, Cost.max = %0.0f\n",  BestCost, CostSum / m_runs, WorstCost);
    if (Optimum == -DBL_MAX)
        Optimum = BestCost;
#if 0
	printf("Gap.min = %0.3f%%, Gap.avg = %0.3f%%, Gap.max = %0.3f%%\n",
           (BestCost - Optimum) / Optimum * 100.0,
           (CostSum / m_runs - Optimum) / Optimum * 100.0,
           (WorstCost - Optimum) / Optimum * 100.0);
    printf("MinTrials = %ld, Trials.avg. = %0.1f\n", MinTrial,
           1.0 * TrialSum / m_runs);
    printf("Time.min = %0.0f sec., Time.avg. = %0.1f sec.\n\n", MinTime,
           TimeSum / m_runs);
    fflush(stdout);
#endif

	/* copy best results to array */
	for(i=0;i<m_numcoords;++i)
		m_bestpath.SetEntry(i,m_besttour[i+1]-1);

	FreeStructures();
done:;
	if(m_thread.GetActive()==true)
		m_thread.Close(true);
}
