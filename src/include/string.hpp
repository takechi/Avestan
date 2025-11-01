/// @file string.hpp
/// 文字列.
#pragma once

#include "reference.hpp"
#include "std/str.hpp"

namespace module {
const bool isNT = true;
extern HMODULE Handle;
}  // namespace module

namespace mew {
//==============================================================================
// クラス

class __declspec(uuid("1E2C5D5A-C248-4A87-B4D9-389C64E96B74")) String;

//==============================================================================
/// 文字列インタフェース.
__interface IString : ISerializable {
  /// null終端のUNICODE文字列を返す.
  PCWSTR GetBuffer() noexcept;
  /// UNICODE文字列の文字数を返す.
  size_t GetLength() noexcept;
};

void StringReplace(IString** pp, IString* s, WCHAR from, WCHAR to);

//==============================================================================

namespace detail {
template <class X>
class ToStringDefault {
 private:
  const X& m_obj;

 public:
  using argument_type = const X&;
  ToStringDefault(argument_type obj) : m_obj(obj) {}
  operator PCWSTR() const { return static_cast<PCWSTR>(m_obj); }
};

template <>
class ToStringDefault<IUnknown*> {
 private:
  ref_base<IString> m_str;

 public:
  using argument_type = IUnknown*;
  ToStringDefault(argument_type value) noexcept { ObjectToString(&m_str, value); }
  operator PCWSTR() const noexcept { return m_str->GetBuffer(); }
};
}  // namespace detail

/// 文字列への変換.
template <class X>
class ToString {
  using Detail = mew::detail::ToStringDefault<typename meta::If<meta::Convertable<X, IUnknown*>::value, IUnknown*, X>::Result>;
  Detail m_str;

 public:
  ToString(typename Detail::argument_type obj) : m_str(obj) {}
  operator PCWSTR() const { return m_str; }
};

/// 文字列.
template <>
class ref<IString> : public ref_base<IString> {
  using super = ref_base<IString>;

 public:
  using const_iterator = PCWSTR;
  using const_pointer = PCWSTR;
  using const_reference = const WCHAR&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using value_type = WCHAR;
  // const_reverse_iterator
  static const size_type npos = (size_type)-1;

 public:
  string() noexcept {}
  string(const Null&) noexcept {}
  string(IString* p) noexcept : super(p) {}
  string(const string& p) noexcept : super(p) {}
  string(PCWSTR format, size_t length = npos, size_t argc = 0, PCWSTR argv[] = 0) noexcept {
    CreateString(&m_ptr, format, length, argc, argv);
  }
  string(PCWSTR beg, PCWSTR end, size_t argc = 0, PCWSTR argv[] = 0) noexcept {
    CreateString(&m_ptr, beg, end - beg, argc, argv);
  }
  explicit string(UINT nID, HMODULE hModule = ::GetModuleHandle(0), size_t argc = 0, PCWSTR argv[] = 0) noexcept {
    CreateString(&m_ptr, nID, hModule, argc, argv);
  }
  string& operator=(const string& s) noexcept {
    super::operator=(s);
    return *this;
  }

  // basic_string compatible methods

  void assign(IString* str) noexcept { super::operator=(str); }
  void assign(PCWSTR str, size_t length = npos) noexcept {
    clear();
    CreateString(&m_ptr, str, length);
  }
  void assign(PCWSTR first, PCWSTR last) noexcept {
    clear();
    CreateString(&m_ptr, first, last - first);
  }

  PCWSTR str() const noexcept { return m_ptr ? m_ptr->GetBuffer() : L""; }
  size_t length() const noexcept { return m_ptr ? m_ptr->GetLength() : 0; }
  PCWSTR begin() const noexcept { return str(); }
  PCWSTR end() const noexcept { return str() + length(); }

