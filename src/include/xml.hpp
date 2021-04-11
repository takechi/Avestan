/// @file xml.hpp
/// XMLの読み書き.

#pragma once

#include "string.hpp"

namespace mew {
__interface IMessage;

/// XMLパーサラッパ.
namespace xml {
//==============================================================================
// インタフェース

__interface __declspec(uuid("A7FBAD61-1BF3-4A72-92EF-6B9D547CFD09")) IXMLReader;
__interface __declspec(uuid("B6090454-00DB-458A-BF81-0342EBF4C954")) IXMLWriter;

//==============================================================================
// 作成可能なクラス

class __declspec(uuid("98AA90C3-22DD-41EF-8AC4-931C95E02C6B")) XMLReader;
class __declspec(uuid("8294FEA9-2FE2-4508-95DD-DCF51B1F64D9")) XMLWriter;

//==============================================================================
// 関数.

/// XMLからmessageを読み込む.
message LoadMessage(IUnknown* src, IXMLReader* sax = null);
/// messageをXMLとして書き出す.
/// 基本POD型、メッセージ、文字列以外のオブジェクトを含んでいてはならない.
HRESULT SaveMessage(const message& msg, IUnknown* dst, IXMLWriter* sax = null);

//==============================================================================
// インタフェース定義.

/// 属性.
struct __declspec(novtable) XMLAttributes {
  virtual string operator[](PCWSTR name) throw() = 0;
  virtual string operator[](size_t index) throw() = 0;
  virtual size_t length() throw() = 0;
};

/// SAX2ハンドラ.
__interface IXMLHandler {
  HRESULT StartDocument();
  HRESULT EndDocument();
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes & attr);
  HRESULT EndElement(PCWSTR name, size_t cch);
  HRESULT Characters(PCWSTR chars, size_t cch);
  HRESULT ProcessingInstruction(PCWSTR wcsTarget, size_t cchTarget, PCWSTR wcsData, size_t cchData);
  HRESULT Error(UINT line, UINT column, PCWSTR msg);
  HRESULT FatalError(UINT line, UINT column, PCWSTR msg);
  HRESULT Warning(UINT line, UINT column, PCWSTR msg);
};

/// SAX2リーダ.
__interface IXMLReader : IUnknown {
  /// XMLを読み込む.
  HRESULT Parse(IXMLHandler * handler, IUnknown * source);
};

/// IXMLHandlerの空の実装.
class XMLHandlerImpl : public IXMLHandler {
 public:
  HRESULT StartDocument() { return S_OK; }
  HRESULT EndDocument() { return S_OK; }
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) { return S_OK; }
  HRESULT EndElement(PCWSTR name, size_t cch) { return S_OK; }
  HRESULT Characters(PCWSTR chars, size_t cch) { return S_OK; }
  HRESULT ProcessingInstruction(PCWSTR wcsTarget, size_t cchTarget, PCWSTR wcsData, size_t cchData) { return S_OK; }
  HRESULT Error(UINT line, UINT column, PCWSTR msg) { return E_FAIL; }
  HRESULT FatalError(UINT line, UINT column, PCWSTR msg) { return E_FAIL; }
  HRESULT Warning(UINT line, UINT column, PCWSTR msg) { return E_FAIL; }
};

/// シンプルなXML出力機構.
__interface IXMLWriter : IUnknown {
  void StartDocument(IStream * stream, PCWSTR encoding = L"Shift_JIS");
  void EndDocument();
  void StartElement(PCWSTR name);
  void EndElement();
  void Attribute(PCWSTR name, PCWSTR data);
  void Characters(PCWSTR chars);
  void ProcessingInstruction(PCWSTR target, PCWSTR data);
};
}  // namespace xml
}  // namespace mew
