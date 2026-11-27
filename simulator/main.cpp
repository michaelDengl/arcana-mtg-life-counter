#include <SDL.h>
#include "lvgl.h"
#include "SimulatorHardware.hpp"
#include "ui/AppUI.hpp"
#include "FileStorage.hpp"
#include "SimulatorRandom.hpp"
#include "LanTransport.hpp"
#include "core/DeviceProfile.hpp"
#include "core/Multiplayer.hpp"
#include "platform/RoundDisplay.hpp"
#include "core/AppInfo.hpp"
#include "ui/FaceDesigns.hpp"
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
namespace {
int W=1040,H=600;
SDL_Renderer* renderer;
SDL_Texture* texture;
mtg::SimulatorHardware hardware;
mtg::GameState game;
mtg::AppUI* app;
mtg::Multiplayer* network;
lv_obj_t* logLabel;
lv_obj_t* device;
std::vector<uint32_t> pixels(W*H);
std::array<lv_obj_t*,mtg::faceDesignCount> designButtons{};
std::array<lv_obj_t*,6> swatches{};
lv_obj_t* designName=nullptr;lv_obj_t* designDescription=nullptr;
lv_obj_t* lifeHelp=nullptr;
int flushes=0;unsigned selectorFace=mtg::faceDesignCount;
void refreshDesignSelector(){
 if(!designName)return;
 selectorFace=app->faceDesign();
 const auto& p=mtg::faceDesigns[selectorFace];
 lv_label_set_text(designName,p.name);lv_obj_set_style_text_color(designName,lv_color_hex(p.accent),0);lv_label_set_text(designDescription,p.description);
 for(unsigned i=0;i<designButtons.size();++i){bool selected=i==app->faceDesign();lv_obj_set_style_bg_color(designButtons[i],lv_color_hex(selected?0x18313a:0x0c1d27),0);lv_obj_set_style_border_color(designButtons[i],lv_color_hex(selected?p.accent:0x203743),0);}
 unsigned colors[]{p.background,p.surface,p.accent,p.secondary,p.gain,p.loss};for(unsigned i=0;i<swatches.size();++i)lv_obj_set_style_bg_color(swatches[i],lv_color_hex(colors[i]),0);
 if(lifeHelp)lv_label_set_text(lifeHelp,mtg::faceLayout(app->faceDesign()).vertical?"Life panel: top + / bottom -. Hold: 5 per second.":"Life panel: left - / right +. Hold: 5 per second.");
}
void chooseDesign(unsigned design){
 if(!game.state().active){network->leave();game.startNewGame(4);}
 app->setFaceDesign(design);refreshDesignSelector();
 if(logLabel)lv_label_set_text(logLabel,(std::string("Face: ")+mtg::faceDesigns[app->faceDesign()].name+"\nMatch values preserved").c_str());
}
void flush(lv_display_t* display,const lv_area_t* area,uint8_t* data) {
 for(int y=area->y1;y<=area->y2;++y)
  for(int x=area->x1;x<=area->x2;++x)
   pixels[y*W+x]=reinterpret_cast<uint32_t*>(data)[(y-area->y1)*(lv_display_get_buf_active(display)->header.stride/4)+x-area->x1];
 if(device){
  lv_area_t face;lv_obj_get_coords(device,&face);
  int left=std::max<int>(area->x1,face.x1),right=std::min<int>(area->x2,face.x2);
  if(left<=right)for(int y=std::max<int>(area->y1,face.y1);y<=std::min<int>(area->y2,face.y2);++y){
   // Composite the round panel onto its bezel. Its square corner pixels belong
   // to the desktop/bezel, while the panel's inner black perimeter stays black.
   double dy=y-face.y1+.5-180,half=std::sqrt(std::max(0.0,180.0*180-dy*dy));
   int begin=std::max(left,face.x1+static_cast<int>(std::ceil(179.5-half))),end=std::min(right+1,face.x1+static_cast<int>(std::floor(179.5+half))+1);
   if(begin<end)mtg::blackOutsideFace(&pixels[y*W+begin],begin-face.x1,y-face.y1,end-begin,1,4);
  }
 }
 ++flushes; lv_display_flush_ready(display);
}
void readTouch(lv_indev_t*,lv_indev_data_t* data) {
 auto touch=hardware.touch(); data->point.x=touch.x; data->point.y=touch.y;
 data->state=touch.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
lv_obj_t* text(lv_obj_t* parent,const char* value,int x,int y,unsigned color=0xc6d2dd) {
 auto* label=lv_label_create(parent); lv_label_set_text(label,value);
 lv_obj_set_pos(label,x,y); lv_obj_set_style_text_color(label,lv_color_hex(color),0); return label;
}
void event(lv_event_t* e) {
 auto action=reinterpret_cast<intptr_t>(lv_event_get_user_data(e));
 if(action==1 || action==-1) { hardware.rotate(static_cast<int>(action)); lv_label_set_text(logLabel,action==1 ? "Encoder: +1" : "Encoder: -1"); }
 if(action==2) { hardware.togglePower(); lv_obj_set_style_opa(device,hardware.powered()?255:0,0); lv_label_set_text(logLabel,hardware.powered()?"Power: on":"Power: off"); }
 if(action==3 && hardware.powered()) { app->requestNewGame(); lv_label_set_text(logLabel,"New game setup"); }
 if(action==4) { network->leave();game=mtg::GameState{};hardware.setTouch({});app->showStartup(); if(!hardware.powered()) hardware.togglePower(); lv_obj_set_style_opa(device,255,0); lv_label_set_text(logLabel,"Restart: new session"); }
 if(action>=100&&action<100+mtg::faceDesignCount)chooseDesign(static_cast<unsigned>(action-100));
 app->update();
}
void button(lv_obj_t* parent,const char* value,int x,int y,intptr_t action) {
 auto* b=lv_button_create(parent); lv_obj_set_pos(b,x,y); lv_obj_set_size(b,190,44);
 lv_obj_set_style_bg_color(b,lv_color_hex(0x0b2130),0);
 auto* l=lv_label_create(b); lv_label_set_text(l,value);lv_obj_center(l);
 lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,reinterpret_cast<void*>(action));
}
}
int main(int argc,char** argv) {
 bool smoke=false,startupSmoke=false;std::string profileFile,exportDirectory;unsigned initialDesign=mtg::faceDesignCount;
 for(int i=1;i<argc;++i) {
  std::string arg=argv[i];if(arg=="--smoke-test")smoke=true;
  else if(arg=="--startup-test"){smoke=true;startupSmoke=true;}
  else if(arg=="--profile-file" && i+1<argc)profileFile=argv[++i];
  else if(arg=="--face-studio"){}
  else if(arg=="--face-design"&&i+1<argc){try{initialDesign=static_cast<unsigned>(std::stoul(argv[++i]));}catch(...){return 2;}if(initialDesign>=mtg::faceDesigns.size())return 2;}
  else if(arg=="--export-faces"&&i+1<argc)exportDirectory=argv[++i];
  else {std::cerr<<"Usage: mtg_simulator [--face-studio] [--face-design 0.."<<mtg::faceDesignCount-1<<"] [--export-faces directory] [--smoke-test | --startup-test] [--profile-file path]\n";return 2;}
 }
 const bool studio=!smoke;if(studio){W=1240;H=760;pixels.assign(W*H,0);}
 SDL_SetMainReady();
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) { std::cerr<<SDL_GetError();return 1; }
 if(profileFile.empty()) {
  if(smoke)profileFile="simulator-smoke-profile.json";
  else {char* pref=SDL_GetPrefPath("Arcana","MTGLifeCounter");if(!pref){std::cerr<<SDL_GetError();SDL_Quit();return 1;}profileFile=std::string(pref)+"current-game-profile.json";SDL_free(pref);}
 }
 mtg::FileStorage profileStorage(std::filesystem::u8path(profileFile));mtg::DeviceProfile profile(&profileStorage);if(!profile.load())std::cerr<<profile.error()<<'\n';
 mtg::LanTransport transport;mtg::Multiplayer multiplayer(game,profile,transport);network=&multiplayer;
 const auto title=studio?std::string("ARCANA | Face Studio"):std::string("MTG Life Counter | Digital Twin ")+mtg::appInfo::version;
 auto* window=SDL_CreateWindow(title.c_str(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,smoke||!exportDirectory.empty()?SDL_WINDOW_HIDDEN:SDL_WINDOW_SHOWN);
 if(!window) { std::cerr<<SDL_GetError();SDL_Quit();return 1; }
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,W,H):nullptr;
 if(!texture) { std::cerr<<SDL_GetError();SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 1; }
 lv_init(); auto* display=lv_display_create(W,H);
 std::vector<uint32_t> buffer(W*60);
 lv_display_set_buffers(display,buffer.data(),nullptr,static_cast<uint32_t>(buffer.size()*4),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(display,flush);
 auto* input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,readTouch);
 auto* root=lv_screen_active();lv_obj_set_style_bg_color(root,lv_color_hex(0x061018),0);
 lv_obj_set_style_text_font(root,&lv_font_montserrat_14,0);
 lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
 lv_obj_set_style_bg_grad_color(root,lv_color_hex(0x0a1a25),0);lv_obj_set_style_bg_grad_dir(root,LV_GRAD_DIR_VER,0);
 auto card=[root](int x,int y,int w,int h){auto* c=lv_obj_create(root);lv_obj_set_pos(c,x,y);lv_obj_set_size(c,w,h);lv_obj_set_style_radius(c,18,0);lv_obj_set_style_bg_color(c,lv_color_hex(0x081924),0);lv_obj_set_style_border_color(c,lv_color_hex(0x12465a),0);lv_obj_set_style_border_width(c,1,0);lv_obj_remove_flag(c,LV_OBJ_FLAG_SCROLLABLE);};
 if(!studio){
 card(18,120,243,250);card(18,381,243,186);card(758,120,264,330);card(758,461,264,106);
 text(root,"ARCANA / LIFE COUNTER",32,28,0x00d8ff);
 text(root,"DESKTOP DIGITAL TWIN",32,54,0x81a5b7);
 text(root,"YOUR ARCANA SESSION",32,136,0x00d8ff);
 text(root,(std::string("P1 life / named network peers\nApp ")+mtg::appInfo::version+" / SDL2").c_str(),32,318,0x81a5b7);
 text(root,"INPUT",32,398,0x00d8ff);
 text(root,"Wheel / arrows: adjust\nTap a tracker to edit\nTab: trackers\nCtrl+Z / Y: undo / redo\nP: power / R: new game\nEsc: close",32,431);
 }
 auto* bezel=lv_obj_create(root);lv_obj_set_pos(bezel,290,91);lv_obj_set_size(bezel,420,420);
 lv_obj_set_style_radius(bezel,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(bezel,lv_color_hex(0x102331),0);
 lv_obj_set_style_border_width(bezel,8,0);lv_obj_set_style_border_color(bezel,lv_color_hex(0xc7364d),0);
 lv_obj_remove_flag(bezel,LV_OBJ_FLAG_SCROLLABLE);
 if(studio)lv_obj_set_pos(bezel,392,174);
 device=lv_obj_create(root);lv_obj_set_pos(device,studio?422:320,studio?204:121);lv_obj_set_size(device,360,360);
 lv_obj_set_style_border_width(device,0,0);lv_obj_set_style_pad_all(device,0,0);
 mtg::SimulatorRandom random;mtg::AppUI ui(game,hardware,device,random,&profile,&multiplayer);app=&ui;if(initialDesign>=mtg::faceDesignCount)initialDesign=ui.faceDesign();
 lv_obj_t* stats=nullptr;
 if(studio){
  text(root,"ARCANA / FACE STUDIO",24,25,0xe7f1f4);text(root,"Choose a design here or in Gear > Watch Face.",24,55,0x81a5b7);
  card(24,101,258,568);text(root,"CHOOSE A FACE",42,116,0x81a5b7);
  for(unsigned i=0;i<mtg::faceDesigns.size();++i){
   auto* b=lv_button_create(root);designButtons[i]=b;lv_obj_set_pos(b,36,143+i*37);lv_obj_set_size(b,234,33);lv_obj_set_style_radius(b,9,0);lv_obj_set_style_border_width(b,1,0);lv_obj_set_style_shadow_width(b,0,0);
   auto* chip=lv_obj_create(b);lv_obj_set_pos(chip,0,0);lv_obj_set_size(chip,5,20);lv_obj_set_style_bg_color(chip,lv_color_hex(mtg::faceDesigns[i].accent),0);lv_obj_set_style_border_width(chip,0,0);lv_obj_set_style_radius(chip,2,0);lv_obj_set_style_pad_all(chip,0,0);lv_obj_remove_flag(chip,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(chip,LV_OBJ_FLAG_SCROLLABLE);
   auto* caption=lv_label_create(b);auto name=(i<10?"0":"")+std::to_string(i)+"  "+mtg::faceDesigns[i].name;lv_label_set_text(caption,name.c_str());lv_obj_set_pos(caption,17,1);lv_obj_set_style_text_color(caption,lv_color_hex(0xe0edf1),0);
   lv_obj_add_event_cb(b,event,LV_EVENT_CLICKED,reinterpret_cast<void*>(static_cast<intptr_t>(100+i)));
  }
  card(24,679,258,63);text(root,"[ / ]  Previous / next design",42,690,0xc6d2dd);text(root,"Switching keeps your match.",42,714,0x81a5b7);
  designName=text(root,"",392,107);lv_obj_set_style_text_font(designName,&lv_font_montserrat_20,0);
  designDescription=text(root,"",392,137,0x81a5b7);lv_obj_set_width(designDescription,420);lv_label_set_long_mode(designDescription,LV_LABEL_LONG_WRAP);lv_obj_set_height(designDescription,34);
  card(392,620,420,111);text(root,"PALETTE / LIVE CONTROLS",411,635,0x81a5b7);
  for(unsigned i=0;i<swatches.size();++i){swatches[i]=lv_obj_create(root);lv_obj_set_pos(swatches[i],411+i*43,660);lv_obj_set_size(swatches[i],34,15);lv_obj_set_style_border_width(swatches[i],1,0);lv_obj_set_style_border_color(swatches[i],lv_color_hex(0x43606a),0);lv_obj_set_style_radius(swatches[i],4,0);lv_obj_set_style_pad_all(swatches[i],0,0);lv_obj_remove_flag(swatches[i],LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(swatches[i],LV_OBJ_FLAG_SCROLLABLE);}
  lifeHelp=text(root,"",411,692,0xc6d2dd);
  card(886,101,330,198);text(root,"YOUR ARCANA SESSION",906,119,0x81a5b7);stats=text(root,"",906,150);lv_obj_set_width(stats,286);lv_label_set_long_mode(stats,LV_LABEL_LONG_WRAP);
  card(886,315,330,296);text(root,"HARDWARE CONTROLS",906,330,0x81a5b7);
  button(root,"Clockwise  +",949,360,1);button(root,"Counterclockwise  -",949,409,-1);button(root,"Power on / off",949,458,2);button(root,"Restart device",949,507,4);button(root,"New game",949,556,3);
  card(886,627,330,104);text(root,"LATEST EVENT",906,642,0x81a5b7);logLabel=text(root,"Local preview ready",906,669);lv_obj_set_width(logLabel,284);lv_label_set_long_mode(logLabel,LV_LABEL_LONG_WRAP);
  game.startNewGame(4);hardware.setBattery(68);ui.setFaceDesign(initialDesign);refreshDesignSelector();
 }else{
 stats=text(root,"",32,173);lv_obj_set_width(stats,216);lv_label_set_long_mode(stats,LV_LABEL_LONG_WRAP);
 text(root,"KNOB TOUCH 1.8  /  DIGITAL TWIN",365,536,0x81a5b7);
 text(root,"HARDWARE CONTROLS",780,136,0x00d8ff);
 button(root,"Clockwise  +",780,174,1);button(root,"Counterclockwise  -",780,228,-1);
 button(root,"Power on / off",780,282,2);button(root,"Restart device",780,336,4);button(root,"New game",780,390,3);
text(root,"LATEST EVENT",780,477,0x00d8ff);logLabel=text(root,"Ready",780,506);
 lv_label_set_long_mode(logLabel,LV_LABEL_LONG_WRAP);lv_obj_set_width(logLabel,220);
 }
 if(!exportDirectory.empty()){
  auto directory=std::filesystem::u8path(exportDirectory);std::filesystem::create_directories(directory);
  game.startNewGame(4);game.adjustLife(1);game.adjustCounter(mtg::Counter::Treasure,7);game.adjustCounter(mtg::Counter::Poison,2);game.applyCommanderDamage(1,6,false);game.applyCommanderDamage(2,3,false);game.applyCommanderDamage(3,12,false);auto sample=game;
  for(unsigned design=0;design<mtg::faceDesigns.size();++design){
   game=sample;game.setActionTime(lv_tick_get());game.adjustLife(-1);ui.setFaceDesign(design);
   for(unsigned step=0;step<18;++step){lv_tick_inc(20);ui.update();lv_timer_handler();}
   if(!ui.faceControlsFit()){std::cerr<<"Layout controls overlap or leave the circular safe area: "<<design<<'\n';return 1;}
   lv_area_t area;lv_obj_get_coords(device,&area);auto* surface=SDL_CreateRGBSurfaceWithFormat(0,360,360,32,SDL_PIXELFORMAT_ARGB8888);
   if(!surface)return 1;
   for(int y=0;y<360;++y)std::memcpy(static_cast<uint8_t*>(surface->pixels)+y*surface->pitch,&pixels[(area.y1+y)*W+area.x1],360*4);
   mtg::blackOutsideFace(surface->pixels,0,0,360,360,4,surface->pitch);
   auto name=directory/("face-"+(design<10?std::string("0"):std::string{})+std::to_string(design)+".bmp");SDL_SaveBMP(surface,name.string().c_str());SDL_FreeSurface(surface);
   std::cout<<design<<": "<<mtg::faceDesigns[design].name<<'\n';
  }
  ui.setFaceDesign(initialDesign);refreshDesignSelector();lv_label_set_text(stats,"PREVIEW MATCH\nPlayers   4\nLife   40\nTreasure   7\nPoison   2 / 10\nCommander damage   6 / 3 / 12");
  for(unsigned step=0;step<18;++step){lv_tick_inc(20);ui.update();lv_timer_handler();}
  auto* studioImage=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),W,H,32,W*4,SDL_PIXELFORMAT_ARGB8888);
  if(studioImage){SDL_SaveBMP(studioImage,(directory/"studio.bmp").string().c_str());SDL_FreeSurface(studioImage);}
  lv_deinit();SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 0;
 }
 auto last=SDL_GetTicks();bool running=true;int frames=0;

 while(running) {
  SDL_Event e;
  while(SDL_PollEvent(&e)) {
   if(e.type==SDL_QUIT) running=false;
   if(e.type==SDL_MOUSEWHEEL) { int delta=e.wheel.direction==SDL_MOUSEWHEEL_FLIPPED?-e.wheel.y:e.wheel.y;hardware.rotate(delta);lv_label_set_text(logLabel,"Mouse wheel rotation"); }
   if(e.type==SDL_KEYDOWN) {
    auto key=e.key.keysym.sym;
    if(key==SDLK_ESCAPE) running=false;
    if(key==SDLK_UP||key==SDLK_RIGHT) hardware.rotate(1);
    if(key==SDLK_DOWN||key==SDLK_LEFT) hardware.rotate(-1);
    if(key==SDLK_r && hardware.powered()) {app->requestNewGame();}
    if(hardware.powered() && key==SDLK_TAB)app->openTrackers();
    if(hardware.powered() && (e.key.keysym.mod & KMOD_CTRL) && key==SDLK_z)app->undo();
    if(hardware.powered() && (e.key.keysym.mod & KMOD_CTRL) && key==SDLK_y)app->redo();
    if(key==SDLK_p) {hardware.togglePower();lv_obj_set_style_opa(device,hardware.powered()?255:0,0);}
    if(studio&&(key==SDLK_LEFTBRACKET||key==SDLK_RIGHTBRACKET))chooseDesign((ui.faceDesign()+mtg::faceDesigns.size()+(key==SDLK_RIGHTBRACKET?1:-1))%mtg::faceDesigns.size());
   }
   if(e.type==SDL_MOUSEBUTTONDOWN||e.type==SDL_MOUSEBUTTONUP) if(e.button.button==SDL_BUTTON_LEFT) hardware.setTouch({e.button.x,e.button.y,e.type==SDL_MOUSEBUTTONDOWN});
   if(e.type==SDL_MOUSEMOTION) hardware.setTouch({e.motion.x,e.motion.y,(e.motion.state&SDL_BUTTON_LMASK)!=0});
   if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_FOCUS_LOST) hardware.setTouch({});
  }
  auto now=SDL_GetTicks();lv_tick_inc(smoke?20:now-last);last=now;ui.update();if(studio&&selectorFace!=ui.faceDesign())refreshDesignSelector();lv_obj_set_style_opa(device,hardware.powered()?hardware.outputBrightness()*255/100:0,0);lv_timer_handler();
  std::string summary=std::string(game.state().active?"MATCH IN PROGRESS":"NO ACTIVE MATCH")+"\nPlayers   "+std::to_string(game.state().playerCount)+"\nLife   "+std::to_string(game.player().life)+"\nPoison   "+std::to_string(game.player().counters[0])+" / "+std::to_string(game.state().poisonLimit)+"\nSources   "+std::to_string(game.state().sources.size())+"\nUndoable actions   "+std::to_string(game.historyCursor());
  if(summary!=lv_label_get_text(stats))lv_label_set_text(stats,summary.c_str());
  SDL_UpdateTexture(texture,nullptr,pixels.data(),W*4);SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,nullptr,nullptr);SDL_RenderPresent(renderer);
  if(smoke) {
   ++frames;
   // Exercise startup, the default four-player setup, wheel and both touch transports.
   auto pointer=[&](bool pressed,int x,int y){SDL_Event e{};e.type=pressed?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_PushEvent(&e);};
   if(!startupSmoke) {
   if(frames==4)pointer(true,500,288);if(frames==10)pointer(false,500,288);
   if(frames==16)pointer(true,500,211);if(frames==22)pointer(false,500,211);
   if(frames==28)pointer(true,500,201);if(frames==34)pointer(false,500,201);
   if(frames==40)pointer(true,500,421);if(frames==46)pointer(false,500,421);
   if(frames==52){SDL_Event e{};e.type=SDL_MOUSEWHEEL;e.wheel.y=7;SDL_PushEvent(&e);}
   if(frames==58)pointer(true,870,195);if(frames==64)pointer(false,870,195);
   if(frames==70)pointer(true,355,262);if(frames==76)pointer(false,355,262);
   if(frames==82){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RIGHT;SDL_PushEvent(&e);}

   }
   if(frames==94)running=false;
  }
  SDL_Delay(smoke?1:5);
 }
 int result=0;
 if(smoke) {
  if(startupSmoke){if(game.state().active||game.player().life!=40||game.canUndo()||flushes==0)result=1;}
  else if(game.player().life!=49 || !game.state().active || game.state().playerCount!=4 || game.player().counters[static_cast<unsigned>(mtg::Counter::Treasure)]!=1 || flushes==0) result=1;
  auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),W,H,32,W*4,SDL_PIXELFORMAT_ARGB8888);
  if(surface) {SDL_SaveBMP(surface,startupSmoke?"simulator-fresh-start.bmp":"simulator-smoke.bmp");SDL_FreeSurface(surface);}
  std::cout<<"Smoke: life="<<game.player().life<<", LVGL flushes="<<flushes<<'\n';
  std::cout<<"Smoke profile="<<profile.name()<<'\n';
 }
 lv_deinit();SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return result;
}
