#pragma once
#include "DeviceProfile.hpp"
#include "GameState.hpp"
#include "platform/IPeerTransport.hpp"
#include <deque>
namespace mtg {
struct LobbyInfo {std::string id,name,endpoint;GameFormat format=GameFormat::Commander;unsigned capacity=4,joined=1;bool started=false;std::uint64_t seen=0;};
struct NetworkPlayer {std::string id,name,endpoint;unsigned seat=1;std::int64_t life=40;std::array<std::int64_t,counterCount> counters{};std::uint64_t ack=0,seen=0;bool online=true;};
class Multiplayer {
public:
 Multiplayer(GameState& game,DeviceProfile& profile,IPeerTransport& transport):game_(game),profile_(profile),transport_(transport){}
 bool host(GameFormat format,unsigned seats);void discover();bool join(std::size_t index);bool start();bool adjustOpenSeat(unsigned seat,std::int64_t delta);void leave();void tick(std::uint64_t now);
 bool attached()const{return !gameId_.empty();}bool hosting()const{return hosting_;}bool connected()const{return connected_;}bool started()const{return started_;}
 const std::vector<LobbyInfo>& lobbies()const{return lobbies_;}std::vector<NetworkPlayer> players()const;
 std::vector<NetworkPlayer> opponents()const;const std::string& error()const{return error_;}const std::string& hostName()const{return hostName_;}
 std::uint64_t revision()const{return revision_;}unsigned capacity()const{return capacity_;}
private:
 struct Command{std::uint64_t seq;std::int64_t life;std::array<std::int64_t,counterCount> counters;};
 void publish();void broadcast();void process(const PeerPacket& packet);void reconcile();bool apply(NetworkPlayer& player,const Command& command);void rememberLocal();
 GameState& game_;DeviceProfile& profile_;IPeerTransport& transport_;std::vector<LobbyInfo> lobbies_;std::vector<NetworkPlayer> players_;std::deque<Command> pending_;
 std::string gameId_,hostEndpoint_,hostName_,error_;GameFormat format_=GameFormat::Commander;unsigned capacity_=4;bool hosting_=false,connected_=false,started_=false,scanning_=false;
 std::uint64_t now_=0,lastSend_=0,lastDiscover_=0,hostSeen_=0,revision_=0,sequence_=0,stateSequence_=0,receivedState_=0;
 std::int64_t localLife_=40;std::array<std::int64_t,counterCount> localCounters_{};
};
}
