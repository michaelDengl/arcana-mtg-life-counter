#include "core/GameState.hpp"
#include "core/RandomTools.hpp"
#include "SimulatorHardware.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
void check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
class ScriptedRandom:public mtg::IRandom {
public:
 std::vector<unsigned> values;std::size_t index=0;bool alwaysMinimum=false;
 unsigned uniform(unsigned lower,unsigned upper)override {
  if(alwaysMinimum)return lower;
  check(index<values.size(),"random sequence exhausted");auto value=values[index++];check(value>=lower&&value<=upper,"random range");return value;
 }
};
int main() {
 mtg::SimulatorHardware hardware;
 for(int i=0;i<100000;++i) hardware.rotate(1);
 check(hardware.takeRotation()==100000 && hardware.takeRotation()==0, "rotation queue loss or duplication");
 hardware.togglePower();hardware.rotate(10);
 check(hardware.takeRotation()==0, "powered off input");
 hardware.togglePower();hardware.rotate(-9);
 check(hardware.takeRotation()==-9, "power on input");
 mtg::GameState game;
 check(game.player().life == 40 && game.player().id == 1, "initial player");
 check(game.adjustLife(5) && game.player().life == 45, "gain");
 check(game.adjustLife(-50) && game.player().life == -5, "negative life");
 for (int i=0;i<100000;++i) game.adjustLife(1);
 check(game.player().life == 99995, "rapid input");
 game.reset(); check(game.player().life == 40, "reset");
 game.adjustLife(std::numeric_limits<std::int64_t>::max()-40);
 check(!game.adjustLife(1), "overflow rejected");
 game.reset(); game.adjustLife(std::numeric_limits<std::int64_t>::min()); game.adjustLife(-40);
 check(!game.adjustLife(-1), "underflow rejected");

 mtg::GameState counters;
 using mtg::Counter;
 auto count=[&](Counter c){return counters.player().counters[static_cast<unsigned>(c)];};
 check(counters.adjustCounter(Counter::Poison,9) && !counters.poisonWarning(), "poison below threshold");
 counters.adjustCounter(Counter::Poison,1);check(counters.poisonWarning(), "poison threshold 10");
 check(!counters.adjustCounter(Counter::Poison,-11) && count(Counter::Poison)==10,"no negative counters");
 counters.setPoisonLimit(15);check(!counters.poisonWarning(),"variant threshold");
 counters.undo();check(counters.poisonWarning(),"undo threshold setting");
 for(unsigned i=1;i<mtg::counterCount;++i) {
  auto c=static_cast<Counter>(i);counters.enableCounter(c,true);counters.adjustCounter(c,5);
  counters.enableCounter(c,false);check(count(c)==5,"hidden counters preserved");
  counters.enableCounter(c,true);check(count(c)==5,"reenable counter");
 }
 counters.newTurn();check(count(Counter::Spells)==0 && count(Counter::Energy)==5,"new turn only resets spells");
 counters.undo();check(count(Counter::Spells)==5,"undo new turn");
 check(!counters.adjustCounter(Counter::Count,1),"invalid counter rejected");
 counters.renameCustomCounter("TREASURE");check(counters.player().customName=="TREASURE","custom name");
 check(!counters.renameCustomCounter("   "),"blank name rejected");
 mtg::GameState damage;
 check(!damage.applyCommanderDamage(999,1,true),"unknown commander rejected");
 check(!damage.applyCommanderDamage(1,-1,true),"damage cannot be negative");
 damage.applyCommanderDamage(1,20,true);
 check(damage.player().life==20 && damage.source(1)->damage==20 && !damage.commanderWarning(),"linked commander damage");
 damage.applyCommanderDamage(2,20,false);
 check(damage.player().life==20 && !damage.commanderWarning(),"separate sources and record-only");
 damage.addSource(2);const auto partnerId=damage.state().sources.back().id;
 damage.applyCommanderDamage(partnerId,20,false);check(!damage.commanderWarning(),"partners do not combine damage");
 damage.applyCommanderDamage(partnerId,1,true);
 check(damage.commanderWarning() && damage.player().life==19,"21 per-source threshold");
 damage.undo();check(!damage.commanderWarning() && damage.player().life==20 && damage.source(partnerId)->damage==20,"atomic damage undo");
 damage.redo();check(damage.commanderWarning() && damage.player().life==19,"atomic damage redo");
 damage.undo();damage.adjustLife(3);check(!damage.canRedo(),"new action discards redo branch");
 damage.renameSource(partnerId,"OPPONENT PARTNER");check(damage.source(partnerId)->id==partnerId,"rename keeps identity");
 mtg::GameState casts;
 check(casts.nextTax(0)==0,"first cast has no tax");
 casts.adjustCasts(0,1);check(casts.nextTax(0)==2,"tax after first cast");
 casts.addOwnCommander();casts.adjustCasts(0,2);casts.adjustCasts(1,1);
 check(casts.nextTax(0)==6 && casts.nextTax(1)==2,"partner tax independent");
 check(!casts.adjustCasts(1,-2),"negative casts rejected");
 casts.undo();check(casts.nextTax(1)==0,"undo cast");
 casts.toggleStatus(mtg::Status::Monarch);casts.toggleStatus(mtg::Status::Initiative);
 check(casts.player().monarch && casts.player().initiative,"statuses independent");
 casts.toggleStatus(mtg::Status::CityBlessing);casts.cycleDayNight();
 check(casts.player().cityBlessing && casts.state().dayNight==mtg::DayNight::Day,"status tracking");
 casts.cycleDayNight();check(casts.state().dayNight==mtg::DayNight::Night,"night");
 casts.cycleDayNight();check(casts.state().dayNight==mtg::DayNight::Neither,"clear day/night");
 casts.reset();check(casts.player().life==40 && casts.nextTax(0)==0 && !casts.player().monarch,"reset advanced state");
 casts.undo();check(casts.player().monarch && casts.nextTax(0)==6,"undo full reset");
 mtg::GameState boundaries;
 boundaries.adjustLife(std::numeric_limits<std::int64_t>::min());boundaries.adjustLife(-40);
 check(!boundaries.applyCommanderDamage(1,1,true) && boundaries.source(1)->damage==0,"damage overflow is atomic");
 boundaries.applyCommanderDamage(1,std::numeric_limits<std::int64_t>::max(),false);
 check(!boundaries.applyCommanderDamage(1,1,false),"damage overflow rejected");
 check(!boundaries.adjustCasts(0,std::numeric_limits<std::int64_t>::max()),"tax overflow rejected");
 boundaries.adjustCounter(Counter::Energy,std::numeric_limits<std::int64_t>::max());
 check(!boundaries.adjustCounter(Counter::Energy,1),"counter overflow rejected");
 check(game.history().size()==64,"history memory bounded");
 ScriptedRandom random;random.values={0,1};check(!mtg::flipCoin(random)&&mtg::flipCoin(random),"both coin outcomes");
 random.values={20,20,1,3,19};random.index=0;auto starter=mtg::rollStarter(3,random);
 check(starter.winner==2&&starter.rounds.size()==2&&starter.rounds[1].size()==2,"only tied high rollers reroll");
 check(starter.rounds[1][0].player==1&&starter.rounds[1][1].player==2,"tiebreak candidates");
 random.alwaysMinimum=true;starter=mtg::rollStarter(6,random);check(starter.winner==0&&starter.rounds.size()==32,"pathological ties bounded without biased winner");
 starter=mtg::rollStarter(1,random);check(starter.winner==1&&starter.rounds[0][0].die==1,"solo D20 roll");
 mtg::GameState treasure;treasure.adjustCounter(mtg::Counter::Treasure,12);treasure.undo();check(treasure.player().counters[6]==0,"treasure undo");treasure.redo();check(treasure.player().counters[6]==12,"treasure redo");treasure.reset();check(treasure.player().counters[6]==0,"treasure reset");
 mtg::GameState recent;recent.setActionTime(1000);recent.adjustLife(-5);recent.setActionTime(2000);recent.adjustLife(-5);
 check(recent.recentUndoLabel(3000)=="Undo -10","recent life summary");
 check(recent.undoRecent(6501)&&recent.player().life==35,"five-second window excludes older edit");
 recent.redo();check(recent.player().life==30,"recent group redo");
 recent.startNewGame(4);recent.setActionTime(10000);for(int i=0;i<100;++i)recent.adjustLife(-1);
 check(recent.history().size()==64&&recent.recentUndoLabel(11000)=="Undo -100","recent group extends beyond bounded history");
 check(recent.undoRecent(11000)&&recent.player().life==40,"all rapid edits reversed");
 recent.setActionTime(11000);recent.redo();check(recent.recentUndoLabel(11000)=="Undo -100","redo can be reversed with recent Undo");recent.undoRecent(11000);
 mtg::GameState recentSaved;check(recentSaved.deserialize(recent.serialize()),"grouped undo serializes coherently");recentSaved.redo();check(recentSaved.player().life==-60,"grouped redo after restore");
 recent.setActionTime(20000);recent.adjustCounter(mtg::Counter::Energy,3);check(recent.recentUndoLabel(25001).empty(),"expired recent action disabled");
 recent.setActionTime(30000);recent.adjustCounter(mtg::Counter::Poison,2);recent.applyCommanderDamage(1,4,true);
 check(recent.undoRecent(31000)&&recent.player().life==40&&recent.source(1)->damage==0&&recent.player().counters[0]==0&&recent.player().counters[1]==3,"mixed recent changes restore atomically");
 mtg::GameState formats;
 for(auto format:{mtg::GameFormat::Classic,mtg::GameFormat::Brawl,mtg::GameFormat::Limited}) {
  check(formats.startNewGame(2,format),"start noncommander preset");auto hp=mtg::startingLife(format);
  check(formats.player().life==hp&&formats.life(2)==hp&&formats.state().sources.empty(),"correct starting life and no CMD sources");
  formats.adjustPlayerLife(2,-7);check(formats.life(2)==hp-7&&formats.player().life==hp,"opponent life independent");
  formats.undo();check(formats.life(2)==hp,"opponent life undo");formats.redo();check(formats.life(2)==hp-7,"opponent life redo");
  check(!formats.applyCommanderDamage(1,21,true)&&!formats.commanderWarning(),"CMD disabled outside commander");
  formats.reset();check(formats.life(2)==hp&&formats.player().life==hp,"reset uses preset life");
 }
 check(!formats.startNewGame(3,mtg::GameFormat::TwoHeadedGiant)&&formats.startNewGame(4,mtg::GameFormat::TwoHeadedGiant),"2HG requires four seats");
 check(formats.state().teams[0]==formats.state().teams[1]&&formats.state().teams[2]==formats.state().teams[3]&&formats.state().teams[0]!=formats.state().teams[2],"explicit 2HG teams");
 formats.adjustPlayerLife(2,-5);check(formats.life(1)==35&&formats.life(2)==35,"local team shares life");
 formats.adjustPlayerLife(4,-8);check(formats.life(3)==32&&formats.life(4)==32&&formats.life(1)==35,"enemy team shares independent life");
 formats.undo();check(formats.life(3)==40&&formats.life(4)==40,"team life undo atomic");formats.redo();
 formats.applyCommanderDamage(1,19,false);formats.applyCommanderDamage(2,2,false);check(!formats.commanderWarning(),"2HG never combines damage dealt to different teammates");formats.adjustCommanderDamage(1,-19,false);formats.adjustCommanderDamage(2,-2,false);
 formats.applyCommanderDamage(1,21,true);check(formats.life(1)==14&&formats.life(2)==14&&formats.commanderWarning(),"2HG linked commander damage is atomic");
 formats.adjustCounter(mtg::Counter::Poison,14);check(!formats.poisonWarning(),"2HG poison below threshold");formats.adjustCounter(mtg::Counter::Poison,1);check(formats.poisonWarning(),"2HG poison shared threshold");
 formats.reset();check(formats.life(1)==40&&formats.life(2)==40&&formats.life(3)==40&&formats.life(4)==40,"2HG reset restores both teams");
 for(unsigned teams=2;teams<=6;++teams){
  check(formats.startNewGame(teams*2,mtg::GameFormat::TwoHeadedGiant),"2HG supports two through six teams");
  for(unsigned team=0;team<teams;++team){unsigned seat=2*team+1;formats.adjustPlayerLife(seat,-static_cast<std::int64_t>(team+1));check(formats.life(seat)==39-static_cast<std::int64_t>(team)&&formats.life(seat+1)==formats.life(seat),"each team has independent shared life");}
  formats.undo();check(formats.life(teams*2)==40,"last team life undo atomic");formats.redo();check(formats.life(teams*2)==40-static_cast<std::int64_t>(teams),"last team life redo atomic");
  formats.reset();for(unsigned seat=1;seat<=teams*2;++seat)check(formats.life(seat)==40,"reset restores every team");
 }
 check(!formats.startNewGame(2,mtg::GameFormat::TwoHeadedGiant)&&!formats.startNewGame(13,mtg::GameFormat::TwoHeadedGiant)&&!formats.startNewGame(5,mtg::GameFormat::TwoHeadedGiant),"reject invalid team counts");
 check(!formats.startNewGame(8,mtg::GameFormat::Commander),"larger limits apply only to 2HG");
 std::cout << "All game engine checks passed\n";
}
