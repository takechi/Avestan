// signal.hpp
#pragma once

#include "message.hpp"

namespace mew {
//==============================================================================
// インタフェース

__interface __declspec(uuid("91E42850-31CF-4B5E-A731-E24D46618E68")) ISignal;
__interface __declspec(uuid("55A305E9-3FAC-4DC1-B992-F25B33D699DE")) IMessenger;

//==============================================================================
// 作成可能なクラス

/// ISignalの機能をすべて実装したヘルパ.
/// 集約によりISignalの実装に利用できる.
class __declspec(uuid("9E76A64B-99FA-4F4C-A4BC-ED8EC3CFC31E")) Messenger;

/// メッセージソース.
__interface ISignal : IDisposable {
  /// シンクを登録する.
  /// シンクとメッセージコードが共に等しいシンクが登録済みの場合は、関連付けられたメッセージが上書きされる.
  /// @retval S_OK         登録成功.
  /// @retval E_INVALIDARG handlerが不正、またはcodeをサポートしていない.
  HRESULT Connect(EventCode code,  ///< 登録するメッセージコード. サポートしていないメッセージコードの場合は呼び出しが失敗する.
                  function fn,     ///< 登録するシンク. nullの場合は呼び出しが失敗する.
                  message msg = null  ///< 送信されるメッセージ.
                  ) throw();
  /// シンクを登録解除する.
  /// @result 削除されたシンクの個数.
  size_t Disconnect(EventCode code,  ///< 削除するメッセージコード. 0の場合は全てのコードが削除される.
                    function fn,     ///< 削除するシンク. nullの場合は全ての関数が削除される.
                    IUnknown* obj = null  ///< 削除するシンクオブジェクト. fnがnull以外の場合は無視される.
                    ) throw();
};

/// メッセンジャー.
__interface IMessenger : ISignal {
  /// メッセージの配信に使用される関数オブジェクトを返す.
  function Invoke(EventCode code) throw();
};

///
template <int code>
struct Event {
  template <class T>
  static void event(message& msg, T* from) {
    msg["from"] = from;
  }
};

/// メッセージソース実装のためのラッパ.
template <class TBase>
class __declspec(novtable) SignalImpl : public TBase {
 protected:
  ref<IMessenger> m_msgr;

 protected:  // invoke
#define MEW_PP_MSGR_FMT(n)                      \
  template <int code PP_TYPENAMES_CAT(n)>       \
  message InvokeEvent(PP_ARGS_CONST(n)) const { \
    if (!m_msgr) return null;                   \
    function fn = m_msgr->Invoke(code);         \
    if (!fn) return null;                       \
    message msg;                                \
    Event<code>::event(msg, PP_ARG_VALUES(n));  \
    fn(msg);                                    \
    return msg;                                 \
  }

  PP_REPEAT(10, MEW_PP_MSGR_FMT)

#undef MEW_PP_MSGR_FMT

 public:
  HRESULT Connect(EventCode code, function fn, message msg = null) throw() {
    return !m_msgr ? E_UNEXPECTED : m_msgr->Connect(code, fn, msg);
  }
  size_t Disconnect(EventCode code, function fn, IUnknown* obj = null) throw() {
    return !m_msgr ? 0 : m_msgr->Disconnect(code, fn, obj);
  }
};
}  // namespace mew
