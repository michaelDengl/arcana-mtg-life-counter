#pragma once
#include "lvgl.h"
namespace mtg::widgets {
enum class ToolIcon {Treasure,Poison,Energy,Experience,Storm,Rad,Shield,Coin,D20,End,Reset,Brightness,Confirm,Cancel,Plus,Minus,LifeLink};
ToolIcon counterIcon(unsigned counter);
unsigned counterColor(unsigned counter);
void drawToolIcon(lv_obj_t* parent,ToolIcon icon,int x,int y,unsigned color);
}
