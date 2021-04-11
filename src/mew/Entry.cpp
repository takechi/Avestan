// Entry.cpp

#include "stdafx.h"
#include <shobjidl.h>
#include "private.h"
#include "io.hpp"
#include "std/map.hpp"
#include "shell.hpp"

using namespace mew::io;

namespace {
//==============================================================================

const int NOIMAGE = -0xFFFF;

//==============================================================================

struct EntryCompare {
  EntryCompare(IShellFolder* p) : parent(p) {}
  ref<IShellFolder> parent;
  bool operator()(IEntry* lhs, IEntry* rhs) const {
    bool isFolderL = lhs->IsFolder();
    bool isFolderR = rhs->IsFolder();
    if (isFolderL && !isFolderR) return true;
    if (!isFolderL && isFolderR) return false;
    HRESULT hr = parent->CompareIDs(0, ILFindLastID(lhs->ID), ILFindLastID(rhs->ID));
    if ((short)HRESULT_CODE(hr) < 0) { /* lhs <  rhs */
      return true;
    } else if ((short)HRESULT_CODE(hr) > 0) { /* lhs >  rhs */
      return false;
    } else { /* lhs == rhs */
      return false;
    }
  }
};

struct IDListCompare {
  bool operator()(LPCITEMIDLIST lhs, const LPCITEMIDLIST rhs) const { return afx::ILCompare(lhs, rhs) < 0; }
};

static string ExtractLeaf(const string& path) {
  if (!path) return null;
  PCWSTR leaf = io::PathFindLeaf(path);
  return string(leaf);
}
static string ExtractBase(const string& path) {
  if (!path) return null;
  string leaf = io::PathFindLeaf(path);
  if (PathIsDirectory(path.str())) return leaf;
  TCHAR buf[MAX_PATH];
  leaf.copyto(buf);
  ::PathRemoveExtension(buf);
  return string(buf);
}
static string ExtractExtension(const string& path) {
  if (!path) return null;
  if (PathIsDirectory(path.str())) return null;
  PCWSTR ext = PathFindExtension(path.str());
  if (str::empty(ext)) return null;
  return string(ext);
}
static string ExtractURL(const string& path) {
  if (!path) return null;
  WCHAR url[MAX_PATH];
  DWORD len = MAX_PATH;
  UrlCreateFromPath(path.str(), url, &len, 0);
  return string(url);
}
static string ExtractID(LPCITEMIDLIST pidl) {
  TRESPASS();
  return null;
}
}  // namespace

//==============================================================================

namespace mew {
namespace io {

class Entry : public Root<implements<IEntry, ISerializable> > {
 private:
  LPITEMIDLIST m_pidl;
  string m_name;
  string m_path;
  int m_image;
  ref<IShellFolder> m_folder;
  DWORD m_attrs;
  bool m_isAttrValid;

