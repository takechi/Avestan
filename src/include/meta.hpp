/// @file meta.hpp
/// テンプレートメタプログラミング.
/// Loki, TTL のあたりをパクりました.
#pragma once

namespace mew
{
	///	コンパイル時に型を扱う.
	namespace meta
	{
		//==============================================================================
		// 無効.

		struct Void;
		template < typename T > struct Void1;

		//==============================================================================
		///	静的な型情報.
		template < typename	T > struct TypeOf
		{
			enum
			{
				is_pointer		= 0, ///< ポインタか否か.
				is_reference	= 0, ///< 参照か否か.
			};
			typedef 	  T		value_type;
			typedef 	  T*	pointer_type;
			typedef 	  T&	reference_type;
			typedef const T&	param_type;
		};
		template < typename	T > struct TypeOf<T*>
		{
			enum
			{
				is_pointer		= 1,
				is_reference	= 0,
			};
			typedef T	value_type;
			typedef T*	pointer_type;
			typedef T&	reference_type;
			typedef T*	param_type;
		};
		template < typename	T > struct TypeOf<T&>
		{
			enum
			{
				is_pointer		= 0,
				is_reference	= 1,
			};
			typedef T	value_type;
			typedef T*	pointer_type;
			typedef T&	reference_type;
			typedef T&	param_type;
		};

		//==============================================================================
		///	bool If<value, Then, Else>::Result = (if value then	Then else Else).
		template < bool	exp, typename Then, typename Else > struct If					 { typedef Then	Result;	};
		template < /*false*/ typename Then, typename Else > struct If<false, Then, Else> { typedef Else	Result;	};

		//==============================================================================
		///	bool Equals<T1, T2>::value = (T1 ==	T2).
		template < typename	T1, typename T2 > struct Equals		  {	enum { value = 0 };	};
		template < typename	T               > struct Equals<T, T> {	enum { value = 1 };	};

		//==============================================================================
		///	bool Convertable<T1, T2>::value	= (if T2 t = T1() is ok).
		template < typename	T1, typename T2 >
		struct Convertable
		{
		private:
			struct U1 {	char dummy[1]; };
			struct U2 {	char dummy[2]; };
			static U1 test(T2);
			static U2 test(...);
			static T1 make();
		public:
			enum { value = (sizeof(U1) == sizeof(test(make()))) };
		};
		template < typename	T1, typename T2, size_t	size >
		struct Convertable<	T1[size], T2 >
		{
			enum { value = Convertable<T1*, T2>::value };
		};
		template < typename	T > struct Convertable<	T	, T	   > { enum	{ value	= 1	}; };
		template < typename	T > struct Convertable<	void, T	   > { enum	{ value	= 0	}; };
		template < typename	T > struct Convertable<	T	, void > { enum	{ value	= 0	}; };
		template <>				struct Convertable<	void, void > { enum	{ value	= 1	}; };

		//==============================================================================

		template <class T, class U>
		struct SuperSubclass
		{
			enum { value = (
				Convertable<const volatile U*, const volatile T*>::value
				&& !Equals<const volatile T*, const volatile void*>::value
			) };
		};

#define	SUPERSUBCLASS(T, U)		mew::meta::SuperSubclass<T,U>::value

		//==============================================================================

		template < class H, class T >
		struct Typelist
		{
			typedef H Head;
			typedef T Tail;
		};

		//==============================================================================

		template < class TList >      struct Length;
		template <>	                  struct Length< Void >				{ enum { value = 0 }; };
		template < class H, class T > struct Length< Typelist<H, T> >	{ enum { value = 1 + Length<T>::value }; };

		//==============================================================================

		template < class TList, unsigned int index>  struct At;
		template < class H, class T >                struct At<Typelist<H, T>, 0> { typedef H Result; };
		template < class H, class T, unsigned int i> struct At<Typelist<H, T>, i> { typedef typename At<T, i-1>::Result Result; };

