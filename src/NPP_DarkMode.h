/*
	Copyright (C) 2025  Peter C. Jones <pryrtcode@pryrt.com>

	This file is part of the source code for the CollectionInterface plugin for Notepad++

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#include <windows.h>

// extracted from https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/NppDarkMode.h
namespace NppDarkMode {
	struct Colors
	{
		COLORREF background = 0;
		COLORREF softerBackground = 0; // ctrl background color
		COLORREF hotBackground = 0;
		COLORREF pureBackground = 0;   // dlg background color
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
		COLORREF hotEdge = 0;
		COLORREF disabledEdge = 0;
	};

	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH ctrlBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH dlgBackground = nullptr;
		HBRUSH errorBackground = nullptr;

		HBRUSH edgeBrush = nullptr;
		HBRUSH hotEdgeBrush = nullptr;
		HBRUSH disabledEdgeBrush = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, ctrlBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, dlgBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))

			, edgeBrush(::CreateSolidBrush(colors.edge))
			, hotEdgeBrush(::CreateSolidBrush(colors.hotEdge))
			, disabledEdgeBrush(::CreateSolidBrush(colors.disabledEdge))
		{
		}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(ctrlBackground);		ctrlBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(dlgBackground);		dlgBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;

			::DeleteObject(edgeBrush);			edgeBrush = nullptr;
			::DeleteObject(hotEdgeBrush);		hotEdgeBrush = nullptr;
			::DeleteObject(disabledEdgeBrush);	disabledEdgeBrush = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(ctrlBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(dlgBackground);
			::DeleteObject(errorBackground);

			::DeleteObject(edgeBrush);
			::DeleteObject(hotEdgeBrush);
			::DeleteObject(disabledEdgeBrush);

			background = ::CreateSolidBrush(colors.background);
			ctrlBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			dlgBackground = ::CreateSolidBrush(colors.pureBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);

			edgeBrush = ::CreateSolidBrush(colors.edge);
			hotEdgeBrush = ::CreateSolidBrush(colors.hotEdge);
			disabledEdgeBrush = ::CreateSolidBrush(colors.disabledEdge);
		}
	};

	struct Pens
	{
		HPEN darkerTextPen = nullptr;
		HPEN edgePen = nullptr;
		HPEN hotEdgePen = nullptr;
		HPEN disabledEdgePen = nullptr;

		Pens(const Colors& colors)
			: darkerTextPen(::CreatePen(PS_SOLID, 1, colors.darkerText))
			, edgePen(::CreatePen(PS_SOLID, 1, colors.edge))
			, hotEdgePen(::CreatePen(PS_SOLID, 1, colors.hotEdge))
			, disabledEdgePen(::CreatePen(PS_SOLID, 1, colors.disabledEdge))
		{
		}

		~Pens()
		{
			::DeleteObject(darkerTextPen);		darkerTextPen = nullptr;
			::DeleteObject(edgePen);			edgePen = nullptr;
			::DeleteObject(hotEdgePen);			hotEdgePen = nullptr;
			::DeleteObject(disabledEdgePen);	disabledEdgePen = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(darkerTextPen);
			::DeleteObject(edgePen);
			::DeleteObject(hotEdgePen);
			::DeleteObject(disabledEdgePen);

			darkerTextPen = ::CreatePen(PS_SOLID, 1, colors.darkerText);
			edgePen = ::CreatePen(PS_SOLID, 1, colors.edge);
			hotEdgePen = ::CreatePen(PS_SOLID, 1, colors.hotEdge);
			disabledEdgePen = ::CreatePen(PS_SOLID, 1, colors.disabledEdge);
		}

	};

}
