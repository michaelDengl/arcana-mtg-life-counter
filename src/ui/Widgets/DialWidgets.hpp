#pragma once
#include "lvgl.h"
#include <string>
namespace mtg::widgets {
constexpr unsigned background=0x07131e, panel=0x0b2130, muted=0x81a5b7, ink=0xf5fbff;
constexpr unsigned violet=0x00d8ff, gold=0xf4c678, teal=0x00e5b1, red=0xff3e70;
inline lv_obj_t* label(lv_obj_t* parent,const std::string& value,int x,int y,int width=0,
                      const lv_font_t* font=&lv_font_montserrat_14,unsigned color=ink) {
 auto* obj=lv_label_create(parent);lv_label_set_text(obj,value.c_str());
 lv_obj_set_style_text_font(obj,font,0);lv_obj_set_style_text_color(obj,lv_color_hex(color),0);
 if(width) {lv_obj_set_width(obj,width);lv_obj_set_style_text_align(obj,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(obj,LV_LABEL_LONG_DOT);lv_obj_set_height(obj,font->line_height);}
 lv_obj_set_pos(obj,x,y);return obj;
}
inline void text(lv_obj_t* obj,const std::string& value) {
 if(value!=lv_label_get_text(obj)) lv_label_set_text(obj,value.c_str());
}
inline lv_obj_t* box(lv_obj_t* parent,int x,int y,int w,int h,unsigned color=panel) {
 auto* obj=lv_obj_create(parent);lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,w,h);
 lv_obj_set_style_pad_all(obj,0,0);lv_obj_set_style_border_width(obj,0,0);
 lv_obj_set_style_bg_color(obj,lv_color_hex(color),0);lv_obj_set_style_radius(obj,14,0);
 lv_obj_remove_flag(obj,LV_OBJ_FLAG_SCROLLABLE);return obj;
}
inline void number(lv_obj_t* obj,const std::string& value) {
 text(obj,value);const auto* font=value.size()>10?&lv_font_montserrat_20:&lv_font_montserrat_48;
 lv_obj_set_style_text_font(obj,font,0);lv_obj_set_height(obj,font->line_height);
}
}
