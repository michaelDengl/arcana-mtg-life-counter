#pragma once
#include "platform/IHardware.hpp"
namespace mtg {
class SimulatorHardware final : public IHardware {
public:
 void rotate(int steps) { if (on_) pending_ += steps; }
 std::int64_t takeRotation() override { auto result = pending_; pending_ = 0; return result; }
 Touch touch() const override { return touch_; }
 void setTouch(Touch value) { touch_ = value; }
 bool powered() const override { return on_; }
 unsigned brightness() const override { return brightness_; }
 void setBrightness(unsigned value) override { brightness_=value>100?100:value; }
 BatteryStatus battery() const override { return battery_; }
 void setBattery(unsigned percent,bool charging=false) { battery_={percent>100?100:percent,charging,true}; }
 void togglePower() { on_ = !on_; pending_ = 0; touch_.pressed = false; }
private:
 std::int64_t pending_ = 0;
 Touch touch_;
 bool on_ = true;
 unsigned brightness_=100;
 BatteryStatus battery_{75,false,true};
};
}
