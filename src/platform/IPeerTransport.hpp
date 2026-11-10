#pragma once
#include <string>
#include <vector>
namespace mtg {
struct PeerPacket {std::string endpoint,data;};
class IPeerTransport {
public:virtual ~IPeerTransport()=default;virtual bool send(const std::string& endpoint,const std::string& data)=0;virtual std::vector<PeerPacket> receive()=0;virtual const std::string& error()const=0;
};
}
