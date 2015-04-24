#ifndef _table
#define _table


#include <iostream>
#include <map>
#include <vector>
#include "node.h"
#include "link.h"
#include "messages.h"
#include <limits>


using namespace std;
class Node;
class Link;

#if defined(GENERIC)
class Table {
  // Students should write this class

 public:
  ostream & Print(ostream &os) const;
};
#endif


#if defined(LINKSTATE)
class NodeInfo {
    public:
	double lsvalue;
	unsigned tmvalue;
	unsigned prev;

	
	NodeInfo();
	NodeInfo(double ls, unsigned t, unsigned p);
	 ~NodeInfo();


	 unsigned GetLsValue() const;
	 virtual void SetLsValue(const double ls);
	 virtual unsigned GetTmValue() const;
	 virtual void SetTmValue(const unsigned t);
	 virtual unsigned GetPrev() const;
	 virtual void SetPrev(const unsigned p);

};

class Table {
  // Students should write this class
   public:
	map<unsigned, map<unsigned, NodeInfo> > * adjmat;
	unsigned number;
	Table();
	Table(unsigned);
	
	//Table(Node *node);
	~Table();
	//unsigned GetNewTmValue(const Link *l) ;
	unsigned LinkUpdate(const Link *l);
	bool AdjmatUpdate(const RoutingMessage * m);
	NodeInfo * FindNodeInfo(const unsigned s, const unsigned d);
    virtual map<unsigned, map<unsigned, NodeInfo> > * GetAdjmat() ;
	virtual void Dijkstra();
	virtual void SetPrevNb(const unsigned s, const unsigned d, const unsigned p);
	ostream & Print(ostream &os) const;
};
#endif

#if defined(DISTANCEVECTOR)

#include <deque>

class Path{		//Stores dv
	unsigned dest;
	unsigned next;
	double dvValue;
public:
	Path(unsigned d, unsigned n, double dvv);
	Path();
	virtual ~Path();

	virtual unsigned GetDest() const;
	virtual void SetDest(const unsigned d);
	virtual unsigned GetNext() const;
	virtual void SetNext(const unsigned n);
	virtual double GetDvValue() const;
	virtual void SetDvValue(const double dvv);
};

class Table {
	map<unsigned, double>* outgoing;		//Stores all outgoing links
	vector<Path>* dvTable;	//Stores dv from node to all known nodes
	map<unsigned, vector<Path> >* dvTable_nb;	//Stores dv from each neighbor to its known nodes

 public:
 	Table(map<unsigned, double>* o, vector<Path>* dvt, map<unsigned, vector<Path> >* dvt_nb);
 	Table(Node *node);
	Table(const Table &rhs);
	virtual ~Table();
	Table & operator=(const Table &rhs);

	//--------outgoing
	virtual map<unsigned, double>* GetOutgoing() const;
	virtual double GetOutgoingLat(const unsigned d) const;		//Get the lantency of outgoing(dest=d)
	virtual void SetOutgoing(const unsigned d, const double l);		//Set outgoing(dest=d).lat=l
	virtual void AddOutgoing(const unsigned d, const double l);		//Add outgoing(dest=d, lat=l)
	
	//--------dvTable
	virtual vector<Path>* GetDvTable() const;
	virtual void AddDv(const unsigned d);	//Add a new known node d
	virtual Path* FindPath(const unsigned d);	//Find the Path(dest=d)
	virtual void SetPath(const unsigned d, const unsigned n, const double dvv);	//Set Path(dest=d).next=n, .dvValue=dvv
    virtual Path* GetNextH(const unsigned d); //Find the Path(dest=d)
	//--------dvTable_nb
	virtual map<unsigned, vector<Path> >* GetDvTable_nb() const;
	virtual void SetDvTable_nb(const unsigned nn, const vector<Path>* dvt);
	virtual double GetNeighborDvValue(const unsigned s, const unsigned d) ;	//Get the dvValue from s to d
	virtual void AddNeighbor(const unsigned s, const vector<Path>* dvt);	//Add a new neighbor s
	virtual Path* FindNeighborPath(const unsigned s, const unsigned d) const;	//Find the Path from s to d
	

  	ostream & Print(ostream &os) const;

};
#endif

inline ostream & operator<<(ostream &os, const Table &t) { return t.Print(os);}

#endif