 private:
  HRESULT ResolveLink(REFINTF ppObject);
  void UpdateFileInfo() {
    UpdateAttribute();
    UINT flags = 0;
    if (m_image == NOIMAGE) flags |= SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    if (!m_name) flags |= SHGFI_DISPLAYNAME;
    if (flags == 0) return;
    DWORD dwFileAttr;
    if (m_attrs & SFGAO_FOLDER)
      dwFileAttr = FILE_ATTRIBUTE_DIRECTORY;
    else
      dwFileAttr = FILE_ATTRIBUTE_NORMAL;
    SHFILEINFO info;
    afx::ILGetFileInfo(get_ID(), &info, flags | SHGFI_USEFILEATTRIBUTES, dwFileAttr);
    if (m_image == NOIMAGE) m_image = info.iIcon;
    if (!m_name) m_name = info.szDisplayName;
  }
  void UpdateAttribute(IShellFolder* pParentHint = null, LPCITEMIDLIST pLeafHint = null) {
    if (m_isAttrValid) return;
    LPCITEMIDLIST pidl = get_ID();
    if (afx::ILIsRoot(pidl)) {
      m_attrs = SFGAO_FOLDER;
    } else {
      ref<IShellFolder> parent(pParentHint);
      LPCITEMIDLIST leaf(pLeafHint);
      if (!parent) afx::ILGetParentFolder(pidl, &parent, &leaf);
      if (!parent) return;
      m_attrs = SFGAO_FOLDER;
      parent->GetAttributesOf(1, &leaf, &m_attrs);
      m_isAttrValid = true;
    }
  }
  void UpdateAttribute(DWORD dwAttr) { m_attrs = dwAttr; }
  void UpdateFolder(IShellFolder* pParentHint = null, LPCITEMIDLIST pLeafHint = null) {
    if (m_isAttrValid) {
      if (!(m_attrs & SFGAO_FOLDER) || m_folder) return;
    }
    LPCITEMIDLIST pidl = get_ID();
    if (afx::ILIsRoot(pidl)) {
      m_attrs = SFGAO_FOLDER;
      m_isAttrValid = true;
      SHGetDesktopFolder(&m_folder);
    } else {
      ref<IShellFolder> parent(pParentHint);
      LPCITEMIDLIST leaf(pLeafHint);
      if (!parent) afx::ILGetParentFolder(pidl, &parent, &leaf);
      if (!parent) return;
      if (!m_isAttrValid) {
        m_attrs = SFGAO_FOLDER;
        parent->GetAttributesOf(1, &leaf, &m_attrs);
        m_isAttrValid = true;
      }
      // TRACE(_T("UpdateFolder(m_attrs=0x%08X, SFGAO_FOLDER=0x%08X, isFolder=%d", m_attrs, SFGAO_FOLDER, (m_attrs & SFGAO_FOLDER) != 0);
      // SHDESCRIPTIONID.dwDescriptionId よりもロバストな判定ができる
      if (m_attrs & SFGAO_FOLDER) {
        HRESULT hr;
        hr = parent->BindToObject(leaf, NULL, IID_IShellFolder, (void**)&m_folder);
        if (!m_folder) {
          TRACE(_T("error: SFGAO_FOLDER なのにフォルダが取得できない, (code=$1)"), (DWORD)hr);
        }
      }
    }
  }

 private:
  Entry(LPITEMIDLIST pidl, IShellFolder* parent, LPCITEMIDLIST leaf) : m_pidl(pidl), m_attrs(0), m_image(NOIMAGE), m_isAttrValid(false) {
    // SHBindToParent()はコストがかかるので、親がわかっているうちに情報を得ておく
    if (parent && leaf) UpdateAttribute(parent, leaf);
  }
  Entry(LPITEMIDLIST pidl, DWORD attrs) : m_pidl(pidl), m_attrs(attrs), m_image(NOIMAGE), m_isAttrValid(true) {}

  static CriticalSection csEntryPool;
  using Pool = std::map<LPCITEMIDLIST, Entry*, IDListCompare>;
  static Pool& GetPool() {
    static Pool pool;
    return pool;
  }
  static void RemoveFromPool(LPCITEMIDLIST pidl) {
    AutoLock lock(csEntryPool);
    Pool& pool = GetPool();
    pool.erase(pidl);
  }
  static void ReplacePool(LPCITEMIDLIST prev, LPCITEMIDLIST next, Entry* entry) {
    AutoLock lock(csEntryPool);
    Pool& pool = GetPool();
    pool.erase(prev);
    pool[next] = entry;
  }
  HRESULT GetFolder(REFINTF pp) {
    UpdateFolder();
    if (m_folder)
      return m_folder->QueryInterface(pp);
    else
      return E_FAIL;
  }
  HRESULT get_Parent(int level, REFINTF parent) {
    if (!parent.pp) return E_POINTER;
    if (afx::ILIsRoot(get_ID())) return E_FAIL;
    LPITEMIDLIST pidl = ILClone(get_ID());
    while (--level > 0 && SUCCEEDED(ILRemoveLastID(pidl))) {
    }
    ref<IShellFolder> pParentFolder;
    LPITEMIDLIST leaf;
    HRESULT hr = afx::ILGetParentFolder(pidl, &pParentFolder, (const ITEMIDLIST**)&leaf);
    if (FAILED(hr) || leaf->mkid.cb == 0) {
      ILFree(pidl);
      return hr;
    }
    // 末端ノードを取り除いている。"Parent/Leaf" => "Parent\0Leaf" というイメージ。
    leaf->mkid.cb = 0;  // item->RemoveLeaf(leaf)
    return ref<IEntry>::from(NewEntry(pidl))->QueryInterface(parent);
  }
  string get_Name() {
    if (!m_name) UpdateFileInfo();
    return m_name;
  }
  string get_Path() {
    if (!m_path) {
      LPCITEMIDLIST pidl = get_ID();
      TCHAR path[MAX_PATH];
      if SUCCEEDED (afx::ILGetPath(pidl, path)) m_path = path;
      /*
                              if(!afx::ILIsRoot(pidl)) // デスクトップはパスを取得できしまうものの、仮想アイテムとして扱いたいため
                              {
                                      TCHAR path[MAX_PATH];
                                      if SUCCEEDED(afx::ILGetPath(pidl, path))
                                              m_path = path;
                              }
      */
    }
    return m_path;
  }

