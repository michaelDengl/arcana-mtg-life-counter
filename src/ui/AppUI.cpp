#include "AppUI.hpp"
#include "Widgets/DialWidgets.hpp"
#include "Widgets/ToolIcons.hpp"
#include <algorithm>
#include <limits>
LV_FONT_DECLARE(life_digits);
namespace mtg {
using namespace widgets;
namespace {
const std::string alphabet=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/'";
const char* dayName(DayNight state) {return state==DayNight::Day?"DAY":state==DayNight::Night?"NIGHT":"NO DAY / NIGHT";}
unsigned counterColor(Counter c) {
 switch(c) {case Counter::Poison:return red;case Counter::Energy:return gold;case Counter::Experience:return violet;case Counter::Rad:return teal;default:return violet;}
}
std::string signedNumber(std::int64_t n){return std::to_string(n);}
}
AppUI::AppUI(GameState& game,IHardware& hardware,lv_obj_t* screen,IRandom& random,DeviceProfile* profile,Multiplayer* multiplayer):profile_(profile?profile:&fallbackProfile_),multiplayer_(multiplayer),game_(game),hardware_(hardware),random_(random),screen_(screen) {build();refresh();}
void AppUI::clicked(lv_event_t* event) {
 auto* binding=static_cast<Binding*>(lv_event_get_user_data(event));
 if(binding->ui->hardware_.powered()) binding->ui->handle(binding->action,binding->arg);
}
lv_obj_t* AppUI::button(lv_obj_t* parent,const std::string& value,int x,int y,int w,int h,Action action,std::uint32_t arg,unsigned color) {
 auto* b=lv_button_create(parent);lv_obj_set_pos(b,x,y);lv_obj_set_size(b,w,h);
 lv_obj_set_style_radius(b,12,0);lv_obj_set_style_bg_color(b,lv_color_hex(color==0x1d1c30?panel:color),0);
 lv_obj_set_style_shadow_width(b,0,0);lv_obj_set_style_border_width(b,1,0);
 lv_obj_set_style_border_color(b,lv_color_hex(0x126580),0);
 lv_obj_set_style_bg_color(b,lv_color_hex(0x10212a),LV_STATE_DISABLED);
 lv_obj_set_style_opa(b,LV_OPA_40,LV_STATE_DISABLED);
 auto* l=label(b,value,0,0,w-8);lv_obj_center(l);
 if(value=="-" || value=="+") {
  auto accent=value=="-"?red:teal;
  lv_obj_set_style_border_color(b,lv_color_hex(accent),0);lv_obj_set_style_border_width(b,2,0);
  lv_obj_set_style_bg_color(b,lv_color_hex(value=="-"?0x28131f:0x092b2a),0);
  lv_obj_set_style_text_color(l,lv_color_hex(accent),0);lv_obj_set_style_text_font(l,&lv_font_montserrat_32,0);lv_obj_set_height(l,38);lv_obj_center(l);
 }
 if(value=="Close" || value=="Confirm" || value=="Continue" || value=="New game") {
  lv_obj_set_style_border_color(b,lv_color_hex(violet),0);lv_obj_set_style_border_width(b,2,0);
 }
 bindings_.push_back({this,action,arg});lv_obj_add_event_cb(b,clicked,LV_EVENT_CLICKED,&bindings_.back());return b;
}
void AppUI::header(const std::string& title,const std::string& subtitle) {
 button(screen_,LV_SYMBOL_LEFT,67,47,36,36,Back);
 label(screen_,title,108,54,(view_==View::Damage || view_==View::Casts)?138:152,&lv_font_montserrat_14,violet);
 if(view_==View::Damage || view_==View::Casts)
  undo_=button(screen_,"Undo",250,49,50,32,Undo);
 if(!subtitle.empty()) label(screen_,subtitle,62,85,236,&lv_font_montserrat_14,muted);
}
void AppUI::historyButtons() {
 auto* undo=undo_=button(screen_,"Undo",77,283,98,34,Undo);
 auto* redo=redo_=button(screen_,"Redo",185,283,98,34,Redo);
 if(!game_.canUndo()) lv_obj_add_state(undo,LV_STATE_DISABLED);
 if(!game_.canRedo()) lv_obj_add_state(redo,LV_STATE_DISABLED);
}
lv_obj_t* AppUI::list() {
 auto* obj=box(screen_,58,109,244,163,background);
 lv_obj_add_flag(obj,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(obj,LV_DIR_VER);
 lv_obj_set_scrollbar_mode(obj,LV_SCROLLBAR_MODE_AUTO);lv_obj_set_style_pad_right(obj,4,0);scroll_=obj;return obj;
}
void AppUI::row(lv_obj_t* parent,int index,const std::string& title,const std::string& detail,Action action,std::uint32_t arg,unsigned color) {
 auto* b=button(parent,"",0,index*55,236,49,action,arg);
 lv_obj_clean(b);
 label(b,title,10,6,212,&lv_font_montserrat_14,color);
 label(b,detail,10,26,212,&lv_font_montserrat_14,muted);
 lv_obj_set_style_pad_all(b,0,0);
}
lv_obj_t* AppUI::bigValue(int y) {
 auto* obj=label(screen_,"0",40,y,280,&lv_font_montserrat_48);return obj;
}
void AppUI::stepButtons(int y) {
 button(screen_,"-",74,y,70,42,Minus);button(screen_,"+",216,y,70,42,Plus);
}
void AppUI::navigate(View view) {transitionPending_=true;lifeHoldActive_=false;lifeHoldCancelled_=true;view_=view;notice_.clear();rebuild_=true;}
void AppUI::beginArcanaSelection(View returnTo){arcanaReturn_=returnTo;hostPending_=false;navigate(View::ArcanaMode);}
void AppUI::beginFormatSelection(View returnTo) {formatReturn_=returnTo;draftFormat_=game_.state().format;draftPlayers_=draftFormat_==GameFormat::Commander||draftFormat_==GameFormat::TwoHeadedGiant?4:2;navigate(View::FormatSelect);}
void AppUI::requestNewGame() {if(game_.state().active){formatReturn_=View::Home;navigate(View::ResetConfirm);}else beginArcanaSelection(View::Startup);}
void AppUI::goHome() {selectedPlayer_=1;navigate(game_.state().active?View::Home:View::Startup);}
void AppUI::setStorageError(const std::string& error) {if(error!=storageError_){storageError_=error;if(view_==View::Startup)rebuild_=true;}}
void AppUI::selectPlayer(unsigned player) {
 if(player<1 || player>(networkFace()?multiplayer_->opponents().size()+1:game_.state().playerCount))return;
 selectedPlayer_=player;
 if(player>1){sourceId_=0;for(const auto& s:game_.state().sources)if(s.owner==player){sourceId_=s.id;break;}}
 navigate(View::Home);
}
void AppUI::handle(Action action,std::uint32_t arg) {
 game_.setActionTime(lv_tick_get());
 if(!game_.state().active&&view_!=View::Startup&&view_!=View::ArcanaMode&&view_!=View::NameSetup&&view_!=View::HostJoin&&view_!=View::BrowseGames&&view_!=View::NetworkLobby&&view_!=View::PlayerSetup&&view_!=View::FormatSelect)return;

 switch(action) {
  case SingleArcana:hostPending_=false;beginFormatSelection(View::ArcanaMode);break;
  case MultiArcana:nameDraft_=profile_->name();nameSymbols_=false;navigate(View::NameSetup);break;
  case NameKey:if(nameDraft_.size()<12)nameDraft_+=static_cast<char>(arg);break;
  case NameDelete:if(!nameDraft_.empty())nameDraft_.pop_back();break;
  case NameSpace:if(!nameDraft_.empty()&&nameDraft_.size()<12&&nameDraft_.back()!=' ')nameDraft_+=' ';break;
  case NameSymbols:nameSymbols_=!nameSymbols_;rebuild_=true;break;
  case NameConfirm:if(profile_->setName(nameDraft_)){notice_.clear();navigate(View::HostJoin);}else notice_=profile_->error();break;
  case HostGame:if(!multiplayer_){notice_="Network unavailable";break;}hostPending_=true;beginFormatSelection(View::HostJoin);break;
  case FindGames:if(!multiplayer_){notice_="Network unavailable";break;}lobbyPage_=0;if(multiplayer_->attached())multiplayer_->leave();multiplayer_->discover();navigate(View::BrowseGames);break;
  case RefreshGames:if(multiplayer_)multiplayer_->discover();break;
  case NextGames:if(multiplayer_)lobbyPage_=(lobbyPage_+1)%std::max<std::size_t>(1,(multiplayer_->lobbies().size()+2)/3);rebuild_=true;break;
  case JoinGame:if(multiplayer_&&multiplayer_->join(arg))navigate(View::NetworkLobby);break;
  case StartNetwork:if(multiplayer_&&multiplayer_->start())goHome();break;
  case NoAction:break;
  case SideStep: {
   unsigned side=arg/2;auto counter=game_.state().shortcuts[side];auto value=game_.player().counters[static_cast<unsigned>(counter)];
   game_.adjustCounter(counter,arg%2?1:value>0?-1:0);break;
  }
  case OpenPicker:pickerSide_=arg;navigate(View::CounterPicker);break;
  case ChooseShortcut:game_.setShortcut(pickerSide_,static_cast<Counter>(arg));goHome();break;
  case RecentUndo:game_.undoRecent(lv_tick_get());break;
  case OpenBrightness:navigate(View::Brightness);break;
  case ToolBack:if(view_==View::ArcanaMode){navigate(arcanaReturn_);break;}if(view_==View::NameSetup){navigate(View::ArcanaMode);break;}if(view_==View::HostJoin){nameDraft_=profile_->name();navigate(View::NameSetup);break;}if(view_==View::BrowseGames){navigate(View::HostJoin);break;}if(view_==View::NetworkLobby){if(multiplayer_)multiplayer_->leave();game_.endGame();navigate(View::HostJoin);break;}if(view_==View::FormatSelect){navigate(formatReturn_);break;}if(view_==View::PlayerSetup){navigate(View::FormatSelect);break;}if(view_==View::Gear||view_==View::CounterPicker)goHome();else if(view_==View::EndConfirm)navigate(endReturn_);else navigate(View::Gear);break;
  case OpenGear:navigate(View::Gear);break;
  case AskReset:formatReturn_=View::Gear;navigate(View::ResetConfirm);break;
  case ResetYes:beginArcanaSelection(formatReturn_);break;
  case ResetNo:navigate(View::Gear);break;
  case FlipCoin:coinHeads_=flipCoin(random_);navigate(View::CoinResult);break;
  case RollD20:starterRoll_=rollStarter(game_.state().format==GameFormat::TwoHeadedGiant?game_.state().playerCount/2:game_.state().playerCount,random_);navigate(View::RollResult);break;
  case NewGame:beginArcanaSelection(View::Startup);break;
  case ChooseFormat:if(arg<static_cast<unsigned>(GameFormat::Count)){draftFormat_=static_cast<GameFormat>(arg);draftPlayers_=draftFormat_==GameFormat::Commander||draftFormat_==GameFormat::TwoHeadedGiant?4:2;navigate(View::PlayerSetup);}break;
  case ContinueGame:if(game_.state().active)selectPlayer(1);break;
  case ConfirmPlayers:if(hostPending_){if(multiplayer_&&multiplayer_->host(draftFormat_,draftPlayers_)){hostPending_=false;navigate(View::NetworkLobby);}else notice_=multiplayer_?multiplayer_->error():"Network unavailable";}else {if(multiplayer_&&multiplayer_->attached())multiplayer_->leave();if(game_.startNewGame(draftPlayers_,draftFormat_)){subtractLife_=true;selectPlayer(1);}}break;
  case SelectPlayer:selectPlayer(arg);break;
  case CloseDial:if(view_==View::CoinResult||view_==View::RollResult)navigate(View::Gear);else selectPlayer(1);break;
  case ToggleRecipient: {
   auto* current=game_.source(sourceId_);unsigned target=current&&current->recipient==1?2:1;
   for(const auto& source:game_.state().sources)if(source.owner==selectedPlayer_&&source.recipient==target){sourceId_=source.id;break;}break;
  }
  case CycleSource: {
   auto* current=game_.source(sourceId_);unsigned target=current?current->recipient:1;
   std::vector<std::uint32_t> sources;for(const auto& s:game_.state().sources)if(s.owner==selectedPlayer_&&s.recipient==target)sources.push_back(s.id);
   if(!sources.empty()){auto it=std::find(sources.begin(),sources.end(),sourceId_);sourceId_=(it==sources.end()||++it==sources.end())?sources.front():*it;}
   break;
  }
  case RequestEnd:endReturn_=view_;navigate(View::EndConfirm);break;
  case FinishGame:if(multiplayer_&&multiplayer_->attached())multiplayer_->leave();game_.endGame();navigate(View::Startup);break;
  case CancelEnd:navigate(endReturn_);break;
  case Back:
   if(view_==View::PlayerSetup) navigate(View::FormatSelect);
   else if(view_==View::FormatSelect)navigate(formatReturn_);
   else if(view_==View::EndConfirm) navigate(endReturn_);
   else if(view_==View::Counter)goHome();
   else if(view_==View::Rename) navigate(renameReturn_);
   else if(view_==View::Damage || view_==View::AddSource) navigate(View::Sources);
   else if(view_==View::Casts) navigate(View::Commanders);
   else if(view_==View::Threshold) navigate(View::Profiles);
   else if(view_==View::Menu) navigate(View::Home);
   else navigate(View::Menu);break;
  case Menu:navigate(View::Menu);break;
  case Minus:adjust(-1);break;
  case Plus:adjust(1);break;
  case OpenCounter:counter_=static_cast<Counter>(arg);navigate(View::Counter);break;
  case OpenSources:navigate(View::Sources);break;
  case OpenOwn:navigate(View::Commanders);break;
  case OpenProfiles:navigate(View::Profiles);break;
  case OpenStatus:navigate(View::Status);break;
  case OpenHistory:navigate(View::History);break;
  case Undo:notice_=game_.undo()?"Action undone":"Nothing to undo";rebuild_=true;break;
  case Redo:notice_=game_.redo()?"Action restored":"Nothing to redo";rebuild_=true;break;
  case Source:sourceId_=arg;pending_=1;navigate(View::Damage);break;
  case Own:ownIndex_=arg;navigate(View::Casts);break;
  case Mode:subtractLife_=!subtractLife_;break;
  case Apply:
   notice_=game_.applyCommanderDamage(sourceId_,pending_,subtractLife_)?"Recorded / Undo in History":"Amount out of range";
   pending_=1;break;
  case NewSource:owner_=game_.state().format==GameFormat::TwoHeadedGiant?3:std::min(2u,game_.state().playerCount);navigate(View::AddSource);break;
  case CreateSource:game_.addSource(owner_);navigate(View::Sources);break;
  case NewOwn:game_.addOwnCommander();navigate(View::Commanders);break;
  case NewTurn:game_.newTurn();notice_="New turn / spells reset";break;
  case ToggleCounter:game_.enableCounter(static_cast<Counter>(arg),!game_.player().enabled[arg]);rebuild_=true;break;
  case ToggleStatus:game_.toggleStatus(static_cast<Status>(arg));rebuild_=true;break;
  case Day:game_.cycleDayNight();rebuild_=true;break;
  case Limit:navigate(View::Threshold);break;
  case Rename:
   renameReturn_=view_;character_=1;
   if(view_==View::Damage && game_.source(sourceId_)) draft_=game_.source(sourceId_)->name;
   else if(view_==View::Casts) draft_=game_.state().commanders[ownIndex_].name;
   else draft_=game_.player().customName;
   navigate(View::Rename);break;
  case Append:if(draft_.size()<24) draft_+=alphabet[character_];break;
  case Delete:if(!draft_.empty())draft_.pop_back();break;
  case SaveName:
   if(draft_.find_first_not_of(' ')==std::string::npos){notice_="Enter a name first";break;}
   if(renameReturn_==View::Damage)game_.renameSource(sourceId_,draft_);
   else if(renameReturn_==View::Casts)game_.renameOwnCommander(ownIndex_,draft_);
   else game_.renameCustomCounter(draft_);
   navigate(renameReturn_);break;
 }
}
void AppUI::adjust(std::int64_t delta) {
 bool ok=true;
 switch(view_) {
  case View::Brightness:hardware_.setBrightness(static_cast<unsigned>(std::clamp<std::int64_t>(hardware_.brightness()+std::clamp<std::int64_t>(delta,-100,100),0,100)));break;
  case View::Startup:case View::ArcanaMode:case View::NameSetup:case View::HostJoin:case View::BrowseGames:case View::NetworkLobby:case View::FormatSelect:case View::EndConfirm:break;
  case View::PlayerSetup:if(draftFormat_==GameFormat::TwoHeadedGiant)draftPlayers_=2*static_cast<unsigned>(std::clamp<std::int64_t>(draftPlayers_/2+std::clamp<std::int64_t>(delta,-6,6),2,6));else if(draftFormat_==GameFormat::Commander||draftFormat_==GameFormat::Classic)draftPlayers_=static_cast<unsigned>(std::clamp<std::int64_t>(draftPlayers_+std::clamp<std::int64_t>(delta,-6,6),2,6));break;
  case View::Home:
   if(selectedPlayer_==1)ok=game_.adjustLife(delta);
   else if(networkFace()){auto peers=multiplayer_->opponents();if(selectedPlayer_-2<peers.size()&&peers[selectedPlayer_-2].id.empty())ok=multiplayer_->adjustOpenSeat(peers[selectedPlayer_-2].seat,delta);break;}
   else if(!usesCommanderDamage(game_.state().format))ok=game_.adjustPlayerLife(selectedPlayer_,delta);
   else {const auto* s=game_.source(sourceId_);if(s && delta<0 && delta < -s->damage)delta=-s->damage;ok=game_.adjustCommanderDamage(sourceId_,delta,subtractLife_);}break;
  case View::Counter:ok=game_.adjustCounter(counter_,delta);break;
  case View::Casts:ok=game_.adjustCasts(ownIndex_,delta);break;
  case View::Threshold: {
   auto old=game_.state().poisonLimit;
   if(delta>0 && old>std::numeric_limits<std::int64_t>::max()-delta)ok=false;
   else if(delta<0 && delta<1-old)ok=false;
   else ok=game_.setPoisonLimit(old+delta);break;
  }
  case View::Damage:
   if(delta>0 && pending_>std::numeric_limits<std::int64_t>::max()-delta)ok=false;
   else if(delta<0 && delta<1-pending_)pending_=1;
   else pending_+=delta;break;
  case View::AddSource:owner_=static_cast<unsigned>(std::clamp<std::int64_t>(static_cast<std::int64_t>(owner_)+std::clamp<std::int64_t>(delta,-6,6),game_.state().format==GameFormat::TwoHeadedGiant?3:1,game_.state().playerCount));break;
  case View::Rename:character_=static_cast<unsigned>((character_+delta%static_cast<std::int64_t>(alphabet.size())+alphabet.size())%alphabet.size());break;
  default:if(scroll_)lv_obj_scroll_by(scroll_,0,static_cast<int>(-std::clamp<std::int64_t>(delta,-10,10))*38,LV_ANIM_OFF);break;
 }
 notice_=ok?"":"Limit reached";
}
void AppUI::build() {
 transitionActive_=false;
 lv_obj_clean(screen_);bindings_.clear();tabEdgePoints_.clear();
 undoCurvedText_.clear();undoArc_=nullptr;battery_=nullptr;brightnessSlider_=nullptr;batteryState_=-2;
 recipientLabel_=nullptr;lifeMarker_=lifeRight_=value_=detail_=hint_=aux_=status_=poison_=damage_=scroll_=apply_=mode_=undo_=redo_=nullptr;
 tabValues_.fill(nullptr);
 lv_obj_set_style_bg_color(screen_,lv_color_hex(background),0);lv_obj_set_style_bg_opa(screen_,255,0);
 lv_obj_set_style_radius(screen_,LV_RADIUS_CIRCLE,0);lv_obj_set_style_clip_corner(screen_,true,0);
 lv_obj_set_style_border_width(screen_,0,0);lv_obj_set_style_pad_all(screen_,0,0);
 lv_obj_remove_flag(screen_,LV_OBJ_FLAG_SCROLLABLE);
 dialFrame();contentFirst_=lv_obj_get_child_count(screen_);
 const auto& state=game_.state();const auto& p=game_.player();
 switch(view_) {
  case View::ArcanaMode:case View::NameSetup:case View::HostJoin:case View::BrowseGames:case View::NetworkLobby:buildMultiplayer();break;
  case View::Startup:case View::FormatSelect:case View::PlayerSetup:buildLifecycle();break;
  case View::EndConfirm:case View::Brightness:case View::Gear:case View::ResetConfirm:case View::CoinResult:case View::RollResult:buildTools();break;
  case View::Home:buildDial();break;
  case View::CounterPicker:buildPicker();break;
  case View::Menu: {
   header("TRACKERS","Tap to edit / turn to scroll");auto* rows=list();int n=0;
   row(rows,n++,"POISON",std::to_string(p.counters[0])+" / "+std::to_string(state.poisonLimit),OpenCounter,0,red);
   if(usesCommanderDamage(state.format))row(rows,n++,"COMMANDER DAMAGE",std::to_string(state.sources.size())+" separate sources",OpenSources,0,gold);
   if(usesCommanderDamage(state.format)||state.format==GameFormat::Brawl)row(rows,n++,"YOUR COMMANDERS","Cast counts / next-cast tax",OpenOwn);
   for(unsigned i=1;i<counterCount;++i)if(p.enabled[i])
    row(rows,n++,i==5?p.customName:counterName(static_cast<Counter>(i)),std::to_string(p.counters[i]),OpenCounter,i,counterColor(static_cast<Counter>(i)));
   row(rows,n++,"DECK TRACKERS","Choose optional counters",OpenProfiles);
   row(rows,n++,"TABLE STATUS","Monarch / initiative / day",OpenStatus,0,gold);
   row(rows,n++,"HISTORY",std::to_string(game_.historyCursor())+" actions / undo & redo",OpenHistory);
   row(rows,n++,"END GAME","Finish match / disable Continue",RequestEnd,0,red);
   historyButtons();break;
  }
  case View::Counter: {
   playerTabs();
   auto* modal=roundPopup();
   label(screen_,counter_==Counter::Custom?p.customName:counterName(counter_),65,107,230,&lv_font_montserrat_20,violet);
   value_=bigValue(146);hint_=label(screen_,"",70,199,220,&lv_font_montserrat_14,muted);
   cutButton(modal,"-",22,223,140,39,Minus);cutButton(modal,"+",198,223,140,39,Plus);
   if(counter_==Counter::Spells)button(screen_,"New turn / reset",105,267,150,25,NewTurn);
   else if(counter_==Counter::Custom)button(screen_,"Rename",116,267,128,25,Rename);
   else undo_=button(screen_,"Undo",116,267,128,25,Undo);
   button(screen_,"Close",118,300,124,29,CloseDial);break;
  }
  case View::Sources: {
   header("DAMAGE RECEIVED","Per commander / threshold 21");auto* rows=list();int n=0;
   for(const auto& s:state.sources)row(rows,n++,s.name,std::to_string(s.damage)+" / 21"+(state.format==GameFormat::TwoHeadedGiant?" to P"+std::to_string(s.recipient):" combat damage"),Source,s.id,s.damage>=21?red:gold);
   button(screen_,"+ Commander source",80,283,200,34,NewSource);break;
  }
  case View::Damage: {
   auto* source=game_.source(sourceId_);if(!source){navigate(View::Sources);break;}
   header("COMBAT DAMAGE");button(screen_,source->name,65,84,230,33,Rename);
   detail_=label(screen_,"",65,123,230,&lv_font_montserrat_14,gold);
   value_=bigValue(147);
   button(screen_,"-",51,154,44,40,Minus);button(screen_,"+",265,154,44,40,Plus);
   hint_=label(screen_,"Amount to add / turn knob",60,203,240,&lv_font_montserrat_14,muted);
   auto* mode=button(screen_,"",66,228,228,35,Mode);mode_=label(mode,"",0,0,216);lv_obj_center(mode_);
   auto* apply=button(screen_,"",94,277,172,39,Apply,0,0x584226);apply_=label(apply,"",0,0,160);lv_obj_center(apply_);break;
  }
  case View::Commanders: {
   header("YOUR COMMANDERS","Count command-zone casts only");auto* rows=list();int n=0;
   for(std::size_t i=0;i<state.commanders.size();++i)
    row(rows,n++,state.commanders[i].name,"Casts "+std::to_string(state.commanders[i].casts)+" / next +"+std::to_string(game_.nextTax(i)),Own,static_cast<std::uint32_t>(i),teal);
   button(screen_,"+ Partner / commander",75,283,210,34,NewOwn);break;
  }
  case View::Casts: {
   if(ownIndex_>=state.commanders.size()){navigate(View::Commanders);break;}
   header("COMMANDER TAX");button(screen_,state.commanders[ownIndex_].name,65,84,230,33,Rename);
   value_=bigValue(127);detail_=label(screen_,"",60,187,240,&lv_font_montserrat_20,teal);
   label(screen_,"Next cast / extra generic mana",55,214,250,&lv_font_montserrat_14,muted);
   button(screen_,"-",75,251,58,40,Minus);button(screen_,"Cast +1",143,251,142,40,Plus,0,0x20463f);
   label(screen_,"Partners tracked separately",55,303,250,&lv_font_montserrat_14,muted);break;
  }
  case View::Profiles: {
   header("DECK TRACKERS","Hide without clearing values");auto* rows=list();int n=0;
   for(unsigned i=1;i<static_cast<unsigned>(Counter::Treasure);++i) row(rows,n++,counterName(static_cast<Counter>(i)),p.enabled[i]?"ENABLED / tap to hide":"OFF / tap to enable",ToggleCounter,i,p.enabled[i]?teal:muted);
   row(rows,n++,"POISON THRESHOLD",std::to_string(state.poisonLimit)+" / tap to customize",Limit,0,red);
   historyButtons();break;
  }
  case View::Status: {
   header("TABLE STATUS","Your badges / table day & night");auto* rows=list();
   row(rows,0,"MONARCH",p.monarch?"I AM THE MONARCH":"Not me",ToggleStatus,0,gold);
   row(rows,1,"INITIATIVE",p.initiative?"I HAVE THE INITIATIVE":"Not me",ToggleStatus,1,teal);
   row(rows,2,"CITY'S BLESSING",p.cityBlessing?"YES / correction toggles off":"Not yet",ToggleStatus,2,violet);
   row(rows,3,"DAY / NIGHT",dayName(state.dayNight),Day);
   historyButtons();break;
  }
  case View::History: {
   header("ACTION HISTORY","64 most recent actions");auto* rows=list();int n=0;
   for(std::size_t i=game_.history().size();i>0;--i) {
    row(rows,n++,game_.history()[i-1].description,i<=game_.historyCursor()?"APPLIED":"UNDONE",OpenHistory,0,i<=game_.historyCursor()?teal:muted);
    lv_obj_remove_flag(lv_obj_get_child(rows,n-1),LV_OBJ_FLAG_CLICKABLE);
   }
   if(game_.history().empty())label(rows,"No actions yet",20,20,196,&lv_font_montserrat_14,muted);
   historyButtons();break;
  }
  case View::AddSource:
   header("ADD COMMANDER","Select the commander's owner");
   value_=bigValue();stepButtons(209);
   label(screen_,"Select owner's player number",55,181,250,&lv_font_montserrat_14,muted);
   button(screen_,"Add source",96,276,168,39,CreateSource,0,0x584226);break;
  case View::Threshold:
   header("POISON LIMIT","Default 10 / adjust for variants");value_=bigValue();stepButtons(214);historyButtons();break;
  case View::Rename:
   header("EDIT NAME","Turn knob to choose a character");
   detail_=label(screen_,"",60,104,240,&lv_font_montserrat_14,teal);
   value_=bigValue(145);button(screen_,"<",55,152,45,40,Minus);button(screen_,">",260,152,45,40,Plus);
   button(screen_,"Append character",83,216,194,41,Append);
   button(screen_,"Delete",76,277,99,38,Delete);button(screen_,"Save",185,277,99,38,SaveName,0,0x40305e);break;
 }
 rebuild_=false;revision_=game_.revision();
 if(transitionPending_&&hardware_.powered()) {transitionActive_=true;transitionStarted_=transitionFrame_=lv_tick_get();setContentOpacity(0);}
 transitionPending_=false;
}
void AppUI::refresh() {
 if(view_==View::Counter)refreshTabs();
 if(undo_ && !(view_==View::Home && selectedPlayer_==1)) {if(game_.canUndo())lv_obj_remove_state(undo_,LV_STATE_DISABLED);else lv_obj_add_state(undo_,LV_STATE_DISABLED);}
 if(redo_) {if(game_.canRedo())lv_obj_remove_state(redo_,LV_STATE_DISABLED);else lv_obj_add_state(redo_,LV_STATE_DISABLED);}
 const auto& p=game_.player();const auto& state=game_.state();
 switch(view_) {
  case View::ArcanaMode:case View::NameSetup:case View::HostJoin:case View::BrowseGames:case View::NetworkLobby:refreshMultiplayer();break;
  case View::Startup:case View::FormatSelect:case View::PlayerSetup:case View::EndConfirm:refreshLifecycle();break;
  case View::Home:refreshDial();break;
  case View::Brightness:text(value_,std::to_string(hardware_.brightness())+"%");if(brightnessSlider_)lv_slider_set_value(brightnessSlider_,hardware_.brightness(),LV_ANIM_OFF);break;
  case View::Counter: {
   auto value=p.counters[static_cast<unsigned>(counter_)];number(value_,std::to_string(value));
   std::string help="Player counter / does not expire";
   if(counter_==Counter::Poison)help=value>=state.poisonLimit?"POISON LIMIT REACHED":std::to_string(value)+" / "+std::to_string(state.poisonLimit)+" poison";
   if(counter_==Counter::Rad)help="Resolve at precombat main phase";
   if(counter_==Counter::Spells)help="All players' spells / this turn";
   if(counter_==Counter::Custom)help="Custom player tracker";
   if(counter_==Counter::Treasure)help="Available Treasure tokens";
   if(counter_==Counter::Shield)help="Manual shield tally";
   text(hint_,notice_.empty()?help:notice_);lv_obj_set_style_text_color(hint_,lv_color_hex(counter_==Counter::Poison&&game_.poisonWarning()?red:muted),0);break;
  }
  case View::Damage: {
   auto* source=game_.source(sourceId_);if(!source)break;
   number(value_,std::to_string(pending_));text(detail_,"RECEIVED  "+std::to_string(source->damage)+" / 21");
   lv_obj_set_style_text_color(detail_,lv_color_hex(source->damage>=21?red:gold),0);
   text(mode_,subtractLife_?"Also subtract life: ON":"Record only / life unchanged");
   text(apply_,"Apply "+std::to_string(pending_)+" damage");
   text(hint_,notice_.empty()?"Amount to add / turn knob":notice_);break;
  }
  case View::Casts:
   if(ownIndex_<state.commanders.size()){number(value_,std::to_string(state.commanders[ownIndex_].casts));text(detail_,"NEXT TAX  +"+std::to_string(game_.nextTax(ownIndex_)));}break;
  case View::AddSource:number(value_,"P"+std::to_string(owner_));break;
  case View::Threshold:number(value_,std::to_string(state.poisonLimit));break;
  case View::Rename:number(value_,std::string(1,alphabet[character_]));text(detail_,draft_.empty()?"(empty)":draft_);break;
  default:break;
 }
}
void AppUI::setContentOpacity(lv_opa_t opacity) {
 for(unsigned i=contentFirst_;i<lv_obj_get_child_count(screen_);++i)lv_obj_set_style_opa(lv_obj_get_child(screen_,i),opacity,0);
}
void AppUI::advanceTransition() {
 if(!transitionActive_)return;
 auto now=lv_tick_get(),elapsed=now-transitionStarted_;
 if(!hardware_.powered()||elapsed>=180){setContentOpacity(LV_OPA_COVER);transitionActive_=false;return;}
 if(now-transitionFrame_<33)return;transitionFrame_=now;
 // Ease out over six short frames, then perform no further animation work.
 unsigned remaining=180-elapsed;setContentOpacity(static_cast<lv_opa_t>(255-255*remaining*remaining/(180*180)));
}
void AppUI::update() {
 if(multiplayer_){
  multiplayer_->tick(lv_tick_get());std::string signature=multiplayer_->error()+std::to_string(multiplayer_->connected())+std::to_string(multiplayer_->started());
  for(const auto& p:multiplayer_->players())signature+=p.id+p.name+std::to_string(p.online);
  for(const auto& l:multiplayer_->lobbies())signature+=l.id+l.name+std::to_string(l.joined)+std::to_string(l.started);
  if(signature!=networkSignature_){networkSignature_=signature;if(view_==View::BrowseGames||view_==View::NetworkLobby||(view_==View::Home&&networkFace()))rebuild_=true;lifeHoldActive_=false;lifeHoldCancelled_=true;}
  if(view_==View::NetworkLobby&&multiplayer_->connected()&&multiplayer_->started())goHome();
 }

 if(lifeHoldActive_){if(!hardware_.powered()){lifeHoldActive_=false;lifeHoldCancelled_=true;}else if(hardware_.touch().pressed)repeatLifeHold(lv_tick_get());}
 if(view_!=View::Startup && view_!=View::PlayerSetup && view_!=View::FormatSelect && view_!=View::ArcanaMode&&view_!=View::NameSetup&&view_!=View::HostJoin&&view_!=View::BrowseGames&&view_!=View::NetworkLobby && !game_.state().active)navigate(View::Startup);
 game_.setActionTime(lv_tick_get());
 auto delta=hardware_.takeRotation();if(delta && hardware_.powered())adjust(delta);
 if((view_==View::Damage && !game_.source(sourceId_)) || (view_==View::Casts && ownIndex_>=game_.state().commanders.size()))navigate(View::Menu);
 if(game_.revision()!=revision_) {
  if(view_==View::Startup)rebuild_=true;
  if(view_==View::Menu || view_==View::Profiles || view_==View::Status || view_==View::History || view_==View::Sources || view_==View::Commanders)rebuild_=true;
  revision_=game_.revision();
 }
 if(rebuild_)build();refresh();advanceTransition();
}
}