		//==============================================================================

		template < class TList, class X> struct IndexOf;
		template < class X >             struct IndexOf<Void, X>          { enum { value = -1 }; };
		template < class X, class T >    struct IndexOf<Typelist<X, T>, X> { enum { value =  0 }; };
		template < class H, class T, class X>
		struct IndexOf<Typelist<H, T>, X>
		{
		private:
			enum { temp	= IndexOf<T, X>::value };
		public:
			enum { value = (temp ==	-1 ? -1	: 1	+ temp)	};
		};

		//==============================================================================

		template < class TList, class X >     struct Append;
		template <>	                          struct Append<Void, Void>				{ typedef Void Result; };
		template < class X >                  struct Append<Void, X>				{ typedef Typelist<X, Void> Result; };
		template < class H, class T >         struct Append<Void, Typelist<H, T> >	{ typedef Typelist<H, T> Result; };
		template < class H, class T, class X> struct Append<Typelist<H, T>, X>		{ typedef Typelist<H, typename Append<T, X>::Result> Result; };
		
		//==============================================================================

		template < class TList, class X >      struct Erase;
		template < class X >                   struct Erase<Void, X>				{ typedef Void Result; };
		template < class X, class T>           struct Erase<Typelist<X, T>, X>		{ typedef T Result; };
		template < class H, class T, class X > struct Erase<Typelist<H, T>, X>		{ typedef Typelist<H, typename Erase<T, X>::Result> Result; };

		//==============================================================================

		template < class TList, class X >      struct EraseAll;
		template < class X >                   struct EraseAll<Void, X>				{ typedef Void Result; };
		template < class X, class T >          struct EraseAll<Typelist<X, T>, X>	{ typedef typename EraseAll<T, X>::Result Result; };
		template < class H, class T, class X > struct EraseAll<Typelist<H, T>, X>	{ typedef Typelist<H, typename EraseAll<T, X>::Result> Result; };

		//==============================================================================

		template < class TList >      struct NoDuplicates;
		template <>                   struct NoDuplicates<Void>			{ typedef Void Result; };
		template < class H, class T > struct NoDuplicates< Typelist<H, T> >	{ typedef Typelist<H, typename Erase<typename NoDuplicates<T>::Result, H>::Result> Result; };

		//==============================================================================

		template < class TList, class X, class Y >      struct Replace;
		template < class X, class Y >                   struct Replace<Void, X, Y>				{ typedef Void Result; };
		template < class X, class T, class Y >          struct Replace<Typelist<X, T>, X, Y>	{ typedef Typelist<Y, T> Result; };
		template < class H, class T, class X, class Y > struct Replace<Typelist<H, T>, X, Y>	{ typedef Typelist<H, typename Replace<T, X, Y>::Result> Result; };

		//==============================================================================

		template < class TList, class X, class Y >      struct ReplaceAll;
		template < class X, class Y >                   struct ReplaceAll<Void, X, Y>			{ typedef Void Result; };
		template < class X, class T, class Y >          struct ReplaceAll<Typelist<X, T>, X, Y>	{ typedef Typelist<Y, typename ReplaceAll<T, X, Y>::Result> Result; };
		template < class H, class T, class X, class Y > struct ReplaceAll<Typelist<H, T>, X, Y>	{ typedef Typelist<H, typename ReplaceAll<T, X, Y>::Result> Result; };

		//==============================================================================

		template < class TList >      struct Reverse;
		template <>                   struct Reverse< Void >			{ typedef Void Result; };
		template < class H, class T > struct Reverse< Typelist<H, T> >	{ typedef typename Append<typename Reverse<T>::Result, H>::Result Result; };

		//==============================================================================

		template < class TList, class X >         struct MostDerived;
		template < class T >                      struct MostDerived<Void, T> { typedef T Result; };
		template < class H, class Tail, class T > struct MostDerived<Typelist<H, Tail>, T>
		{
		private:
			typedef typename MostDerived<Tail, T>::Result Candidate;
		public:
			typedef typename If< SuperSubclass<Candidate,H>::value, H, Candidate>::Result Result;
		};

