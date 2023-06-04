// AvestaSDK.hpp

#pragma once

namespace avesta {
/// .
enum AvestaComponent {
  AvestaForm,
  AvestaTab,
  AvestaFolder,
  AvestaStatus,
  AvestaPreview,
};

/// .
enum NotifyPriority {
  NotifyIdle = 0,      //
  NotifyInfo = 10,     // システムからの情報
  NotifyResult = 20,   // ユーザの操作の結果
  NotifyWarning = 30,  // ユーザの誤操作
  NotifyError = 40,    // システムの動作不良
};

/// .
enum Navigation {
  NaviCancel,      ///< 移動を取り消す
  NaviGoto,        ///< 移動＋キー
  NaviGotoAlways,  ///< 移動
  NaviOpen,        ///< 開く＋キー
  NaviOpenAlways,  ///< 開く
  NaviAppend,      ///< 追加
  NaviReserve,     ///< 追加＋隠す
  NaviSwitch,      ///< 追加＋フォーカス
  NaviReplace,     ///< 追加＋フォーカス＋元を隠す
};

///.
enum Fonts {
  FontTab,
  FontAddress,
  FontList,
  FontStatus,
  NumOfFonts,
};

/// .
__interface IAvesta {
  /// コンポーネント数を取得する.
  size_t GetComponentCount(AvestaComponent type);
  /// コンポーネントを取得する.
  HRESULT GetComponent(mew::REFINTF pp, AvestaComponent type, size_t index = (size_t)-1);
  /// IShellListView を列挙する.
  mew::ref<IEnumUnknown> EnumFolders(mew::Status status) const;

  mew::ref<mew::ui::IShellListView> CurrentView() const;
  mew::ref<mew::io::IEntry> CurrentFolder() const;
  mew::string CurrentPath() const;

  /// フォルダとして開きます.
  mew::ref<mew::ui::IShellListView> OpenFolder(mew::io::IEntry * entry, Navigation navi, Navigation* naviResult = nullptr);
  /// フォルダとして開くか、実行します.
  HRESULT OpenOrExecute(mew::io::IEntry * entry);

  /// 通知をユーザの邪魔にならないように表示する.
  void Notify(DWORD priority, mew::string msg);
  ///
  void ParseCommandLine(PCWSTR args);
  ///
  void Restart(PCWSTR newDLL = nullptr);
};

/// 関連付け実行します.
HRESULT AvestaExecute(mew::io::IEntry* entry);
}  // namespace avesta

namespace ave {
MEW_API void GetDriveLetter(PCWSTR path, PWSTR buffer);
MEW_API UINT64 GetTotalBytes(mew::ui::IShellListView* view);
MEW_API UINT64 GetSelectedBytes(mew::ui::IShellListView* view);

inline mew::string ResolvePath(PCWSTR path) {
  const int CSIDL_AVESTA = 0xFFFFFFFF;
  return mew::io::PathResolvePath(path, L"AVESTA", CSIDL_AVESTA);
}
inline mew::string ResolvePath(const mew::string& path) { return ResolvePath(path.str()); }
}  // namespace ave

//==============================================================================

void NewFolder(mew::ui::IShellListView* view);

void DlgNew(mew::ui::IShellListView* view);
void DlgSelect(mew::ui::IShellListView* view);
void DlgPattern(mew::ui::IShellListView* view);
void DlgRename(mew::ui::IShellListView* view, bool paste);

// void DlgOpen(IShellListView* view);
