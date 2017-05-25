/// @file message.hpp
/// message.
#pragma once

#include "struct.hpp"

namespace mew
{
	//==============================================================================

	/// variantに格納されたデータタイプ.
	enum Code
	{
		TypeNull		= 0,		///< empty.
		TypeBool		= 'BOOL',	///< 1byte.
		TypeSint8		= '__S1',	///< 1byte.
		TypeUint8		= '__U1',	///< 1byte.
		TypeSint16		= '__S2',	///< 2byte.
		TypeUint16		= '__U2',	///< 2byte.
		TypeSint32		= '__S4',	///< 4byte.
		TypeUint32		= '__U4',	///< 4byte.
		TypeSint64		= '__S8',	///< 8byte.
		TypeUint64		= '__U8',	///< 8byte.
		TypeSize		= 'SZS4',	///< 8byte.
		TypePoint		= 'V2S4',	///< 8byte.
		TypeRect		= 'RCS2',	///< 8byte.
		TypeColor		= 'CLU1',	///< 4byte.
		TypeUnknown		= 'UNKN',	///< 4byte.
		TypeFunction	= 'FUNC',	///< 8byte.
	};

	//==============================================================================

	class function;
	template < typename T > struct VariantType;
	template < typename T > struct VariantResult	{ using Result = T; };

	//==============================================================================

	using TypeCode = FourCC;

	/// variant.
	class variant
	{
	public:
		static const size_t INTERNAL_BUFFER_SIZE = 8;	///< 内部に格納できる構造体の最大サイズ.
		union Union
		{
			IUnknown*	 unknown;
			UINT8		 buffer[INTERNAL_BUFFER_SIZE];
		};
	private:
		TypeCode	m_type;		///< タイプコード.
		Union		m_var;		///< データバッファ.

	private:
		template < typename T, bool isPOD > struct Get_; //friend struct Get_;
		template < typename T, bool isPOD > struct Set_; //friend struct Set_;
		template < typename T > struct Get_<T, true>
		{
			enum { Code = VariantType<T>::Code };
			using Result = typename VariantResult<T>::Result;
			static Result get(const variant* self)	{ Result result; self->ToPOD(Code, &result, sizeof(result)); return result; }
		};
		template < typename T > struct Set_<T, true>
		{
			enum { Code = VariantType<T>::Code };
			using param_type = typename meta::TypeOf<T>::param_type;
			static void set(variant* self, param_type v)	{ self->FromPOD(Code, &v, sizeof(v)); }
		};
		template < typename T > struct Get_<T, false>
		{
			using Result = typename VariantResult<T>::Result;
			static Result get(const variant* self)	{ Result result; self->ToUnknown(&result); return result; }
		};
		template < typename T > struct Set_<T, false>
		{
			using param_type = typename meta::TypeOf<T>::param_type;
			static void set(variant* self, param_type v)	{ self->FromUnknown(v); }
		};
		template < typename T > struct Get : Get_<T, IsPOD<T>::value>	{};
		template < typename T > struct Set : Set_<T, IsPOD<T>::value>	{};
		template <> struct Set<function>
		{
			static void set(variant* self, const function& v)	{ self->FromFunction(v); }
		};
		template <> struct Get<Null>	{}; // invalid
		template <> struct Set<Null>	{}; // invalid
		template <> struct Get<variant>	{}; // invalid
		template <> struct Set<variant>	{}; // invalid

	public:
		static const variant null;
		// constructor & destructor
		variant() throw()												{ Zero(); }
		variant(const Null&) throw()									{ Zero(); }
		variant(const variant& rhs) throw()								{ FromVariant(rhs); }
		template < typename T > explicit variant(const T& rhs) throw()	{ Set<T>::set(this, rhs); }
		variant(TypeCode typecode, string str)							{ FromString(typecode, str); }
		~variant() throw()	{ MakeEmpty(); }

		// get & operator cast
		template < typename T > typename Get<T>::Result get() const		{ return Get<T>::get(this); }
		template < typename T > operator T () const throw()				{ return Get<T>::get(this); }

