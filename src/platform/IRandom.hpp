#pragma once
namespace mtg {
class IRandom {
public:
 virtual ~IRandom()=default;
 virtual unsigned uniform(unsigned lower,unsigned upper)=0;
};
}
