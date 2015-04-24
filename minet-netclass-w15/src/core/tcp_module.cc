
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include <iostream>
#include <list>

#include "Minet.h"
#include "tcpstate.h"
#include "tcp.h"
#include "sockint.h"

using namespace std;

//seven statuses, used in create packet
static const int F_SYN = 1;
static const int F_ACK = 2;
static const int F_SYNACK = 3;
static const int F_PSHACK = 4;
static const int F_FIN = 5;
static const int F_FINACK = 6;
static const int F_RST = 7;


void handle_mux(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist);
void handle_sock(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist);
int SendData(const MinetHandle &mux, const MinetHandle &sock, ConnectionToStateMapping<TCPState> &theCTSM, Buffer data);
void createPacket(Packet &pack, ConnectionToStateMapping<TCPState> &a_mapping, int size_of_data, int header);
void handle_timeout(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist);

int main(int argc, char * argv[]) {
  MinetHandle mux;
  MinetHandle sock;
    
  ConnectionList<TCPState> clist;

  MinetInit(MINET_TCP_MODULE);
  //mux for passive model, sock for active model
  mux = MinetIsModuleInConfig(MINET_IP_MUX) ?  MinetConnect(MINET_IP_MUX) : MINET_NOHANDLE;  
  sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? MinetAccept(MINET_SOCK_MODULE) : MINET_NOHANDLE;

  if ( (mux == MINET_NOHANDLE) && (MinetIsModuleInConfig(MINET_IP_MUX)) ) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't connect to ip_mux"));
    return -1;
  }

  if ( (sock == MINET_NOHANDLE) && (MinetIsModuleInConfig(MINET_SOCK_MODULE)) ) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock_module"));
    return -1;
  }
  
  cerr << "tcp_module STUB VERSION handling tcp traffic.......\n";

  MinetSendToMonitor(MinetMonitoringEvent("tcp_module STUB VERSION handling tcp traffic........"));

  MinetEvent event;
  double timeout = 1;

  while (MinetGetNextEvent(event, timeout) == 0) 
  {
    if ((event.eventtype == MinetEvent::Dataflow) && (event.direction == MinetEvent::IN)) 
    {
      if (event.handle == mux) 
      {
        // ip packet has arrived!
        handle_mux(mux, sock, clist);
      }
      if (event.handle == sock) 
      {
        // socket request or response has arrived
        handle_sock(mux, sock, clist);
      }
      cout<<endl<<endl;
    }
    if (event.eventtype == MinetEvent::Timeout) 
    {
      // timeout ! probably need to resend some packets
      handle_timeout(mux,sock,clist);
    }
  }

  MinetDeinit();

  return 0;
}

