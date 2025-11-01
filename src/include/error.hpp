/// @file error.hpp
/// 例外.
#pragma once

#include "mew.hpp"
#include "string.hpp"

namespace mew {
/// 例外.
namespace exceptions {
/// 例外基底クラス.
class Error {
 public:
  virtual ~Error() {}

#ifndef DOXYGEN
  virtual string get_Message() noexcept = 0;
  virtual HRESULT get_Code() noexcept = 0;
#endif  // DOXYGEN

  /// 例外メッセージ [get].
  __declspec(property(get = get_Message)) string Message;
  /// 例外コード [get].
  __declspec(property(get = get_Code)) HRESULT Code;
};

/// システムエラー.
class SystemError : public Error {
 protected:
  string m_Message;
  HRESULT m_Code;

 public:
  SystemError(const string& msg, HRESULT code) : m_Message(msg), m_Code(code) {}

#ifndef DOXYGEN
  virtual string get_Message() noexcept { return m_Message; }
  virtual HRESULT get_Code() noexcept { return m_Code; }
#endif  // DOXYGEN
};

/// ロジックエラー.
class LogicError : public SystemError {
 public:
  LogicError(const string& msg, HRESULT code) : SystemError(msg, code) {}
};

/// ランタイムエラー.
class RuntimeError : public SystemError {
 public:
  RuntimeError(const string& msg, HRESULT code) : SystemError(msg, code) {}
};

/// 不正なメソッド呼び出し.
class ArgumentError : public LogicError {
 public:
  ArgumentError(const string& msg, HRESULT code = E_INVALIDARG) : LogicError(msg, code) {}
};

/// キャストエラー.
class CastError : public RuntimeError {
 public:
  CastError(const string& msg, HRESULT code = E_NOINTERFACE) : RuntimeError(msg, code) {}
};

/// インスタンス作成エラー.
class ClassError : public RuntimeError {
 public:
  ClassError(const string& msg, HRESULT code) : RuntimeError(msg, code) {}
};

/// IOエラー.
class IOError : public RuntimeError {
 public:
  IOError(const string& msg, HRESULT code) : RuntimeError(msg, code) {}
};
}  // namespace exceptions
}  // namespace mew
