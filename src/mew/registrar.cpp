// registrar.cpp

#include "stdafx.h"
#include "private.h"
#include "std/hash_map.hpp"

inline bool operator<(REFGUID lhs, REFGUID rhs) { return memcmp(&lhs, &rhs, sizeof(GUID)) < 0; }

//==============================================================================

namespace {
using Registrar = stdext::hash_map<CLSID, mew::FactoryProc>;
// ���̃O���[�o���ϐ��̏��������[�`������Ă΂��Ă��ǂ��悤�ɁA
// �֐��Ȃ��X�^�e�B�b�N�ɂ��č\�z�����𐧌䂷��K�v������B
Registrar& GetRegistrar() {
  static Registrar theRegistrar;
  return theRegistrar;
}
}  // namespace

namespace mew {
MEW_API void CreateInstance(REFCLSID clsid, REFINTF ppInterface, IUnknown* arg) throw(...) {
  // �܂��A���W���[���N���X�}�b�v���猟�����A������Ȃ����COM�N���X���쐬����.
  HRESULT hr;
  Registrar::const_iterator i = GetRegistrar().find(clsid);
  if (i != GetRegistrar().end()) {  // �t�@�N�g���֐�����������
    i->second(ppInterface, arg);
    ASSERT(*ppInterface.pp);
    return;
  } else if (SUCCEEDED(
                 hr = ::CoCreateInstance(clsid, null, CLSCTX_ALL, ppInterface.iid, ppInterface.pp))) {  // COM�N���X�̍쐬�ɐ���
    TRACE(L"info: CoCreateInstance($1)", clsid);
    ASSERT(*ppInterface.pp);
    return;
  } else {  // �N���X��������Ȃ�
    throw mew::exceptions::ClassError(string::load(IDS_ERR_INVALIDCLSID, clsid), hr);
  }
}
MEW_API void RegisterFactory(REFCLSID clsid, FactoryProc factory) throw() {
  ASSERT(GetRegistrar()[clsid] == null && "�쐬�\�N���X�̓�d�o�^");
  GetRegistrar()[clsid] = factory;
}
}  // namespace mew
