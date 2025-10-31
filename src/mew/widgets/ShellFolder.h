// ShellFolder.h
#pragma once

#include "WindowImpl.h"
#include "path.hpp"
#include "std/deque.hpp"

const PCTSTR SHSTR_MENU_RENAME = _T("名前の変更(&M)");
const PCTSTR SHSTR_MENU_GO = _T("移動(&G)");

namespace mew {
namespace ui {

class ShellBrowser;
using ListCtrl = WTL::CListViewCtrlT<CWindowEx>;
using HeaderCtrl = WTL::CHeaderCtrlT<CWindowEx>;

class Shell : public WindowImplBase {
 private:
  friend class ShellBrowser;
  using CContainedShell = ATL::CContainedWindowT<CWindowEx>;
  using CContainedList = ATL::CContainedWindowT<ListCtrl>;
  using CContainedHeader = ATL::CContainedWindowT<HeaderCtrl>;

  class History {
   private:
    std::deque<ref<io::IEntry> > m_history;
    size_t m_cursor;

   public:
    History();
    bool Add(io::IEntry* entry);
    void Replace(io::IEntry* entry);
    io::IEntry* GetRelativeHistory(int step, size_t* index = null);
    size_t Back(size_t step = 1);
    size_t BackLength() const;
    size_t Forward(size_t step = 1);
    size_t ForwardLength() const;
  };

 public:
  enum GoType { GoNew, GoAgain, GoReplace };

 protected:
  ref<io::IEntry> m_pCurrentEntry;
  ref<IShellFolder> m_pCurrentFolder;  // 現在表示中のフォルダ
  ref<IShellView> m_pShellView;        // 現在表示中のビュー
  ref<IStream> m_pViewStateStream;
  ref<IShellBrowser> m_pShellBrowser;
  FOLDERSETTINGS m_FolderSettings;  // フォルダ表示オプション
  CContainedShell m_wndShell;       // ビューのハンドル。ただし、リストビューはこれの子供。
  CContainedList m_wndList;         // リストビュー
  CContainedHeader m_wndHeader;     // ヘッダコントロール.
  History m_History;
  string m_PatternMask;
  bool m_CheckBoxChangeEnabled;

  bool m_AutoArrange, m_RenameExtension, m_CheckBox, m_FullRowSelect, m_GridLine, m_ShowAllFiles, m_Grouping;

  string m_WallPaperFile;
  UINT32 m_WallPaperAlign;
  Size m_WallPaperSize;

 protected:  // overrides
  virtual bool OnDirectoryChanging(io::IEntry* entry, GoType go) { return true; }
  // ディレクトリを移動した直後に呼ばれます。
  virtual void OnDirectoryChanged(io::IEntry* entry, GoType go) {}
  // デフォルトコマンドでエントリを実行しようとしている.
  virtual void OnDefaultExecute(io::IEntry* entry) { avesta::ILExecute(entry->ID, null, null, null, m_hWnd); }
  // デフォルトコマンドでディレクトリを移動しようとしている
  virtual void OnDefaultDirectoryChange(io::IEntry* folder, IShellFolder* pShellFolder);
  virtual HRESULT OnQueryStream(DWORD grfMode, IStream** ppStream) { return E_NOTIMPL; }
  virtual HRESULT OnStateChange(IShellView* pShellView, ULONG uChange) { return E_NOTIMPL; }
  virtual bool OnListWheel(WPARAM wParam, LPARAM lParam) { return false; }

  BOOL ProcessShellMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
  LRESULT DefaultContextMenu(WPARAM wParam, LPARAM lParam);

 public:
  Shell();
  ~Shell();
  void DisposeShellView();

 public:
  io::IEntry* GetCurrentEntry() const { return m_pCurrentEntry; }
  IShellFolder* GetCurrentFolder() const { return m_pCurrentFolder; }
  CWindowEx get_ShellView() const { return m_wndShell; }
  ListCtrl get_ListView() const { return m_wndList; }
  __declspec(property(get = get_ShellView)) CWindowEx ShellView;
  __declspec(property(get = get_ListView)) ListCtrl ListView;

  HRESULT GetAt(REFINTF pp, size_t index);
  HRESULT GetContents(REFINTF ppInterface, Status option);

