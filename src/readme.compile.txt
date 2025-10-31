コンパイルに必要なもの：
・Visual Studio.NET 2003 以降（方言とテンプレートの部分特殊化が必要なため）
・Python2.4 以降
・WTL7.5 以降

以下を書き換える必要があるかもしれません。

<pyconfig.h>
#define PY_UNICODE_TYPE unsigned short
↓
#define PY_UNICODE_TYPE wchar_t
