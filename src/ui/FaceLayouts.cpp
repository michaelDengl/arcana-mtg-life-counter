#include "assets/Fonts.hpp"
#include "FaceDesigns.hpp"
#include "FaceLayouts.hpp"
#include "ui/AppUI.hpp"
#include "ui/Widgets/DialWidgets.hpp"
#include <cmath>
#include <cstring>
extern "C" {
 LV_IMAGE_DECLARE(arcana_after_dark_art);
 LV_IMAGE_DECLARE(arcana_cinder_hex_art);
}
namespace mtg {
using namespace widgets;
namespace {
constexpr std::array<unsigned,5> manaColors{{0xeee2be,0x70aed2,0xa493a8,0xd97658,0x80a98a}};
constexpr std::array<const char*,5> manaGlyphs{{"\xee\x98\x80","\xee\x98\x81","\xee\x98\x82","\xee\x98\x83","\xee\x98\x84"}};
unsigned tint(unsigned a,unsigned b,double amount){unsigned result=0;for(unsigned shift:{0u,8u,16u})result|=static_cast<unsigned>(std::lround(((a>>shift)&255)*(1-amount)+((b>>shift)&255)*amount))<<shift;return result;}
void drawManaGlass(lv_event_t* event){
 auto* object=lv_event_get_target_obj(event);lv_area_t area;lv_obj_get_coords(object,&area);auto* layer=lv_event_get_layer(event);
 // Inset octagonal panes stay inside the bronze frame without a masking layer.
 constexpr std::array<lv_point_t,8> edge{{{17,4},{186,4},{199,17},{199,73},{186,86},{17,86},{4,73},{4,17}}};
 constexpr std::array<unsigned,8> colors{{0,1,1,2,2,3,3,4}};
 lv_draw_triangle_dsc_t pane;lv_draw_triangle_dsc_init(&pane);pane.bg_opa=LV_OPA_COVER;
 for(unsigned i=0;i<edge.size();++i){
  auto a=edge[i],b=edge[(i+1)%edge.size()];pane.p[0]={area.x1+102,area.y1+45};pane.p[1]={area.x1+a.x,area.y1+a.y};pane.p[2]={area.x1+b.x,area.y1+b.y};
  pane.bg_color=lv_color_hex(tint(0x242019,manaColors[colors[i]],.13));lv_draw_triangle(layer,&pane);
 }
 lv_draw_line_dsc_t lead;lv_draw_line_dsc_init(&lead);lead.color=lv_color_hex(0x66583e);lead.width=1;lead.opa=LV_OPA_40;
 for(unsigned i:{0u,1u,3u,5u,7u}){lead.p1={area.x1+102,area.y1+45};lead.p2={area.x1+edge[i].x,area.y1+edge[i].y};lv_draw_line(layer,&lead);}
}
void fitText(lv_obj_t* obj,const std::string& text,FaceRect r,const lv_font_t* font,unsigned color,double maximum=1){
 if(text==lv_label_get_text(obj)&&lv_obj_get_style_text_font(obj,0)==font&&lv_color_eq(lv_obj_get_style_text_color(obj,0),lv_color_hex(color)))return;
 lv_label_set_text(obj,text.c_str());textFont(obj,font);textColor(obj,lv_color_hex(color));
 int width=std::max<int>(1,lv_text_get_width(text.c_str(),static_cast<uint32_t>(text.size()),font,0)+2),height=font->line_height;
 double scale=std::min({maximum,static_cast<double>(std::max(1,r.w-4))/width,static_cast<double>(std::max(1,r.h-2))/height});
 lv_obj_set_size(obj,width,height);lv_obj_set_style_transform_pivot_x(obj,width/2,0);lv_obj_set_style_transform_pivot_y(obj,height/2,0);
 lv_obj_set_style_transform_scale_x(obj,static_cast<int>(scale*256),0);lv_obj_set_style_transform_scale_y(obj,static_cast<int>(scale*256),0);
 lv_obj_set_pos(obj,r.cx()-width/2,r.cy()-height/2);
}
FaceRect localRect(lv_obj_t* obj,lv_obj_t* root){lv_area_t a,b;lv_obj_get_coords(obj,&a);lv_obj_get_coords(root,&b);return {a.x1-b.x1,a.y1-b.y1,lv_area_get_width(&a),lv_area_get_height(&a)};}
bool contains(FaceRect r,int x,int y){return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
}
bool AppUI::redesignedLifeHoldValid()const{
 auto* target=layoutLifeTargets_[lifeHoldSign_>0?1:0];if(!target)return false;
 auto point=hardware_.touch();lv_area_t root;lv_obj_get_coords(screen_,&root);int x=point.x-root.x1,y=point.y-root.y1;
 if(!contains(localRect(target,screen_),x,y))return false;
 for(auto* control:layoutControls_)if(control!=layoutLifeTargets_[0]&&control!=layoutLifeTargets_[1]&&contains(localRect(control,screen_),x,y))return false;
 return true;
}
bool AppUI::faceControlsFit()const{
 if(!redesignedFace(faceDesign_)||view_!=View::Home||selectedPlayer_!=1)return true;
 lv_obj_update_layout(screen_);
 for(auto* obj:layoutControls_)if(!faceRectInside(localRect(obj,screen_)))return false;
 for(std::size_t i=2;i<layoutControls_.size();++i)for(std::size_t j=i+1;j<layoutControls_.size();++j)
  if(faceRectsOverlap(localRect(layoutControls_[i],screen_),localRect(layoutControls_[j],screen_)))return false;
 return true;
}
void AppUI::buildFaceLayout(){
 const auto& p=faceDesigns[faceDesign_];const unsigned id=faceDesign_;
 unsigned first=networkFace()?2:game_.state().format==GameFormat::TwoHeadedGiant?3:2;
 unsigned opponents=networkFace()?static_cast<unsigned>(multiplayer_->opponents().size()):game_.state().playerCount-first+1;
 activeFaceLayout_=mtg::faceLayout(id,opponents);const auto& l=activeFaceLayout_;
 conceptBatteryState_=-2;lv_obj_set_style_bg_color(screen_,lv_color_hex(p.background),0);
 for(unsigned i=0;i<lv_obj_get_child_count(screen_);++i){auto* obj=lv_obj_get_child(screen_,i);if(lv_obj_check_type(obj,&lv_arc_class))lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);}
 auto* art=box(screen_,0,0,360,360,p.background);lv_obj_set_style_bg_opa(art,0,0);lv_obj_set_style_radius(art,0,0);lv_obj_remove_flag(art,LV_OBJ_FLAG_CLICKABLE);lv_obj_move_background(art);
 auto rect=[&](FaceRect r,unsigned color,int radius=12){auto* o=box(art,r.x,r.y,r.w,r.h,color);lv_obj_set_style_radius(o,radius,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;};
 auto line=[&](int x1,int y1,int x2,int y2,unsigned color,int width=1){tabEdgePoints_.push_back({lv_point_precise_t{x1,y1},lv_point_precise_t{x2,y2}});auto* o=lv_line_create(art);lv_line_set_points(o,tabEdgePoints_.back().data(),2);lv_obj_set_style_line_color(o,lv_color_hex(color),0);lv_obj_set_style_line_width(o,width,0);lv_obj_set_style_line_rounded(o,true,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);};
 auto arc=[&](int cx,int cy,int diameter,unsigned start,unsigned end,unsigned color,int width=1){auto* o=lv_arc_create(art);lv_obj_set_size(o,diameter,diameter);lv_obj_set_pos(o,cx-diameter/2,cy-diameter/2);lv_obj_remove_style(o,nullptr,LV_PART_KNOB);lv_obj_set_style_arc_opa(o,0,LV_PART_INDICATOR);lv_obj_set_style_arc_color(o,lv_color_hex(color),LV_PART_MAIN);lv_obj_set_style_arc_width(o,width,LV_PART_MAIN);lv_obj_set_style_arc_rounded(o,false,LV_PART_MAIN);lv_arc_set_bg_angles(o,start,end);lv_obj_remove_flag(o,LV_OBJ_FLAG_CLICKABLE);};
 unsigned faint=tint(p.background,p.accent,.22);
 if(id!=8&&id<11)arc(180,180,344,0,360,faint,id==3?2:1);
 switch(id){
  case 1:
   arc(186,174,320,120,235,p.accent,2);arc(186,174,306,125,230,faint);
   for(int i=0;i<6;++i)rect({102-i*6,104+i*11,3,3},tint(p.background,p.secondary,.25+i*.1),2);
   line(110,251,288,251,faint);break;
  case 2:
   arc(180,158,264,12,168,p.secondary,2);arc(180,158,254,194,346,faint);
   line(90,230,270,230,faint);break;
  case 3:
   for(int x:{40,320})for(int y:{108,222}){int dx=x==40?18:-18,dy=y==108?12:-12;line(x,y,x+dx,y,p.accent,2);line(x,y,x,y+dy,p.accent,2);}
   for(int i=0;i<9;++i)rect({86+i*22,108,12,2},i%3?p.accent:p.secondary,0);break;
  case 4:
   line(108,128,108,276,p.secondary);line(252,128,252,276,p.secondary);
   for(int i=0;i<7;++i){line(103,140+i*20,108,140+i*20,faint);line(252,140+i*20,257,140+i*20,faint);}break;
  case 5:
   arc(144,171,232,100,258,p.accent,2);arc(144,171,246,115,245,faint);
   line(239,135,239,278,p.secondary);break;
  case 6:
   arc(137,181,218,92,267,p.secondary,2);
   for(int i=0;i<11;++i){double a=(105+i*13)*3.141592653589793/180;int x=137+114*std::cos(a),y=181+114*std::sin(a);auto* leaf=rect({x-4,y-2,8,4},i%3?p.accent:p.secondary,4);lv_obj_set_style_transform_pivot_x(leaf,4,0);lv_obj_set_style_transform_pivot_y(leaf,2,0);lv_obj_set_style_transform_rotation(leaf,(i*130+450)%3600,0);}break;
  case 7:
   for(unsigned side=0;side<2;++side){auto r=side?l.rightValue:l.value;r.x-=2;r.y-=5;r.w+=4;r.h+=8;auto* card=rect(r,tint(p.surface,side?p.secondary:p.accent,.12),12);lv_obj_set_style_border_color(card,lv_color_hex(side?p.secondary:p.accent),0);lv_obj_set_style_border_width(card,1,0);}
   line(72,132,288,132,faint);break;
  case 8:
   line(78,237,282,237,tint(p.background,p.ink,.27));line(142,345,218,345,faint);break;
  case 9:
   for(unsigned side=0;side<2;++side){auto r=side?l.rightValue:l.value;auto* card=rect(r,tint(p.surface,side?p.secondary:p.accent,.14),10);lv_obj_set_style_border_color(card,lv_color_hex(side?p.secondary:p.accent),0);lv_obj_set_style_border_width(card,1,0);}
   line(104,106,104,246,p.accent);break;
  case 11: {
   // Five quiet mana bands and bronze inlay, drawn once rather than animated.
   arc(180,180,344,0,360,tint(p.background,p.accent,.30),1);
   arc(180,180,330,0,360,tint(p.background,p.accent,.48),1);
   for(unsigned i=0;i<manaColors.size();++i){
    unsigned start=(236+i*72)%360,end=(start+64)%360;
    arc(180,180,342,start,end,manaColors[i],4);
    double theta=(232+i*72)*3.141592653589793/180;
    int x=180+166*std::cos(theta),y=180+166*std::sin(theta);
    rect({x-2,y-2,4,4},p.accent,LV_RADIUS_CIRCLE);
   }
   arc(180,105,66,162,18,tint(p.surface,p.accent,.42),1);
   arc(180,105,78,218,322,tint(p.background,p.accent,.28),1);
   auto* plate=rect({78,139,204,91},tint(p.surface,p.accent,.09),17);
   lv_obj_set_style_border_width(plate,1,0);lv_obj_set_style_border_color(plate,lv_color_hex(tint(p.surface,p.accent,.72)),0);
   lv_obj_add_event_cb(plate,drawManaGlass,LV_EVENT_DRAW_MAIN,nullptr);
   for(int x:{71,289}){line(x,158,x,211,p.accent);line(x,158,x+(x<180?5:-5),153,p.accent);line(x,211,x+(x<180?5:-5),216,p.accent);}
   // Printed mana pips use the project's licensed Mana font, in W/U/B/R/G order.
   for(unsigned i=0;i<manaGlyphs.size();++i){
    auto* pip=rect({113+static_cast<int>(i)*28,285,22,22},manaColors[i],LV_RADIUS_CIRCLE);
    auto* symbol=label(pip,manaGlyphs[i],0,0,22,&mana_pips,0x181713);lv_obj_center(symbol);
   }
   break;
  }
  case cinderHexFace:
  case afterDarkFace: {
   auto* portrait=lv_image_create(art);
   lv_image_set_src(portrait,id==afterDarkFace?&arcana_after_dark_art:&arcana_cinder_hex_art);
   lv_obj_set_pos(portrait,0,0);
   lv_obj_remove_flag(portrait,LV_OBJ_FLAG_CLICKABLE);
   if(id==afterDarkFace){
    arc(180,180,349,0,360,tint(p.background,p.accent,.60));
    line(145,226,215,226,tint(p.background,p.accent,.64));
   }else{
    arc(180,180,348,96,264,tint(p.background,p.secondary,.76),2);
    arc(180,180,348,276,84,tint(p.background,p.accent,.76),2);
    line(147,220,173,220,tint(p.background,p.secondary,.78));
    line(187,220,213,220,tint(p.background,p.accent,.78));
   }
   break;
  }
 }
 auto targets=l.lifeTargets();
 for(unsigned sign=0;sign<2;++sign){auto r=targets[sign];auto* half=box(screen_,r.x,r.y,r.w,r.h,tint(p.surface,sign?p.gain:p.loss,id==8?.015:id==3?.05:.09));
  lv_obj_set_style_radius(half,id==3?5:id==4?20:14,0);if(id==7||id==8||id==9||id>=11)lv_obj_set_style_bg_opa(half,0,0);
  bindings_.push_back({this,sign?Plus:Minus,sign});lv_obj_add_event_cb(half,lifeTouch,LV_EVENT_ALL,&bindings_.back());layoutLifeTargets_[sign]=half;layoutControls_.push_back(half);
 }
 auto action=[&](FaceRect r,const std::string& title,Action a,unsigned arg,unsigned accent,bool round=false){
  auto* b=button(screen_,"",r.x,r.y,r.w,r.h,a,arg,p.surface);lv_obj_set_style_pad_all(b,0,0);lv_obj_set_style_radius(b,round?LV_RADIUS_CIRCLE:id==3?6:10,0);lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_border_color(b,lv_color_hex(tint(p.surface,accent,.65)),0);
  lv_obj_set_style_bg_color(b,lv_color_hex(tint(p.surface,accent,.13)),LV_STATE_PRESSED);lv_obj_set_style_bg_color(b,lv_color_hex(p.surface),LV_STATE_DISABLED);
  auto* text=lv_obj_get_child(b,0);lv_label_set_text(text,title.c_str());const auto* font=title=="+"||title=="-"||title==LV_SYMBOL_SETTINGS?&lv_font_montserrat_20:&lv_font_montserrat_14;lv_obj_set_style_text_font(text,font,0);lv_obj_set_height(text,font->line_height);lv_obj_set_style_text_color(text,lv_color_hex(accent),0);lv_obj_set_width(text,r.w-6);lv_label_set_long_mode(text,LV_LABEL_LONG_DOT);lv_obj_set_style_text_align(text,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(text);layoutControls_.push_back(b);return b;
 };
 for(unsigned side=0;side<2;++side){const auto& c=l.counters[side];
  action(c.minus,"-",SideStep,side*2,p.loss);auto* icon=action(c.icon,"",OpenPicker,side,p.accent);lv_obj_clean(icon);
  drawToolIcon(icon,counterIcon(static_cast<unsigned>(game_.state().shortcuts[side])),0,(c.icon.h-30)/2,counterColor(static_cast<unsigned>(game_.state().shortcuts[side])));
  auto* symbol=lv_obj_get_child(icon,0);lv_obj_set_style_transform_pivot_x(symbol,15,0);lv_obj_set_style_transform_pivot_y(symbol,15,0);lv_obj_set_style_transform_scale_x(symbol,224,0);lv_obj_set_style_transform_scale_y(symbol,224,0);
  auto* count=label(icon,"",30,0,c.icon.w-33,&lv_font_montserrat_14,p.ink);lv_obj_set_y(count,(c.icon.h-lv_font_montserrat_14.line_height)/2);if(side)poison_=count;else aux_=count;
  action(c.plus,"+",SideStep,side*2+1,p.gain);
 }
 action(l.gear,LV_SYMBOL_SETTINGS,OpenGear,0,id==8?p.secondary:p.ink,true);
 undo_=action(l.undo,"Undo",RecentUndo,0,p.ink);
 value_=label(screen_,"",0,0,0,&life_digits,p.ink);
 if(l.format==FaceNumberFormat::Split)lifeRight_=label(screen_,"",0,0,0,&life_digits,p.ink);
 lifeMarker_=label(screen_,"",0,0,0,&heart_symbol,p.loss);
 battery_=box(screen_,l.battery.x,l.battery.y,28,14,p.surface);lv_obj_set_style_bg_opa(battery_,0,0);lv_obj_remove_flag(battery_,LV_OBJ_FLAG_CLICKABLE);
 status_=label(screen_,"",l.status.x,l.status.y,l.status.w,&tab_font,p.ink);lv_obj_set_height(status_,12);lv_label_set_long_mode(status_,LV_LABEL_LONG_DOT);lv_obj_set_y(status_,l.status.cy()-6);
 if(networkFace())label(screen_,profile_->name(),l.name.x,l.name.y,l.name.w,&tab_font,p.ink);
 else if(id==11)label(screen_,"YOUR LIFE",l.name.x,l.name.y,l.name.w,&tab_font,p.muted);
 if(id==11){label(screen_,"-",86,177,20,&lv_font_montserrat_14,p.loss);label(screen_,"+",254,177,20,&lv_font_montserrat_14,p.gain);}
 if(id==afterDarkFace||id==cinderHexFace){
  label(screen_,"-",126,214,18,&lv_font_montserrat_14,p.loss);
  label(screen_,"+",216,214,18,&lv_font_montserrat_14,p.gain);
 }
 if(l.vertical){label(screen_,"+",l.life.x+l.life.w-24,l.life.y+7,16,&lv_font_montserrat_14,p.gain);label(screen_,"-",l.life.x+l.life.w-24,l.life.y+l.life.h-22,16,&lv_font_montserrat_14,p.loss);}
 // Large tables use two complete rows, rather than overlapping top controls.
 unsigned drawn=0,rows=opponents>5?2:1;
 for(unsigned row=0;row<rows;++row){unsigned count=rows==1?opponents:row==0?(opponents+1)/2:opponents-(opponents+1)/2;if(!count)continue;
  int y=rows==1?l.playersY:30+30*row,h=rows==1?34:26,gap=3;double dy=std::max(std::abs(y-179.5),std::abs(y+h-1-179.5));int available=static_cast<int>(2*std::sqrt(168.0*168-dy*dy))-4;
  int w=std::min(76,(available-static_cast<int>(count-1)*gap)/static_cast<int>(count)),start=180-(w*count+(count-1)*gap)/2;
  for(unsigned i=0;i<count;++i,++drawn){unsigned owner=first+drawn;FaceRect r{start+static_cast<int>(i)*(w+gap),y,w,h};auto* b=action(r,"",SelectPlayer,owner,p.accent);lv_obj_clean(b);
   lv_obj_set_style_bg_color(b,lv_color_hex(tint(p.surface,drawn%2?p.secondary:p.accent,.37)),0);
   if(id==11){auto color=manaColors[(drawn+1)%manaColors.size()];lv_obj_set_style_bg_color(b,lv_color_hex(tint(p.surface,color,.24)),0);lv_obj_set_style_border_color(b,lv_color_hex(tint(p.surface,color,.68)),0);}
   auto name=networkFace()?multiplayer_->opponents()[drawn].name:"P"+std::to_string(owner);
   auto* n=label(b,name,3,2,w-6,&tab_font,p.ink);lv_label_set_long_mode(n,LV_LABEL_LONG_DOT);
   const auto* font=rows==1?&lv_font_montserrat_14:&tab_font;
   tabValues_[owner-1]=label(b,"",3,rows==1?15:14,w-6,font,p.ink);lv_label_set_long_mode(tabValues_[owner-1],LV_LABEL_LONG_DOT);
  }
 }
}
void AppUI::refreshFaceLayout(){
 const auto& p=faceDesigns[faceDesign_];const auto& l=activeFaceLayout_;
 auto number=std::to_string(game_.player().life);
 if(l.format==FaceNumberFormat::Padded){if(game_.player().life>=0)while(number.size()<3)number="0"+number;else if(number.size()<3)number.insert(1,"0");}
 if(l.format==FaceNumberFormat::Split){if(game_.player().life>=0&&number.size()<2)number="0"+number;auto split=(number.size()+1)/2;fitText(value_,number.substr(0,split),l.value,&life_digits,p.ink,l.numberScale);fitText(lifeRight_,number.substr(split),l.rightValue,&life_digits,p.ink,l.numberScale);}
 else if(faceDesign_>=11){
  // Use native bitmap sizes: clipped resampling can otherwise leave edge pixels.
  const lv_font_t* font=&tab_font;
  for(const auto* candidate:{&mana_life_digits,&life_digits,&lv_font_montserrat_48,&lv_font_montserrat_32,&lv_font_montserrat_20,&lv_font_montserrat_14,&tab_font})
   if(candidate->line_height<=l.value.h-2&&lv_text_get_width(number.c_str(),static_cast<uint32_t>(number.size()),candidate,0)<=l.value.w-6){font=candidate;break;}
  fitText(value_,number,l.value,font,p.ink,1);
 }else fitText(value_,number,l.value,&life_digits,p.ink,l.numberScale);
 bool noLife=game_.player().life<=0,poison=game_.poisonWarning(),commander=game_.commanderWarning(),lethal=noLife||poison||commander;
 std::string reason;if(noLife)reason="LIFE 0 OR LESS";if(poison)reason=reason.empty()?"POISON LIMIT":"LIFE / POISON";if(commander)reason=reason.empty()?"21 CMD DAMAGE":noLife&&poison?"LIFE / POISON / CMD":noLife?"LIFE / CMD":"POISON / CMD";
 if(!lethal&&game_.player().monarch)reason="MONARCH";if(!lethal&&networkFace()&&!multiplayer_->error().empty())reason="NETWORK ISSUE";
 fitText(lifeMarker_,lethal?"\xef\x95\x8c":"\xef\x80\x84",l.heart,&heart_symbol,lethal?0xffffff:p.loss);
 text(status_,reason);textColor(status_,lv_color_hex(lethal?0xffffff:p.accent));
 auto recent=game_.recentUndoLabel(lv_tick_get());undoCurvedText_=recent.empty()?"Undo":recent;auto* caption=lv_obj_get_child(undo_,0);text(caption,undoCurvedText_);
 const auto* font=lv_text_get_width(undoCurvedText_.c_str(),static_cast<uint32_t>(undoCurvedText_.size()),&lv_font_montserrat_14,0)>l.undo.w-10?&tab_font:&lv_font_montserrat_14;textFont(caption,font);if(lv_obj_get_height(caption)!=font->line_height)lv_obj_set_height(caption,font->line_height);lv_obj_center(caption);
 if(recent.empty())lv_obj_add_state(undo_,LV_STATE_DISABLED);else lv_obj_remove_state(undo_,LV_STATE_DISABLED);
 for(unsigned side=0;side<2;++side){auto* count=side?poison_:aux_;auto number=std::to_string(game_.player().counters[static_cast<unsigned>(game_.state().shortcuts[side])]);text(count,number);int width=l.counters[side].icon.w-33;const auto* font=lv_text_get_width(number.c_str(),static_cast<uint32_t>(number.size()),&lv_font_montserrat_14,0)>width?&tab_font:&lv_font_montserrat_14;textFont(count,font);lv_obj_set_height(count,font->line_height);lv_obj_set_y(count,(l.counters[side].icon.h-font->line_height)/2);}
 if(conceptBatteryState_!=batteryState_){conceptBatteryState_=batteryState_;lv_obj_update_layout(battery_);for(unsigned i=0;i<lv_obj_get_child_count(battery_);++i){auto* item=lv_obj_get_child(battery_,i);if(lv_obj_check_type(item,&lv_label_class))textColor(item,lv_color_hex(p.ink));else if(lv_obj_get_width(item)==24){lv_obj_set_style_bg_color(item,lv_color_hex(p.surface),0);lv_obj_set_style_border_color(item,lv_color_hex(p.ink),0);}else if(lv_obj_get_x(item)==24)lv_obj_set_style_bg_color(item,lv_color_hex(p.ink),0);}}
}
void AppUI::buildIdleDial(){
 for(unsigned side=0;side<2;++side){auto* half=box(screen_,180*side,0,180,360,0);lv_obj_set_style_bg_opa(half,0,0);lv_obj_set_style_radius(half,0,0);bindings_.push_back({this,side?Plus:Minus,side});lv_obj_add_event_cb(half,lifeTouch,LV_EVENT_ALL,&bindings_.back());}
 value_=label(screen_,"",0,0,0,&mana_life_digits,faceDesigns[faceDesign_].ink);
}
void AppUI::refreshIdleDial(){
 auto number=std::to_string(game_.player().life);FaceRect r{34,116,292,128};const lv_font_t* font=&tab_font;
 for(const auto* candidate:{&mana_life_digits,&life_digits,&lv_font_montserrat_48,&lv_font_montserrat_32,&lv_font_montserrat_20,&tab_font})if(candidate->line_height<=r.h-2&&lv_text_get_width(number.c_str(),static_cast<uint32_t>(number.size()),candidate,0)<=r.w-6){font=candidate;break;}
 fitText(value_,number,r,font,faceDesigns[faceDesign_].ink,1);
}

}
