// DefaultTreeItem.cpp

#include "stdafx.h"
#include "../private.h"
#include "widgets.hpp"

using namespace mew::ui;

//==============================================================================

namespace mew {
namespace ui {

class DefaultTreeItem : public Root<implements<ITreeItem, IEditableTreeItem> > {
 private:  // variables
  string m_Text;
  ref<ICommand> m_pCommand;
  int m_Image;
  UINT32 m_TimeStamp;
  array<ITreeItem> m_Children;

 public:  // Object
  void __init__(IUnknown* arg) {
    ASSERT(!arg);
    m_TimeStamp = 0;
    m_Image = I_IMAGENONE;
  }
  void Dispose() throw() {
    m_Text.clear();
    m_pCommand.clear();
    m_Children.clear();
    ++m_TimeStamp;
  }

 public:  // ITreeItem
  string get_Name() { return m_Text; }
  ref<ICommand> get_Command() { return m_pCommand; }
  int get_Image() { return m_Image; }
  void set_Name(string value) {
    m_Text = value;
    ++m_TimeStamp;
  }
  void set_Command(ICommand* value) {
    m_pCommand = value;
    ++m_TimeStamp;
  }
  void set_Image(int value) {
    m_Image = value;
    ++m_TimeStamp;
  }

  bool HasChildren() { return !m_Children.empty(); }
  size_t GetChildCount() { return m_Children.size(); }
  HRESULT GetChild(REFINTF ppChild, size_t index) {
    if (index >= m_Children.size()) return E_INVALIDARG;
    return m_Children[index]->QueryInterface(ppChild);
  }
  UINT32 OnUpdate() { return m_TimeStamp; }
  void AddChild(ITreeItem* child) {
    m_Children.push_back(child);
    ++m_TimeStamp;
  }
  bool RemoveChild(ITreeItem* child) {
    if (!m_Children.erase(child)) return false;
    ++m_TimeStamp;
    return true;
  }
};

}  // namespace ui
}  // namespace mew

AVESTA_EXPORT(DefaultTreeItem)
