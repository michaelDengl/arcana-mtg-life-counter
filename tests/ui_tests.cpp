#include "lvgl.h"
#include "ui/AppUI.hpp"
#include "SimulatorHardware.hpp"
#include "third_party/json.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cmath>
namespace {
constexpr int size=360;
mtg::SimulatorHardware hardware;
mtg::GameState game;
mtg::AppUI* app;
class TestRandom final:public mtg::IRandom {
public:
 std::vector<unsigned> values{1,0,4,19,6,2,9,8};std::size_t index=0;
 unsigned uniform(unsigned lower,unsigned upper)override {
  if(index>=values.size()||values[index]<lower||values[index]>upper){std::cerr<<"Invalid random test sequence";std::exit(1);}return values[index++];
 }
};
class ProfileMemory final:public mtg::IStorage {public:std::string value;bool load(std::string& data,std::string&)override{data=value;return true;}bool save(const std::string& data,std::string&)override{value=data;return true;}};
class UIWire final:public mtg::IPeerTransport {public:std::vector<mtg::PeerPacket> incoming,sent;std::string empty;bool send(const std::string& endpoint,const std::string& data)override{sent.push_back({endpoint,data});return true;}std::vector<mtg::PeerPacket> receive()override{auto result=std::move(incoming);incoming.clear();return result;}const std::string& error()const override{return empty;}};
std::vector<uint32_t> pixels(size*size);
void check(bool condition,const std::string& message) {
 if(!condition){std::cerr<<"UI: "<<message<<'\n';std::exit(1);}
}
void flush(lv_display_t* d,const lv_area_t* a,uint8_t* data) {
 for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x)
  pixels[y*size+x]=reinterpret_cast<uint32_t*>(data)[(y-a->y1)*(a->x2-a->x1+1)+x-a->x1];
 lv_display_flush_ready(d);
}
void read(lv_indev_t*,lv_indev_data_t* data) {
 auto t=hardware.touch();data->point.x=t.x;data->point.y=t.y;data->state=t.pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
}
void tick(int times=4) {for(int i=0;i<times;++i){app->update();lv_tick_inc(20);lv_timer_handler();}}
lv_obj_t* find(lv_obj_t* parent,const std::string& text) {
 if(lv_obj_check_type(parent,&lv_label_class) && text==lv_label_get_text(parent))return parent;
 for(unsigned i=0;i<lv_obj_get_child_count(parent);++i)if(auto* found=find(lv_obj_get_child(parent,i),text))return found;
 return nullptr;
}
void click(const std::string& text) {
 tick();auto* target=find(lv_screen_active(),text);auto* wedgeLabel=(text.size()==2&&text[0]=='P'&&text[1]>='2'&&text[1]<='6')?target:nullptr;check(target!=nullptr,"missing label: "+text);auto* labelTarget=target;bool wedge=target==wedgeLabel||text=="New Game"||text=="Flip Coin"||text=="Roll"||text=="End Game"||text=="Brightness";
 while(target && !lv_obj_check_type(target,&lv_button_class))target=lv_obj_get_parent(target);
 check(target!=nullptr,"no button for: "+text);wedge=wedge||((lv_obj_get_width(target)==300||lv_obj_get_width(target)==326||lv_obj_get_width(target)==340)&&lv_obj_get_height(target)==lv_obj_get_width(target));
 lv_obj_scroll_to_view_recursive(target,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());tick();
 lv_area_t a;lv_obj_get_coords(wedge?labelTarget:target,&a);int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;
 check(x>=0&&x<size&&y>=0&&y<size,"offscreen button: "+text);
 hardware.setTouch({x,y,true});tick(5);hardware.setTouch({x,y,false});tick(5);
}
lv_obj_t* labelAt(lv_obj_t* parent,int x,int y) {
 if(lv_obj_check_type(parent,&lv_label_class)){lv_area_t a;lv_obj_get_coords(parent,&a);if(x>=a.x1&&x<=a.x2&&y>=a.y1&&y<=a.y2)return parent;}
 for(unsigned i=0;i<lv_obj_get_child_count(parent);++i)if(auto* found=labelAt(lv_obj_get_child(parent,i),x,y))return found;return nullptr;
}
void checkDeath(const std::string& reason) {
 auto* skull=find(lv_screen_active(),"\xef\x95\x8c");auto* explanation=find(lv_screen_active(),reason);
 check(skull&&explanation,"skull and defeat reason present");
 lv_area_t marker;lv_obj_get_coords(skull,&marker);check(marker.y1==120&&marker.y2<168,"skull uses the new upper heart position");
 check(lv_color_eq(lv_obj_get_style_text_color(skull,0),lv_color_white())&&lv_color_eq(lv_obj_get_style_text_color(explanation,0),lv_color_white()),"skull and reason are white");
}
void clickAt(int x,int y) {hardware.setTouch({x,y,true});tick(5);hardware.setTouch({x,y,false});tick(5);}
void holdAt(int x,int y){hardware.setTouch({x,y,true});tick(35);hardware.setTouch({x,y,false});tick(5);}
void trackers(){app->openTrackers();tick();}
void rotate(int steps){hardware.rotate(steps);tick();}
void screenshot(const char* file) {
 tick();std::ofstream out(file,std::ios::binary);
 auto u16=[&](unsigned v){out.put(static_cast<char>(v));out.put(static_cast<char>(v>>8));};
 auto u32=[&](unsigned v){u16(v);u16(v>>16);};
 out.write("BM",2);u32(54+size*size*4);u32(0);u32(54);u32(40);u32(size);u32(size);u16(1);u16(32);u32(0);u32(size*size*4);u32(2835);u32(2835);u32(0);u32(0);
 for(int y=size-1;y>=0;--y)for(int x=0;x<size;++x)u32(pixels[y*size+x]);
}
}
int main() {
 lv_init();auto* display=lv_display_create(size,size);std::vector<uint32_t> buffer(size*40);
 lv_display_set_buffers(display,buffer.data(),nullptr,static_cast<uint32_t>(buffer.size()*4),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
 auto* input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,read);
 lv_obj_set_style_bg_color(lv_screen_active(),lv_color_hex(0),0);
 auto* device=lv_obj_create(lv_screen_active());lv_obj_set_size(device,size,size);lv_obj_set_pos(device,0,0);
 TestRandom random;mtg::AppUI ui(game,hardware,device,random);app=&ui;tick();screenshot("ui-startup.bmp");
 auto* continueLabel=find(lv_screen_active(),"Continue");
 check(continueLabel && lv_obj_has_state(lv_obj_get_parent(continueLabel),LV_STATE_DISABLED),"Continue disabled without unfinished game");
 rotate(100);check(!game.state().active && game.player().life==40,"startup rotary cannot edit game");
 click("New game");click("SINGLE ARCANA");screenshot("ui-formats.bmp");click("Commander");check(find(lv_screen_active(),"4-Players"),"four players by default");screenshot("ui-setup.bmp");
 rotate(100);check(find(lv_screen_active(),"6-Players"),"setup clamps at six");
 rotate(-100);check(find(lv_screen_active(),"2-Players"),"setup clamps at two");
 click("Back");check(!game.state().active,"setup Back does not start a game");
 click("Commander");click("Confirm");check(game.state().active && game.state().playerCount==4,"confirmed four-player match");
 check(!find(lv_screen_active(),"P1") && find(lv_screen_active(),"P4") && !find(lv_screen_active(),"P5"),"tabs match selected player count");screenshot("ui-home.bmp");
 // Regression: the whole top segment must start at the divider, not merely change color there.
 bool strayStroke=false;
 for(int y=82;y<=86;++y)for(int x=174;x<180;++x){auto pixel=pixels[y*size+x];unsigned r=(pixel>>16)&255,g=(pixel>>8)&255,b=pixel&255;if((r>180&&g<110)||(g>150&&r<70&&b<220))strayStroke=true;}
 auto joinPixel=pixels[83*size+180];
 check(!strayStroke&&((joinPixel>>8)&255)>150&&((joinPixel>>16)&255)<70,"top divider meets the actual segment end without a left overhang");
 clickAt(232,190);check(game.player().life==41,"green life region tap adds one");clickAt(128,190);check(game.player().life==40,"red life region tap subtracts one");
 hardware.setTouch({232,190,true});tick(2);tick(45);check(game.player().life==40,"life hold waits one second");
 tick(10);check(game.player().life==45,"first hold beat adds five");tick(50);check(game.player().life==50,"second hold beat adds five");
 hardware.setTouch({232,190,false});tick(5);check(game.player().life==50,"held release does not add a tap");tick(60);check(game.player().life==50,"release stops hold repeats");
 hardware.setTouch({128,190,true});tick(60);hardware.setTouch({128,190,false});tick(5);check(game.player().life==45,"left hold subtracts five");
 hardware.setTouch({128,190,true});tick(5);hardware.togglePower();tick(70);check(game.player().life==45,"power-off cancels life hold");hardware.togglePower();hardware.setTouch({128,190,false});tick();
 click(LV_SYMBOL_SETTINGS);check(find(lv_screen_active(),"End Game")&&!find(lv_screen_active(),"Trackers"),"gear has fourth End Game option and no Trackers button");check(game.player().life==45,"gear does not trigger life tap");click("Back");
 game.reset();ui.goHome();tick();
 clickAt(35,195);check(find(lv_screen_active(),"TREASURES")&&find(lv_screen_active(),"Back"),"treasure tap opens radial picker");click("Back");
 for(int i=0;i<7;++i)clickAt(35,141);
 check(game.player().counters[static_cast<unsigned>(mtg::Counter::Treasure)]==7 && game.player().life==40,"treasures independent of life");screenshot("ui-treasures.bmp");
 clickAt(325,195);check(find(lv_screen_active(),"POISON"),"poison tap opens picker");click("Back");clickAt(325,141);check(game.player().counters[0]==1,"poison shortcut counts");clickAt(325,245);
 rotate(3);check(game.player().life==43,"life rotary");rotate(-1);check(game.player().life==42,"life rotary decrement");
 click("P2");rotate(5);check(game.source(1)->damage==5&&game.player().life==37,"P2 rotary adjusts linked commander damage");
 screenshot("ui-opponent.bmp");click("-");check(game.source(1)->damage==4&&game.player().life==38,"damage correction refunds linked life");
 click("+");click("Life linked: ON / 21");rotate(3);check(game.source(1)->damage==8&&game.player().life==37,"record-only dial mode");
 rotate(-2);check(game.source(1)->damage==6&&game.player().life==37,"record-only correction does not refund life");
 click("Back");click("P3");rotate(2);check(game.source(2)->damage==2&&game.source(1)->damage==6,"opponent tabs isolate sources");
 click("Record only / 21");click("Back");check(find(lv_screen_active(),"\xef\x80\x84"),"radial Back returns to P1");
 game.reset();game.adjustLife(2);tick();
 trackers();click("POISON");rotate(10);
 check(game.poisonWarning(),"poison editor and threshold");screenshot("ui-poison.bmp");
 click("Undo");check(game.player().counters[0]==0,"counter undo button enabled after edit");
 ui.redo();tick();check(game.player().counters[0]==10,"counter redo button");
 click("Close");trackers();click("COMMANDER DAMAGE");click("P2 / COMMANDER A");rotate(4);click("Apply 5 damage");
 check(game.source(1)->damage==5&&game.player().life==37,"linked damage receipt");
 click("Also subtract life: ON");rotate(20);click("Apply 21 damage");
 check(game.source(1)->damage==26&&game.player().life==37,"record-only receipt");screenshot("ui-damage.bmp");
 click("Undo");check(game.source(1)->damage==5&&game.player().life==37,"record-only undo");
 ui.undo();tick();check(game.source(1)->damage==0&&game.player().life==42,"linked receipt atomic undo");
 ui.redo();tick();check(game.source(1)->damage==5&&game.player().life==37,"linked receipt redo");
 click(LV_SYMBOL_LEFT);click("+ Commander source");click("Add source");
 check(game.state().sources.size()==4&&game.state().sources.back().owner==2,"add partner source");
 click("P2 / COMMANDER B");check(find(lv_screen_active(),"RECEIVED  0 / 21"),"partner damage independent");
 click(LV_SYMBOL_LEFT);click(LV_SYMBOL_LEFT);click("YOUR COMMANDERS");click("MY COMMANDER");rotate(2);
 check(game.nextTax(0)==4,"cast count editing");screenshot("ui-casts.bmp");
 click(LV_SYMBOL_LEFT);click("+ Partner / commander");click("MY COMMANDER 2");click("Cast +1");
 check(game.nextTax(1)==2&&game.nextTax(0)==4,"partner tax editor independent");
 click(LV_SYMBOL_LEFT);click(LV_SYMBOL_LEFT);click("DECK TRACKERS");
 click("ENERGY");click("EXPERIENCE");click("RAD");click("STORM");click("CUSTOM");screenshot("ui-profiles.bmp");
 for(unsigned i=1;i<mtg::counterCount;++i)check(game.player().enabled[i],"enable optional counter");
 click(LV_SYMBOL_LEFT);click("ENERGY");rotate(7);check(game.player().counters[1]==7,"energy edit");
 click("Close");trackers();click("STORM");rotate(9);click("New turn / reset");
 check(game.player().counters[4]==0&&game.player().counters[1]==7,"turn reset scope");
 click("Close");trackers();click("CUSTOM");click("Rename");
 for(int i=0;i<6;++i)click("Delete");rotate(4);click("Append character");click("Save");
 check(game.player().customName=="E","knob name editor");
 click("Close");trackers();click("TABLE STATUS");click("MONARCH");click("INITIATIVE");click("CITY'S BLESSING");click("DAY / NIGHT");
 check(game.player().monarch&&game.player().initiative&&game.player().cityBlessing&&game.state().dayNight==mtg::DayNight::Day,"status toggles");screenshot("ui-status.bmp");
 click(LV_SYMBOL_LEFT);click("HISTORY");screenshot("ui-history.bmp");
 click("Undo");check(game.state().dayNight==mtg::DayNight::Neither,"history undo");click("Redo");check(game.state().dayNight==mtg::DayNight::Day,"history redo");
 ui.goHome();tick();hardware.togglePower();rotate(5);check(game.player().life==37,"power-off rotation ignored");
 hardware.togglePower();rotate(-2);check(game.player().life==35,"power-on rotation works");
 game.reset();ui.goHome();tick();check(game.player().life==40&&game.player().counters[0]==0,"reset refresh");
 ui.showStartup();tick();click("Continue");check(find(lv_screen_active(),"\xef\x80\x84"),"Continue reopens unfinished match");
 click(LV_SYMBOL_SETTINGS);click("End Game");click("Back");check(game.state().active&&find(lv_screen_active(),"Brightness"),"cancel End Game returns to gear menu");
 click("End Game");click("End game");check(!game.state().active,"explicitly ended match");
 continueLabel=find(lv_screen_active(),"Continue");check(continueLabel&&lv_obj_has_state(lv_obj_get_parent(continueLabel),LV_STATE_DISABLED),"Continue disabled after End game");screenshot("ui-ended.bmp");
 click("New game");click("SINGLE ARCANA");click("Commander");rotate(2);click("Confirm");screenshot("ui-six-players.bmp");check(game.state().playerCount==6&&find(lv_screen_active(),"P6"),"six-player tabs");screenshot("ui-six-players.bmp");
 click(LV_SYMBOL_SETTINGS);click("Brightness");rotate(-1000);check(hardware.brightness()==0&&find(lv_screen_active(),"0%"),"brightness minimum");rotate(1000);check(hardware.brightness()==100&&find(lv_screen_active(),"100%"),"brightness maximum");rotate(-35);check(hardware.brightness()==65,"brightness encoder adjustment");click("+");check(hardware.brightness()==66,"brightness plus button");click("-");check(hardware.brightness()==65,"brightness minus button");clickAt(180,122);check(hardware.brightness()>=49&&hardware.brightness()<=51,"brightness touch slider");hardware.setBrightness(65);tick();screenshot("ui-brightness.bmp");click("Back");check(find(lv_screen_active(),"Flip Coin"),"brightness Back returns one step");click("Back");hardware.setBrightness(100);
 auto beforeTools=game.serialize();click(LV_SYMBOL_SETTINGS);screenshot("ui-tools.bmp");click("Flip Coin");check(find(lv_screen_active(),"HEADS"),"heads result from injected randomness");screenshot("ui-coin.bmp");
 click("Flip again");check(find(lv_screen_active(),"TAILS"),"tails result");click("Back");click("Back");
 click(LV_SYMBOL_SETTINGS);click("Roll");check(find(lv_screen_active(),"P2")&&find(lv_screen_active(),"STARTS"),"highest D20 determines starter");screenshot("ui-roll.bmp");click("Back");click("Back");
 check(game.serialize()==beforeTools,"random tools do not mutate match stats");
 game.adjustLife(-5);game.adjustCounter(mtg::Counter::Treasure,7);game.adjustCounter(mtg::Counter::Poison,3);tick();
 click(LV_SYMBOL_SETTINGS);click("New Game");screenshot("ui-reset-confirm.bmp");click("No");check(game.player().life==35&&game.player().counters[6]==7,"No preserves match");
 auto preservedMatch=game.serialize();click("New Game");click("Yes");click("SINGLE ARCANA");click("Classic");click("Back");click("Back");click("Back");check(game.serialize()==preservedMatch&&find(lv_screen_active(),"New Game"),"cancel format setup preserves current match");
 click("New Game");click("Yes");click("SINGLE ARCANA");click("Commander");rotate(2);click("Confirm");check(game.state().playerCount==6&&game.player().life==40&&game.player().counters[6]==0&&game.player().counters[0]==0,"Yes resets current player count and stats");
 game.startNewGame(1);ui.goHome();tick();check(!find(lv_screen_active(),"P1")&&!find(lv_screen_active(),"P2")&&find(lv_screen_active(),LV_SYMBOL_SETTINGS),"solo layout has no opponent tabs");
 game.startNewGame(4);ui.goHome();tick();rotate(-5);rotate(-5);
 check(game.player().life==30&&find(lv_screen_active(),"Undo -10"),"two -5 rotations aggregate on main Undo");screenshot("ui-adjustments-home.bmp");
 click("Undo -10");check(game.player().life==40,"main Undo reverses five-second group");
 rotate(5);tick(251);check(game.player().life==45&&lv_obj_has_state(lv_obj_get_parent(find(lv_screen_active(),"Undo")),LV_STATE_DISABLED),"expired main Undo disabled");
 clickAt(35,195);check(find(lv_screen_active(),"ENERGY")&&find(lv_screen_active(),"Back"),"short tap opens left picker without editing a counter");click("ENERGY");
 check(game.state().shortcuts[0]==mtg::Counter::Energy,"left picker changes shortcut");
 clickAt(35,141);clickAt(35,141);clickAt(35,245);check(game.player().counters[1]==1&&game.player().life==45,"left side +/- edit selected energy only");
 clickAt(35,195);click("TREASURES");clickAt(35,195);click("ENERGY");check(game.player().counters[1]==1,"switching shortcuts preserves counts");
 clickAt(325,195);check(find(lv_screen_active(),"SHIELD")&&find(lv_screen_active(),"Back"),"short tap opens right picker");click("SHIELD");
 clickAt(325,141);check(game.player().counters[7]==1&&game.player().counters[1]==1,"right side increments selected shield independently");
 clickAt(325,245);clickAt(325,245);check(game.player().counters[7]==0,"side count does not become negative");
 game.adjustCounter(mtg::Counter::Treasure,7);game.adjustCounter(mtg::Counter::Poison,6);game.adjustCounter(mtg::Counter::Experience,4);game.adjustCounter(mtg::Counter::Spells,5);game.adjustCounter(mtg::Counter::Rad,2);tick();
 clickAt(35,195);check(find(lv_screen_active(),"7")&&find(lv_screen_active(),"6")&&find(lv_screen_active(),"4")&&find(lv_screen_active(),"5")&&find(lv_screen_active(),"2"),"picker contains current counts for all counters");screenshot("ui-picker.bmp");
 click("RAD");clickAt(35,141);check(game.player().counters[3]==3,"rad shortcut edits existing count");
 clickAt(325,195);click("EXPERIENCE");clickAt(325,195);check(find(lv_screen_active(),"EXPERIENCE"),"short tap opens radial selector");click("Back");clickAt(325,141);clickAt(325,141);
 check(game.player().counters[2]==6&&game.state().shortcuts[1]==mtg::Counter::Experience,"radial choice and side controls share counter state");
 screenshot("ui-selected-shortcuts.bmp");
 auto pickerState=game.serialize();clickAt(35,195);clickAt(180,180);check(game.serialize()==pickerState,"central picker Back preserves selection and all stats");
 const mtg::Counter pickerTypes[]{mtg::Counter::Treasure,mtg::Counter::Poison,mtg::Counter::Energy,mtg::Counter::Experience,mtg::Counter::Spells,mtg::Counter::Rad,mtg::Counter::Shield};
 for(unsigned i=0;i<7;++i){clickAt(35,195);double angle=(270+360.0*i/7)*3.141592653589793/180;clickAt(static_cast<int>(180+145*std::cos(angle)),static_cast<int>(180+145*std::sin(angle)));check(game.state().shortcuts[0]==pickerTypes[i],"entire radial counter section selects correct type");}

 game.startNewGame(4);ui.goHome();tick();
 for(int value=0;value<=11;++value) {
  game.adjustLife(value-game.player().life);tick();auto* left=labelAt(lv_screen_active(),128,190);auto* right=labelAt(lv_screen_active(),232,190);
  check(left&&right&&std::string(lv_label_get_text(left))==std::to_string(value/10)&&std::string(lv_label_get_text(right))==std::to_string(value%10),"life uses tens on left and units on right");
  if(value==0)checkDeath("LIFE 0 OR LESS");
  if(value==1)check(find(lv_screen_active(),"\xef\x80\x84"),"heart returns after healing above zero");
 }
 game.adjustLife(7-game.player().life);tick();screenshot("ui-single-digit.bmp");game.adjustLife(-game.player().life);tick();screenshot("ui-defeat-life.bmp");
 game.reset();game.applyCommanderDamage(1,21,false);tick();checkDeath("21 COMMANDER\nDAMAGE");screenshot("ui-defeat-commander.bmp");
 game.reset();game.adjustCounter(mtg::Counter::Poison,10);tick();checkDeath("POISON LIMIT\nREACHED");screenshot("ui-defeat-poison.bmp");
 game.adjustCounter(mtg::Counter::Poison,-1);tick();check(find(lv_screen_active(),"\xef\x80\x84"),"heart returns below poison threshold");
 game.adjustLife(-40);game.adjustCounter(mtg::Counter::Poison,1);game.applyCommanderDamage(1,21,false);tick();checkDeath("LIFE / POISON\n/ CMD");
 for(unsigned players=2;players<=6;++players) {
  game.startNewGame(players);ui.goHome();tick();double span=90.0/(players-1);
  for(unsigned owner=2;owner<=players;++owner) {
   double start=225+(owner-2)*span;
   for(auto point:std::vector<std::pair<double,double>>{{160,start+2},{160,start+span-2},{135,start+span/2}}) {
    double angle=point.second*3.141592653589793/180;
    clickAt(static_cast<int>(std::round(180+point.first*std::cos(angle))),static_cast<int>(std::round(180+point.first*std::sin(angle))));
    check(find(lv_screen_active(),"P"+std::to_string(owner)+" / COMMANDER A"),"entire player wedge responds near its edges");click("Back");
   }
  }
 }
 screenshot("ui-cmd-tabs.bmp");
 game.startNewGame(4);ui.goHome();tick(12);
 std::vector<uint32_t> previousBattery;int previousBand=-1;
 for(unsigned percent: {0u,5u,6u,20u,21u,40u,41u,60u,61u,80u,81u,100u}) {
  hardware.setBattery(percent);tick();
  std::vector<uint32_t> indicator;for(int y=236;y<248;++y)for(int x=166;x<192;++x)indicator.push_back(pixels[y*size+x]);
  int band=percent<=5?0:percent<=20?1:percent<=40?2:percent<=60?3:percent<=80?4:5;
  if(previousBand>=0)check((indicator==previousBattery)==(band==previousBand),"battery renders correct threshold transitions");
  previousBattery=indicator;previousBand=band;
  screenshot(("ui-battery-"+std::to_string(percent)+".bmp").c_str());
 }
 hardware.setBattery(50,true);tick();screenshot("ui-battery-charging.bmp");
 clickAt(180,242);check(game.player().life==41,"battery overlay leaves underlying life touch active");
 hardware.setBattery(75);tick();
 for(auto entry:std::vector<std::pair<const char*,std::int64_t>>{{"Classic",20},{"Brawl",25},{"Limited",20}}){
  game.endGame();ui.showStartup();tick(12);check(find(lv_screen_active(),"LIFE COUNTER")&&!find(lv_screen_active(),"COMMANDER LIFE COUNTER"),"startup title has no Commander prefix");
  click("New game");click("SINGLE ARCANA");click(entry.first);check(find(lv_screen_active(),"2-Players"),"noncommander preset defaults to two players");click("Confirm");tick(12);
  check(game.player().life==entry.second&&game.life(2)==entry.second&&!game.commanderWarning(),"preset starts both players at correct HP");
  screenshot((std::string("ui-")+entry.first+"-home.bmp").c_str());
  click("P2");check(find(lv_screen_active(),"P2 / LIFE")&&!find(lv_screen_active(),"LIMIT 21"),"noncommander opponent menu edits life");
  rotate(-3);click("+");check(game.life(2)==entry.second-2&&game.player().life==entry.second,"opponent life changes independently");
  screenshot((std::string("ui-")+entry.first+"-opponent.bmp").c_str());click("Back");
  auto stored=game.serialize();mtg::GameState reloaded;check(reloaded.deserialize(stored)&&reloaded.life(2)==entry.second-2&&reloaded.state().format==game.state().format,"noncommander opponent HP survives recovery");
  ui.showStartup();tick();click("Continue");click("P2");check(find(lv_screen_active(),std::to_string(entry.second-2)),"Continue restores opponent life menu");click("Back");
  game.reset();tick();check(game.life(2)==entry.second&&game.player().life==entry.second,"reset restores both starting life totals");
 }
 game.endGame();ui.showStartup();tick();click("New game");click("SINGLE ARCANA");click("2HG");check(find(lv_screen_active(),"2 Teams / 4P"),"2HG shows team setup");rotate(100);check(find(lv_screen_active(),"6 Teams / 12P"),"team setup clamps at six teams");rotate(-100);check(find(lv_screen_active(),"2 Teams / 4P"),"team setup clamps at two teams");click("Confirm");tick(12);
 check(game.state().playerCount==4&&game.state().poisonLimit==15&&!find(lv_screen_active(),"P2")&&find(lv_screen_active(),"P3")&&find(lv_screen_active(),"P4"),"2HG has fixed teams and enemy-only tabs");
 game.adjustPlayerLife(2,-3);tick();check(game.player().life==37&&game.life(2)==37,"teammates share life");
 click("P3");check(find(lv_screen_active(),"P3 / CMD"),"2HG retains commander damage");rotate(4);check(game.player().life==33&&game.life(2)==33,"2HG linked damage reduces shared life");click("To P1");check(find(lv_screen_active(),"To P2"),"2HG switches the commander damage recipient");rotate(2);check(game.source(1)->damage==4&&game.source(2)->damage==2&&game.life(1)==31&&game.life(2)==31,"2HG damage kept separate by recipient while sharing life");screenshot("ui-2HG-opponent.bmp");click("Back");
 game.adjustCounter(mtg::Counter::Poison,14);check(!game.poisonWarning(),"2HG survives fourteen poison");game.adjustCounter(mtg::Counter::Poison,1);check(game.poisonWarning(),"2HG poison threshold fifteen");
 game.endGame();ui.showStartup();tick();click("New game");click("SINGLE ARCANA");click("2HG");click("+");check(find(lv_screen_active(),"3 Teams / 6P"),"team plus adds two players");rotate(3);check(find(lv_screen_active(),"6 Teams / 12P"),"six-team setup");screenshot("ui-6-teams-setup.bmp");click("Confirm");tick(12);
 check(game.state().playerCount==12&&find(lv_screen_active(),"P12"),"twelve-player game includes last enemy tile");screenshot("ui-6-teams-home.bmp");
 for(unsigned owner=3;owner<=12;++owner){double angle=(210+(owner-3+0.5)*12)*3.141592653589793/180;clickAt(static_cast<int>(std::round(180+160*std::cos(angle))),static_cast<int>(std::round(180+160*std::sin(angle))));check(find(lv_screen_active(),"P"+std::to_string(owner)+" / CMD"),"all ten enemy tiles open their own radial menu");click("Back");}
 game.adjustPlayerLife(12,-5);check(game.life(11)==35&&game.life(12)==35,"sixth team shares independent life");ui.showStartup();tick();click("Continue");check(game.state().playerCount==12&&find(lv_screen_active(),"P12"),"Continue restores six teams");
 random.values={1,2,3,4,5,20};random.index=0;click(LV_SYMBOL_SETTINGS);click("Roll");check(find(lv_screen_active(),"T6"),"starter roll supports all six teams");screenshot("ui-6-teams-roll.bmp");click("Back");click("Back");
 game.startNewGame(4);ui.goHome();tick(12);
 ui.openTrackers();ui.update();lv_obj_update_layout(lv_screen_active());auto* transitionTitle=find(lv_screen_active(),"TRACKERS");
 check(transitionTitle&&lv_obj_get_style_opa(transitionTitle,0)==0,"navigation starts a content fade");
 check(lv_obj_get_style_opa(lv_obj_get_child(lv_screen_active(),0),0)==255,"outer ring stays opaque during transition");
 tick(4);auto midway=lv_obj_get_style_opa(transitionTitle,0);check(midway>0&&midway<255,"transition fades through intermediate opacity");
 tick(8);check(lv_obj_get_style_opa(transitionTitle,0)==255,"transition finishes fully opaque");
 game.adjustLife(1);tick();transitionTitle=find(lv_screen_active(),"TRACKERS");check(transitionTitle&&lv_obj_get_style_opa(transitionTitle,0)==255,"stat refresh does not trigger a transition");
 ui.goHome();tick(12);auto* heart=find(lv_screen_active(),"\xef\x80\x84");game.adjustLife(1);tick();check(heart&&lv_obj_get_style_opa(heart,0)==255,"life adjustments remain instant");
 ui.openTrackers();ui.update();ui.goHome();tick(12);heart=find(lv_screen_active(),"\xef\x80\x84");check(heart&&lv_obj_get_style_opa(heart,0)==255,"rapid navigation cancels the previous transition");
 ui.openTrackers();ui.update();hardware.togglePower();ui.update();lv_obj_update_layout(lv_screen_active());transitionTitle=find(lv_screen_active(),"TRACKERS");check(transitionTitle&&lv_obj_get_style_opa(transitionTitle,0)==255,"power off cancels the transition");hardware.togglePower();
 ProfileMemory profileMemory;mtg::DeviceProfile identity(&profileMemory);UIWire wire;mtg::Multiplayer networking(game,identity,wire);
 game.endGame();mtg::AppUI multiUI(game,hardware,device,random,&identity,&networking);app=&multiUI;tick(12);
 click("New game");screenshot("ui-arcana-mode.bmp");click("MULTI ARCANA");
 for(char character:std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ")){click(std::string(1,character));auto* entered=labelAt(lv_screen_active(),180,96);check(entered&&std::string(lv_label_get_text(entered))==std::string(1,character),"all curved letter keys are reachable");click("DEL");}
 click("123");for(char character:std::string("0123456789-_")){click(std::string(1,character));auto* entered=labelAt(lv_screen_active(),180,96);check(entered&&std::string(lv_label_get_text(entered))==std::string(1,character),"all curved symbol keys are reachable");click("DEL");}screenshot("ui-name-symbol-keyboard.bmp");click("ABC");
 click("A");click("SP");click("B");check(find(lv_screen_active(),"A B"),"curved space key inserts a space");click("DEL");click("DEL");click("DEL");
 for(const char* key:{"A","L","I","C","E"})click(key);check(find(lv_screen_active(),"ALICE"),"name keyboard composes a name");screenshot("ui-name-keyboard.bmp");auto* changeLabel=find(lv_screen_active(),"CHANGE");lv_area_t changeArea;lv_obj_get_coords(lv_obj_get_parent(changeLabel),&changeArea);check(changeArea.y1>=300&&changeArea.y2<335,"CHANGE is an inset bottom button");auto* backLabel=find(lv_screen_active(),"Back");lv_area_t backArea;lv_obj_get_coords(lv_obj_get_parent(backLabel),&backArea);check(backArea.y1==136&&backArea.y2==223,"Back remains centered in its original circle");check(!find(lv_screen_active(),"YOUR NAME"),"OK replaces the old name heading");click("CHANGE");check(find(lv_screen_active(),"Enter your name")&&identity.name().empty(),"CHANGE clears only the unsaved draft");for(const char* key:{"A","L","I","C","E"})click(key);click("OK");check(find(lv_screen_active(),"Host"),"top OK confirms the name");
 mtg::DeviceProfile rebooted(&profileMemory);check(rebooted.load()&&rebooted.name()=="ALICE"&&rebooted.id()==identity.id(),"saved name survives a new profile reader");screenshot("ui-host-join.bmp");click("Join");check(find(lv_screen_active(),"Refresh"),"join opens discovery screen");
 using Json=nlohmann::json;const std::string bobId(32,'b');
 wire.incoming.push_back({"remote",Json{{"v",1},{"t","advert"},{"game","bob-game"},{"name","BOB"},{"format",0},{"capacity",4},{"joined",1},{"started",false}}.dump()});tick(12);check(find(lv_screen_active(),"BOB"),"host name appears in browser");screenshot("ui-discovered-games.bmp");click("BOB");
 auto networkState=[&](unsigned seq,bool started,int bobLife){return Json{{"v",1},{"t","state"},{"game","bob-game"},{"seq",seq},{"format",0},{"capacity",4},{"started",started},{"members",Json::array({Json{{"id",bobId},{"name","BOB"},{"seat",1},{"life",bobLife},{"counters",std::array<int,8>{}},{"ack",0},{"online",true}},Json{{"id",identity.id()},{"name","ALICE"},{"seat",2},{"life",40},{"counters",std::array<int,8>{}},{"ack",0},{"online",true}}})}}.dump();};
 wire.incoming.push_back({"remote",networkState(1,false,40)});tick(12);check(find(lv_screen_active(),"Waiting"),"joined guest waits for host start");screenshot("ui-network-lobby.bmp");
 wire.incoming.push_back({"remote",networkState(2,true,40)});tick(12);check(find(lv_screen_active(),"BOB")&&!find(lv_screen_active(),"P2"),"registered name replaces player number");auto* ownName=labelAt(lv_screen_active(),180,112);check(ownName&&std::string(lv_label_get_text(ownName))=="ALICE","own saved name appears above the upper icon in Multi");screenshot("ui-multi-home.bmp");
 click("BOB");check(find(lv_screen_active(),"Synced"),"remote player life is read-only");rotate(5);check(game.player().life==40&&networking.opponents()[0].life==40,"guest cannot edit another player's life");
 wire.incoming.push_back({"remote",networkState(3,true,32)});tick(12);check(find(lv_screen_active(),"32"),"remote life refreshes in player menu");screenshot("ui-multi-opponent.bmp");click("Back");
 click(LV_SYMBOL_SETTINGS);click("New Game");click("Yes");click("SINGLE ARCANA");click("Commander");click("Confirm");check(identity.name()=="ALICE"&&!networking.attached(),"new single game preserves identity and leaves network");
 click(LV_SYMBOL_SETTINGS);click("New Game");click("Yes");click("MULTI ARCANA");check(find(lv_screen_active(),"ALICE"),"name entry remembers saved name");click("CHANGE");check(identity.name()=="ALICE","CHANGE preserves the previously saved name");for(const char* key:{"A","L","I","C","E"})click(key);click("OK");click("Host");click("Classic");click("Confirm");check(networking.hosting()&&find(lv_screen_active(),"Start game"),"host configures a real lobby");click("Start game");check(networking.started()&&find(lv_screen_active(),"P2"),"host starts with one device and displays an open slot");click("P2");check(find(lv_screen_active(),"Open slot"),"unclaimed peer has a clear placeholder screen");click("+");check(networking.opponents()[0].life==21&&game.player().life==20,"host tracks a player without an Arcana");screenshot("ui-open-network-slot.bmp");click("Back");
 wire.incoming.push_back({"visitor",Json{{"v",1},{"t","find"}}.dump()});tick();std::string hostedId;for(const auto& packet:wire.sent){auto value=Json::parse(packet.data);if(value["t"]=="advert")hostedId=value["game"].get<std::string>();}check(!hostedId.empty(),"host advertises its lobby");
 wire.incoming.push_back({"guest",Json{{"v",1},{"t","join"},{"game",hostedId},{"id",std::string(32,'c')},{"name","CAROL"}}.dump()});tick(12);check(find(lv_screen_active(),"CAROL")&&!find(lv_screen_active(),"P2")&&game.player().life==20&&networking.opponents()[0].life==21,"late join replaces the placeholder while preserving tracked HP");screenshot("ui-late-joined-player.bmp");
 lv_deinit();std::cout<<"All round-screen counter interaction checks passed\n";
}
