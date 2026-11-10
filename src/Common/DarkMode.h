/*
 DarkMode.h - Dark Mode im Stil von Anduril Lattice fuer die Windows-Oberflaeche
 von VeraCrypt (VeraCrypt.exe, "VeraCrypt Format.exe", "VeraCryptExpander.exe").

 Diese Datei veraendert ausschliesslich die DARSTELLUNG der Dialoge.
 Verschluesselung, Header-Verarbeitung, Schluesselableitung, Treiber-
 Kommunikation usw. werden NICHT beruehrt.

 Designprinzipien (angelehnt an Anduril Lattice):
  - fast schwarzer Grafit-Hintergrund, leicht abgesetzte Eingabeflaechen
  - haarfeine Linien statt 3D-Kanten, keine Rundungen, keine Verlaeufe
  - genau eine Akzentfarbe: Lime #DFF140 (Fokus, Auswahl, Standard-Schaltflaeche)
  - Beschriftungen von Schaltflaechen, Rahmen, Tabs und Spaltenkoepfen in
    Grossbuchstaben (nur wenn der Platz reicht)

 Funktionsweise:
  - Jeder eigene Dialog wird beim Lokalisieren (LocalizeDialog) bzw. per
    WH_CALLWNDPROCRET-Hook nach WM_INITDIALOG per SetWindowSubclass erfasst.
  - WM_CTLCOLOR*-Nachrichten werden nach der VeraCrypt-eigenen Behandlung
    nachbearbeitet. Von VeraCrypt gesetzte Warnfarben (rot) bleiben erhalten.
  - Schaltflaechen, Checkboxen, Optionsfelder, Gruppenrahmen, Tabs,
    Spaltenkoepfe, Fortschrittsbalken und Rahmen werden selbst gezeichnet.
  - Titelleiste (DWM), Menueleiste (UAH-Nachrichten) und Popup-Menues dunkel.

 Standardmaessig AKTIV. Abschaltbar ueber das Menue
 "Einstellungen -> Dunkler Modus" in VeraCrypt.exe oder per Registry:
   HKCU\Software\VeraCrypt  DarkMode (DWORD)  0 = aus, 1 = an
 Die Aenderung wirkt nach einem Neustart des Programms.

 Diese Datei wird nur von Common/Dlgcode.c eingebunden.
*/

#ifndef TC_HEADER_Common_DarkMode
#define TC_HEADER_Common_DarkMode

#if defined(_WIN32) && (defined(TCMOUNT) || defined(VOLFORMAT)) && !defined(SETUP) && !defined(VCSDK_DLL) && !defined(VC_COMREG)
#define VC_DARKMODE_ENABLED 1
#endif

#define VC_DARKMODE_VERSION		"1.1.0"

#ifdef VC_DARKMODE_ENABLED

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#pragma comment(lib, "comctl32.lib")

#define DM_REG_KEY			L"Software\\VeraCrypt"
#define DM_REG_VALUE		L"DarkMode"
#define DM_PROP_DIALOG		L"VCDarkModeDlg"
#define DM_PROP_CONTROL		L"VCDarkModeCtl"
#define DM_PROP_HOT			L"VCDarkModeHot"

/* ------------------------------------------------------------------ */
/* Farbpalette (Lattice)                                               */
/* ------------------------------------------------------------------ */

#define LT_BG				RGB (10, 12, 14)		/* Fensterhintergrund #0A0C0E */
#define LT_PANEL			RGB (18, 22, 26)		/* Eingabefelder, Listen #12161A */
#define LT_RAISED			RGB (26, 31, 37)		/* Schaltflaechen #1A1F25 */
#define LT_HOT				RGB (35, 42, 49)		/* Hover #232A31 */
#define LT_PRESSED			RGB (46, 54, 63)		/* gedrueckt #2E363F */
#define LT_BORDER			RGB (42, 49, 57)		/* Haarlinie #2A3139 */
#define LT_BORDER_STRONG	RGB (64, 74, 85)		/* Rahmen von Bedienelementen #404A55 */
#define LT_TEXT				RGB (228, 231, 234)		/* Text #E4E7EA */
#define LT_TEXT_MUTED		RGB (138, 147, 157)		/* Beschriftungen #8A939D */
#define LT_TEXT_DIS			RGB (88, 96, 105)		/* deaktiviert #586069 */
#define LT_ACCENT			RGB (223, 241, 64)		/* Anduril Lime #DFF140 */
#define LT_ACCENT_HOT		RGB (236, 248, 120)		/* Lime hell #ECF878 */
#define LT_ACCENT_PRESSED	RGB (185, 201, 53)		/* Lime dunkel #B9C935 */
#define LT_ACCENT_DIM		RGB (38, 44, 14)		/* Auswahlhintergrund #262C0E */
#define LT_RED				RGB (255, 53, 53)		/* Warnung #FF3535 */
#define LT_AMBER			RGB (245, 166, 35)		/* Hinweis #F5A623 */

#define DM_SUBCLASS_DIALOG		0x56434431
#define DM_SUBCLASS_LISTVIEW	0x56434432
#define DM_SUBCLASS_TAB			0x56434433
#define DM_SUBCLASS_RICHEDIT	0x56434434
#define DM_SUBCLASS_BUTTON		0x56434435
#define DM_SUBCLASS_FRAME		0x56434436
#define DM_SUBCLASS_HEADER		0x56434437
#define DM_SUBCLASS_PROGRESS	0x56434438
#define DM_SUBCLASS_COMBO		0x56434439
#define DM_SUBCLASS_STATIC		0x5643443A

/* Undokumentierte Menue-Nachrichten (uxtheme), zum Zeichnen der Menueleiste */
#define DM_WM_UAHDRAWMENU		0x0091
#define DM_WM_UAHDRAWMENUITEM	0x0092

#ifndef BS_SPLITBUTTON
#define BS_SPLITBUTTON			0x0000000CL
#endif
#ifndef BS_DEFSPLITBUTTON
#define BS_DEFSPLITBUTTON		0x0000000DL
#endif
#ifndef BS_COMMANDLINK
#define BS_COMMANDLINK			0x0000000EL
#endif
#ifndef BS_DEFCOMMANDLINK
#define BS_DEFCOMMANDLINK		0x0000000FL
#endif
#ifndef BST_HOT
#define BST_HOT					0x0200
#endif

typedef union
{
	struct { DWORD cx; DWORD cy; } rgsizeBar[2];
	struct { DWORD cx; DWORD cy; } rgsizePopup[4];
} DmUahMenuItemMetrics;

typedef struct
{
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2;
} DmUahMenuPopupMetrics;

typedef struct
{
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
} DmUahMenu;

typedef struct
{
	int iPosition;
	DmUahMenuItemMetrics umim;
	DmUahMenuPopupMetrics umpm;
} DmUahMenuItem;

typedef struct
{
	DRAWITEMSTRUCT dis;
	DmUahMenu um;
	DmUahMenuItem umi;
} DmUahDrawMenuItem;

typedef int (WINAPI *DmSetPreferredAppModeFn) (int appMode);
typedef BOOL (WINAPI *DmAllowDarkModeForWindowFn) (HWND hwnd, BOOL allow);
typedef void (WINAPI *DmVoidFn) (void);
typedef HRESULT (WINAPI *DmSetWindowThemeFn) (HWND hwnd, LPCWSTR appName, LPCWSTR idList);
typedef HRESULT (WINAPI *DmDwmSetWindowAttributeFn) (HWND hwnd, DWORD attr, LPCVOID data, DWORD size);

static BOOL DmActive = FALSE;
static BOOL DmInitDone = FALSE;
static HBRUSH DmBrushBg = NULL;
static HBRUSH DmBrushPanel = NULL;
static DmAllowDarkModeForWindowFn DmAllowDarkModeForWindowPtr = NULL;
static DmSetWindowThemeFn DmSetWindowThemePtr = NULL;
static DmDwmSetWindowAttributeFn DmDwmSetWindowAttributePtr = NULL;
static __declspec(thread) HHOOK DmThreadHook = NULL;

static void DarkMode_ApplyToControl (HWND hwnd);

/* ------------------------------------------------------------------ */
/* Einstellung                                                         */
/* ------------------------------------------------------------------ */

