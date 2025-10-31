// MenuProvider.hpp
#pragma once

#include "widgets.hpp"

//==============================================================================

namespace mew {
namespace ui {

class MenuProvider {
 private:
  ref<IImageList> m_pImageList;

 public:
  static ref<ICommand> PopupMenu(ITreeItem* menu, HWND hWnd, UINT tpm, int x, int y, const RECT* rcExclude = null);
  static HMENU ConstructMenu(ITreeItem* menu, HMENU hMenu = null, int index = -1);
  static void DisposeMenu(HMENU hMenu, bool destroy = true);
  static ref<ITreeItem> FindByCommand(HMENU hMenu, UINT wID);

  BOOL HandleMenuMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

  IImageList* GetMenuImageList() const { return m_pImageList; }
  void SetMenuImageList(IImageList* image) { m_pImageList = image; }
};

}  // namespace ui
}  // namespace mew
