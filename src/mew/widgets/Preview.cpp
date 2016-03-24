// Preview.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "impl/WTLControls.hpp"
#include "shell.hpp"
#include "drawing.hpp"

using namespace io;
using namespace drawing;

//==============================================================================

namespace
{
	typedef WindowImplBase	PreviewBase;

	const DWORD MEW_WS_PREVIEW    = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	const DWORD MEW_WS_EX_PREVIEW = WS_EX_OVERLAPPEDWINDOW | WS_EX_TOOLWINDOW;

	class Drawable
	{
	public:
		virtual ~Drawable()	{}
		virtual string GetText(IWindow* owner)	{ return null; }
		virtual void Draw(IWindow* owner, DC dc, const Size& screen) = 0;
	};

	class MessageDrawable : public Drawable
	{
	private:
		string	m_text;
		DWORD	m_flags;
	public:
		MessageDrawable(string text, DWORD flags = DT_CENTER | DT_NOPREFIX)
			: m_text(text), m_flags(flags)
		{ }
		virtual void Draw(IWindow* owner, DC dc, const Size& screen)
		{
			Rect bounds(0, 0, screen.w, screen.h);
			dc.FillRect(&bounds, ::GetSysColorBrush(COLOR_WINDOW));
			if(m_flags & DT_CENTER)
			{
				Rect rc = bounds;
				dc.DrawText(m_text.str(), m_text.length(), &rc, m_flags | DT_CALCRECT);
				rc.x = (bounds.w - rc.w) / 2;
				rc.y = (bounds.h - rc.h) / 2;
				dc.DrawText(m_text.str(), m_text.length(), &rc, m_flags);
			}
			else
			{
				dc.DrawText(m_text.str(), m_text.length(), &bounds, m_flags);
			}
		}
	};

	class NotSupported : public MessageDrawable
	{
	public:
		NotSupported(string name)
			: MessageDrawable(string::load(IDS_PREVIEW_ERROR, name))
		{ }
	};

	class NowLoading : public MessageDrawable
	{
	public:
		NowLoading(string name)
			: MessageDrawable(string::load(IDS_PREVIEW_LOADING, name))
		{ }
	};

	class Image : public Drawable
	{
	private:
		CBitmap m_bitmap;
		Size	m_size;
		int		m_depth;

	private:
		Image(HBITMAP hBitmap, Size sz, Gdiplus::PixelFormat format) : m_bitmap(hBitmap), m_size(sz)
		{
			using namespace Gdiplus;
			switch(format)
			{
			case PixelFormat1bppIndexed:
				m_depth =  1; break;
			case PixelFormat4bppIndexed:
				m_depth =  4; break;
			case PixelFormat8bppIndexed:
				m_depth =  8; break;
			case PixelFormat16bppARGB1555:
			case PixelFormat16bppGrayScale:
			case PixelFormat16bppRGB555:
			case PixelFormat16bppRGB565:
				m_depth = 16; break;
			case PixelFormat24bppRGB:
				m_depth = 24; break;
			case PixelFormat32bppARGB:
			case PixelFormat32bppPARGB:
			case PixelFormat32bppRGB:
				m_depth = 32; break;
			case PixelFormat48bppRGB:
				m_depth = 48; break;
			case PixelFormat64bppARGB:
			case PixelFormat64bppPARGB:
				m_depth = 64; break;
			default:
				m_depth =  0; break;
			}
		}
		~Image()
		{
			if(m_bitmap)
				m_bitmap.DeleteObject();
		}

