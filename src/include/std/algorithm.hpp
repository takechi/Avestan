/// @file algorithm.hpp
/// UNDOCUMENTED
#pragma once

#include <algorithm>
#include <functional>

namespace mew {
/// stdアルゴリズム拡張.
namespace algorithm {
/// for_all
template <class Cont, class Pred1>
Pred1 for_all(Cont& cont, Pred1 pred) {
  return std::for_each(cont.begin(), cont.end(), pred);
}

/// for_all
template <class Cont, class Pred1>
Pred1 for_all(const Cont& cont, Pred1 pred) {
  return std::for_each(cont.begin(), cont.end(), pred);
}

/// erase_it
template <class Cont, class Item>
void erase_it(Cont& cont, const Item& item) {
  cont.erase(std::remove(cont.begin(), cont.end(), item), cont.end());
}

/// erase_if
template <class Cont, class Pred1>
void erase_if(Cont& cont, Pred1 pred) {
  cont.erase(std::remove_if(cont.begin(), cont.end(), pred), cont.end());
}

/// find_it
template <class Cont, class Item>
typename Cont::iterator find_it(Cont& cont, const Item& item) {
  return std::find(cont.begin(), cont.end(), item);
}

/// find_if
template <class Cont, class Pred1>
typename Cont::iterator find_if(Cont& cont, Pred1 pred) {
  return std::find_if(cont.begin(), cont.end(), pred);
}

/// contains
template <class Cont, class Item>
bool contains(Cont& cont, const Item& item) {
  return std::find(cont.begin(), cont.end(), item) != cont.end();
}

/// binary_search
/// std::binary_search() は
/// bool値しか返さないのに対し、これは見つかったイタレータを返す.
template <class FwdIter, class type>
FwdIter binary_search(FwdIter _First, FwdIter _Last, const type& _Val) {
  FwdIter result = std::lower_bound(_First, _Last, _Val);
  if (result != _Last && *result == _Val)
    return result;
  else
    return _Last;
}

/// operator delete
struct operator_delete {
  template <typename T>
  void operator()(T* ptr) const {
    delete ptr;
  }
};

/// _1->method()
template <class T, class Result>
std::mem_fun_t<Result, T> mem_fun(Result (T::*m)()) {
  return std::mem_fun(m);
}

template <class P, class M, class R, class A>
class ptr_method_1_t {
  P p;
  M m;

 public:
  ptr_method_1_t(P pp, M mm) : p(pp), m(mm) {}
  R operator()(A a) const { return (p->*m)(a); }
};

/// obj->method(_1)
template <class Obj, class T, class Result, class Arg>
ptr_method_1_t<Obj, Result (T::*)(Arg), Result, Arg> mem_fun(Obj obj, Result (T::*m)(Arg)) {
  return ptr_method_1_t<Obj, Result (T::*)(Arg), Result, Arg>(obj, m);
}
/*  template < class Obj, class T, class Result, class Arg >
                std::binder1st< std::mem_fun1_t<Result, T, Arg> >
                mem_fun(Obj obj, Result (T::*m)(Arg))
                {
                        return std::bind1st(std::mem_fun1(m), obj);
                }
*/
/// _1->method(obj)
template <class T, class Result, class Arg, class Obj>
std::binder2nd<std::mem_fun1_t<Result, T, Arg> > mem_fun(Result (T::*m)(Arg), Obj obj) {
  return std::bind2nd(std::mem_fun1(m), obj);
}

/// obj->method(_1)
template <class Obj, class T, class Result, class Arg>
std::binder1st<std::mem_fun1_ref_t<Result, T, Arg> > mem_fun_ref(Obj obj, Result (T::*m)(Arg)) {
  return std::bind1st(std::mem_fun1_ref(m), obj);
}

/// _1->method(obj)
template <class T, class Result, class Arg, class Obj>
std::binder2nd<std::mem_fun1_ref_t<Result, T, Arg> > mem_fun_ref(Result (T::*m)(Arg), Obj obj) {
  return std::bind2nd(std::mem_fun1_ref(m), obj);
}

template <class First>
class first_equals_t {
 private:
  First m_First;

 public:
  first_equals_t(const First& first) : m_First(first) {}
  template <class Second>
  bool operator()(const std::pair<First, Second>& p) const {
    return p.first == m_First;
  }
};
template <class First>
inline first_equals_t<First> first_equals(const First& first) {
  return first_equals_t<First>(first);
}
}  // namespace algorithm
}  // namespace mew
