#include "LanTransport.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
using SOCKET=int;constexpr SOCKET INVALID_SOCKET=-1;
#endif
#include <array>
namespace mtg {
namespace {
const char* multicast="239.255.42.88";
void closeSocket(SOCKET socket){if(socket==INVALID_SOCKET)return;
#ifdef _WIN32
 closesocket(socket);
#else
 close(socket);
#endif
}
void nonblocking(SOCKET socket){
#ifdef _WIN32
 u_long enabled=1;ioctlsocket(socket,FIONBIO,&enabled);
#else
 fcntl(socket,F_SETFL,O_NONBLOCK);
#endif
}
bool endpoint(const std::string& value,sockaddr_in& address){auto colon=value.rfind(':');if(colon==std::string::npos)return false;try{auto port=std::stoul(value.substr(colon+1));if(!port||port>65535)return false;address={};address.sin_family=AF_INET;address.sin_port=htons(static_cast<unsigned short>(port));return inet_pton(AF_INET,value.substr(0,colon).c_str(),&address.sin_addr)==1;}catch(...){return false;}}
}
struct LanTransport::Impl {SOCKET data=INVALID_SOCKET,discovery=INVALID_SOCKET;std::string error;unsigned short port=0,discoveryPort=42688;bool initialized=false;};
LanTransport::LanTransport(unsigned short discoveryPort):impl_(new Impl){
 impl_->discoveryPort=discoveryPort;
#ifdef _WIN32
 WSADATA data;if(WSAStartup(MAKEWORD(2,2),&data)){impl_->error="Network initialization failed";return;}
#endif
 impl_->initialized=true;impl_->data=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);impl_->discovery=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
 if(impl_->data==INVALID_SOCKET||impl_->discovery==INVALID_SOCKET){impl_->error="Cannot create LAN sockets";return;}
 sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_ANY);
 if(bind(impl_->data,reinterpret_cast<sockaddr*>(&address),sizeof(address))){impl_->error="Cannot open LAN endpoint";return;}
#ifdef _WIN32
 int length=sizeof(address);
#else
 socklen_t length=sizeof(address);
#endif
 getsockname(impl_->data,reinterpret_cast<sockaddr*>(&address),&length);impl_->port=ntohs(address.sin_port);
 int reuse=1;setsockopt(impl_->discovery,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));address.sin_port=htons(discoveryPort);
 if(bind(impl_->discovery,reinterpret_cast<sockaddr*>(&address),sizeof(address))){impl_->error="Cannot open LAN discovery";return;}
 ip_mreq membership{};inet_pton(AF_INET,multicast,&membership.imr_multiaddr);membership.imr_interface.s_addr=htonl(INADDR_ANY);
 if(setsockopt(impl_->discovery,IPPROTO_IP,IP_ADD_MEMBERSHIP,reinterpret_cast<const char*>(&membership),sizeof(membership))){impl_->error="LAN discovery unavailable";return;}
 unsigned char ttl=1,loop=1;setsockopt(impl_->data,IPPROTO_IP,IP_MULTICAST_TTL,reinterpret_cast<const char*>(&ttl),sizeof(ttl));setsockopt(impl_->data,IPPROTO_IP,IP_MULTICAST_LOOP,reinterpret_cast<const char*>(&loop),sizeof(loop));nonblocking(impl_->data);nonblocking(impl_->discovery);
}
LanTransport::~LanTransport(){closeSocket(impl_->data);closeSocket(impl_->discovery);
#ifdef _WIN32
 if(impl_->initialized)WSACleanup();
#endif
}
const std::string& LanTransport::error()const{return impl_->error;}
std::string LanTransport::address()const{return "127.0.0.1:"+std::to_string(impl_->port);}
bool LanTransport::send(const std::string& destination,const std::string& data){if(!impl_->error.empty()||data.size()>16000)return false;if(destination=="discovery"){bool result=send(std::string(multicast)+":"+std::to_string(impl_->discoveryPort),data);send("127.0.0.1:"+std::to_string(impl_->discoveryPort),data);return result;}sockaddr_in target{};if(!endpoint(destination,target))return false;return sendto(impl_->data,data.data(),static_cast<int>(data.size()),0,reinterpret_cast<sockaddr*>(&target),sizeof(target))==static_cast<int>(data.size());}
std::vector<PeerPacket> LanTransport::receive(){std::vector<PeerPacket> packets;if(!impl_->error.empty())return packets;std::array<char,16384> buffer{};for(auto socket:{impl_->data,impl_->discovery})for(int i=0;i<32;++i){sockaddr_in source{};
#ifdef _WIN32
 int length=sizeof(source);
#else
 socklen_t length=sizeof(source);
#endif
 int count=recvfrom(socket,buffer.data(),static_cast<int>(buffer.size()),0,reinterpret_cast<sockaddr*>(&source),&length);if(count<0)break;if(count>16000)continue;char ip[INET_ADDRSTRLEN]{};inet_ntop(AF_INET,&source.sin_addr,ip,sizeof(ip));packets.push_back({std::string(ip)+":"+std::to_string(ntohs(source.sin_port)),std::string(buffer.data(),count)});}return packets;}
}
