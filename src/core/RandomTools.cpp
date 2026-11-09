#include "RandomTools.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
namespace mtg {
bool flipCoin(IRandom& random) {return random.uniform(0,1)==1;}
StarterRoll rollStarter(unsigned players,IRandom& random) {
 if(players<1||players>6)throw std::invalid_argument("Player count must be 1-6");
 StarterRoll result;std::vector<unsigned> candidates;
 for(unsigned p=1;p<=players;++p)candidates.push_back(p);
 // Bound pathological tie sequences; an unresolved tie offers an explicit reroll.
 for(unsigned round=0;round<32;++round) {
  std::vector<PlayerRoll> rolls;unsigned highest=0;
  for(auto p:candidates){unsigned die=random.uniform(1,20);rolls.push_back({p,die});highest=std::max(highest,die);}
  candidates.clear();for(const auto& r:rolls)if(r.die==highest)candidates.push_back(r.player);
  result.rounds.push_back(std::move(rolls));
  if(candidates.size()==1){result.winner=candidates.front();break;}
 }
 return result;
}
}
