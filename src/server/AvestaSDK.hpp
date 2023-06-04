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
  NotifyInfo = 10,     // �V�X�e������̏��
  NotifyResult = 20,   // ���[�U�̑���̌���
  NotifyWarning = 30,  // ���[�U�̌둀��
  NotifyError = 40,    // �V�X�e���̓���s��
};

/// .
enum Navigation {
  NaviCancel,      ///< �ړ���������
  NaviGoto,        ///< �ړ��{�L�[
  NaviGotoAlways,  ///< �ړ�
  NaviOpen,        ///< �J���{�L�[
  NaviOpenAlways,  ///< �J��
  NaviAppend,      ///< �ǉ�
  NaviReserve,     ///< �ǉ��{�B��
  NaviSwitch,      ///< �ǉ��{�t�H�[�J�X
  NaviReplace,     ///< �ǉ��{�t�H�[�J�X�{�����B��
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
  /// �R���|�[�l���g�����擾����.
  size_t GetComponentCount(AvestaComponent type);
  /// �R���|�[�l���g���擾����.
  HRESULT GetComponent(mew::REFINTF pp, AvestaComponent type, size_t index = (size_t)-1);
  /// IShellListView ��񋓂���.
  mew::ref<IEnumUnknown> EnumFolders(mew::Status status) const;

  mew::ref<mew::ui::IShellListView> CurrentView() const;
  mew::ref<mew::io::IEntry> CurrentFolder() const;
  mew::string CurrentPath() const;

  /// �t�H���_�Ƃ��ĊJ���܂�.
  mew::ref<mew::ui::IShellListView> OpenFolder(mew::io::IEntry * entry, Navigation navi, Navigation* naviResult = nullptr);
  /// �t�H���_�Ƃ��ĊJ�����A���s���܂�.
  HRESULT OpenOrExecute(mew::io::IEntry * entry);

  /// �ʒm�����[�U�̎ז��ɂȂ�Ȃ��悤�ɕ\������.
  void Notify(DWORD priority, mew::string msg);
  ///
  void ParseCommandLine(PCWSTR args);
  ///
  void Restart(PCWSTR newDLL = nullptr);
};

/// �֘A�t�����s���܂�.
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