 public:
  static Entry* NewEntry(LPITEMIDLIST pidl, IShellFolder* parent = null, LPCITEMIDLIST leaf = null) {
    ASSERT(pidl);
    AutoLock lock(csEntryPool);
    Pool& pool = GetPool();
    Entry*& entry = pool[pidl];
    if (entry) {
      entry->AddRef();
      if (parent && leaf) entry->UpdateAttribute(parent, leaf);
      // TRACE(_T("info: Recycle Entry( $1 , size=$2)"), entry->Name, ILGetSize(pidl));
      ILFree(pidl);
    } else {
      entry = new Entry(pidl, parent, leaf);
      // TRACE(_T("info: New Entry( $1 , size=$2)"), entry->Name, ILGetSize(pidl));
    }
    return entry;
  }
  static Entry* NewEntry(LPITEMIDLIST pidl, DWORD attrs) {
    ASSERT(pidl);
    AutoLock lock(csEntryPool);
    Pool& pool = GetPool();
    Entry*& entry = pool[pidl];
    if (entry) {
      entry->AddRef();
      entry->UpdateAttribute(attrs);
      // TRACE(_T("info: Recycle Entry( $1 , size=$2)"), entry->Name, ILGetSize(pidl));
      ILFree(pidl);
    } else {
      entry = new Entry(pidl, attrs);
      // TRACE(_T("info: New Entry( $1 , size=$2)"), entry->Name, ILGetSize(pidl));
    }
    return entry;
  }
  void Dispose() throw() {
    if (m_pidl) {
      // TRACE(_T("info: Remove Entry( $1 )"), this->Name);
      RemoveFromPool(m_pidl);
      ILFree(m_pidl);
      m_pidl = null;
    }
    m_folder.clear();
  }
  static Entry* CreateEntryFromStandardPath(PCTSTR path) {
    WCHAR path2[MAX_PATH];
    afx::PathNormalize(path2, path);
    DWORD attrs = SFGAO_FOLDER;
    if (LPITEMIDLIST pidl = afx::ILFromPath(path2, &attrs)) return Entry::NewEntry(pidl, attrs);
    throw IOError(string::format(L"$1 は無効なパスです", path), STG_E_PATHNOTFOUND);
  }
  static Entry* CreateEntryFromCSIDL(int csidl, PCWSTR path, PCWSTR next) {
    LPITEMIDLIST pidl = null;
    if (FAILED(SHGetFolderLocation(null, csidl, null, 0, &pidl)) || !pidl)  // SUCCEEDED なのに null を返すことがある！
      throw IOError(string::format(_T("特殊フォルダ $1 (CSIDL=$2) を取得できません"), path, csidl), STG_E_PATHNOTFOUND);
    if (!*next) return Entry::NewEntry(pidl);
    // 特殊フォルダに続いて、何かしらのパスが指定されていた
    Entry* parent = Entry::NewEntry(pidl);
    IEntry* result = null;
    HRESULT hr = parent->ParseDisplayName(&result, next);
    parent->Release();
    if FAILED (hr) throw IOError(string::format(_T("$1 は無効なパスです"), path), STG_E_PATHNOTFOUND);
    return static_cast<Entry*>(result);
  }
  static Entry* CreateEntryFromString(PCWSTR path) {
    const int CSIDL_AVESTA = 0xFFFFFFFF;
    std::pair<int, PCWSTR> result = io::PathResolveCSIDL(path, L"AVESTA", CSIDL_AVESTA);
    int csidl = result.first;
    PCWSTR next = result.second;
    if (!next) return CreateEntryFromStandardPath(path);
    io::Path file;
    switch (csidl) {
      case CSIDL_AVESTA:
        ::GetModuleFileName(null, file, MAX_PATH);
        file.RemoveLeaf().RemoveLeaf();
        break;
      case CSIDL_MYDOCUMENTS:  // CSIDL_PERSONAL は、なぜか失敗するので特別処理を行う。
        str::copy(file, GUID_MyDocument);
        break;
      default:
        return CreateEntryFromCSIDL(csidl, path, next);
    }
    if (*next) file.Append(next);
    return CreateEntryFromStandardPath(file);
  }
  template <typename T>
  static T* __new__(IUnknown* arg) {
    if (!arg) {
      LPITEMIDLIST pidl = null;
      SHGetFolderLocation(null, CSIDL_DESKTOP, 0, 0, &pidl);
      ASSERT(pidl);
      return Entry::NewEntry(pidl);
    } else if (string path = cast(arg)) {
      PCWSTR wcsPath = path.str();
      try {
        return CreateEntryFromString(wcsPath);
      } catch (Error&) {
        WCHAR wcsExpanded[MAX_PATH];
        ::ExpandEnvironmentStrings(wcsPath, wcsExpanded, MAX_PATH);
        if (str::equals_nocase(wcsPath, wcsExpanded)) throw;
        return CreateEntryFromString(wcsExpanded);
      }
    } else {
      LPITEMIDLIST pidl = null;
      Stream stream(__uuidof(io::Reader), arg);
      HRESULT hr = ILLoadFromStreamEx(stream, &pidl);
      if FAILED (hr) throw IOError(_T("シリアライズから復元できません"), hr);
      ASSERT(pidl);
      return Entry::NewEntry(pidl);
    }
  }