	public:
		static Drawable* FromFile(PCWSTR filename)
		{
			CAutoPtr<Gdiplus::Image> image(Gdiplus::Image::FromFile(filename));
			if(!image || image->GetLastStatus() != Gdiplus::Ok)
				return null;
			Rect rc(0, 0, image->GetWidth(), image->GetHeight());

			DC dcScreen = GetDC(null);
			CBitmapHandle bitmap;
			bitmap.CreateCompatibleBitmap(dcScreen, rc.w, rc.h);
			DC dc;
			dc.CreateCompatibleDC(dcScreen);
			ReleaseDC(null, dcScreen);
			CBitmap old = dc.SelectBitmap(bitmap);
			dc.FillSolidRect(&rc, RGB(255, 255, 255));
			{
				Gdiplus::Graphics g(dc);
				g.DrawImage(image, 0, 0, rc.w, rc.h);
			}
			dc.SelectBitmap(old);
			dc.DeleteDC();
			return new Image(bitmap, rc.size, image->GetPixelFormat());
		}
		virtual string GetText(IWindow* owner)
		{
			int scale = (int)(100.0 * GetScale(owner, owner->ClientSize));
			return string::format(L"$1 x $2 x $3 ($4%)", m_size.w, m_size.h, m_depth, scale);
		}
		double GetScale(IWindow* owner, const Size& sz) const
		{
			const double RATIO = 0.5;
			double rW = (double)sz.w / m_size.w;
			double rH = (double)sz.h / m_size.h;
			double r;
			if(::IsZoomed(owner->Handle))
				r = math::min(rW, rH);
			else
				r = rW * RATIO + rH * (1.0 - RATIO);
			if(r > 1.0)
				r = 1.0;
			return r;
		}
		virtual void Draw(IWindow* owner, DC dc, const Size& screen)
		{
			if(screen.empty())
				return;
			double r = GetScale(owner, screen);
			int w = (int)(m_size.w * r);
			int h = (int)(m_size.h * r);
			int x = (screen.w - w) / 2;
			int y = (screen.h - h) / 2;

			DC srcdc;
			srcdc.CreateCompatibleDC(dc);
			srcdc.SelectBitmap(m_bitmap);
			dc.SetStretchBltMode(HALFTONE);
			dc.StretchBlt(x, y, w, h, srcdc, 0, 0, m_size.w, m_size.h, SRCCOPY);
			if(dc.ExcludeClipRect(x, y, x+w, y+h) != NULLREGION)
			{
				Rect rc(0, 0, screen.w, screen.h);
				dc.FillSolidRect(&rc, RGB(255, 255, 255));
			}
			srcdc.DeleteDC();
		}
	};

	class Text : public Drawable
	{
	private:
		static const int MAX_TEXT = 4096;
		char	m_text[MAX_TEXT];
		DWORD	m_length;

	public:
		static Drawable* FromFile(PCWSTR filename)
		{
			HANDLE handle = ::CreateFile(filename, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
			if(handle && handle != INVALID_HANDLE_VALUE)
			{
				Text* p = new Text();
				p->m_length = 0;
				::ReadFile(handle, p->m_text, MAX_TEXT, &p->m_length, null);
				::CloseHandle(handle);
				return p;
			}
			return null;
		}
		virtual void Draw(IWindow* owner, DC dc, const Size& screen)
		{
			Rect rc(0, 0, screen.w, screen.h);
			dc.FillRect(&rc, ::GetSysColorBrush(COLOR_WINDOW));
			::DrawTextA(dc, m_text, m_length, &rc, DT_NOPREFIX | DT_EXPANDTABS);
			// 一行なら、以下がベスト。
			// ::ExtTextOutA(dc, 0, 0, ETO_OPAQUE, &rc, m_text, m_length, NULL);
		}
	};

	class Executable
	{
	public:
		static Drawable* FromFile(PCWSTR filename)
		{
			io::Version ver;
			if(ver.Open(filename))
			{
				PCTSTR name    = ver.QueryValue(_T("ProductName"));
				PCTSTR version = ver.QueryValue(_T("FileVersion"));
				PCTSTR author  = ver.QueryValue(_T("LegalCopyright"));
				PCTSTR comment = ver.QueryValue(_T("Comments"));
				string text = string::format(
					L"名前\t\t: $1\n"
					L"バージョン\t: $2\n"
					L"著作者\t\t: $3\n"
					L"コメント\t\t: $4",
					name, version, author, comment);
				ver.Close();
				return new MessageDrawable(text, DT_NOPREFIX | DT_EXPANDTABS);
			}
			return null;
		}
	};