		// set & operator =
		variant& operator = (const variant& rhs) throw()					{ set(rhs); return *this; }
		template < typename T > variant& operator = (const T& rhs) throw()	{ set(rhs); return *this; }
		template < typename T > void set(const T& v) throw()				{ MakeEmpty(); Set<T>::set(this, v); }
		template <> void set<variant>(const variant& v) throw()				{ if(this != &v) { MakeEmpty(); FromVariant(v); } }
		template <> void set<Null>(const Null&) throw()						{ clear(); }
		void set(TypeCode typecode, string str)								{ MakeEmpty(); FromString(typecode, str); }

		// propertries
		bool		empty() const throw()		{ return m_type == TypeNull; }
		TypeCode	get_type() const throw()	{ return m_type; }
		__declspec(property(get=get_type)) TypeCode type;	///< タイプ.

		// operations
		void clear() throw() { MakeEmpty(); Zero(); }
		void load(IStream& stream);
		void save(IStream& stream) const;

	private:
		void Zero() throw()	{ memset(this, 0, sizeof(variant)); }
		void FromVariant(const variant& rhs) throw();
		void FromFunction(const function& handler);
		void FromString(TypeCode typecode, string str);
		void FromPOD(TypeCode code, const void* data, size_t size);
		template < typename T > void FromUnknown(const T& v) throw()
		{
			VariantType<T>::ToUnknown(v, &m_var.unknown);
			if(m_var.unknown)
				m_type = TypeUnknown;
			else
				m_type = TypeNull;
		}
		void ToPOD(TypeCode code, void* data, size_t size) const;
		void ToUnknown(REFINTF ppInterface) const;
		MEW_API void MakeEmpty();

		friend IStream& operator >> (IStream& stream, variant& v)			{ v.load(stream); return stream; }
		friend IStream& operator << (IStream& stream, const variant& v)	{ v.save(stream); return stream; }
	};

	__declspec(selectany) const variant variant::null;

	//==============================================================================
	// インタフェース

	__interface __declspec(uuid("DBBCEE59-E76B-4F0D-B946-976F8FF0F5F3")) IEnumVariant;
	__interface __declspec(uuid("B09128A7-CC7D-406C-A8BF-6BBB95B6C0A5")) IPersistMessage;

	//==============================================================================
	// インタフェース定義.

	using EventCode = FourCC;

	/// メッセージ.
	/// @see ref<IMessage>
	__interface IMessage : ISerializable
	{
		/// エントリ値を取得する.
		/// 指定した名前が見つからない場合は空のvariantが返される.
		const variant& Get(const Guid& key) throw();
		/// エントリ値を設定する.
		void Set(const Guid& key, const variant& var) throw();
		/// 内部エントリを列挙する.
		ref<IEnumVariant> Enumerate() throw();
	};

	/// メッセージを作成する.
	void CreateMessage(
		IMessage** ppMessage,	///< 作成されたメッセージ.
		EventCode  code,		///< 初期メッセージコード.
		IUnknown*  arg   = null	///< コンストラクタ引数.
	);

	/// verb.
	/// {9A2442FD-FA30-4bcb-BB88-A920570A336E}
	const GUID GUID_Verb = { 0x9a2442fd, 0xfa30, 0x4bcb, { 0xbb, 0x88, 0xa9, 0x20, 0x57, 0xa, 0x33, 0x6e } };

