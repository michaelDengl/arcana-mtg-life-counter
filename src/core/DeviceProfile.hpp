#pragma once
#include "platform/IStorage.hpp"
#include <string>
namespace mtg {
class DeviceProfile {
public:
 explicit DeviceProfile(IStorage* storage=nullptr);
 bool load();bool setName(const std::string& name);
 const std::string& name()const{return name_;}const std::string& id()const{return id_;}const std::string& error()const{return error_;}
 static bool validName(const std::string& value);
private:IStorage* storage_;std::string id_,name_,error_;
};
}
