#include "ToolIcons.hpp"
#include "DialWidgets.hpp"
#include "core/Player.hpp"
LV_FONT_DECLARE(mtg_symbols);
namespace mtg::widgets {
namespace {
void rectangle(lv_obj_t* parent,int x,int y,int w,int h,unsigned color,int radius=0) {
 auto* o=box(parent,x,y,w,h,color);lv_obj_set_style_radius(o,radius,0);
 lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);
}
void line(lv_obj_t* parent,const lv_point_precise_t* points,unsigned count,unsigned color) {
 auto* l=lv_line_create(parent);lv_line_set_points(l,points,count);lv_obj_set_style_line_width(l,2,0);lv_obj_set_style_line_color(l,lv_color_hex(color),0);lv_obj_set_style_line_rounded(l,true,0);lv_obj_remove_flag(l,LV_OBJ_FLAG_CLICKABLE);
}
}
unsigned counterColor(unsigned counter) {
 switch(static_cast<Counter>(counter)) {
  case Counter::Treasure:return gold;case Counter::Poison:return teal;
  case Counter::Energy:return 0xe5a833;case Counter::Experience:return 0x7c58ba;
  case Counter::Spells:return 0x468ca9;case Counter::Rad:return 0x349353;
  case Counter::Shield:return 0x607eb1;default:return violet;
 }
}
ToolIcon counterIcon(unsigned counter) {
 switch(static_cast<Counter>(counter)) {
  case Counter::Poison:return ToolIcon::Poison;case Counter::Energy:return ToolIcon::Energy;
  case Counter::Experience:return ToolIcon::Experience;case Counter::Spells:return ToolIcon::Storm;
  case Counter::Rad:return ToolIcon::Rad;case Counter::Shield:return ToolIcon::Shield;
  default:return ToolIcon::Treasure;
 }
}
void drawToolIcon(lv_obj_t* parent,ToolIcon icon,int x,int y,unsigned color) {
 auto* root=box(parent,x,y,30,30);lv_obj_set_style_bg_opa(root,0,0);lv_obj_remove_flag(root,LV_OBJ_FLAG_CLICKABLE);
 if(icon==ToolIcon::Treasure) {
  // Original chest artwork, not an asserted official Wizards symbol.
  static const lv_point_precise_t lid[]{{3,13},{5,5},{23,5},{27,13},{3,13}};
  static const lv_point_precise_t body[]{{4,16},{4,26},{26,26},{26,16}};
  static const lv_point_precise_t rim[]{{2,16},{28,16}};
  line(root,lid,5,color);line(root,body,4,color);line(root,rim,2,color);
  rectangle(root,12,13,6,8,color,1);rectangle(root,7,5,2,8,color);rectangle(root,21,5,2,8,color);
 }else if(icon==ToolIcon::Poison || icon==ToolIcon::Energy || icon==ToolIcon::Experience || icon==ToolIcon::Rad || icon==ToolIcon::Shield) {
  const char* glyph=icon==ToolIcon::Poison?"\xee\x98\x98":icon==ToolIcon::Energy?"\xee\xa4\x87":icon==ToolIcon::Experience?"\xee\xa4\x80":icon==ToolIcon::Rad?"\xee\xa9\x90":"\xee\xa7\x83";
  auto* symbol=label(root,glyph,0,0,30,&mtg_symbols,color);lv_obj_center(symbol);
 }else if(icon==ToolIcon::Storm) {
  rectangle(root,2,8,25,10,color,5);rectangle(root,7,3,12,14,color,6);
  static const lv_point_precise_t bolt[]{{17,17},{10,23},{17,23},{12,29}};line(root,bolt,4,color);
 }else if(icon==ToolIcon::Plus||icon==ToolIcon::Minus) {
  auto* glyph=label(root,icon==ToolIcon::Plus?"+":"-",0,0,30,&lv_font_montserrat_32,color);lv_obj_center(glyph);
 }else if(icon==ToolIcon::LifeLink) {
  for(int i=0;i<2;++i){auto* link=box(root,2+10*i,7+7*i,17,10,color);lv_obj_set_style_bg_opa(link,0,0);lv_obj_set_style_radius(link,5,0);lv_obj_set_style_border_width(link,2,0);lv_obj_set_style_border_color(link,lv_color_hex(color),0);lv_obj_remove_flag(link,LV_OBJ_FLAG_CLICKABLE);}
  static const lv_point_precise_t join[]{{10,13},{20,18}};line(root,join,2,color);
 }else if(icon==ToolIcon::Brightness) {
  auto* sun=box(root,9,9,12,12,color);lv_obj_set_style_radius(sun,LV_RADIUS_CIRCLE,0);lv_obj_remove_flag(sun,LV_OBJ_FLAG_CLICKABLE);
  rectangle(root,14,1,2,5,color);rectangle(root,14,24,2,5,color);rectangle(root,1,14,5,2,color);rectangle(root,24,14,5,2,color);
  static const lv_point_precise_t rays[][2]={{{4,4},{7,7}},{{23,23},{26,26}},{{4,26},{7,23}},{{23,7},{26,4}}};for(auto& ray:rays)line(root,ray,2,color);
 }else if(icon==ToolIcon::Coin) {
  auto* coin=box(root,2,2,26,26,background);lv_obj_set_style_bg_opa(coin,0,0);lv_obj_set_style_radius(coin,LV_RADIUS_CIRCLE,0);lv_obj_set_style_border_width(coin,2,0);lv_obj_set_style_border_color(coin,lv_color_hex(color),0);lv_obj_remove_flag(coin,LV_OBJ_FLAG_CLICKABLE);
  label(root,"H",6,6,18,&lv_font_montserrat_14,color);
 }else if(icon==ToolIcon::D20) {
  static const lv_point_precise_t dice[]{{15,1},{28,8},{28,22},{15,29},{2,22},{2,8},{15,1}};
  line(root,dice,7,color);label(root,"20",3,7,24,&lv_font_montserrat_14,color);
 }else {
  label(root,icon==ToolIcon::Confirm?LV_SYMBOL_OK:icon==ToolIcon::Cancel?LV_SYMBOL_CLOSE:icon==ToolIcon::End?LV_SYMBOL_POWER:LV_SYMBOL_REFRESH,1,2,28,&lv_font_montserrat_20,color);
 }
}
}
