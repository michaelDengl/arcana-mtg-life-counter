#pragma once
#include <array>
#include "core/DeviceSettings.hpp"
namespace mtg {
// Shared desktop and firmware catalog; IDs are persisted in the device profile.
struct FaceDesign {
 const char* name;const char* description;
 unsigned background,surface,accent,secondary,gain,loss,ink,muted;
};
inline constexpr std::array<FaceDesign,faceDesignCount> faceDesigns{{
 {"Original","The current ARCANA face, unchanged.",0x07131e,0x0b2130,0x00d8ff,0x00e5b1,0x00e5b1,0xff3e70,0xf5fbff,0x81a5b7},
 {"Azure Comet","An offset life display, a left rail and a floating footer.",0x081b2c,0x102d47,0x79baff,0xf0d38d,0x9de8cb,0xff9e9a,0xf0f7ff,0x89abc7},
 {"Copper Petal","A broad life capsule with copper and powder-blue controls.",0x21131d,0x352333,0xf2c0a8,0xb6c4ee,0xabdcb9,0xed97ae,0xfff0df,0xc4a5ae},
 {"Neon Circuit","A three-digit dashboard with settings above the display.",0x090f12,0x101a20,0xc5f15b,0x9d83ff,0xc5f15b,0xe389cf,0xedf8e7,0x859696},
 {"Citrus Relay","Vertical life control between two complete counter rails.",0x0b211f,0x14342b,0xf4df71,0x77cfc3,0x95dfb7,0xffa271,0xfff9df,0x8cac9f},
 {"Sunset Meridian","Life on the left, a tall right counter and sunset colours.",0x20122b,0x341d39,0xffb66e,0xd58bf1,0xf2d67b,0xf98c9c,0xfff2dd,0xbca0b4},
 {"Verdant","A tall life card with two stacked counter tiles to its right.",0x0e1914,0x192a21,0xb7d19a,0xb78c64,0xb7d19a,0xdc9987,0xedf2df,0x91aa93},
 {"Prism","Split flip-card digits with both counters above the life display.",0x120f26,0x201c39,0xba9bff,0x68d7e8,0x8ee6d5,0xf493cf,0xf3edff,0xa09bbc},
 {"Obsidian","Oversized typography, Undo above and settings at the bottom.",0x090b0d,0x101316,0xe2e7e9,0xe5ad62,0xc7d1d2,0xe4a293,0xf4f6f6,0x7c898f},
 {"Magenta Ledger","Staggered digits and offset rails in pink and sky blue.",0x140e22,0x25182f,0xff8bd0,0x79bcef,0x83e6bd,0xf4b071,0xfff1fb,0xb29dbb},
 {"Ember Forge","Copper hardware, rivets and a bold instrument bezel.",0x171413,0x262320,0xf2a05f,0x8b9da1,0xe5bd70,0xec8070,0xf6e9d7,0xb3a492},
 {"Mana Confluence","Ivory, bronze and the five mana colours around a spell-circle dial.",0x15130f,0x242019,0xc8aa73,0x79b4d2,0x83b78e,0xd97760,0xf4ead5,0xa59a86},
 {"After Dark","Two retro fantasy pin-ups frame an ivory life display.",0x180c17,0x281524,0xf5c987,0xcb9adb,0xafe3c0,0xffa3a5,0xffefd9,0xd6b6c3},
 {"Cinder & Hex","Two original mages in the forge heat, framed by violet and amber.",0x210d15,0x301421,0xffb570,0xc68dff,0xffc58c,0xe3b5ff,0xfff3e7,0xd2b6bc}
}};
}
