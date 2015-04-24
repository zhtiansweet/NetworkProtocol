# Networking Protocols Implementation
_January 2015 - March 2015_  
_By Tian Zhang, Li Xu and Di Tian_   
_Course: Introduction to Computer Networking, Northwestern University, Evanston, IL_
## Network Layer
Accomplished two IP algorithms (Link State and Distance Vector), designed the data structure of routing table stored in
each node, and wrote code on Dijkstra’s and Bellman-Ford algorithms to find the shortest route.  

Source Code: [node.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/node.cc), [node.h](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/node.h), [table.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/table.cc), [table.h](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/table.h), [messages.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/messages.cc), [messages.h](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/routelab-w15/messages.h)
## Transport Layer
Implemented TCP based on RFC793, achieved both passive and active opens, made actions to socket requests and incoming
packets, handled the timeout event with Go-Back-N mechanism, and improved transfer reliability with flow control.  

Source Code: [tcp_module.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/minet-netclass-w15/src/core/tcp_module.cc)
## Application Layer
Built a HTTP client and two HTTP servers. The advanced server could handle multiple sockets simultaneously.

Source Code: [client.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/minet-netclass-w15/src/apps/http_client.cc), [server1.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/minet-netclass-w15/src/apps/http_server1.cc), [server2.cc](https://github.com/zhtiansweet/NetworkProtocol_EECS340/blob/master/minet-netclass-w15/src/apps/http_server2.cc)