	/// メッセージ.
	/// @see IMessage
	template <> class ref<IMessage> : public ref_base<IMessage>
	{
	private:
		using super = ref_base<IMessage>;

		template < typename Owner >
		class proxy
		{
			friend class ref;
		private:
			Owner	m_owner;
			Guid	m_key;
		public:
			proxy(ref& owner, const Guid& key) : m_owner(owner), m_key(key) {}
			const variant& get() const	{ return m_owner ? m_owner->Get(m_key) : variant::null; }
			operator const variant& () const				{ return get(); }
			template < typename T > T get() const			{ return get(); }
			template < typename T > operator T () const		{ return get(); }
			template < typename T > T operator | (const T& def) const
			{
				if(!m_owner) return def;
				variant var = m_owner->Get(m_key);
				if(var.empty()) { return def; } else { return var; }
			}
			void set(const variant& value)							{ m_owner.DoSet(m_key, value); }
			template < typename T > void operator = (const T& rhs)	{ m_owner.DoSet(m_key, variant(rhs)); }
			template < typename T > const proxy<ref> operator [] (const T& key) const
			{
				return const_cast<proxy*>(this)->operator [] (key);
			}
			template < typename T > proxy<ref> operator [] (const T& key)
			{
				ref msg = get();
				ASSERT(msg);
				return proxy<ref>(msg, key);
			}
		};
		void DoSet(const Guid& key, const variant& value)
		{
			if(!m_ptr) create(0); // auto create
			m_ptr->Set(key, value);
		}

	public:
		ref() throw()	{}
		ref(const Null&) throw()	{}
		ref(const ref& p) throw() : super(p)	{}
		ref(pointer_in p) throw() : super(p)			{}
		ref(INT32    code, IUnknown* arg = null) throw(...)	{ create(code, arg); }
		ref(EventCode code, IUnknown* arg = null) throw(...)	{ create(code, arg); }
		ref& operator = (const ref& p) throw()	{ super::operator = (p); return *this; }

		void create(EventCode code, IUnknown* arg = null) throw(...)
		{
			ASSERT(!m_ptr);
			CreateMessage(&m_ptr, code, arg);
			ASSERT(m_ptr);
		}

		EventCode get_code() const throw()
		{
			if(!m_ptr)
				return 0;
			const variant& verb = m_ptr->Get(GUID_Verb);
			if(verb.empty())
				return 0;
			else
				return (INT32)verb;
		}
		__declspec(property(get=get_code)) EventCode code;

		template < typename T > const proxy<ref&> get(const T& key) const			{ return const_cast<ref*>(this)->get(key); }
		template < typename T >       proxy<ref&> get(const T& key)					{ return proxy<ref&>(*this, Guid(key)); }
		template < typename T > const proxy<ref&> operator [] (const T& key) const	{ return get(key); }
		template < typename T >       proxy<ref&> operator [] (const T& key)		{ return get(key); }
	};

	/// 名前付きvariant列挙.
	__interface IEnumVariant : IUnknown
	{
		/// 現在のオブジェクトを返し、内部イテレータをひとつ進める。
		/// すでに終端に達している場合は false を返す.
		bool Next(GUID* key, variant* var);
		/// もう一度はじめから列挙する.
		void Reset();
	};

	/// メッセージシンク.
	/// HRESULT function(message msg);
	class function
	{
	public:
		using static_function = HRESULT (*)(message);
		template < class TClass > using instance_function_t = HRESULT(TClass::*)(message);
		using instance_function = instance_function_t<IUnknown>;

	private:
		ref<IUnknown> m_target;
		union
		{
			DWORD				m_FUNCTION;
			static_function		m_fnStatic;
			instance_function	m_fnInstance;
		};

	public:
		function() throw() : m_FUNCTION(0) {}
		function(const Null&) throw() : m_FUNCTION(0) {}
		function(static_function m) throw() : m_fnStatic(m) {}
		template < class T, class U > function(const T& p, HRESULT (U::*m)(message)) throw()	{ assign(&*p, m); }
		void assign(static_function m) throw()
		{
			m_target.clear();
			m_fnStatic = m;
		}
		template < class T, class U > void assign(T* p, HRESULT (U::*m)(message)) throw()
		{
			__if_exists(T::__primary__)
			{
				m_target = static_cast<typename T::__primary__*>(p);
				using method = typename instance_function_t<T::__primary__>;
				m_fnInstance = static_cast<instance_function>( static_cast<method>(m) );
			}
			__if_not_exists(T::__primary__)
			{
				m_target = p;
				m_fnInstance = static_cast<instance_function>(m);
			}
		}
		IUnknown* get_target() const throw()	{ return m_target; }
		__declspec(property(get=get_target)) IUnknown* target;
		operator bool () const   { return m_FUNCTION != 0; }
		bool operator ! () const { return m_FUNCTION == 0; }
		friend bool operator == (const function& lhs, const function& rhs)
		{
			if(lhs.m_FUNCTION != rhs.m_FUNCTION)
				return false;
			return objcmp(lhs.m_target, rhs.m_target);
		}
		void clear() throw()
		{
			m_target.clear();
			m_FUNCTION = 0;
		}
		HRESULT operator () (const message& msg) const
		{
			ASSERT(m_FUNCTION);
			if(m_target)
				return (m_target->*m_fnInstance)(msg);
			else
				return (*m_fnStatic)(msg);
		}
	};

