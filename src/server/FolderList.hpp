// FolderList.hpp
#pragma once

#include "main.hpp"

//==============================================================================

class Main;

template <class TBase>
class __declspec(novtable) FolderListContainer : public TBase {
 private:
  mew::ref<mew::ui::IKeymap> m_keymap;
  mew::ref<mew::ui::IGestureTable> m_gesture;

 protected:
  void Dispose() throw() {
    m_gesture.dispose();
    m_keymap.dispose();
  }

 public:
  mew::ref<mew::ui::IShellListView> CurrentView() const {
    mew::ref<mew::ui::IShellListView> focus;
    if SUCCEEDED (m_tab->GetContents(&focus, mew::FOCUSED)) {
      return focus;
    } else {
      return mew::null;
    }
  }
  mew::ref<mew::io::IEntry> CurrentFolder() const {
    if (mew::ref<mew::ui::IShellListView> view = CurrentView()) {
      mew::ref<mew::io::IEntry> folder;
      if SUCCEEDED (view->GetFolder(&folder)) {
        return folder;
      }
    }
    return mew::null;
  }
  mew::string CurrentPath() const {
    if (mew::ref<mew::io::IEntry> folder = CurrentFolder()) {
      return folder->Path;
    }
    return mew::null;
  }
  int CurrentFolderIndex() const {
    mew::ref<mew::ui::IWindow> focus;
    m_tab->GetContents(&focus, mew::FOCUSED);
    size_t index;
    if SUCCEEDED (m_tab->GetStatus(focus, mew::null, &index)) {
      return (int)index;
    } else {
      return -1;
    }
  }

 public:  // IAvesta
  HRESULT GetComponent(mew::REFINTF pp, avesta::AvestaComponent type, size_t index = (size_t)-1) {
    switch (type) {
      case avesta::AvestaForm:
        return m_form.copyto(pp);
      case avesta::AvestaTab:
        return m_tab.copyto(pp);
      case avesta::AvestaFolder:
        if (index == (size_t)-1) {
          return m_tab->GetContents(pp, mew::FOCUSED);
        } else {
          return m_tab ? m_tab->GetAt(pp, index) : E_UNEXPECTED;
        }
      default:
        return E_INVALIDARG;
    }
  }
  size_t GetComponentCount(avesta::AvestaComponent type) {
    switch (type) {
      case avesta::AvestaForm:
        return m_form ? 1 : 0;
      case avesta::AvestaTab:
        return m_tab ? 1 : 0;
      case avesta::AvestaFolder:
        return m_tab ? m_tab->Count : 0;
      default:
        ASSERT(0);
        return 0;
    }
  }
  mew::ref<IEnumUnknown> EnumFolders(mew::Status status) const {
    mew::ref<IEnumUnknown> pEnum;
    m_tab->GetContents(&pEnum, status);
    return pEnum;
  }

