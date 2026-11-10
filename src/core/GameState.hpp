#pragma once
#include "Player.hpp"
#include <vector>
#include <deque>
#include <optional>
namespace mtg {
struct GameSnapshot {
 Player player;
 GameFormat format=GameFormat::Commander;
 std::array<std::int64_t,11> opponentLife{{40,40,40,40,40,40,40,40,40,40,40}};
 std::array<unsigned,12> teams{{0,1,2,3,4,5,6,7,8,9,10,11}};
 unsigned playerCount = 4;
 bool active = false;
 std::array<Counter,2> shortcuts{{Counter::Treasure,Counter::Poison}};
 std::vector<CommanderSource> sources{{1,2,"P2 / COMMANDER A",0},{2,3,"P3 / COMMANDER A",0},{3,4,"P4 / COMMANDER A",0}};
 std::vector<OwnCommander> commanders{{"MY COMMANDER",0}};
 std::int64_t poisonLimit = 10;
 DayNight dayNight = DayNight::Neither;
 std::uint32_t nextSourceId = 4;
};
struct HistoryEntry { GameSnapshot before, after; std::string description; };
class GameState {
public:
 const Player& player() const { return state_.player; }
 const GameSnapshot& state() const { return state_; }
 std::uint64_t revision() const { return revision_; }
 bool startNewGame(unsigned players,GameFormat format=GameFormat::Commander);
 std::int64_t life(unsigned owner) const;
 bool adjustPlayerLife(unsigned owner,std::int64_t delta);
 bool synchronizeLocal(std::int64_t life,const std::array<std::int64_t,counterCount>& counters);
 void endGame();
 std::string serialize() const;
 bool deserialize(const std::string& json);
 bool adjustCommanderDamage(std::uint32_t sourceId, std::int64_t delta, bool subtractLife);
 bool setShortcut(unsigned side,Counter counter);
 void setActionTime(std::uint64_t now) {actionTime_=now;}
 std::string recentUndoLabel(std::uint64_t now) const;
 bool undoRecent(std::uint64_t now);
 bool adjustLife(std::int64_t delta);
 bool adjustCounter(Counter counter, std::int64_t delta);
 bool enableCounter(Counter counter, bool enabled);
 bool setPoisonLimit(std::int64_t limit);
 bool applyCommanderDamage(std::uint32_t sourceId, std::int64_t amount, bool subtractLife);
 const CommanderSource* source(std::uint32_t id) const;
 bool addSource(unsigned owner);
 bool renameSource(std::uint32_t id, const std::string& name);
 bool addOwnCommander();
 bool renameOwnCommander(std::size_t index, const std::string& name);
 bool renameCustomCounter(const std::string& name);
 bool adjustCasts(std::size_t index, std::int64_t delta);
 std::int64_t nextTax(std::size_t index) const;
 bool toggleStatus(Status status);
 bool cycleDayNight();
 bool newTurn();
 bool poisonWarning() const;
 bool commanderWarning() const;
 bool canUndo() const { return cursor_ > 0; }
 bool canRedo() const { return cursor_ < history_.size(); }
 bool undo();
 bool redo();
 const std::vector<HistoryEntry>& history() const { return history_; }
 std::size_t historyCursor() const { return cursor_; }
 void reset();
private:
 bool commit(GameSnapshot next, std::string description);
 struct RecentEdit {GameSnapshot before;std::uint64_t time;};
 std::deque<RecentEdit> recent_;
 std::optional<std::uint64_t> actionTime_;
 GameSnapshot state_;
 std::vector<HistoryEntry> history_;
 std::size_t cursor_ = 0;
 std::uint64_t revision_ = 0;
};
}
