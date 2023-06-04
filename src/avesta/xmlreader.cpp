// xmlreader.cpp

#include "stdafx.h"
#import <msxml6.dll> raw_interfaces_only
#include "../mew/private.h"
#include "xml.hpp"

// XMLReader.

namespace {

// wchar_t* を unsigned short* としてインポートしてしまうので、
// USHORT* ⇔ PWSTR のキャストを多用することになる。
using XMLSTR = unsigned short*;
STATIC_ASSERT(sizeof(unsigned short) == sizeof(wchar_t));

class AttributesImpl : public mew::xml::XMLAttributes {
 private:
  MSXML2::ISAXAttributes* const m_pAttr;

 public:
  AttributesImpl(MSXML2::ISAXAttributes* attr) : m_pAttr(attr) { ASSERT(m_pAttr); }
  virtual mew::string operator[](PCWSTR name) throw() {
    static XMLSTR EMPTY_ATTR = (XMLSTR)(L"");
    PCWSTR val;
    INT len;
    if SUCCEEDED (m_pAttr->getValueFromName(EMPTY_ATTR, 0, (XMLSTR)name, (int)mew::str::length(name), (XMLSTR*)&val, &len)) {
      return mew::string(val, len);
    } else {
      return mew::string();
    }
  }
  virtual mew::string operator[](size_t index) throw() {
    PCWSTR val;
    INT len;
    if SUCCEEDED (m_pAttr->getValue(index, (XMLSTR*)&val, &len)) {
      return mew::string(val, len);
    } else {
      return mew::string();
    }
  }
  virtual size_t length() throw() {
    INT len = 0;
    m_pAttr->getLength(&len);
    return len;
  }
};

// MSXML4 では、nullターミネートされていないことに注意！

#define TO_STRING(chars, cch) (PCWSTR) chars, cch
#define TO_STRING0(chars) (PCWSTR) chars

class SAXHandlerAdapter : public MSXML2::ISAXContentHandler, public MSXML2::ISAXErrorHandler {
 public:
  mew::xml::IXMLHandler* m_pHandler;

  STDMETHODIMP_(ULONG) AddRef() { return 1; }
  STDMETHODIMP_(ULONG) Release() { return 1; }
  STDMETHODIMP QueryInterface(REFIID iid, void** ppObj) {
    if (iid == __uuidof(MSXML2::ISAXContentHandler) || iid == __uuidof(IUnknown)) {
      *ppObj = static_cast<MSXML2::ISAXContentHandler*>(this);
      return S_OK;
    } else if (iid == __uuidof(MSXML2::ISAXErrorHandler)) {
      *ppObj = static_cast<MSXML2::ISAXErrorHandler*>(this);
      return S_OK;
    }
    *ppObj = nullptr;
    return E_NOINTERFACE;
  }

  STDMETHODIMP putDocumentLocator(MSXML2::ISAXLocator* pLocator) { return S_OK; }

  STDMETHODIMP startDocument() { return m_pHandler->StartDocument(); }

  STDMETHODIMP endDocument() { return m_pHandler->EndDocument(); }

  STDMETHODIMP startPrefixMapping(XMLSTR pwchPrefix, int cchPrefix, XMLSTR pwchUri, int cchUri) { return S_OK; }

  STDMETHODIMP endPrefixMapping(XMLSTR pwchPrefix, int cchPrefix) { return S_OK; }

  STDMETHODIMP startElement(XMLSTR pwchNamespaceUri, int cchNamespaceUri, XMLSTR pwchLocalName, int cchLocalName,
                            XMLSTR pwchRawName, int cchRawName, MSXML2::ISAXAttributes* pAttributes) {
    AttributesImpl attr(pAttributes);
    return m_pHandler->StartElement(TO_STRING(pwchRawName, cchRawName), attr);
  }