static BOOL DarkMode_ReadSetting (void)
{
	HKEY hkey;
	DWORD value = 1, size = sizeof (value), type = 0;

	if (RegOpenKeyExW (HKEY_CURRENT_USER, DM_REG_KEY, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW (hkey, DM_REG_VALUE, NULL, &type, (LPBYTE) &value, &size) != ERROR_SUCCESS || type != REG_DWORD)
			value = 1;
		RegCloseKey (hkey);
	}

	return value != 0;
}

static BOOL DarkMode_WriteSetting (BOOL enabled)
{
	HKEY hkey;
	DWORD value = enabled ? 1 : 0;
	BOOL ok = FALSE;

	if (RegCreateKeyExW (HKEY_CURRENT_USER, DM_REG_KEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey, NULL) == ERROR_SUCCESS)
	{
		ok = RegSetValueExW (hkey, DM_REG_VALUE, 0, REG_DWORD, (const BYTE *) &value, sizeof (value)) == ERROR_SUCCESS;
		RegCloseKey (hkey);
	}

	return ok;
}

/* Wird beim naechsten Programmstart wirksam */
BOOL DarkMode_IsEnabledSetting (void)
{
	return DarkMode_ReadSetting ();
}

BOOL DarkMode_SetEnabledSetting (BOOL enabled)
{
	return DarkMode_WriteSetting (enabled);
}

BOOL DarkMode_IsActive (void)
{
	return DmActive;
}

/* ------------------------------------------------------------------ */
/* Zeichenhilfen                                                       */
/* ------------------------------------------------------------------ */

static BOOL DmIsClass (HWND hwnd, const wchar_t *className)
{
	wchar_t name[64];
	if (!hwnd || !GetClassNameW (hwnd, name, ARRAYSIZE (name)))
		return FALSE;
	return _wcsicmp (name, className) == 0;
}

static void DmAllowDark (HWND hwnd)
{
	if (DmAllowDarkModeForWindowPtr && hwnd)
		DmAllowDarkModeForWindowPtr (hwnd, TRUE);
}

static void DmSetTheme (HWND hwnd, LPCWSTR appName, LPCWSTR idList)
{
	if (DmSetWindowThemePtr && hwnd)
		DmSetWindowThemePtr (hwnd, appName, idList);
}

static int DmScale (HDC hdc, int value)
{
	int dpi = hdc ? GetDeviceCaps (hdc, LOGPIXELSY) : 96;
	if (dpi <= 0)
		dpi = 96;
	return MulDiv (value, dpi, 96);
}

static void DmFill (HDC hdc, const RECT *rc, COLORREF color)
{
	COLORREF old = SetDCBrushColor (hdc, color);
	FillRect (hdc, rc, (HBRUSH) GetStockObject (DC_BRUSH));
	SetDCBrushColor (hdc, old);
}

static void DmFrame (HDC hdc, const RECT *rc, COLORREF color)
{
	COLORREF old = SetDCBrushColor (hdc, color);
	FrameRect (hdc, rc, (HBRUSH) GetStockObject (DC_BRUSH));
	SetDCBrushColor (hdc, old);
}

static void DmHLine (HDC hdc, int x1, int x2, int y, int thickness, COLORREF color)
{
	RECT rc;
	SetRect (&rc, x1, y, x2, y + thickness);
	DmFill (hdc, &rc, color);
}

static void DmVLine (HDC hdc, int x, int y1, int y2, int thickness, COLORREF color)
{
	RECT rc;
	SetRect (&rc, x, y1, x + thickness, y2);
	DmFill (hdc, &rc, color);
}

static void DmLine (HDC hdc, int x1, int y1, int x2, int y2, int width, COLORREF color)
{
	HPEN pen = CreatePen (PS_SOLID, width, color);
	HGDIOBJ old = SelectObject (hdc, pen);
	MoveToEx (hdc, x1, y1, NULL);
	LineTo (hdc, x2, y2);
	SelectObject (hdc, old);
	DeleteObject (pen);
}

static HFONT DmSelectFont (HWND hwnd, HDC hdc)
{
	HFONT font = (HFONT) SendMessageW (hwnd, WM_GETFONT, 0, 0);
	return (HFONT) SelectObject (hdc, font ? (HGDIOBJ) font : GetStockObject (DEFAULT_GUI_FONT));
}

/* Text in Grossbuchstaben umwandeln, wenn er in maxWidth passt */
static void DmUpperIfFits (HDC hdc, wchar_t *text, int maxWidth, UINT dtFlags)
{
	wchar_t upper[512];
	RECT calc;
	size_t len = wcslen (text);

	if (len == 0 || len >= ARRAYSIZE (upper))
		return;

	memcpy (upper, text, (len + 1) * sizeof (wchar_t));
	CharUpperBuffW (upper, (DWORD) len);

	SetRect (&calc, 0, 0, maxWidth, 1000);
	DrawTextW (hdc, upper, -1, &calc, (dtFlags & ~(DT_CENTER | DT_RIGHT | DT_VCENTER | DT_BOTTOM)) | DT_CALCRECT | DT_SINGLELINE);
	if (calc.right - calc.left <= maxWidth)
		memcpy (text, upper, (len + 1) * sizeof (wchar_t));
}

