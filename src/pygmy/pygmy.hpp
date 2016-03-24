/// @file pygmy.hpp
/// シェルとファイルシステム.
#pragma once

#pragma push_macro("_DEBUG")
#undef _DEBUG
//#ifdef _DEBUG
//#	define Py_DEBUG
//#endif
#include <Python.h>
#if PY_MAJOR_VERSION >= 3
#define PyInt_FromLong PyLong_FromLong
#define PyNumber_Int PyNumber_Long
#define PyInt_AS_LONG PyLong_AsLong
#define PyString_FromString PyBytes_FromString
#define PyString_FromStringAndSize PyBytes_FromStringAndSize
#define PyString_FromFormatV PyBytes_FromFormatV
#define PyString_AS_STRING PyBytes_AsString
#define PyString_GET_SIZE PyBytes_GET_SIZE
#define PyNumber_InPlaceDivide PyNumber_InPlaceTrueDivide
#define PyNumber_Divide PyNumber_TrueDivide
#endif
#pragma pop_macro("_DEBUG")

#include "mew.hpp"
#include "preprocessor.hpp"

#define PYGMY_MAX_ARGC	6

namespace mew
{
	/// Python bridge library.
	namespace pygmy
	{
		//============================================================================================================
		// contents.

		class Object;
		class Type;
		class None;
		class Tuple;
		class List;
		class Dictionary;
		class Iterator;
		class Module;
		template < typename T > struct ValueTraits;
		template < typename T > struct StringTraits;
		template < typename T, typename Traits = ValueTraits<T>  > class ValueObject;
		template < typename T, typename Traits = StringTraits<T> > class StringObject;
		typedef ValueObject<bool>		Bool;
		typedef ValueObject<int>		Int;
		typedef ValueObject<__int64>	Long;
		typedef ValueObject<double>		Real;
		typedef StringObject<char>		StringA;
		typedef StringObject<wchar_t>	StringW;
		typedef StringObject<TCHAR>		String;

		//============================================================================================================

		template <> struct ValueTraits<bool>
		{
			static PyObject* create()				{ return create(false); }
			static PyObject* create(bool value)	
			{
				PyObject* obj = value ? Py_True : Py_False;
				Py_INCREF(obj);
				return obj;
			}
			static PyObject* from(PyObject* obj)	{ return create(PyObject_IsTrue(obj) != 0); }
			static bool value(PyObject* self)		{ return PyObject_IsTrue(self) != 0; }
		};
		template <> struct ValueTraits<int>
		{
			static PyObject* create()				{ return create(0); }
			static PyObject* create(int value)		{ return PyInt_FromLong(value); }
			static PyObject* from(PyObject* obj)	{ return obj ? PyNumber_Int(obj) : NULL; }
			static int value(PyObject* self)		{ return self ? PyInt_AS_LONG(self) : 0; }
		};
		template <> struct ValueTraits<__int64>
		{
			static PyObject* create()				{ return create(0); }
			static PyObject* create(__int64 value)	{ return PyLong_FromLongLong(value); }
			static PyObject* from(PyObject* obj)	{ return obj ? PyNumber_Long(obj) : NULL; }
			static __int64 value(PyObject* self)	{ return self ? PyLong_AsLongLong(self) : 0; }
		};
		template <> struct ValueTraits<double>
		{
			static PyObject* create()				{ return create(0); }
			static PyObject* create(double value)	{ return PyFloat_FromDouble(value); }
			static PyObject* from(PyObject* obj)	{ return obj ? PyNumber_Float(obj) : NULL; }
			static double value(PyObject* self)		{ return self ? PyFloat_AS_DOUBLE(self) : 0; }
		};

		//============================================================================================================

