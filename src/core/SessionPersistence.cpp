#include "SessionPersistence.hpp"
namespace mtg {
bool SessionPersistence::load() {
 std::string data;error_.clear();
 if(!storage_.load(data,error_))return false;
 if(!data.empty()&&!game_.deserialize(data)){error_="Saved game is invalid / New game available";return false;}
 revision_=game_.revision();active_=game_.state().active;dirty_=false;return true;
}
void SessionPersistence::tick(std::uint64_t now) {
 if(revision_!=game_.revision()) {
  revision_=game_.revision();dirty_=true;changedAt_=now;
  bool lifecycle=active_!=game_.state().active || game_.history().empty();active_=game_.state().active;
  if(lifecycle){flush();lastAttempt_=now;return;}
 }
 if(dirty_ && now-changedAt_>=600 && (error_.empty()||now-lastAttempt_>=2000)){lastAttempt_=now;flush();}
}
bool SessionPersistence::flush() {
 if(revision_!=game_.revision()){revision_=game_.revision();dirty_=true;active_=game_.state().active;}
 if(!dirty_)return error_.empty();
 if(!storage_.save(game_.serialize(),error_))return false;
 dirty_=false;error_.clear();return true;
}
}
