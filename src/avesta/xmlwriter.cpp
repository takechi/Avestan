// xmlwriter.cpp

#include "stdafx.h"
#include "../mew/private.h"
#include "xml.hpp"

mew::Stream& operator<<(mew::Stream& stream, const char* v) {
  stream.write(v, strlen(v));
  return stream;
}

mew::Stream& operator<<(mew::Stream& stream, const wchar_t* v) {
  ATL::CW2A w(v);
  stream.write(w, strlen(w));
  return stream;
}

mew::Stream& operator<<(mew::Stream& stream, char c) {
  stream.write(&c, 1);
  return stream;
}

//==============================================================================
// XMLWriter.

namespace mew {
namespace xml {

class XMLWriter : public Root<implements<IXMLWriter> > {
 private:
  Stream m_stream;
  std::vector<string> m_stack;
  bool m_isTagClosed;
  bool m_hasChild;

 public:
  void __init__(IUnknown* arg) { ASSERT(!arg); }
  void Dispose() {
    if (!m_stack.empty()) {
      TRACE(_T("warning: �^�O�������Ȃ��܂� XMLWriter ���j������܂���"));
      EndDocument();
    }
    m_stack.clear();
    m_stream.clear();
  }

 public:  // IXMLWriter
  void StartDocument(IStream* stream, PCWSTR encoding) {
    if (m_stream) {
      throw mew::exceptions::ArgumentError(_T("���ɊJ����Ă��܂�"), E_UNEXPECTED);
    }
    m_isTagClosed = true;
    m_hasChild = false;
    m_stream = stream;
    m_stream << "<?xml version=\"1.0\" encoding=\"" << encoding << "\"?>\r\n";
  }
  void EndDocument() {
#if _DEBUG
    if (!m_stack.empty()) {
      TRACE(_T("warning: �^�O�������Ȃ��܂� XMLWriter.EndDocument() ����܂���"));
    }
#endif
    while (!m_stack.empty()) {
      EndElement();
    }
    m_stream.clear();
  }
  void StartElement(PCWSTR name) {
    if (!m_isTagClosed) {
      m_stream << ">\r\n";
    }
    Indent(m_stack.size());
    m_stream << '<' << name;
    m_stack.push_back(string(name));
    m_isTagClosed = false;
    m_hasChild = false;
  }
  void EndElement() {
    if (m_stack.empty()) {
      throw mew::exceptions::ArgumentError(_T("EndElement()�̌Ăяo�����AStartElement()�ƈ�v���܂���"), E_UNEXPECTED);
    }
    if (!m_isTagClosed) {
      m_stream << "/>\r\n";
      m_isTagClosed = true;
    } else {
      if (m_hasChild) {
        Indent(m_stack.size() - 1);
      }
      m_stream << "</" << m_stack.back().str() << ">\r\n";
    }
    m_stack.pop_back();
    m_hasChild = true;
  }
  void Attribute(PCWSTR name, PCWSTR data) {
    if (m_isTagClosed) {
      throw mew::exceptions::ArgumentError(_T("�����̓e�L�X�g����Ɏw�肷��K�v������܂�"), E_UNEXPECTED);
    }
    m_stream << ' ' << name << "=\"" << data << '\"';
  }
  void Characters(PCWSTR chars) {
    if (!chars) {
      chars = L"";
    }
    if (!m_isTagClosed) {
      m_stream << '>';
      m_isTagClosed = true;
    }
    m_stream << chars;
  }
  void ProcessingInstruction(PCWSTR target, PCWSTR data) {
    if (!m_stream) {
      throw mew::exceptions::ArgumentError(_T("xml�o�͐悪�w�肳���ȑO�� ProcessingInstruction ���Ăяo����܂���"),
                                           E_UNEXPECTED);
    }
    if (!m_stack.empty()) {
      throw mew::exceptions::ArgumentError(_T("ProcessingInstruction �̓��[�g�m�[�h����ɐ錾����K�v������܂�"),
                                           E_UNEXPECTED);
    }
    m_stream << "<?" << target << ' ' << data << "?>\r\n";
  }

 private:
  void Indent(size_t size) {
    for (size_t i = 0; i < size; i++) {
      m_stream << '\t';
    }
  }
};

AVESTA_EXPORT(XMLWriter)

}  // namespace xml
}  // namespace mew