static UINT DmPrefixFlag (HWND hwnd)
{
	LRESULT ui = SendMessageW (hwnd, WM_QUERYUISTATE, 0, 0);
	return (ui & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;
}

static BOOL DmShowFocus (HWND hwnd)
{
	LRESULT ui = SendMessageW (hwnd, WM_QUERYUISTATE, 0, 0);
	return GetFocus () == hwnd && !(ui & UISF_HIDEFOCUS);
}

static void DmDarkTitleBar (HWND hwnd)
{
	BOOL dark = TRUE;
	COLORREF caption = LT_BG, text = LT_TEXT, border = LT_BORDER;
	DWORD corner = 1; /* DWMWCP_DONOTROUND - eckige Fenster wie bei Lattice */

	if (!DmDwmSetWindowAttributePtr)
		return;

	/* 20 = DWMWA_USE_IMMERSIVE_DARK_MODE (ab Win10 20H1), 19 = aeltere Win10-Builds */
	if (FAILED (DmDwmSetWindowAttributePtr (hwnd, 20, &dark, sizeof (dark))))
		DmDwmSetWindowAttributePtr (hwnd, 19, &dark, sizeof (dark));

	/* Nur Windows 11 - unter Windows 10 schlagen diese Aufrufe folgenlos fehl */
	DmDwmSetWindowAttributePtr (hwnd, 33, &corner, sizeof (corner));	/* DWMWA_WINDOW_CORNER_PREFERENCE */
	DmDwmSetWindowAttributePtr (hwnd, 34, &border, sizeof (border));	/* DWMWA_BORDER_COLOR */
	DmDwmSetWindowAttributePtr (hwnd, 35, &caption, sizeof (caption));	/* DWMWA_CAPTION_COLOR */
	DmDwmSetWindowAttributePtr (hwnd, 36, &text, sizeof (text));		/* DWMWA_TEXT_COLOR */
}

static BOOL DmIsHighContrast (void)
{
	HIGHCONTRASTW hc;
	memset (&hc, 0, sizeof (hc));
	hc.cbSize = sizeof (hc);
	if (SystemParametersInfoW (SPI_GETHIGHCONTRAST, sizeof (hc), &hc, 0))
		return (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
	return FALSE;
}

static BOOL DmHasUnderlinedFont (HWND hwnd)
{
	LOGFONTW lf;
	HFONT font = (HFONT) SendMessageW (hwnd, WM_GETFONT, 0, 0);
	if (!font || !GetObjectW (font, sizeof (lf), &lf))
		return FALSE;
	return lf.lfUnderline != 0;
}

/* Doppelpuffer fuer flimmerfreies Zeichnen */
typedef struct
{
	HDC target;
	HDC mem;
	HBITMAP bmp;
	HGDIOBJ oldBmp;
	RECT rc;
} DmBuffer;

static HDC DmBeginBuffer (DmBuffer *b, HDC target, const RECT *rc)
{
	b->target = target;
	b->rc = *rc;
	b->mem = CreateCompatibleDC (target);
	b->bmp = b->mem ? CreateCompatibleBitmap (target, rc->right - rc->left, rc->bottom - rc->top) : NULL;
	if (!b->mem || !b->bmp)
	{
		if (b->mem)
			DeleteDC (b->mem);
		b->mem = NULL;
		return target;
	}
	b->oldBmp = SelectObject (b->mem, b->bmp);
	SetWindowOrgEx (b->mem, rc->left, rc->top, NULL);
	return b->mem;
}

static void DmEndBuffer (DmBuffer *b)
{
	if (!b->mem)
		return;
	SetWindowOrgEx (b->mem, 0, 0, NULL);
	BitBlt (b->target, b->rc.left, b->rc.top, b->rc.right - b->rc.left, b->rc.bottom - b->rc.top, b->mem, 0, 0, SRCCOPY);
	SelectObject (b->mem, b->oldBmp);
	DeleteObject (b->bmp);
	DeleteDC (b->mem);
}

/* ------------------------------------------------------------------ */
/* WM_CTLCOLOR* - nach der Original-Behandlung dunkel einfaerben        */
/* ------------------------------------------------------------------ */

static LRESULT DmHandleCtlColor (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc = (HDC) wParam;
	HWND hCtl = (HWND) lParam;
	COLORREF defaultText = GetSysColor (COLOR_WINDOWTEXT);
	COLORREF text;
	LRESULT result;
	BOOL field;

	SetTextColor (hdc, defaultText);
	result = DefSubclassProc (hwnd, msg, wParam, lParam);

	field = (msg == WM_CTLCOLOREDIT)
		|| (msg == WM_CTLCOLORLISTBOX)
		|| ((HBRUSH) result == GetSysColorBrush (COLOR_WINDOW))
		|| DmIsClass (hCtl, L"Edit");

	text = GetTextColor (hdc);
	if (text == defaultText || text == GetSysColor (COLOR_BTNTEXT) || text == RGB (0, 0, 0))
	{
		if (hCtl && !IsWindowEnabled (hCtl))
			SetTextColor (hdc, LT_TEXT_DIS);
		else if (msg == WM_CTLCOLORSTATIC && !field && DmHasUnderlinedFont (hCtl))
			SetTextColor (hdc, LT_ACCENT);	/* Hyperlinks */
		else
			SetTextColor (hdc, LT_TEXT);
	}
	else if (GetRValue (text) > 150 && GetGValue (text) < 80 && GetBValue (text) < 80)
		SetTextColor (hdc, LT_RED);			/* Warnungen von VeraCrypt (rot) */
	else if (GetBValue (text) > 150 && GetRValue (text) < 80)
		SetTextColor (hdc, LT_ACCENT);		/* blaue Links */

	SetBkColor (hdc, field ? LT_PANEL : LT_BG);
	return (LRESULT) (field ? DmBrushPanel : DmBrushBg);
}

/* ------------------------------------------------------------------ */
/* Menueleiste                                                         */
/* ------------------------------------------------------------------ */

static void DmDrawMenuBarBottomLine (HWND hwnd)
{
	MENUBARINFO mbi;
	RECT rcClient, rcWindow, rcLine;
	HDC hdc;

	if (!GetMenu (hwnd))
		return;

	memset (&mbi, 0, sizeof (mbi));
	mbi.cbSize = sizeof (mbi);
	if (!GetMenuBarInfo (hwnd, OBJID_MENU, 0, &mbi))
		return;

	GetClientRect (hwnd, &rcClient);
	MapWindowPoints (hwnd, NULL, (POINT *) &rcClient, 2);
	GetWindowRect (hwnd, &rcWindow);
	OffsetRect (&rcClient, -rcWindow.left, -rcWindow.top);

	rcLine = rcClient;
	rcLine.bottom = rcLine.top;
	rcLine.top--;

	hdc = GetWindowDC (hwnd);
	if (hdc)
	{
		DmFill (hdc, &rcLine, LT_BORDER);
		ReleaseDC (hwnd, hdc);
	}
}

static LRESULT DmDrawMenuBar (HWND hwnd, DmUahMenu *pUdm)
{
	MENUBARINFO mbi;
	RECT rcWindow, rc;

	memset (&mbi, 0, sizeof (mbi));
	mbi.cbSize = sizeof (mbi);
	if (!GetMenuBarInfo (hwnd, OBJID_MENU, 0, &mbi))
		return FALSE;

	GetWindowRect (hwnd, &rcWindow);
	rc = mbi.rcBar;
	OffsetRect (&rc, -rcWindow.left, -rcWindow.top);
	rc.top -= 1;
	DmFill (pUdm->hdc, &rc, LT_BG);
	return TRUE;
}

static LRESULT DmDrawMenuBarItem (DmUahDrawMenuItem *p)
{
	wchar_t text[256];
	MENUITEMINFOW mii;
	DWORD flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	COLORREF bg = LT_BG;
	COLORREF color = LT_TEXT;
	UINT state = p->dis.itemState;
	BOOL active = (state & (ODS_HOTLIGHT | ODS_SELECTED)) != 0;

	memset (text, 0, sizeof (text));
	memset (&mii, 0, sizeof (mii));
	mii.cbSize = sizeof (mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = text;
	mii.cch = (UINT) (ARRAYSIZE (text) - 1);
	GetMenuItemInfoW (p->um.hmenu, p->umi.iPosition, TRUE, &mii);

	if (active)
		bg = LT_HOT;
	if (state & (ODS_GRAYED | ODS_DISABLED))
		color = LT_TEXT_DIS;
	else if (state & ODS_INACTIVE)
		color = LT_TEXT_MUTED;
	if (state & ODS_NOACCEL)
		flags |= DT_HIDEPREFIX;

	DmFill (p->um.hdc, &p->dis.rcItem, bg);
	if (active)
	{
		int t = DmScale (p->um.hdc, 2);
		DmHLine (p->um.hdc, p->dis.rcItem.left, p->dis.rcItem.right, p->dis.rcItem.bottom - t, t, LT_ACCENT);
	}

	SetBkMode (p->um.hdc, TRANSPARENT);
	SetTextColor (p->um.hdc, color);
	DrawTextW (p->um.hdc, text, -1, &p->dis.rcItem, flags);
	return TRUE;
}

/* ------------------------------------------------------------------ */
/* Schaltflaechen, Checkboxen, Optionsfelder, Gruppenrahmen             */
/* ------------------------------------------------------------------ */

static void DmPaintGroupBox (HWND hwnd, HDC hdc)
{
	RECT rc, textRc;
	wchar_t text[256];
	TEXTMETRICW tm;
	HFONT oldFont = DmSelectFont (hwnd, hdc);
	int lineY, pad = DmScale (hdc, 6), textLeft;
	UINT dt = DT_SINGLELINE | DT_LEFT | DT_TOP | DmPrefixFlag (hwnd);

	GetClientRect (hwnd, &rc);
	GetTextMetricsW (hdc, &tm);
	text[0] = 0;
	GetWindowTextW (hwnd, text, ARRAYSIZE (text));

	lineY = rc.top + tm.tmHeight / 2;
	textLeft = rc.left + pad + DmScale (hdc, 2);

	if (text[0])
	{
		DmUpperIfFits (hdc, text, rc.right - textLeft - pad, dt);
		SetRect (&textRc, textLeft, rc.top, rc.right - pad, rc.top + tm.tmHeight);
		DrawTextW (hdc, text, -1, &textRc, dt | DT_CALCRECT);
		if (textRc.right > rc.right - pad)
			textRc.right = rc.right - pad;
	}
	else
		SetRect (&textRc, textLeft, lineY, textLeft, lineY);

	/* Rahmen aus Haarlinien, oben mit Luecke fuer die Beschriftung */
	DmHLine (hdc, rc.left, textLeft - DmScale (hdc, 4) > rc.left ? textLeft - DmScale (hdc, 4) : rc.left, lineY, 1, LT_BORDER);
	DmHLine (hdc, text[0] ? textRc.right + DmScale (hdc, 4) : textLeft, rc.right, lineY, 1, LT_BORDER);
	DmVLine (hdc, rc.left, lineY, rc.bottom, 1, LT_BORDER);
	DmVLine (hdc, rc.right - 1, lineY, rc.bottom, 1, LT_BORDER);
	DmHLine (hdc, rc.left, rc.right, rc.bottom - 1, 1, LT_BORDER);

	/* kleiner Akzentstrich oben links */
	DmHLine (hdc, rc.left, rc.left + DmScale (hdc, 3), lineY, 1, LT_ACCENT);

	if (text[0])
	{
		RECT bgRc = textRc;
		InflateRect (&bgRc, DmScale (hdc, 3), 0);
		DmFill (hdc, &bgRc, LT_BG);
		SetBkMode (hdc, TRANSPARENT);
		SetTextColor (hdc, IsWindowEnabled (hwnd) ? LT_TEXT_MUTED : LT_TEXT_DIS);
		DrawTextW (hdc, text, -1, &textRc, dt);
	}

	SelectObject (hdc, oldFont);
}

static void DmPaintCheckOrRadio (HWND hwnd, HDC hdc, BOOL radio)
{
	RECT rc, box, textRc, calc;
	wchar_t text[512];
	TEXTMETRICW tm;
	LONG_PTR style = GetWindowLongPtrW (hwnd, GWL_STYLE);
	BOOL enabled = IsWindowEnabled (hwnd);
	LRESULT check = SendMessageW (hwnd, BM_GETCHECK, 0, 0);
	LRESULT state = SendMessageW (hwnd, BM_GETSTATE, 0, 0);
	BOOL hot = GetPropW (hwnd, DM_PROP_HOT) != NULL;
	BOOL pushed = (state & BST_PUSHED) != 0;
	BOOL multiline = (style & BS_MULTILINE) != 0;
	BOOL boxRight = (style & BS_LEFTTEXT) != 0;
	HFONT oldFont = DmSelectFont (hwnd, hdc);
	UINT dt = DmPrefixFlag (hwnd) | (multiline ? DT_WORDBREAK : (DT_SINGLELINE | DT_VCENTER));
	COLORREF boxBorder, boxFill;
	int size, gap, textHeight;

	GetClientRect (hwnd, &rc);
	DmFill (hdc, &rc, LT_BG);
	GetTextMetricsW (hdc, &tm);

	size = tm.tmAscent - DmScale (hdc, 1);
	if (size < DmScale (hdc, 11))
		size = DmScale (hdc, 11);
	gap = DmScale (hdc, 6);

	text[0] = 0;
	GetWindowTextW (hwnd, text, ARRAYSIZE (text));

	if (boxRight)
		SetRect (&textRc, rc.left, rc.top, rc.right - size - gap, rc.bottom);
	else
		SetRect (&textRc, rc.left + size + gap, rc.top, rc.right, rc.bottom);

	/* Vertikale Position (mehrzeilig: Text zentriert berechnen) */
	textHeight = tm.tmHeight;
	if (multiline && text[0])
	{
		calc = textRc;
		DrawTextW (hdc, text, -1, &calc, dt | DT_CALCRECT);
		textHeight = calc.bottom - calc.top;
		textRc.top = rc.top + ((rc.bottom - rc.top) - textHeight) / 2;
		if (textRc.top < rc.top)
			textRc.top = rc.top;
		textRc.bottom = textRc.top + textHeight;
	}

	{
		int firstLineTop = multiline ? textRc.top : rc.top + ((rc.bottom - rc.top) - tm.tmHeight) / 2;
		int boxTop = firstLineTop + (tm.tmHeight - size) / 2;
		int boxLeft = boxRight ? rc.right - size : rc.left;
		SetRect (&box, boxLeft, boxTop, boxLeft + size, boxTop + size);
	}

	if (!enabled)
	{
		boxBorder = LT_BORDER;
		boxFill = LT_BG;
	}
	else if (check == BST_CHECKED)
	{
		boxBorder = pushed ? LT_ACCENT_PRESSED : (hot ? LT_ACCENT_HOT : LT_ACCENT);
		boxFill = boxBorder;
	}
	else
	{
		boxBorder = pushed ? LT_ACCENT : (hot ? LT_TEXT_MUTED : LT_BORDER_STRONG);
		boxFill = pushed ? LT_HOT : LT_PANEL;
	}

	if (radio)
	{
		HPEN pen = CreatePen (PS_SOLID, 1, boxBorder);
		HBRUSH brush = CreateSolidBrush (check == BST_CHECKED ? LT_PANEL : boxFill);
		HGDIOBJ oldPen = SelectObject (hdc, pen);
		HGDIOBJ oldBrush = SelectObject (hdc, brush);
		Ellipse (hdc, box.left, box.top, box.right, box.bottom);
		SelectObject (hdc, oldBrush);
		DeleteObject (brush);

		if (check == BST_CHECKED)
		{
			RECT dot = box;
			HBRUSH dotBrush = CreateSolidBrush (enabled ? boxBorder : LT_TEXT_DIS);
			HPEN dotPen = CreatePen (PS_SOLID, 1, enabled ? boxBorder : LT_TEXT_DIS);
			int inset = size / 4 + 1;
			InflateRect (&dot, -inset, -inset);
			SelectObject (hdc, dotPen);
			oldBrush = SelectObject (hdc, dotBrush);
			Ellipse (hdc, dot.left, dot.top, dot.right, dot.bottom);
			SelectObject (hdc, oldBrush);
			DeleteObject (dotBrush);
			SelectObject (hdc, pen);
			DeleteObject (dotPen);
		}

		SelectObject (hdc, oldPen);
		DeleteObject (pen);
	}
	else
	{
		DmFill (hdc, &box, boxFill);
		DmFrame (hdc, &box, boxBorder);

		if (check == BST_CHECKED)
		{
			/* Haken in Hintergrundfarbe auf Lime */
			int w = DmScale (hdc, 2);
			int x1 = box.left + size * 22 / 100, y1 = box.top + size * 52 / 100;
			int x2 = box.left + size * 42 / 100, y2 = box.top + size * 72 / 100;
			int x3 = box.left + size * 78 / 100, y3 = box.top + size * 30 / 100;
			COLORREF mark = enabled ? LT_BG : LT_TEXT_DIS;
			DmLine (hdc, x1, y1, x2, y2, w, mark);
			DmLine (hdc, x2, y2, x3, y3, w, mark);
		}
		else if (check == BST_INDETERMINATE)
		{
			RECT inner = box;
			InflateRect (&inner, -(size / 4 + 1), -(size / 4 + 1));
			DmFill (hdc, &inner, enabled ? LT_ACCENT : LT_TEXT_DIS);
		}
	}

	if (text[0])
	{
		SetBkMode (hdc, TRANSPARENT);
		SetTextColor (hdc, enabled ? LT_TEXT : LT_TEXT_DIS);
		DrawTextW (hdc, text, -1, &textRc, dt);

		if (DmShowFocus (hwnd))
		{
			calc = textRc;
			DrawTextW (hdc, text, -1, &calc, dt | DT_CALCRECT);
			if (!multiline)
			{
				int h = calc.bottom - calc.top;
				calc.top = textRc.top + ((textRc.bottom - textRc.top) - h) / 2;
				calc.bottom = calc.top + h;
			}
			DmHLine (hdc, calc.left, calc.right, calc.bottom, 1, LT_ACCENT);
		}
	}

	SelectObject (hdc, oldFont);
}

static void DmPaintPushButton (HWND hwnd, HDC hdc)
{
	RECT rc, textRc;
	wchar_t text[512];
	LONG_PTR style = GetWindowLongPtrW (hwnd, GWL_STYLE);
	LONG_PTR type = style & BS_TYPEMASK;
	BOOL enabled = IsWindowEnabled (hwnd);
	LRESULT state = SendMessageW (hwnd, BM_GETSTATE, 0, 0);
	BOOL hot = GetPropW (hwnd, DM_PROP_HOT) != NULL;
	BOOL pushed = (state & BST_PUSHED) != 0 || ((style & BS_PUSHLIKE) && (state & BST_CHECKED));
	BOOL isDefault = (type == BS_DEFPUSHBUTTON || type == BS_DEFSPLITBUTTON);
	BOOL split = (type == BS_SPLITBUTTON || type == BS_DEFSPLITBUTTON);
	BOOL focus = DmShowFocus (hwnd);
	HFONT oldFont = DmSelectFont (hwnd, hdc);
	UINT dt = DmPrefixFlag (hwnd) | DT_CENTER | ((style & BS_MULTILINE) ? DT_WORDBREAK : (DT_SINGLELINE | DT_VCENTER));
	COLORREF bg, border, color;
	int pad = DmScale (hdc, 8), splitWidth = 0;

	GetClientRect (hwnd, &rc);

	if (!enabled)
	{
		bg = LT_BG; border = LT_BORDER; color = LT_TEXT_DIS;
	}
	else if (isDefault)
	{
		/* Standard-Schaltflaeche: Lime-Flaeche mit dunkler Schrift */
		bg = pushed ? LT_ACCENT_PRESSED : (hot ? LT_ACCENT_HOT : LT_ACCENT);
		border = bg;
		color = LT_BG;
	}
	else
	{
		bg = pushed ? LT_PRESSED : (hot ? LT_HOT : LT_RAISED);
		border = focus ? LT_ACCENT : (hot ? LT_TEXT_MUTED : LT_BORDER_STRONG);
		color = LT_TEXT;
	}

	DmFill (hdc, &rc, LT_BG);
	DmFill (hdc, &rc, bg);
	DmFrame (hdc, &rc, border);

	if (isDefault && focus && enabled)
	{
		RECT inner = rc;
		InflateRect (&inner, -DmScale (hdc, 2), -DmScale (hdc, 2));
		DmFrame (hdc, &inner, LT_BG);
	}

	textRc = rc;
	if (split)
	{
		int cx, cy, s;
		splitWidth = (rc.bottom - rc.top) * 4 / 5;
		if (splitWidth < DmScale (hdc, 14))
			splitWidth = DmScale (hdc, 14);
		DmVLine (hdc, rc.right - splitWidth, rc.top + DmScale (hdc, 4), rc.bottom - DmScale (hdc, 4), 1, isDefault && enabled ? LT_BG : border);
		cx = rc.right - splitWidth / 2;
		cy = (rc.top + rc.bottom) / 2;
		s = DmScale (hdc, 3);
		DmLine (hdc, cx - s, cy - s / 2, cx, cy + s / 2 + 1, DmScale (hdc, 1) + 1, color);
		DmLine (hdc, cx, cy + s / 2 + 1, cx + s, cy - s / 2, DmScale (hdc, 1) + 1, color);
		textRc.right -= splitWidth;
	}

	InflateRect (&textRc, -pad / 2, 0);

	text[0] = 0;
	GetWindowTextW (hwnd, text, ARRAYSIZE (text));
	if (text[0])
	{
		if (!(style & BS_MULTILINE))
			DmUpperIfFits (hdc, text, (textRc.right - textRc.left) - pad, dt);
		else
		{
			RECT calc = textRc;
			int h;
			DrawTextW (hdc, text, -1, &calc, dt | DT_CALCRECT);
			h = calc.bottom - calc.top;
			textRc.top = rc.top + ((rc.bottom - rc.top) - h) / 2;
			textRc.bottom = textRc.top + h;
		}
		SetBkMode (hdc, TRANSPARENT);
		SetTextColor (hdc, color);
		DrawTextW (hdc, text, -1, &textRc, dt);
	}

	SelectObject (hdc, oldFont);
}

static BOOL DmButtonPaintable (HWND hwnd, LONG_PTR *pType)
{
	LONG_PTR style = GetWindowLongPtrW (hwnd, GWL_STYLE);
	LONG_PTR type = style & BS_TYPEMASK;

	if (style & (BS_ICON | BS_BITMAP))
		return FALSE;
	if (type == BS_OWNERDRAW || type == BS_USERBUTTON || type == BS_COMMANDLINK || type == BS_DEFCOMMANDLINK)
		return FALSE;
	if (SendMessageW (hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0) || SendMessageW (hwnd, BM_GETIMAGE, IMAGE_ICON, 0))
		return FALSE;

	if (pType)
		*pType = type;
	return TRUE;
}

static void DmPaintButtonAny (HWND hwnd, HDC hdc)
{
	LONG_PTR type = 0;
	LONG_PTR style = GetWindowLongPtrW (hwnd, GWL_STYLE);

	if (!DmButtonPaintable (hwnd, &type))
		return;

	if (type == BS_GROUPBOX)
		DmPaintGroupBox (hwnd, hdc);
	else if ((type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE) && !(style & BS_PUSHLIKE))
		DmPaintCheckOrRadio (hwnd, hdc, FALSE);
	else if ((type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) && !(style & BS_PUSHLIKE))
		DmPaintCheckOrRadio (hwnd, hdc, TRUE);
	else
		DmPaintPushButton (hwnd, hdc);
}

static LRESULT CALLBACK DmButtonSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	LONG_PTR type = 0;

	switch (msg)
	{
	case WM_ERASEBKGND:
		if (DmButtonPaintable (hwnd, NULL))
			return 1;
		break;

	case WM_PAINT:
		if (DmButtonPaintable (hwnd, &type))
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint (hwnd, &ps);
			if (type == BS_GROUPBOX)
			{
				/* Gruppenrahmen ist transparent - nicht puffern, nichts ueberdecken */
				DmPaintGroupBox (hwnd, hdc);
			}
			else
			{
				RECT rc;
				DmBuffer buf;
				HDC mem;
				GetClientRect (hwnd, &rc);
				mem = DmBeginBuffer (&buf, hdc, &rc);
				DmPaintButtonAny (hwnd, mem);
				DmEndBuffer (&buf);
			}
			EndPaint (hwnd, &ps);
			return 0;
		}
		break;

	case WM_PRINTCLIENT:
		if (DmButtonPaintable (hwnd, NULL))
		{
			DmPaintButtonAny (hwnd, (HDC) wParam);
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		if (!GetPropW (hwnd, DM_PROP_HOT))
		{
			TRACKMOUSEEVENT tme;
			SetPropW (hwnd, DM_PROP_HOT, (HANDLE) 1);
			memset (&tme, 0, sizeof (tme));
			tme.cbSize = sizeof (tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
			TrackMouseEvent (&tme);
			InvalidateRect (hwnd, NULL, FALSE);
		}
		break;

	case WM_MOUSELEAVE:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			RemovePropW (hwnd, DM_PROP_HOT);
			InvalidateRect (hwnd, NULL, FALSE);
			return r;
		}

	case BM_SETSTATE:
	case BM_SETCHECK:
	case BM_SETSTYLE:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_ENABLE:
	case WM_SETTEXT:
	case WM_UPDATEUISTATE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CAPTURECHANGED:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			if (DmButtonPaintable (hwnd, &type))
			{
				if (type == BS_GROUPBOX)
				{
					if (msg == WM_SETTEXT || msg == WM_ENABLE)
					{
						RECT rc;
						HWND parent = GetParent (hwnd);
						GetWindowRect (hwnd, &rc);
						MapWindowPoints (NULL, parent, (POINT *) &rc, 2);
						InvalidateRect (parent, &rc, TRUE);
					}
				}
				else
				{
					InvalidateRect (hwnd, NULL, FALSE);
					UpdateWindow (hwnd);
				}
			}
			return r;
		}

	case WM_NCDESTROY:
		RemovePropW (hwnd, DM_PROP_HOT);
		RemoveWindowSubclass (hwnd, DmButtonSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Rahmen (Eingabefelder, Listen, Fortschritt, Statics mit Kante)       */
/* ------------------------------------------------------------------ */

static void DmPaintNcFrame (HWND hwnd)
{
	LONG_PTR ex = GetWindowLongPtrW (hwnd, GWL_EXSTYLE);
	LONG_PTR st = GetWindowLongPtrW (hwnd, GWL_STYLE);
	int thick = 0;
	HDC hdc;
	RECT rc;
	COLORREF outer = LT_BORDER_STRONG;

	if (ex & WS_EX_CLIENTEDGE)
		thick = 2;
	else if ((ex & WS_EX_STATICEDGE) || (st & WS_BORDER))
		thick = 1;

	if (!thick)
		return;

	if (DmIsClass (hwnd, L"Edit") || DmIsClass (hwnd, L"RichEdit20W") || DmIsClass (hwnd, L"RICHEDIT50W"))
	{
		if (!IsWindowEnabled (hwnd))
			outer = LT_BORDER;
		else if (GetFocus () == hwnd && !(st & ES_READONLY))
			outer = LT_ACCENT;
	}
	else if (DmIsClass (hwnd, PROGRESS_CLASSW) || DmIsClass (hwnd, L"Static"))
		outer = LT_BORDER;

	hdc = GetWindowDC (hwnd);
	if (!hdc)
		return;

	GetWindowRect (hwnd, &rc);
	OffsetRect (&rc, -rc.left, -rc.top);
	DmFrame (hdc, &rc, outer);
	if (thick == 2)
	{
		InflateRect (&rc, -1, -1);
		DmFrame (hdc, &rc, DmIsClass (hwnd, L"Static") ? LT_BG : LT_PANEL);
	}

	ReleaseDC (hwnd, hdc);
}

static LRESULT CALLBACK DmFrameSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_NCPAINT:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			DmPaintNcFrame (hwnd);
			return r;
		}

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_ENABLE:
	case WM_SIZE:
	case WM_STYLECHANGED:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			DmPaintNcFrame (hwnd);
			return r;
		}

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmFrameSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* Kombinationsfelder: Rahmen nach dem Standard-Zeichnen ueberzeichnen */
static void DmPaintComboFrame (HWND hwnd)
{
	RECT rc;
	HDC hdc = GetDC (hwnd);
	COMBOBOXINFO cbi;
	BOOL focused = FALSE;
	COLORREF border;

	if (!hdc)
		return;

	memset (&cbi, 0, sizeof (cbi));
	cbi.cbSize = sizeof (cbi);
	if (GetComboBoxInfo (hwnd, &cbi))
		focused = (GetFocus () == hwnd || (cbi.hwndItem && GetFocus () == cbi.hwndItem));

	border = !IsWindowEnabled (hwnd) ? LT_BORDER : (focused ? LT_ACCENT : LT_BORDER_STRONG);

	GetClientRect (hwnd, &rc);
	DmFrame (hdc, &rc, border);
	ReleaseDC (hwnd, hdc);
}

static LRESULT CALLBACK DmComboSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_PAINT:
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_ENABLE:
	case WM_MOUSELEAVE:
	case CB_SETCURSEL:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			DmPaintComboFrame (hwnd);
			return r;
		}

	case WM_COMMAND:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			if (HIWORD (wParam) == EN_SETFOCUS || HIWORD (wParam) == EN_KILLFOCUS)
				DmPaintComboFrame (hwnd);
			return r;
		}

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmComboSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Fortschrittsbalken: flach, Lime auf dunkler Spur                     */
/* ------------------------------------------------------------------ */