 public:  // ISerializable
  REFCLSID get_Class() throw() { return __uuidof(this); }
  void Serialize(IStream& stream) { ILSaveToStream(&stream, get_ID()); }

 public:  // IEntry
  LPCITEMIDLIST get_ID() throw() { return m_pidl; }
  int get_Image() {
    if (m_image == NOIMAGE) UpdateFileInfo();
    return m_image;
  }
  bool IsFolder() {
    UpdateAttribute();
    return !!(m_attrs & SFGAO_FOLDER);
  }
  HRESULT QueryObject(REFINTF ppObject, IndexOrIDList relpath = 0) {
    if (relpath == 0) {  // 自分自身
      if SUCCEEDED (QueryInterface(ppObject)) {
        return S_OK;
      } else if (ppObject.iid == IID_IContextMenu || ppObject.iid == IID_IContextMenu2 || ppObject.iid == IID_IDropTarget || ppObject.iid == IID_IDataObject || ppObject.iid == IID_IExtractIcon || ppObject.iid == IID_IQueryInfo) {
        ref<IEntry> parentEntry;
        ref<IShellFolder> parentFolder;
        LPCITEMIDLIST pidl = ILFindLastID(get_ID());
        HRESULT hr;
        if (SUCCEEDED(hr = QueryObject(&parentEntry, IDList_Parent)) && SUCCEEDED(hr = parentEntry->QueryObject(&parentFolder)) && SUCCEEDED(hr = parentFolder->GetUIObjectOf(null, 1, &pidl, ppObject.iid, 0, ppObject.pp)))
          return S_OK;
        else
          return hr;
      } else {
        return GetFolder(ppObject);
      }
    } else if (relpath.is_index()) {  // N 段階上の親
      if (relpath == IDList_Linked)
        return ResolveLink(ppObject);
      else
        return get_Parent((int)relpath, ppObject);
    } else {  // 相対パスでの子供
      return ref<IEntry>::from(NewEntry(ILCombine(get_ID(), relpath)))->QueryInterface(ppObject);
    }
  }
  HRESULT ParseDisplayName(REFINTF ppObject, PCWSTR relpath) {
    if (str::empty(relpath)) return QueryObject(ppObject);
    HRESULT hr;
    ref<IShellFolder> folder;
    if FAILED (hr = GetFolder(&folder)) return hr;
    DWORD attrs = SFGAO_FOLDER;
    LPITEMIDLIST leaf;
    WCHAR path2[MAX_PATH];
    afx::PathNormalize(path2, relpath);
    if FAILED (hr = folder->ParseDisplayName(null, null, path2, null, &leaf, &attrs)) return hr;
    LPITEMIDLIST pidl = ILCombine(get_ID(), leaf);
    ILFree(leaf);
    ref<Entry> entry;
    entry.attach(Entry::NewEntry(pidl, attrs));
    return entry->QueryObject(ppObject);
  }
  bool Exists() {
    string path = get_Path();
    if (!path) {  // システムフォルダ…たぶん存在しているだろう
      return true;
    }
    PCTSTR szPath = path.str();
    return ::PathFileExists(szPath) != 0;
  }
  bool Equals(IEntry* rhs, NameType what) {
    if (!rhs) return false;
    ref<IUnknown> unk;
    rhs->QueryInterface(&unk);
    if (OID == unk) return true;
    if (ILIsEqual(this->get_ID(), rhs->get_ID())) return true;
    switch (what) {
      case IDENTIFIER:
        return false;
      default:
        break;
    }
    string pathL = this->GetName(what), pathR = rhs->GetName(what);
    if (!pathL && !pathR) return false;
    return pathL.equals_nocase(pathR);
  }