		//==============================================================================

		template < class TList >      struct DerivedToFront;
		template <>                   struct DerivedToFront<Void> { typedef Void Result; };
		template < class H, class T > struct DerivedToFront< Typelist<H, T> >
		{
		private:
			typedef typename MostDerived<T, H>::Result TheMostDerived;
		public:
			typedef Typelist<
				TheMostDerived,
				typename DerivedToFront<typename Replace<T, TheMostDerived, H>::Result>::Result
			> Result;
		};
		
		//==============================================================================
		///	TList のそれぞれの要素 Element に対し、Pred<Status, Element>を呼び出す.
		///	繰り返しごとに Status =	Pred<Status, Element>::Result が次のStatusとして使用される.
		template < class TList, class Status, template <class Status, class T> class Pred >
		struct Fold
		{
			typedef typename Fold<
				typename TList::Tail, // 残り
				typename Pred<Status, typename TList::Head>::Result, // Head	の結果
				Pred
			>::Result Result;
		};
		template < class Status, template <class, class> class Pred >
		struct Fold<Void, Status, Pred>
		{
			typedef Status Result;
		};

		//==============================================================================
		///	TList に T よりも継承の下流のクラスが含まれている場合、TListからTを消去する.
		template < class TList, class T >
		struct RemoveIfNotMostDerived
		{
			typedef typename If<
				Equals<T, typename MostDerived<TList, T>::Result>::value,
				TList,
				typename Erase<TList, T>::Result
			>::Result Result;
		};

		//==============================================================================
		///	TList の中のすべての型は、それよりも上流の型が含まれていないようにする.
		template < class TList >
		struct MostDerivedOnly
		{
			typedef typename Fold<TList, TList, RemoveIfNotMostDerived>::Result	Result;
		};

		//==============================================================================
		///	TList に含まれるすべての型を多重継承したクラスを定義する.
		///	GenScatterHierarchyを利用した場合は余分なvtableが追加されるが、このクラスでは最適化されている.
		template < class TList >
		struct __declspec(novtable)	Inherits : TList::Head, Inherits<typename TList::Tail>
		{
			typedef typename TList::Head	Head;
			typedef typename TList::Tail	Tail;
		};
		template < class H >
		struct __declspec(novtable)	Inherits< Typelist<H, Void> > : H
		{};
		template <>
		struct __declspec(novtable)	Inherits<Void>
		{};

		//==============================================================================
		///	TList が T を含んでいるか否か.
		template < class TList, class T > struct Contains
		{
			enum { value = (IndexOf<TList, T>::value !=	-1)	};
		};

		//==============================================================================
		///	差集合を返す.
		template < class TLhs, class TRhs >
		struct Subtract
		{
			///	TLhsの中からTRhsに含まれる型を取り除く.
			///	TRhsには含まれるが、TLhsに含まれない型についてはなにもしない.
			typedef typename Fold<TRhs, TLhs, Erase>::Result Result;
		};

		//==============================================================================
		///	積集合を返す.
		template < class TLhs, class TRhs >
		struct Intersection
		{
		private:
			typedef typename NoDuplicates<TLhs>::Result	UniqueLeft;
		public:
			///	TLhsとTRhsの両方に含まれる型を得る.
			typedef typename Subtract<UniqueLeft, typename Subtract<UniqueLeft, TRhs>::Result>::Result Result;
		};

		//==============================================================================
		///	和集合を返す.
		template < class TLhs, class TRhs >
		struct Union
		{
			///	TLhsの末尾にTRhsを連結し、重複を取り除く.
			typedef typename NoDuplicates<typename Fold<TLhs, TRhs, Append>::Result>::Result;
		};

		//排他的和集合
		// Disjunction
	}
}
