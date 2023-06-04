// xmlmessage.cpp

#include "stdafx.h"
#include "../mew/private.h"
#include "xml.hpp"
#include "message.hpp"

namespace {

static PCWSTR TypeToName(mew::TypeCode typecode) {
  switch (typecode) {
    case mew::TypeNull:
      return L"Null";
    case mew::TypeBool:
      return L"Bool";
    case mew::TypeSint8:
      return L"Sint8";
    case mew::TypeUint8:
      return L"Uint8";
    case mew::TypeSint16:
      return L"Sint16";
    case mew::TypeUint16:
      return L"Uint16";
    case mew::TypeSint32:
      return L"Sint32";
    case mew::TypeUint32:
      return L"Uint32";
    case mew::TypeSint64:
      return L"Sint64";
    case mew::TypeUint64:
      return L"Uint64";
    case mew::TypeSize:
      return L"Size";
    case mew::TypePoint:
      return L"Point";
    case mew::TypeRect:
      return L"Rect";
    case mew::TypeColor:
      return L"Color";
    case mew::TypeUnknown:
      return L"String";  // すべてのオブジェクトは文字列として保存する
    default:
      return nullptr;
  }
}

struct Name2Type {
  PCWSTR Guid;
  INT32 Type;
};
#define N2T(what) {L#what, mew::Type##what},
static const Name2Type N2T_BEGIN[] = {N2T(Bool) N2T(Color) N2T(Null) N2T(Point) N2T(Rect) N2T(Sint16) N2T(Sint32) N2T(Sint64)
                                          N2T(Sint8) N2T(Size){L"String", mew::TypeUnknown},  // N2T(String)
                                      N2T(Uint16) N2T(Uint32) N2T(Uint64) N2T(Uint8)};
#undef N2T
static const size_t N2T_LENGTH = sizeof(N2T_BEGIN) / sizeof(N2T_BEGIN[0]);
static const Name2Type* N2T_END = N2T_BEGIN + N2T_LENGTH;

struct StringAndLength {
  PCWSTR str;
  size_t cch;

  StringAndLength(PCWSTR s, size_t c) : str(s), cch(c) {}
};

inline static bool operator<(const Name2Type& lhs, const StringAndLength& rhs) throw() {
  return wcsncmp(lhs.Guid, rhs.str, rhs.cch) < 0;
}
inline static bool operator==(const Name2Type& lhs, const StringAndLength& rhs) throw() {
  return wcsncmp(lhs.Guid, rhs.str, rhs.cch) == 0;
}

static mew::TypeCode NameToType(PCWSTR name, size_t cch) throw() {
#ifdef _DEBUG
  static bool checked = false;
  if (!checked) {
    for (size_t i = 0; i < N2T_LENGTH - 1; ++i) {
      ASSERT(wcscmp(N2T_BEGIN[i].Guid, N2T_BEGIN[i + 1].Guid) < 0);
    }
    checked = true;
  }
#endif
  const Name2Type* found = mew::algorithm::binary_search(N2T_BEGIN, N2T_END, StringAndLength(name, cch));
  if (found == N2T_END) {
    ASSERT(!"理解できないタイプ名");
    return mew::TypeNull;
  } else
    return found->Type;
}

inline static bool equals(PCWSTR lhs, size_t cch, PCWSTR rhs) throw() { return mew::str::compare(lhs, rhs, cch) == 0; }

class XMLMessageLoader : public mew::xml::XMLHandlerImpl {
 private:
  std::vector<mew::message> m_stack;
  struct {
    mew::TypeCode type;
    mew::Guid name;
    mew::string text;
  } m_var;

  static mew::TypeCode GetCodeAttr(mew::xml::XMLAttributes& attr) {
    mew::TypeCode code = 0;
    if (mew::string var = attr[L"code"]) {
      PCWSTR str = var.str();
      if (str[0] == L'#') {
        code = mew::str::atoi(str + 1);
      } else {
        WCHAR buf[4];
        wcsncpy(buf, str, 4);
        code = (UINT32)(str[0] << 24) | (UINT32)(str[1] << 16) | (UINT32)(str[2] << 8) | (UINT32)(str[3]);
      }
    }
    return code;
  }
  static mew::Guid GetNameAttr(mew::xml::XMLAttributes& attr) {
    mew::Guid name;
    if (mew::string var = attr[L"name"]) {
      PCWSTR str = var.str();
      if (mew::str::atoguid(name, str)) {
        return name;
      }
      size_t length = wcslen(str);
      if (str[0] == '#') {
        return mew::Guid(mew::str::atoi(str + 1));
      } else if (length <= 16) {
        memset(&name, 0, sizeof(mew::Guid));
        mew::str::convert((char*)&name, str, 16);
        return name;
      }
    }
    return GUID_NULL;
  }

