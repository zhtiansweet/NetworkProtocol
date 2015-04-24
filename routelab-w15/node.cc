#include "node.h"
#include "context.h"
#include "error.h"
//#include "float.h"

using namespace std;
#define POS_INF 100000000000000




Node::Node() 
{ throw GeneralException(); }

Node::Node(const Node &rhs) : 
  number(rhs.number), context(rhs.context), bw(rhs.bw), lat(rhs.lat), table(rhs.table) {}

Node & Node::operator=(const Node &rhs) 
{
  return *(new(this)Node(rhs));
}

void Node::SetNumber(const unsigned n) 
{ number=n;}

unsigned Node::GetNumber() const 
{ return number;}

SimulationContext *Node::GetContext() const{
  return context;
}

void Node::SetLatency(const double l)
{ lat=l;}

double Node::GetLatency() const 
{ return lat;}

void Node::SetBW(const double b)
{ bw=b;}

double Node::GetBW() const 
{ return bw;}

Table* Node::GetTable() const{
  return table;
}

Node::~Node()
{}

// Implement these functions  to post an event to the event queue in the event simulator
// so that the corresponding node can recieve the ROUTING_MESSAGE_ARRIVAL event at the proper time
void Node::SendToNeighbors(const RoutingMessage *m)
{
  return context->SendToNeighbors(this, m);
}

void Node::SendToNeighbor(const Node *n, const RoutingMessage *m)
{
  return context->SendToNeighbor(this, n, m);
}

deque<Node*> *Node::GetNeighbors() const
{
  return context->GetNeighbors(this);
}

deque<Link*> *Node::GetOutgoingLinks()
{
  return context->GetOutgoingLinks(this);
}
void Node::SetTimeOut(const double timefromnow)
{
  context->TimeOut(this,timefromnow);
}


bool Node::Matches(const Node &rhs) const
{
  return number==rhs.number;
}


#if defined(GENERIC)
void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this << " got a link update: "<<*l<<endl;
  //Do Something generic:
  SendToNeighbors(new RoutingMessage);
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " got a routing messagee: "<<*m<<" Ignored "<<endl;
}


void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination) const
{
  return 0;
}

Table *Node::GetRoutingTable() const
{
  return new Table;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}

#endif

#if defined(LINKSTATE)


Node::Node(const unsigned n, SimulationContext *c, double b, double l) : 
    number(n), context(c), bw(b), lat(l){       
      if(b!=0||l!=0){
      table = new Table(n);
      table->Print(cout);
      //cout<<"**********Node********"<<n<<"***********Added**********"<<endl;
      cout<<endl;
      } 
    }


void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this<<": Link Update: "<<*l<<endl;
  //Get link information
  unsigned source = l->GetSrc();
  unsigned dest = l->GetDest();
  double lat = l->GetLatency();
  unsigned tmv = table->LinkUpdate(l);
  const RoutingMessage *msend = new RoutingMessage(source, dest, lat, tmv);
  SendToNeighbors(msend);
  //table->Dijkstra();
  table->Print(cout);
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " Routing Message: "<<*m <<endl;
  
  bool changed = table->AdjmatUpdate(m);
  table->Print(cout);
  //table->Dijkstra();
  if (changed){
    SendToNeighbors(m);
  }
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination)
{
  // WRITE
  table->Dijkstra();

  unsigned src = GetNumber();
  unsigned cur_dst = destination->GetNumber();
  if (cur_dst == src)
  {
    return this;
  }

  while (1)
  {
    NodeInfo *cur_ni = table->FindNodeInfo(src, cur_dst);
    if (cur_ni->GetPrev() == src)
    {
      return new Node(cur_dst, context, bw, cur_ni->GetLsValue());
    }
    cur_dst = cur_ni->GetPrev();
  }
}

Table *Node::GetRoutingTable() const
{
  // WRITE
  return table;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;

}
#endif

//------------------------------------------------------------------------------------------------------------------------------
#if defined(DISTANCEVECTOR)
Node::Node(const unsigned n, SimulationContext *c, double b, double l) : 
    number(n), context(c), bw(b), lat(l)
    {
      if(b!=0||l!=0){
      table = new Table(this);
      table->Print(cout);
      cout<<"-------Node"<<n<<" Added------"<<endl;
      cout<<endl;
      }
    }

