#pragma once
#include <array>
#include <algorithm>
#include "core/DeviceSettings.hpp"
namespace mtg {
struct FaceRect {
 int x=0,y=0,w=0,h=0;
 int cx()const{return x+w/2;}int cy()const{return y+h/2;}
};
struct FaceCounterLayout {FaceRect minus,icon,plus;};
enum class FaceNumberFormat {Whole,Padded,Split};
struct FaceLayout {
 FaceRect life,value,rightValue,heart,battery,gear,undo,status,name;
 std::array<FaceCounterLayout,2> counters;
 FaceNumberFormat format=FaceNumberFormat::Whole;
 bool vertical=false;
 int playersY=44;double numberScale=1.5;
 std::array<FaceRect,2> lifeTargets()const{
  if(vertical){int half=life.h/2;return {{{life.x,life.y+half,life.w,life.h-half},{life.x,life.y,life.w,half}}};}
  int half=life.w/2;return {{{life.x,life.y,half,life.h},{life.x+half,life.y,life.w-half,life.h}}};
 }
};
inline bool redesignedFace(unsigned id){return (id>0&&id<10)||(id>=11&&id<faceDesignCount);}
inline FaceCounterLayout counterBar(int x,int y,int w,int h){
 int step=w>=112?28:26;return {{x,y,step,h},{x+step+2,y,w-2*step-4,h},{x+w-step,y,step,h}};
}
inline FaceCounterLayout counterRail(int x,int y,int w,int h){
 int step=std::min(32,(h-44)/2-6);return {{x,y+h-step,w,step},{x,y+step+6,w,h-2*step-12},{x,y,w,step}};
}
inline FaceCounterLayout counterTile(int x,int y,int w,int h){
 int step=(w-2)/2;return {{x,y+h-30,step,30},{x,y,w,h-34},{x+step+2,y+h-30,w-step-2,30}};
}
inline FaceLayout faceLayout(unsigned id,unsigned opponents=3){
 FaceLayout p;
 p.name={106,84,148,16};p.playersY=44;
 switch(id){
  case 1:
   p.life={108,110,184,116};p.value={118,123,164,87};p.heart={268,86,32,32};p.battery={153,270,28,14};p.gear={110,265,36,36};p.undo={132,304,96,30};p.status={100,228,184,24};p.name={96,88,156,16};
   p.counters={counterRail(42,116,52,128),counterBar(190,254,116,40)};break;
  case 2:
   p.life={70,102,220,118};p.value={104,116,152,80};p.heart={52,112,34,34};p.battery={272,112,28,14};p.gear={100,288,38,38};p.undo={146,300,106,30};p.status={92,213,176,24};p.name={96,86,168,16};
   p.counters={counterBar(54,238,116,44),counterBar(190,238,116,44)};break;
  case 3:
   p.life={40,114,280,102};p.value={74,126,212,74};p.heart={48,76,32,32};p.battery={166,94,28,14};p.gear={276,76,36,36};p.undo={132,310,96,30};p.status={88,218,184,24};p.name={110,72,140,14};p.playersY=36;p.format=FaceNumberFormat::Padded;p.numberScale=1.25;
   p.counters={counterBar(54,248,116,42),counterBar(190,248,116,42)};break;
  case 4:
   p.life={116,124,128,158};p.value={120,166,120,68};p.heart={264,86,32,32};p.battery={166,104,28,14};p.gear={64,86,36,36};p.undo={132,300,96,30};p.status={118,282,124,16};p.name={106,82,148,16};p.playersY=38;p.vertical=true;p.numberScale=1.15;
   p.counters={counterRail(36,146,62,120),counterRail(262,146,62,120)};break;
  case 5:
   p.life={62,116,168,112};p.value={76,130,140,82};p.heart={56,82,32,32};p.battery={184,102,28,14};p.gear={260,92,38,38};p.undo={144,302,96,30};p.status={64,220,166,24};p.name={96,84,146,16};p.playersY=38;
   p.counters={counterBar(54,246,116,44),counterRail(252,144,62,132)};break;
  case 6:
   p.life={68,104,142,164};p.value={78,140,122,80};p.heart={124,228,30,30};p.battery={212,282,28,14};p.gear={98,286,40,40};p.undo={160,300,96,30};p.status={84,266,128,24};p.numberScale=1.3;
   p.counters={counterTile(232,116,78,70),counterTile(232,206,78,70)};break;
  case 7:
   p.life={64,136,232,106};p.value={70,145,102,88};p.rightValue={188,145,102,88};p.heart={82,258,34,34};p.battery={166,278,28,14};p.gear={244,258,40,40};p.undo={132,310,96,30};p.status={122,248,116,24};p.name={106,76,148,12};p.playersY=34;p.format=FaceNumberFormat::Split;p.numberScale=1.35;
   p.counters={counterBar(70,88,116,38),counterBar(190,88,116,38)};break;
  case 8:
   p.life={72,126,216,104};p.value={92,126,176,104};p.heart={46,90,32,32};p.battery={280,104,28,14};p.gear={160,300,40,40};p.undo={134,88,92,28};p.status={102,231,156,22};p.name={106,76,148,14};p.playersY=34;p.numberScale=1.65;
   p.counters={counterBar(54,254,116,40),counterBar(190,254,116,40)};break;
  case 9:
   p.life={112,106,152,140};p.value={112,110,76,84};p.rightValue={188,154,76,84};p.heart={160,238,32,32};p.battery={274,252,28,14};p.gear={86,278,40,40};p.undo={166,292,96,30};p.status={132,272,152,18};p.format=FaceNumberFormat::Split;p.numberScale=1.25;
   p.counters={counterRail(38,110,58,116),counterRail(272,132,54,112)};break;
  case 11:
   p.life={82,143,196,84};p.value={95,145,170,79};p.heart={78,94,32,32};p.battery={250,105,28,14};p.gear={163,88,34,34};p.undo={134,314,92,28};p.status={111,229,138,14};p.name={108,125,144,12};p.playersY=36;p.numberScale=1.4;
   p.counters={counterBar(54,246,116,37),counterBar(190,246,116,37)};break;
  case afterDarkFace:
   p.life={104,142,152,87};p.value={107,148,146,71};
   p.heart={163,112,34,30};p.battery={166,246,28,14};p.status={98,232,164,13};
   p.name={104,92,152,16};p.gear={206,302,36,34};p.undo={114,305,80,28};
   p.counters={counterBar(60,264,116,32),counterBar(184,264,116,32)};
   p.playersY=36;break;
  case cinderHexFace:
   p.life={104,139,152,89};p.value={108,147,144,68};
   p.heart={164,108,32,31};p.battery={166,245,28,14};p.status={97,229,166,14};
   p.name={108,90,144,15};p.gear={118,303,36,34};p.undo={166,305,80,28};
   p.counters={counterBar(60,263,116,32),counterBar(184,263,116,32)};
   p.playersY=34;break;
  default:
   p.life={72,82,216,216};p.value={94,161,68,58};p.rightValue={198,161,68,58};p.heart={162,120,36,37};p.battery={166,235,28,14};p.gear={158,168,44,44};p.undo={145,310,70,28};p.status={95,260,170,34};p.format=FaceNumberFormat::Split;
   p.counters={FaceCounterLayout{{24,245,40,30},{13,173,44,44},{24,129,40,36}},FaceCounterLayout{{302,245,40,30},{303,173,44,44},{302,129,40,36}}};break;
 }
 if(opponents>5)switch(id){
  case 3:p.gear={284,212,36,36};p.heart={44,92,32,32};p.name={84,96,140,14};p.battery={232,94,28,14};break;
  case 4:p.name={122,151,116,12};break;
  case 5:p.heart={52,90,32,32};p.name={96,94,146,14};p.battery={196,286,28,14};break;
  case 6:p.name={82,88,128,14};break;
  case 7:p.name={112,137,136,12};break;
  case 8:p.name={106,122,148,12};break;
  case 9:p.name={110,88,146,12};break;
  case 11:p.gear={163,90,34,34};break;
 }
 return p;
}
inline bool faceRectInside(FaceRect r,int radius=170){
 if(r.w<=0||r.h<=0)return false;
 for(int x:{r.x,r.x+r.w-1})for(int y:{r.y,r.y+r.h-1}){int dx=2*x+1-360,dy=2*y+1-360;if(dx*dx+dy*dy>4*radius*radius)return false;}
 return true;
}
inline bool faceRectsOverlap(FaceRect a,FaceRect b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
}
