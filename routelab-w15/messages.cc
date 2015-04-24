#include "messages.h"
using namespace std;


#if defined(GENERIC)
ostream &RoutingMessage::Print(ostream &os) const
{
  os << "RoutingMessage()";
  return os;
}
#endif


#if defined(LINKSTATE)

ostream &RoutingMessage::Print(ostream &os) const
{
  return os;
}

RoutingMessage::RoutingMessage()
{}

RoutingMessage::RoutingMessage(const unsigned s, const unsigned d, const double l, const unsigned t):
source(s), dest(d), lat(l), tmvalue(t){}

RoutingMessage::RoutingMessage(const RoutingMessage &rhs):
source(rhs.source), dest(rhs.dest), lat(rhs.lat), tmvalue(rhs.tmvalue){}

#endif


#if defined(DISTANCEVECTOR)

ostream &RoutingMessage::Print(ostream &os) const
{
  os<<"Node: "<<nodenum<<endl;
  os<<"Distance Vectors: ";
	for(vector<Path>::const_iterator i=dvTable->begin();
		i !=dvTable->end(); ++i){
		os<<"To "<<i->GetDest()<<"(by "<<i->GetNext()<<"): "<<i->GetDvValue()<<", ";
	}
	os<<endl;
  os<<endl;
  return os;
}

RoutingMessage::RoutingMessage()
{}

RoutingMessage::RoutingMessage(const unsigned nn, vector<Path>* dvt):
nodenum(nn), dvTable(dvt){
}

RoutingMessage::RoutingMessage(const RoutingMessage &rhs):
nodenum(rhs.nodenum), dvTable(rhs.dvTable){}

#endif

