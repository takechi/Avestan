// main.cpp

#include "stdafx.h"
#include "../server/main.hpp"
#include "object.hpp"
#include "pygmy.hpp"
#include "std/buffer.hpp"

namespace {
static string FunctionError(PCWSTR fn) {
  pygmy::ErrorInfo error;
  pygmy::fetch_error(error);
  return string::format(L"ScriptError: $1 の呼び出しでエラーが発生しました。（$2: $3）", fn,
                        pygmy::String(error.exception).str(), pygmy::String(error.value).str());
}
static string FunctionNotFound(PCWSTR fn) { return string::format(L"ScriptError: $1 が見つかりません。", fn); }
inline pygmy::Object StringOrNone(IString* str) {
  if (str)
    return pygmy::StringW(str->GetBuffer());
  else
    return pygmy::None();
}
inline pygmy::Object PathOrNone(IEntry* entry) {
  if (!entry)
    return pygmy::None();
  else
    return StringOrNone(entry->Path);
}
}  // namespace

//==============================================================================

namespace StatusTextParam {
ref<IShellListView> view;
}

//==============================================================================

class PythonCallback : public Root<implements<ICallback, IDisposable> > {
 private:
  pygmy::Host m_host;
  pygmy::Object m_fnCaption;
  pygmy::Object m_fnStatusText;
  pygmy::Object m_fnGestureText;
  pygmy::Object m_fnNavigateVerb;
  pygmy::Object m_fnExecuteVerb;
  pygmy::Object m_fnWallPaper;

 public:
  void __init__(IUnknown* arg) {
    m_host.Initialize();
    pygmy::Module callback = pygmy::Module::import("callback");
    m_fnCaption = callback["Caption"];
    m_fnStatusText = callback["StatusText"];
    m_fnGestureText = callback["GestureText"];
    m_fnNavigateVerb = callback["NavigateVerb"];
    m_fnExecuteVerb = callback["ExecuteVerb"];
    m_fnWallPaper = callback["WallPaper"];
  }
  void Dispose() throw() { m_host.Terminate(); }

 public:  // ICallback
  string Caption(const string& name, const string& path) {
    PCWSTR fn = L"Caption()";
    if (m_fnCaption) {
      PyErr_Clear();
      if (pygmy::String result = m_fnCaption(StringOrNone(name), StringOrNone(path))) return result.str();
      return FunctionError(fn);
    }
    return FunctionNotFound(fn);
  }
  string StatusText(const string& text, IShellListView* view) {
    PCWSTR fn = L"StatusText()";
    ASSERT(!StatusTextParam::view);
    if (m_fnStatusText && !StatusTextParam::view) {
      StatusTextParam::view = view;

      ref<IEntry> folder;
      view->GetFolder(&folder);

      PyErr_Clear();
      pygmy::String statustext = m_fnStatusText(StringOrNone(text), StringOrNone(folder->Name), PathOrNone(folder));
      StatusTextParam::view.clear();
      if (statustext)
        return string(statustext.str());
      else
        return FunctionError(fn);
    }
    return FunctionNotFound(fn);
  }
  string GestureText(const Gesture gesture[], size_t length, const string& description) {
    PCWSTR fn = L"GestureText()";
    if (m_fnGestureText) {
      pygmy::List list(length);
      for (size_t i = 0; i < length; ++i) list[i] = pygmy::Int(gesture[i]);
      PyErr_Clear();
      if (pygmy::String result = m_fnGestureText(list, StringOrNone(description))) {
        return result.str();
      }
      return FunctionError(fn);
    }
    return FunctionNotFound(fn);
  }
  void ExecuteFolder(PCWSTR exe, IEntry* where) {
    string path = where->Path;
    WCHAR dir[MAX_PATH];
    path.copyto(dir, MAX_PATH);
    PathRemoveFileSpec(dir);
    ::ShellExecute(null, null, exe, path.str(), dir, SW_SHOWNORMAL);
  }
  Navigation NavigateVerb(IEntry* current, IEntry* where, UINT mods, bool locked, Navigation defaultVerb) {
    if (m_fnNavigateVerb) {
      if (IsKeyPressed(VK_APPS)) mods |= MouseButtonRight;
      PyErr_Clear();
      pygmy::Int result = m_fnNavigateVerb(PathOrNone(current), PathOrNone(where), pygmy::Int(mods), pygmy::Bool(locked),
                                           pygmy::Int(defaultVerb));
      if ((PyObject*)result) {
        return (Navigation)result.value();
      }
      ASSERT(!"NavigateVerb");
      TRACE(FunctionError(L"NavigateVerb"));
    }
    // error
    return defaultVerb;
  }
  string ExecuteVerb(IEntry* current, IEntry* what, UINT mods) {
    if (m_fnExecuteVerb) {
      PyErr_Clear();
      if (pygmy::String result = m_fnExecuteVerb(PathOrNone(current), PathOrNone(what), pygmy::Int(mods))) {
        return result.str();
      }
      ASSERT(!"ExecuteVerb");
      TRACE(FunctionError(L"ExecuteVerb"));
    }
    return null;
  }
  string WallPaper(const string& filename, const string& name, const string& path) {
    if (m_fnWallPaper) {
      PyErr_Clear();
      if (pygmy::String result = m_fnWallPaper(StringOrNone(filename), StringOrNone(name), StringOrNone(path)))
        return result.str();
    }
    return filename;
  }
};