		template <> struct StringTraits<char>
		{
			static PyObject* create(const char* value)
			{
				return PyString_FromString(value);
			}
			static PyObject* create(const char* value, size_t length)
			{
				return PyString_FromStringAndSize(value, (int)length);
			}
			static PyObject* format(const char* fmt, va_list args)		{ return PyString_FromFormatV(fmt, args); }
			static PyObject* from(PyObject* obj)						{ return obj ? PyObject_Str(obj) : NULL; }
			static const char* str(PyObject* obj)						{ return obj ? PyString_AS_STRING(obj) : NULL; }
			static size_t length(PyObject* obj)							{ return obj ? PyString_GET_SIZE(obj) : 0; }
			static bool empty(PyObject* obj)							{ return length(obj) == 0; }
		};
		template <> struct StringTraits<wchar_t>
		{
			static PyObject* create(const wchar_t* value)
			{
				return PyUnicode_FromUnicode(value, value ? (int)wcslen(value): 0);
			}
			static PyObject* create(const wchar_t* value, size_t length)
			{
				return PyUnicode_FromUnicode(value, (int)length);
			}
			static PyObject* format(const wchar_t* fmt, va_list args)
			{
				const size_t buflen = 1024;
				wchar_t buffer[buflen];
				int len = _vsnwprintf(buffer, buflen, fmt, args);
				return PyUnicode_FromUnicode(buffer, len);
			}
			static PyObject* from(PyObject* obj)
			{
				if(!obj)
					return NULL;
				else if(PyObject* ret = PyUnicode_FromObject(obj))
					return ret;
				else if(PyObject* str = PyObject_Str(obj))
				{
					PyObject* uni = PyUnicode_DecodeMBCS(PyString_AS_STRING(str), PyString_GET_SIZE(str), NULL);
					Py_DECREF(str);
					return uni;
				}
				else
					return NULL;
			}
			static const wchar_t* str(PyObject* obj)					{ return obj ? PyUnicode_AS_UNICODE(obj) : NULL; }
			static size_t length(PyObject* obj)							{ return obj ? PyUnicode_GET_SIZE(obj) : 0; }
			static bool empty(PyObject* obj)							{ return length(obj) == 0; }
		};

		//============================================================================================================

		class Object
		{
		protected:
			PyObject*	m_self;

		public: // self.name
			static Object GetAttr(PyObject* self, PyObject* name)					{ return Object::from(PyObject_GetAttr(self, name)); }
			static Object GetAttr(PyObject* self, const char* name)					{ return Object::from(PyObject_GetAttrString(self, const_cast<char*>(name))); }
			static bool SetAttr(PyObject* self, PyObject*   name, PyObject* obj)	{ return PyObject_SetAttr(self, name, obj) != -1; }
			static bool SetAttr(PyObject* self, const char* name, PyObject* obj)	{ return PyObject_SetAttrString(self, const_cast<char*>(name), obj) != -1; }
			static bool HasAttr(PyObject* self, PyObject*   name)					{ return PyObject_HasAttr(self, name) != 0; }
			static bool HasAttr(PyObject* self, const char* name)					{ return PyObject_HasAttrString(self, const_cast<char*>(name)) != 0; }
			static bool DelAttr(PyObject* self, PyObject*   name)					{ return PyObject_DelAttr(self, name) != -1; }
			static bool DelAttr(PyObject* self, const char* name)					{ return PyObject_DelAttrString(self, const_cast<char*>(name)) != -1; }

		public: // self[name]
			static Object GetItem(PyObject* self, PyObject*   name)					{ return Object::from(PyObject_GetItem(self, name)); }
			static Object GetItem(PyObject* self, const char* name)					{ return Object::from(PyMapping_GetItemString(self, const_cast<char*>(name))); }
			static Object GetItem(PyObject* self, int         i)					{ return Object::from(PySequence_GetItem(self, i)); }
			static bool SetItem(PyObject* self, PyObject*   name, PyObject* obj)	{ return PyObject_SetItem(self, name, obj) != -1; }
			static bool SetItem(PyObject* self, const char* name, PyObject* obj)	{ return PyMapping_SetItemString(self, const_cast<char*>(name), obj) != -1; }
			static bool SetItem(PyObject* self, int         i   , PyObject* obj)	{ return PySequence_SetItem(self, i, obj) != -1; }
			static bool DelItem(PyObject* self, PyObject*   name)					{ return PyObject_DelItem(self, name) != -1; }
			static bool DelItem(PyObject* self, const char* name)					{ return PyMapping_DelItemString(self, const_cast<char*>(name)) != -1; }
			static bool DelItem(PyObject* self, int         i)						{ return PySequence_DelItem(self, i) != -1; }

