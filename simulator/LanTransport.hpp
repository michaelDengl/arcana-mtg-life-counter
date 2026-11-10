#pragma once
#include "platform/IPeerTransport.hpp"
#include <memory>
namespace mtg {
class LanTransport final:public IPeerTransport {
public:explicit LanTransport(unsigned short discoveryPort=42688);~LanTransport();bool send(const std::string& endpoint,const std::string& data)override;std::vector<PeerPacket> receive()override;const std::string& error()const override;std::string address()const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