AVESTA_EXPORT(PythonCallback)

//==============================================================================

namespace {
inline PWSTR ParseTupleForStringW(PyObject* args) {
  PWSTR wcs = null;
  if (PyArg_ParseTuple(args, "u", &wcs))
    return wcs;
  else
    return null;
}

inline PSTR ParseTupleForStringA(PyObject* args) {
  PSTR str = null;
  if (PyArg_ParseTuple(args, "s", &str))
    return str;
  else
    return null;
}

inline PWSTR ParseTupleForStringW(PyObject* args, PWSTR* option) {
  *option = null;
  PWSTR wcs = null;
  if (PyArg_ParseTuple(args, "u|u", &wcs, option))
    return wcs;
  else
    return null;
}

inline PSTR ParseTupleForStringA(PyObject* args, PSTR* option) {
  *option = null;
  PSTR str = null;
  if (PyArg_ParseTuple(args, "s|s", &str, option))
    return str;
  else
    return null;
}

// def Execute(path, ...)
static PyObject* Execute(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  {
    PWSTR param = null;
    if (PWSTR wcs = ParseTupleForStringW(args, &param)) {
      ShellExecuteW(null, null, wcs, param, null, SW_SHOWNORMAL);
      Py_RETURN_NONE;
    }
  }
  PyErr_Clear();
  // 失敗したので、stringで取得してみる
  {
    PSTR param = null;
    if (PCSTR str = ParseTupleForStringA(args, &param)) {
      ShellExecuteA(null, null, str, param, null, SW_SHOWNORMAL);
      Py_RETURN_NONE;
    }
  }
  return Py_None;
}

// def GetFreeBytes(path)
static PyObject* GetFreeBytes(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  if (PWSTR wcs = ParseTupleForStringW(args)) {
    UINT64 uFreeBytes = 0;
    ::GetDiskFreeSpaceExW(wcs, (ULARGE_INTEGER*)&uFreeBytes, null, null);
    return pygmy::Long(uFreeBytes).detach();
  }
  // 失敗したので、stringで取得してみる
  PyErr_Clear();
  if (PCSTR str = ParseTupleForStringA(args)) {
    UINT64 uFreeBytes = 0;
    ::GetDiskFreeSpaceExA(str, (ULARGE_INTEGER*)&uFreeBytes, null, null);
    return pygmy::Long(uFreeBytes).detach();
  }
  // だめぽ。
  return Py_None;
}

// def GetDriveLetter(path)
static PyObject* GetDriveLetter(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  if (PWSTR wcs = ParseTupleForStringW(args)) {
    WCHAR buffer[MAX_PATH];
    ave::GetDriveLetter(wcs, buffer);
    return PyUnicode_FromWideChar(buffer, wcslen(buffer));
  }
  // 失敗したので、stringで取得してみる
  PyErr_Clear();
  if (PCSTR str = ParseTupleForStringA(args)) {
    WCHAR buffer[MAX_PATH];
    ave::GetDriveLetter(ATL::CA2W(str), buffer);
    return PyUnicode_FromWideChar(buffer, wcslen(buffer));
  }
  // だめぽ。
  return Py_None;
}

// def GetTotalCount()
static PyObject* GetTotalCount(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) return Py_None;
  return pygmy::Int(StatusTextParam::view->Count).detach();
}