void handle_mux(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist){

  cout << "new ip in_pkt arrived" << endl;
  Packet in_pkt;
  Packet out_pkt;
  bool check_sum;
  TCPHeader tcp_header;
  IPHeader ip_header;
  Connection connection;
        
  unsigned int ack;
  unsigned int seq;
  SockRequestResponse repl, req;
  unsigned short length;
  unsigned char tcp_header_len, ip_header_len,flags;
  unsigned short window_size, urgent_ptr;
        
  Buffer buffer;
        
  MinetReceive(mux, in_pkt);

  // extract tcp header
  in_pkt.ExtractHeaderFromPayload<TCPHeader>(TCPHeader::EstimateTCPHeaderLength(in_pkt));
  tcp_header = in_pkt.FindHeader(Headers::TCPHeader);

  // extract ip header
  ip_header = in_pkt.FindHeader(Headers::IPHeader);

  // extract ip header data 
  ip_header.GetDestIP(connection.src);
  ip_header.GetSourceIP(connection.dest);
  ip_header.GetProtocol(connection.protocol);

  // extract tcp header data
  tcp_header.GetSourcePort(connection.destport);        
  tcp_header.GetDestPort(connection.srcport);
  tcp_header.GetSeqNum(seq);
  tcp_header.GetAckNum(ack);
  tcp_header.GetFlags(flags);
  tcp_header.GetWinSize(window_size);  
  tcp_header.GetUrgentPtr(urgent_ptr);
        
  // checksum ok?
  bool check_sum_ok;
  check_sum_ok = tcp_header.IsCorrectChecksum(in_pkt);
        
  // get data      
  ip_header.GetTotalLength(length);
  ip_header.GetHeaderLength(ip_header_len);
  ip_header_len<<=2;
  
  tcp_header.GetHeaderLen(tcp_header_len);
  tcp_header_len<<=2;
  
  length = length - tcp_header_len - ip_header_len;
  cout<<"ip_header len = "<<length<<endl;
  buffer = in_pkt.GetPayload().ExtractFront(length);

  // erify checksum
  check_sum = tcp_header.IsCorrectChecksum(in_pkt);

  ConnectionList<TCPState>::iterator matched_c = clist.FindMatching(connection);
  printf("seq %d, ack %d\n",seq,ack);

  if (matched_c != clist.end())
  {
    cout<<"connection exists"<<endl;
    cout<<connection<<endl;
    
    ConnectionToStateMapping<TCPState>& connstate = *matched_c;
    unsigned int current_state = (connstate).state.GetState();
    cout<<"state = "<<current_state<<endl;
  
    switch(current_state)
    {
      case CLOSED:
      {
        cout<<"closed"<<endl;
        break;
      }
      case LISTEN:
      {
        cout<<"listen"<<endl;
        if(IS_SYN(flags))
        {
          (*matched_c).state.SetState(SYN_RCVD);
          (*matched_c).state.last_acked = (*matched_c).state.last_sent-1;
          (*matched_c).state.SetLastRecvd(seq+1);
          (*matched_c).bTmrActive = true;
          (*matched_c).timeout=Time() + 80;
          (*matched_c).connection = connection;//update connection
          
          createPacket(out_pkt, *matched_c, 0, 3);//create syn ack in_pkt
          MinetSend(mux, out_pkt);
          sleep(2);
          MinetSend(mux, out_pkt);
          repl.type = STATUS;
          repl.connection = connection;
          repl.error = EOK;
          repl.bytes = 0;
          MinetSend(sock, repl);
          break;
        }
        else if(IS_FIN(flags))
        {
          createPacket(out_pkt, *matched_c, 0, 6);
          MinetSend(mux, out_pkt);
        }
        break;
      }
      case SYN_RCVD:
      {
        cout<<"syn_rcvd"<<endl;
        if(IS_ACK(flags))
        {
          if((connstate).state.GetLastSent()+1 == ack)
          {
            cout<<"established!"<<endl;
            (connstate).state.SetState(ESTABLISHED);
            (connstate).state.SetLastAcked(ack);
            (connstate).state.SetSendRwnd(window_size); 
            repl.type = WRITE;
            repl.connection = connection;
            repl.error = EOK;
            repl.bytes = 0;
            MinetSend(sock, repl);
          }
        }
        break;
      }
      case SYN_SENT:
      {
        cout<<"syn sent"<<endl;
        if(IS_SYN(flags) && IS_ACK(flags))
        {
          (connstate).state.SetLastRecvd(seq+1);
          cout<<"set acked "<<(((connstate).state.SetLastAcked(ack))?"yes":"no")<<endl;
          (connstate).state.SetSendRwnd(window_size);
          cout<<"last ackd"<<(*matched_c).state.GetLastAcked()<<endl;  

          createPacket(out_pkt, *matched_c, 0, 2);//create an ack in_pkt
          MinetSend(mux, out_pkt);
          (connstate).state.SetState(ESTABLISHED);
          SockRequestResponse write (WRITE, connstate.connection, buffer, 0, EOK);
          MinetSend(sock, write);
        }
        else if (IS_SYN(flags))
        {
          (connstate).state.SetLastRecvd(seq);
          createPacket(out_pkt, connstate, 0, 2);//create an ack in_pkt
          MinetSend(mux, out_pkt);
          (connstate).state.SetState(SYN_RCVD);
          (connstate).state.SetLastSent((connstate).state.GetLastSent()+1);
        }
        else if(IS_FIN(flags) && IS_ACK(flags))
        {
          // Update state given FIN received                                    
          (connstate).state.SetLastRecvd(seq+1);
                                        
          // Update seq & ack numbers, tbd how
          createPacket(out_pkt, *matched_c, 0, 6);
          MinetSend(mux, out_pkt);

          // Update state
          (connstate).state.SetState(CLOSE_WAIT);
                                
          // Notify socket of close
          //SocketRequestResponse close_reply(CLOSE, connstate.connection, buffer, 0, EOK);
          repl.connection=req.connection;
          repl.type=STATUS;
          if (matched_c==clist.end())
          {
            repl.error=ENOMATCH;
          }
          else
          {
            repl.error=EOK;
            clist.erase(matched_c);
          }
          MinetSend(sock,repl);
        }
        break;
      }
      case SYN_SENT1:
      {
        cout<<"syn_sent1"<<endl;
        break;
      }
      case ESTABLISHED:
      {
        cout<<"established"<<endl;
        if (IS_PSH(flags))
        {
          // check sequence number and checksum
          printf("seq = %d, last recv = %d, checksum = %d\n",seq,
                 (connstate).state.GetLastRecvd(),check_sum);
          if(seq == (connstate).state.GetLastRecvd() && check_sum)
          {
            cout<<"recv data good"<<endl;
            (connstate).state.SetLastRecvd(seq+length);
            cout<<"set acked "<<(((connstate).state.SetLastAcked(ack))?"yes":"no")<<endl;
            // Make and send an ACK Packet, assuming ordering
            createPacket(out_pkt, *matched_c, 0, 2);//create ack in_pkt
            MinetSend(mux, out_pkt);

            // Write to socket
            SockRequestResponse write (WRITE, connstate.connection, buffer, length, EOK);
            MinetSend(sock,write);
                                
          }
          else 
          {
            // Unexpected seq: create ACK Packet
            createPacket(out_pkt, *matched_c, 0, 2);
            MinetSend(mux,out_pkt);
          }
        }
        else if (IS_FIN(flags))
        {
          // Update state given FIN received                                    
          (connstate).state.SetLastRecvd(seq+1);
                                        
          // Update seq & ack numbers, tbd how
          createPacket(out_pkt, *matched_c, 0, 6);//create fin ack in_pkt
          MinetSend(mux, out_pkt);

          // Update state
          (connstate).state.SetState(LAST_ACK);
        }
        else if (IS_ACK(flags))
        {
          cout<<"established only ack ";
          // For this state transition, we receive an ACK but send nothing, though resend may occur on timeout
          if (ack == (connstate).state.GetLastAcked())
          {
            if ((connstate).state.SendBuffer.GetSize() == 0)
            {
              matched_c->state.SetTimerTries(0);
            }
            else
            {
              //matched_c->state.SetTimerTries(80 + Time());  
              matched_c->state.tmrTries--;
            }                                   
          }
          else
          { //update tcp state
            (*matched_c).state.SetLastRecvd((unsigned int)seq+length);
            cout<<"set ack "<<((*matched_c).state.SetLastAcked(ack)?"Yes":"No");
          }
          cout<<endl;
        } 

        // Update state receiver window
        (connstate).state.SetSendRwnd(window_size);                             

        break;  
      }
      case LAST_ACK: //waiting for last ack
      {
        if(IS_ACK(flags))
        {
          cout<<"last ack coming"<<endl;
          repl.connection = connection;
          repl.type = WRITE;
          repl.bytes = 0;
          if (matched_c==clist.end())
          {
            repl.error=ENOMATCH;
          }
          else
          {
            cout<<"conn ok"<<endl;
            repl.error=EOK;
          }
          //MinetSend(sock,repl);
          (*matched_c).state.SetState(CLOSING);
          cout<<"send close to sock"<<endl;
        }
        break;
      }
      default:
      {
        cout<<"default"<<endl;
        break;
      }
    }               
  }
  /*
  else
  { //connection not exist
    cout<<"conn not exist"<<endl;
    if(IS_FIN(flags))
    {
      cout<<connection<<endl;
      TCPState a_new_state(1, CLOSED, 3);
      a_new_state.SetLastRecvd(seq);
      a_new_state.last_acked = (ack-1);
      ConnectionToStateMapping<TCPState> a_new_map(connection, Time(), a_new_state, false);
      createPacket(out_pkt, a_new_map, 0, 6);
      MinetSend(mux, out_pkt);
      cout<<"fin ack sent"<<endl;
    }
    else if(IS_SYN(flags))
    {
      cout<<"syn"<<endl;
    }
  }
  */        
}

