#pragma once
#include "lvgl.h"
#include "core/GameState.hpp"
#include "core/RandomTools.hpp"
#include "core/Multiplayer.hpp"
#include "platform/IHardware.hpp"
#include "Widgets/ToolIcons.hpp"
#include <deque>
namespace mtg {
class AppUI {
public:
 AppUI(GameState& game,IHardware& hardware,lv_obj_t* screen,IRandom& random,DeviceProfile* profile=nullptr,Multiplayer* multiplayer=nullptr);
 void update();
 void goHome();
 void openTrackers() { if(game_.state().active) navigate(View::Menu); }
 void showStartup() { navigate(View::Startup); }
 void requestNewGame();
 void setStorageError(const std::string& error);
 void undo() { handle(Undo,0); }
 void redo() { handle(Redo,0); }
private:
 enum class View { Startup, ArcanaMode, NameSetup, HostJoin, BrowseGames, NetworkLobby, FormatSelect, PlayerSetup, EndConfirm, Gear, Brightness, ResetConfirm, CoinResult, RollResult, CounterPicker, Home, Menu, Counter, Sources, Damage, Commanders, Casts, Profiles, Status, History, Rename, AddSource, Threshold };
 enum Action { SingleArcana,MultiArcana,NameKey,NameChange,NameDelete,NameSpace,NameSymbols,NameConfirm,HostGame,FindGames,JoinGame,RefreshGames,NextGames,StartNetwork, ToggleRecipient, ChooseFormat, NoAction, OpenBrightness, ToolBack, SideStep, OpenPicker, ChooseShortcut, RecentUndo, OpenGear, AskReset, ResetYes, ResetNo, FlipCoin, RollD20, NewGame, ContinueGame, ConfirmPlayers, SelectPlayer, CycleSource, CloseDial, RequestEnd, FinishGame, CancelEnd, Back, Menu, Minus, Plus, OpenCounter, OpenSources, OpenOwn, OpenProfiles, OpenStatus,
  OpenHistory, Undo, Redo, Apply, Mode, NewSource, CreateSource, NewOwn, NewTurn, Rename,
  Append, Delete, SaveName, Limit, ToggleCounter, Source, Own, ToggleStatus, Day };
 struct Binding { AppUI* ui; Action action; std::uint32_t arg; double sectorStart=0,sectorSpan=0; };
 static void undoSectorHit(lv_event_t* event);
 static void toolSectorHit(lv_event_t* event);
 static void brightnessChanged(lv_event_t* event);
 static void clicked(lv_event_t* event);
 static void lifeTouch(lv_event_t* event);
 static void playerWedgeHit(lv_event_t* event);
 void repeatLifeHold(std::uint32_t now);
 bool lifeHoldPositionValid() const;
 void handle(Action action,std::uint32_t arg);
 void navigate(View view);
 void adjust(std::int64_t delta);
 void build();
 void advanceTransition();
 void setContentOpacity(lv_opa_t opacity);
 void buildDial();
 void buildTools();
 void buildMultiplayer();
 void refreshMultiplayer();
 void beginArcanaSelection(View returnTo);
 bool networkFace()const{return multiplayer_&&multiplayer_->attached()&&multiplayer_->connected()&&multiplayer_->started();}
 lv_obj_t* radialSector(const std::string& title,unsigned index,unsigned count,Action action,std::uint32_t arg,unsigned color,widgets::ToolIcon icon);
 void radialBack(Action action=ToolBack);
 void buildPicker();
 lv_obj_t* roundPopup();
 void counterShortcuts();
 void refreshDial();
 void refreshBattery();
 void curvedUndoText(const std::string& value);
 void buildLifecycle();
 void beginFormatSelection(View returnTo);
 void refreshLifecycle();
 void playerTabs();
 void refreshTabs();
 void selectPlayer(unsigned player);
 void dialFrame();
 void refresh();
 void header(const std::string& title,const std::string& subtitle="");
 void historyButtons();
 lv_obj_t* list();
 void row(lv_obj_t* parent,int index,const std::string& title,const std::string& detail,Action action,std::uint32_t arg=0,unsigned color=0xb798ff);
 lv_obj_t* button(lv_obj_t* parent,const std::string& value,int x,int y,int w,int h,Action action,std::uint32_t arg=0,unsigned color=0x1d1c30);
 lv_obj_t* cutButton(lv_obj_t* circle,const std::string& value,int x,int y,int w,int h,Action action,std::uint32_t arg=0);
 void counterRockers();
 void partialDividers();
 lv_obj_t* bigValue(int y=115);
 void stepButtons(int y=216);
 DeviceProfile fallbackProfile_;DeviceProfile* profile_;Multiplayer* multiplayer_;
 std::string nameDraft_,networkSignature_;bool nameSymbols_=false,hostPending_=false;unsigned lobbyPage_=0;View arcanaReturn_=View::Startup;
 GameState& game_;
 IHardware& hardware_;
 IRandom& random_;
 unsigned pickerSide_=0;
 bool coinHeads_=false;
 StarterRoll starterRoll_;
 lv_obj_t* screen_;
 GameFormat draftFormat_=GameFormat::Commander;
 View formatReturn_=View::Startup;
 View view_=View::Startup, renameReturn_=View::Home, endReturn_=View::Gear;
 bool lifeHoldActive_=false, lifeHoldCancelled_=false;
 int lifeHoldSign_=1;
 std::uint32_t lifeHoldStarted_=0,lifeHoldRepeats_=0;
 Counter counter_=Counter::Poison;
 std::uint32_t sourceId_=1;
 std::size_t ownIndex_=0;
 std::int64_t pending_=1;
 unsigned owner_=2, character_=0, selectedPlayer_=1, draftPlayers_=4;
 std::array<lv_obj_t*,12> tabValues_{};
 std::string storageError_,undoCurvedText_;
 bool subtractLife_=true, rebuild_=true;
 bool transitionPending_=false,transitionActive_=false;
 std::uint32_t transitionStarted_=0,transitionFrame_=0,contentFirst_=0;
 std::string draft_, notice_;
 std::uint64_t revision_=0;
 std::deque<Binding> bindings_;
 std::deque<std::array<lv_point_precise_t,2>> tabEdgePoints_;
 int batteryState_=-2;
 lv_obj_t* battery_=nullptr;
 lv_obj_t* undoArc_=nullptr;
 lv_obj_t* brightnessSlider_=nullptr;
 lv_obj_t *recipientLabel_=nullptr,*lifeMarker_=nullptr,*lifeRight_=nullptr,*value_=nullptr,*detail_=nullptr,*hint_=nullptr,*aux_=nullptr,*status_=nullptr,*poison_=nullptr,*damage_=nullptr,*scroll_=nullptr,*apply_=nullptr,*mode_=nullptr,*undo_=nullptr,*redo_=nullptr;
};
}