 public:
  HRESULT StartDocument() {
    m_stack.clear();
    m_stack.reserve(10);
    return S_OK;
  }
  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    if (equals(name, cch, L"Message")) {
      mew::message msg(GetCodeAttr(attr));
      if (!m_stack.empty()) {
        m_stack.back()[GetNameAttr(attr)] = msg;
      }
      m_stack.push_back(msg);
    } else {
      mew::TypeCode code = NameToType(name, cch);
      m_var.type = code;
      m_var.name = GetNameAttr(attr);
      m_var.text.clear();
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, cch, L"Message")) {
      if (m_stack.size() > 1) {
        m_stack.pop_back();
      }
    } else if (NameToType(name, cch)) {
      mew::variant var(m_var.type, m_var.text);
      if (m_stack.empty()) {
        m_stack.push_back(var);
      } else {
        m_stack.back()[m_var.name] = var;
      }
    } else {
      TRACE(_T("warning: 未知のエレメント : $1"), mew::string(name, cch));
    }
    m_var.text.clear();
    return S_OK;
  }
  HRESULT Characters(PCWSTR chars, size_t cch) {
    m_var.text.assign(chars, cch);
    return S_OK;
  }

 public:
  HRESULT GetProduct(mew::REFINTF pp) {
    if (m_stack.size() != 1) {
      return E_FAIL;
    }
    return m_stack.front()->QueryInterface(pp);
  }
};

static void WriteMessage(mew::xml::IXMLWriter* sax, const mew::message& msg, PCWSTR name);
static void WriteVariant(mew::xml::IXMLWriter* sax, const mew::variant& var, PCWSTR name) {
  if (var.type == mew::TypeUnknown) {
    if (mew::message submsg = var) {
      WriteMessage(sax, submsg, name);
      return;
    }
  }
  // else
  PCWSTR type = TypeToName(var.type);
  if (!type) {
    throw mew::exceptions::ArgumentError(mew::string::load(IDS_ERR_XMLMSG_TYPECODE, name, (mew::string)var), var.type);
  }
  sax->StartElement(type);
  if (name) {
    sax->Attribute(L"name", name);
  }
  mew::string chars(var);
  sax->Characters(chars.str());
  sax->EndElement();
}
static void WriteMessage(mew::xml::IXMLWriter* sax, const mew::message& msg, PCWSTR name) {
  sax->StartElement(L"Message");
  if (name) {
    sax->Attribute(L"name", name);
  }
  if (mew::EventCode code = msg.code) {
    sax->Attribute(L"code", mew::ToString<mew::TypeCode>(code));
  }
  mew::Guid key;
  mew::variant var;
  for (mew::ref<mew::IEnumVariant> pEnum = msg->Enumerate(); pEnum && pEnum->Next(&key, &var);) {
    WriteVariant(sax, var, mew::ToString<mew::Guid>(key));
  }
  sax->EndElement();
}
}  // namespace

namespace mew {
namespace xml {
message LoadMessage(IUnknown* src, IXMLReader* sax_) {
  Stream stream(__uuidof(io::Reader), src);
  ref<IXMLReader> sax(sax_);
  if (!sax) {
    sax.create(__uuidof(XMLReader));
  }
  XMLMessageLoader handler;
  HRESULT hr = sax->Parse(&handler, stream);
  if FAILED (hr) {
    return null;
  }
  message msg;
  handler.GetProduct(&msg);
  return msg;
}

HRESULT SaveMessage(const message& msg, IUnknown* dst, IXMLWriter* sax_) {
  ASSERT(msg);
  Stream stream(__uuidof(io::Writer), dst);
  if (!msg) {
    return E_INVALIDARG;
  }
  ref<IXMLWriter> sax(sax_);
  if (!sax) {
    sax.create(__uuidof(XMLWriter));
  }
  sax->StartDocument(stream);
  WriteMessage(sax, msg, null);
  sax->EndDocument();
  return S_OK;
}
}  // namespace xml
}  // namespace mew