void handle_sock(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist)
{
  // define packet and socket
  Packet packet_out;
  Packet packet_ack;
  SockRequestResponse reply;
  SockRequestResponse request;
  Buffer buff;
  unsigned int isn;
  cout<<"handle sock"<<endl;
  //receive sock request here.
  MinetReceive(sock, request);
  cerr << "Received Socket Request:" << request << endl;
  // Check if there is a matching connection in the ConnectionList
  ConnectionList<TCPState>::iterator connls = clist.FindMatching(request.connection);
  //open a new connection
  if(connls == clist.end()){
    cout<<"connection not exists: "<<request.type<<endl;
    switch(request.type){
      case CONNECT:{
        cout<<"Connect event"<<endl;
        isn = 1;
        //next state is SYN_SENT after sending syn packet
        TCPState a_new_state(isn, SYN_SENT, 3);
        ConnectionToStateMapping<TCPState> a_new_map(request.connection, Time(), a_new_state, false);

        //create syn packet
        createPacket(packet_out, a_new_map, 0, 1);
        MinetSend(mux, packet_out);
        sleep(2);
        MinetSend(mux, packet_out);
        a_new_map.state.SetLastSent(a_new_map.state.GetLastSent()+1);
        clist.push_back(a_new_map);
        reply.type = STATUS;
        reply.connection = request.connection;
        reply.bytes = 0;
        reply.error = EOK;
        MinetSend(sock, reply); 
      }
      break;
      case ACCEPT:{
        cout<<"accept: "<<endl;
        isn = 1;
        TCPState a_new_state(isn, LISTEN, 3);
        ConnectionToStateMapping<TCPState> a_new_map(request.connection, Time(), a_new_state, false);
        a_new_map.bTmrActive = false;
        clist.push_back(a_new_map);
      }
      break;
      case FORWARD:{
        cout<<"forward: "<<endl;
        reply.type = STATUS;
        reply.bytes = 0;
        reply.error = EOK;
        MinetSend(sock, reply);
      }
      break;
      case WRITE:{
        cout<<"write: "<<endl;
        reply.type = STATUS;
        reply.bytes = 0;
        reply.error = EUNKNOWN;
        MinetSend(sock, reply);
      }
      break;
      case CLOSE:{
        cout<<"close: "<<endl;
        reply.type = STATUS;
        reply.bytes = 0;
        reply.error = ENOMATCH;
        MinetSend(sock, reply);
      }
      break;
      case STATUS: {
        cout<<"status: "<<endl;
      }
      break;
      default:
      break;
    }

  }
  //already exists a connection.
  else {
    cout<<"connection exists"<<endl;
    int state = connls->state.GetState();
    Packet out;
    Buffer buff;
    cout<<"state equals"<<state<<endl;
    switch(state){
    case LISTEN:
      createPacket(packet_out, *connls, 0, 1);
      MinetSend(mux, packet_out);
      connls->state.SetState(SYN_SENT);
      connls->timeout.SetToCurrentTime();
      break;
    case SYN_RCVD:
      if(request.type == CLOSE){
        Packet out;
        Buffer buff;
        //create an fin packet
        createPacket(out, *connls, 0, 5);
        MinetSend(mux, out);
        connls->state.SetLastSent(connls->state.GetLastSent() + 1);
        connls->state.SetState(FIN_WAIT1);
      } else {
        cerr << "SYN_RCVD, some error happens." << endl;
      }
      break;
    case SYN_SENT:
      if (request.type == CLOSE){
        clist.erase(connls);
      }
      break;
    case SYN_SENT1:
      cerr << "SYN_SENT1, some error happens." << endl;
      break;    
    case ESTABLISHED:
      switch(request.type){
      case WRITE:{
        cerr << "start writing\n" << endl;
          // If there isn't enough space in the buffer
          if(connls->state.SendBuffer.GetSize()+request.data.GetSize() > connls->state.TCP_BUFFER_SIZE) { 
            reply.type = STATUS;
            reply.connection = request.connection;
            reply.bytes = 0;
            reply.error = EBUF_SPACE;
            MinetSend(sock, reply);
          }
          // If there is enough space in the buffer
          else {
            
          Buffer copy_buffer = request.data; // Dupe the buffer
          
          int return_value = SendData(mux, sock, *connls, copy_buffer);
          
          if (return_value == 0) {
            reply.type = STATUS;
            reply.connection = request.connection;
            reply.bytes = copy_buffer.GetSize();
            reply.error = EOK;
            MinetSend(sock, reply);
          }
          }
        
        cerr << "finish writing \n" << endl;
      }
      break;
      case CLOSE:
        createPacket(out, *connls, 0, 5);//create fin packet
        MinetSend(mux, out);
        connls->state.SetLastSent(connls->state.GetLastSent() + 1);
        connls->state.SetState(FIN_WAIT1);
        break;
      case STATUS:
        cout<<"status: "<<endl;
        
        break;
      default:
        break;
      }
    case SEND_DATA:
      
      break;
    case CLOSE_WAIT:
      if(request.type == CLOSE){
        createPacket(packet_out, *connls, 0, 6);//create fin ack packet
        connls->state.SetState(LAST_ACK);
        connls->state.SetLastSent(connls->state.GetLastSent()+1);
      } else {
        cerr << "Shouldn't get here. CLOSE_WAIT" << endl;
      }
      break;
    case FIN_WAIT1:
      if(request.type == ACCEPT){
        cerr << "Trying to open an existing conneciton." <<endl;
      }
      break;
    case CLOSING:
      cout<<"closing "<<endl;
      /*
      connls->state.SetState(TIME_WAIT);
      connls->state.SetLastSent(connls->state.GetLastSent()+1);*/
      if(request.type == STATUS) {
        cout<<"status "<<endl;
      }
      else if(request.type == CLOSE) {
        cout<<"close "<<endl;
        clist.erase(connls);
        TCPState a_new_state(1, LISTEN, 3);
        ConnectionToStateMapping<TCPState> a_new_map(request.connection, Time(), a_new_state, false);
        a_new_map.bTmrActive = false;
        //clist.push_back(a_new_map);
        
        reply.type = STATUS;
        reply.connection = request.connection;
        reply.bytes = 0;
        reply.error = EOK;
        MinetSend(sock, reply);
        
      }
      break;
    case LAST_ACK:
      
      break;
    case FIN_WAIT2:
      {
        Packet packet_ack;
        createPacket(packet_ack, *connls, 0, 2);
        MinetSend(mux,packet_ack);
        connls->state.SetLastSent(connls->state.GetLastSent()+1);
        connls->state.SetState(TIME_WAIT);
      }
      break;
    case TIME_WAIT:
      break; 
    }
  }

}