static void DmPaintProgress (HWND hwnd, HDC hdc)
{
	RECT rc, bar;
	PBRANGE range;
	int pos, span;
	LRESULT state;
	COLORREF color = LT_ACCENT;

	GetClientRect (hwnd, &rc);
	DmFill (hdc, &rc, LT_PANEL);

	memset (&range, 0, sizeof (range));
	SendMessageW (hwnd, PBM_GETRANGE, TRUE, (LPARAM) &range);
	pos = (int) SendMessageW (hwnd, PBM_GETPOS, 0, 0);
	state = SendMessageW (hwnd, PBM_GETSTATE, 0, 0);

	if (state == PBST_ERROR)
		color = LT_RED;
	else if (state == PBST_PAUSED)
		color = LT_AMBER;
	if (!IsWindowEnabled (hwnd))
		color = LT_TEXT_DIS;

	span = range.iHigh - range.iLow;
	if (span > 0 && pos > range.iLow)
	{
		if (pos > range.iHigh)
			pos = range.iHigh;
		bar = rc;
		bar.right = rc.left + (int) (((LONGLONG) (rc.right - rc.left) * (pos - range.iLow)) / span);
		DmFill (hdc, &bar, color);
	}
}

static LRESULT CALLBACK DmProgressSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	BOOL marquee = (GetWindowLongPtrW (hwnd, GWL_STYLE) & PBS_MARQUEE) != 0;

	switch (msg)
	{
	case WM_ERASEBKGND:
		if (!marquee)
			return 1;
		break;

	case WM_PAINT:
		if (!marquee)
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint (hwnd, &ps);
			DmPaintProgress (hwnd, hdc);
			EndPaint (hwnd, &ps);
			return 0;
		}
		break;

	case WM_PRINTCLIENT:
		if (!marquee)
		{
			DmPaintProgress (hwnd, (HDC) wParam);
			return 0;
		}
		break;

	case PBM_SETPOS:
	case PBM_DELTAPOS:
	case PBM_STEPIT:
	case PBM_SETRANGE:
	case PBM_SETRANGE32:
	case PBM_SETSTATE:
	case WM_ENABLE:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			InvalidateRect (hwnd, NULL, FALSE);
			return r;
		}

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmProgressSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Trennlinien und Rahmen-Statics                                      */
/* ------------------------------------------------------------------ */

