// WindowMessage.h
#pragma once

#include "signal.hpp"
#include "widgets.hpp"

namespace mew {
using namespace ui;

//==============================================================================

template <>
struct Event<EventPreClose> {
  static void event(message& msg, IWindow* from) { msg["from"] = from; }
};
template <>
struct Event<EventRename> {
  static void event(message& msg, IWindow* from, const string& name) {
    msg["from"] = from;
    msg["name"] = name;
  }
};
template <>
struct Event<EventResize> {
  static void event(message& msg, IWindow* from, Size size) {
    msg["from"] = from;
    msg["size"] = size;
  }
};
template <>
struct Event<EventResizeDefault> {
  static void event(message& msg, IWindow* from, Size size) {
    msg["from"] = from;
    msg["size"] = size;
  }
};
template <>
struct Event<EventMouseWheel> {
  static void event(message& msg, IWindow* from, Point where, UINT32 state, INT32 wheel) {
    msg["from"] = from;
    msg["where"] = where;
    msg["state"] = state;
    msg["wheel"] = wheel;
  }
};
template <>
struct Event<EventData> {
  static void event(message& msg, IWindow* from, string data) {
    msg["from"] = from;
    msg["data"] = data;
  }
};
template <>
struct Event<EventUnsupported> {
  static void event(message& msg, IWindow* from, IMessage* what) {
    msg["from"] = from;
    msg["what"] = what;
  }
};

//==============================================================================

template <>
struct Event<EventItemFocus> {
  static void event(message& msg, IWindow* from, IUnknown* what) {
    msg["from"] = from;
    msg["what"] = what;
  }
};

//==============================================================================
}  // namespace mew
