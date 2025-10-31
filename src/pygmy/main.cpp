// main.cpp

#include "stdafx.h"
#include "../server/main.hpp"
#include "object.hpp"
#include "pygmy.hpp"
#include "std/buffer.hpp"

namespace {
static mew::string FunctionError(PCWSTR fn) {
  mew::pygmy::ErrorInfo error;
  mew::pygmy::fetch_error(error);
  return mew::string::format(L"ScriptError: $1 の呼び出しでエラーが発生しました。（$2: $3）", fn,
                             mew::pygmy::String(error.exception).str(), mew::pygmy::String(error.value).str());
}
static mew::string FunctionNotFound(PCWSTR fn) { return mew::string::format(L"ScriptError: $1 が見つかりません。", fn); }
inline mew::pygmy::Object StringOrNone(mew::IString* str) {
  if (str) {
    return mew::pygmy::StringW(str->GetBuffer());
  } else {
    return mew::pygmy::None();
  }
}
inline mew::pygmy::Object PathOrNone(mew::io::IEntry* entry) {
  if (!entry) {
    return mew::pygmy::None();
  } else {
    return StringOrNone(entry->Path);
  }
}
}  // namespace

//==============================================================================

namespace StatusTextParam {
mew::ref<mew::ui::IShellListView> view;
}

//==============================================================================

class PythonCallback : public mew::Root<mew::implements<ICallback, mew::IDisposable> > {
 private:
  mew::pygmy::Host m_host;
  mew::pygmy::Object m_fnCaption;
  mew::pygmy::Object m_fnStatusText;
  mew::pygmy::Object m_fnGestureText;
  mew::pygmy::Object m_fnNavigateVerb;
  mew::pygmy::Object m_fnExecuteVerb;
  mew::pygmy::Object m_fnWallPaper;

 public:
  void __init__(IUnknown* arg) {
    m_host.Initialize();
    mew::pygmy::Module callback = mew::pygmy::Module::import("callback");
    m_fnCaption = callback["Caption"];
    m_fnStatusText = callback["StatusText"];
    m_fnGestureText = callback["GestureText"];
    m_fnNavigateVerb = callback["NavigateVerb"];
    m_fnExecuteVerb = callback["ExecuteVerb"];
    m_fnWallPaper = callback["WallPaper"];
  }
  void Dispose() throw() { m_host.Terminate(); }