		public: // self.__iter__
			static Iterator GetIter(PyObject* self);

		public:
			Object() : m_self(0)							{}
			Object(const Null&) : m_self(0)					{}
			explicit Object(PyObject* obj) : m_self(obj)	{ Py_XINCREF(m_self); }
			Object(const Object& rhs) : m_self(rhs.m_self)	{ Py_XINCREF(m_self); }
			Object& operator = (const Object& rhs)			{ assign(rhs); return *this; }
			~Object()										{ Py_XDECREF(m_self); }

			static Object from(PyObject* self)	{ Object obj; obj.m_self = self; return obj; }
			void assign(const Object& rhs)	{ assign(rhs.m_self); }
			void assign(PyObject* self)		{ Py_XINCREF(self); Py_XDECREF(m_self); m_self = self; }

		public:
			void attach(PyObject* self)		{ if(m_self != self) { Py_XDECREF(m_self); m_self = self; } }
			PyObject* detach()				{ PyObject* self = m_self; m_self = 0; return self; }
			operator PyObject* () const		{ return m_self; }
			bool is_callable() const		{ return PyCallable_Check(m_self) != 0; }
			bool is_true() const			{ return m_self && PyObject_IsTrue(m_self) == 1; }
			bool is_false() const			{ return !m_self || PyObject_Not(m_self) == 1; }
			bool operator ! () const		{ return is_false(); }

		public: // self.name
			Object getattr(const char* name) const			{ return GetAttr(m_self, name); }
			Object getattr(PyObject*   name) const			{ return GetAttr(m_self, name); }
			bool setattr(const char* name, PyObject* obj)	{ return SetAttr(m_self, name, obj); }
			bool setattr(PyObject*   name, PyObject* obj)	{ return SetAttr(m_self, name, obj); }
			bool hasattr(const char* name) const			{ return HasAttr(m_self, name); }
			bool hasattr(PyObject*   name) const			{ return HasAttr(m_self, name); }
			bool delattr(const char* name)					{ return DelAttr(m_self, name); }
			bool delattr(PyObject*   name)					{ return DelAttr(m_self, name); }
			StringA repr() const;
			Type type() const;

		public: // self[name]
			template < typename T > Object operator [] (const T& name) const	{ return getitem(name); }
			Object getitem(PyObject*   name) const	{ return GetItem(m_self, name); }
			Object getitem(const char* name) const	{ return GetItem(m_self, name); }
			Object getitem(int         i   ) const	{ return GetItem(m_self, i); }

		public: // self()
			Object apply(const Tuple& args) const;
			Object apply(const Tuple& args, const Dictionary& kwds) const;

			Object operator () () const	{ return m_self ? Object::from(PyObject_CallObject(m_self, null)) : Object(); }

#define PYGMY_TUPLE(n)		args[n] = PP_CAT(arg, n)
#define PYGMY_APPLY(n)											\
			template < PP_TYPENAMES(n) >						\
			Object operator () ( PP_ARGS_CONST(n) ) const		\
			{													\
				if(!m_self) return null;						\
				Tuple args(n); PP_CSV1(n, PYGMY_TUPLE);			\
				return apply(args);								\
			}

			PP_REPEAT_FROM_1(PYGMY_MAX_ARGC, PYGMY_APPLY)

#undef PYGMY_TUPLE
#undef PYGMY_APPLY

