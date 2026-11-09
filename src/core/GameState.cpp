#include "GameState.hpp"
#include <algorithm>
#include <limits>
#include <utility>
namespace mtg {
namespace {
bool sum(std::int64_t a, std::int64_t b, std::int64_t& result) {
 if ((b > 0 && a > std::numeric_limits<std::int64_t>::max()-b) ||
     (b < 0 && a < std::numeric_limits<std::int64_t>::min()-b)) return false;
 result = a+b; return true;
}
bool validName(const std::string& name) {
 return !name.empty() && name.size() <= 24 && name.find_first_not_of(' ') != std::string::npos;
}
std::string change(std::int64_t delta) { return (delta > 0 ? "+" : "") + std::to_string(delta); }
}
const char* counterName(Counter counter) {
 static const char* names[]{"POISON", "ENERGY", "EXPERIENCE", "RAD", "STORM", "CUSTOM", "TREASURES", "SHIELD"};
 const auto i=static_cast<unsigned>(counter); return i<counterCount ? names[i] : "COUNTER";
}
bool GameState::commit(GameSnapshot next, std::string description) {
 if(actionTime_) {
  while(!recent_.empty() && (*actionTime_<recent_.front().time || *actionTime_-recent_.front().time>5000))recent_.pop_front();
  recent_.push_back({state_,*actionTime_});
 }
 // Bounded history: snapshots keep multi-field changes atomic, including linked life loss.
 history_.erase(history_.begin()+cursor_,history_.end());
 if(history_.size()==64) history_.erase(history_.begin());
 history_.push_back({state_,next,std::move(description)});
 state_=std::move(next); cursor_=history_.size(); ++revision_; return true;
}
bool GameState::startNewGame(unsigned players) {
 if(players<1 || players>6)return false;
 GameSnapshot next;next.playerCount=players;next.active=true;next.sources.clear();next.nextSourceId=1;
 for(unsigned owner=2;owner<=players;++owner)
  next.sources.push_back({next.nextSourceId++,owner,"P"+std::to_string(owner)+" / COMMANDER A",0,0});
 state_=std::move(next);history_.clear();recent_.clear();cursor_=0;++revision_;return true;
}
void GameState::endGame() {
 if(!state_.active)return;
 state_.active=false;history_.clear();recent_.clear();cursor_=0;++revision_;
}
bool GameState::adjustCommanderDamage(std::uint32_t id,std::int64_t delta,bool subtractLife) {
 if(delta>0)return applyCommanderDamage(id,delta,subtractLife);
 if(delta==0 || delta==std::numeric_limits<std::int64_t>::min())return false;
 auto next=state_;auto it=std::find_if(next.sources.begin(),next.sources.end(),[id](const auto& s){return s.id==id;});
 const auto amount=-delta;
 if(it==next.sources.end() || amount>it->damage)return false;
 // Remove record-only damage first; never refund life that was not deducted.
 const auto refund=std::max<std::int64_t>(0,amount-(it->damage-it->lifeLinkedDamage));
 if(!sum(next.player.life,refund,next.player.life))return false;
 it->damage-=amount;it->lifeLinkedDamage-=refund;
 auto description=it->name+" "+change(delta)+" / correction";
 return commit(std::move(next),std::move(description));
}
bool GameState::setShortcut(unsigned side,Counter counter) {
 if(side>1 || counter==Counter::Custom || static_cast<unsigned>(counter)>=counterCount || state_.shortcuts[side]==counter)return false;
 auto next=state_;next.shortcuts[side]=counter;
 return commit(std::move(next),"Changed shortcut / "+std::string(counterName(counter)));
}
std::string GameState::recentUndoLabel(std::uint64_t now) const {
 auto it=recent_.begin();while(it!=recent_.end()&&(now<it->time||now-it->time>5000))++it;
 if(it==recent_.end())return {};
 const auto& before=it->before.player;const auto& after=state_.player;
 auto delta=[](std::int64_t old,std::int64_t value){
  return value>=old?"+"+std::to_string(static_cast<std::uint64_t>(value)-static_cast<std::uint64_t>(old)):
   "-"+std::to_string(static_cast<std::uint64_t>(old)-static_cast<std::uint64_t>(value));
 };
 if(before.life!=after.life && before.counters==after.counters)return "Undo "+delta(before.life,after.life);
 if(before.life==after.life) {
  unsigned changed=0,index=0;for(unsigned i=0;i<counterCount;++i)if(before.counters[i]!=after.counters[i]){++changed;index=i;}
  if(changed==1)return "Undo "+delta(before.counters[index],after.counters[index])+" "+counterName(static_cast<Counter>(index));
 }
 return "Undo "+std::to_string(static_cast<unsigned>(recent_.end()-it))+" edits";
}
bool GameState::undoRecent(std::uint64_t now) {
 while(!recent_.empty()&&(now<recent_.front().time||now-recent_.front().time>5000))recent_.pop_front();
 if(recent_.empty())return false;
 auto before=recent_.front().before;auto after=state_;
 auto remove=std::min(cursor_,recent_.size());history_.erase(history_.begin()+(cursor_-remove),history_.end());
 cursor_=history_.size();history_.push_back({before,std::move(after),"Five-second edit group"});
 state_=std::move(before);recent_.clear();++revision_;return true;
}
bool GameState::adjustLife(std::int64_t delta) {
 auto next=state_; if(!delta || !sum(next.player.life,delta,next.player.life)) return false;
 return commit(std::move(next),"Life " + change(delta));
}
bool GameState::adjustCounter(Counter counter, std::int64_t delta) {
 auto i=static_cast<unsigned>(counter); if(i>=counterCount || !delta) return false;
 auto next=state_; auto& value=next.player.counters[i];
 if(!sum(value,delta,value) || value<0) return false;
 return commit(std::move(next),std::string(counterName(counter))+" "+change(delta));
}
bool GameState::enableCounter(Counter counter,bool enabled) {
 auto i=static_cast<unsigned>(counter); if(i==0 || counter==Counter::Treasure || i>=counterCount || state_.player.enabled[i]==enabled) return false;
 auto next=state_; next.player.enabled[i]=enabled;
 return commit(std::move(next),std::string(counterName(counter))+(enabled?" shown":" hidden"));
}
bool GameState::setPoisonLimit(std::int64_t limit) {
 if(limit<1 || limit==state_.poisonLimit) return false;
 auto next=state_;next.poisonLimit=limit;return commit(std::move(next),"Poison limit "+std::to_string(limit));
}
const CommanderSource* GameState::source(std::uint32_t id) const {
 for(const auto& source:state_.sources) if(source.id==id) return &source;
 return nullptr;
}
bool GameState::applyCommanderDamage(std::uint32_t id,std::int64_t amount,bool subtractLife) {
 if(amount<=0) return false;
 auto next=state_;
 auto it=std::find_if(next.sources.begin(),next.sources.end(),[id](const auto& s){return s.id==id;});
 if(it==next.sources.end() || !sum(it->damage,amount,it->damage)) return false;
 if(subtractLife) {
  if(!sum(next.player.life,-amount,next.player.life))return false;
  it->lifeLinkedDamage+=amount;
 }
 return commit(std::move(next),it->name+" +"+std::to_string(amount)+(subtractLife?" / life":" / record"));
}
bool GameState::addSource(unsigned owner) {
 if(owner<1 || owner>state_.playerCount || state_.nextSourceId==std::numeric_limits<std::uint32_t>::max()) return false;
 auto next=state_;unsigned count=0;for(const auto& s:next.sources) if(s.owner==owner) ++count;
 auto suffix=count<26 ? std::string(1,static_cast<char>('A'+count)) : std::to_string(count+1);
 next.sources.push_back({next.nextSourceId++,owner,"P"+std::to_string(owner)+" / COMMANDER "+suffix,0});
 return commit(std::move(next),"Added commander source");
}
bool GameState::renameSource(std::uint32_t id,const std::string& name) {
 if(!validName(name)) return false;
 auto next=state_;for(auto& s:next.sources) if(s.id==id) {
  if(s.name==name) return false;s.name=name;return commit(std::move(next),"Renamed damage source");
 } return false;
}
bool GameState::addOwnCommander() {
 auto next=state_;next.commanders.push_back({"MY COMMANDER "+std::to_string(next.commanders.size()+1),0});
 return commit(std::move(next),"Added own commander");
}
bool GameState::renameOwnCommander(std::size_t index,const std::string& name) {
 if(index>=state_.commanders.size() || !validName(name) || state_.commanders[index].name==name) return false;
 auto next=state_;next.commanders[index].name=name;return commit(std::move(next),"Renamed own commander");
}
bool GameState::renameCustomCounter(const std::string& name) {
 if(!validName(name) || name==state_.player.customName) return false;
 auto next=state_;next.player.customName=name;return commit(std::move(next),"Renamed custom counter");
}
bool GameState::adjustCasts(std::size_t index,std::int64_t delta) {
 if(index>=state_.commanders.size() || !delta) return false;
 auto next=state_;auto& value=next.commanders[index].casts;
 if(!sum(value,delta,value) || value<0 || value>std::numeric_limits<std::int64_t>::max()/2) return false;
 return commit(std::move(next),"Command-zone casts "+change(delta));
}
std::int64_t GameState::nextTax(std::size_t index) const {
 return index<state_.commanders.size()?state_.commanders[index].casts*2:0;
}
bool GameState::toggleStatus(Status status) {
 auto next=state_;bool* value=nullptr;std::string name;
 switch(status) {
  case Status::Monarch:value=&next.player.monarch;name="Monarch";break;
  case Status::Initiative:value=&next.player.initiative;name="Initiative";break;
  case Status::CityBlessing:value=&next.player.cityBlessing;name="City's blessing";break;
  default:return false;
 }
 *value=!*value;return commit(std::move(next),name+(*value?" on":" off"));
}
bool GameState::cycleDayNight() {
 auto next=state_;next.dayNight=static_cast<DayNight>((static_cast<int>(next.dayNight)+1)%3);
 return commit(std::move(next),"Day / night changed");
}
bool GameState::newTurn() {
 if(state_.player.counters[static_cast<unsigned>(Counter::Spells)]==0) return false;
 auto next=state_;next.player.counters[static_cast<unsigned>(Counter::Spells)]=0;
 return commit(std::move(next),"New turn / spells reset");
}
bool GameState::poisonWarning() const { return state_.player.counters[0]>=state_.poisonLimit; }
bool GameState::commanderWarning() const {
 for(const auto& s:state_.sources) if(s.damage>=21) return true;return false;
}
bool GameState::undo() { if(!canUndo()) return false;if(!recent_.empty())recent_.pop_back();state_=history_[--cursor_].before;++revision_;return true; }
bool GameState::redo() { if(!canRedo()) return false;
 if(actionTime_) {
  while(!recent_.empty()&&(*actionTime_<recent_.front().time||*actionTime_-recent_.front().time>5000))recent_.pop_front();
  recent_.push_back({state_,*actionTime_});
 }
 state_=history_[cursor_++].after;++revision_;return true; }
void GameState::reset() {
 auto next=state_;next.player.life=40;next.player.counters.fill(0);
 next.player.monarch=next.player.initiative=next.player.cityBlessing=false;
 for(auto& s:next.sources){s.damage=0;s.lifeLinkedDamage=0;}for(auto& c:next.commanders)c.casts=0;
 next.dayNight=DayNight::Neither;commit(std::move(next),"New game / 40 life");
}
}