void createPacket(Packet &packet, ConnectionToStateMapping<TCPState> &a_mapping, int size_of_data, int header)
{
  cout<<"create new ip packet"<<endl;
  unsigned char flags = 0;

  int packet_length = size_of_data + TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH;
  IPHeader ip;
  TCPHeader tcp;
  IPAddress source = a_mapping.connection.src;
  IPAddress destination = a_mapping.connection.dest;
  unsigned short source_port = a_mapping.connection.srcport;
  unsigned short destination_port = a_mapping.connection.destport;

  //create the IP header
  ip.SetSourceIP(source);
  ip.SetDestIP(destination);
  ip.SetTotalLength(packet_length);
  ip.SetProtocol(IP_PROTO_TCP);

  packet.PushFrontHeader(ip);

  //create the TCP header
  tcp.SetSourcePort(source_port, packet);
  tcp.SetDestPort(destination_port, packet);
  tcp.SetSeqNum(a_mapping.state.GetLastAcked()+1, packet);
  tcp.SetAckNum(a_mapping.state.GetLastRecvd(), packet);
  
  //set the flag
  switch (header){
  case F_SYN:{
    SET_SYN(flags);
    cerr << "HEADER TYPE SYN!" << endl;
  }
  break;

  case F_ACK:{
    SET_ACK(flags);
    cerr << "HEADER TYPE ACK!" << endl;
  }
  break;

  case F_SYNACK:{
    SET_ACK(flags);
    SET_SYN(flags);
    cerr << "HEADER TYPE SYN ACK!" << endl;
  }
  break;

  case F_PSHACK:{
    SET_PSH(flags);
    SET_ACK(flags);
    cerr << "HEADER TYPE PSH ACK!" << endl;
  }
  break;

  case F_FIN:{
    SET_FIN(flags);
    cerr << "HEADER TYPE FIN!" << endl;
  }
  break;

  case F_FINACK:{
    SET_FIN(flags);
    SET_ACK(flags);
    cerr << "HEADER TYPE FIN ACK!" << endl;
  }
  break;

  case F_RST:{
    SET_RST(flags);
    cerr << "HEADER TYPE RST!" << endl;
  }
  break;

  default:
    break;
  }

  tcp.SetFlags(flags, packet);
  tcp.SetWinSize(a_mapping.state.GetN(), packet);
  tcp.SetChecksum(tcp.ComputeChecksum(packet));
  tcp.SetHeaderLen(TCP_HEADER_BASE_LENGTH/4, packet);
  
  //tcp.SetUrgentPtr(0, packet);
  //tcp.RecomputeChecksum(packet);
  packet.PushBackHeader(tcp);
}

