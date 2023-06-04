// string.cpp

#include "stdafx.h"
#include <new>
#include "private.h"
#include "io.hpp"
#include "math.hpp"

//==============================================================================

namespace {
/// �Œ�\���̌�ɉϒ��o�b�t�@�������\���̂��\�z���邽�߂̃o�b�t�@.
template <class Struct, class T>
class VariableLengthBuffer {
 private:
  T* m_data;
  size_t m_length;
  size_t m_capacity;

 public:
  VariableLengthBuffer() throw() : m_data(0), m_length(0), m_capacity(0) {}
  ~VariableLengthBuffer() { ::free(detach()); }
  T* va_data() throw() { return m_data; }
  Struct* data() throw() { return m_data ? (Struct*)(((UINT8*)m_data) - sizeof(Struct)) : 0; }
  Struct* detach() throw() {
    Struct* head = data();
    m_data = 0;
    m_length = 0;
    m_capacity = 0;
    return head;
  }

 public:  // std::vector compatible methods
  void append(PCWSTR data, size_t len) throw() {
    reserve(m_length + len);
    memcpy(m_data + m_length, data, len * sizeof(T));
    m_length += len;
  }
  void push_back(T c) throw() {
    reserve(m_length + 1);
    m_data[m_length] = c;
    ++m_length;
  }
  bool empty() const throw() { return m_length == 0; }
  size_t size() const throw() { return m_length; }
  void resize(size_t sz) throw() {
    reserve(sz);
    m_length = sz;
  }
  void reserve(size_t capacity) throw() {
    if (m_capacity >= capacity) {
      return;
    }
    m_capacity = mew::math::max(static_cast<size_t>(16), capacity, m_capacity * 2);
    const size_t bytesize = sizeof(Struct) + m_capacity * sizeof(T);
    m_data = (T*)((UINT8*)::realloc(data(), bytesize) + sizeof(Struct));
  }
  void clear() throw() { m_length = 0; }

 private:  // non-copyable
  VariableLengthBuffer(VariableLengthBuffer&) throw();
  VariableLengthBuffer& operator=(VariableLengthBuffer&) throw();
};
}  // namespace

//==============================================================================

namespace mew {
class String : public Root<implements<IString, ISerializable> > {
 public:
  using Buffer = VariableLengthBuffer<String, WCHAR>;

 private:
  size_t m_length;
#pragma warning(disable : 4200)
  WCHAR m_buffer[0];
#pragma warning(default : 4200)

 public:  // ISerializable
  REFCLSID get_Class() throw() { return __uuidof(this); }
  void Serialize(IStream& stream) {
    size_t len = GetLength();
    PCWSTR str = GetBuffer();
    io::StreamWriteExact(&stream, &len, sizeof(len));
    io::StreamWriteExact(&stream, str, len * sizeof(WCHAR));
  }

 public:  // IString
  PCWSTR GetBuffer() throw() { return m_buffer; }
  size_t GetLength() throw() { return m_length; }

 private:
  String(PCWSTR str, size_t len) throw() {
    m_length = len;
    str::copy(m_buffer, str, len);
    m_buffer[len] = '\0';
  }
  String(size_t len) throw() { m_length = len; }
  String(const String&);
  String& operator=(const String&);

 private:
  static void* Allocate(size_t len) throw() { return ::malloc(sizeof(String) + (len + 1) * sizeof(WCHAR)); }

 public:
  /// @param src ���L���͏��n�����.
  static IString* NewHere(void* src, size_t len) throw() {
    ASSERT(src);
    ASSERT(len > 0);
    String* s = new (src) String(len);
    ASSERT(s->m_buffer[len] == L'\0');
    return s;
  }
  /// @param src ���L���͌Ăяo�����̂܂�.
  static IString* NewCopy(PCWSTR src, size_t len) throw() {
    ASSERT(len > 0);
    void* p = Allocate(len);
    return new (p) String(src, len);
  }
  ///
  static PWSTR NewRaw(IString** pp, size_t len) throw() {
    ASSERT(len > 0);
    void* p = Allocate(len);
    String* s = new (p) String(len);
    *pp = s;
    return s->m_buffer;
  }
  void __free__() throw() {
    this->~String();
    ::free(this);
  }
};

// static mew::FunctionFactory<mew::String, CreateString> creatable_String;

//==============================================================================

void CreateString(mew::REFINTF pp, IUnknown* arg) throw(...) {
  if (!arg) {
    *pp.pp = nullptr;
  } else if (mew::ref<IStream> stream = mew::cast(arg)) {
    size_t len;
    mew::io::StreamReadExact(stream, &len, sizeof(len));
    if (len == 0) {  // �����[���̕�����
      *pp.pp = nullptr;
    } else {
      mew::IString* obj;
      PWSTR buf = mew::String::NewRaw(&obj, len);
      mew::io::StreamReadExact(stream, buf, len * sizeof(WCHAR));
      buf[len] = L'\0';
      VERIFY_HRESULT(obj->QueryInterface(pp.iid, pp.pp));
      obj->Release();
    }
  } else {  // �R���X�g���N�^������stream�łȂ�
    throw mew::exceptions::ArgumentError(L"String�̈�����stream�łȂ�");
  }
}

AVESTA_EXPORT_FUNC(String)
}  // namespace mew

