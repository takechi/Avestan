// xmlmessage.cpp

#include "stdafx.h"
#include "../mew/private.h"
#include "xml.hpp"
#include "message.hpp"

using namespace mew;
using namespace mew::xml;

namespace {
static PCWSTR TypeToName(TypeCode typecode) {
  switch (typecode) {
    case TypeNull:
      return L"Null";
    case TypeBool:
      return L"Bool";
    case TypeSint8:
      return L"Sint8";
    case TypeUint8:
      return L"Uint8";
    case TypeSint16:
      return L"Sint16";
    case TypeUint16:
      return L"Uint16";
    case TypeSint32:
      return L"Sint32";
    case TypeUint32:
      return L"Uint32";
    case TypeSint64:
      return L"Sint64";
    case TypeUint64:
      return L"Uint64";
    case TypeSize:
      return L"Size";
    case TypePoint:
      return L"Point";
    case TypeRect:
      return L"Rect";
    case TypeColor:
      return L"Color";
    case TypeUnknown:
      return L"String";  // すべてのオブジェクトは文字列として保存する
    default:
      return null;
  }
}

struct Name2Type {
  PCWSTR Guid;
  INT32 Type;
};
#define N2T(what) {L#what, Type##what},
static const Name2Type N2T_BEGIN[] = {N2T(Bool) N2T(Color) N2T(Null) N2T(Point) N2T(Rect) N2T(Sint16) N2T(Sint32) N2T(Sint64)
                                          N2T(Sint8) N2T(Size){L"String", TypeUnknown},  // N2T(String)
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

static TypeCode NameToType(PCWSTR name, size_t cch) throw() {
#ifdef _DEBUG
  static bool checked = false;
  if (!checked) {
    for (size_t i = 0; i < N2T_LENGTH - 1; ++i) {
      ASSERT(wcscmp(N2T_BEGIN[i].Guid, N2T_BEGIN[i + 1].Guid) < 0);
    }
    checked = true;
  }
#endif
  const Name2Type* found = algorithm::binary_search(N2T_BEGIN, N2T_END, StringAndLength(name, cch));
  if (found == N2T_END) {
    ASSERT(!"理解できないタイプ名");
    return TypeNull;
  } else
    return found->Type;
}

inline static bool equals(PCWSTR lhs, size_t cch, PCWSTR rhs) throw() { return str::compare(lhs, rhs, cch) == 0; }

class XMLMessageLoader : public XMLHandlerImpl {
 private:
  std::vector<message> m_stack;
  struct {
    TypeCode type;
    Guid name;
    string text;
  } m_var;

  static TypeCode GetCodeAttr(XMLAttributes& attr) {
    TypeCode code = 0;
    if (string var = attr[L"code"]) {
      PCWSTR str = var.str();
      if (str[0] == L'#')
        code = str::atoi(str + 1);
      else {
        WCHAR buf[4];
        wcsncpy(buf, str, 4);
        code = (UINT32)(str[0] << 24) | (UINT32)(str[1] << 16) | (UINT32)(str[2] << 8) | (UINT32)(str[3]);
      }
    }
    return code;
  }
  static Guid GetNameAttr(XMLAttributes& attr) {
    Guid name;
    if (string var = attr[L"name"]) {
      PCWSTR str = var.str();
      if (str::atoguid(name, str)) return name;
      size_t length = wcslen(str);
      if (str[0] == '#') {
        return Guid(str::atoi(str + 1));
      } else if (length <= 16) {
        memset(&name, 0, sizeof(Guid));
        str::convert((char*)&name, str, 16);
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
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    if (equals(name, cch, L"Message")) {
      message msg(GetCodeAttr(attr));
      if (!m_stack.empty()) m_stack.back()[GetNameAttr(attr)] = msg;
      m_stack.push_back(msg);
    } else {
      TypeCode code = NameToType(name, cch);
      m_var.type = code;
      m_var.name = GetNameAttr(attr);
      m_var.text.clear();
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, cch, L"Message")) {
      if (m_stack.size() > 1) m_stack.pop_back();
    } else if (NameToType(name, cch)) {
      variant var(m_var.type, m_var.text);
      if (m_stack.empty())
        m_stack.push_back(var);
      else
        m_stack.back()[m_var.name] = var;
    } else {
      TRACE(_T("warning: 未知のエレメント : $1"), string(name, cch));
    }
    m_var.text.clear();
    return S_OK;
  }
  HRESULT Characters(PCWSTR chars, size_t cch) {
    m_var.text.assign(chars, cch);
    return S_OK;
  }

 public:
  HRESULT GetProduct(REFINTF pp) {
    if (m_stack.size() != 1) return E_FAIL;
    return m_stack.front()->QueryInterface(pp);
  }
};

static void WriteMessage(IXMLWriter* sax, const message& msg, PCWSTR name);
static void WriteVariant(IXMLWriter* sax, const variant& var, PCWSTR name) {
  if (var.type == TypeUnknown) {
    if (message submsg = var) {
      WriteMessage(sax, submsg, name);
      return;
    }
  }
  // else
  PCWSTR type = TypeToName(var.type);
  if (!type) throw ArgumentError(string::load(IDS_ERR_XMLMSG_TYPECODE, name, (string)var), var.type);
  sax->StartElement(type);
  if (name) sax->Attribute(L"name", name);
  string chars(var);
  sax->Characters(chars.str());
  sax->EndElement();
}
static void WriteMessage(IXMLWriter* sax, const message& msg, PCWSTR name) {
  sax->StartElement(L"Message");
  if (name) sax->Attribute(L"name", name);
  if (EventCode code = msg.code) sax->Attribute(L"code", ToString<TypeCode>(code));
  Guid key;
  variant var;
  for (ref<IEnumVariant> pEnum = msg->Enumerate(); pEnum && pEnum->Next(&key, &var);) {
    WriteVariant(sax, var, ToString<Guid>(key));
  }
  sax->EndElement();
}
}  // namespace

message mew::xml::LoadMessage(IUnknown* src, IXMLReader* sax_) {
  Stream stream(__uuidof(io::Reader), src);
  ref<IXMLReader> sax(sax_);
  if (!sax) sax.create(__uuidof(XMLReader));
  XMLMessageLoader handler;
  HRESULT hr = sax->Parse(&handler, stream);
  if FAILED (hr) return null;
  message msg;
  handler.GetProduct(&msg);
  return msg;
}

HRESULT mew::xml::SaveMessage(const message& msg, IUnknown* dst, IXMLWriter* sax_) {
  ASSERT(msg);
  Stream stream(__uuidof(io::Writer), dst);
  if (!msg) return E_INVALIDARG;
  ref<IXMLWriter> sax(sax_);
  if (!sax) sax.create(__uuidof(XMLWriter));
  sax->StartDocument(stream);
  WriteMessage(sax, msg, null);
  sax->EndDocument();
  return S_OK;
}