static BOOL DmIsFrameStatic (HWND hwnd)
{
	LONG_PTR type = GetWindowLongPtrW (hwnd, GWL_STYLE) & SS_TYPEMASK;
	return type == SS_ETCHEDHORZ || type == SS_ETCHEDVERT || type == SS_ETCHEDFRAME
		|| type == SS_BLACKFRAME || type == SS_GRAYFRAME || type == SS_WHITEFRAME
		|| type == SS_BLACKRECT || type == SS_GRAYRECT || type == SS_WHITERECT;
}

static void DmPaintFrameStatic (HWND hwnd, HDC hdc)
{
	RECT rc;
	LONG_PTR type = GetWindowLongPtrW (hwnd, GWL_STYLE) & SS_TYPEMASK;

	GetClientRect (hwnd, &rc);
	switch (type)
	{
	case SS_ETCHEDHORZ:
		DmHLine (hdc, rc.left, rc.right, rc.top, 1, LT_BORDER);
		break;
	case SS_ETCHEDVERT:
		DmVLine (hdc, rc.left, rc.top, rc.bottom, 1, LT_BORDER);
		break;
	case SS_BLACKRECT:
	case SS_GRAYRECT:
		DmFill (hdc, &rc, LT_BORDER);
		break;
	case SS_WHITERECT:
		DmFill (hdc, &rc, LT_PANEL);
		break;
	default:
		DmFrame (hdc, &rc, LT_BORDER);
		break;
	}
}