		public: // number protocol.
			friend Object operator + (const Object& lhs, const Object& rhs)
			{
				return Object::from( PyNumber_Add(lhs.m_self, rhs.m_self) );
			}
			friend Object operator - (const Object& lhs, const Object& rhs)
			{
				return Object::from( PyNumber_Subtract(lhs.m_self, rhs.m_self) );
			}
			friend Object operator * (const Object& lhs, const Object& rhs)
			{
				return Object::from( PyNumber_Multiply(lhs.m_self, rhs.m_self) );
			}
			friend Object operator / (const Object& lhs, const Object& rhs)
			{
				return Object::from( PyNumber_Divide(lhs.m_self, rhs.m_self) );
			}
			Object& operator += (const Object& rhs)
			{
				PyObject* obj = PyNumber_InPlaceAdd(m_self, rhs.m_self);
				attach(obj);
				return *this;
			}
			Object& operator -= (const Object& rhs)
			{
				PyObject* obj = PyNumber_InPlaceSubtract(m_self, rhs.m_self);
				attach(obj);
				return *this;
			}
			Object& operator *= (const Object& rhs)
			{
				PyObject* obj = PyNumber_InPlaceMultiply(m_self, rhs.m_self);
				attach(obj);
				return *this;
			}
			Object& operator /= (const Object& rhs)
			{
				PyObject* obj = PyNumber_InPlaceDivide(m_self, rhs.m_self);
				attach(obj);
				return *this;
			}
/*
		Slice slice(int left, int right) const
		{
			if(PySequence_Check(m_obj))
				return Slice(m_obj, left, right);
			else
				throw CastError();
		}
		class Slice : public Proxy
		{
		private:
			PyObject* m_obj;
			int m_left, m_right;

		protected:
			Object get_object() const
			{
				return Object::from( PySequence_GetSlice(m_obj, m_left, m_right);
				if(!obj)
					throw InvalidOperationException();
				else
					return Object(obj);
			}
			void set_object(const Object& val) const
			{
				int result = PySequence_SetSlice(m_obj, m_left, m_right, val.m_obj);
				if(result == -1)
					throw InvalidOperationException();
			}

		public:
			Slice(PyObject* obj, int left, int right) : m_obj(obj), m_left(left), m_right(right)	{}
			template < typename T > void operator = (const T val) const { set_object(val); }
		};
*/
			};

		//============================================================================================================
		/// Value object client.
		template < typename T, typename Traits > class ValueObject : public Object
		{
		public:
			typedef T		value_type;
			typedef Traits	traits;
		public:

		public:
			ValueObject()	{}
			ValueObject(value_type value)				{ m_self = traits::create(value); }
			ValueObject(const Object& rhs)				{ m_self = traits::from(rhs); }
			explicit ValueObject(PyObject* rhs)			{ m_self = traits::from(rhs); }
			const value_type value() const				{ return traits::value(m_self); }
			operator const value_type () const			{ return value(); }
		};

		//============================================================================================================

		class None : public Object
		{
		public:
			None() : Object(Py_None)
			{
			}
		};

		//============================================================================================================
		/// Type object client.
		class Type : public Object
		{
		public:
			static bool is_type(PyObject* obj)	{ return obj && PyType_Check(obj); }
			Type() {}
			Type(const Object& rhs)			{ if(is_type(rhs)) assign(rhs); }
			explicit Type(PyObject* rhs)	{ if(is_type(rhs)) assign(rhs); }
			const char* name() const
			{
				if(!m_self) return null;
				return ((PyTypeObject*)m_self)->tp_name;
			}
		};

		inline Type Object::type() const
		{
			Type result;
			result.attach(PyObject_Type(m_self));
			return result;
		}

		//============================================================================================================
		/// String object client.
		template < typename T, typename Traits > class StringObject : public Object
		{
		public:
			typedef T			value_type;
			typedef const T*	const_pointer;
			typedef Traits		traits;
		public:
			static StringObject format(const_pointer fmt, ...)
			{
				va_list vars;
				va_start(vars, fmt);
				StringObject obj;
				obj.attach(traits::format(fmt, vars));
				va_end(vars);
				return obj;
			}
			StringObject()	{}
			StringObject(const_pointer value)					{ m_self = traits::create(value); }
			StringObject(const_pointer value, size_t length)	{ m_self = traits::create(value, length); }
			StringObject(const Object& rhs)						{ m_self = traits::from(rhs); }
			explicit StringObject(PyObject* rhs)				{ m_self = traits::from(rhs); }
			const_pointer str() const							{ return traits::str(m_self); }
			size_t length() const								{ return traits::length(m_self); }
			bool empty() const									{ return traits::empty(m_self); }
		};

