/// @file drawing.hpp
/// ê}å`ï`âÊ.
#pragma once

#include "struct.hpp"

namespace mew
{
	/// ê}å`ï`âÊ.
	namespace drawing
	{
		//==============================================================================
		// ÉCÉìÉ^ÉtÉFÅ[ÉX

		/// 
		[ uuid("0779F971-F896-41A9-88E6-1C66978ECE65") ]
		__interface IImageList2 : IImageList
		{
			HIMAGELIST get_Normal() throw();
			HIMAGELIST get_Disabled() throw();
			HIMAGELIST get_Hot() throw();

			__declspec(property(get=get_Normal  )) HIMAGELIST Normal;
			__declspec(property(get=get_Disabled)) HIMAGELIST Disabled;
			__declspec(property(get=get_Hot     )) HIMAGELIST Hot;
		};

		//======================================================================
		// Color operation

		inline COLORREF BlendRGB(COLORREF c1, COLORREF c2, int factor)
		{
			return RGB( GetRValue(c1) + ((GetRValue(c2) - GetRValue(c1)) * factor / 100),
						GetGValue(c1) + ((GetGValue(c2) - GetGValue(c1)) * factor / 100),
						GetBValue(c1) + ((GetBValue(c2) - GetBValue(c1)) * factor / 100) );
		}
		inline int MaxColorDistance(COLORREF lhs, COLORREF rhs)
		{
			int r = math::abs(GetRValue(lhs) - GetRValue(rhs));
			int g = math::abs(GetGValue(lhs) - GetGValue(rhs));
			int b = math::abs(GetBValue(lhs) - GetBValue(rhs));
			return math::max(r, g, b);
		}
		inline int MaxColorDistance(COLORREF lhs)
		{
			int r = math::abs(GetRValue(lhs));
			int g = math::abs(GetGValue(lhs));
			int b = math::abs(GetBValue(lhs));
			return math::max(r, g, b);
		}
		inline Color SysColor(INT what)
		{
			COLORREF c = ::GetSysColor(what);
			return Color(GetRValue(c), GetGValue(c), GetBValue(c));
		}

		//==============================================================================
		/// Pen.
		template < bool t_bManaged > class PenT : public CPenT<t_bManaged>
		{
			typedef CPenT<t_bManaged>	super;
		public:
			PenT() : super()	{}
			PenT(HPEN hPen) : super(hPen)	{}
			explicit PenT(COLORREF crColor, int nWidth = 1, int nPenStyle = PS_SOLID)
			{
				CreatePen(nPenStyle, nWidth, crColor);
			}
		};
		typedef PenT<true>	Pen;
		typedef PenT<false>	PenHandle;

		//==============================================================================
		/// Brush.
		template < bool t_bManaged > class BrushT : public CBrushT<t_bManaged>
		{
			typedef CBrushT<t_bManaged>	super;
		public:
			BrushT() : super()	{}
			BrushT(HBRUSH hBrush) : super(hBrush)	{}
			explicit BrushT(COLORREF crColor)
			{
				CreateSolidBrush(crColor);
			}
		};
		typedef BrushT<true>	Brush;
		typedef BrushT<false>	BrushHandle;

		//==============================================================================
		/// DC.
		template < bool t_bManaged > class DCT : public CDCT<t_bManaged>
		{
			typedef CDCT<t_bManaged> super;
		public:
			DCT() : super()	{}
			DCT(HDC hDC) : super(hDC)	{}
			void DrawLine(int x1, int y1, int x2, int y2) throw()
			{
				MoveTo(x1, y1);
				LineTo(x2, y2);
			}
			void DrawLine(HPEN pen, int x1, int y1, int x2, int y2) throw()
			{
				PenHandle penDefault = SelectPen(pen);
				DrawLine(x1, y1, x2, y2);
				SelectPen(penDefault);
			}
			void DrawRect(const RECT& rc, COLORREF penColor, COLORREF brushColor) throw()
			{
				Pen pen(penColor);
				Brush brush(brushColor);
				PenHandle penDefault = SelectPen(pen);
				BrushHandle brushDefult = SelectBrush(brush);
				Rectangle(&rc);
				SelectPen(penDefault);
				SelectBrush(brushDefult);
			}
			using super::DrawText;
			void DrawText(HFONT font, PCWSTR text, RECT* rc, DWORD dt)
			{
				CFontHandle fontDefault = SelectFont(font);
				DrawText(text, -1, rc, dt);
				SelectFont(fontDefault);
			}
			SIZE GetTextExtent(HFONT font, PCWSTR text)
			{
				SIZE sz;
				CFontHandle fontDefault = SelectFont(font);
				::GetTextExtentPoint32(m_hDC, text, lstrlen(text), &sz);
				SelectFont(fontDefault);
				return sz;
			}
		};
		typedef DCT<true>	DC;
		typedef DCT<false>	DCHandle;
	}
}