static LRESULT CALLBACK DmStaticSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		if (DmIsFrameStatic (hwnd))
			return 1;
		break;

	case WM_PAINT:
		if (DmIsFrameStatic (hwnd))
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint (hwnd, &ps);
			DmPaintFrameStatic (hwnd, hdc);
			EndPaint (hwnd, &ps);
			return 0;
		}
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmStaticSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Listen: Spaltenkoepfe und Zeilenauswahl                             */
/* ------------------------------------------------------------------ */

static void DmPaintHeader (HWND hwnd, HDC hdc)
{
	RECT rc;
	int count = Header_GetItemCount (hwnd);
	int i;
	HFONT oldFont = DmSelectFont (hwnd, hdc);
	int pad = DmScale (hdc, 6);

	GetClientRect (hwnd, &rc);
	DmFill (hdc, &rc, LT_PANEL);
	SetBkMode (hdc, TRANSPARENT);

	for (i = 0; i < count; i++)
	{
		RECT item, textRc;
		HDITEMW hdi;
		wchar_t text[256];
		UINT dt = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;

		if (!Header_GetItemRect (hwnd, i, &item))
			continue;

		memset (text, 0, sizeof (text));
		memset (&hdi, 0, sizeof (hdi));
		hdi.mask = HDI_TEXT | HDI_FORMAT;
		hdi.pszText = text;
		hdi.cchTextMax = (int) (ARRAYSIZE (text) - 1);
		SendMessageW (hwnd, HDM_GETITEMW, i, (LPARAM) &hdi);

		if ((hdi.fmt & HDF_JUSTIFYMASK) == HDF_RIGHT)
			dt |= DT_RIGHT;
		else if ((hdi.fmt & HDF_JUSTIFYMASK) == HDF_CENTER)
			dt |= DT_CENTER;

		textRc = item;
		InflateRect (&textRc, -pad, 0);
		DmUpperIfFits (hdc, text, textRc.right - textRc.left, dt);

		SetTextColor (hdc, LT_TEXT_MUTED);
		DrawTextW (hdc, text, -1, &textRc, dt);

		DmVLine (hdc, item.right - 1, item.top + DmScale (hdc, 4), item.bottom - DmScale (hdc, 4), 1, LT_BORDER);
	}

	DmHLine (hdc, rc.left, rc.right, rc.bottom - 1, 1, LT_BORDER);
	SelectObject (hdc, oldFont);
}