  ListStyle get_Style() const;
  HRESULT set_Style(ListStyle style);
  bool get_CheckBox() const { return m_CheckBox; }
  HRESULT set_CheckBox(bool value);
  bool get_FullRowSelect() const { return m_FullRowSelect; }
  HRESULT set_FullRowSelect(bool value);
  bool get_GridLine() const { return m_GridLine; }
  HRESULT set_GridLine(bool value);
  bool get_Grouping() { return m_Grouping; }
  HRESULT set_Grouping(bool value);
  bool get_AutoArrange() const;
  HRESULT set_AutoArrange(bool value);
  bool get_ShowAllFiles() const { return m_ShowAllFiles; }
  HRESULT set_ShowAllFiles(bool value);
  bool get_RenameExtension() const { return m_RenameExtension; }
  HRESULT set_RenameExtension(bool value) {
    m_RenameExtension = value;
    return S_OK;
  }

  HRESULT SetStatusByIndex(int index, Status status, bool unique = false);
  HRESULT SetStatusByUnknown(IUnknown* unk, Status status, bool unique = false);
  HRESULT SetStatusByIDList(LPCITEMIDLIST leaf, Status status, bool unique = false);

  HRESULT Cut() { return MimicKeyDown('X', ModifierControl); }
  HRESULT Copy() { return MimicKeyDown('C', ModifierControl); }
  HRESULT Paste() { return MimicKeyDown('V', ModifierControl); }
  HRESULT Delete() { return MimicKeyDown(VK_DELETE, 0); }
  HRESULT Bury() { return MimicKeyDown(VK_DELETE, ModifierShift); }
  HRESULT Undo() { return MimicKeyDown('Z', ModifierControl); }
  HRESULT Select(LPCITEMIDLIST leaf, UINT svsi);
  HRESULT SelectAll();
  // HRESULT SelectAll() { return MimicKeyDown('A', ModifierControl); }
  HRESULT SelectNone();
  HRESULT SelectChecked();
  HRESULT SelectReverse();
  HRESULT SelectToFirst() { return MimicKeyDown(VK_HOME, ModifierShift); }
  HRESULT SelectToLast() { return MimicKeyDown(VK_END, ModifierShift); }
  HRESULT CheckAll(bool check = true);
  HRESULT CheckNone() { return CheckAll(false); }
  HRESULT Refresh();
  HRESULT Rename() { return MimicKeyDown(VK_F2, 0); }
  HRESULT Property() { return MimicKeyDown(VK_RETURN, ModifierAlt); }

  HRESULT GoUp(size_t step = 1, bool selectPrev = true);
  HRESULT GoBack(size_t step = 1, bool selectPrev = true);
  HRESULT GoForward(size_t step = 1, bool selectPrev = true);
  HRESULT GoAbsolute(io::IEntry* path, GoType go = GoNew);

  HRESULT EndContextMenu(IContextMenu* pMenu, UINT cmd);
  HRESULT SaveViewState();
  HRESULT UpdateShellState();

  LRESULT DefaultContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);

  HRESULT GetSortKey(int* column, bool* ascending) { return afx::ListView_GetSortKey(m_wndList, column, ascending); }
  HRESULT SetSortKey(int column, bool ascending) { return afx::ListView_SetSortKey(m_wndList, column, ascending); }

 public:  // IWallPaper
  string get_WallPaperFile() { return m_WallPaperFile; }
  UINT32 get_WallPaperAlign() { return m_WallPaperAlign; }
  void set_WallPaperFile(string value);
  void set_WallPaperAlign(UINT32 value);

 protected:
  HRESULT MimicKeyDown(UINT vkey, UINT mods);

 private:
  HRESULT ReCreateViewWindow(IShellFolder* pShellFolder, bool reload);
  HRESULT GetViewFlags(DWORD* dw);
  HRESULT IncludeObject(IShellView* pShellView, LPCITEMIDLIST pidl);
  HRESULT GetViewStateStream(DWORD grfMode, IStream** ppStream);
  bool PreTranslateMessage(MSG* msg);
  void UpdateBackground();
  HRESULT UpdateStyle();
  void OnListViewModeChanged();

  HRESULT GetContents_FolderView(REFINTF ppInterface, int svgio);
  HRESULT GetContents_ShellView(REFINTF ppInterface, int svgio);
  HRESULT GetContents_Select(REFINTF ppInterface);
  HRESULT GetContents_Checked(REFINTF ppInterface);
};

}  // namespace ui
}  // namespace mew
