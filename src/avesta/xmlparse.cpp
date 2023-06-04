// xmlparse.cpp

#include "stdafx.h"
#include "../server/main.hpp"
#include "std/algorithm.hpp"

namespace {

const PCWSTR ATTR_MODIFIER = L"modifier";
const PCWSTR ATTR_TEXT = L"text";
const PCWSTR ATTR_IMAGE = L"image";
const PCWSTR ATTR_KEY = L"key";
// const PCWSTR ATTR_INPUT = L"input";

struct VirtualKey {
  PCWSTR name;
  UINT8 vkey;
};

static bool operator<(const VirtualKey& lhs, PCWSTR rhs) { return _wcsicmp(lhs.name, rhs) < 0; }
static bool operator==(const VirtualKey& lhs, PCWSTR rhs) { return _wcsicmp(lhs.name, rhs) == 0; }
static bool operator<(const VirtualKey& lhs, DWORD key) { return lhs.vkey < key; }
static bool operator==(const VirtualKey& lhs, DWORD key) { return lhs.vkey == key; }

// キー名 to キーコード 配列。大文字小文字を区別しないキー名でソート済みであること。
const VirtualKey VIRTUALKEYS[] = {
    L",",        0xBC,        L"-",        0xBD,        L".",         0xBE,         L"/",         0xBF,          L"0",
    0x30,        L"1",        0x31,        L"2",        0x32,         L"3",         0x33,         L"4",          0x34,
    L"5",        0x35,        L"6",        0x36,        L"7",         0x37,         L"8",         0x38,          L"9",
    0x39,        L":",        0xBA,        L";",        0xBB,         L"@",         0xC0,         L"[",          0xDB,
    L"\\",       0xDC,        L"]",        0xDD,        L"^",         0xDE,         L"_",         0xE2,          L"a",
    0x41,        L"add",      VK_ADD,      L"b",        0x42,         L"back",      VK_BACK,      L"backspace",  VK_BACK,
    L"bs",       VK_BACK,     L"c",        0x43,        L"d",         0x44,         L"decimal",   VK_DECIMAL,    L"delete",
    VK_DELETE,   L"div",      VK_DIVIDE,   L"divide",   VK_DIVIDE,    L"down",      VK_DOWN,      L"e",          0x45,
    L"end",      VK_END,      L"enter",    VK_RETURN,   L"esc",       VK_ESCAPE,    L"escape",    VK_ESCAPE,     L"f",
    0x46,        L"f1",       VK_F1,       L"f10",      VK_F10,       L"f11",       VK_F11,       L"f12",        VK_F12,
    L"f13",      VK_F13,      L"f14",      VK_F14,      L"f15",       VK_F15,       L"f16",       VK_F16,        L"f17",
    VK_F17,      L"f18",      VK_F18,      L"f19",      VK_F19,       L"f2",        VK_F2,        L"f20",        VK_F20,
    L"f21",      VK_F21,      L"f22",      VK_F22,      L"f23",       VK_F23,       L"f24",       VK_F24,        L"f3",
    VK_F3,       L"f4",       VK_F4,       L"f5",       VK_F5,        L"f6",        VK_F6,        L"f7",         VK_F7,
    L"f8",       VK_F8,       L"f9",       VK_F9,       L"g",         0x47,         L"h",         0x48,          L"home",
    VK_HOME,     L"i",        0x49,        L"ins",      VK_INSERT,    L"insert",    VK_INSERT,    L"j",          0x4A,
    L"k",        0x4B,        L"l",        0x4C,        L"left",      VK_LEFT,      L"m",         0x4D,          L"mul",
    VK_MULTIPLY, L"multiply", VK_MULTIPLY, L"n",        0x4E,         L"next",      VK_NEXT,      L"num *",      VK_MULTIPLY,
    L"num +",    VK_ADD,      L"num -",    VK_SUBTRACT, L"num .",     VK_DECIMAL,   L"num /",     VK_DIVIDE,     L"num 0",
    VK_NUMPAD0,  L"num 1",    VK_NUMPAD1,  L"num 2",    VK_NUMPAD2,   L"num 3",     VK_NUMPAD3,   L"num 4",      VK_NUMPAD4,
    L"num 5",    VK_NUMPAD5,  L"num 6",    VK_NUMPAD6,  L"num 7",     VK_NUMPAD7,   L"num 8",     VK_NUMPAD8,    L"num 9",
    VK_NUMPAD9,  L"o",        0x4F,        L"p",        0x50,         L"pagedown",  VK_NEXT,      L"pageup",     VK_PRIOR,
    L"prior",    VK_PRIOR,    L"q",        0x51,        L"r",         0x52,         L"return",    VK_RETURN,     L"right",
    VK_RIGHT,    L"s",        0x53,        L"sep",      VK_SEPARATOR, L"separator", VK_SEPARATOR, L"space",      VK_SPACE,
    L"sub",      VK_SUBTRACT, L"subtract", VK_SUBTRACT, L"t",         0x54,         L"tab",       VK_TAB,        L"u",
    0x55,        L"up",       VK_UP,       L"v",        0x56,         L"w",         0x57,         L"x",          0x58,
    L"y",        0x59,        L"z",        0x5A,        L"←",         VK_LEFT,      L"↑",         VK_UP,         L"→",
    VK_RIGHT,    L"↓",        VK_DOWN,     L"カタカナ", VK_KANA,      L"カナ",      VK_KANA,      L"ローマ字",   VK_KANA,
    L"半角",     VK_KANJI,    L"変換",     VK_CONVERT,  L"漢字",      VK_KANJI,     L"無変換",    VK_NONCONVERT,
};

#ifdef _DEBUG
struct VirtualKeyVerifier {
  VirtualKeyVerifier() {
    // VIRTUALKEYS はソート済みでなければならないので、一応確認しておく。
    const int NUMKEYS = sizeof(VIRTUALKEYS) / sizeof(VIRTUALKEYS[0]);
    for (int i = 0; i < NUMKEYS - 1; i++) {
      if (!(VIRTUALKEYS[i] < VIRTUALKEYS[i + 1].name)) {
        PCWSTR lhs = VIRTUALKEYS[i].name;
        PCWSTR rhs = VIRTUALKEYS[i + 1].name;
        ASSERT(!"VIRTUALKEYS はソート済みでなければならない");
      }
    }
  }
} _verify;
#endif
}  // namespace

