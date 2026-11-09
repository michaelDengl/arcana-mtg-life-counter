#include "AppUI.hpp"
#include "Widgets/DialWidgets.hpp"
#include "Widgets/ToolIcons.hpp"
// LVGL 9.2 keeps its custom hit-test payload in this pinned-version header.
#include "src/core/lv_obj_event_private.h"
#include <algorithm>
#include <cmath>
LV_FONT_DECLARE(life_digits);
LV_FONT_DECLARE(heart_symbol);
LV_FONT_DECLARE(tab_font);
namespace mtg {
using namespace widgets;
namespace {
// Match the interactive region to the circular mask, including partially cut buttons.
void circularClipHit(lv_event_t* event) {
 auto* hit=static_cast<lv_hit_test_info_t*>(lv_event_get_param(event));
 auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
 for(auto* parent=obj;parent;parent=lv_obj_get_parent(parent)) {
  if(!lv_obj_get_style_clip_corner(parent,0))continue;
  lv_area_t area;lv_obj_get_coords(parent,&area);
  auto diameter=std::min(lv_area_get_width(&area),lv_area_get_height(&area));
  if(lv_obj_get_style_radius(parent,0)<diameter/2)continue;
  std::int64_t dx=2*hit->point->x-area.x1-area.x2,dy=2*hit->point->y-area.y1-area.y2;
  if(dx*dx+dy*dy>static_cast<std::int64_t>(diameter)*diameter){hit->res=false;return;}
 }
}
lv_obj_t* heart(lv_obj_t* parent,int x,int y) {
 // One antialiased outline avoids seams between separately rotated shapes.
 return label(parent,"\xef\x80\x84",x,y,36,&heart_symbol,0xff2449);
}
lv_obj_t* arc(lv_obj_t* parent,int size,int y,unsigned from,unsigned to,unsigned color,int width) {
 auto* a=lv_arc_create(parent);lv_obj_set_size(a,size,size);lv_obj_set_pos(a,(360-size)/2,y);
 lv_obj_set_style_bg_opa(a,0,LV_PART_KNOB);lv_obj_set_style_arc_opa(a,0,LV_PART_INDICATOR);
 lv_obj_set_style_arc_width(a,width,LV_PART_MAIN);lv_obj_set_style_arc_color(a,lv_color_hex(color),LV_PART_MAIN);
 lv_obj_set_style_arc_rounded(a,false,LV_PART_MAIN);lv_arc_set_bg_angles(a,from,to);
 lv_obj_remove_flag(a,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_pad_all(a,0,0);return a;
}
void lifeSegments(lv_obj_t* screen) {
 // Anchor actual segment starts at 270 degrees (top) and 90 degrees (bottom).
 lv_obj_t* halves[2];
 for(unsigned side=0;side<2;++side){halves[side]=box(screen,72+108*side,82,108,216);lv_obj_set_style_bg_opa(halves[side],0,0);lv_obj_set_style_radius(halves[side],0,0);lv_obj_set_style_clip_corner(halves[side],true,0);lv_obj_remove_flag(halves[side],LV_OBJ_FLAG_CLICKABLE);}
 for(unsigned i=0;i<16;++i) {
  unsigned from=(270+static_cast<unsigned>(std::round(i*22.5)))%360,to=from+17;
  unsigned side=(from<90 || from>=270)?1:0;
  auto* segment=arc(halves[side],216,0,from,to,side?teal:red,4);lv_obj_set_x(segment,side?-108:0);
 }
}

}
void AppUI::dialFrame() {
 arc(screen_,348,6,0,360,violet,2);
 if(view_!=View::Home || selectedPlayer_!=1)arc(screen_,338,11,0,360,violet,1);
}
void AppUI::playerWedgeHit(lv_event_t* event) {
 auto* binding=static_cast<Binding*>(lv_event_get_user_data(event));auto& self=*binding->ui;
 auto* hit=static_cast<lv_hit_test_info_t*>(lv_event_get_param(event));lv_area_t area;lv_obj_get_coords(self.screen_,&area);
 double dx=hit->point->x-area.x1-180,dy=hit->point->y-area.y1-180,r2=dx*dx+dy*dy;
 auto count=self.game_.state().playerCount-1;if(!count){hit->res=false;return;}
 double angle=std::atan2(dy,dx)*180/3.141592653589793;if(angle<0)angle+=360;
 double start=225.0+(binding->arg-2)*90.0/count,end=start+90.0/count;
 hit->res=r2>=133.0*133.0 && r2<=164.0*164.0 && angle>=start && angle<=end;
}
void AppUI::playerTabs() {
 const unsigned colors[]{0xb62e51,0xb9a33e,0x23966b,0x497e9e,0x7d4ba1,0xba7146};
 auto count=game_.state().playerCount-1;if(count==0)return;const double span=90.0/count;
 for(unsigned i=1;i<=count;++i) {
  double start=225.0+(i-1)*span,middle=start+span/2,angle=middle*3.141592653589793/180.0;
  auto from=static_cast<unsigned>(start+1),to=static_cast<unsigned>(start+span-1);
  arc(screen_,326,17,from,to,colors[i],29);
  if(selectedPlayer_==i+1) {
   arc(screen_,326,17,from,to,violet,1);arc(screen_,270,45,from,to,violet,1);
   for(double edge:{start+1,start+span-1}) {
    double theta=edge*3.141592653589793/180.0;
    tabEdgePoints_.push_back({lv_point_precise_t{static_cast<int>(std::round(180+134*std::cos(theta))),static_cast<int>(std::round(180+134*std::sin(theta)))},lv_point_precise_t{static_cast<int>(std::round(180+163*std::cos(theta))),static_cast<int>(std::round(180+163*std::sin(theta)))}});
    auto* line=lv_line_create(screen_);lv_line_set_points(line,tabEdgePoints_.back().data(),2);lv_obj_set_style_line_width(line,1,0);lv_obj_set_style_line_color(line,lv_color_hex(violet),0);lv_obj_remove_flag(line,LV_OBJ_FLAG_CLICKABLE);
   }
  }
  // The transparent target covers the complete annular sector, not just its text.
  auto* b=button(screen_,"",17,17,326,326,SelectPlayer,i+1);auto* binding=&bindings_.back();
  lv_obj_set_style_pad_all(b,0,0);lv_obj_set_style_bg_opa(b,0,0);lv_obj_set_style_bg_opa(b,0,LV_STATE_PRESSED);lv_obj_set_style_border_width(b,0,0);lv_obj_set_style_radius(b,0,0);lv_obj_clean(b);
  lv_obj_add_flag(b,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(b,playerWedgeHit,LV_EVENT_HIT_TEST,binding);
  const auto* font=count>3?&tab_font:&lv_font_montserrat_14;int w=count>3?48:62,h=font->line_height*2;
  int x=static_cast<int>(std::round(180+148*std::cos(angle))),y=static_cast<int>(std::round(180+148*std::sin(angle)));
  auto* caption=box(b,x-17-w/2,y-17-h/2,w,h);lv_obj_set_style_bg_opa(caption,0,0);lv_obj_set_style_radius(caption,0,0);lv_obj_remove_flag(caption,LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_transform_pivot_x(caption,w/2,0);lv_obj_set_style_transform_pivot_y(caption,h/2,0);
  int rotation=static_cast<int>(std::round((middle-270)*10));lv_obj_set_style_transform_rotation(caption,(rotation+3600)%3600,0);
  label(caption,"P"+std::to_string(i+1),0,0,w,font);
  tabValues_[i]=label(caption,"",0,font->line_height,w,font);
 }
}
void AppUI::buildLifecycle() {
 if(view_==View::Startup) {
  label(screen_,"A R C A N A",65,67,230,&lv_font_montserrat_20,violet);
  label(screen_,"COMMANDER LIFE COUNTER",55,103,250,&lv_font_montserrat_14,muted);
  button(screen_,"New game",83,147,194,48,NewGame);
  auto* resume=button(screen_,"Continue",83,213,194,48,ContinueGame);
  if(!game_.state().active)lv_obj_add_state(resume,LV_STATE_DISABLED);
  hint_=label(screen_,"",65,283,230,&lv_font_montserrat_14,muted);
 }else if(view_==View::PlayerSetup) {
  label(screen_,"NEW GAME",70,69,220,&lv_font_montserrat_20,violet);
  label(screen_,"Players in this match",60,111,240,&lv_font_montserrat_14,muted);
  value_=label(screen_,"",45,153,270,&lv_font_montserrat_32);
  label(screen_,"Turn knob to choose / 1 - 6",55,213,250,&lv_font_montserrat_14,violet);
  button(screen_,"Confirm",76,265,101,42,ConfirmPlayers);button(screen_,"Back",183,265,101,42,Back);

 }
}
void AppUI::refreshLifecycle() {
 if(view_==View::PlayerSetup)text(value_,std::to_string(draftPlayers_)+"-Players");
 if(view_==View::Startup) {
  text(hint_,!storageError_.empty()?"SAVE ERROR / match not saved":game_.state().active?"Unfinished match ready":"Ready for a new match");
  lv_obj_set_style_text_color(hint_,lv_color_hex(storageError_.empty()?muted:red),0);
 }
}
void AppUI::buildDial() {
 if(selectedPlayer_==1)playerTabs();
 if(selectedPlayer_==1) {
  counterRockers();
  auto* rim=box(screen_,68,78,224,224,background);lv_obj_set_style_radius(rim,LV_RADIUS_CIRCLE,0);lv_obj_set_style_clip_corner(rim,true,0);
  lv_obj_add_flag(rim,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(rim,circularClipHit,LV_EVENT_HIT_TEST,nullptr);
  auto* lifeCircle=box(rim,4,4,216,216,background);
  lv_obj_set_style_radius(lifeCircle,LV_RADIUS_CIRCLE,0);lv_obj_set_style_clip_corner(lifeCircle,true,0);
  lv_obj_add_flag(lifeCircle,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(lifeCircle,circularClipHit,LV_EVENT_HIT_TEST,nullptr);
  for(unsigned side=0;side<2;++side){
   auto* half=box(lifeCircle,108*side,0,108,216,side?0x09281f:0x29131f);lv_obj_set_style_radius(half,0,0);
   lv_obj_add_flag(half,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(half,circularClipHit,LV_EVENT_HIT_TEST,nullptr);
   bindings_.push_back({this,side?Plus:Minus,side});lv_obj_add_event_cb(half,lifeTouch,LV_EVENT_ALL,&bindings_.back());
  }
  auto* upper=box(screen_,180,82,1,108,teal);lv_obj_remove_flag(upper,LV_OBJ_FLAG_CLICKABLE);
  auto* lower=box(screen_,180,190,1,108,red);lv_obj_remove_flag(lower,LV_OBJ_FLAG_CLICKABLE);
  battery_=box(screen_,166,132,28,14,background);lv_obj_set_style_bg_opa(battery_,0,0);lv_obj_remove_flag(battery_,LV_OBJ_FLAG_CLICKABLE);
  value_=label(screen_,"4",94,161,68,&life_digits,ink);
  lifeRight_=label(screen_,"0",198,161,68,&life_digits,ink);
  lifeSegments(screen_);
  arc(screen_,224,78,0,360,violet,2);
  lifeMarker_=heart(screen_,162,223);
  status_=label(screen_,"",95,260,170,&lv_font_montserrat_14,teal);
  lv_label_set_long_mode(status_,LV_LABEL_LONG_WRAP);lv_obj_set_height(status_,34);
  partialDividers();
  counterShortcuts();
  auto* gear=button(screen_,LV_SYMBOL_SETTINGS,158,168,44,44,OpenGear);
  lv_obj_set_style_radius(gear,LV_RADIUS_CIRCLE,0);lv_obj_set_style_pad_all(gear,0,0);lv_obj_set_style_border_color(gear,lv_color_hex(violet),0);
  auto* glyph=lv_obj_get_child(gear,0);lv_obj_set_style_text_font(glyph,&lv_font_montserrat_32,0);
  lv_obj_set_width(glyph,LV_SIZE_CONTENT);lv_obj_set_height(glyph,LV_SIZE_CONTENT);lv_obj_center(glyph);
  undoArc_=arc(screen_,326,17,60,120,0x303c43,29);
  undo_=button(screen_,"Undo",17,17,326,326,RecentUndo);
  lv_obj_set_style_bg_opa(undo_,0,0);lv_obj_set_style_bg_opa(undo_,0,LV_STATE_PRESSED);lv_obj_set_style_bg_opa(undo_,0,LV_STATE_DISABLED);lv_obj_set_style_border_width(undo_,0,0);lv_obj_set_style_radius(undo_,0,0);lv_obj_set_style_pad_all(undo_,0,0);
  lv_obj_add_flag(undo_,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(undo_,undoSectorHit,LV_EVENT_HIT_TEST,this);
  auto* undoText=lv_obj_get_child(undo_,0);lv_obj_set_align(undoText,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(undoText,85,302);lv_obj_set_size(undoText,156,18);

 }else {
  auto* total=radialSector("",0,5,NoAction,0,0x246e88,ToolIcon::D20);lv_obj_clean(total);
  label(screen_,"P"+std::to_string(selectedPlayer_)+" / CMD",117,47,126,&lv_font_montserrat_14,ink);
  value_=label(screen_,"0",120,69,120,&lv_font_montserrat_32,ink);
  label(screen_,"LIMIT 21",133,120,94,&tab_font,ink);
  radialSector("Add 1",1,5,Plus,0,teal,ToolIcon::Plus);
  auto* mode=radialSector("",2,5,Mode,0,0x327c68,ToolIcon::LifeLink);mode_=lv_obj_get_child(mode,1);
  lv_obj_set_style_text_font(mode_,&tab_font,0);lv_obj_set_height(mode_,30);lv_label_set_long_mode(mode_,LV_LABEL_LONG_WRAP);
  auto* source=radialSector("",3,5,CycleSource,0,0x7c58ba,ToolIcon::Reset);detail_=lv_obj_get_child(source,1);
  lv_obj_set_style_text_font(detail_,&tab_font,0);lv_obj_set_height(detail_,30);lv_label_set_long_mode(detail_,LV_LABEL_LONG_WRAP);
  radialSector("Remove 1",4,5,Minus,0,red,ToolIcon::Minus);
  radialBack(CloseDial);

 }
}
bool AppUI::lifeHoldPositionValid() const {
 auto point=hardware_.touch();lv_area_t area;lv_obj_get_coords(screen_,&area);
 int x=point.x-area.x1,y=point.y-area.y1,dx=x-180,dy=y-190;
 return dx*dx+dy*dy<=108*108 && dx*dx+dy*dy>23*23 && (lifeHoldSign_>0?x>=180:x<180);
}
void AppUI::repeatLifeHold(std::uint32_t now) {
 if(!lifeHoldActive_||lifeHoldCancelled_)return;
 if(view_!=View::Home||selectedPlayer_!=1||!hardware_.powered()||!lifeHoldPositionValid()){lifeHoldActive_=false;lifeHoldCancelled_=true;return;}
 auto due=static_cast<std::uint32_t>(now-lifeHoldStarted_)/1000;
 if(due>lifeHoldRepeats_){game_.setActionTime(now);game_.adjustLife(static_cast<std::int64_t>(due-lifeHoldRepeats_)*5*lifeHoldSign_);lifeHoldRepeats_=due;}
}
void AppUI::lifeTouch(lv_event_t* event) {
 auto* binding=static_cast<Binding*>(lv_event_get_user_data(event));auto& self=*binding->ui;
 if(!self.hardware_.powered())return;
 switch(lv_event_get_code(event)) {
  case LV_EVENT_PRESSED:self.lifeHoldActive_=true;self.lifeHoldCancelled_=false;self.lifeHoldSign_=binding->arg?1:-1;self.lifeHoldStarted_=lv_tick_get();self.lifeHoldRepeats_=0;break;
  case LV_EVENT_RELEASED:self.repeatLifeHold(lv_tick_get());self.lifeHoldActive_=false;break;
  case LV_EVENT_PRESS_LOST:self.lifeHoldActive_=false;self.lifeHoldCancelled_=true;break;
  case LV_EVENT_CLICKED:
   if(!self.lifeHoldCancelled_ && self.lifeHoldRepeats_==0 && self.view_==View::Home && self.selectedPlayer_==1 && self.lifeHoldPositionValid()){self.game_.setActionTime(lv_tick_get());self.game_.adjustLife(binding->arg?1:-1);}break;
  default:break;
 }
}
lv_obj_t* AppUI::roundPopup() {
 auto* modal=box(screen_,37,61,286,286);lv_obj_set_style_radius(modal,LV_RADIUS_CIRCLE,0);
 lv_obj_set_style_border_width(modal,2,0);lv_obj_set_style_border_color(modal,lv_color_hex(violet),0);
 lv_obj_set_style_clip_corner(modal,true,0);lv_obj_remove_flag(modal,LV_OBJ_FLAG_CLICKABLE);return modal;
}
lv_obj_t* AppUI::cutButton(lv_obj_t* circle,const std::string& value,int x,int y,int w,int h,Action action,std::uint32_t arg) {
 lv_obj_update_layout(circle);lv_area_t parentArea,screenArea;lv_obj_get_coords(circle,&parentArea);lv_obj_get_coords(screen_,&screenArea);
 auto* control=button(circle,value,x-(parentArea.x1-screenArea.x1),y-(parentArea.y1-screenArea.y1),w,h,action,arg);
 lv_obj_add_flag(control,LV_OBJ_FLAG_ADV_HITTEST);lv_obj_add_event_cb(control,circularClipHit,LV_EVENT_HIT_TEST,nullptr);
 double radius=std::min(lv_area_get_width(&parentArea),lv_area_get_height(&parentArea))/2.0;
 double cx=(parentArea.x1+parentArea.x2)/2.0-screenArea.x1,cy=(parentArea.y1+parentArea.y2)/2.0-screenArea.y1;
 double dy=y+h/2.0-cy,half=std::sqrt(std::max(0.0,radius*radius-dy*dy));
 double visibleCenter=(std::max(static_cast<double>(x),cx-half)+std::min(static_cast<double>(x+w),cx+half))/2;
 lv_obj_align(lv_obj_get_child(control,0),LV_ALIGN_CENTER,static_cast<int>(std::round(visibleCenter-x-w/2.0)),0);
 return control;
}
void AppUI::partialDividers() {
 // Use the same two-pixel stroke and two-pixel clear gap as the inner rings.
 constexpr int stroke=2,gap=2,diameter=348-2*(stroke+gap);
 arc(screen_,diameter,(360-diameter)/2,204,220,violet,stroke);
 arc(screen_,diameter,(360-diameter)/2,320,336,violet,stroke);
 arc(screen_,diameter,(360-diameter)/2,38,142,violet,stroke);
 // Derive both ends from the circle intersections, so the joins touch exactly.
 static const auto turns=[] {
  std::array<std::array<lv_point_precise_t,2>,4> paths{};
  constexpr double radius=diameter/2.0-stroke/2.0;
  unsigned index=0;
  for(unsigned angle:{204u,38u}) {
   int y=static_cast<int>(std::round(180+radius*std::sin(angle*3.141592653589793/180)));
   int outerX=static_cast<int>(std::round(180-std::sqrt(radius*radius-(y-180)*(y-180))));
   int innerX=static_cast<int>(std::round(180-std::sqrt(112.0*112.0-(y-190)*(y-190))));
   paths[index++]={lv_point_precise_t{outerX,y},lv_point_precise_t{innerX,y}};
   paths[index++]={lv_point_precise_t{360-innerX,y},lv_point_precise_t{360-outerX,y}};
  }
  return paths;
 }();
 for(const auto& points:turns) {
  auto* line=lv_line_create(screen_);lv_line_set_points(line,points.data(),2);
  lv_obj_set_style_line_color(line,lv_color_hex(violet),0);lv_obj_set_style_line_width(line,2,0);
  lv_obj_set_style_line_rounded(line,true,0);lv_obj_remove_flag(line,LV_OBJ_FLAG_CLICKABLE);
 }
}
void AppUI::counterRockers() {
 auto* outer=box(screen_,8,8,344,344);lv_obj_set_style_bg_opa(outer,LV_OPA_TRANSP,0);
 lv_obj_set_style_radius(outer,LV_RADIUS_CIRCLE,0);lv_obj_set_style_clip_corner(outer,true,0);lv_obj_remove_flag(outer,LV_OBJ_FLAG_CLICKABLE);
 for(unsigned side=0;side<2;++side) {
  for(unsigned up=0;up<2;++up) {
   auto* b=cutButton(outer,up?LV_SYMBOL_PLUS:LV_SYMBOL_MINUS,side?219:-4,up?115:229,145,up?51:52,SideStep,side*2+up);
   auto accent=up?teal:red;
   lv_obj_set_style_pad_all(b,0,0);
   lv_obj_set_style_border_color(b,lv_color_hex(accent),0);lv_obj_set_style_border_width(b,2,0);
   lv_obj_set_style_bg_color(b,lv_color_hex(up?0x092b2a:0x28131f),0);lv_obj_set_style_radius(b,8,0);
   auto* mark=lv_obj_get_child(b,0);lv_obj_set_width(mark,34);lv_obj_set_height(mark,36);
   lv_obj_set_style_text_font(mark,&lv_font_montserrat_32,0);lv_obj_set_style_text_color(mark,lv_color_hex(accent),0);
   double buttonY=up?115:229,buttonH=up?51:52,centerY=buttonY+buttonH/2;
   double outerLeft=180-std::sqrt(172.0*172.0-(centerY-180)*(centerY-180));
   double innerLeft=180-std::sqrt(112.0*112.0-(centerY-190)*(centerY-190));
   double centerX=(outerLeft+innerLeft)/2;if(side)centerX=360-centerX;
   lv_obj_set_align(mark,LV_ALIGN_TOP_LEFT);
   lv_obj_set_pos(mark,static_cast<int>(std::round(centerX-(side?219:-4)-17)),static_cast<int>(std::round((buttonH-36)/2)));
   // Center the visible symbol rather than Font Awesome's asymmetric bearings.
   lv_obj_set_style_translate_x(mark,-2,0);lv_obj_set_style_translate_y(mark,up?-1:0,0);
  }
 }
}
void AppUI::curvedUndoText(const std::string& value) {
 if(value==undoCurvedText_)return;undoCurvedText_=value;
 // Keep the complete label for semantic lookup; render its letters on the arc.
 auto* semantic=lv_obj_get_child(undo_,0);text(semantic,value);lv_obj_set_style_text_opa(semantic,0,0);
 while(lv_obj_get_child_count(undo_)>1)lv_obj_delete(lv_obj_get_child(undo_,1));
 const auto* font=&lv_font_montserrat_14;
 double width=lv_text_get_width(value.c_str(),static_cast<uint32_t>(value.size()),font,0),offset=-width/2;
 constexpr double radius=148,pi=3.141592653589793;
 for(char character:value) {
  std::string glyph(1,character);int advance=lv_text_get_width(glyph.c_str(),1,font,0);
  double angle=pi/2-(offset+advance/2.0)/radius;
  if(character!=' '){
   auto* letter=label(undo_,glyph,0,0,0,font,ink);lv_obj_set_size(letter,advance+2,font->line_height);
   lv_obj_set_pos(letter,static_cast<int>(std::round(163+radius*std::cos(angle)-(advance+2)/2.0)),static_cast<int>(std::round(163+radius*std::sin(angle)-font->line_height/2.0)));
   lv_obj_set_style_transform_pivot_x(letter,(advance+2)/2,0);lv_obj_set_style_transform_pivot_y(letter,font->line_height/2,0);
   int rotation=static_cast<int>(std::round((angle*180/pi-90)*10));lv_obj_set_style_transform_rotation(letter,(rotation+3600)%3600,0);lv_obj_remove_flag(letter,LV_OBJ_FLAG_CLICKABLE);
  }
  offset+=advance;
 }
}
void AppUI::undoSectorHit(lv_event_t* event) {
 auto* self=static_cast<AppUI*>(lv_event_get_user_data(event));auto* hit=static_cast<lv_hit_test_info_t*>(lv_event_get_param(event));lv_area_t area;lv_obj_get_coords(self->screen_,&area);
 double x=hit->point->x-area.x1-180,y=hit->point->y-area.y1-180;double angle=std::atan2(y,x)*180/3.141592653589793;
 hit->res=x*x+y*y>=134*134&&x*x+y*y<=164*164&&angle>=60&&angle<=120;
}
void AppUI::counterShortcuts() {
 for(unsigned i=0;i<2;++i) {
  auto counter=game_.state().shortcuts[i];auto color=violet;
  auto* b=button(screen_,"",i?303:13,173,44,44,OpenPicker,i);
  lv_obj_set_style_pad_all(b,0,0);lv_obj_set_style_radius(b,LV_RADIUS_CIRCLE,0);lv_obj_set_style_border_color(b,lv_color_hex(color),0);lv_obj_set_style_border_width(b,2,0);lv_obj_clean(b);
  drawToolIcon(b,counterIcon(static_cast<unsigned>(counter)),6,4,counterColor(static_cast<unsigned>(counter)));
  // Root-level badges avoid being clipped by the shortcut button's rounded edge.
  auto* badge=button(screen_,"",i?323:34,201,22,22,OpenPicker,i);
  lv_obj_set_style_pad_all(badge,0,0);lv_obj_set_style_radius(badge,LV_RADIUS_CIRCLE,0);lv_obj_set_style_border_width(badge,1,0);lv_obj_set_style_border_color(badge,lv_color_hex(color),0);lv_obj_set_style_bg_color(badge,lv_color_hex(background),0);lv_obj_clean(badge);
  auto* count=label(badge,"0",0,0,18,&lv_font_montserrat_14,color);lv_obj_center(count);if(i)poison_=count;else aux_=count;
 }
}
void AppUI::buildPicker() {
 const Counter counters[]{Counter::Treasure,Counter::Poison,Counter::Energy,Counter::Experience,Counter::Spells,Counter::Rad,Counter::Shield};
 for(unsigned i=0;i<7;++i){auto c=counters[i];auto* tile=radialSector(c==Counter::Spells?"STORM":counterName(c),i,7,ChooseShortcut,static_cast<unsigned>(c),counterColor(static_cast<unsigned>(c)),counterIcon(static_cast<unsigned>(c)));
  auto* title=lv_obj_get_child(tile,1);lv_obj_set_style_text_font(title,&tab_font,0);lv_obj_set_height(title,tab_font.line_height);
  double angle=(270+360.0*i/7)*3.141592653589793/180;int x=150+112*std::cos(angle),y=150+112*std::sin(angle);
  label(tile,std::to_string(game_.player().counters[static_cast<unsigned>(c)]),x-27,y+19,54,&lv_font_montserrat_14,0xffffff);
  if(c==game_.state().shortcuts[pickerSide_]){auto* mark=label(tile,LV_SYMBOL_OK,x-8,y-42,16,&lv_font_montserrat_14,0xffffff);lv_obj_remove_flag(mark,LV_OBJ_FLAG_CLICKABLE);}
 }
 radialBack();
}
void AppUI::refreshTabs() {
 const auto& state=game_.state();
 for(unsigned i=0;i<state.playerCount;++i)if(tabValues_[i]) {
  std::int64_t value=game_.player().life;
  if(i>0){value=0;for(const auto& s:state.sources)if(s.owner==i+1)value=std::max(value,s.damage);}
  text(tabValues_[i],std::to_string(value));
 }
 }
void AppUI::refreshBattery() {
 if(!battery_)return;
 auto reading=hardware_.battery();unsigned percent=std::min(reading.percent,100u);
 int state=!reading.available?-1:reading.charging?6:percent<=5?0:percent<=20?1:percent<=40?2:percent<=60?3:percent<=80?4:5;
 if(state==batteryState_)return;batteryState_=state;lv_obj_clean(battery_);
 auto part=[&](int x,int y,int w,int h,unsigned color){auto* obj=box(battery_,x,y,w,h,color);lv_obj_set_style_radius(obj,0,0);lv_obj_remove_flag(obj,LV_OBJ_FLAG_CLICKABLE);return obj;};
 auto* body=part(0,1,24,12,background);lv_obj_set_style_radius(body,2,0);lv_obj_set_style_border_width(body,1,0);lv_obj_set_style_border_color(body,lv_color_hex(ink),0);
 part(24,5,2,4,ink);
 const unsigned colors[]={0xd52942,0xd52942,0xf15b29,0xf6b541,0x4db748,0x00854c,0x00854c};
 if(state>=1&&state<=5)for(int i=0;i<state;++i)part(2+4*i,3,3,8,colors[state]);
 if(state==0||state==6){
  if(state==6)part(2,3,20,8,colors[state]);
  static const lv_point_precise_t bolt[]={{14,2},{10,7},{14,7},{10,12}};
  auto* line=lv_line_create(battery_);lv_line_set_points(line,bolt,4);lv_obj_set_style_line_width(line,2,0);lv_obj_set_style_line_color(line,lv_color_hex(state==0?colors[0]:ink),0);lv_obj_remove_flag(line,LV_OBJ_FLAG_CLICKABLE);
 }
 if(state<0)part(9,6,6,1,muted);
}
void AppUI::refreshDial() {
 refreshBattery();
 refreshTabs();
 if(selectedPlayer_==1) {
  auto number=std::to_string(game_.player().life);if(game_.player().life>=0 && number.size()<2)number="0"+number;auto split=(number.size()+1)/2;
  auto left=number.substr(0,split),right=number.substr(split);auto longest=std::max(left.size(),right.size());
  const auto* font=longest<=1?&life_digits:longest<=2?&lv_font_montserrat_48:longest<=3?&lv_font_montserrat_32:longest<=6?&lv_font_montserrat_20:&lv_font_montserrat_14;
  int width=longest<=1?68:longest<=3?76:longest<=6?88:92;
  int leftX=longest<=1?94:longest<=3?90:longest<=6?76:70;
  int rightX=longest<=1?198:longest<=3?194:longest<=6?196:198;
  for(auto* part:{value_,lifeRight_}){lv_obj_set_style_text_font(part,font,0);lv_obj_set_height(part,font->line_height);lv_obj_set_width(part,width);lv_obj_set_y(part,190-font->line_height/2);}
  lv_obj_set_x(value_,leftX);lv_obj_set_x(lifeRight_,rightX);text(value_,left);text(lifeRight_,right);
  bool noLife=game_.player().life<=0,poison=game_.poisonWarning(),commander=game_.commanderWarning();
  bool lethal=noLife||poison||commander;std::string info;
  if(noLife)info="LIFE 0 OR LESS";
  if(poison)info=info.empty()?"POISON LIMIT\nREACHED":"LIFE / POISON";
  if(commander)info=info.empty()?"21 COMMANDER\nDAMAGE":noLife&&poison?"LIFE / POISON\n/ CMD":noLife?"LIFE /\nCOMMANDER":"POISON /\nCOMMANDER";
  if(!lethal&&game_.player().monarch)info="MONARCH";
  if(!storageError_.empty()&&!lethal)info="SAVE FAILED";
  text(lifeMarker_,lethal?"\xef\x95\x8c":"\xef\x80\x84");lv_obj_set_style_text_color(lifeMarker_,lethal?lv_color_white():lv_color_hex(0xff2449),0);
  text(status_,info);lv_obj_set_style_text_color(status_,lethal?lv_color_white():lv_color_hex(teal),0);
  auto undoLabel=game_.recentUndoLabel(lv_tick_get());curvedUndoText(undoLabel.empty()?"Undo":undoLabel);
  if(undoLabel.empty())lv_obj_add_state(undo_,LV_STATE_DISABLED);else lv_obj_remove_state(undo_,LV_STATE_DISABLED);
  if(undoArc_)lv_obj_set_style_arc_color(undoArc_,lv_color_hex(undoLabel.empty()?0x303c43:0x17586a),LV_PART_MAIN);
  for(unsigned side=0;side<2;++side) {
   auto* count=side?poison_:aux_;auto value=std::to_string(game_.player().counters[static_cast<unsigned>(game_.state().shortcuts[side])]);
   text(count,value);int width=std::clamp(static_cast<int>(value.size())*9+8,22,44);
   auto* badge=lv_obj_get_parent(count);lv_obj_set_width(badge,width);lv_obj_set_x(badge,(side?345:56)-width);lv_obj_set_width(count,width-4);lv_obj_center(count);
  }
 }else {
  auto* source=game_.source(sourceId_);
  number(value_,std::to_string(source?source->damage:0));
  text(detail_,source?source->name:"No commander source");
  lv_obj_set_style_text_color(value_,lv_color_hex(source&&source->damage>=21?red:ink),0);
  text(mode_,subtractLife_?"Life linked: ON / 21":"Record only / 21");
 }
}
}
