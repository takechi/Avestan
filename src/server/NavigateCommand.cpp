// NavigateCommand.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

namespace {
class NavigateCommand : public mew::Root<mew::implements<mew::ICommand> > {
 private:
  mew::string m_path;
  avesta::Navigation m_type;

 public:
  NavigateCommand(mew::string path, avesta::Navigation type) : m_path(path), m_type(type) {}
  mew::string get_Description() { return m_path; }
  UINT32 QueryState(IUnknown* owner) { return mew::ENABLED; }
  void Invoke() {
    try {
      mew::ref<mew::io::IEntry> entry(__uuidof(mew::io::Entry), m_path);
      switch (m_type) {
        case avesta::NaviOpen:
          Open(entry);
          break;
        case avesta::NaviOpenAlways:
          OpenAlways(entry);
          break;
        case avesta::NaviAppend:
          OpenAppend(entry);
          break;
        case avesta::NaviReserve:
          OpenReserve(entry);
          break;
        case avesta::NaviGoto:
          Goto(entry);
          break;
        case avesta::NaviGotoAlways:
          GotoAlways(entry);
          break;
        case avesta::NaviSwitch:
        case avesta::NaviReplace:
        default:
          Open(entry);
          break;
      }
    } catch (mew::exceptions::Error& e) {
      TRACE(e.Message);
      theAvesta->Notify(avesta::NotifyError, e.Message);
    }
  }

 private:
  /// 開く（修飾キーは自動）.
  static void Open(mew::io::IEntry* entry) { theAvesta->OpenFolder(entry, avesta::NaviOpen); }
  /// 開き、それのみを選択する.
  static void OpenAlways(mew::io::IEntry* entry) { theAvesta->OpenFolder(entry, avesta::NaviOpenAlways); }
  /// 追加で開く.
  static void OpenAppend(mew::io::IEntry* entry) { theAvesta->OpenFolder(entry, avesta::NaviAppend); }
  /// 非表示で開く.
  static void OpenReserve(mew::io::IEntry* entry) { theAvesta->OpenFolder(entry, avesta::NaviReserve); }
  /// 移動する（修飾キーは自動）.
  static void Goto(mew::io::IEntry* entry) {
    ASSERT(entry);
    mew::ref<mew::ui::IShellListView> current;
    if SUCCEEDED (theAvesta->GetComponent(&current, avesta::AvestaFolder)) {
      current->Go(entry);
    } else {
      theAvesta->OpenFolder(entry, avesta::NaviOpen);
    }
  }
  /// 常に移動する.
  static void GotoAlways(mew::io::IEntry* entry) {
    // TODO: これバグ！
    afx::SetModifierState(0, 0);
    Goto(entry);
    afx::RestoreModifierState(0);
  }
};
}  // namespace

mew::ref<mew::ICommand> CreateNavigateCommand(mew::string path, mew::string args) {
  if (!path) {
    return mew::null;
  }
  avesta::Navigation navi = ParseNavigate(args.str(), avesta::NaviOpen);
  return mew::objnew<NavigateCommand>(path, navi);
}