		inline StringA Object::repr() const
		{
			StringA result;
			result.attach(PyObject_Repr(m_self));
			return result;
		}

		//============================================================================================================
		/// Iterator object client.
		class Iterator : public Object
		{
		public:
			Iterator() : Object()								{}
			Iterator(const Null& null) : Object(null)			{}
			explicit Iterator(PyObject* obj) : Object(obj)		{ Py_XINCREF(m_self); }
			Iterator(const Iterator& rhs) : Object(rhs)			{ Py_XINCREF(m_self); }
			Iterator& operator = (const Iterator& rhs)			{ assign(rhs); return *this; }

			static Iterator from(PyObject* self)	{ Iterator obj; obj.m_self = self; return obj; }
			void assign(const Iterator& rhs)		{ assign(rhs.m_self); }
			void assign(PyObject* self)				{ Py_XINCREF(self); Py_XDECREF(m_self); m_self = self; }

			Object next()	{ return Object::from(PyIter_Next(m_self)); }
		};

		static Iterator GetIter(PyObject* self)		{ return Iterator::from(PyObject_GetIter(self)); }

		//============================================================================================================
		/// Tuple object client.
		class Tuple : public Object
		{
		private:
			class Proxy
			{
			private:
				PyObject* const	m_self;
				const int		m_index;
			public:
				Proxy(PyObject* obj, int index) : m_self(obj), m_index(index)	{}
				Proxy& operator = (PyObject* obj)
				{
					// PyTuple_SetItem : 参照カウントを増加させない
					PyTuple_SetItem(m_self, m_index, obj);
					Py_XINCREF(obj);
					return *this;
				}
				operator Object () const
				{
					// PyTuple_GetItem : 参照カウントを増加させない
					return Object(PyTuple_GetItem(m_self, m_index));
				}
			};
		public:
			static bool is_tuple(PyObject* obj)	{ return obj && PyTuple_Check(obj); }
			static PyObject* create(size_t size)	{ return PyTuple_New((int)size); }
			Tuple()							{}
			Tuple(size_t size)				{ m_self = create(size); }
			Tuple(const Object& rhs)		{ if(is_tuple(rhs)) assign(rhs); }
			explicit Tuple(PyObject* rhs)	{ if(is_tuple(rhs)) assign(rhs); }
			void set(int index, PyObject* obj)
			{
				// PyTuple_SetItem : 参照カウントを増加させない
				PyTuple_SetItem(m_self, index, obj);
				Py_XINCREF(obj);
			}
			Proxy operator [] (int index)					{ return Proxy(m_self, index); }
			Proxy operator [] (size_t index)				{ return Proxy(m_self, (int)index); }
			const Proxy operator [] (int index) const		{ return Proxy(m_self, index); }
			const Proxy operator [] (size_t index) const	{ return Proxy(m_self, (int)index); }
			size_t size() const	{ return m_self ? PyTuple_GET_SIZE(m_self) : 0; }
		};

		inline Object Object::apply(const Tuple& args) const
		{
			ASSERT(m_self);
			return m_self ? Object::from(PyObject_CallObject(m_self, args)) : Object();
		}