 public:  // ICallback
  mew::string Caption(const mew::string& name, const mew::string& path) {
    PCWSTR fn = L"Caption()";
    if (m_fnCaption) {
      PyErr_Clear();
      if (mew::pygmy::String result = m_fnCaption(StringOrNone(name), StringOrNone(path))) {
        return result.str();
      }
      return FunctionError(fn);
    }
    return FunctionNotFound(fn);
  }
  mew::string StatusText(const mew::string& text, mew::ui::IShellListView* view) {
    PCWSTR fn = L"StatusText()";
    ASSERT(!StatusTextParam::view);
    if (m_fnStatusText && !StatusTextParam::view) {
      StatusTextParam::view = view;

      mew::ref<mew::io::IEntry> folder;
      view->GetFolder(&folder);

      PyErr_Clear();
      mew::pygmy::String statustext = m_fnStatusText(StringOrNone(text), StringOrNone(folder->Name), PathOrNone(folder));
      StatusTextParam::view.clear();
      if (statustext) {
        return mew::string(statustext.str());
      } else {
        return FunctionError(fn);
      }
    }
    return FunctionNotFound(fn);
  }
  mew::string GestureText(const mew::ui::Gesture gesture[], size_t length, const mew::string& description) {
    PCWSTR fn = L"GestureText()";
    if (m_fnGestureText) {
      mew::pygmy::List list(length);
      for (size_t i = 0; i < length; ++i) {
        list[i] = mew::pygmy::Int(gesture[i]);
      }
      PyErr_Clear();
      if (mew::pygmy::String result = m_fnGestureText(list, StringOrNone(description))) {
        return result.str();
      }
      return FunctionError(fn);
    }
    return FunctionNotFound(fn);
  }
  void ExecuteFolder(PCWSTR exe, mew::io::IEntry* where) {
    mew::string path = where->Path;
    WCHAR dir[MAX_PATH];
    path.copyto(dir, MAX_PATH);
    PathRemoveFileSpec(dir);
    ::ShellExecute(nullptr, nullptr, exe, path.str(), dir, SW_SHOWNORMAL);
  }
  avesta::Navigation NavigateVerb(mew::io::IEntry* current, mew::io::IEntry* where, UINT mods, bool locked,
                                  avesta::Navigation defaultVerb) {
    if (m_fnNavigateVerb) {
      if (mew::ui::IsKeyPressed(VK_APPS)) {
        mods |= mew::ui::MouseButtonRight;
      }
      PyErr_Clear();
      mew::pygmy::Int result = m_fnNavigateVerb(PathOrNone(current), PathOrNone(where), mew::pygmy::Int(mods),
                                                mew::pygmy::Bool(locked), mew::pygmy::Int(defaultVerb));
      if ((PyObject*)result) {
        return (avesta::Navigation)result.value();
      }
      ASSERT(!"NavigateVerb");
      TRACE(FunctionError(L"NavigateVerb"));
    }
    // error
    return defaultVerb;
  }
  mew::string ExecuteVerb(mew::io::IEntry* current, mew::io::IEntry* what, UINT mods) {
    if (m_fnExecuteVerb) {
      PyErr_Clear();
      if (mew::pygmy::String result = m_fnExecuteVerb(PathOrNone(current), PathOrNone(what), mew::pygmy::Int(mods))) {
        return result.str();
      }
      ASSERT(!"ExecuteVerb");
      TRACE(FunctionError(L"ExecuteVerb"));
    }
    return mew::null;
  }
  mew::string WallPaper(const mew::string& filename, const mew::string& name, const mew::string& path) {
    if (m_fnWallPaper) {
      PyErr_Clear();
      if (mew::pygmy::String result = m_fnWallPaper(StringOrNone(filename), StringOrNone(name), StringOrNone(path))) {
        return result.str();
      }
    }
    return filename;
  }
};

AVESTA_EXPORT(PythonCallback)

//==============================================================================

