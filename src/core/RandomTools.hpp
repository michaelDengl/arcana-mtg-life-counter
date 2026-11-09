#pragma once
#include "platform/IRandom.hpp"
#include <vector>
namespace mtg {
struct PlayerRoll {unsigned player,die;};
struct StarterRoll {std::vector<std::vector<PlayerRoll>> rounds;unsigned winner=0;};
bool flipCoin(IRandom& random);
StarterRoll rollStarter(unsigned players,IRandom& random);
}