		//============================================================================================================
		/// List object client.
		class List : public Object
		{
		private:
			class Proxy
			{
			private:
				PyObject* const	m_self;
				const int		m_index;
			public:
				Proxy(PyObject* obj, int index) : m_self(obj), m_index(index)	{}
				Proxy& operator = (PyObject* obj)
				{
					// PyList_SetItem : 参照カウントを増加させない
					PyList_SetItem(m_self, m_index, obj);
					Py_XINCREF(obj);
					return *this;
				}
				operator Object () const
				{
					// PyList_GetItem : 参照カウントを増加させない
					return Object(PyList_GetItem(m_self, m_index));
				}
			};
		public:
			static bool is_list(PyObject* obj)	{ return obj && PyList_Check(obj); }
			static PyObject* create(size_t size)	{ return PyList_New((int)size); }
			List()							{}
			List(size_t size)				{ m_self = create(size); }
			List(const Object& rhs)			{ if(is_list(rhs)) assign(rhs); }
			explicit List(PyObject* rhs)	{ if(is_list(rhs)) assign(rhs); }
			void set(int index, PyObject* obj)
			{
				// PyList_SetItem : 参照カウントを増加させない
				PyList_SetItem(m_self, index, obj);
				Py_XINCREF(obj);
			}
			Proxy operator [] (int index)					{ return Proxy(m_self, index); }
			Proxy operator [] (size_t index)				{ return Proxy(m_self, (int)index); }
			const Proxy operator [] (int index) const		{ return Proxy(m_self, index); }
			const Proxy operator [] (size_t index) const	{ return Proxy(m_self, (int)index); }
			size_t size() const	{ return m_self ? PyList_GET_SIZE(m_self) : 0; }
		};

		//============================================================================================================
		/// Dictionary object client.
		class Dictionary : public Object
		{
		public:
			static Object GetItem(PyObject* self, const char* name)
			{
				// PyDict_GetItemString : returns borrowed reference
				return Object(PyDict_GetItemString(self, name));
			}
			static Object GetItem(PyObject* self, const wchar_t* name)
			{
				// PyDict_GetItem : returns borrowed reference
				return Object(PyDict_GetItem(self, StringW(name)));
			}
			static bool SetItem(PyObject* self, const char* name, PyObject* obj)
			{
				return PyDict_SetItemString(self, name, obj) != -1;
			}
			static bool SetItem(PyObject* self, const wchar_t* name, PyObject* obj)
			{
				return PyDict_SetItem(self, StringW(name), obj) != -1;
			}

		private:
			template < typename Ch >
			class Proxy
			{
			private:
				PyObject* const	m_self;
				const Ch* const m_key;
			public:
				Proxy(PyObject* self, const Ch* key) : m_self(self), m_key(key)	{}
				Proxy& operator = (PyObject* obj)
				{
					Dictionary::SetItem(m_self, m_key, obj);
					return *this;
				}
				operator Object () const
				{
					return Dictionary::GetItem(m_self, m_key);
				}
			};
		public:
			static bool is_dict(PyObject* obj)	{ return obj && PyDict_Check(obj); }
			static Dictionary create()		{ Dictionary dict; dict.attach(PyDict_New()); return dict; }
			Dictionary()	{}
			Dictionary(const Object& rhs)		{ if(is_dict(rhs)) assign(rhs); }
			Dictionary(const Dictionary& rhs) : Object(rhs)	{}
			explicit Dictionary(PyObject* rhs)	{ if(is_dict(rhs)) assign(rhs); }
			Proxy<char> operator [] (const char* key)					{ return Proxy<char>(m_self, key); }
			Proxy<wchar_t> operator [] (const wchar_t* key)				{ return Proxy<wchar_t>(m_self, key); }
			const Proxy<char> operator [] (const char* key) const		{ return Proxy<char>(m_self, key); }
			const Proxy<wchar_t> operator [] (const wchar_t* key) const	{ return Proxy<wchar_t>(m_self, key); }
		};

		inline Object Object::apply(const Tuple& args, const Dictionary& kwds) const
		{
			ASSERT(m_self);
			if(!m_self)
				return Object();
			if(!args && kwds)
                return Object::from(PyObject_Call(m_self, Tuple((size_t)0), kwds));
			else
                return Object::from(PyObject_Call(m_self, args, kwds));
		}

