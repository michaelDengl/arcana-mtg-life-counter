#pragma once
#include "platform/IRandom.hpp"
#include <random>
namespace mtg {
class SimulatorRandom final:public IRandom {
public:
 SimulatorRandom():engine_(std::random_device{}()){}
 unsigned uniform(unsigned lower,unsigned upper)override {return std::uniform_int_distribution<unsigned>(lower,upper)(engine_);}
private:
 std::mt19937 engine_;
};
}