	typedef Drawable* (*LoaderProc)(PCWSTR filename);
	static const struct Loader
	{
		PCWSTR extension;
		LoaderProc	proc;
	}
	LOADER_MAP [] =
	{
		L"ahk"		, Text::FromFile,
		L"bat"		, Text::FromFile,
		L"bmp"		, Image::FromFile,
		L"c"		, Text::FromFile,
		L"cmd"		, Text::FromFile,
		L"cpp"		, Text::FromFile,
		L"cs"		, Text::FromFile,
		L"css"		, Text::FromFile,
		L"csv"		, Text::FromFile,
		L"cue"		, Text::FromFile,
		L"cxx"		, Text::FromFile,
		L"def"		, Text::FromFile,
		L"diff"		, Text::FromFile,
		L"dll"		, Executable::FromFile,
		L"emf"		, Image::FromFile,
		L"exe"		, Executable::FromFile,
		L"exif"		, Image::FromFile,
		L"gif"		, Image::FromFile,
		L"h"		, Text::FromFile,
		L"hpp"		, Text::FromFile,
		L"htm"		, Text::FromFile,
		L"html"		, Text::FromFile,
		L"hxx"		, Text::FromFile,
		L"ico"		, Image::FromFile,
		L"ini"		, Text::FromFile,
		L"inl"		, Text::FromFile,
		L"java"		, Text::FromFile,
		L"jpe"		, Image::FromFile,
		L"jpeg"		, Image::FromFile,
		L"jpg"		, Image::FromFile,
		L"js"		, Text::FromFile,
		L"kbm"		, Text::FromFile,
		L"log"		, Text::FromFile,
		L"m"		, Text::FromFile,
		L"manifest"	, Text::FromFile,
		L"patch"	, Text::FromFile,
		L"pl"		, Text::FromFile,
		L"png"		, Image::FromFile,
		L"py"		, Text::FromFile,
		L"pyw"		, Text::FromFile,
		L"rb"		, Text::FromFile,
		L"reg"		, Text::FromFile,
		L"rle"		, Image::FromFile,
		L"sgml"		, Text::FromFile,
		L"sgm"		, Text::FromFile,
		L"sh"		, Text::FromFile,
		L"shtml"	, Text::FromFile,
		L"shtm"		, Text::FromFile,
		L"spi"		, Executable::FromFile,
		L"text"		, Text::FromFile,
		L"tif"		, Image::FromFile,
		L"tiff"		, Image::FromFile,
		L"tsv"		, Text::FromFile,
		L"txt"		, Text::FromFile,
		L"vb"		, Text::FromFile,
		L"vbs"		, Text::FromFile,
		L"wmf"		, Image::FromFile,
		L"xml"		, Text::FromFile,
		L"xsl"		, Text::FromFile,
	};
	static bool operator < (const Loader& lhs, PCWSTR rhs)
	{
		return _wcsicmp(lhs.extension, rhs) < 0;
	}
	static bool operator == (const Loader& lhs, PCWSTR rhs)
	{
		return _wcsicmp(lhs.extension, rhs) == 0;
	}

