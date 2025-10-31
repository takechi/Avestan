// WindowMessage.h
#pragma once

#include "signal.hpp"
#include "widgets.hpp"

namespace mew {

template <>
struct Event<ui::EventPreClose> {
  static void event(message& msg, ui::IWindow* from) { msg["from"] = from; }
};
template <>
struct Event<ui::EventRename> {
  static void event(message& msg, ui::IWindow* from, const string& name) {
    msg["from"] = from;
    msg["name"] = name;
  }
};
template <>
struct Event<ui::EventResize> {
  static void event(message& msg, ui::IWindow* from, Size size) {
    msg["from"] = from;
    msg["size"] = size;
  }
};
template <>
struct Event<ui::EventResizeDefault> {
  static void event(message& msg, ui::IWindow* from, Size size) {
    msg["from"] = from;
    msg["size"] = size;
  }
};
template <>
struct Event<ui::EventMouseWheel> {
  static void event(message& msg, ui::IWindow* from, Point where, UINT32 state, INT32 wheel) {
    msg["from"] = from;
    msg["where"] = where;
    msg["state"] = state;
    msg["wheel"] = wheel;
  }
};
template <>
struct Event<ui::EventData> {
  static void event(message& msg, ui::IWindow* from, string data) {
    msg["from"] = from;
    msg["data"] = data;
  }
};
template <>
struct Event<ui::EventUnsupported> {
  static void event(message& msg, ui::IWindow* from, IMessage* what) {
    msg["from"] = from;
    msg["what"] = what;
  }
};

//==============================================================================

template <>
struct Event<ui::EventItemFocus> {
  static void event(message& msg, ui::IWindow* from, IUnknown* what) {
    msg["from"] = from;
    msg["what"] = what;
  }
};

//==============================================================================
}  // namespace mew