		//============================================================================================================
		/// Module object client.
		class Module : public Object
		{
		public:
			static Module import(const char* name)
			{
				Module module;
				module.attach(PyImport_ImportModule(const_cast<char*>(name)));
				return module;
			}
			static Module import(const wchar_t* name)
			{
				return import(StringW(name));
			}
			static Module import(PyObject* name)
			{
				Module module;
				module.attach(PyImport_Import(name));
				return module;
			}
			Module() {}
			Module(const char* name, PyMethodDef methods[], const char* doc)
			{
        #if PY_MAJOR_VERSION >= 3
          static struct PyModuleDef moduleDef = {
          PyModuleDef_HEAD_INIT,
          name, NULL, -1, methods
        };
        assign(PyModule_Create(&moduleDef));
        #else
        assign(Py_InitModule3(const_cast<char*>(name), methods, const_cast<char*>(doc)));
        #endif
			}
			Object operator [] (const char* name) const
			{
				if(!m_self)
					return null;
				// PyModule_GetDict : returns borrowed reference
				// PyDict_GetItemString : returns borrowed reference
				return Object(PyDict_GetItemString(PyModule_GetDict(m_self), name));
			}
			Object operator [] (const wchar_t* name) const
			{
				if(!m_self)
					return null;
				// PyModule_GetDict : returns borrowed reference
				// PyDict_GetItem : returns borrowed reference
				return Object(PyDict_GetItem(PyModule_GetDict(m_self), StringW(name)));
			}
			/// __dict__.
			Dictionary dict() const
			{
				// PyModule_GetDict : returns borrowed reference
				return Dictionary(PyModule_GetDict(m_self));
			}
			/// __name__.
			const char* name() const		{ return PyModule_GetName(m_self); }
			/// __file__.
			const char* file() const		{ return PyModule_GetFilename(m_self); }
			///
			bool add(const char* name, PyObject* object)
			{
				if(PyModule_AddObject(m_self, const_cast<char*>(name), object) == -1)
					return false;
				Py_XINCREF(object); // PyModule_AddObject() don't increfed object.
				return true;
			}
			bool add(const char* name, int integer)		{ return PyModule_AddIntConstant(m_self, const_cast<char*>(name), integer) != -1; }
			bool add(const char* name, const char* str)	{ return PyModule_AddStringConstant(m_self, const_cast<char*>(name), const_cast<char*>(str)) != -1; }
		};

		//============================================================================================================
		/// Python scripting host.

		/// Error information.
		struct ErrorInfo
		{
			Object	exception;
			Object	value;
			List	traceback;
		};

		class Host
		{
		private:
			bool m_init;
		public:
			Host() : m_init(false)
			{
			}
			~Host()
			{
				Terminate();
			}
			void Initialize(const char* more = null)
			{
				PySys_ResetWarnOptions();
				char program[MAX_PATH];
				GetModuleFileNameA(0, program, MAX_PATH);
				if(more)
					PathAppendA(program, more);
        #if PY_MAJOR_VERSION >= 3
        wchar_t programW[MAX_PATH];
        mbstowcs(programW, program, strlen(program) + 1);
        Py_SetProgramName(programW);
        #else
				Py_SetProgramName(program);
        #endif
				Py_Initialize();
				m_init = true;
			}
			void Terminate()
			{
				if(m_init)
				{
					PyErr_Clear(); // エラーをクリアせずに Py_Finalize() すると、abort() してしまう
					Py_Finalize();
					m_init = false;
				}
			}
		};
		//PyObject* PyImport_ImportModuleEx (char *name, PyObject *globals, PyObject *locals, PyObject *fromlist)
		//PyObject* PyImport_ReloadModule (PyObject *m) 
		//borrowedPyObject* PyImport_AddModule (char *name) 
		//PyObject* PyImport_ExecCodeModule (char *name, PyObject *co) 
		//borrowed PyObject* PyImport_GetModuleDict ()
		bool fetch_error(ErrorInfo& error)
		{
			PyObject *exception, *value, *traceback;
			PyErr_Fetch(&exception, &value, &traceback);
			PyErr_NormalizeException(&exception, &value, &traceback);
			if(!exception)
				return false;
			error.exception.attach(exception);
			error.value.attach(value);
			// エラーの種類によっては、この呼び出しで segfault するため、try-catch で囲んでしまう.
			try { error.traceback = Module::import("traceback")["extract_tb"](traceback); } catch(...) {}
			Py_XDECREF(traceback);
			return true;
		}

		//============================================================================================================
	}
}
