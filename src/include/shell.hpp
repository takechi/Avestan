/// @file shell.hpp
/// シェルとファイルシステム.
#pragma once

#include <shellapi.h>
#include <shlobj.h>

#include "mew.hpp"
#include "path.hpp"
#include "widgets.hpp"

namespace mew {
/// シェル、ファイルシステム、プロセス間通信.
namespace io {
//==============================================================================
// インタフェース定義.

/// シェルエントリ配列(CIDA).
__interface IEntryList : IUnknown {
#ifndef DOXYGEN
  size_t get_Count();
  LPCITEMIDLIST get_Parent();
  LPCITEMIDLIST get_Leaf(size_t index);
#endif  // DOXYGEN

  __declspec(property(get = get_Count)) size_t Count;
  __declspec(property(get = get_Parent)) LPCITEMIDLIST Parent;
  __declspec(property(get = get_Leaf)) LPCITEMIDLIST Leaf[];

  const CIDA* GetCIDA();
  HRESULT GetAt(IEntry * *ppShellItem, size_t index);

  /// このエントリ配列のうち、指定のエントリだけを抽出する.
  HRESULT CloneSubset(REFINTF pp,        ///< 抽出された配列.
                      size_t subsets[],  ///< 抽出するエントリインデックス配列.
                      size_t length      ///< subsetsの長さ.
  );
};

/// シェルフォルダメニュー.
__interface IFolder : ui::ITreeItem {
#ifndef DOXYGEN
  IEntry* get_Entry();
  bool get_IncludeFiles();
  void set_IncludeFiles(bool value);
  int get_Depth();
  void set_Depth(int depth);
#endif
  __declspec(property(get = get_Entry)) IEntry* Entry;                                     ///< エントリ.
  __declspec(property(get = get_IncludeFiles, put = set_IncludeFiles)) bool IncludeFiles;  ///< ファイルを含むか否か.
  __declspec(property(get = get_Depth, put = set_Depth)) int Depth;                        ///< 展開する深さ.

  void Reset();
};

/// ドロップエフェクト.
enum DropEffect {
  DropEffectNone = 0x00000000,
  DropEffectCopy = 0x00000001,
  DropEffectMove = 0x00000002,
  DropEffectLink = 0x00000004,
  // DropEffectScroll= 0x80000000
};

/// ドラッグ＆ドロップソース.
__interface IDragSource : IDataObject {
  DropEffect DoDragDrop(DWORD dwSupportsEffect);
  DropEffect DoDragDrop(DWORD dwSupportsEffect, HWND hWndDrag, POINT ptCursor);
  DropEffect DoDragDrop(DWORD dwSupportsEffect, HBITMAP hBitmap, COLORREF colorkey, POINT ptHotspot);
  /// データを追加する.
  HRESULT AddData(CLIPFORMAT cfFormat, const void* data, size_t size);
  /// グローバルハンドルを追加する.
  HRESULT AddGlobalData(CLIPFORMAT cfFormat, HGLOBAL hGlobal);
  /// ひとつのIDListを追加する.
  HRESULT AddIDList(LPCITEMIDLIST pIDList);
  /// IDList配列を追加する.
  HRESULT AddIDList(const CIDA* pCIDA);
  /// テキストを追加する.
  HRESULT AddText(string text);
  /// URLを追加する.
  HRESULT AddURL(string url);
};

//======================================================================
// Version

class Version {
 private:
  string m_Path;
  BYTE* m_VersionInfo;
  WORD m_Language;
  WORD m_CodePage;

 public:
  Version();
  ~Version();
  string GetPath() const;
  bool Open(string filename);
  void Close();
  PCWSTR QueryValue(PCWSTR what);
};
}  // namespace io
}  // namespace mew
