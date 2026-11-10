#include "DeviceProfile.hpp"
#include "third_party/json.hpp"
#include <random>
namespace mtg {
DeviceProfile::DeviceProfile(IStorage* storage):storage_(storage){std::random_device random;const char* hex="0123456789abcdef";for(int i=0;i<32;++i)id_+=hex[random()%16];}
bool DeviceProfile::validName(const std::string& value){return !value.empty()&&value.size()<=12&&value.front()!=' '&&value.back()!=' '&&value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -_")==std::string::npos;}
bool DeviceProfile::load(){if(!storage_)return true;std::string data;if(!storage_->load(data,error_))return false;if(data.empty())return true;try{auto j=nlohmann::json::parse(data);auto id=j.at("id").get<std::string>(),name=j.at("name").get<std::string>();if(j.at("version")!=1||id.size()!=32||id.find_first_not_of("0123456789abcdef")!=std::string::npos||!validName(name))throw 1;id_=id;name_=name;error_.clear();return true;}catch(...){error_="Name profile is invalid";return false;}}
bool DeviceProfile::setName(const std::string& value){auto name=value;while(!name.empty()&&name.back()==' ')name.pop_back();while(!name.empty()&&name.front()==' ')name.erase(0,1);if(!validName(name)){error_="Use 1-12 letters or digits";return false;}if(storage_){auto data=nlohmann::json{{"version",1},{"id",id_},{"name",name}}.dump(2);if(!storage_->save(data,error_))return false;}name_=name;error_.clear();return true;}
}