void Node::LinkHasBeenUpdated(const Link *l)
{
  // update our table
  // send out routing mesages
  cout<<"======================="<<endl;
  cerr << *this<<": Link Update: "<<*l<<endl;

  cout<<"-----------------------"<<endl;
  cout<<"Old Table"<<endl;
  table->Print(cout);

  //Get link information
  unsigned src_new=l->GetSrc();
  unsigned dest_new=l->GetDest();
  unsigned lat_new=l->GetLatency();

  bool link_changed=false;
  bool link_found=false;
  bool link_added=false;
  bool dv_changed=false;

  //-------Update outgoing
  //Find the old link
  if(table->GetOutgoing()->find(dest_new) != table->GetOutgoing()->end()){\
    link_found=true;
    if(table->GetOutgoing()->find(dest_new)->second != lat_new){
      table->SetOutgoing(dest_new, lat_new);
      link_changed=true;
    }
  }

  if(link_found==false){  //Link not exists
      table->AddOutgoing(dest_new, lat_new);
      link_added=true;
  }
    
  if(link_added==true || link_changed==true){
    if(link_added==true){
      //-------Update dvTable
      Path *p = table->FindPath(dest_new);
      if (p->GetDest()==-1 && p->GetNext()==-1 && p->GetDvValue()==-1){ //dest_new is not in dvTable
          table->AddDv(dest_new);
      }
      //-----Update dvTable_nb
      deque<Node*>* neighbors = GetNeighbors();
      for(deque<Node*>::iterator i=neighbors->begin();i !=neighbors->end();++i){        
        if((*i)->GetNumber()==dest_new){
          table->AddNeighbor(dest_new, (*i)->table->GetDvTable());
        }
      }
    }

    //Bellman-Ford 
    vector<Path>* dvt=table->GetDvTable();
    map<unsigned, vector<Path> >* dvt_nb=table->GetDvTable_nb();
    
    for(vector<Path>::iterator i=dvt->begin();i != dvt->end();++i){
      if(i==dvt->begin()){
        continue;
      }
      double min=POS_INF;
      unsigned next=-1;
      for(map<unsigned, vector<Path> >::iterator j=dvt_nb->begin();j !=dvt_nb->end();++j){
        Path* pt=table->FindNeighborPath(j->first, i->GetDest());
        if(pt!=NULL){     
          double temp=pt->GetDvValue()+table->GetOutgoing()->find(j->first)->second;
          if(min>temp){
            min=temp;
            next=j->first;
          }
        }
      }
      if(min !=i->GetDvValue()&&min!=POS_INF){
        table->SetPath(i->GetDest(), next, min);
        dv_changed=true;
      }
    } 

  cout<<"-----------------------"<<endl;

    cout<<"New Table"<<endl;
    table->Print(cout);

    //Send RoutingMessage
    if(dv_changed==true){
    SendToNeighbors(new RoutingMessage(GetNumber(), table->GetDvTable()));
    RoutingMessage* m=new RoutingMessage(GetNumber(), table->GetDvTable());
    
    cout<<"-----------------------"<<endl;
    cout<<"Send routing message:"<<endl;
    m->Print(cout);
    
    }
  }

  
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  //Extract information from routing message
  
  cout<<"======================="<<endl;

  cout<<"Recieve routing message:"<<endl;
  m->Print(cout);

  cout<<"-----------------------"<<endl;
  cout<<"Old Table"<<endl;
  table->Print(cout);

  unsigned nn = m->nodenum;
  vector<Path>* new_dvTable= m->dvTable;
  bool dv_changed=false;
  bool dvnb_changed=false;

  //Update dvTable_nb
  if(table->GetDvTable_nb()->find(nn) != table->GetDvTable_nb()->end()){
    table->SetDvTable_nb(nn, new_dvTable);
    dvnb_changed=true;
  }

  //Update dvTable
  for(vector<Path>::iterator i=new_dvTable->begin(); i !=new_dvTable->end();++i){
    Path *p = table->FindPath(i->GetDest());
    if (p->GetDest()==-1 && p->GetNext()==-1 && p->GetDvValue()==-1){ //dest_new is not in dvTable
      table->AddDv(i->GetDest());
    }
  }
  

  vector<Path>* dvt=table->GetDvTable();
  map<unsigned, vector<Path> >* dvt_nb=table->GetDvTable_nb();
  
  //Bellman-Ford
  if(dvnb_changed==true){
    for(vector<Path>::iterator i=dvt->begin();i != dvt->end();++i){
      double min=POS_INF;
    unsigned next=-1;
      if(i==dvt->begin()){
        continue;
      }
      for(map<unsigned, vector<Path> >::iterator j=dvt_nb->begin();j !=dvt_nb->end();++j){
        Path* pt=table->FindNeighborPath(j->first, i->GetDest());
        if(pt!=NULL){     
          double temp=pt->GetDvValue()+table->GetOutgoing()->find(j->first)->second;
          if(min>temp){
            min=temp;
            next=j->first;
          }
        }
      }
      if(min !=i->GetDvValue()&&min!=POS_INF){
        table->SetPath(i->GetDest(), next, min);
        dv_changed=true;
      }
    } 
  }

  cout<<"-----------------------"<<endl;
  cout<<"New Table"<<endl;
  table->Print(cout);

  //Send RoutingMessage
  if(dv_changed==true){
    SendToNeighbors(new RoutingMessage(GetNumber(), table->GetDvTable()));
  }
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}


Node *Node::GetNextHop(const Node *destination){

  Path *p = table->FindPath(destination->GetNumber());
  return new Node(p->GetNext(), context, bw, p->GetDvValue());
  
}

Table *Node::GetRoutingTable() const{
    table->Print(cerr);
    return table;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw;
  return os;
}
#endif
