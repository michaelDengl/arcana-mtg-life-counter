#pragma once
#include "GameState.hpp"
#include "platform/IStorage.hpp"
namespace mtg {
class SessionPersistence {
public:
 SessionPersistence(GameState& game,IStorage& storage):game_(game),storage_(storage),revision_(game.revision()),active_(game.state().active){}
 bool load();
 void tick(std::uint64_t now);
 bool flush();
 const std::string& error() const {return error_;}
private:
 GameState& game_;
 IStorage& storage_;
 std::uint64_t revision_,changedAt_=0,lastAttempt_=0;
 bool active_,dirty_=false;
 std::string error_;
};
}
