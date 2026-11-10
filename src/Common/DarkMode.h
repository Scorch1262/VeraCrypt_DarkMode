/*
 DarkMode.h - Dark Mode fuer die Windows-Oberflaeche von VeraCrypt
 (VeraCrypt.exe, "VeraCrypt Format.exe", "VeraCrypt Expander.exe").

 Diese Datei veraendert ausschliesslich die DARSTELLUNG der Dialoge.
 Verschluesselung, Header-Verarbeitung, Schluesselableitung, Treiber-
 Kommunikation usw. werden NICHT beruehrt.

 Funktionsweise:
  - Pro GUI-Thread wird ein WH_CALLWNDPROCRET-Hook installiert. Sobald ein
    Dialog aus dem eigenen Modul WM_INITDIALOG verarbeitet hat, wird er
    (per SetWindowSubclass) dunkel eingefaerbt.
  - WM_CTLCOLOR*-Nachrichten werden nach der VeraCrypt-eigenen Behandlung
    nachbearbeitet: Hintergrund dunkel, Standardtextfarbe hell. Von
    VeraCrypt gesetzte Sonderfarben (z.B. rote PIM-Warnung) bleiben erhalten.
  - Titelleiste (DWM), Menueleiste (UAH-Nachrichten), Popup-Menues,
    Listen, Tabs, RichEdit, Scrollbars werden dunkel dargestellt.

 Standardmaessig AKTIV. Abschaltbar ueber das Menue
 "Einstellungen -> Dark Mode" in VeraCrypt.exe oder per Registry:
   HKCU\Software\VeraCrypt  DarkMode (DWORD)  0 = aus, 1 = an
 Die Aenderung wirkt nach einem Neustart des Programms.

 Diese Datei wird nur von Common/Dlgcode.c eingebunden.
*/

#ifndef TC_HEADER_Common_DarkMode
#define TC_HEADER_Common_DarkMode

#if defined(_WIN32) && (defined(TCMOUNT) || defined(VOLFORMAT)) && !defined(SETUP) && !defined(VCSDK_DLL) && !defined(VC_COMREG)
#define VC_DARKMODE_ENABLED 1
#endif

#define VC_DARKMODE_VERSION		"1.0.0"

#ifdef VC_DARKMODE_ENABLED

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#pragma comment(lib, "comctl32.lib")

#define DM_REG_KEY			L"Software\\VeraCrypt"
#define DM_REG_VALUE		L"DarkMode"
#define DM_PROP_DIALOG		L"VCDarkModeDlg"
#define DM_PROP_CONTROL		L"VCDarkModeCtl"

#define DM_COLOR_BG			RGB (32, 32, 32)
#define DM_COLOR_CTRL_BG	RGB (45, 45, 45)
#define DM_COLOR_HOT		RGB (62, 62, 62)
#define DM_COLOR_BORDER		RGB (90, 90, 90)
#define DM_COLOR_TEXT		RGB (232, 232, 232)
#define DM_COLOR_TEXT_DIS	RGB (140, 140, 140)

#define DM_SUBCLASS_DIALOG		0x56434431
#define DM_SUBCLASS_LISTVIEW	0x56434432
#define DM_SUBCLASS_TAB			0x56434433
#define DM_SUBCLASS_RICHEDIT	0x56434434

/* Undokumentierte Menue-Nachrichten (uxtheme), zum Zeichnen der Menueleiste */
#define DM_WM_UAHDRAWMENU		0x0091
#define DM_WM_UAHDRAWMENUITEM	0x0092

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
static HMODULE DmModule = NULL;
static HBRUSH DmBrushBg = NULL;
static HBRUSH DmBrushCtrl = NULL;
static HBRUSH DmBrushHot = NULL;
static HBRUSH DmBrushBorder = NULL;
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
/* Hilfsfunktionen                                                     */
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

