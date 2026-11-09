#include "AppUI.hpp"
#include "Widgets/DialWidgets.hpp"
#include "src/core/lv_obj_event_private.h"
#include <cmath>
LV_FONT_DECLARE(tab_font);
namespace mtg {
using namespace widgets;
void AppUI::toolSectorHit(lv_event_t* event) {
 auto* binding=static_cast<Binding*>(lv_event_get_user_data(event));auto* hit=static_cast<lv_hit_test_info_t*>(lv_event_get_param(event));
 lv_area_t area;lv_obj_get_coords(binding->ui->screen_,&area);double x=hit->point->x-area.x1-180,y=hit->point->y-area.y1-180;
 double angle=std::atan2(y,x)*180/3.141592653589793;if(angle<0)angle+=360;
 double offset=std::fmod(angle-binding->sectorStart+720,360);
 hit->res=x*x+y*y>=46*46&&x*x+y*y<=150*150&&offset<=binding->sectorSpan;
}
void AppUI::brightnessChanged(lv_event_t* event) {auto* ui=static_cast<AppUI*>(lv_event_get_user_data(event));ui->hardware_.setBrightness(lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(event))));}
lv_obj_t* AppUI::radialSector(const std::string& title,unsigned index,unsigned count,Action action,std::uint32_t arg,unsigned color,ToolIcon icon) {
 double span=360.0/count,start=270-span/2+span*index;
 auto* sector=lv_arc_create(screen_);lv_obj_set_size(sector,300,300);lv_obj_set_pos(sector,30,30);
 lv_obj_remove_style(sector,nullptr,LV_PART_KNOB);lv_obj_set_style_arc_opa(sector,0,LV_PART_INDICATOR);
 lv_obj_set_style_arc_width(sector,104,LV_PART_MAIN);lv_obj_set_style_arc_color(sector,lv_color_hex(color),LV_PART_MAIN);lv_obj_set_style_arc_rounded(sector,false,LV_PART_MAIN);
 auto angle=[](double value){return static_cast<unsigned>(std::round(value+720))%360;};
 lv_arc_set_bg_angles(sector,angle(start+1),angle(start+span-1));lv_obj_remove_flag(sector,LV_OBJ_FLAG_CLICKABLE);
 auto* b=button(screen_,"",30,30,300,300,action,arg);lv_obj_clean(b);
 lv_obj_set_style_bg_opa(b,0,0);lv_obj_set_style_bg_opa(b,0,LV_STATE_PRESSED);lv_obj_set_style_border_width(b,0,0);lv_obj_set_style_radius(b,0,0);lv_obj_set_style_pad_all(b,0,0);
 bindings_.back().sectorStart=start;bindings_.back().sectorSpan=span;
 lv_obj_add_flag(b,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(b,toolSectorHit,LV_EVENT_HIT_TEST,&bindings_.back());
 double theta=(270+span*index)*3.141592653589793/180,radius=count>=7?112:100;
 int x=150+radius*std::cos(theta),y=150+radius*std::sin(theta);
 drawToolIcon(b,icon,x-15,y-25,0xffffff);label(b,title,x-49,y+10,98,&lv_font_montserrat_14,0xffffff);
 return b;
}
void AppUI::radialBack(Action action) {
 auto* back=button(screen_,"Back",136,136,88,88,action,0,0x000000);lv_obj_set_style_radius(back,LV_RADIUS_CIRCLE,0);lv_obj_set_style_border_width(back,0,0);
 lv_obj_set_style_text_color(lv_obj_get_child(back,0),lv_color_hex(0xaaaaaa),0);
}
void AppUI::buildTools() {
 if(view_==View::Gear) {
  const char* titles[]{"New Game","Flip Coin","Roll","End Game","Brightness"};
  const Action actions[]{AskReset,FlipCoin,RollD20,RequestEnd,OpenBrightness};
  const ToolIcon icons[]{ToolIcon::Reset,ToolIcon::Coin,ToolIcon::D20,ToolIcon::End,ToolIcon::Brightness};
  const unsigned colors[]{red,gold,violet,red,teal};
  for(unsigned i=0;i<5;++i)radialSector(titles[i],i,5,actions[i],0,colors[i],icons[i]);
 }else if(view_==View::Brightness) {
  auto* top=radialSector("",0,3,NoAction,0,0x166853,ToolIcon::Brightness);lv_obj_clean(top);
  label(screen_,"BRIGHTNESS",105,49,150,&lv_font_montserrat_14,ink);
  value_=label(screen_,"",115,74,130,&lv_font_montserrat_32,ink);
  brightnessSlider_=lv_slider_create(screen_);lv_obj_set_pos(brightnessSlider_,105,117);lv_obj_set_size(brightnessSlider_,150,10);lv_slider_set_range(brightnessSlider_,0,100);lv_slider_set_value(brightnessSlider_,hardware_.brightness(),LV_ANIM_OFF);lv_obj_add_event_cb(brightnessSlider_,brightnessChanged,LV_EVENT_VALUE_CHANGED,this);
  radialSector("+",1,3,Plus,0,teal,ToolIcon::Brightness);radialSector("-",2,3,Minus,0,red,ToolIcon::Brightness);
 }else if(view_==View::ResetConfirm||view_==View::EndConfirm) {
  bool ending=view_==View::EndConfirm;
  auto* top=radialSector("",0,3,NoAction,0,0x773044,ending?ToolIcon::End:ToolIcon::Reset);lv_obj_clean(top);
  label(screen_,ending?"END GAME?":"NEW GAME?",95,57,170,&lv_font_montserrat_20,ink);
  label(screen_,ending?"Disable Continue?":"Reset all stats?",95,99,170,&lv_font_montserrat_14,ink);
  radialSector(ending?"End game":"Yes",1,3,ending?FinishGame:ResetYes,0,red,ToolIcon::Confirm);
  radialSector("No",2,3,ending?CancelEnd:ResetNo,0,teal,ToolIcon::Cancel);
 }else if(view_==View::CoinResult) {
  auto* result=radialSector("",0,2,NoAction,0,0x93702f,ToolIcon::Coin);lv_obj_clean(result);
  drawToolIcon(screen_,ToolIcon::Coin,165,56,0xffffff);
  label(screen_,coinHeads_?"HEADS":"TAILS",95,94,170,&lv_font_montserrat_32,0xffffff);
  radialSector("Flip again",1,2,FlipCoin,0,gold,ToolIcon::Reset);
 }else if(view_==View::RollResult) {
  const auto& rolls=starterRoll_.rounds.back();unsigned count=static_cast<unsigned>(rolls.size())+2;
  auto* result=radialSector("",0,count,NoAction,0,0x246e88,ToolIcon::D20);lv_obj_clean(result);
  label(screen_,starterRoll_.winner?"P"+std::to_string(starterRoll_.winner):"TIE",130,54,100,&lv_font_montserrat_32,ink);
  label(screen_,starterRoll_.winner?"STARTS":"REROLL",144,99,72,&lv_font_montserrat_14,ink);
  for(unsigned i=0;i<rolls.size();++i){
   auto* tile=radialSector("P"+std::to_string(rolls[i].player)+": "+std::to_string(rolls[i].die),i+1,count,NoAction,0,rolls[i].player==starterRoll_.winner?0x159870:0x326381,ToolIcon::D20);
   auto* title=lv_obj_get_child(tile,1);lv_obj_set_style_text_font(title,&tab_font,0);lv_obj_set_height(title,tab_font.line_height);
  }
  radialSector("Roll again",count-1,count,RollD20,0,violet,ToolIcon::Reset);
 }
 radialBack();
}
}