	LoaderProc GetLoader(PCWSTR extension)
	{
		if(str::empty(extension))
			return null;
        const Loader* begin = LOADER_MAP;
		const Loader* end   = begin + lengthof(LOADER_MAP);
		const Loader* found = algorithm::binary_search(begin, end, extension+1);
		if(found != end)
			return found->proc;
		else
			return null;
	}
}


//==============================================================================

namespace mew { namespace ui {

class Preview :
	public WindowImpl
	<
		CWindowImplEx< Preview, PreviewBase >,
		implements<IPreview, IWindow, ISignal, IDisposable>
	>
{
private:
	CAutoPtr<Drawable> m_drawable;
	ref<IEntry> m_entry;
	ref<IEntry> m_linked;
	HANDLE	m_complete;
	bool	m_HideOnOwnerMinimize; // FIXME: こんなことしなくてもよいと思うんだが？

	enum
	{
		LOAD_TIMEOUT	= 200,
	};

public:
	Preview()
	{
		m_complete = ::CreateEvent(null, true, false, null);
		m_HideOnOwnerMinimize = false;
	}
	~Preview()
	{
		::CloseHandle(m_complete);
	}
	void DoCreate(CWindowEx parent)
	{
		ASSERT(parent);
		Rect rc;
		parent.GetWindowRect(&rc);
		::InflateRect(&rc, -rc.w/3, -rc.h/3);
		__super::DoCreate(parent, &rc, DirNone, MEW_WS_PREVIEW, MEW_WS_EX_PREVIEW);
		ASSERT(GetWindow(GW_OWNER) == parent);
		ImmAssociateContext(m_hWnd, null);
	}
	void Clear()
	{
		m_drawable.Free();
		m_entry.clear();
		m_linked.clear();
	}
	void HandleDestroy()
	{
		Clear();
	}

public: // msg map
	DECLARE_WND_CLASS_EX(L"mew.ui.Preview", CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS, -1)

	BEGIN_MSG_MAP_(HandleWindowMessage)
		MESSAGE_HANDLER(WM_MOUSEWHEEL	, OnMouseWheel)
		MESSAGE_HANDLER(WM_LBUTTONDOWN	, OnLRButtonDown)
		MESSAGE_HANDLER(WM_RBUTTONDOWN	, OnLRButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
		MESSAGE_HANDLER(WM_CONTEXTMENU	, OnContextMenu)
		MESSAGE_HANDLER(WM_SHOWWINDOW 	, OnShowWindow)
		MESSAGE_HANDLER(WM_CLOSE		, OnClose)
		MESSAGE_HANDLER(WM_KEYDOWN		, OnKeyDown)
		MESSAGE_HANDLER(WM_PAINT		, OnPaint)
		MESSAGE_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
		CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
	END_MSG_MAP()

	void FocusOwner()
	{
		if(IsZoomed())
			ShowWindow(SW_SHOWNORMAL);
		::SetFocus(GetWindow(GW_OWNER));
	}

	void ToggleZoomed()
	{
		if(IsZoomed())
			ShowWindow(SW_SHOWNORMAL);
		else
			ShowWindow(SW_MAXIMIZE);
	}

	LRESULT OnMouseWheel(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		int wheel = GET_WHEEL_DELTA_WPARAM(wParam);
		MSG msg = { m_hWnd, WM_KEYDOWN, (wheel < 0 ? VK_DOWN : VK_UP), 0 };
		m_Extensions.ProcessKeymap(this, &msg);
		return 0;
	}

	LRESULT OnLButtonDblClk(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ToggleZoomed();
		return 0;
	}

	LRESULT OnLRButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(m_entry)
		{
			Point ptClient(GET_XY_LPARAM(lParam));
			if(DragDetect(ptClient))
			{
				ref<IDragSource> source(__uuidof(DragSource));
				source->AddIDList(m_entry->ID);
				source->DoDragDrop(DropEffectCopy | DropEffectMove | DropEffectLink);
				return 0;
			}
		}
		bHandled = false;
		return 0;
	}

	LRESULT OnContextMenu(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(m_entry)
		{
			Point ptScreen(GET_XY_LPARAM(lParam));
			if(ptScreen.x < 0 || ptScreen.y < 0)
			{
				ptScreen = Point::Zero;
				ClientToScreen(&ptScreen);
			}
			ref<IContextMenu> menu;
			if SUCCEEDED(m_entry->QueryObject(&menu))
			{
				if(CMenu popup = afx::SHBeginContextMenu(menu))
				{
					UINT cmd = afx::SHPopupContextMenu(menu, popup, ptScreen);
					afx::SHEndContextMenu(menu, cmd, m_hWnd);
					return 0;
				}
			}
		}
		bHandled = false;
		return 0;
	}

	LRESULT OnShowWindow(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
	{
		if(lParam == 0)
		{
			if(wParam)
				Load();
		}
		return 0;
	}

	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&)
	{	// 閉じずに、隠す
		this->Visible = false;
		return 0;
	}

	LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
	{
		switch(wParam)
		{
		case VK_ESCAPE:
		case VK_TAB:
			FocusOwner();
			break;
		case VK_RETURN:
			ToggleZoomed();
			break;
		case VK_SPACE:
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		default:
			bHandled = false;
			break;
		}
		return 0;
	}

	LRESULT OnUpdateUIState(UINT, WPARAM, LPARAM, BOOL&)
	{
		if(IsWindowVisible())
		{
			CWindowEx owner = GetWindow(GW_OWNER);
			if(owner.IsIconic() || !owner.IsWindowVisible())
			{
				m_HideOnOwnerMinimize = true;
				ShowWindow(SW_HIDE);
			}
		}
		else if(m_HideOnOwnerMinimize)
		{
			CWindowEx owner = GetWindow(GW_OWNER);
			if(!owner.IsIconic() && owner.IsWindowVisible())
			{
				m_HideOnOwnerMinimize = false;
				ShowWindow(SW_SHOW);
			}
		}
		return 0;
	}
	
	LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&)
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(&ps);
		HGDIOBJ hOldFont = ::SelectObject(hDC, AtlGetDefaultGuiFont());
		OnDraw(hDC, ps.rcPaint);
		::SelectObject(hDC, hOldFont);
		EndPaint(&ps);
		return 0;
	}