  string GetName(NameType what) {
    switch (what) {
      case NAME:
        return get_Name();
      case PATH:
        return get_Path();
      case LEAF:
        return ExtractLeaf(get_Path());
      case BASE:
        return ExtractBase(get_Path());
      case EXTENSION:
        return ExtractExtension(get_Path());
      case URL:
        return ExtractURL(get_Path());
      case IDENTIFIER:
        return ExtractID(get_ID());
      case PATH_OR_NAME: {
        string path = get_Path();
        return !!path ? path : get_Name();
      }
      case LEAF_OR_NAME: {
        string path = get_Path();
        return !!path ? ExtractLeaf(path) : get_Name();
      }
      case BASE_OR_NAME: {
        string path = get_Path();
        return !!path ? ExtractBase(path) : get_Name();
      }
      default:
        TRESPASS_DBG(return null);
    }
  }
  HRESULT SetName(PCWSTR name, HWND hwnd = null) {
    HRESULT hr;
    ref<IShellFolder> parent;
    LPCITEMIDLIST leaf = null;
    if FAILED (hr = afx::ILGetParentFolder(get_ID(), &parent, &leaf)) return hr;
    LPITEMIDLIST pidl = null;
    if FAILED (hr = parent->SetNameOf(hwnd, leaf, name, SHGDN_FOREDITING, &pidl)) return hr;
    ASSERT(pidl);
    if (pidl) {
      m_name.clear();
      m_path.clear();
      m_image = -2;
      m_attrs = 0;
      m_folder.clear();
      m_isAttrValid = false;
      ReplacePool(get_ID(), pidl, this);
      ILFree(m_pidl);
      m_pidl = pidl;
    }
    return S_OK;
  }
  ref<IEnumUnknown> EnumChildren(bool includeFiles) {
    HRESULT hr;

    UINT flags = SHCONTF_FOLDERS;                   //
    if (includeFiles) flags |= SHCONTF_NONFOLDERS;  // | SHCONTF_INCLUDEHIDDEN;

    ref<IShellFolder> folder;
    if FAILED (hr = GetFolder(&folder)) return null;
    ref<IEnumIDList> pEnum;
    if FAILED (hr = folder->EnumObjects(NULL, flags, &pEnum)) return null;

    using EnumEntry = Enumerator<IEntry>;
    ref<EnumEntry> e;
    e.attach(new EnumEntry());
    LPCITEMIDLIST parent = get_ID();
    LPITEMIDLIST leaf;
    while (pEnum->Next(1, &leaf, NULL) == S_OK) {
      LPITEMIDLIST pidl = ILCombine(parent, leaf);
      e->push_back(ref<IEntry>::from(NewEntry(pidl, folder, leaf)));
    }
    std::sort(e->begin(), e->end(), EntryCompare(folder));
    e->Reset();
    return e;
  }
};

//==============================================================================

class EntryAlias : public Root<implements<IEntry, ISerializable> > {
 private:
  ref<Entry> m_pInner;
  string m_AliasName;