  template <typename T>
  HRESULT copyto(T** pp) const noexcept {
    return mew::objcpy(m_ptr, pp);
  }
  HRESULT copyto(REFINTF pp) const noexcept { return mew::objcpy(m_ptr, pp); }
  void copyto(PWSTR dst) const {
    if (m_ptr)
      str::copy(dst, m_ptr->GetBuffer());
    else
      str::clear(dst);
  }
  void copyto(PWSTR dst, size_t len) const {
    if (m_ptr)
      str::copy(dst, m_ptr->GetBuffer(), len);
    else
      str::clear(dst);
  }

  bool equals(const string& rhs) const { return m_ptr == rhs.m_ptr || str::equals(str(), rhs.str()); }
  bool equals(PCWSTR rhs) const { return str::equals(str(), rhs); }
  bool equals_nocase(const string& rhs) const { return m_ptr == rhs.m_ptr || str::equals_nocase(str(), rhs.str()); }
  bool equals_nocase(PCWSTR rhs) const { return str::equals_nocase(str(), rhs); }
  int compare(const string& rhs) const { return str::compare(str(), rhs.str()); }
  int compare(PCWSTR rhs) const { return str::compare(str(), rhs); }
  int compare_nocase(const string& rhs) const { return str::compare_nocase(str(), rhs.str()); }
  int compare_nocase(PCWSTR rhs) const { return str::compare_nocase(str(), rhs); }

  string replace(WCHAR from, WCHAR to) const {
    string s;
    StringReplace(&s, m_ptr, from, to);
    return s;
  }

  // compare operations

  friend bool operator==(const string& lhs, const string& rhs) noexcept { return lhs.equals(rhs); }
  friend bool operator==(const string& lhs, PCWSTR rhs) noexcept { return lhs.equals(rhs); }
  friend bool operator==(PCWSTR lhs, const string& rhs) noexcept { return rhs.equals(lhs); }
  friend bool operator!=(const string& lhs, const string& rhs) noexcept { return !(lhs == rhs); }
  friend bool operator!=(const string& lhs, PCWSTR rhs) noexcept { return !(lhs == rhs); }
  friend bool operator!=(PCWSTR lhs, const string& rhs) noexcept { return !(lhs == rhs); }
  friend bool operator<(const string& lhs, const string& rhs) noexcept { return lhs.compare(rhs) < 0; }
  friend bool operator<(const string& lhs, PCWSTR rhs) noexcept { return lhs.compare(rhs) < 0; }
  friend bool operator<(PCWSTR lhs, const string& rhs) noexcept { return rhs.compare(lhs) > 0; }

 public:  // format
#define MEW_PP_STRING_TMP(n) ToString<TArg##n> str##n(arg##n);
#define MEW_PP_STRING_STR(n) str##n
#define MEW_PP_STRING_FMT(n)                                                  \
  template <PP_TYPENAMES(n)>                                                  \
  static string format(PCWSTR format, PP_ARGS_CONST(n)) noexcept {             \
    PP_REPEAT(n, MEW_PP_STRING_TMP)                                           \
    PCWSTR args[] = {PP_CSV1(n, MEW_PP_STRING_STR)};                          \
    return string(format, npos, n, args);                                     \
  }                                                                           \
  template <PP_TYPENAMES(n)>                                                  \
  static string format(UINT nID, HMODULE hModule, PP_ARGS_CONST(n)) noexcept { \
    PP_REPEAT(n, MEW_PP_STRING_TMP)                                           \
    PCWSTR args[] = {PP_CSV1(n, MEW_PP_STRING_STR)};                          \
    return string(nID, hModule, n, args);                                     \
  }                                                                           \
  template <PP_TYPENAMES(n)>                                                  \
  static string load(UINT nID, PP_ARGS_CONST(n)) noexcept {                    \
    return format(nID, module::Handle, PP_ARG_VALUES(n));                     \
  }

  PP_REPEAT_FROM_1(10, MEW_PP_STRING_FMT)

#undef MEW_PP_STRING_FMT
#undef MEW_PP_STRING_STR
#undef MEW_PP_STRING_TMP

  static string load(UINT nID) noexcept { return string(nID, module::Handle); }
};

//==============================================================================

// 文字列への変換(bool).
template <>
class ToString<bool> {
 private:
  bool m_value;