static void DmDarkTitleBar (HWND hwnd)
{
	BOOL dark = TRUE;
	if (!DmDwmSetWindowAttributePtr)
		return;

	/* 20 = DWMWA_USE_IMMERSIVE_DARK_MODE (ab Win10 20H1), 19 = aeltere Win10-Builds */
	if (FAILED (DmDwmSetWindowAttributePtr (hwnd, 20, &dark, sizeof (dark))))
		DmDwmSetWindowAttributePtr (hwnd, 19, &dark, sizeof (dark));
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
		SetTextColor (hdc, (hCtl && !IsWindowEnabled (hCtl)) ? DM_COLOR_TEXT_DIS : DM_COLOR_TEXT);

	SetBkColor (hdc, field ? DM_COLOR_CTRL_BG : DM_COLOR_BG);
	return (LRESULT) (field ? DmBrushCtrl : DmBrushBg);
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
		FillRect (hdc, &rcLine, DmBrushBg);
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
	FillRect (pUdm->hdc, &rc, DmBrushBg);
	return TRUE;
}

static LRESULT DmDrawMenuBarItem (DmUahDrawMenuItem *p)
{
	wchar_t text[256];
	MENUITEMINFOW mii;
	DWORD flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	HBRUSH bg = DmBrushBg;
	COLORREF color = DM_COLOR_TEXT;
	UINT state = p->dis.itemState;

	memset (text, 0, sizeof (text));
	memset (&mii, 0, sizeof (mii));
	mii.cbSize = sizeof (mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = text;
	mii.cch = (UINT) (ARRAYSIZE (text) - 1);
	GetMenuItemInfoW (p->um.hmenu, p->umi.iPosition, TRUE, &mii);

	if (state & (ODS_HOTLIGHT | ODS_SELECTED))
		bg = DmBrushHot;
	if (state & (ODS_GRAYED | ODS_DISABLED))
		color = DM_COLOR_TEXT_DIS;
	if (state & ODS_NOACCEL)
		flags |= DT_HIDEPREFIX;

	FillRect (p->um.hdc, &p->dis.rcItem, bg);
	SetBkMode (p->um.hdc, TRANSPARENT);
	SetTextColor (p->um.hdc, color);
	DrawTextW (p->um.hdc, text, -1, &p->dis.rcItem, flags);
	return TRUE;
}

/* ------------------------------------------------------------------ */
/* Subclass-Prozeduren                                                 */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK DmDialogSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_CTLCOLORDLG:
		DefSubclassProc (hwnd, msg, wParam, lParam);
		SetBkColor ((HDC) wParam, DM_COLOR_BG);
		SetTextColor ((HDC) wParam, DM_COLOR_TEXT);
		return (LRESULT) DmBrushBg;

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSCROLLBAR:
		return DmHandleCtlColor (hwnd, msg, wParam, lParam);

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

static LRESULT CALLBACK DmListViewSubclassProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg)
	{
	case WM_NOTIFY:
		{
			NMHDR *hdr = (NMHDR *) lParam;
			if (hdr && hdr->code == NM_CUSTOMDRAW && hdr->hwndFrom == ListView_GetHeader (hwnd))
			{
				NMCUSTOMDRAW *cd = (NMCUSTOMDRAW *) lParam;
				if (cd->dwDrawStage == CDDS_PREPAINT)
					return CDRF_NOTIFYITEMDRAW;
				if (cd->dwDrawStage == CDDS_ITEMPREPAINT)
				{
					SetTextColor (cd->hdc, DM_COLOR_TEXT);
					return CDRF_DODEFAULT;
				}
			}
		}
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmListViewSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
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
			RECT rc, display;
			HDC hdc = BeginPaint (hwnd, &ps);
			HFONT font = (HFONT) SendMessageW (hwnd, WM_GETFONT, 0, 0);
			HGDIOBJ oldFont = SelectObject (hdc, font ? (HGDIOBJ) font : GetStockObject (DEFAULT_GUI_FONT));
			int count = TabCtrl_GetItemCount (hwnd);
			int sel = TabCtrl_GetCurSel (hwnd);
			int i;

			GetClientRect (hwnd, &rc);
			FillRect (hdc, &rc, DmBrushBg);

			display = rc;
			TabCtrl_AdjustRect (hwnd, FALSE, &display);
			InflateRect (&display, 2, 2);
			FrameRect (hdc, &display, DmBrushBorder);

			SetBkMode (hdc, TRANSPARENT);
			for (i = 0; i < count; i++)
			{
				RECT item;
				wchar_t text[128];
				TCITEMW ti;

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
					item.top -= 2;
					FillRect (hdc, &item, DmBrushCtrl);
				}
				else
					FillRect (hdc, &item, DmBrushBg);

				FrameRect (hdc, &item, DmBrushBorder);
				SetTextColor (hdc, IsWindowEnabled (hwnd) ? DM_COLOR_TEXT : DM_COLOR_TEXT_DIS);
				DrawTextW (hdc, text, -1, &item, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}

			SelectObject (hdc, oldFont);
			EndPaint (hwnd, &ps);
			return 0;
		}

	case WM_NCDESTROY:
		RemoveWindowSubclass (hwnd, DmTabSubclassProc, uIdSubclass);
		break;
	}

	return DefSubclassProc (hwnd, msg, wParam, lParam);
}

