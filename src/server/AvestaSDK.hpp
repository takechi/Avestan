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
  HRESULT GetComponent(REFINTF pp, AvestaComponent type, size_t index = (size_t)-1);
  /// IShellListView を列挙する.
  ref<IEnumUnknown> EnumFolders(Status status) const;

  ref<IShellListView> CurrentView() const;
  ref<IEntry> CurrentFolder() const;
  string CurrentPath() const;

  /// フォルダとして開きます.
  ref<IShellListView> OpenFolder(IEntry * entry, Navigation navi, Navigation* naviResult = null);
  /// フォルダとして開くか、実行します.
  HRESULT OpenOrExecute(IEntry * entry);

  /// 通知をユーザの邪魔にならないように表示する.
  void Notify(DWORD priority, string msg);
  ///
  void ParseCommandLine(PCWSTR args);
  ///
  void Restart(PCWSTR newDLL = null);
};

/// 関連付け実行します.
HRESULT AvestaExecute(IEntry* entry);
}  // namespace avesta

namespace ave {
MEW_API void GetDriveLetter(PCWSTR path, PWSTR buffer);
MEW_API UINT64 GetTotalBytes(IShellListView* view);
MEW_API UINT64 GetSelectedBytes(IShellListView* view);

inline string ResolvePath(PCWSTR path) {
  const int CSIDL_AVESTA = 0xFFFFFFFF;
  return io::PathResolvePath(path, L"AVESTA", CSIDL_AVESTA);
}
inline string ResolvePath(const string& path) { return ResolvePath(path.str()); }
}  // namespace ave

//==============================================================================

void NewFolder(IShellListView* view);

void DlgNew(IShellListView* view);
void DlgSelect(IShellListView* view);
void DlgPattern(IShellListView* view);
void DlgRename(IShellListView* view, bool paste);

// void DlgOpen(IShellListView* view);
