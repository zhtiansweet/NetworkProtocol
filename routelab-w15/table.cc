#include "table.h"

using namespace::std;

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  // WRITE THIS
  os << "Table()";
  return os;
}
#endif

#if defined(LINKSTATE)
NodeInfo::NodeInfo(double ls, unsigned t, unsigned p){
	lsvalue = ls;
	tmvalue = t;
	prev = p;
}

NodeInfo::NodeInfo(){
	lsvalue = -1;
	tmvalue = 0;
	prev = -1;
}

NodeInfo::~NodeInfo(){
}

unsigned NodeInfo::GetLsValue() const{
	return lsvalue;
}

void NodeInfo::SetLsValue(const double ls){
	lsvalue = ls;
}

unsigned NodeInfo::GetTmValue() const{
	return tmvalue;
}

void NodeInfo::SetTmValue(const unsigned t){
	tmvalue=t;
}

unsigned NodeInfo::GetPrev() const{
	return prev;
}

void NodeInfo::SetPrev(const unsigned p){
	prev = p;
}



Table::Table(){
}

Table::Table(unsigned n){
	// cout<<"*******Constructor n********"<<n<<"***********Added**********"<<endl;
	adjmat = new map< unsigned, map<unsigned, NodeInfo> >;
    map<unsigned, NodeInfo> mp;
	NodeInfo ni;

	mp.insert(pair<unsigned, NodeInfo>(n, ni));
	adjmat->insert(pair< unsigned, map<unsigned, NodeInfo> > (n, mp));
	number = n;
}

void Table::SetPrevNb(const unsigned s, const unsigned d, const unsigned p){
	((*adjmat)[s][d]).prev = p; 
}

Table::~Table(){
}

map<unsigned, map<unsigned, NodeInfo> > * Table::GetAdjmat() {
	return adjmat;
}

//every time a link is updated, update the lsvalue and tmvalue
unsigned Table::LinkUpdate(const Link *l){
	unsigned source = l->GetSrc();
	unsigned dest = l->GetDest();
	double lat = l->GetLatency();
	cout<<"------update the link in LinkUpdate-------"<<endl;
	unsigned temptmvalue = (*adjmat)[source][dest].GetTmValue();
	temptmvalue = temptmvalue+1;
	(*adjmat)[source][dest].SetTmValue(temptmvalue);
	(*adjmat)[source][dest].SetLsValue(lat);
	return temptmvalue;
}

//unsigned Table::GetNewTmValue( Link *l) {
//	LinkUpdate(l);
//	return (*adjmat)[l->GetSrc()][l->GetDest()].GetTmValue();
//}


NodeInfo * Table::FindNodeInfo(const unsigned s, const unsigned d){
	return &((*adjmat)[s][d]);
}

//everytime a routing message comes, updat the adjacent matrix
//if the matrix is updated, return true, else return false
bool Table::AdjmatUpdate(const RoutingMessage * m){
	 const unsigned source = m->source;
	 const unsigned dest = m->dest;
	 const double lat = m->lat;
	 const unsigned temptmvalue = m->tmvalue;
	//this link is a default link and has not been updated yet
	if ((*adjmat)[source][dest].GetLsValue() == -1){
		(*adjmat)[source][dest].SetLsValue(lat);
		(*adjmat)[source][dest].SetTmValue(temptmvalue);
		cout<<"------adjacent matrix has been updated-111-----"<<endl;
		return true;
	}
	//this link has been updated 
	if ((*adjmat)[source][dest].GetTmValue() < temptmvalue){
		(*adjmat)[source][dest].SetLsValue(lat);
		(*adjmat)[source][dest].SetTmValue(temptmvalue);
		cout<<"------adjacent matrix has been updated-222----"<<endl;
		return true;
	}
	cout<<"adjacent matrix is not updated"<<endl;
	return false;
}