int SendData(const MinetHandle &mux, const MinetHandle &sock, ConnectionToStateMapping<TCPState> &a_mapping, Buffer data) {
    cerr << "start Sending Data\n" << endl;
    Packet p;
        a_mapping.state.SendBuffer.AddBack(data);
        unsigned int bytesLeft = data.GetSize();
        while(bytesLeft != 0) {
                unsigned int bytesToSend = min(bytesLeft, TCP_MAXIMUM_SEGMENT_SIZE);
                p = a_mapping.state.SendBuffer.Extract(0, bytesToSend);
                createPacket(p, a_mapping, 4, bytesToSend);
                MinetSend(mux, p);
                
                //a_mapping.state.SetLastSent(a_mapping.state.GetLastSent()+bytesToSend);
                a_mapping.state.last_sent = a_mapping.state.last_sent + bytesToSend;
        
                bytesLeft -= bytesToSend;
        }
  cerr << "finish Sending data\n" << endl;
  return bytesLeft;
}

void handle_timeout(const MinetHandle &mux, const MinetHandle &sock, ConnectionList<TCPState> &clist) {
  Time now;
  std::list<ConnectionList<TCPState>::iterator> dellist;

  for(ConnectionList<TCPState>::iterator cs = clist.begin(); cs!=clist.end(); cs++) {
   
    //did not activate timer 
    if(!cs->bTmrActive)
      continue;
    
    //a timeout occurs
    if(now > cs->timeout || now == cs->timeout) {
      cout<<"time out"<<endl;
      switch (cs->state.GetState()) {
      case SYN_SENT: {//sent out syn waiting for synack
        if(cs->state.ExpireTimerTries()) {//we run out of tries, close the connection
          cout<<"number of tries run out"<<endl<<endl;
          //close the connection
          Buffer buffer;
          SockRequestResponse reply(WRITE, cs->connection, buffer, 0, ECONN_FAILED);//close it
          MinetSend(sock, reply);
          cs->bTmrActive = false;
          cs->state.SetState(CLOSING);
        }
        else {//send syn again
          cout<<"send out syn again"<<endl<<endl;;
          Packet outgoing_packet;
          createPacket(outgoing_packet, *cs, 0, F_SYN);//create syn packet
          MinetSend(mux, outgoing_packet);
          cs->timeout = now+0.2;
        }
      }
        break;
      case SYN_RCVD: {//waiting for last ack coming
        if(cs->state.ExpireTimerTries()) {//we run out of tries, close the connection
          cout<<"number of tries run out"<<endl<<endl;
          //close the connection
          //Buffer buffer;
          //SockRequestResponse reply(WRITE, cs->connection, buffer, 0, ECONN_FAILED);//close it
          //MinetSend(sock, reply);
          cs->bTmrActive = false;
          cs->state.SetState(LISTEN);
        }
        else {
          cout<<"send out syn ack again"<<endl<<endl;
          Packet outgoing_packet;
          createPacket(outgoing_packet, *cs, 0, F_SYNACK);//create syn ack packet
          MinetSend(mux, outgoing_packet);
          cs->timeout = now+0.2;
        }
      }
        break;
      case TIME_WAIT: {
        cout<<"time wait"<<endl;
        //close the connection
        dellist.push_back(cs);
        
      }
        break;
      case ESTABLISHED: {
        //todo
        //we have outstanding acked 
        if(cs->state.last_acked < cs->state.last_sent) {
          //activate timer
          cs->bTmrActive = true;
          cs->timeout = Time()+5;//expire in 5 sec
          unsigned int totalsend = 0;//total data send this time
          Buffer buffer = cs->state.SendBuffer;//copy buffer
          //get rid of data we already send, what left is data we haven't send
          buffer = buffer.ExtractFront(cs->state.last_sent-cs->state.last_acked);  
          unsigned int totaleft = buffer.GetSize();
          Packet outgoing_packet;
          //maximum amount of data we can sent and not cause over flow
          //Note, N is updated to be always <= rwnd
          ///////////unsigned int MAX_SENT = cs->state.GetN()-(cs->state.last_sent-cs->state.last_acked)-TCP_MAXIMUM_SEGMENT_SIZE;
          //while we have data left to send and total data send does not exceed 
          //our window size
          while(totaleft!=0 ){
                //////////////totalsend < MAX_SENT) {
            unsigned int bytes = min(totaleft,TCP_MAXIMUM_SEGMENT_SIZE);//data we send this time
            cout<<totalsend<<", "<<bytes<<endl;
            outgoing_packet = buffer.Extract(0,bytes);
            //creat packet
            createPacket(outgoing_packet, *cs, bytes, F_PSHACK);
            //send ip packet
            MinetSend(mux, outgoing_packet);

            /////////////cs->state.SetLastSent(cs->state.GetLastSent()+bytes);
            /////////////totalsend += bytes;
            //update totaleft
            totaleft -= bytes;
          }
        }
        else {//we dont have outstanding ack
          //turn off timer
          cs->bTmrActive = false;
        }
      }
        break;
      default:
        break;
      }
    }    
  }//end of for loop for each conn

  for(std::list<ConnectionList<TCPState>::iterator>::iterator it = dellist.begin(); it!=dellist.end();it++) {
    clist.erase(*it);
  }
}