// def GetTotalBytes()
static PyObject* GetTotalBytes(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) return Py_None;
  return pygmy::Long(ave::GetTotalBytes(StatusTextParam::view)).detach();
}

// def GetSelectedCount()
static PyObject* GetSelectedCount(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) return Py_None;
  return pygmy::Int(StatusTextParam::view->SelectedCount).detach();
}

// def GetSelectedBytes()
static PyObject* GetSelectedBytes(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) return Py_None;
  return pygmy::Long(ave::GetSelectedBytes(StatusTextParam::view)).detach();
}

// def GetDriveLetter(path)
static PyObject* MyMessageBox(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  if (PWSTR wcs = ParseTupleForStringW(args)) {
    ::MessageBoxW(::GetActiveWindow(), wcs, L"pygmy", MB_OK);
    Py_RETURN_NONE;
  }
  // 失敗したので、stringで取得してみる
  PyErr_Clear();
  if (PCSTR str = ParseTupleForStringA(args)) {
    ::MessageBoxA(::GetActiveWindow(), str, "pygmy", MB_OK);
    Py_RETURN_NONE;
  }
  // だめぽ。
  Py_RETURN_NONE;
}

static PyMethodDef pygmy_functions[] = {{"Execute", Execute, METH_VARARGS},
                                        {"GetFreeBytes", GetFreeBytes, METH_VARARGS},
                                        {"GetDriveLetter", GetDriveLetter, METH_VARARGS},
                                        {"GetTotalCount", GetTotalCount, METH_NOARGS},
                                        {"GetTotalBytes", GetTotalBytes, METH_NOARGS},
                                        {"GetSelectedCount", GetSelectedCount, METH_NOARGS},
                                        {"GetSelectedBytes", GetSelectedBytes, METH_NOARGS},
                                        {"MessageBox", MyMessageBox, METH_VARARGS},
                                        {NULL, NULL}};
}  // namespace

extern "C" __declspec(dllexport) void APIENTRY initpygmy() {
  pygmy::Module module("pygmy", pygmy_functions, "Avesta extension");
  //
  module.add("LEFT", MouseButtonLeft);
  module.add("RIGHT", MouseButtonRight);
  module.add("MIDDLE", MouseButtonMiddle);
  module.add("X1", MouseButtonX1);
  module.add("X2", MouseButtonX2);
  module.add("CONTROL", ModifierControl);
  module.add("SHIFT", ModifierShift);
  module.add("ALT", ModifierAlt);
  module.add("WINDOWS", ModifierWindows);
  //
  module.add("CANCEL", NaviCancel);
  module.add("GOTO", NaviGoto);
  module.add("GOTO_ALWAYS", NaviGotoAlways);
  module.add("OPEN", NaviOpen);
  module.add("OPEN_ALWAYS", NaviOpenAlways);
  module.add("APPEND", NaviAppend);
  module.add("RESERVE", NaviReserve);
  module.add("SWITCH", NaviSwitch);
  module.add("REPLACE", NaviReplace);
}

static struct PyModuleDef moduledef = {PyModuleDef_HEAD_INIT, "pygmy", NULL, -1, pygmy_functions, NULL, NULL, NULL, NULL};

PyMODINIT_FUNC PyInit_pygmy() {
  pygmy::Module module;
  module.attach(PyModule_Create(&moduledef));
  module.add("LEFT", MouseButtonLeft);
  module.add("RIGHT", MouseButtonRight);
  module.add("MIDDLE", MouseButtonMiddle);
  module.add("X1", MouseButtonX1);
  module.add("X2", MouseButtonX2);
  module.add("CONTROL", ModifierControl);
  module.add("SHIFT", ModifierShift);
  module.add("ALT", ModifierAlt);
  module.add("WINDOWS", ModifierWindows);
  //
  module.add("CANCEL", NaviCancel);
  module.add("GOTO", NaviGoto);
  module.add("GOTO_ALWAYS", NaviGotoAlways);
  module.add("OPEN", NaviOpen);
  module.add("OPEN_ALWAYS", NaviOpenAlways);
  module.add("APPEND", NaviAppend);
  module.add("RESERVE", NaviReserve);
  module.add("SWITCH", NaviSwitch);
  module.add("REPLACE", NaviReplace);
  return module;
}