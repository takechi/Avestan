// main.cpp

#include "stdafx.h"
#include "std/str.hpp"

namespace {

inline PWSTR ParseTupleForSingleUnicode(PyObject* args) {
  PWSTR wcs = NULL;
  if (PyArg_ParseTuple(args, "u", &wcs)) {
    return wcs;
  } else {
    return NULL;
  }
}

inline PSTR ParseTupleForSingleString(PyObject* args) {
  PSTR str = NULL;
  if (PyArg_ParseTuple(args, "s", &str)) {
    return str;
  } else {
    return NULL;
  }
}

inline bool SetClipboard(HGLOBAL hMem, int format) {
  if (!::OpenClipboard(NULL)) {
    return false;
  }
  ::EmptyClipboard();
  ::SetClipboardData(format, hMem);
  ::CloseClipboard();
  return true;
}

template <typename Ch>
struct CF;
template <>
struct CF<char> {
  enum { value = CF_TEXT };
};
template <>
struct CF<wchar_t> {
  enum { value = CF_UNICODETEXT };
};

template <typename Ch>
static void CopyTextToClipboard(const Ch* text) {
  size_t len = mew::str::length(text);
  size_t sz = sizeof(Ch) * (len + 1);
  HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, sz);
  void* p = ::GlobalLock(hGlobal);
  memcpy(p, text, sz);
  ::GlobalUnlock(hGlobal);
  SetClipboard(hGlobal, CF<Ch>::value);
}

static PyObject* copy(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  if (PWSTR wcs = ParseTupleForSingleUnicode(args)) {
    CopyTextToClipboard(wcs);
    Py_RETURN_NONE;
  }
  // 失敗したので、Stringで取得してみる
  PyErr_Clear();
  if (PCSTR str = ParseTupleForSingleString(args)) {
    CopyTextToClipboard(str);
    Py_RETURN_NONE;
  }
  // だめぽ。
  return NULL;
}

static PyObject* paste(PyObject* self, PyObject* args) {
  if (!::OpenClipboard(NULL)) {
    return NULL;
  }
  mew::pygmy::StringW result;
  if (HGLOBAL hText = ::GetClipboardData(CF_UNICODETEXT)) {
    PCWSTR data = (PCWSTR)::GlobalLock(hText);
    result = mew::pygmy::StringW(data);
    ::GlobalUnlock(hText);
  }
  ::CloseClipboard();
  if (result) {
    return result.detach();
  }
  // だめぽ。
  return NULL;
}

static PyMethodDef functions[] = {{"copy", copy, METH_VARARGS}, {"paste", paste, METH_NOARGS}, {NULL, NULL}};
}  // namespace

extern "C" __declspec(dllexport) void APIENTRY initclipboard() {
  mew::pygmy::Module module("clipboard", functions, "clipboard");
}
