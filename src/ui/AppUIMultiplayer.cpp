#include "AppUI.hpp"
#include "Widgets/DialWidgets.hpp"
#include <algorithm>
namespace mtg {
using namespace widgets;
void AppUI::buildMultiplayer(){
 if(view_==View::ArcanaMode){auto* single=radialSector("SINGLE ARCANA",0,2,SingleArcana,0,0x246e88,ToolIcon::Shield);auto* multi=radialSector("MULTI ARCANA",1,2,MultiArcana,0,0x159870,ToolIcon::LifeLink);for(auto* tile:{single,multi}){auto* caption=lv_obj_get_child(tile,1);lv_obj_set_x(caption,60);lv_obj_set_width(caption,180);}}
 else if(view_==View::NameSetup){
  auto* top=radialSector("",0,2,NoAction,0,0x246e88,ToolIcon::LifeLink);lv_obj_clean(top);auto* lower=radialSector("",1,2,NoAction,0,0x16483e,ToolIcon::LifeLink);lv_obj_clean(lower);
  label(screen_,"YOUR NAME",100,51,160,&lv_font_montserrat_14,ink);value_=label(screen_,"",75,86,210,&lv_font_montserrat_20,ink);hint_=label(screen_,"",75,116,210,&lv_font_montserrat_14,ink);
  const std::string rows[3]={nameSymbols_?"1234567890":"QWERTYUIOP",nameSymbols_?"-_":"ASDFGHJKL",nameSymbols_?"":"ZXCVBNM"};
  for(unsigned r=0;r<3;++r){int width=static_cast<int>(rows[r].size())*24-3,x=(360-width)/2;for(unsigned c=0;c<rows[r].size();++c){auto* key=button(screen_,std::string(1,rows[r][c]),x+24*c,232+23*r,21,21,NameKey,static_cast<unsigned>(rows[r][c]),0x163747);lv_obj_set_style_pad_all(key,0,0);auto* glyph=lv_obj_get_child(key,0);lv_obj_set_width(glyph,LV_SIZE_CONTENT);lv_obj_center(glyph);}}
  auto* toggle=button(screen_,nameSymbols_?"ABC":"123",92,305,41,22,NameSymbols,0,0x163747);auto* space=button(screen_,"SP",137,305,41,22,NameSpace,0,0x163747);auto* del=button(screen_,"DEL",182,305,41,22,NameDelete,0,0x163747);auto* save=button(screen_,"OK",227,305,41,22,NameConfirm,0,0x166853);for(auto* b:{toggle,space,del,save})lv_obj_set_style_pad_all(b,0,0);
 }else if(view_==View::HostJoin){
  radialSector("Host",0,2,HostGame,0,0x246e88,ToolIcon::LifeLink);radialSector("Join",1,2,FindGames,0,0x159870,ToolIcon::Plus);
  hint_=label(screen_,"",84,116,192,&lv_font_montserrat_14,ink);
 }else if(view_==View::BrowseGames){
  auto count=multiplayer_?multiplayer_->lobbies().size():0;unsigned pageCount=static_cast<unsigned>(std::max<std::size_t>(1,(count+2)/3));lobbyPage_%=pageCount;unsigned begin=lobbyPage_*3,visible=static_cast<unsigned>(std::min<std::size_t>(3,count-begin));
  if(!count){radialSector("No games yet",0,2,NoAction,0,0x246e88,ToolIcon::LifeLink);radialSector("Refresh",1,2,RefreshGames,0,0x159870,ToolIcon::Reset);}
  else{unsigned sectors=visible+1+(count>3?1:0);for(unsigned i=0;i<visible;++i){const auto& lobby=multiplayer_->lobbies()[begin+i];radialSector(lobby.name,i,sectors,JoinGame,begin+i,0x246e88,ToolIcon::LifeLink);}radialSector("Refresh",visible,sectors,RefreshGames,0,0x159870,ToolIcon::Reset);if(count>3)radialSector("Next",visible+1,sectors,NextGames,0,0x7c58ba,ToolIcon::Plus);}
 }else if(view_==View::NetworkLobby){
  auto* top=radialSector("",0,3,NoAction,0,0x246e88,ToolIcon::LifeLink);lv_obj_clean(top);
  label(screen_,multiplayer_?multiplayer_->hostName():"Game",87,49,186,&lv_font_montserrat_20,ink);
  label(screen_,multiplayer_?std::to_string(multiplayer_->players().size())+" / "+std::to_string(multiplayer_->capacity())+" players":"",95,83,170,&lv_font_montserrat_14,ink);
  hint_=label(screen_,"",65,110,230,&lv_font_montserrat_14,ink);
  bool host=multiplayer_&&multiplayer_->hosting();radialSector(host?"Start game":"Waiting",1,3,host?StartNetwork:NoAction,0,host?teal:0x327c68,ToolIcon::Confirm);
  radialSector(profile_->name(),2,3,NoAction,0,0x7c58ba,ToolIcon::LifeLink);
 }
 radialBack();
}
void AppUI::refreshMultiplayer(){
 if(view_==View::NameSetup){text(value_,nameDraft_.empty()?"Enter your name":nameDraft_);const auto* font=nameDraft_.size()>10?&lv_font_montserrat_14:&lv_font_montserrat_20;lv_obj_set_style_text_font(value_,font,0);lv_obj_set_height(value_,font->line_height);text(hint_,notice_.empty()?std::to_string(nameDraft_.size())+" / 12":notice_);}
 else if(hint_)text(hint_,!notice_.empty()?notice_:multiplayer_?multiplayer_->error():"Network unavailable");
}
}