void Table::Dijkstra(){

    int nodenum = (GetAdjmat())->size();
    int inf = numeric_limits<int>::max();
    vector<unsigned> dx(nodenum, inf);
    vector<unsigned> px(nodenum, inf);
    vector<bool> visited(nodenum, false);
    //initialize of this node
    dx[number] = 0;
    cout<<"number=="<<number<<endl;
    int currentdist = dx[number];
    visited[number] = true;
    px[number] = -1;

    //give initialization value for this node's neighbors
    for (map<unsigned, NodeInfo>::iterator i = (*(GetAdjmat()))[number].begin();
      i != (*(GetAdjmat()))[number].end();i++){
      dx[i->first] = (i->second).GetLsValue();
      px[i->first] = number;
      SetPrevNb(number, i->first,number);
    }

    for (int i = 0; i< nodenum-1; i++)
    {
      int current_relay = -1; 
      int min_dist = inf;
      
      for (int i = 0; i< dx.size(); i++)
      {
        if (visited[i]==false)
        {
          if(min_dist > dx[i])
          {
            current_relay = i;
            min_dist = dx[i];
          }
        }
      }

      visited[current_relay] = true;
      if(min_dist != inf)
      {
        currentdist = min_dist;
      }

      
      for(map<unsigned, NodeInfo>::iterator i = (*(GetAdjmat()))[current_relay].begin();
        i != (*(GetAdjmat()))[current_relay].end();i++)
      {
        if ((dx[i->first]>(i->second.GetLsValue() + currentdist)) && (visited[i->first] == false))
        {
          dx[i->first] = i->second.GetLsValue() + currentdist;
          px[i->first] = current_relay;
          SetPrevNb(number, i->first ,current_relay);
        }
      }

    }
}



//print the adjacent matrix 
ostream & Table::Print(ostream &os) const{

	os<<"routing table of number: "<<number<<endl;
	os<<"start---------------------------------------------------"<<endl;
	for (map<unsigned, map<unsigned, NodeInfo> >::iterator i = (*adjmat).begin();
		i !=(*adjmat).end();i++){
		os<<"From Node"<<i->first<<": ";
		for(map<unsigned, NodeInfo>::iterator j = i->second.begin();j != i->second.end();++j){
			os<<"To Node"<<j->first<< "(Prev Node"<<int(j->second.GetPrev())<<")="<<int((j->second).GetLsValue())<<", ";
			//os<<" "<<(j->second).GetLsValue()<<" ";
		}
		os<<endl;
	}
	
	os<<"--------------------------------------------------------end ";
	os<<endl;
	
	return os;
}

#endif

#if defined(DISTANCEVECTOR)

Path::Path(unsigned d, unsigned n, double dvv)//: dest(d), next(n), dvValue(dvv) 
	{
		dest = d;
	    next = n;
	    dvValue = dvv;
	}

Path::Path(){
	dest=-1, next=-1, dvValue=-1;
}

Path::~Path()
{
}

unsigned Path::GetDest() const{
	return dest;
}

void Path::SetDest(const unsigned d){
	dest=d;
}

unsigned Path::GetNext() const{
	return next;
}

void Path::SetNext(const unsigned n){
	next=n;
}

double Path::GetDvValue() const{
	return dvValue;
}

void Path::SetDvValue(const double dvv){
	dvValue=dvv;
}

Table::Table(map<unsigned, double>* o, 
			vector<Path>* dvt, 
			map<unsigned, vector<Path> >* dvt_nb):
	outgoing(o), dvTable(dvt), dvTable_nb(dvt_nb) {}

Table::Table(Node *node){

	outgoing = new map<unsigned, double>;
	dvTable = new vector<Path>;
	dvTable_nb = new map<unsigned, vector<Path> >;

	deque<Link*>* outgoings=node->GetOutgoingLinks();
	for(deque<Link*>::iterator i=outgoings->begin();
		i !=outgoings->end();++i){
		//Initialize outgoing
		outgoing->insert(make_pair((*i)->GetDest(), (*i)->GetLatency()));
		//Initialize dvTable
		dvTable->push_back(Path((*i)->GetSrc(), (*i)->GetDest(), (*i)->GetLatency()));
	}
	//Add node itself
	Path p; 
	p=Path(node->GetNumber(), node->GetNumber(), 0);
	dvTable->push_back(p);

	//Initialize dvTable_nb
	deque<Node*>* neighbors=node->GetNeighbors();
	for(deque<Node*>::iterator i=neighbors->begin();
		i !=neighbors->end();++i){
		unsigned nm=(*i)->GetNumber();
		vector<Path>* vp=(*i)->GetTable()->GetDvTable();
		dvTable_nb->insert(make_pair(nm, *vp));
	}

}

Table::Table(const Table &rhs):
outgoing(rhs.outgoing), dvTable(rhs.dvTable), dvTable_nb(rhs.dvTable_nb) {}

Table & Table::operator=(const Table &rhs){
	return *(new(this) Table(rhs));
}