 public:
  EntryAlias(LPITEMIDLIST pidl, const string& name) : m_AliasName(name) { m_pInner.attach(Entry::NewEntry(pidl)); }
  void Dispose() throw() {
    m_pInner.clear();
    m_AliasName.clear();
  }

 public:  // ISerializable
  REFCLSID get_Class() throw() { return m_pInner->get_Class(); }
  void Serialize(IStream& stream) { m_pInner->Serialize(stream); }

 public:  // IEntry
  string GetName(NameType what) {
    switch (what) {
      case NAME:
        return m_AliasName;
      case PATH_OR_NAME: {
        string path = m_pInner->Path;
        return !!path ? path : m_AliasName;
      }
      case LEAF_OR_NAME: {
        string path = m_pInner->GetName(LEAF);
        return !!path ? path : m_AliasName;
      }
      case BASE_OR_NAME: {
        string path = m_pInner->GetName(BASE);
        return !!path ? path : m_AliasName;
      }
      default:
        return m_pInner->GetName(what);
    }
  }
  HRESULT SetName(PCWSTR name, HWND hwnd = null) {
    m_AliasName = name;
    return S_OK;
  }
  LPCITEMIDLIST get_ID() { return m_pInner->get_ID(); }
  int get_Image() { return m_pInner->get_Image(); }
  bool IsFolder() { return m_pInner->IsFolder(); }
  HRESULT QueryObject(REFINTF ppObject, IndexOrIDList relpath = 0) {
    if (!relpath && SUCCEEDED(QueryInterface(ppObject)))
      return S_OK;
    else
      return m_pInner->QueryObject(ppObject, relpath);
  }
  HRESULT ParseDisplayName(REFINTF ppObject, PCWSTR relpath) { return m_pInner->ParseDisplayName(ppObject, relpath); }
  bool Exists() { return m_pInner->Exists(); }
  bool Equals(IEntry* rhs, NameType what) {
    if (!rhs) return false;
    ref<IUnknown> unk;
    rhs->QueryInterface(&unk);
    if (OID == unk) return true;
    return m_pInner->Equals(rhs, what);
  }
  ref<IEnumUnknown> EnumChildren(bool includeFiles) { return m_pInner->EnumChildren(includeFiles); }
};

//==============================================================================

class EntryList : public Root<implements<IEntryList> > {
 private:
  HGLOBAL m_hGlobal;
  CIDA* m_pCIDA;