//==============================================================================
namespace {

static void FormatString(mew::IString** pp, PCWSTR format, size_t length, size_t argc, PCWSTR argv[]) throw() {
  ASSERT(pp);
  ASSERT(format);
  const size_t MAX_ARGS = 9;
  ASSERT(argc <= MAX_ARGS);
  size_t arglen[MAX_ARGS];
  mew::String::Buffer buf;

  size_t enough = length;
  for (size_t i = 0; i < argc; i++) {
    size_t len = mew::str::length(argv[i]);
    arglen[i] = len;
    enough += len;
  }
  buf.reserve(enough + 1);
  for (size_t i = 0; i < length;) {
    if (format[i] == L'$') {
      size_t index = (size_t)(format[i + 1] - L'1');
      ASSERT(index < argc);
      if (index < argc && argv[index]) {
        buf.append(argv[index], arglen[index]);
      }
      i += 2;
    } else {
      buf.push_back(format[i]);
      ++i;
    }
  }
  size_t buflen = buf.size();
  buf.push_back(L'\0');
  *pp = mew::String::NewHere(buf.detach(), buflen);
}
}  // namespace

//==============================================================================
namespace mew {
MEW_API void CreateString(mew::IString** pp, PCWSTR format, size_t length, size_t argc, PCWSTR argv[]) throw() {
  if (length == (size_t)-1) {
    length = mew::str::length(format);
  }
  if (length == 0) {
    *pp = nullptr;
  } else if (argc > 0) {
    FormatString(pp, format, length, argc, argv);
  } else {
    *pp = mew::String::NewCopy(format, length);
  }
}

MEW_API void CreateString(mew::IString** pp, UINT nID, HMODULE hModule, size_t argc, PCWSTR argv[]) throw() {
  const int BUFEXPAND = 256;
  int bufsize = BUFEXPAND, length;
  CHeapPtr<WCHAR> buffer;
  while (true) {
    buffer.Reallocate(bufsize);
    length = ::LoadStringW(hModule, nID, buffer, bufsize);
    if (length < bufsize - 1) {
      break;
    }
    bufsize += BUFEXPAND;
  }
  CreateString(pp, (PCWSTR)buffer, (size_t)length, argc, argv);
}
}  // namespace mew

void mew::StringReplace(IString** pp, IString* s, WCHAR from, WCHAR to) {
  if (!s) {
    *pp = null;
    return;
  }
  size_t len = s->GetLength();
  PCWSTR src = s->GetBuffer();
  PWSTR dst = String::NewRaw(pp, len);
  for (size_t i = 0; i < len; ++i) dst[i] = (src[i] == from ? to : src[i]);
  dst[len] = L'\0';
}

void mew::ObjectToString(IString** pp, IUnknown* obj) throw() {
  if (!obj) {  // �ʂ��
    CreateString(pp, L"null", string::npos, 0, null);
  } else if SUCCEEDED (objcpy(obj, pp)) {  // ok. already string
  }
  // else if(ref<IConvertToString> conv = cast(obj)) {
  //  conv->ConvertToString(pp);
  //}
  else {
    ref<IUnknown> unk;
    objcpy(obj, &unk);
    *pp = string::format(_T("object[$1]"), (DWORD)(IUnknown*)unk).detach();
  }
}

/*
UINT32 CStringHashFunc(const char * n)
{
    UINT32 h = 0, g;
    const unsigned char * name = (const unsigned char *) n;
    while (*name)
    {
        h = (h << 4) + *name++;
        if ((g = h & 0xF0000000) != 0) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

*/