Table::~Table()
{}

map<unsigned, double>* Table::GetOutgoing() const{
	return outgoing;
}

double Table::GetOutgoingLat(const unsigned d) const{
	return outgoing->find(d)->second;
}

void Table::SetOutgoing(const unsigned d, const double l){
	(*outgoing)[d]=l;
}

void Table::AddOutgoing(const unsigned d, const double l){
	outgoing->insert(make_pair(d, l));
}

vector<Path>* Table::GetDvTable() const{
	return dvTable;
}

void Table::AddDv(const unsigned d){
	Path p;
	p=Path(d, -1, -1);
	dvTable->push_back(p);
}

Path * Table::FindPath(const unsigned d){
	for(int i=0;i<dvTable->size();i++){

		if((*dvTable)[i].GetDest()==d){
			//return &((*dvTable)[i]);
			//cout<<i<<endl;
			return new Path((*dvTable)[i].GetDest(),(*dvTable)[i].GetNext(),(*dvTable)[i].GetDvValue());
		}
	}
	return new Path(-1, -1, -1);
}

Path * Table::GetNextH(const unsigned d){

}



void Table::SetPath(const unsigned d, const unsigned n, const double dvv){
	for(vector<Path>::iterator i=dvTable->begin();
		i !=dvTable->end();++i){
		if(i->GetDest()==d){
			i->SetNext(n);
			i->SetDvValue(dvv);
		}
	}
}

map<unsigned, vector<Path> >* Table::GetDvTable_nb() const{
	return dvTable_nb;
}

void Table::SetDvTable_nb(const unsigned nn, const vector<Path>* dvt){
	    for(map<unsigned, vector<Path> >::iterator i=GetDvTable_nb()->begin();
        i !=GetDvTable_nb()->end();++i){
        //Go through dvTable_nb to find the targeted dvTable
	    	if (i->first == nn){
	    		(*dvTable_nb)[i->first] = *dvt;
	    	}
        }
	}

double Table::GetNeighborDvValue(const unsigned s, const unsigned d){
	for(map<unsigned, vector<Path> >::const_iterator i=dvTable_nb->begin();
		i != dvTable_nb->end();++i){
		if(i->first==s){
		for(vector<Path>::const_iterator j=i->second.begin();
			j !=i->second.end(); ++j){
			if(j->GetDest()==d){
				return j->GetDvValue();
			}
		}
	//cerr<<"Not found"<<endl;
	return -1;
	}
}
}

void Table::AddNeighbor(const unsigned s, const vector<Path>* dvt){
	dvTable_nb->insert(make_pair(s, *dvt));
}

Path* Table::FindNeighborPath(const unsigned s, const unsigned d) const{

	for(map<unsigned, vector<Path> >::iterator i=dvTable_nb->begin();
		i != dvTable_nb->end();++i){
		if(i->first==s){
			for(vector<Path>::iterator j=i->second.begin();
				j !=i->second.end();++j){
				if(j->GetDest()==d){
					return new Path(j->GetDest(), j->GetNext(), j->GetDvValue());
				}
			}
		}
	}

	//cerr<<"Not found"<<endl;
	return NULL;
}

ostream & Table::Print(ostream &os) const{
	//outgoing
	os<<"Outgoing Links: ";
	for (map<unsigned, double>::const_iterator i = outgoing->begin(); 
		i != outgoing->end(); ++i){
		os<<"-->"<<i->first<<"="<<i->second<<", ";
	}
	os<<endl;
	
	//dvTable
	os<<"Distance Vectors: ";
	for(vector<Path>::const_iterator i=dvTable->begin();
		i !=dvTable->end(); ++i){
		os<<"To Node"<<i->GetDest()<<"(by Node"<<i->GetNext()<<"): "<<i->GetDvValue()<<", ";
	}
	os<<endl;
	
	//dvTable_nb
	os<<"Distance Vectors of Neighbors: "<<endl;
	for(map<unsigned, vector<Path> >::const_iterator i = dvTable_nb->begin(); 
		i != dvTable_nb->end(); ++i){
		os<<"	Node"<<i->first<<": ";
		for(vector<Path>::const_iterator j=i->second.begin();
			j !=i->second.end();++j){
			os<<"To Node"<<j->GetDest()<<"(by Node"<<j->GetNext()<<")="<<j->GetDvValue()<<", ";
		}
		os<<endl;
	}
	os<<endl;
	return os;
}




#endif