static void DmApplyRichEditColors (HWND hwnd)
{
	CHARFORMAT2W cf;
	LRESULT modified = SendMessageW (hwnd, EM_GETMODIFY, 0, 0);

	SendMessageW (hwnd, EM_SETBKGNDCOLOR, 0, (LPARAM) DM_COLOR_CTRL_BG);

	memset (&cf, 0, sizeof (cf));
	cf.cbSize = sizeof (cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	cf.crTextColor = DM_COLOR_TEXT;
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
/* Anwenden auf Steuerelemente und Dialoge                             */
/* ------------------------------------------------------------------ */

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
		LONG_PTR type = style & BS_TYPEMASK;
		if (!(style & BS_PUSHLIKE) && (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE
			|| type == BS_AUTO3STATE || type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON || type == BS_GROUPBOX))
		{
			/* Klassische Darstellung, damit die helle Textfarbe aus WM_CTLCOLORSTATIC uebernommen wird */
			DmSetTheme (hwnd, L"", L"");
		}
		else
		{
			DmAllowDark (hwnd);
			DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		}
	}
	else if (_wcsicmp (cls, L"Edit") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, (style & ES_MULTILINE) ? L"DarkMode_Explorer" : L"DarkMode_CFD", NULL);
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
				DmAllowDark (cbi.hwndItem);
				DmSetTheme (cbi.hwndItem, L"DarkMode_CFD", NULL);
			}
		}
	}
	else if (_wcsicmp (cls, L"ListBox") == 0 || _wcsicmp (cls, L"ScrollBar") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
	}
	else if (_wcsicmp (cls, WC_LISTVIEWW) == 0)
	{
		HWND header = ListView_GetHeader (hwnd);

		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		ListView_SetBkColor (hwnd, DM_COLOR_CTRL_BG);
		ListView_SetTextBkColor (hwnd, DM_COLOR_CTRL_BG);
		ListView_SetTextColor (hwnd, DM_COLOR_TEXT);

		if (header)
		{
			DmAllowDark (header);
			DmSetTheme (header, L"DarkMode_ItemsView", NULL);
		}

		SetWindowSubclass (hwnd, DmListViewSubclassProc, DM_SUBCLASS_LISTVIEW, 0);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, WC_TREEVIEWW) == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		TreeView_SetBkColor (hwnd, DM_COLOR_CTRL_BG);
		TreeView_SetTextColor (hwnd, DM_COLOR_TEXT);
	}
	else if (_wcsicmp (cls, WC_TABCONTROLW) == 0)
	{
		SetWindowSubclass (hwnd, DmTabSubclassProc, DM_SUBCLASS_TAB, 0);
		InvalidateRect (hwnd, NULL, TRUE);
	}
	else if (_wcsicmp (cls, L"RichEdit20W") == 0 || _wcsicmp (cls, L"RichEdit20A") == 0 || _wcsicmp (cls, L"RICHEDIT50W") == 0)
	{
		DmAllowDark (hwnd);
		DmSetTheme (hwnd, L"DarkMode_Explorer", NULL);
		DmApplyRichEditColors (hwnd);
		SetWindowSubclass (hwnd, DmRichEditSubclassProc, DM_SUBCLASS_RICHEDIT, 0);
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

	DmBrushBg = CreateSolidBrush (DM_COLOR_BG);
	DmBrushCtrl = CreateSolidBrush (DM_COLOR_CTRL_BG);
	DmBrushHot = CreateSolidBrush (DM_COLOR_HOT);
	DmBrushBorder = CreateSolidBrush (DM_COLOR_BORDER);

	if (!DmBrushBg || !DmBrushCtrl || !DmBrushHot || !DmBrushBorder)
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
