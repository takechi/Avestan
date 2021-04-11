// FolderList.hpp
#pragma once

#include "main.hpp"

//==============================================================================

class Main;

template <class TBase>
class __declspec(novtable) FolderListContainer : public TBase {
 private:
  ref<IKeymap> m_keymap;
  ref<IGestureTable> m_gesture;

 protected:
  void Dispose() throw() {
    m_gesture.dispose();
    m_keymap.dispose();
  }

 public:
  ref<IShellListView> CurrentView() const {
    ref<IShellListView> focus;
    if SUCCEEDED (m_tab->GetContents(&focus, FOCUSED))
      return focus;
    else
      return null;
  }
  ref<IEntry> CurrentFolder() const {
    if (ref<IShellListView> view = CurrentView()) {
      ref<IEntry> folder;
      if SUCCEEDED (view->GetFolder(&folder)) return folder;
    }
    return null;
  }
  string CurrentPath() const {
    if (ref<IEntry> folder = CurrentFolder()) return folder->Path;
    return null;
  }
  int CurrentFolderIndex() const {
    ref<IWindow> focus;
    m_tab->GetContents(&focus, FOCUSED);
    size_t index;
    if SUCCEEDED (m_tab->GetStatus(focus, null, &index))
      return (int)index;
    else
      return -1;
  }

 public:  // IAvesta
  HRESULT GetComponent(REFINTF pp, AvestaComponent type, size_t index = (size_t)-1) {
    switch (type) {
      case AvestaForm:
        return m_form.copyto(pp);
      case AvestaTab:
        return m_tab.copyto(pp);
      case AvestaFolder:
        if (index == (size_t)-1)
          return m_tab->GetContents(pp, FOCUSED);
        else
          return m_tab ? m_tab->GetAt(pp, index) : E_UNEXPECTED;
      default:
        return E_INVALIDARG;
    }
  }
  size_t GetComponentCount(AvestaComponent type) {
    switch (type) {
      case AvestaForm:
        return m_form ? 1 : 0;
      case AvestaTab:
        return m_tab ? 1 : 0;
      case AvestaFolder:
        return m_tab ? m_tab->Count : 0;
      default:
        ASSERT(0);
        return 0;
    }
  }
  ref<IEnumUnknown> EnumFolders(Status status) const {
    ref<IEnumUnknown> pEnum;
    m_tab->GetContents(&pEnum, status);
    return pEnum;
  }