static LRESULT CALLBACK DmHeaderSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			DmBuffer buf;
			HDC mem;
			HDC hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rc);
			mem = DmBeginBuffer (&buf, hdc, &rc);
			DmPaintHeader (hwnd, mem);
			DmEndBuffer (&buf);
			EndPaint (hwnd, &ps);
			return 0;
		}

	case WM_PRINTCLIENT:
		DmPaintHeader (hwnd, (HDC) wParam);
		return 0;

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmHeaderSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK DmListViewSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmListViewSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* Zeilen einer Liste: Auswahl als Lime-Text auf dunkel getoentem Grund */
static BOOL DmHandleListViewCustomDraw (HWND hwndDlg, LPARAM lParam, LRESULT appResult, LRESULT *pResult)
{
	NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW *) lParam;
	HWND list = cd->nmcd.hdr.hwndFrom;

	if (!DmIsClass (list, WC_LISTVIEWW))
		return FALSE;

	/* Wenn VeraCrypt selbst etwas anderes als "Standard" verlangt, nicht eingreifen */
	if (appResult & (CDRF_SKIPDEFAULT | CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT))
		return FALSE;

	switch (cd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = appResult | CDRF_NOTIFYITEMDRAW;
		return TRUE;

	case CDDS_ITEMPREPAINT:
		{
			int item = (int) cd->nmcd.dwItemSpec;
			BOOL selected = ListView_GetItemState (list, item, LVIS_SELECTED) != 0;

			if (selected)
			{
				cd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS);
				cd->clrText = IsWindowEnabled (list) ? LT_ACCENT : LT_TEXT_MUTED;
				cd->clrTextBk = LT_ACCENT_DIM;
			}
			else
			{
				cd->clrText = IsWindowEnabled (list) ? LT_TEXT : LT_TEXT_DIS;
				cd->clrTextBk = LT_PANEL;
			}
			*pResult = appResult | CDRF_NEWFONT;
			return TRUE;
		}
	}

	return FALSE;
}

/* ------------------------------------------------------------------ */
/* Tabs: Beschriftung in Grossbuchstaben, aktive Seite mit Lime-Linie   */
/* ------------------------------------------------------------------ */

static void DmPaintTab (HWND hwnd, HDC hdc)
{
	RECT rc, display, rowRc;
	HFONT oldFont = DmSelectFont (hwnd, hdc);
	int count = TabCtrl_GetItemCount (hwnd);
	int sel = TabCtrl_GetCurSel (hwnd);
	int i, bar = DmScale (hdc, 2);
	int rowBottom = 0;

	GetClientRect (hwnd, &rc);
	DmFill (hdc, &rc, LT_BG);

	for (i = 0; i < count; i++)
	{
		RECT item;
		if (TabCtrl_GetItemRect (hwnd, i, &item) && item.bottom > rowBottom)
			rowBottom = item.bottom;
	}

	display = rc;
	TabCtrl_AdjustRect (hwnd, FALSE, &display);
	InflateRect (&display, 2, 2);
	if (rowBottom > 0 && display.top < rowBottom)
		display.top = rowBottom;
	DmFrame (hdc, &display, LT_BORDER);

	SetRect (&rowRc, rc.left, rc.top, rc.right, display.top);
	SetBkMode (hdc, TRANSPARENT);

	for (i = 0; i < count; i++)
	{
		RECT item;
		wchar_t text[128];
		TCITEMW ti;
		UINT dt = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DmPrefixFlag (hwnd);

		memset (text, 0, sizeof (text));
		memset (&ti, 0, sizeof (ti));
		ti.mask = TCIF_TEXT;
		ti.pszText = text;
		ti.cchTextMax = (int) (ARRAYSIZE (text) - 1);
		SendMessageW (hwnd, TCM_GETITEMW, i, (LPARAM) &ti);

		if (!TabCtrl_GetItemRect (hwnd, i, &item))
			continue;

		if (i == sel)
		{
			DmFill (hdc, &item, LT_PANEL);
			DmHLine (hdc, item.left, item.right, item.bottom - bar, bar, LT_ACCENT);
		}

		DmUpperIfFits (hdc, text, (item.right - item.left) - DmScale (hdc, 4), dt);
		SetTextColor (hdc, !IsWindowEnabled (hwnd) ? LT_TEXT_DIS : (i == sel ? LT_TEXT : LT_TEXT_MUTED));
		DrawTextW (hdc, text, -1, &item, dt);
	}

	SelectObject (hdc, oldFont);
}

static LRESULT CALLBACK DmTabSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			DmBuffer buf;
			HDC mem;
			HDC hdc = BeginPaint (hwnd, &ps);
			GetClientRect (hwnd, &rc);
			mem = DmBeginBuffer (&buf, hdc, &rc);
			DmPaintTab (hwnd, mem);
			DmEndBuffer (&buf);
			EndPaint (hwnd, &ps);
			return 0;
		}

	case WM_PRINTCLIENT:
		DmPaintTab (hwnd, (HDC) wParam);
		return 0;

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmTabSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* RichEdit                                                            */
/* ------------------------------------------------------------------ */

static void DmApplyRichEditColors (HWND hwnd)
{
	CHARFORMAT2W cf;
	LRESULT modified = SendMessageW (hwnd, EM_GETMODIFY, 0, 0);

	SendMessageW (hwnd, EM_SETBKGNDCOLOR, 0, (LPARAM) LT_PANEL);

	memset (&cf, 0, sizeof (cf));
	cf.cbSize = sizeof (cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	cf.crTextColor = LT_TEXT;
	SendMessageW (hwnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cf);
	SendMessageW (hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);

	SendMessageW (hwnd, EM_SETMODIFY, (WPARAM) modified, 0);
}

static LRESULT CALLBACK DmRichEditSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_SETTEXT:
	case EM_SETTEXTEX:
	case EM_STREAMIN:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			DmApplyRichEditColors (hwnd);
			return r;
		}

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmRichEditSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Dialog-Subclass                                                     */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK DmDialogSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_CTLCOLORDLG:
		DefSubclassProc (hwnd, msg, wParam, lParam);
		SetBkColor ((HDC) wParam, LT_BG);
		SetTextColor ((HDC) wParam, LT_TEXT);
		return (LRESULT) DmBrushBg;

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSCROLLBAR:
		return DmHandleCtlColor (hwnd, msg, wParam, lParam);

	case WM_NOTIFY:
		if (lParam && ((NMHDR *) lParam)->code == NM_CUSTOMDRAW)
		{
			LRESULT appResult = DefSubclassProc (hwnd, msg, wParam, lParam);
			LRESULT result = appResult;
			if (DmHandleListViewCustomDraw (hwnd, lParam, appResult, &result))
				return result;
			return appResult;
		}
		break;

	case DM_WM_UAHDRAWMENU:
		return DmDrawMenuBar (hwnd, (DmUahMenu *) lParam);

	case DM_WM_UAHDRAWMENUITEM:
		return DmDrawMenuBarItem ((DmUahDrawMenuItem *) lParam);

	case WM_NCPAINT:
	case WM_NCACTIVATE:
		{
			LRESULT r = DefSubclassProc (hwnd, msg, wParam, lParam);
			DmDrawMenuBarBottomLine (hwnd);
			return r;
		}

	case WM_PARENTNOTIFY:
		if (LOWORD (wParam) == WM_CREATE && lParam)
			DarkMode_ApplyToControl ((HWND) lParam);
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmDialogSubclassProc, uIdSubclass);
		RemovePropW (hwnd, DM_PROP_DIALOG);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* Anwenden auf Steuerelemente und Dialoge                             */
/* ------------------------------------------------------------------ */

