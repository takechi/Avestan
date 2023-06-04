// commandline.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "object.hpp"

using namespace avesta;

//==============================================================================

namespace {
class CommandLine : public mew::Root<mew::implements<avesta::ICommandLine> > {
 private:
  struct Argv {
    PTSTR Option;
    PTSTR Value;
  };
  std::vector<TCHAR> m_text;
  std::vector<Argv> m_args;
  std::vector<Argv>::iterator m_iter;

 public:
  CommandLine(PCTSTR commandLine) {
    m_text.assign(commandLine, commandLine + mew::str::length(commandLine) + 1);
    if (!m_text.empty()) ParseArgv(&m_text[0], m_args);
    Reset();
  }
  bool Next(PTSTR* option, PTSTR* value) {
    if (m_iter == m_args.end()) return false;
    *option = m_iter->Option;
    *value = m_iter->Value;
    ++m_iter;
    return true;
  }
  void Reset() { m_iter = m_args.begin(); }

 private:
  static void ParseArgv(PTSTR commandline, std::vector<Argv>& args) {
    bool quoted = false;
    PTSTR beg, end;
    beg = end = commandline;
    while (true) {
      switch (*end) {
        case _T('\0'):
          // �R�}���h���C���I��
          if (beg < end) args.push_back(ParseOption(beg));
          return;
        case _T('\"'):
          if (quoted) {  // �N�H�[�g���ꂽ������̒��o
            *end = _T('\0');
            args.push_back(ParseOption(beg));
            quoted = false;
            beg = ++end;
          } else {  // �N�H�[�g�J�n
            quoted = true;
            beg = ++end;
          }
          break;
        case _T(' '):
          if (quoted) {  // �N�H�[�g���̃X�y�[�X�͕����Ƃ��Ĉ���
            end++;
          } else {  // �N�H�[�g����Ă��Ȃ��X�y�[�X�͋�؂�Ƃ��Ĉ���
            *end = _T('\0');
            if (beg < end) args.push_back(ParseOption(beg));
            beg = ++end;
          }
          break;
        default:
          // ���̑��͕����Ƃ��Ĉ���
          end = mew::str::inc(end);
      }
    }
  }
  static Argv ParseOption(PTSTR argv) {
    struct Argv arg;
    if (argv[0] == _T('-') || argv[0] == _T('/')) {  // �I�v�V�����ƒl�̕���
      TCHAR* strOption = argv + 1;
      TCHAR* strValue = mew::str::find_some_of(argv, _T(":="));
      if (strValue) {  // �����t���I�v�V����
        *strValue = _T('\0');
        strValue++;
        arg.Option = strOption;
        arg.Value = strValue;
      } else {  // �����Ȃ��I�v�V����
        strValue++;
        arg.Option = strOption;
        arg.Value = nullptr;
      }
    } else {  // �I�v�V�����Ȃ��G��Ƀt�@�C���p�X
      arg.Option = nullptr;
      arg.Value = argv;
    }
    return arg;
  }
};
}  // namespace

//==============================================================================

mew::ref<ICommandLine> avesta::ParseCommandLine(PCWSTR args) {
  if (mew::str::empty(args)) {
    return mew::null;
  }
  return mew::objnew<CommandLine>(args);
}