 public:
  HRESULT ForwardToCurrent(mew::message msg) {
    if (mew::ref<mew::ui::IWindow> focus = CurrentView()) {
      SendMessageToWindow(focus, msg);
    } else {
      switch (msg.code) {
        case AVEOBS_AutoArrange:
        case AVEOBS_ShowAllFiles:
        case AVEOBS_Grouping:
          msg["state"] = 0;
          break;
        default:
          theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTAB));
          break;
      }
    }
    return S_OK;
  }
  HRESULT EnableIfAnyFolder(mew::message msg) {
    UINT32 state = 0;
    if (CurrentView()) {
      state = mew::ENABLED;
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT EnableIfCheckBoxEnabled(mew::message msg) {
    UINT32 state = 0;
    if (m_booleans[avesta::BoolCheckBox] && CurrentView()) {
      state = mew::ENABLED;
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT EnableIfAnySelection(mew::message msg) {
    UINT32 state = 0;
    if (mew::ui::IShellListView* current = CurrentView()) {
      if (current->SelectedCount > 0) {
        state = mew::ENABLED;
      }
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ForwardToAll(mew::message msg) {
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::StatusNone); i.next();) {
      SendMessageToWindow(i, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToShown(mew::message msg) {
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::SELECTED); i.next();) {
      SendMessageToWindow(i, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToHidden(mew::message msg) {
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::UNSELECTED); i.next();) {
      SendMessageToWindow(i, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToLeft(mew::message msg) {
    int index = CurrentFolderIndex();
    if (index < 0) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTAB));
      return S_OK;
    }
    for (int i = 0; i < index; ++i) {
      mew::ref<mew::ui::IWindow> w;
      if SUCCEEDED (GetComponent(&w, avesta::AvestaFolder, i)) {
        SendMessageToWindow(w, msg);
      }
    }
    return S_OK;
  }
  HRESULT ForwardToRight(mew::message msg) {
    int index = CurrentFolderIndex();
    if (index < 0) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTAB));
      return S_OK;
    }
    int count = GetComponentCount(avesta::AvestaFolder);
    for (int i = index + 1; i < count; ++i) {
      mew::ref<mew::ui::IWindow> w;
      if SUCCEEDED (GetComponent(&w, avesta::AvestaFolder, i)) {
        SendMessageToWindow(w, msg);
      }
    }
    return S_OK;
  }
  HRESULT ForwardToOthers(mew::message msg) {
    mew::ref<mew::ui::IWindow> focus = CurrentView();
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::StatusNone); i.next();) {
      if (!objcmp(focus, i)) {
        SendMessageToWindow(i, msg);
      }
    }
    return S_OK;
  }
  HRESULT ForwardToDuplicate(mew::message msg) {
    std::vector<mew::ref<mew::ui::IShellListView>> views;
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      if (mew::ref<mew::ui::IWindow> w = FindDuplicates(views, i)) {
        SendMessageToWindow(w, msg);
      }
    }
    return S_OK;
  }

 private:
  static mew::ref<mew::ui::IShellListView> FindDuplicates(std::vector<mew::ref<mew::ui::IShellListView>>& views,
                                                          mew::ui::IShellListView* viewL) {
    mew::ref<mew::io::IEntry> entryL;
    viewL->GetFolder(&entryL);
    for (size_t i = 0; i < views.size(); ++i) {
      mew::ref<mew::ui::IShellListView> viewR = views[i];
      mew::ref<mew::io::IEntry> entryR;
      viewR->GetFolder(&entryR);
      if (entryL->Equals(entryR, mew::io::IEntry::PATH)) {
        if (viewL->Visible) {
          views[i] = viewL;
          return viewR;
        } else {
          return viewL;
        }
      }
    }
    views.push_back(viewL);
    return mew::null;
  }
  void SendMessageToWindow(mew::ui::IWindow* window, mew::message msg) {
    ASSERT(window);
    switch (msg.code) {
      case mew::ui::CommandClose:
        if (!avesta::GetOption(avesta::BoolLockClose) && IsLocked(window, m_tab)) {
          return;
        }
        break;
    }
    window->Send(msg);
  }

 protected:
  HRESULT CurrentLock(mew::message msg) {
    INT32 boolean = msg.code;
    mew::ref<mew::ui::IWindow> focus = CurrentView();
    if (!focus) {
      return S_OK;
    }
    switch (boolean) {
      case BoolTrue:
        m_tab->SetStatus(focus, mew::CHECKED);
        break;
      case BoolFalse:
        m_tab->SetStatus(focus, mew::UNCHECKED);
        break;
      case BoolUnknown:
        m_tab->SetStatus(focus, mew::ToggleCheck);
        break;
      default:
        TRESPASS();
    }
    return S_OK;
  }
  HRESULT ObserveLock(mew::message msg) {
    UINT32 state = 0;
    INT32 boolean = msg.code;
    if (mew::ref<mew::ui::IWindow> focus = CurrentView()) {
      DWORD status = 0;
      m_tab->GetStatus(focus, &status);
      if (status & mew::CHECKED) {  // locked
        switch (boolean) {
          case BoolTrue:
            state = mew::CHECKED;
            break;
          case BoolFalse:
            state = mew::ENABLED;
            break;
          case BoolUnknown:
            state = mew::ENABLED | mew::CHECKED;
            break;
          default:
            TRESPASS();
        }
      } else {  // unlocked
        switch (boolean) {
          case BoolTrue:
            state = mew::ENABLED;
            break;
          case BoolFalse:
            state = mew::CHECKED;
            break;
          case BoolUnknown:
            state = mew::ENABLED;
            break;
          default:
            TRESPASS();
        }
      }
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ProcessTabShow(mew::message msg) {
    if (!m_tab) {
      return false;
    }
    UINT index = msg.code;
    if FAILED (m_tab->SetStatus(index, mew::ToggleSelect)) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_ERR_NTH_TAB, index + 1));
    }
    return S_OK;
  }
  HRESULT ObserveTabShow(mew::message msg) {
    if (!m_tab) {
      return false;
    }
    UINT32 state = 0;
    DWORD status = 0;
    if SUCCEEDED (m_tab->GetStatus((int)msg.code, &status)) {
      state = mew::ENABLED | ((status & mew::SELECTED) ? mew::CHECKED : 0);
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ProcessTabFocus(mew::message msg) {
    if (!m_tab) {
      return false;
    }
    UINT index = msg.code;
    mew::ref<mew::ui::IWindow> window;
    if (SUCCEEDED(m_tab->SetStatus(index, mew::SELECTED)) && SUCCEEDED(m_tab->GetAt(&window, index))) {
      window->Focus();
    } else {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_ERR_NTH_TAB, index + 1));
    }
    return S_OK;
  }
  HRESULT ObserveTabFocus(mew::message msg) {
    if (!m_tab) {
      return false;
    }
    UINT32 state = 0;
    if ((size_t)msg.code < GetComponentCount(avesta::AvestaFolder)) {
      state = mew::ENABLED;
    }
    msg["state"] = state;
    return S_OK;
  }

 protected:  // Keymap and Gesture
  void SpoilKeymapAndGesture(mew::ui::IWindow* from) {
    m_keymap = mew::null;
    from->GetExtension(__uuidof(mew::ui::IKeymap), &m_keymap);
    from->SetExtension(__uuidof(mew::ui::IKeymap), mew::null);
    m_gesture = mew::null;
    from->GetExtension(__uuidof(mew::ui::IGesture), &m_gesture);
    from->SetExtension(__uuidof(mew::ui::IGesture), mew::null);
  }

 public:  // IKeymap
  HRESULT OnKeyDown(mew::ui::IWindow* window, UINT16 modifiers, UINT8 vkey) {
    if (!m_keymap) {
      return E_FAIL;
    }
    return m_keymap->OnKeyDown(window, modifiers, vkey);
  }

 public:  // IGesture
  HRESULT OnGestureAccept(HWND hWnd, mew::Point ptScreen, size_t length, const mew::ui::Gesture gesture[]) {
    return m_gesture ? m_gesture->OnGestureAccept(hWnd, ptScreen, length, gesture) : E_FAIL;
  }
  HRESULT OnGestureUpdate(UINT16 modifiers, size_t length, const mew::ui::Gesture gesture[]) {
    if (!m_gesture) {
      return E_FAIL;
    }
    HRESULT hr;
    if FAILED (hr = m_gesture->OnGestureUpdate(modifiers, length, gesture)) {
      return hr;
    }
    if (length == 0 || !m_status) {
      return S_OK;
    }

    mew::string description;
    mew::ref<mew::ICommand> command;
    if SUCCEEDED (m_gesture->GetGesture(modifiers, length, gesture, &command)) {
      description = command->Description;
    }

    mew::string text;
    if (m_callback) {
      text = m_callback->GestureText(gesture, length, description);
    }
    SetStatusText(avesta::NotifyResult, text);
    return S_OK;
  }
  HRESULT OnGestureFinish(UINT16 modifiers, size_t length, const mew::ui::Gesture gesture[]) {
    if (!m_gesture) {
      return E_FAIL;
    }
    SetStatusText(avesta::NotifyResult, mew::null);
    if (mew::ref<mew::ui::IShellListView> current = CurrentView()) {
      SetStatusText(avesta::NotifyInfo, FormatViewStatusText(current, current->GetLastStatusText()));
      return m_gesture->OnGestureFinish(modifiers, length, gesture);
    } else {
      return E_FAIL;
    }
  }
};