namespace {
inline PWSTR ParseTupleForStringW(PyObject* args) {
  PWSTR wcs = nullptr;
  if (PyArg_ParseTuple(args, "u", &wcs)) {
    return wcs;
  } else {
    return nullptr;
  }
}

inline PSTR ParseTupleForStringA(PyObject* args) {
  PSTR str = nullptr;
  if (PyArg_ParseTuple(args, "s", &str)) {
    return str;
  } else {
    return nullptr;
  }
}

inline PWSTR ParseTupleForStringW(PyObject* args, PWSTR* option) {
  *option = nullptr;
  PWSTR wcs = nullptr;
  if (PyArg_ParseTuple(args, "u|u", &wcs, option)) {
    return wcs;
  } else {
    return nullptr;
  }
}

inline PSTR ParseTupleForStringA(PyObject* args, PSTR* option) {
  *option = nullptr;
  PSTR str = nullptr;
  if (PyArg_ParseTuple(args, "s|s", &str, option)) {
    return str;
  } else {
    return nullptr;
  }
}

// def Execute(path, ...)
static PyObject* Execute(PyObject* self, PyObject* args) {
  // まずはUnicodeで取得してみる
  {
    PWSTR param = nullptr;
    if (PWSTR wcs = ParseTupleForStringW(args, &param)) {
      ShellExecuteW(nullptr, nullptr, wcs, param, nullptr, SW_SHOWNORMAL);
      Py_RETURN_NONE;
    }
  }
  PyErr_Clear();
  // 失敗したので、stringで取得してみる
  {
    PSTR param = nullptr;
    if (PCSTR str = ParseTupleForStringA(args, &param)) {
      ShellExecuteA(nullptr, nullptr, str, param, nullptr, SW_SHOWNORMAL);
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
    ::GetDiskFreeSpaceExW(wcs, (ULARGE_INTEGER*)&uFreeBytes, nullptr, nullptr);
    return mew::pygmy::Long(uFreeBytes).detach();
  }
  // 失敗したので、stringで取得してみる
  PyErr_Clear();
  if (PCSTR str = ParseTupleForStringA(args)) {
    UINT64 uFreeBytes = 0;
    ::GetDiskFreeSpaceExA(str, (ULARGE_INTEGER*)&uFreeBytes, nullptr, nullptr);
    return mew::pygmy::Long(uFreeBytes).detach();
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
  if (!StatusTextParam::view) {
    return Py_None;
  }
  return mew::pygmy::Int(StatusTextParam::view->Count).detach();
}

// def GetTotalBytes()
static PyObject* GetTotalBytes(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) {
    return Py_None;
  }
  return mew::pygmy::Long(ave::GetTotalBytes(StatusTextParam::view)).detach();
}

// def GetSelectedCount()
static PyObject* GetSelectedCount(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) {
    return Py_None;
  }
  return mew::pygmy::Int(StatusTextParam::view->SelectedCount).detach();
}

// def GetSelectedBytes()
static PyObject* GetSelectedBytes(PyObject* self, PyObject* args) {
  if (!StatusTextParam::view) {
    return Py_None;
  }
  return mew::pygmy::Long(ave::GetSelectedBytes(StatusTextParam::view)).detach();
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
  mew::pygmy::Module module("pygmy", pygmy_functions, "Avesta extension");
  //
  module.add("LEFT", mew::ui::MouseButtonLeft);
  module.add("RIGHT", mew::ui::MouseButtonRight);
  module.add("MIDDLE", mew::ui::MouseButtonMiddle);
  module.add("X1", mew::ui::MouseButtonX1);
  module.add("X2", mew::ui::MouseButtonX2);
  module.add("CONTROL", mew::ui::ModifierControl);
  module.add("SHIFT", mew::ui::ModifierShift);
  module.add("ALT", mew::ui::ModifierAlt);
  module.add("WINDOWS", mew::ui::ModifierWindows);
  //
  module.add("CANCEL", avesta::NaviCancel);
  module.add("GOTO", avesta::NaviGoto);
  module.add("GOTO_ALWAYS", avesta::NaviGotoAlways);
  module.add("OPEN", avesta::NaviOpen);
  module.add("OPEN_ALWAYS", avesta::NaviOpenAlways);
  module.add("APPEND", avesta::NaviAppend);
  module.add("RESERVE", avesta::NaviReserve);
  module.add("SWITCH", avesta::NaviSwitch);
  module.add("REPLACE", avesta::NaviReplace);
}

static struct PyModuleDef moduledef = {PyModuleDef_HEAD_INIT, "pygmy", NULL, -1, pygmy_functions, NULL, NULL, NULL, NULL};

PyMODINIT_FUNC PyInit_pygmy() {
  mew::pygmy::Module module;
  module.attach(PyModule_Create(&moduledef));
  module.add("LEFT", mew::ui::MouseButtonLeft);
  module.add("RIGHT", mew::ui::MouseButtonRight);
  module.add("MIDDLE", mew::ui::MouseButtonMiddle);
  module.add("X1", mew::ui::MouseButtonX1);
  module.add("X2", mew::ui::MouseButtonX2);
  module.add("CONTROL", mew::ui::ModifierControl);
  module.add("SHIFT", mew::ui::ModifierShift);
  module.add("ALT", mew::ui::ModifierAlt);
  module.add("WINDOWS", mew::ui::ModifierWindows);
  //
  module.add("CANCEL", avesta::NaviCancel);
  module.add("GOTO", avesta::NaviGoto);
  module.add("GOTO_ALWAYS", avesta::NaviGotoAlways);
  module.add("OPEN", avesta::NaviOpen);
  module.add("OPEN_ALWAYS", avesta::NaviOpenAlways);
  module.add("APPEND", avesta::NaviAppend);
  module.add("RESERVE", avesta::NaviReserve);
  module.add("SWITCH", avesta::NaviSwitch);
  module.add("REPLACE", avesta::NaviReplace);
  return module;
}