#include "AppUI.hpp"
#include "core/AppInfo.hpp"
#include "core/WifiSetup.hpp"
#include <cstring>
#include "Widgets/DialWidgets.hpp"
namespace mtg {
using namespace widgets;
void AppUI::buildInformation(){
 if(view_==View::ContactCode){
  label(screen_,contactLink_?appInfo::group:"NOVA DYNAMICS",50,34,260,&lv_font_montserrat_14,violet);
  const std::string link=contactLink_?appInfo::whatsapp:appInfo::website;
  auto* quiet=box(screen_,64,55,232,232,0xffffff);lv_obj_set_style_radius(quiet,10,0);
  auto* qr=lv_qrcode_create(quiet);lv_qrcode_set_size(qr,180);lv_obj_set_pos(qr,26,26);
  lv_qrcode_set_dark_color(qr,lv_color_black());lv_qrcode_set_light_color(qr,lv_color_white());
  lv_qrcode_update(qr,link.data(),static_cast<uint32_t>(link.size()));
  // Keep the QR unobstructed, including its quiet zone, for phone cameras.
  button(screen_,"Back",123,302,114,34,ToolBack,0,0x000000);return;
 }else if(view_==View::About){
  label(screen_,"A R C A N A",65,57,230,&lv_font_montserrat_20,violet);
  label(screen_,std::string("Created by ")+appInfo::creator,60,90,240,&lv_font_montserrat_14,muted);
  label(screen_,std::string("Version ")+appInfo::version,80,113,200,&lv_font_montserrat_14,ink);
  button(screen_,appInfo::website,58,236,244,32,ContactLink,0);
  button(screen_,appInfo::group,75,276,210,32,ContactLink,1);
  button(screen_,"Wi-Fi & Updates",115,312,130,27,OpenUpdates);
 }else if(view_==View::Updates){
  auto* top=radialSector("",0,3,NoAction,0,0x246e88,ToolIcon::Shield);lv_obj_clean(top);
  label(screen_,"FIRMWARE",100,50,160,&lv_font_montserrat_20,ink);
  label(screen_,std::string("Installed ")+appInfo::version,80,82,200,&lv_font_montserrat_14,ink);
  value_=label(screen_,"",80,105,200,&lv_font_montserrat_14,ink);
  radialSector("Wi-Fi setup",1,3,SetupWifi,0,0x24765c,ToolIcon::LifeLink);
  updateButton_=radialSector("Install",2,3,InstallUpdate,0,0x8151a3,ToolIcon::Confirm);
  detail_=label(screen_,"",81,280,198,&lv_font_montserrat_14,ink);
  lv_label_set_long_mode(detail_,LV_LABEL_LONG_WRAP);lv_obj_set_height(detail_,35);
  button(screen_,"Check",136,318,88,28,CheckUpdate);
 }else{
  label(screen_,"WI-FI SETUP",70,30,220,&lv_font_montserrat_20,violet);
  label(screen_,"Scan to join Wi-Fi",60,54,240,&lv_font_montserrat_14,muted);
  auto* quiet=box(screen_,72,76,216,216,0xffffff);lv_obj_set_style_radius(quiet,8,0);
  setupQr_=lv_qrcode_create(quiet);lv_qrcode_set_size(setupQr_,180);lv_obj_set_pos(setupQr_,18,18);
  lv_qrcode_set_dark_color(setupQr_,lv_color_black());lv_qrcode_set_light_color(setupQr_,lv_color_white());
  lv_obj_add_flag(quiet,LV_OBJ_FLAG_HIDDEN);
  value_=label(screen_,"",60,157,240,&lv_font_montserrat_14,ink);
  lv_label_set_long_mode(value_,LV_LABEL_LONG_WRAP);lv_obj_set_height(value_,65);
  detail_=label(screen_,"",50,296,260,&lv_font_montserrat_14,violet);
  button(screen_,"Back",124,318,112,26,ToolBack,0,0x000000);
  refreshInformation();return;
 }
 radialBack();refreshInformation();
}
void AppUI::refreshInformation(){
 if(view_==View::Updates){
  text(value_,updateStatus_.latest.empty()?"Release check":std::string("Latest ")+updateStatus_.latest);
  text(detail_,updateStatus_.phase==UpdatePhase::Installing?"Installing "+std::to_string(updateStatus_.progress)+"%":updateStatus_.message);
  if(updateStatus_.phase==UpdatePhase::Available)lv_obj_remove_state(updateButton_,LV_STATE_DISABLED);else lv_obj_add_state(updateButton_,LV_STATE_DISABLED);
 }else if(view_==View::WifiSetup){
  bool ready=updateStatus_.phase==UpdatePhase::Setup&&!updateStatus_.setupNetwork.empty();
  if(ready){
   auto payload=wifiSetupQr(updateStatus_.setupNetwork,updateStatus_.setupPassword);
   if(payload!=setupQrPayload_){
    ready=lv_qrcode_update(setupQr_,payload.data(),static_cast<uint32_t>(payload.size()))==LV_RESULT_OK;
    if(ready)setupQrPayload_=std::move(payload);
   }
  }
  if(ready){lv_obj_remove_flag(lv_obj_get_parent(setupQr_),LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(value_,LV_OBJ_FLAG_HIDDEN);}
  else {lv_obj_add_flag(lv_obj_get_parent(setupQr_),LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(value_,LV_OBJ_FLAG_HIDDEN);text(value_,updateStatus_.phase==UpdatePhase::Setup?updateStatus_.setupNetwork+"\nPassword: "+updateStatus_.setupPassword:updateStatus_.message);}
  text(detail_,"If needed: http://192.168.4.1");
 }
}
}