	/// メッセージとして状態を保存・復元できる.
	__interface IPersistMessage : IUnknown
	{
		/// 
		void LoadFromMessage(const message& msg);
		/// 
		message SaveToMessage();
	};

	HRESULT LoadPersistMessage(IPersistMessage* obj, const message& msg, const char* key);
	HRESULT SavePersistMessage(IPersistMessage* obj, message& msg, const char* key);
}

//==============================================================================
// detail

namespace mew
{
  #ifndef _WIN64
	STATIC_ASSERT(sizeof(FourCC)  == 4);  // == sizeof(INT32)
	STATIC_ASSERT(sizeof(Guid)    == 16); // == sizeof(GUID)
	STATIC_ASSERT(sizeof(variant) == 12); // XXX: 8バイト境界に並ぶようにすべき?
  #endif

	template <> class ToString<FourCC>
	{
	private:
		WCHAR m_str[8];
	public:
		ToString(FourCC value) throw();
		operator PCWSTR () const throw()	{ return m_str; }
	};

	template <> class ToString<Guid>
	{
	private:
		WCHAR m_str[40];
	public:
		ToString(const Guid& value) throw();
		operator PCWSTR () const throw()	{ return m_str; }
	};

	template <> class ToString<variant>
	{
	private:
		string	m_str;
	public:
		ToString(const variant& value) throw() : m_str(value) {}
		operator PCWSTR () const throw()	{ return m_str.str(); }
	};

	//==============================================================================

	template <> struct VariantType<bool>		{ enum { Code = TypeBool   }; };
	template <> struct VariantType<INT8>		{ enum { Code = TypeSint8  }; };
	template <> struct VariantType<UINT8>		{ enum { Code = TypeUint8  }; };
	template <> struct VariantType<INT16>		{ enum { Code = TypeSint16 }; };
	template <> struct VariantType<UINT16>		{ enum { Code = TypeUint16 }; };
	template <> struct VariantType<INT32>		{ enum { Code = TypeSint32 }; };
	template <> struct VariantType<UINT32>		{ enum { Code = TypeUint32 }; };
	template <> struct VariantType<INT64>		{ enum { Code = TypeSint64 }; };
	template <> struct VariantType<UINT64>		{ enum { Code = TypeUint64 }; };
	template <> struct VariantType<Size>		{ enum { Code = TypeSize   }; };
	template <> struct VariantType<Point>		{ enum { Code = TypePoint  }; };
	template <> struct VariantType<Rect>		{ enum { Code = TypeRect   }; };
	template <> struct VariantType<Color>		{ enum { Code = TypeColor  }; };

	//==============================================================================

	template <> struct VariantType<IUnknown*>
	{
		enum { Code = TypeUnknown };
		static void ToUnknown(IUnknown* p, IUnknown** pp) throw() { objcpy(p, pp); }
	};
	template <> struct VariantType<wchar_t*> : VariantType<IUnknown*>
	{
		static void ToUnknown(const wchar_t* v, IUnknown** pp) throw() { string(v).copyto(pp); }
	};
	template < typename T > struct VariantType< T* >                : VariantType<IUnknown*>	{};
	template < typename T > struct VariantType< ref<T> >            : VariantType<IUnknown*>	{};
	template <>             struct VariantType< const wchar_t* >    : VariantType<wchar_t*>		{};
	template < size_t sz >  struct VariantType< wchar_t[sz] >       : VariantType<wchar_t*>		{};
	template < size_t sz >  struct VariantType< const wchar_t[sz] > : VariantType<wchar_t*>		{};

	//==============================================================================

	template < typename T > struct VariantResult< T* >			{ using Result = ref<T>; };
	template < typename T > struct VariantResult< ref<T> >		{ using Result = ref<T>; };
	template < typename T > struct VariantResult< ref_base<T> >	{ using Result = ref_base<T>; };
}
