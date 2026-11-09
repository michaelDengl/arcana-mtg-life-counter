#include <SDL.h>
#include "lvgl.h"
#include "SimulatorHardware.hpp"
#include "ui/AppUI.hpp"
#include "FileStorage.hpp"
#include "SimulatorRandom.hpp"
#include "core/SessionPersistence.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>
namespace {
constexpr int W=1040, H=600;
SDL_Renderer* renderer;
SDL_Texture* texture;
mtg::SimulatorHardware hardware;
mtg::GameState game;
mtg::AppUI* app;
mtg::SessionPersistence* persistence;
lv_obj_t* logLabel;
lv_obj_t* device;
std::vector<uint32_t> pixels(W*H);
int flushes=0;
void flush(lv_display_t* display,const lv_area_t* area,uint8_t* data) {
 for(int y=area->y1;y<=area->y2;++y)
  for(int x=area->x1;x<=area->x2;++x)
   pixels[y*W+x]=reinterpret_cast<uint32_t*>(data)[(y-area->y1)*(lv_display_get_buf_active(display)->header.stride/4)+x-area->x1];
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
 if(action==2) { if(hardware.powered())persistence->flush(); hardware.togglePower(); lv_obj_set_style_opa(device,hardware.powered()?255:0,0); lv_label_set_text(logLabel,hardware.powered()?"Power: on":"Power: off"); }
 if(action==3 && hardware.powered()) { app->requestNewGame(); lv_label_set_text(logLabel,"New game setup"); }
 if(action==4) { persistence->flush();persistence->load();app->showStartup(); if(!hardware.powered()) hardware.togglePower(); lv_obj_set_style_opa(device,255,0); lv_label_set_text(logLabel,"Restart: saved session loaded"); }
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
 bool smoke=false,resumeSmoke=false;std::string saveFile;
 for(int i=1;i<argc;++i) {
  std::string arg=argv[i];if(arg=="--smoke-test")smoke=true;
  else if(arg=="--resume-test"){smoke=true;resumeSmoke=true;}
  else if(arg=="--save-file" && i+1<argc)saveFile=argv[++i];
  else {std::cerr<<"Usage: mtg_simulator [--smoke-test | --resume-test] [--save-file path]\n";return 2;}
 }
 SDL_SetMainReady();
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) { std::cerr<<SDL_GetError();return 1; }
 if(saveFile.empty()) {
  if(smoke)saveFile="simulator-smoke-session.json";
  else {char* pref=SDL_GetPrefPath("Arcana","MTGLifeCounter");if(!pref){std::cerr<<SDL_GetError();SDL_Quit();return 1;}saveFile=std::string(pref)+"current-game.json";SDL_free(pref);}
 }
 mtg::FileStorage storage(std::filesystem::u8path(saveFile));mtg::SessionPersistence session(game,storage);persistence=&session;
 if(!session.load())std::cerr<<session.error()<<'\n';
 auto* window=SDL_CreateWindow("MTG Life Counter | Digital Twin 0.11.0",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,smoke?SDL_WINDOW_HIDDEN:SDL_WINDOW_SHOWN);
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
 card(18,120,243,250);card(18,381,243,186);card(758,120,264,330);card(758,461,264,106);
 text(root,"ARCANA / LIFE COUNTER",32,28,0x00d8ff);
 text(root,"DESKTOP DIGITAL TWIN",32,54,0x81a5b7);
 text(root,"YOUR COMMANDER SESSION",32,136,0x00d8ff);
 auto* stats=text(root,"",32,173);
 lv_obj_set_width(stats,216);lv_label_set_long_mode(stats,LV_LABEL_LONG_WRAP);
 text(root,"P1 life / opponent damage\nApp 0.11.0 / SDL2",32,318,0x81a5b7);
 text(root,"INPUT",32,398,0x00d8ff);
 text(root,"Wheel / arrows: adjust\nTap a tracker to edit\nTab: trackers\nCtrl+Z / Y: undo / redo\nP: power / R: new game\nEsc: close",32,431);
 auto* bezel=lv_obj_create(root);lv_obj_set_pos(bezel,290,91);lv_obj_set_size(bezel,420,420);
 lv_obj_set_style_radius(bezel,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(bezel,lv_color_hex(0x102331),0);
 lv_obj_set_style_border_width(bezel,8,0);lv_obj_set_style_border_color(bezel,lv_color_hex(0xc7364d),0);
 lv_obj_remove_flag(bezel,LV_OBJ_FLAG_SCROLLABLE);
 device=lv_obj_create(root);lv_obj_set_pos(device,320,121);lv_obj_set_size(device,360,360);
 lv_obj_set_style_border_width(device,0,0);lv_obj_set_style_pad_all(device,0,0);
 mtg::SimulatorRandom random;mtg::AppUI ui(game,hardware,device,random);app=&ui;
text(root,"KNOB TOUCH 1.8  /  DIGITAL TWIN",365,536,0x81a5b7);
 text(root,"HARDWARE CONTROLS",780,136,0x00d8ff);
 button(root,"Clockwise  +",780,174,1);button(root,"Counterclockwise  -",780,228,-1);
 button(root,"Power on / off",780,282,2);button(root,"Restart device",780,336,4);button(root,"New game",780,390,3);
text(root,"LATEST EVENT",780,477,0x00d8ff);logLabel=text(root,"Ready",780,506);
 lv_label_set_long_mode(logLabel,LV_LABEL_LONG_WRAP);lv_obj_set_width(logLabel,220);
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
    if(key==SDLK_p) {if(hardware.powered())session.flush();hardware.togglePower();lv_obj_set_style_opa(device,hardware.powered()?255:0,0);}
   }
   if(e.type==SDL_MOUSEBUTTONDOWN||e.type==SDL_MOUSEBUTTONUP) if(e.button.button==SDL_BUTTON_LEFT) hardware.setTouch({e.button.x,e.button.y,e.type==SDL_MOUSEBUTTONDOWN});
   if(e.type==SDL_MOUSEMOTION) hardware.setTouch({e.motion.x,e.motion.y,(e.motion.state&SDL_BUTTON_LMASK)!=0});
   if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_FOCUS_LOST) hardware.setTouch({});
  }
  auto now=SDL_GetTicks();lv_tick_inc(smoke?20:now-last);last=now;ui.update();lv_obj_set_style_opa(device,hardware.powered()?hardware.brightness()*255/100:0,0);lv_timer_handler();session.tick(SDL_GetTicks64());if(!hardware.powered())session.flush();ui.setStorageError(session.error());
  std::string summary=std::string(game.state().active?"MATCH IN PROGRESS":"NO ACTIVE MATCH")+"\nPlayers   "+std::to_string(game.state().playerCount)+"\nLife   "+std::to_string(game.player().life)+"\nPoison   "+std::to_string(game.player().counters[0])+" / "+std::to_string(game.state().poisonLimit)+"\nSources   "+std::to_string(game.state().sources.size())+"\nUndoable actions   "+std::to_string(game.historyCursor());
  if(summary!=lv_label_get_text(stats))lv_label_set_text(stats,summary.c_str());
  SDL_UpdateTexture(texture,nullptr,pixels.data(),W*4);SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,nullptr,nullptr);SDL_RenderPresent(renderer);
  if(smoke) {
   ++frames;
   // Exercise startup, the default four-player setup, wheel and both touch transports.
   auto pointer=[&](bool pressed,int x,int y){SDL_Event e{};e.type=pressed?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;SDL_PushEvent(&e);};
   if(resumeSmoke) {
    if(frames==4)pointer(true,500,358);if(frames==10)pointer(false,500,358);
   }else {
   if(frames==4)pointer(true,500,292);if(frames==10)pointer(false,500,292);
   if(frames==16)pointer(true,446,407);if(frames==22)pointer(false,446,407);
   if(frames==28){SDL_Event e{};e.type=SDL_MOUSEWHEEL;e.wheel.y=7;SDL_PushEvent(&e);}
   if(frames==34)pointer(true,870,195);if(frames==40)pointer(false,870,195);
   if(frames==46)pointer(true,355,262);if(frames==52)pointer(false,355,262);
   if(frames==54){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RIGHT;SDL_PushEvent(&e);}
   }
   if(frames==62)running=false;
  }
  SDL_Delay(smoke?1:5);
 }
 int result=session.flush()?0:1;
 if(result)std::cerr<<"Save failed: "<<session.error()<<'\n';
 if(smoke) {
  if(game.player().life!=49 || !game.state().active || game.state().playerCount!=4 || game.player().counters[static_cast<unsigned>(mtg::Counter::Treasure)]!=1 || flushes==0) result=1;
  auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),W,H,32,W*4,SDL_PIXELFORMAT_ARGB8888);
  if(surface) {SDL_SaveBMP(surface,resumeSmoke?"simulator-resume.bmp":"simulator-smoke.bmp");SDL_FreeSurface(surface);}
  std::cout<<"Smoke: life="<<game.player().life<<", LVGL flushes="<<flushes<<'\n';
 }
 lv_deinit();SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return result;
}