//==============================================================================

UINT8 avesta::XmlAttrKey(mew::xml::XMLAttributes& attr) {
  if (mew::string key = attr[ATTR_KEY]) {
    const VirtualKey* begin = VIRTUALKEYS;
    const VirtualKey* end = begin + lengthof(VIRTUALKEYS);
    const VirtualKey* found = mew::algorithm::binary_search(begin, end, key.str());
    return (found == end) ? 0 : found->vkey;
  }
  return 0;
}

//==============================================================================

mew::string avesta::XmlAttrText(mew::xml::XMLAttributes& attr) {
  if (mew::string text = attr[ATTR_TEXT]) {
    return text.replace(L'_', L'&');
  } else {
    return mew::null;
  }
}

//==============================================================================

int avesta::XmlAttrImage(mew::xml::XMLAttributes& attr) {
  if (mew::string img = attr[ATTR_IMAGE]) {
    return mew::str::atoi(img.str());
  } else {
    return -2;
  }
}

//==============================================================================

UINT16 avesta::XmlAttrModifiers(mew::xml::XMLAttributes& attr) {
  mew::string s = attr[ATTR_MODIFIER];
  PCWSTR m = s.str();
  UINT16 mod = 0;
  if (!m) {
    return mod;
  }
  for (; *m != L'\0'; ++m) {
    switch (*m) {
      case 'a':
      case 'A':
        mod |= mew::ui::ModifierAlt;
        break;
      case 'c':
      case 'C':
        mod |= mew::ui::ModifierControl;
        break;
      case 's':
      case 'S':
        mod |= mew::ui::ModifierShift;
        break;
      case 'w':
      case 'W':
        mod |= mew::ui::ModifierWindows;
        break;
      default:
        ASSERT(!L"INVALID_MODIFIER");
        break;
    }
  }
  return mod;
}

//==============================================================================

namespace {
static const struct NameToCLSID {
  LPCWSTR name;
  CLSID clsid;

  friend bool operator==(const NameToCLSID& lhs, const mew::string& rhs) { return lhs.name == rhs; }
  friend bool operator<(const NameToCLSID& lhs, const mew::string& rhs) { return lhs.name < rhs; }
} CLSIDs[] = {
    {L"Display", __uuidof(mew::ui::Display)},
    {L"FolderList", __uuidof(mew::ui::TabPanel)},
    {L"FolderTree", __uuidof(FolderTree)},
    {L"Form", __uuidof(mew::ui::Form)},
    {L"LinkBar", __uuidof(mew::ui::LinkBar)},
    {L"Main", __uuidof(mew::ui::DockPanel)},
    {L"MenuBar", __uuidof(mew::ui::MenuBar)},
    // { L"Panel", __uuidof(Panel) },
    {L"Preview", __uuidof(mew::ui::Preview)},
    {L"ReBar", __uuidof(mew::ui::ReBar)},
    {L"StatusBar", __uuidof(mew::ui::StatusBar)},
    {L"System", __uuidof(mew::ui::Display)}, /* alias */
    {L"ToolBar", __uuidof(mew::ui::ToolBar)},
    //{ L"TreeView", __uuidof(TreeView) },
};
}  // namespace

REFCLSID avesta::ToCLSID(const mew::string& value) {
  const NameToCLSID* begin = CLSIDs;
  const NameToCLSID* end = begin + lengthof(CLSIDs);
  const NameToCLSID* found = mew::algorithm::binary_search(begin, end, value);
  return (found == end) ? GUID_NULL : found->clsid;
}

//==============================================================================

avesta::Navigation ParseNavigate(PCWSTR s, avesta::Navigation defaultNavi) {
  if (mew::str::empty(s)) {
    return defaultNavi;
  } else if (wcsicmp(s, L"goto") == 0) {
    return avesta::NaviGoto;
  } else if (wcsicmp(s, L"goto-always") == 0) {
    return avesta::NaviGotoAlways;
  } else if (wcsicmp(s, L"open") == 0) {
    return avesta::NaviOpen;
  } else if (wcsicmp(s, L"open-always") == 0) {
    return avesta::NaviOpenAlways;
  } else if (wcsicmp(s, L"append") == 0) {
    return avesta::NaviAppend;
  } else if (wcsicmp(s, L"reserve") == 0) {
    return avesta::NaviReserve;
  } else if (wcsicmp(s, L"switch") == 0) {
    return avesta::NaviSwitch;
  } else if (wcsicmp(s, L"replace") == 0) {
    return avesta::NaviReplace;
  } else {
    return defaultNavi;
  }
}
