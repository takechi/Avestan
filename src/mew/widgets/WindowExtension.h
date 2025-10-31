// WindowExtension.h
#pragma once

namespace mew {
namespace ui {

class Extension {
 public:  // 今のところ数が少ないので、マップなどを使わずにそのまま
  ref<IKeymap> m_keymap;
  ref<IGesture> m_gesture;
  ref<IDropTarget> m_drop;

 private:
  template <class T>
  static bool DoGet(HRESULT& hr, REFGUID which, REFINTF what, const ref<T>& p) {
    if (which != __uuidof(T)) return false;
    hr = p.copyto(what);
    return true;
  }
  template <class T>
  static bool DoSet(HRESULT& hr, REFGUID which, IUnknown* what, ref<T>& p) {
    if (which != __uuidof(T)) return false;
    if (!what) {
      p.clear();
      hr = S_OK;
    } else {
      ref<T> tmp;
      hr = what->QueryInterface(__uuidof(T), (void**)&tmp);
      p = tmp;
    }
    return true;
  }

 public:
  Extension() {}
  void Dispose() {
    m_keymap.clear();
    m_gesture.clear();
    m_drop.clear();
  }
  HRESULT Get(REFGUID which, REFINTF what) const {
    HRESULT hr = E_NOTIMPL;
    DoGet(hr, which, what, m_keymap) || DoGet(hr, which, what, m_gesture) || DoGet(hr, which, what, m_drop);
    return hr;
  }
  HRESULT Set(REFGUID which, IUnknown* what) {
    HRESULT hr = E_NOTIMPL;
    DoSet(hr, which, what, m_keymap) || DoSet(hr, which, what, m_gesture) || DoSet(hr, which, what, m_drop);
    return hr;
  }
  bool ProcessKeymap(IWindow* self, MSG* msg) {
    if (m_keymap) {
      switch (msg->message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
          return SUCCEEDED(m_keymap->OnKeyDown(self, GetCurrentModifiers(), (UINT8)msg->wParam));
        default:
          break;
      }
    }
    return false;
  }
  bool ProcessQueryGesture(IGesture** pp) { return SUCCEEDED(m_gesture.copyto(pp)); }
  bool ProcessQueryDrop(IDropTarget** pp) { return SUCCEEDED(m_drop.copyto(pp)); }
};

}  // namespace ui
}  // namespace mew
