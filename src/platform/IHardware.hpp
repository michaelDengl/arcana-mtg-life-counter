#pragma once
#include <cstdint>
namespace mtg {
struct Touch { int x = 0; int y = 0; bool pressed = false; };
struct BatteryStatus { unsigned percent=0; bool charging=false; bool available=false; };
class IHardware {
public:
 virtual ~IHardware() = default;
 virtual std::int64_t takeRotation() = 0;
 virtual Touch touch() const = 0;
 virtual bool powered() const = 0;
 virtual unsigned brightness() const { return 100; }
 virtual void setBrightness(unsigned) {}
 virtual BatteryStatus battery() const { return {}; }
};
}
