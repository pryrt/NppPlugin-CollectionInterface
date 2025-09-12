// Hyperlinks.h
//
// Copyright 2002 Neal Stublen
// All rights reserved.
//
// http://www.awesoftware.com
//

#ifndef HYPERLINKS_H
#define HYPERLINKS_H

BOOL ConvertStaticToHyperlink(HWND hwndCtl);
BOOL ConvertStaticToHyperlink(HWND hwndParent, UINT uiCtlId);
COLORREF SetHyperlinkRGB(COLORREF rgbNew);	// added by ConfigUpdater plugin

#endif
