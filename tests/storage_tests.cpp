#include "core/SessionPersistence.hpp"
#include "FileStorage.hpp"
#include "third_party/json.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
class MemoryStorage:public mtg::IStorage {
public:
 std::string data;unsigned writes=0;bool fail=false;
 bool load(std::string& out,std::string& error)override{out=data;error.clear();return true;}
 bool save(const std::string& value,std::string& error)override{++writes;if(fail){error="Simulated write failure";return false;}data=value;error.clear();return true;}
};
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
int main() {
 using mtg::Counter;mtg::GameState game;
 check(!game.startNewGame(0)&&!game.startNewGame(7),"invalid player counts");
 game.startNewGame(6);game.adjustLife(-17);game.adjustCounter(Counter::Poison,5);game.enableCounter(Counter::Energy,true);game.adjustCounter(Counter::Energy,19);game.adjustCounter(Counter::Treasure,13);
 game.addSource(2);game.renameSource(game.state().sources.back().id,"PARTNER");game.applyCommanderDamage(1,3,true);game.addOwnCommander();game.adjustCasts(1,2);game.toggleStatus(mtg::Status::Monarch);game.cycleDayNight();game.undo();
 auto saved=game.serialize();mtg::GameState restored;check(restored.deserialize(saved),"deserialize complete unfinished game");
 check(restored.player().counters[6]==13,"treasure persisted");
 check(restored.serialize()==saved&&restored.state().playerCount==6&&restored.state().active,"all stats and redo history preserved");
 restored.redo();check(restored.state().dayNight==mtg::DayNight::Day,"restored redo");
 auto legacy=nlohmann::json::parse(saved);legacy["schema_version"]=1;
 auto legacySnapshot=[](nlohmann::json& snapshot){while(snapshot["player"]["counters"].size()>6)snapshot["player"]["counters"].erase(snapshot["player"]["counters"].size()-1);while(snapshot["player"]["enabled"].size()>6)snapshot["player"]["enabled"].erase(snapshot["player"]["enabled"].size()-1);snapshot.erase("shortcuts");};
 legacySnapshot(legacy["state"]);for(auto& entry:legacy["history"]){legacySnapshot(entry["before"]);legacySnapshot(entry["after"]);}
 mtg::GameState migrated;check(migrated.deserialize(legacy.dump())&&migrated.player().counters[6]==0&&migrated.player().life==game.player().life,"version 1 saved games migrate with zero treasures");
 check(nlohmann::json::parse(migrated.serialize())["schema_version"]==5,"migrated save writes version 5");
 auto v2=nlohmann::json::parse(saved);v2["schema_version"]=2;
 auto stripShield=[](nlohmann::json& snapshot){snapshot["player"]["counters"].erase(7);snapshot["player"]["enabled"].erase(7);snapshot.erase("shortcuts");};
 stripShield(v2["state"]);for(auto& entry:v2["history"]){stripShield(entry["before"]);stripShield(entry["after"]);}
 check(migrated.deserialize(v2.dump())&&migrated.player().counters[6]==13&&migrated.player().counters[7]==0,"version 2 preserves treasures and adds shield");
 game.setShortcut(0,Counter::Shield);game.setShortcut(1,Counter::Rad);game.adjustCounter(Counter::Shield,4);game.adjustCounter(Counter::Rad,2);
 check(migrated.deserialize(game.serialize())&&migrated.state().shortcuts[0]==Counter::Shield&&migrated.player().counters[7]==4,"selected shortcuts and counts persist");
 auto oldV3=nlohmann::json::parse(saved);oldV3["schema_version"]=3;
 auto stripFormat=[](nlohmann::json& snapshot){snapshot.erase("format");snapshot.erase("teams");snapshot.erase("opponent_life");};
 stripFormat(oldV3["state"]);for(auto& entry:oldV3["history"]){stripFormat(entry["before"]);stripFormat(entry["after"]);}
 check(migrated.deserialize(oldV3.dump())&&migrated.state().format==mtg::GameFormat::Commander&&migrated.player().life==game.player().life,"version 3 migrates as Commander preserving local life");
 for(auto format:{mtg::GameFormat::Classic,mtg::GameFormat::Brawl,mtg::GameFormat::Limited,mtg::GameFormat::TwoHeadedGiant}){
  mtg::GameState variant;variant.startNewGame(format==mtg::GameFormat::TwoHeadedGiant?4:2,format);variant.adjustPlayerLife(format==mtg::GameFormat::TwoHeadedGiant?4:2,-5);variant.adjustCounter(Counter::Poison,2);
  mtg::GameState copy;check(copy.deserialize(variant.serialize())&&copy.serialize()==variant.serialize(),"format life teams counters and history round trip");
  auto invalid=nlohmann::json::parse(variant.serialize());invalid["state"]["format"]=99;auto unchanged=copy.serialize();check(!copy.deserialize(invalid.dump())&&copy.serialize()==unchanged,"invalid format rejected atomically");
  if(format==mtg::GameFormat::TwoHeadedGiant){invalid=nlohmann::json::parse(variant.serialize());invalid["state"]["opponent_life"][0]=17;check(!copy.deserialize(invalid.dump()),"mismatched shared life rejected");}
 }
 mtg::GameState legacyTeam;legacyTeam.startNewGame(4,mtg::GameFormat::TwoHeadedGiant);legacyTeam.adjustPlayerLife(4,-7);
 auto v4=nlohmann::json::parse(legacyTeam.serialize());v4["schema_version"]=4;
 auto shrinkSeats=[](nlohmann::json& snap){while(snap["opponent_life"].size()>5)snap["opponent_life"].erase(snap["opponent_life"].size()-1);while(snap["teams"].size()>6)snap["teams"].erase(snap["teams"].size()-1);};
 shrinkSeats(v4["state"]);for(auto& entry:v4["history"]){shrinkSeats(entry["before"]);shrinkSeats(entry["after"]);}
 check(migrated.deserialize(v4.dump())&&migrated.life(3)==33&&migrated.life(4)==33,"schema 4 team saves migrate without losing life or history");
 for(unsigned teams=2;teams<=6;++teams){mtg::GameState large;check(large.startNewGame(teams*2,mtg::GameFormat::TwoHeadedGiant),"start adjustable team game");large.adjustPlayerLife(teams*2,-9);mtg::GameState copy;check(copy.deserialize(large.serialize())&&copy.serialize()==large.serialize()&&copy.life(teams*2-1)==31,"all team sizes preserve shared life and history");}
 auto json=nlohmann::json::parse(saved);json["schema_version"]=999;auto before=restored.serialize();
 check(!restored.deserialize(json.dump())&&restored.serialize()==before,"unknown version rejected atomically");
 json=nlohmann::json::parse(saved);json["state"]["player"]["life"]=std::numeric_limits<std::uint64_t>::max();check(!restored.deserialize(json.dump()),"oversized integer rejected");
 json=nlohmann::json::parse(saved);json["state"]["sources"][0]["linked_life"]=100;check(!restored.deserialize(json.dump()),"invalid damage ledger rejected");
 check(!restored.deserialize("{broken")&&!restored.deserialize(""),"corrupt JSON rejected");
 MemoryStorage memory;mtg::GameState current;mtg::SessionPersistence persistence(current,memory);
 check(persistence.load()&&!current.state().active,"no saved session");current.startNewGame(4);persistence.tick(0);
 for(int i=1;i<=100;++i){current.adjustLife(-1);persistence.tick(i);}
 check(memory.writes==1,"encoder burst saves are coalesced");persistence.tick(699);check(memory.writes==1,"debounce not expired");
 persistence.tick(700);check(memory.writes==2,"one save after quiet period");
 mtg::GameState resumed;mtg::SessionPersistence recovery(resumed,memory);check(recovery.load()&&resumed.state().active&&resumed.player().life==-60,"resume last match after simulated crash");
 check(resumed.canUndo(),"saved undo history available");
 resumed.adjustLife(3);memory.fail=true;check(!recovery.flush()&&!recovery.error().empty(),"save failure reported");memory.fail=false;check(recovery.flush()&&recovery.error().empty(),"failed save can retry");
 resumed.endGame();recovery.tick(1000);mtg::GameState finished;check(finished.deserialize(memory.data)&&!finished.state().active,"end game saved immediately");
 check(!finished.canUndo(),"ending cannot be undone into an active match");
 mtg::GameState linked;linked.startNewGame(4);linked.applyCommanderDamage(1,5,true);linked.applyCommanderDamage(1,3,false);
 linked.adjustCommanderDamage(1,-2,true);check(linked.player().life==35&&linked.source(1)->damage==6,"unlinked correction no refund");
 linked.adjustCommanderDamage(1,-4,false);check(linked.player().life==38&&linked.source(1)->damage==2,"linked correction refunds only deducted life");
 linked.undo();check(linked.player().life==35&&linked.source(1)->damage==6,"correction undo atomic");
 mtg::FileStorage file(std::filesystem::current_path()/"storage-test"/"current-game.json");std::string error,data;
 check(file.save(saved,error)&&file.load(data,error)&&data==saved,"file storage round trip");
 game.endGame();check(file.save(game.serialize(),error)&&file.load(data,error),"atomic replacement of existing file");
 check(restored.deserialize(data)&&!restored.state().active,"finished marker survives file replacement");
 std::filesystem::remove(std::filesystem::current_path()/"storage-test"/"current-game.json");
 std::cout<<"Saved-game, recovery and lifecycle checks passed\n";
}
