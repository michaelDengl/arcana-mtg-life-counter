#include "GameState.hpp"
#include "third_party/json.hpp"
#include <limits>
#include <set>
#include <stdexcept>
namespace mtg {
namespace {
using Json=nlohmann::json;
std::int64_t integer(const Json& j) {
 if(!j.is_number_integer() || (j.is_number_unsigned() && j.get<std::uint64_t>()>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())))throw std::runtime_error("Invalid integer");
 return j.get<std::int64_t>();
}
std::string name(const Json& j) {
 auto s=j.get<std::string>();if(s.empty()||s.size()>24||s.find_first_not_of(' ')==std::string::npos)throw std::runtime_error("Invalid name");return s;
}
Json encode(const GameSnapshot& s) {
 const auto& p=s.player;
 Json j={{"format",static_cast<unsigned>(s.format)},{"opponent_life",s.opponentLife},{"teams",s.teams},{"player_count",s.playerCount},{"active",s.active},{"shortcuts",{static_cast<unsigned>(s.shortcuts[0]),static_cast<unsigned>(s.shortcuts[1])}},{"poison_limit",s.poisonLimit},{"day_night",static_cast<int>(s.dayNight)},{"next_source_id",s.nextSourceId},
 {"player",{{"id",p.id},{"name",p.name},{"life",p.life},{"counters",p.counters},{"enabled",p.enabled},{"custom_name",p.customName},{"monarch",p.monarch},{"initiative",p.initiative},{"city_blessing",p.cityBlessing}}}};
 j["sources"]=Json::array();for(const auto& c:s.sources)j["sources"].push_back({{"id",c.id},{"owner",c.owner},{"name",c.name},{"damage",c.damage},{"linked_life",c.lifeLinkedDamage},{"recipient",c.recipient}});
 j["commanders"]=Json::array();for(const auto& c:s.commanders)j["commanders"].push_back({{"name",c.name},{"casts",c.casts}});
 return j;
}
GameSnapshot decode(const Json& j,unsigned version) {
 GameSnapshot s;auto count=integer(j.at("player_count"));if(count<1||count>12)throw std::runtime_error("Invalid players");s.playerCount=static_cast<unsigned>(count);
 if(version>=4){
  auto format=integer(j.at("format"));if(format<0||format>=static_cast<int>(GameFormat::Count))throw std::runtime_error("Invalid format");s.format=static_cast<GameFormat>(format);
  if((s.format==GameFormat::TwoHeadedGiant&&(count<4||count%2))||((s.format==GameFormat::Brawl||s.format==GameFormat::Limited)&&count!=2))throw std::runtime_error("Invalid format seats");
  unsigned storedSeats=version>=5?12:6;
  if(!j.at("opponent_life").is_array()||j.at("opponent_life").size()!=storedSeats-1||!j.at("teams").is_array()||j.at("teams").size()!=storedSeats)throw std::runtime_error("Invalid teams");
  for(unsigned i=0;i<storedSeats-1;++i)s.opponentLife[i]=integer(j.at("opponent_life")[i]);
  for(unsigned i=0;i<s.teams.size();++i)s.teams[i]=s.format==GameFormat::TwoHeadedGiant?i/2:i;
  for(unsigned i=0;i<storedSeats;++i){auto team=integer(j.at("teams")[i]);unsigned expected=s.format==GameFormat::TwoHeadedGiant?i/2:i;if(team!=expected)throw std::runtime_error("Invalid team mapping");s.teams[i]=static_cast<unsigned>(team);}
 }
 s.active=j.at("active").get<bool>();s.poisonLimit=integer(j.at("poison_limit"));
 auto day=integer(j.at("day_night"));auto next=integer(j.at("next_source_id"));
 if(s.poisonLimit<1||day<0||day>2||next<1||next>std::numeric_limits<std::uint32_t>::max())throw std::runtime_error("Invalid settings");
 s.dayNight=static_cast<DayNight>(day);s.nextSourceId=static_cast<std::uint32_t>(next);
 const auto& p=j.at("player");if(integer(p.at("id"))!=1)throw std::runtime_error("Local player must be P1");
 s.player.name=name(p.at("name"));s.player.life=integer(p.at("life"));s.player.customName=name(p.at("custom_name"));
 const unsigned storedCounters=version==1?6:version==2?7:counterCount;
 if(!p.at("counters").is_array()||p.at("counters").size()!=storedCounters||!p.at("enabled").is_array()||p.at("enabled").size()!=storedCounters)throw std::runtime_error("Invalid counters");
 for(unsigned i=0;i<storedCounters;++i){s.player.counters[i]=integer(p.at("counters").at(i));s.player.enabled[i]=p.at("enabled").at(i).get<bool>();if(s.player.counters[i]<0)throw std::runtime_error("Negative counter");}
 if(version>=3) {
  if(!j.at("shortcuts").is_array()||j.at("shortcuts").size()!=2)throw std::runtime_error("Invalid shortcuts");
  for(unsigned i=0;i<2;++i){auto c=integer(j.at("shortcuts").at(i));if(c<0||c>=counterCount||c==static_cast<int>(Counter::Custom))throw std::runtime_error("Invalid shortcut");s.shortcuts[i]=static_cast<Counter>(c);}
 }
 if(!s.player.enabled[0])throw std::runtime_error("Poison must be enabled");
 s.player.monarch=p.at("monarch").get<bool>();s.player.initiative=p.at("initiative").get<bool>();s.player.cityBlessing=p.at("city_blessing").get<bool>();
 if(!j.at("sources").is_array()||!j.at("commanders").is_array()||j.at("commanders").empty())throw std::runtime_error("Invalid commanders");
 s.sources.clear();std::set<std::uint32_t> ids;
 for(const auto& c:j.at("sources")) {
  auto id=integer(c.at("id")),owner=integer(c.at("owner")),damage=integer(c.at("damage")),linked=integer(c.at("linked_life"));
  if(id<1||id>=next||owner<1||owner>count||damage<0||linked<0||linked>damage||!ids.insert(static_cast<std::uint32_t>(id)).second)throw std::runtime_error("Invalid source");
  auto recipient=version>=4?integer(c.at("recipient")):1;
  if(recipient<1||recipient>(s.format==GameFormat::TwoHeadedGiant?2:1)||(s.format==GameFormat::TwoHeadedGiant&&owner<3))throw std::runtime_error("Invalid commander recipient");
  s.sources.push_back({static_cast<std::uint32_t>(id),static_cast<unsigned>(owner),name(c.at("name")),damage,linked,static_cast<unsigned>(recipient)});
 }
 s.commanders.clear();for(const auto& c:j.at("commanders")) {
  auto casts=integer(c.at("casts"));if(casts<0||casts>std::numeric_limits<std::int64_t>::max()/2)throw std::runtime_error("Invalid casts");
  s.commanders.push_back({name(c.at("name")),casts});
 }
 if(s.format!=GameFormat::TwoHeadedGiant&&count>6)throw std::runtime_error("Invalid format seats");
 if(version<5&&count>6)throw std::runtime_error("Invalid legacy seats");
 if(s.format==GameFormat::TwoHeadedGiant){
  if(s.opponentLife[0]!=s.player.life)throw std::runtime_error("Mismatched local team life");
  for(unsigned seat=3;seat<s.playerCount;seat+=2)if(s.opponentLife[seat-2]!=s.opponentLife[seat-1])throw std::runtime_error("Mismatched team life");
 }
 if(!usesCommanderDamage(s.format)&&!s.sources.empty())throw std::runtime_error("Commander damage disabled");
 return s;
}
}
std::string GameState::serialize() const {
 Json j={{"schema_version",5},{"state",encode(state_)},{"cursor",cursor_},{"history",Json::array()}};
 for(const auto& h:history_)j["history"].push_back({{"before",encode(h.before)},{"after",encode(h.after)},{"description",h.description}});
 return j.dump(2);
}
bool GameState::deserialize(const std::string& data) {
 try {
  if(data.size()>4*1024*1024)return false;
  auto j=Json::parse(data);auto version=integer(j.at("schema_version"));if(version!=1&&version!=2&&version!=3&&version!=4&&version!=5)return false;
  auto state=decode(j.at("state"),static_cast<unsigned>(version));auto cursor=integer(j.at("cursor"));const auto& entries=j.at("history");
  if(!entries.is_array()||entries.size()>64||cursor<0||static_cast<std::size_t>(cursor)>entries.size())return false;
  std::vector<HistoryEntry> history;
  for(const auto& e:entries) {
   auto description=e.at("description").get<std::string>();if(description.size()>256)return false;
   history.push_back({decode(e.at("before"),static_cast<unsigned>(version)),decode(e.at("after"),static_cast<unsigned>(version)),std::move(description)});
   if(history.size()>1 && encode(history[history.size()-2].after)!=encode(history.back().before))return false;
  }
  if(!history.empty() && encode(state)!=encode(cursor?history[static_cast<std::size_t>(cursor)-1].after:history.front().before))return false;
  recent_.clear();actionTime_.reset();state_=std::move(state);history_=std::move(history);cursor_=static_cast<std::size_t>(cursor);++revision_;return true;
 }catch(const std::exception&){return false;}
}
}