static void DmAddFrame (HWND hwnd)
{
	SetWindowSubclass (hwnd, DmFrameSubclassProc, DM_SUBCLASS_FRAME, 0);
	SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

static void DarkMode_ApplyToControl (HWND hwnd)
{
	wchar_t cls[64];
	LONG_PTR style;

	if (!DmActive || !hwnd || GetPropW (hwnd, DM_PROP_CONTROL))
		return;

	if (!GetClassNameW (hwnd, cls, ARRAYSIZE (cls)))
		return;

	/* Untergeordnete Dialoge (z.B. Wizard-Seiten) werden ueber ihr eigenes WM_INITDIALOG behandelt */
	if (_wcsicmp (cls, L"#32770") == 0)
		return;

	SetPropW (hwnd, DM_PROP_CONTROL, (HANDLE) 1);
	style = GetWindowLongPtrW (hwnd, GWL_STYLE);

	if (_wcsicmp (cls, L"Button") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		SetWindowSubclass (hwnd, DmButtonSubclassProc, DM_SUBCLASS_BUTTON, 0);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, L"Edit") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, (style & ES_MULTILINE) ? L"DarkMode_Explorer" : L"DarkMode_CFD", NULL);
		DmAddFrame (hwnd);
	}
	else if (_wcsicmp (cls, L"ComboBox") == 0)
	{
		COMBOBOXINFO cbi;
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_CFD", NULL);

		memset (&cbi, 0, sizeof (cbi));
		cbi.cbSize = sizeof (cbi);
		if (GetComboBoxInfo (hwnd, &cbi))
		{
			if (cbi.hwndList)
			{
				DmAllowDark (cbi.hwndList);
				DmSetTheme (cbi.hwndList, L"DarkMode_Explorer", NULL);
			}
			if (cbi.hwndItem && cbi.hwndItem != hwnd)
			{
				SetPropW (cbi.hwndItem, DM_PROP_CONTROL, (HANDLE) 1);
				DmAllowDark (cbi.hwndItem);
				DmSetTheme (cbi.hwndItem, L"DarkMode_CFD", NULL);
			}
		}
		SetWindowSubclass (hwnd, DmComboSubclassProc, DM_SUBCLASS_COMBO, 0);
	}
	else if (_wcsicmp (cls, L"ListBox") == 0 || _wcsicmp (cls, L"ComboLBox") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		DmAddFrame (hwnd);
	}
	else if (_wcsicmp (cls, L"ScrollBar") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
	}
	else if (_wcsicmp (cls, WC_LISTVIEWW) == 0)
	{
		HWND header = ListView_GetHeader (hwnd);

		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		ListView_SetBkColor (hwnd, LT_PANEL);
		ListView_SetTextBkColor (hwnd, LT_PANEL);
		ListView_SetTextColor (hwnd, LT_TEXT);

		if (header)
		{
			DmAllowDark (header);
			DmSetTheme (header, L"DarkMode_ItemsView", NULL);
			SetWindowSubclass (header, DmHeaderSubclassProc, DM_SUBCLASS_HEADER, 0);
			InvalidateRect (header, NULL, TRUE);
		}

		SetWindowSubclass (hwnd, DmListViewSubclassProc, DM_SUBCLASS_LISTVIEW, 0);
		DmAddFrame (hwnd);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, WC_TREEVIEWW) == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		TreeView_SetBkColor (hwnd, LT_PANEL);
		TreeView_SetTextColor (hwnd, LT_TEXT);
		DmAddFrame (hwnd);
	}
	else if (_wcsicmp (cls, WC_TABCONTROLW) == 0)
	{
		SetWindowSubclass (hwnd, DmTabSubclassProc, DM_SUBCLASS_TAB, 0);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, PROGRESS_CLASSW) == 0)
	{
		SetWindowSubclass (hwnd, DmProgressSubclassProc, DM_SUBCLASS_PROGRESS, 0);
		DmAddFrame (hwnd);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, L"Static") == 0)
	{
		if (DmIsFrameStatic (hwnd))
		{
			SetWindowSubclass (hwnd, DmStaticSubclassProc, DM_SUBCLASS_STATIC, 0);
			InvalidateRect (hwnd, NULL, TRUE);
		}
		if ((GetWindowLongPtrW (hwnd, GWL_EXSTYLE) & (WS_EX_STATICEDGE | WS_EX_CLIENTEDGE)) || (style & WS_BORDER))
			DmAddFrame (hwnd);
	}
	else if (_wcsicmp (cls, L"RichEdit20W") == 0 || _wcsicmp (cls, L"RichEdit20A") == 0 || _wcsicmp (cls, L"RICHEDIT50W") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		DmApplyRichEditColors (hwnd);
		SetWindowSubclass (hwnd, DmRichEditSubclassProc, DM_SUBCLASS_RICHEDIT, 0);
		DmAddFrame (hwnd);
	}
}

static BOOL CALLBACK DmApplyEnumProc (HWND hwnd, LPARAM lParam)
{
	DarkMode_ApplyToControl (hwnd);
	return TRUE;
}

void DarkMode_ApplyToDialog (HWND hwnd)
{
	if (!DmActive || !hwnd || !IsWindow (hwnd))
		return;

	/* Nur eigene Dialoge (keine System-MessageBoxen oder Datei-Dialoge) */
	if ((HINSTANCE) GetWindowLongPtrW (hwnd, GWLP_HINSTANCE) != GetModuleHandleW (NULL))
		return;

	if (GetPropW (hwnd, DM_PROP_DIALOG))
		return;

	SetPropW (hwnd, DM_PROP_DIALOG, (HANDLE) 1);

	DmAllowDark (hwnd);
	if (!(GetWindowLongPtrW (hwnd, GWL_STYLE) & WS_CHILD))
		DmDarkTitleBar (hwnd);

	SetWindowSubclass (hwnd, DmDialogSubclassProc, DM_SUBCLASS_DIALOG, 0);
	EnumChildWindows (hwnd, DmApplyEnumProc, 0);

	if (GetMenu (hwnd))
		DrawMenuBar (hwnd);

	InvalidateRect (hwnd, NULL, TRUE);
}

/* ------------------------------------------------------------------ */
/* Hook und Initialisierung                                            */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK DmCallWndRetProc (int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && lParam)
	{
		CWPRETSTRUCT *p = (CWPRETSTRUCT *) lParam;
		if (p->message == WM_INITDIALOG)
			DarkMode_ApplyToDialog (p->hwnd);
	}

	return CallNextHookEx (DmThreadHook, nCode, wParam, lParam);
}

/* Muss in jedem Thread aufgerufen werden, der Dialoge anzeigt */
void DarkMode_InstallThreadHook (void)
{
	if (!DmActive || DmThreadHook)
		return;

	DmThreadHook = SetWindowsHookExW (WH_CALLWNDPROCRET, DmCallWndRetProc, NULL, GetCurrentThreadId ());
}

void DarkMode_RemoveThreadHook (void)
{
	if (DmThreadHook)
	{
		UnhookWindowsHookEx (DmThreadHook);
		DmThreadHook = NULL;
	}
}

/* Einmalig beim Programmstart (InitApp) aufrufen */
void DarkMode_Init (void)
{
	HMODULE hUxTheme, hDwm;

	if (DmInitDone)
		return;
	DmInitDone = TRUE;

	if (!DarkMode_ReadSetting () || DmIsHighContrast ())
		return;

	hUxTheme = LoadLibraryExW (L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hUxTheme)
		return;

	DmSetWindowThemePtr = (DmSetWindowThemeFn) GetProcAddress (hUxTheme, "SetWindowTheme");
	DmAllowDarkModeForWindowPtr = (DmAllowDarkModeForWindowFn) GetProcAddress (hUxTheme, MAKEINTRESOURCEA (133));

	{
		/* Ordinal 135: SetPreferredAppMode (ab 1903) bzw. AllowDarkModeForApp (1809).
		   Der Wert 2 (ForceDark) entspricht bei 1809 TRUE. */
		DmSetPreferredAppModeFn setAppMode = (DmSetPreferredAppModeFn) GetProcAddress (hUxTheme, MAKEINTRESOURCEA (135));
		DmVoidFn refreshPolicy = (DmVoidFn) GetProcAddress (hUxTheme, MAKEINTRESOURCEA (104));
		DmVoidFn flushMenuThemes = (DmVoidFn) GetProcAddress (hUxTheme, MAKEINTRESOURCEA (136));

		if (setAppMode)
			setAppMode (2);
		if (refreshPolicy)
			refreshPolicy ();
		if (flushMenuThemes)
			flushMenuThemes ();
	}

	hDwm = LoadLibraryExW (L"dwmapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDwm)
		DmDwmSetWindowAttributePtr = (DmDwmSetWindowAttributeFn) GetProcAddress (hDwm, "DwmSetWindowAttribute");

	DmBrushBg = CreateSolidBrush (LT_BG);
	DmBrushPanel = CreateSolidBrush (LT_PANEL);

	if (!DmBrushBg || !DmBrushPanel)
		return;

	DmActive = TRUE;
	DarkMode_InstallThreadHook ();
}

#else /* !VC_DARKMODE_ENABLED */

#define DarkMode_Init()
#define DarkMode_InstallThreadHook()
#define DarkMode_RemoveThreadHook()

#endif /* VC_DARKMODE_ENABLED */

#endif /* TC_HEADER_Common_DarkMode */