 public:
  void InitOnHGlobal(HGLOBAL hGlobal) {
    m_hGlobal = hGlobal;
    m_pCIDA = (CIDA*)::GlobalLock(m_hGlobal);
  }
  void InitOnMalloc(CIDA* cida) {
    m_hGlobal = null;
    m_pCIDA = cida;
  }
  void __init__(IUnknown* arg) {
    m_hGlobal = NULL;
    m_pCIDA = NULL;
    if (ref<IDataObject> data = cast(arg)) {
      FORMATETC FORMAT_IDLIST = {(CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
      STGMEDIUM medium;
      HRESULT hr = data->GetData(&FORMAT_IDLIST, &medium);
      if FAILED (hr) throw IOError(_T("IDataObject から CFSTR_SHELLIDLIST/TYMED_HGLOBAL を取得できません"), hr);
      InitOnHGlobal(medium.hGlobal);
    } else {
      throw ArgumentError(_T("invalid constructor args for EntryList"));
    }
  }
  void Dispose() throw() {
    if (m_hGlobal) {
      ::GlobalUnlock(m_hGlobal);
      // ::GlobalFree(m_hGlobal); // XXX: 解放っていらないの？
      m_hGlobal = NULL;
    } else {
      ::free(m_pCIDA);
    }
    m_pCIDA = NULL;
  }

 public:  // IEntryList
  size_t get_Count() { return afx::CIDAGetCount(m_pCIDA); }
  LPCITEMIDLIST get_Parent() { return afx::CIDAGetParent(m_pCIDA); }
  LPCITEMIDLIST get_Leaf(size_t index) {
    if (index >= get_Count()) return null;
    return afx::CIDAGetAt(m_pCIDA, index);
  }
  const CIDA* GetCIDA() { return m_pCIDA; }
  HRESULT GetAt(IEntry** ppShellItem, size_t index) {
    LPCITEMIDLIST leaf = this->Leaf[index];
    if (!leaf) return E_INVALIDARG;
    Entry* entry = Entry::NewEntry(ILCombine(this->Parent, leaf));
    HRESULT hr = entry->QueryInterface(__uuidof(IEntry), (void**)ppShellItem);
    entry->Release();
    return hr;
  }
  HRESULT CloneSubset(REFINTF pp, size_t subsets[], size_t length) {
    // 本当は、抽出してもよいのだが、簡単のためにmemcpyし、ヘッダだけを変更する.
    size_t size = afx::CIDAGetSize(m_pCIDA);
    CIDA* cida = (CIDA*)::malloc(size);

    std::vector<UINT> newindex;
    newindex.reserve(length + 1);
    newindex.push_back(m_pCIDA->aoffset[0]);
    for (size_t i = 0; i < length; ++i) {
      size_t at = subsets[i];
      if (at < get_Count()) newindex.push_back(m_pCIDA->aoffset[at + 1]);
    }
    cida->cidl = newindex.size() - 1;
    memcpy(cida->aoffset, &newindex[0], sizeof(UINT) * newindex.size());

    ref<EntryList> clone;
    clone.attach(new EntryList());
    clone->InitOnMalloc(cida);
    return clone.copyto(pp);
  }
};

HRESULT Entry::ResolveLink(REFINTF ppObject) {
  ref<IShellLink> link;
  HRESULT hr;
  if FAILED (hr = ::CoCreateInstance(CLSID_ShellLink, null, CLSCTX_INPROC, IID_IShellLink, (void**)&link)) return hr;
  LPITEMIDLIST pidl = null;
  if (SUCCEEDED(hr = cast<IPersistFile>(link)->Load(get_Path().str(), STGM_READ)) && SUCCEEDED(hr = link->Resolve(null, SLR_NOLINKINFO | SLR_NO_UI | SLR_NOUPDATE | SLR_NOSEARCH | SLR_NOTRACK)) &&
      (hr = link->GetIDList(&pidl)) == S_OK)  // ← SUCCEEDED ではなく、S_OK を使う必要あり
  {
    return objnew<EntryAlias>(pidl, get_Name())->QueryObject(ppObject);
  } else {
    return QueryObject(ppObject);
  }
}

}  // namespace io
}  // namespace mew

// HRESULT CreateEntry(IEntry** pp, STRING src, PathFrom from = None);
HRESULT mew::io::CreateEntry(IEntry** ppEntry, LPCITEMIDLIST pidl, PathFrom from) {
  if (!ppEntry) return E_POINTER;
  *ppEntry = null;
  if (!pidl) return E_POINTER;
  *ppEntry = Entry::NewEntry(ILClone(pidl));
  ASSERT(*ppEntry);
  return S_OK;
}
// ref<IEntry> CreateEntry(const ITEMIDLIST* src, PathFrom from = None);
// ref<IEntry> CreateEntry(STRING src, PathFrom from = None);
// HRESULT CIDACreate(IEntryList** pp, IDataObject* data);
// HRESULT CIDACreate(IEntryList** pp, HWND hwndListView, IShellView* pShellView, INT svgio);

AVESTA_EXPORT(Entry)
AVESTA_EXPORT(EntryList)

//==============================================================================

CriticalSection Entry::csEntryPool;
