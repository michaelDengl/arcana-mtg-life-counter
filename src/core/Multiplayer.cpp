#include "Multiplayer.hpp"
#include "third_party/json.hpp"
#include <algorithm>
#include <limits>
#include <random>
namespace mtg {
namespace {
using Json=nlohmann::json;
bool add(std::int64_t a,std::int64_t b,std::int64_t& out){if((b>0&&a>INT64_MAX-b)||(b<0&&a<INT64_MIN-b))return false;out=a+b;return true;}
std::int64_t number(const Json& value){if(!value.is_number_integer()||(value.is_number_unsigned()&&value.get<std::uint64_t>()>INT64_MAX))throw 1;return value.get<std::int64_t>();}
bool identity(const std::string& id){return id.size()==32&&id.find_first_not_of("0123456789abcdef")==std::string::npos;}
}
void Multiplayer::rememberLocal(){localLife_=game_.player().life;localCounters_=game_.player().counters;}
std::vector<NetworkPlayer> Multiplayer::players()const{std::vector<NetworkPlayer> result;for(const auto& p:players_)if(!p.id.empty())result.push_back(p);return result;}
std::vector<NetworkPlayer> Multiplayer::opponents()const{
 std::vector<NetworkPlayer> result;for(const auto& p:players_)if(p.id!=profile_.id()){auto copy=p;if(copy.id.empty())copy.name="P"+std::to_string(result.size()+2);result.push_back(copy);}return result;
}

bool Multiplayer::host(GameFormat format,unsigned seats){
 if(profile_.name().empty()||!transport_.error().empty()){error_=profile_.name().empty()?"Set your name first":transport_.error();return false;}
 GameState validation;if(!validation.startNewGame(seats,format)){error_="Invalid game setup";return false;}
 leave();game_.startNewGame(seats,format);std::random_device random;gameId_=profile_.id()+"-"+std::to_string(random());hostName_=profile_.name();format_=format;capacity_=seats;
 hosting_=connected_=true;started_=scanning_=false;sequence_=stateSequence_=receivedState_=0;error_.clear();players_.clear();pending_.clear();
 for(unsigned seat=1;seat<=seats;++seat)players_.push_back({"","P"+std::to_string(seat),"",seat,startingLife(format),{},0,now_,false});players_[0]={profile_.id(),profile_.name(),"",1,startingLife(format),{},0,now_,true};rememberLocal();++revision_;return true;
}
void Multiplayer::discover(){scanning_=true;error_.clear();transport_.send("discovery",Json{{"v",1},{"t","find"}}.dump());lastDiscover_=now_;}
bool Multiplayer::join(std::size_t index){
 if(index>=lobbies_.size()||profile_.name().empty())return false;auto lobby=lobbies_[index];leave();
 gameId_=lobby.id;hostEndpoint_=lobby.endpoint;hostName_=lobby.name;format_=lobby.format;capacity_=lobby.capacity;hosting_=connected_=started_=scanning_=false;receivedState_=sequence_=0;pending_.clear();players_.clear();hostSeen_=now_;error_="Connecting...";
 transport_.send(hostEndpoint_,Json{{"v",1},{"t","join"},{"game",gameId_},{"id",profile_.id()},{"name",profile_.name()}}.dump());++revision_;return true;
}
bool Multiplayer::start(){if(!hosting_){error_="Only the host can start";return false;}started_=true;error_.clear();++revision_;broadcast();return true;}
bool Multiplayer::adjustOpenSeat(unsigned seat,std::int64_t delta){
 if(!hosting_||!started_||seat<1||seat>players_.size()||!players_[seat-1].id.empty()||!delta)return false;
 publish();Command command{};command.seq=players_[seat-1].ack+1;command.life=delta;
 if(!apply(players_[seat-1],command))return false;reconcile();broadcast();return true;
}

void Multiplayer::leave(){
 if(!gameId_.empty()){
  Json message{{"v",1},{"t",hosting_?"closed":"leave"},{"game",gameId_},{"id",profile_.id()}};
  if(hosting_){for(const auto& p:players_)if(!p.endpoint.empty())transport_.send(p.endpoint,message.dump());}
  else if(!hostEndpoint_.empty())transport_.send(hostEndpoint_,message.dump());
 }
 gameId_.clear();players_.clear();pending_.clear();hosting_=connected_=started_=scanning_=false;hostEndpoint_.clear();error_.clear();++revision_;
}
bool Multiplayer::apply(NetworkPlayer& player,const Command& command){
 if(command.seq!=player.ack+1)return false;
 if(command.life>1000000||command.life< -1000000)return false;
 for(auto delta:command.counters)if(delta>1000000||delta< -1000000)return false;
 auto changed=players_;for(auto& p:changed){bool shared=format_==GameFormat::TwoHeadedGiant&&(p.seat-1)/2==(player.seat-1)/2;
  if(p.seat==player.seat||shared){if(!add(p.life,command.life,p.life))return false;}
  for(unsigned c=0;c<counterCount;++c)if(p.seat==player.seat||(shared&&c==0)){if(!add(p.counters[c],command.counters[c],p.counters[c])||p.counters[c]<0)return false;}
  if(p.seat==player.seat){p.ack=command.seq;p.seen=now_;p.online=!p.id.empty();}
 }
 players_=std::move(changed);++revision_;return true;
}
void Multiplayer::publish(){
 if(!connected_||!started_)return;
 auto life=game_.player().life;auto counters=game_.player().counters;
 if(life==localLife_&&counters==localCounters_)return;
 Command command{};command.seq=sequence_+1;
 // The transport bounds one edit; large local changes stay visible as an error rather than overflowing.
 auto difference=[](std::int64_t current,std::int64_t before,std::int64_t& result){if((before<0&&current>INT64_MAX+before)||(before>0&&current<INT64_MIN+before))return false;result=current-before;return result>=-1000000&&result<=1000000;};
 if(!difference(life,localLife_,command.life)){error_="Network edit too large";return;}
 for(unsigned c=0;c<counterCount;++c)if(!difference(counters[c],localCounters_[c],command.counters[c])){error_="Network edit too large";return;}
 if(hosting_){auto it=std::find_if(players_.begin(),players_.end(),[&](const auto& p){return p.id==profile_.id();});if(it==players_.end()||!apply(*it,command)){error_="Network edit rejected";return;}}
 else{
  // Only the front command can be in flight. Merge bursts into one unsent command.
  if(pending_.size()>=2){auto merged=pending_.back();if(!add(merged.life,command.life,merged.life)||merged.life>1000000||merged.life< -1000000){error_="Network edit too large";return;}for(unsigned c=0;c<counterCount;++c)if(!add(merged.counters[c],command.counters[c],merged.counters[c])||merged.counters[c]>1000000||merged.counters[c]< -1000000){error_="Network edit too large";return;}pending_.back()=merged;rememberLocal();return;}
  pending_.push_back(command);
 }
 sequence_=command.seq;rememberLocal();
}
void Multiplayer::reconcile(){
 auto it=std::find_if(players_.begin(),players_.end(),[&](const auto& p){return p.id==profile_.id();});if(it==players_.end())return;
 while(!pending_.empty()&&pending_.front().seq<=it->ack)pending_.pop_front();sequence_=std::max(sequence_,it->ack);
 auto life=it->life;auto counters=it->counters;for(const auto& command:pending_){if(!add(life,command.life,life))return;for(unsigned c=0;c<counterCount;++c)if(!add(counters[c],command.counters[c],counters[c])||counters[c]<0)return;}
 game_.synchronizeLocal(life,counters);rememberLocal();
}
void Multiplayer::broadcast(){
 Json members=Json::array();for(const auto& p:players_)members.push_back({{"id",p.id},{"name",p.name},{"seat",p.seat},{"life",p.life},{"counters",p.counters},{"ack",p.ack},{"online",p.online}});
 auto data=Json{{"v",1},{"t","state"},{"game",gameId_},{"seq",++stateSequence_},{"format",static_cast<unsigned>(format_)},{"capacity",capacity_},{"started",started_},{"members",members}}.dump();
 for(const auto& p:players_)if(!p.endpoint.empty())transport_.send(p.endpoint,data);
}
void Multiplayer::process(const PeerPacket& packet){
 if(packet.data.size()>16000)return;
 try{
  auto j=Json::parse(packet.data);if(j.at("v")!=1)return;auto type=j.at("t").get<std::string>();
  if(type=="find"&&hosting_){transport_.send(packet.endpoint,Json{{"v",1},{"t","advert"},{"game",gameId_},{"name",hostName_},{"format",static_cast<unsigned>(format_)},{"capacity",capacity_},{"joined",players().size()},{"started",started_}}.dump());return;}
  if(type=="advert"&&scanning_){auto id=j.at("game").get<std::string>(),name=j.at("name").get<std::string>();auto format=number(j.at("format")),capacity=number(j.at("capacity")),joined=number(j.at("joined"));if(id.empty()||id.size()>80||!DeviceProfile::validName(name)||format<0||format>=5||capacity<2||capacity>12||joined<1||joined>capacity)return;
   LobbyInfo lobby{id,name,packet.endpoint,static_cast<GameFormat>(format),static_cast<unsigned>(capacity),static_cast<unsigned>(joined),j.at("started").get<bool>(),now_};auto it=std::find_if(lobbies_.begin(),lobbies_.end(),[&](const auto& l){return l.id==id;});if(it==lobbies_.end()){if(lobbies_.size()<24){lobbies_.push_back(lobby);++revision_;}}else{bool changed=it->name!=name||it->joined!=lobby.joined||it->started!=lobby.started;*it=lobby;if(changed)++revision_;}return;
  }
  if(gameId_.empty()||j.at("game")!=gameId_)return;
  if(type=="join"&&hosting_){auto id=j.at("id").get<std::string>(),name=j.at("name").get<std::string>();if(!identity(id)||!DeviceProfile::validName(name)||id==profile_.id())return;
   auto it=std::find_if(players_.begin(),players_.end(),[&](const auto& p){return p.id==id;});
   if(it==players_.end()){
    auto slot=std::find_if(players_.begin(),players_.end(),[](const auto& p){return p.id.empty();});
    if(slot==players_.end()){transport_.send(packet.endpoint,Json{{"v",1},{"t","reject"},{"game",gameId_},{"reason","Game is full"}}.dump());return;}
    // Claim the existing seat; retain its manually tracked/shared life and counters.
    slot->id=id;slot->name=name;slot->endpoint=packet.endpoint;slot->ack=0;slot->seen=now_;slot->online=true;
   }else{it->endpoint=packet.endpoint;it->name=name;it->seen=now_;it->online=true;}
   ++revision_;broadcast();return;
  }
  if(hosting_&&(type=="cmd"||type=="ping"||type=="leave")){
   auto id=j.at("id").get<std::string>();auto it=std::find_if(players_.begin(),players_.end(),[&](const auto& p){return p.id==id&&p.endpoint==packet.endpoint;});if(it==players_.end())return;it->seen=now_;it->online=true;
   if(type=="leave"){if(!started_){it->id.clear();it->name="P"+std::to_string(it->seat);it->endpoint.clear();it->ack=0;it->online=false;}else it->online=false;++revision_;broadcast();return;}
   if(type=="cmd"&&started_){Command command{};auto seq=number(j.at("seq"));if(seq<1)return;command.seq=static_cast<std::uint64_t>(seq);command.life=number(j.at("life"));if(!j.at("counters").is_array()||j.at("counters").size()!=counterCount)return;for(unsigned c=0;c<counterCount;++c)command.counters[c]=number(j.at("counters")[c]);if(command.seq>it->ack)apply(*it,command);reconcile();broadcast();}return;
  }
  if(hosting_||packet.endpoint!=hostEndpoint_)return;
  if(type=="reject"){error_="Game is full";return;}
  if(type=="closed"){error_="Host ended the game";connected_=started_=false;game_.endGame();++revision_;return;}
  if(type!="state")return;
  auto seq=number(j.at("seq")),format=number(j.at("format")),capacity=number(j.at("capacity"));if(seq<1||static_cast<std::uint64_t>(seq)<=receivedState_||format<0||format>=5||capacity<2||capacity>12)return;
  auto incoming=j.at("members");if(!incoming.is_array()||incoming.empty()||incoming.size()>static_cast<unsigned>(capacity))return;
  std::vector<NetworkPlayer> members;
  for(const auto& value:incoming){NetworkPlayer p;p.id=value.at("id").get<std::string>();p.name=value.at("name").get<std::string>();auto seat=number(value.at("seat")),ack=number(value.at("ack"));if((!p.id.empty()&&!identity(p.id))||!DeviceProfile::validName(p.name)||seat<1||seat>capacity||ack<0)return;p.seat=static_cast<unsigned>(seat);p.ack=static_cast<std::uint64_t>(ack);p.life=number(value.at("life"));p.online=value.at("online").get<bool>();if(value.at("counters").size()!=counterCount)return;for(unsigned c=0;c<counterCount;++c){p.counters[c]=number(value.at("counters")[c]);if(p.counters[c]<0)return;}if(std::any_of(members.begin(),members.end(),[&](const auto& other){return (!p.id.empty()&&p.id==other.id)||p.seat==other.seat;}))return;members.push_back(p);}
  if(std::none_of(members.begin(),members.end(),[&](const auto& p){return p.id==profile_.id();}))return;
  for(unsigned seat=1;seat<=static_cast<unsigned>(capacity);++seat)if(std::none_of(members.begin(),members.end(),[&](const auto& p){return p.seat==seat;}))members.push_back({"","P"+std::to_string(seat),"",seat,startingLife(static_cast<GameFormat>(format)),{},0,now_,false});
  std::sort(members.begin(),members.end(),[](const auto& a,const auto& b){return a.seat<b.seat;});
  if(!connected_&&!game_.startNewGame(static_cast<unsigned>(capacity),static_cast<GameFormat>(format)))return;
  connected_=true;format_=static_cast<GameFormat>(format);capacity_=static_cast<unsigned>(capacity);started_=j.at("started").get<bool>();players_=std::move(members);receivedState_=static_cast<std::uint64_t>(seq);hostSeen_=now_;error_.clear();reconcile();++revision_;
 }catch(...){/* Ignore malformed or unrelated LAN traffic. */}
}
void Multiplayer::tick(std::uint64_t now){
 now_=now;publish();
 for(const auto& packet:transport_.receive())process(packet);
 if(scanning_){if(now-lastDiscover_>=1000)discover();auto count=lobbies_.size();lobbies_.erase(std::remove_if(lobbies_.begin(),lobbies_.end(),[&](const auto& l){return now-l.seen>5000;}),lobbies_.end());if(count!=lobbies_.size())++revision_;}
 if(gameId_.empty())return;
 if(hosting_){for(auto& p:players_)if(!p.endpoint.empty()&&p.online&&now-p.seen>6000){p.online=false;++revision_;}if(now-lastSend_>=300){broadcast();lastSend_=now;}}
 else if(now-lastSend_>=300){
  if(!connected_)transport_.send(hostEndpoint_,Json{{"v",1},{"t","join"},{"game",gameId_},{"id",profile_.id()},{"name",profile_.name()}}.dump());
  else if(!pending_.empty()){const auto& command=pending_.front();transport_.send(hostEndpoint_,Json{{"v",1},{"t","cmd"},{"game",gameId_},{"id",profile_.id()},{"seq",command.seq},{"life",command.life},{"counters",command.counters}}.dump());}
  else transport_.send(hostEndpoint_,Json{{"v",1},{"t","ping"},{"game",gameId_},{"id",profile_.id()}}.dump());
  lastSend_=now;
 }
 if(!hosting_&&now-hostSeen_>6000)error_=connected_?"Host unavailable / reconnecting":"No reply from host";
 if(!transport_.error().empty())error_=transport_.error();
}
}