 public:
  ToString(bool value) noexcept : m_value(value) {}
  operator PCWSTR() const noexcept { return m_value ? _T("true") : _T("false"); }
};

// 文字列への変換(DWORD).
template <>
class ToString<DWORD> {
 private:
  WCHAR m_str[12];  // 0x12345678
 public:
  ToString(int value) noexcept { _stprintf(m_str, _T("0x%08X"), value); }
  operator PCWSTR() const noexcept { return m_str; }
};

#define DECLARE_STRING_CAST(type)                             \
  template <>                                                 \
  class ToString<type> {                                      \
   private:                                                   \
    WCHAR m_str[sizeof(type) > 4 ? 24 : 12];                  \
                                                              \
   public:                                                    \
    ToString(type value) noexcept { str::itoa(m_str, value); } \
    operator PCWSTR() const noexcept { return m_str; }         \
  };

DECLARE_STRING_CAST(INT8)
DECLARE_STRING_CAST(UINT8)
DECLARE_STRING_CAST(INT16)
DECLARE_STRING_CAST(UINT16)
DECLARE_STRING_CAST(INT32)
DECLARE_STRING_CAST(UINT32)
DECLARE_STRING_CAST(INT64)
DECLARE_STRING_CAST(UINT64)

#undef DECLARE_STRING_CAST

#define DECLARE_STRING_CAST(type)                             \
  template <>                                                 \
  class ToString<type> {                                      \
   private:                                                   \
    WCHAR m_str[32];                                          \
                                                              \
   public:                                                    \
    ToString(type value) noexcept { str::ftoa(m_str, value); } \
    operator PCWSTR() const noexcept { return m_str; }         \
  };

DECLARE_STRING_CAST(float)
DECLARE_STRING_CAST(double)

#undef DECLARE_STRING_CAST

// 文字列への変換(GUID).
template <>
class ToString<GUID> {
 private:
  WCHAR m_str[40];  // {12345678-1234-1234-1234-1234567890AB} : 38 chars
 public:
  ToString(const GUID& value) noexcept { str::guidtoa(m_str, value, true); }
  operator PCWSTR() const noexcept { return m_str; }
};

//==============================================================================

struct STRING {
  const PCWSTR str;

  STRING(PCWSTR s) noexcept : str(s) {}
  STRING(IString* s) noexcept : str(s ? s->GetBuffer() : null) {}
  STRING(const string& s) noexcept : str(s.str()) {}

  operator PCWSTR() const noexcept { return str; }
  bool operator!() const noexcept { return str::empty(str); }
};

//==============================================================================

class StringSplit {
 private:
  string m_src;  ///< ソース文字列.
  PCWSTR m_cur;
  string m_sep;   ///< 区切り文字.
  string m_trim;  ///< 区切り文字の前後で取り除く文字.

 public:
  StringSplit() {}
  StringSplit(const string& source, const string& separator, const string& trim) { set(source, separator, trim); }
  void set(const string& source, const string& separator, const string& trim) {
    m_src = source;
    m_cur = m_src.str();
    m_sep = separator;
    m_trim = trim;
  }
  bool next(PCWSTR& token, size_t& length) {
    token = 0;
    length = 0;
    if (!m_cur) return false;
    const PCWSTR sep = m_sep.str();
    const PCWSTR trim = m_trim.str();
    m_cur += str::span(m_cur, trim);
    size_t nextsep = str::cspan(m_cur, sep);
    if (nextsep == 0) {
      m_cur = 0;
      return false;
    }
    size_t len = nextsep - 1;
    while (len > 0 && str::find(trim, m_cur[len])) {
      --len;
    }
    token = m_cur;
    length = len + 1;
    if (m_cur[nextsep] == _T('\0'))
      m_cur = null;
    else
      m_cur += nextsep + 1;
    return true;
  }
  string next() {
    PCWSTR token;
    size_t length;
    if (!next(token, length)) return null;
    return string(token, length);
  }
};
}  // namespace mew
