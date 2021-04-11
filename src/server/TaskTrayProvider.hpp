// TaskTrayProvider.hpp
#pragma once

#include "main.hpp"

//==============================================================================

template <class TBase>
class __declspec(novtable) TaskTrayProvider : public TBase {
 private:
  bool m_EnableTray;
  bool m_AlwaysTray;
  bool m_CloseToTray;

 private:
  HRESULT TaskTray_HandleResize(message msg) {
    HWND hwnd = m_form->Handle;
    if (!IsWindowEnabled(hwnd)) return S_OK;
    if (m_EnableTray && ::IsIconic(hwnd) &&
        m_form->Visible) {  // 復帰は、Form.CommandRestore で実装されているため処理する必要は無い
      m_form->Visible = false;
      m_form->TaskTray = true;
    }
    // 表示されているのにタスクトレイアイコンがある場合は消す
    if (!m_AlwaysTray && m_form->Visible) {
      m_form->TaskTray = false;
    }
    return S_OK;
  }
  HRESULT TaskTray_HandleQueryClose(message msg) {
    if (m_form && m_CloseToTray && m_form->Visible && theMainResult == 0) {  // cancel close and minimize to tasktray
      ShowWindow(m_form->Handle, SW_MINIMIZE);
      m_form->Visible = false;
      m_form->TaskTray = true;
      msg["cancel"] = true;
    }
    return S_OK;
  }

 public:
  TaskTrayProvider() {
    m_EnableTray = false;
    m_AlwaysTray = false;
    m_CloseToTray = false;
  }
  void TaskTray_Load(message& msg) {
    m_EnableTray = msg["TaskTray"] | false;
    m_AlwaysTray = msg["AlwaysTray"] | false;
    m_CloseToTray = msg["CloseToTray"] | false;
    m_form->TaskTray = m_AlwaysTray;
  }
  void TaskTray_Save(message& msg) {
    msg["TaskTray"] = m_EnableTray;
    msg["AlwaysTray"] = m_AlwaysTray;
    msg["CloseToTray"] = m_CloseToTray;
  }
  void TaskTray_InitComponents(IWindow* form) {
    HandleEvent(form, EventResize, &TaskTrayProvider::TaskTray_HandleResize);
    HandleEvent(form, EventPreClose, &TaskTrayProvider::TaskTray_HandleQueryClose);
  }

 public:  // Commands
  HRESULT OptionTaskTray(message = null) {
    m_EnableTray = !m_EnableTray;
    return S_OK;
  }
  HRESULT ObserveTaskTray(message msg) {
    msg["state"] = ENABLED | (m_EnableTray ? CHECKED : 0);
    return S_OK;
  }
  HRESULT OptionAlwaysTray(message = null) {
    m_AlwaysTray = !m_AlwaysTray;
    m_form->TaskTray = m_AlwaysTray;
    return S_OK;
  }
  HRESULT ObserveAlwaysTray(message msg) {
    msg["state"] = ENABLED | (m_AlwaysTray ? CHECKED : 0);
    return S_OK;
  }
  HRESULT OptionCloseToTray(message = null) {
    m_CloseToTray = !m_CloseToTray;
    return S_OK;
  }
  HRESULT ObserveCloseToTray(message msg) {
    msg["state"] = ENABLED | (m_CloseToTray ? CHECKED : 0);
    return S_OK;
  }
};