  STDMETHODIMP endElement(XMLSTR pwchNamespaceUri, int cchNamespaceUri, XMLSTR pwchLocalName, int cchLocalName,
                          XMLSTR pwchRawName, int cchRawName) {
    return m_pHandler->EndElement(TO_STRING(pwchRawName, cchRawName));
  }

  STDMETHODIMP characters(XMLSTR pwchChars, int cchChars) { return m_pHandler->Characters(TO_STRING(pwchChars, cchChars)); }

  STDMETHODIMP ignorableWhitespace(XMLSTR pwchChars, int cchChars) { return S_OK; }

  STDMETHODIMP processingInstruction(XMLSTR pwchTarget, int cchTarget, XMLSTR pwchData, int cchData) {
    return m_pHandler->ProcessingInstruction(TO_STRING(pwchTarget, cchTarget), TO_STRING(pwchData, cchData));
  }

  STDMETHODIMP skippedEntity(XMLSTR pwchName, int cchName) { return S_OK; }

  STDMETHODIMP error(MSXML2::ISAXLocator* pLocator, XMLSTR pwchErrorMessage, HRESULT errCode) {
    int line, column;
    pLocator->getLineNumber(&line);
    pLocator->getColumnNumber(&column);
    return m_pHandler->Error(line, column, TO_STRING0(pwchErrorMessage));
  }

  STDMETHODIMP fatalError(MSXML2::ISAXLocator* pLocator, XMLSTR pwchErrorMessage, HRESULT errCode) {
    int line, column;
    pLocator->getLineNumber(&line);
    pLocator->getColumnNumber(&column);
    return m_pHandler->FatalError(line, column, TO_STRING0(pwchErrorMessage));
  }

  STDMETHODIMP ignorableWarning(MSXML2::ISAXLocator* pLocator, XMLSTR pwchErrorMessage, HRESULT errCode) {
    int line, column;
    pLocator->getLineNumber(&line);
    pLocator->getColumnNumber(&column);
    return m_pHandler->Warning(line, column, TO_STRING0(pwchErrorMessage));
  }
};

// GUIDs
class __declspec(uuid("079aa557-4a18-424a-8eee-e39f0a8d41b9")) SAXXMLReader;
class __declspec(uuid("3124c396-fb13-4836-a6ad-1317f1713688")) SAXXMLReader30;
class __declspec(uuid("7c6e29bc-8b8b-4c3d-859e-af6cd158be0f")) SAXXMLReader40;
}  // namespace

//==============================================================================

namespace mew {
namespace xml {

class XMLReader : public Root<implements<IXMLReader> > {
 private:
  ref<MSXML2::ISAXXMLReader> m_pNativeReader;
  SAXHandlerAdapter m_Adapter;

 public:  // Object
  void __init__(IUnknown* arg) {
    ASSERT(!arg);
    if SUCCEEDED (::CoCreateInstance(__uuidof(SAXXMLReader40), null, CLSCTX_INPROC, __uuidof(MSXML2::ISAXXMLReader),
                                     (void**)&m_pNativeReader)) {
      TRACE(L"info: XMLReader using MSXML4");
    } else {
      m_pNativeReader.create(__uuidof(SAXXMLReader));
      TRACE(L"info: XMLReader using MSXML3");
    }
    m_pNativeReader->putContentHandler(&m_Adapter);
    m_pNativeReader->putErrorHandler(&m_Adapter);
  }
  void Dispose() { m_pNativeReader.clear(); }

 public:  // IXMLReader
  HRESULT Parse(IXMLHandler* handler, IUnknown* pSource) {
    if (!handler) {
      ASSERT(!"ぬるぽ");
      return E_POINTER;
    }
    m_Adapter.m_pHandler = handler;
    if (string xmlfile = cast(pSource))
      return m_pNativeReader->parseURL((XMLSTR)xmlfile.str());
    else
      return m_pNativeReader->parse(ATL::CComVariant(pSource));
  }
};

AVESTA_EXPORT(XMLReader)
}  // namespace xml
}  // namespace mew