 public:
  HRESULT ForwardToCurrent(message msg) {
    if (ref<IWindow> focus = CurrentView())
      SendMessageToWindow(focus, msg);
    else {
      switch (msg.code) {
        case AVEOBS_AutoArrange:
        case AVEOBS_ShowAllFiles:
        case AVEOBS_Grouping:
          msg["state"] = 0;
          break;
        default:
          theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOTAB));
          break;
      }
    }
    return S_OK;
  }
  HRESULT EnableIfAnyFolder(message msg) {
    UINT32 state = 0;
    if (CurrentView()) state = ENABLED;
    msg["state"] = state;
    return S_OK;
  }
  HRESULT EnableIfCheckBoxEnabled(message msg) {
    UINT32 state = 0;
    if (m_booleans[BoolCheckBox] && CurrentView()) state = ENABLED;
    msg["state"] = state;
    return S_OK;
  }
  HRESULT EnableIfAnySelection(message msg) {
    UINT32 state = 0;
    if (IShellListView* current = CurrentView())
      if (current->SelectedCount > 0) state = ENABLED;
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ForwardToAll(message msg) {
    for (each<IWindow> i = EnumFolders(StatusNone); i.next();) SendMessageToWindow(i, msg);
    return S_OK;
  }
  HRESULT ForwardToShown(message msg) {
    for (each<IWindow> i = EnumFolders(SELECTED); i.next();) SendMessageToWindow(i, msg);
    return S_OK;
  }
  HRESULT ForwardToHidden(message msg) {
    for (each<IWindow> i = EnumFolders(UNSELECTED); i.next();) SendMessageToWindow(i, msg);
    return S_OK;
  }
  HRESULT ForwardToLeft(message msg) {
    int index = CurrentFolderIndex();
    if (index < 0) {
      theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOTAB));
      return S_OK;
    }
    for (int i = 0; i < index; ++i) {
      ref<IWindow> w;
      if SUCCEEDED (GetComponent(&w, AvestaFolder, i)) SendMessageToWindow(w, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToRight(message msg) {
    int index = CurrentFolderIndex();
    if (index < 0) {
      theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOTAB));
      return S_OK;
    }
    int count = GetComponentCount(AvestaFolder);
    for (int i = index + 1; i < count; ++i) {
      ref<IWindow> w;
      if SUCCEEDED (GetComponent(&w, AvestaFolder, i)) SendMessageToWindow(w, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToOthers(message msg) {
    ref<IWindow> focus = CurrentView();
    for (each<IWindow> i = EnumFolders(StatusNone); i.next();) {
      if (!objcmp(focus, i)) SendMessageToWindow(i, msg);
    }
    return S_OK;
  }
  HRESULT ForwardToDuplicate(message msg) {
    std::vector<ref<IShellListView> > views;
    for (each<IShellListView> i = EnumFolders(StatusNone); i.next();) {
      if (ref<IWindow> w = FindDuplicates(views, i)) SendMessageToWindow(w, msg);
    }
    return S_OK;
  }

 private:
  static ref<IShellListView> FindDuplicates(std::vector<ref<IShellListView> >& views, IShellListView* viewL) {
    ref<IEntry> entryL;
    viewL->GetFolder(&entryL);
    for (size_t i = 0; i < views.size(); ++i) {
      ref<IShellListView> viewR = views[i];
      ref<IEntry> entryR;
      viewR->GetFolder(&entryR);
      if (entryL->Equals(entryR, IEntry::PATH)) {
        if (viewL->Visible) {
          views[i] = viewL;
          return viewR;
        } else
          return viewL;
      }
    }
    views.push_back(viewL);
    return null;
  }
  void SendMessageToWindow(IWindow* window, message msg) {
    ASSERT(window);
    switch (msg.code) {
      case CommandClose:
        if (!avesta::GetOption(BoolLockClose) && IsLocked(window, m_tab)) return;
        break;
    }
    window->Send(msg);
  }

 protected:
  HRESULT CurrentLock(message msg) {
    INT32 boolean = msg.code;
    ref<IWindow> focus = CurrentView();
    if (!focus) return S_OK;
    switch (boolean) {
      case BoolTrue:
        m_tab->SetStatus(focus, CHECKED);
        break;
      case BoolFalse:
        m_tab->SetStatus(focus, UNCHECKED);
        break;
      case BoolUnknown:
        m_tab->SetStatus(focus, ToggleCheck);
        break;
      default:
        TRESPASS();
    }
    return S_OK;
  }
  HRESULT ObserveLock(message msg) {
    UINT32 state = 0;
    INT32 boolean = msg.code;
    if (ref<IWindow> focus = CurrentView()) {
      DWORD status = 0;
      m_tab->GetStatus(focus, &status);
      if (status & CHECKED) {  // locked
        switch (boolean) {
          case BoolTrue:
            state = CHECKED;
            break;
          case BoolFalse:
            state = ENABLED;
            break;
          case BoolUnknown:
            state = ENABLED | CHECKED;
            break;
          default:
            TRESPASS();
        }
      } else {  // unlocked
        switch (boolean) {
          case BoolTrue:
            state = ENABLED;
            break;
          case BoolFalse:
            state = CHECKED;
            break;
          case BoolUnknown:
            state = ENABLED;
            break;
          default:
            TRESPASS();
        }
      }
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ProcessTabShow(message msg) {
    if (!m_tab) return false;
    UINT index = msg.code;
    if FAILED (m_tab->SetStatus(index, ToggleSelect))
      theAvesta->Notify(NotifyWarning, string::load(IDS_ERR_NTH_TAB, index + 1));
    return S_OK;
  }
  HRESULT ObserveTabShow(message msg) {
    if (!m_tab) return false;
    UINT32 state = 0;
    DWORD status = 0;
    if SUCCEEDED (m_tab->GetStatus((int)msg.code, &status)) {
      state = ENABLED | ((status & SELECTED) ? CHECKED : 0);
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ProcessTabFocus(message msg) {
    if (!m_tab) return false;
    UINT index = msg.code;
    ref<IWindow> window;
    if (SUCCEEDED(m_tab->SetStatus(index, SELECTED)) && SUCCEEDED(m_tab->GetAt(&window, index))) {
      window->Focus();
    } else {
      theAvesta->Notify(NotifyWarning, string::load(IDS_ERR_NTH_TAB, index + 1));
    }
    return S_OK;
  }
  HRESULT ObserveTabFocus(message msg) {
    if (!m_tab) return false;
    UINT32 state = 0;
    if ((size_t)msg.code < GetComponentCount(AvestaFolder)) state = ENABLED;
    msg["state"] = state;
    return S_OK;
  }

 protected:  // Keymap and Gesture
  void SpoilKeymapAndGesture(IWindow* from) {
    m_keymap = null;
    from->GetExtension(__uuidof(IKeymap), &m_keymap);
    from->SetExtension(__uuidof(IKeymap), null);
    m_gesture = null;
    from->GetExtension(__uuidof(IGesture), &m_gesture);
    from->SetExtension(__uuidof(IGesture), null);
  }

 public:  // IKeymap
  HRESULT OnKeyDown(IWindow* window, UINT16 modifiers, UINT8 vkey) {
    if (!m_keymap) return E_FAIL;
    return m_keymap->OnKeyDown(window, modifiers, vkey);
  }

 public:  // IGesture
  HRESULT OnGestureAccept(HWND hWnd, Point ptScreen, size_t length, const Gesture gesture[]) {
    return m_gesture ? m_gesture->OnGestureAccept(hWnd, ptScreen, length, gesture) : E_FAIL;
  }
  HRESULT OnGestureUpdate(UINT16 modifiers, size_t length, const Gesture gesture[]) {
    if (!m_gesture) return E_FAIL;
    HRESULT hr;
    if FAILED (hr = m_gesture->OnGestureUpdate(modifiers, length, gesture)) return hr;
    if (length == 0 || !m_status) return S_OK;

    string description;
    ref<ICommand> command;
    if SUCCEEDED (m_gesture->GetGesture(modifiers, length, gesture, &command)) description = command->Description;

    string text;
    if (m_callback) text = m_callback->GestureText(gesture, length, description);
    SetStatusText(NotifyResult, text);
    return S_OK;
  }
  HRESULT OnGestureFinish(UINT16 modifiers, size_t length, const Gesture gesture[]) {
    if (!m_gesture) return E_FAIL;
    SetStatusText(NotifyResult, null);
    if (ref<IShellListView> current = CurrentView()) {
      SetStatusText(NotifyInfo, FormatViewStatusText(current, current->GetLastStatusText()));
      return m_gesture->OnGestureFinish(modifiers, length, gesture);
    } else {
      return E_FAIL;
    }
  }
};