	void OnDraw(DC dc, const RECT& update)
	{
		if(m_drawable)
		{
			m_drawable->Draw(this, dc, ClientSize);
		}
		else
		{
			dc.FillRect(&update, ::GetSysColorBrush(COLOR_WINDOW));
		}
	}

	void HandleUpdateLayout()
	{
		if(IsWindowVisible())
		{
			Load();
			if(m_linked && m_drawable)
				UpdateTitle(m_linked, m_drawable->GetText(this));
		}
	}

public: // IPreview
	HRESULT GetContents(REFINTF pp)
	{
		return objcpy(m_entry, pp);
	}
	HRESULT SetContents(IUnknown* p)
	{
		if(objcmp(m_entry, p))
			return S_OK;
		Invalidate();
		if(ref<IEntry> entry = cast(p))
		{
			SetEntry(entry);
		}
		else
		{
			this->Name = L"";
			Clear();
		}
		return S_OK;
	}

private:
	void SetEntry(IEntry* entry)
	{
		this->Name = entry->Name;
		m_entry = entry;
		m_linked.clear();
		m_drawable.Free();
		if(!m_entry)
			return;
		if(!IsWindowVisible())
			return;
		Load();
	}
	static bool CanPreview(IEntry* entry)
	{
		string path = entry->Path;
		PCWSTR filename = path.str();
		if(!path)
			return false;
		DWORD attrs = GetFileAttributes(filename);
		if(attrs == -1 || (attrs & FILE_ATTRIBUTE_DIRECTORY))
			return false;
		if(!GetLoader(::PathFindExtension(filename)))
			return false;
		return true;
	}

	/// @result スワップされたらtrue。
	template < typename T >
	static bool AutoPtrSwapIfNull(CAutoPtr<T>& lhs, CAutoPtr<T>& rhs)
	{
		if(::InterlockedCompareExchangePointer((void**)&lhs.m_p, rhs, null))
			return false;
		rhs.Detach();
		return true;
	}

	template < typename T >
	static void AutoPtrSwap(CAutoPtr<T>& lhs, CAutoPtr<T>& rhs)
	{
		rhs.m_p = (T*)::InterlockedExchangePointer((void**)&lhs.m_p, rhs);
	}

	/// @result 更新されたらtrue。
	bool Load()
	{
		if(m_drawable || !m_entry)
			return false;
		m_linked.clear();
		m_entry->GetLinked(&m_linked);
		if(!CanPreview(m_linked))
		{
			m_drawable.Attach(new NotSupported(m_entry->Name));
			return true;
		}
		// 非同期読み込み
		::ResetEvent(m_complete);
		::QueueUserWorkItem(AsyncLoad, this, WT_EXECUTEDEFAULT);
		if(::WaitForSingleObject(m_complete, LOAD_TIMEOUT) == WAIT_TIMEOUT)
		{
			if(!m_drawable)
			{
				CAutoPtr<Drawable> p(new NowLoading(m_entry->Name));
				AutoPtrSwapIfNull(m_drawable, p);
			}
		}
		return true;
	}
	volatile void AsyncLoad()
	{
		// 読み込み中に変化する場合があるため、ローカル変数に保存しておく。
		ref<IEntry> linked = m_linked;
		if(!linked)
			return;
		CAutoPtr<Drawable> drawable;
		try
		{
			string path = linked->Path;
			if(!!path)
			{
				PCWSTR filename = path.str();
				PCWSTR extension = ::PathFindExtension(filename);
				if(LoaderProc loader = GetLoader(extension))
					drawable.Attach(loader(filename));
			}
		}
		catch(...)
		{
		}
		if(!drawable)
			drawable.Attach(new NotSupported(linked->Name));
		//
		ASSERT(drawable);
		string text = drawable->GetText(this);
		// すでに別のファイルの読み込みを求められている。
		if(linked != ((volatile Preview*)this)->m_linked.get())
			return;
		if(!IsWindow())
			return; 
		AutoPtrSwap(m_drawable, drawable);
		::SetEvent(m_complete);
		UpdateTitle(linked, text);
		Invalidate();
	}
	static DWORD WINAPI AsyncLoad(LPVOID pThis)
	{
		((Preview*)pThis)->AsyncLoad();
		return 0;
	}
	void UpdateTitle(IEntry* linked, const string& text)
	{
		if(text)
			this->Name = string::format(L"$1 : $2", linked->Name, text);
	}
};

} } // namespace

//==============================================================================

AVESTA_EXPORT( Preview )
