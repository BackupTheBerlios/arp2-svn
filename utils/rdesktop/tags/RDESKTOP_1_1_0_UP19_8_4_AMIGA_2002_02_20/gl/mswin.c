/*
   rdesktop: A Remote Desktop Protocol client.
   User interface services - MS-Windows
   Copyright (C) Matthew Chapman 1999-2001

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "rdesktop.h"

extern int width;
extern int height;
extern BOOL sendmotion;
extern BOOL fullscreen;

static HINSTANCE instance;
static HWND wnd;
static HCURSOR cursor;
static HPALETTE pal;
static RECT clip_rect;

/* colour maps */
static BOOL owncolmap;
static RGBQUAD pal_entries[256];

/* software backing store */
static HBITMAP backstore;
static HDC backstore_dc;
static uint8 *backstore_bits;

static void
mswin_invalidate(int x, int y, int cx, int cy)
{
 RECT rc = {x, y, x + cx, y + cy};
 InvalidateRect(wnd, &rc, False);
}

static void
mswin_bgr2rgb(RGBQUAD *colours)
{
 int i;
 for(i = 0; i < 256; i++)
 {
  uint8 blue = colours[i].rgbRed;
  colours[i].rgbRed = colours[i].rgbBlue;
  colours[i].rgbBlue = blue;
 }
}

static COLORREF
mswin_pal_colour(int pal_colour)
{
 RGBQUAD rc;
 if (pal_colour > 256 || pal_colour < 0)
  return 0;

 rc = pal_entries[pal_colour];
 return RGB(rc.rgbRed, rc.rgbGreen, rc.rgbBlue);
}

static HGLYPH
mswin_create_bicolour_image(int width, int height, uint8 *data, int
bgcolour, int fgcolour)
{
 PBITMAPINFO pbmi;
 int width_bytes;

 pbmi = (PBITMAPINFO)_alloca(sizeof(BITMAPINFOHEADER) + 2 *
sizeof(RGBQUAD));
 ZeroMemory(pbmi, sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD));

 pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 pbmi->bmiHeader.biWidth = width;
 pbmi->bmiHeader.biHeight = -height;
 pbmi->bmiHeader.biPlanes = 1;
 pbmi->bmiHeader.biBitCount = 1;
 pbmi->bmiHeader.biCompression = BI_RGB;
 pbmi->bmiHeader.biClrUsed = 2;

 pbmi->bmiColors[0] = pal_entries[fgcolour];
 pbmi->bmiColors[1] = pal_entries[bgcolour];

 width_bytes = (width + 7) / 8;

 if (width_bytes & 3)
 {
  /* must be dword-aligned */
  int aligned_width = (width_bytes + 3) & ~3;
  uint8 *tmp = (uint8 *)_alloca(height * aligned_width);

  while(--height >= 0)
   CopyMemory(tmp + height * aligned_width, data + height * width_bytes,
width_bytes);

  data = tmp;
 }

 return CreateDIBitmap(backstore_dc, &pbmi->bmiHeader, CBM_INIT, data, pbmi,
DIB_RGB_COLORS);
}

static HPEN
mswin_create_pen(PEN *pen)
{
 LOGPEN lp;

 lp.lopnStyle = pen->style;
 lp.lopnColor = mswin_pal_colour(pen->colour);
 lp.lopnWidth.x = pen->width;
 /* lp.lopnWidth.y will never be used (MS suxx) */

 return CreatePenIndirect(&lp);
}

static HBRUSH
mswin_create_brush(BRUSH *brush, int bgcolour, int fgcolour)
{
 HBRUSH hBrush;
 LOGBRUSH lb;
 lb.lbStyle = brush->style;

 switch (brush->style)
 {
  case BS_HATCHED:
   lb.lbHatch = brush->pattern[0];
   /* fallsthrough */

  case BS_SOLID:
   lb.lbColor = mswin_pal_colour(fgcolour);
   break;

  case BS_PATTERN:
   lb.lbHatch = (ULONG)mswin_create_bicolour_image(8, 8, brush->pattern,
bgcolour, fgcolour);
   break;

  default:
   unimpl("brush %d\n", brush->style);
   return NULL;
 }

 hBrush = CreateBrushIndirect(&lb);

 if (BS_PATTERN == brush->style)
  DeleteObject((HBITMAP)lb.lbHatch);

 return hBrush;
}

static UINT
mswin_realize_palette(BOOL bBackground)
{
 UINT nColorsChanged;
 HPALETTE hOldPal;
 HDC hDC;
 if (!pal)
  return 0;

 /* Selet palette into client DC and realize it */
 hDC = GetDC(wnd);
 if (!hDC)
  return 0;

 hOldPal = SelectPalette(hDC, pal, bBackground);
 nColorsChanged = RealizePalette(hDC);

#if 0
 if (bBackground)
  UpdateColors(hDC);
 else
#endif

 if (nColorsChanged > 0)
  InvalidateRgn(wnd, NULL, FALSE); /* tell Windows to repaint */

 if (hOldPal)
  SelectPalette(hDC, hOldPal, TRUE);
 return nColorsChanged;
}

/* convert a win32 mouse message into DRP one */
static uint16
mswin_translate_mouse(UINT uMsg)
{
 switch(uMsg)
 {
 case WM_MOUSEMOVE:
  return MOUSE_FLAG_MOVE;
 case WM_LBUTTONDOWN:
  return MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN;
 case WM_LBUTTONUP:
  return MOUSE_FLAG_BUTTON1;
 case WM_RBUTTONDOWN:
  return MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN;
 case WM_RBUTTONUP:
  return MOUSE_FLAG_BUTTON2;
 case WM_MBUTTONDOWN:
  return MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN;
 case WM_MBUTTONUP:
  return MOUSE_FLAG_BUTTON3;
 }
 return 0;
}

static LRESULT CALLBACK
mswin_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
 {
 case WM_PALETTECHANGED:
  if (wParam != (WPARAM)wnd)
   mswin_realize_palette(True);
  break;

 case WM_QUERYNEWPALETTE:
  return mswin_realize_palette(False);

 case WM_PAINT:
  {
   HPALETTE hOldPal;
   PAINTSTRUCT ps;
   BeginPaint(hWnd, &ps);
   if (pal)
   {
    hOldPal = SelectPalette(ps.hdc, pal, True);
    RealizePalette(ps.hdc);
   }
   BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top,
    ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
    backstore_dc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
   if (pal)
    SelectPalette(ps.hdc, hOldPal, True);
   EndPaint(hWnd, &ps);
  }
  break;
 case WM_SETCURSOR:
  SetCursor(cursor);
  break;
 case WM_MOUSEMOVE:
 case WM_LBUTTONDOWN:
 case WM_LBUTTONUP:
 case WM_RBUTTONDOWN:
 case WM_RBUTTONUP:
 case WM_MBUTTONDOWN:
 case WM_MBUTTONUP:
  {
   DWORD nTime = GetTickCount();
   rdp_send_input(nTime, RDP_INPUT_MOUSE,
        mswin_translate_mouse(uMsg),
        GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam));
  }
  return 0;
 case WM_MOUSEWHEEL:
  {
   int delta = GET_WHEEL_DELTA_WPARAM(wParam);
   DWORD nTime = GetTickCount();

   if (delta > 0)
    delta |= 0x0200;
   else
    delta &= 0x03FF;

   rdp_send_input(nTime, RDP_INPUT_MOUSE,
        delta, 0, 0);
  }
  return 0;
 case WM_KEYDOWN:
 case WM_KEYUP:
 case WM_SYSKEYDOWN:
 case WM_SYSKEYUP:
  {
   uint32 nTime = GetTickCount();
   uint16 nScanCode, nFlags;

    nScanCode = nFlags = HIWORD(lParam);
    nScanCode &= 0xFF;
    nFlags &= 0xC100;

    /* PB 12/18/2001 No ext flag for numlock - why? */
    if (0x0045 == nScanCode)
     nFlags &= ~KBD_FLAG_EXT;

   rdp_send_input(nTime, RDP_INPUT_SCANCODE,
    nFlags, nScanCode, 0);
  }
   /* fallsthrough */
  case WM_CHAR:
  case WM_DEADCHAR:
  case WM_SYSCHAR:
  case WM_SYSDEADCHAR:
  /* eat WM_CHAR message */
  return 0;
 }

 return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void
mswin_draw_glyph(int x, int y, int cx, int cy, HGLYPH glyph, uint8 nColour)
{
 uint8 *pSrc = (uint8 *)glyph;
 uint8 *dest = backstore_bits + y * width + x;

 while(cy--)
 {
  if (y >= clip_rect.top && y < clip_rect.bottom)
  {
   int nBits = 0;
   uint8 *pTmpDest = dest;
   register uint8 nSrc;
   register uint8 nBit = 0;

   while(nBits++ < cx)
   {
    if (0 == nBit)
    {
     nBit = 0x80;
     nSrc = *pSrc++;
    }

    if ((nSrc & nBit) && x + nBits >= clip_rect.left && x + nBits <
clip_rect.right)
     *pTmpDest = nColour;

    pTmpDest++;
    nBit >>= 1;
   }
  }
  else
   pSrc += (cx + 7) / 8;

  dest += width;
  y++;
 }
}

BOOL
ui_create_window(char *title)
{
 RECT rc;
    WNDCLASSEX wcx;
 HBITMAP hOldBitmap;
 HDC hDC;
 PBITMAPINFO pbmi;

 if (width == -1)
   width = 800;

 if (height == -1)
   height = 600;
 
 instance = GetModuleHandle(NULL);

    /* Fill in the window class structure with parameters
    that describe the main window. */

 ZeroMemory(&wcx, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = mswin_wndproc;     /* points to window procedure */
    wcx.lpszClassName = _T("rdesktop");  /* name of window class */

    /* Register the window class. */

 if (!RegisterClassEx(&wcx))
  return False; /* Failed to register window class */

 rc.left = 0;
 rc.right = width;
 rc.top = 0;
 rc.bottom = height;

 AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION, False);

    wnd =
ateWindow(
        wcx.lpszClassName,   /* name of window class */
        title,               /* title-bar string */
        WS_OVERLAPPED | WS_CAPTION, /* window style */
        CW_USEDEFAULT,       /* default horizontal position */
        CW_USEDEFAULT,       /* default vertical position */
        rc.right - rc.left,  /* default width */
        rc.bottom - rc.top,  /* default height */
        (HWND) NULL,         /* no owner window */
        (HMENU) NULL,        /* use class menu */
        instance,            /* handle to application instance */
        (LPVOID) NULL);      /* no window-creation data */

    if (!wnd)
        return False; /* Failed to create ui window */

    /* Show the window and send a WM_PAINT message to the window
    procedure. */

    ShowWindow(wnd, SW_SHOWDEFAULT);
    UpdateWindow(wnd);

 hDC = GetWindowDC(wnd);
 backstore_dc = CreateCompatibleDC(hDC);

 pbmi = (PBITMAPINFO)_alloca(sizeof(BITMAPINFOHEADER));
 pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 pbmi->bmiHeader.biWidth = width;
 pbmi->bmiHeader.biHeight = -height;
 pbmi->bmiHeader.biPlanes = 1;
 pbmi->bmiHeader.biBitCount = 8;
 pbmi->bmiHeader.biCompression = BI_RGB;
 pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

 backstore = CreateDIBSection(backstore_dc, pbmi,
  DIB_RGB_COLORS, &backstore_bits, NULL, 0);

 hOldBitmap = SelectObject(backstore_dc, backstore);

 if (hOldBitmap)
  DeleteObject(hOldBitmap);

 ReleaseDC(wnd, hDC);

 GetSystemPaletteEntries(backstore_dc, 0, 256, (PPALETTEENTRY)pal_entries)
;
 mswin_bgr2rgb(pal_entries);
 SetDIBColorTable(backstore_dc, 0, 256, pal_entries);

 ui_reset_clip();
 return True;
}

void
ui_destroy_window(void)
{
 DeleteObject(backstore);
 DeleteDC(backstore_dc);
 PostQuitMessage(0);
}

void
ui_select(int rdp_socket)
{
 static BOOL bFirstTime = True;
 MSG msg;

 if (bFirstTime)
 {
  bFirstTime = False;
  WSAAsyncSelect(rdp_socket, wnd, WM_APP, FD_READ);
 }

 while(GetMessage(&msg, wnd, 0, 0))
 {
  if (WM_APP == msg.message && msg.wParam == (WPARAM)rdp_socket)
  {
   /* eat all dups */
   while(PeekMessage(&msg, wnd, WM_APP, WM_APP, PM_REMOVE))
     continue;
   return;
  }

  TranslateMessage(&msg);
  DispatchMessage(&msg);
 }
}

void
ui_move_pointer(int x, int y)
{
 if (GetFocus() == wnd)
 {
  POINT pt = {x, y};
  ClientToScreen(wnd, &pt);
  SetCursorPos(pt.x, pt.y);
 }
}

HBITMAP
ui_create_bitmap(int width, int height, uint8 *data)
{
 HBITMAP hBmp;
 PBITMAPINFO pbmi;
 LPVOID pBits;

 pbmi = (PBITMAPINFO)_alloca(sizeof(BITMAPINFOHEADER) + 256 *
sizeof(RGBQUAD));
 pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 pbmi->bmiHeader.biWidth = width;
 pbmi->bmiHeader.biHeight = -height;
 pbmi->bmiHeader.biPlanes = 1;
 pbmi->bmiHeader.biBitCount = 8;
 pbmi->bmiHeader.biCompression = BI_RGB;
 pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 256;
    pbmi->bmiHeader.biClrImportant = 0;

 CopyMemory(pbmi->bmiColors, pal_entries, 256 * sizeof(RGBQUAD));
 hBmp = CreateDIBSection(backstore_dc, pbmi, DIB_RGB_COLORS, &pBits, NULL,
0);

 if (hBmp)
  CopyMemory(pBits, data, width * height);
 else
  error("ui_create bitmap\n");

 return hBmp;
}

void
ui_paint_bitmap(int x, int y, int cx, int cy, int bitmap_width, int
bitmap_height, uint8 *data)
{
 uint8 *bits = backstore_bits + y * width + x;

 /* no clip_rect checking here */
 while (bitmap_height-- > 0)
 {
  CopyMemory(bits, data, cx);
  bits += width;
  data += bitmap_width;
 }

 /* draw the bitmap as soon as possible */
 mswin_invalidate(x, y, cx, cy);
}

void
ui_destroy_bitmap(HBITMAP hBmp)
{
 DeleteObject(hBmp);
}

HCURSOR
ui_create_mono_cursor(unsigned int x, unsigned int y,
  int width, int height, uint8 *andmask, uint8 *xormask)
{
 HCURSOR hCur = CreateCursor(instance, x, y, width, height, andmask,
xormask);
 return hCur ? hCur : LoadCursor(NULL, IDC_ARROW);
}


/* create super-puper True colour cursor */
HCURSOR
ui_create_cursor(unsigned int x, unsigned int y, unsigned int bpp,
  int width, int height, uint8 *andmask, uint8 *xormask)
{
 ICONINFO ii;
 HCURSOR hCur;
 HWND hWndDesktop;
 HDC hDCDesktop, hDCMono;
 PBITMAPINFO pbmi;

 /* Init icon info */
 ii.fIcon = False;
 ii.xHotspot = x;
 ii.yHotspot = y;

 hWndDesktop = GetDesktopWindow();
 hDCDesktop = GetWindowDC(hWndDesktop);

 if (!hDCDesktop)
  return NULL;

 pbmi = (PBITMAPINFO)_alloca(sizeof(BITMAPINFOHEADER) + 256 *
sizeof(RGBQUAD));
 pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 pbmi->bmiHeader.biWidth = width;
 pbmi->bmiHeader.biHeight = height;
 pbmi->bmiHeader.biPlanes = 1;
 pbmi->bmiHeader.biBitCount = bpp;
 pbmi->bmiHeader.biCompression = BI_RGB;
 pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 256;
    pbmi->bmiHeader.biClrImportant = 0;

 /* Create true color XOR image */
 CopyMemory(pbmi->bmiColors, pal_entries, 256 * sizeof(RGBQUAD));
 ii.hbmColor = CreateDIBitmap(hDCDesktop, &pbmi->bmiHeader, CBM_INIT,
xormask, pbmi, DIB_RGB_COLORS);
 ReleaseDC(hWndDesktop, hDCDesktop);

 pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biClrUsed = 2;
 pbmi->bmiColors[0].rgbRed = pbmi->bmiColors[0].rgbGreen =
pbmi->bmiColors[0].rgbBlue = 0;
 pbmi->bmiColors[1].rgbRed = pbmi->bmiColors[1].rgbGreen =
pbmi->bmiColors[1].rgbBlue = 0xFF;
 pbmi->bmiColors[0].rgbReserved = pbmi->bmiColors[1].rgbReserved = 0;

 hDCMono = CreateCompatibleDC(NULL);
 if (hDCMono)
 {
  /* Create mono AND image */
  ii.hbmMask = CreateDIBitmap(hDCMono, &pbmi->bmiHeader, CBM_INIT, andmask,
pbmi, DIB_RGB_COLORS);
  DeleteDC(hDCMono);
 }
 else
  ii.hbmMask = NULL;

 /* Create cursor from two bitmaps */
 hCur = CreateIconIndirect(&ii);
 DeleteObject(ii.hbmMask);
 DeleteObject(ii.hbmColor);

 return hCur ? hCur : LoadCursor(NULL, IDC_ARROW);
}

void
ui_set_cursor(HCURSOR hCursor)
{
 cursor = hCursor;
 SetCursor(cursor);
}

void
ui_destroy_cursor(HCURSOR hCursor)
{
 DestroyCursor(hCursor);
}

/* Just save the bits */
HGLYPH
ui_create_glyph(int width, int height, uint8 *data)
{
 int nLen = (height * ((width + 7) / 8) + 3) & ~3;
 LPVOID pData = LocalAlloc(LMEM_FIXED, nLen);
 if (pData)
  CopyMemory(pData, data, nLen);
 return pData;
}

void
ui_destroy_glyph(HGLYPH glyph)
{
 LocalFree(glyph);
}

HCOLOURMAP
ui_create_colourmap(COLOURMAP *colours)
{
 uint16 i, ncolours = colours->ncolours;
 LPLOGPALETTE lp = (LPLOGPALETTE)_alloca(sizeof(LOGPALETTE) +
colours->ncolours * sizeof(PALETTEENTRY));

 lp->palVersion = 0x300;
 lp->palNumEntries = colours->ncolours;

 for (i = 0; i < ncolours; i++)
 {
  lp->palPalEntry[i].peRed = colours->colours[i].red;
  lp->palPalEntry[i].peGreen = colours->colours[i].green;
  lp->palPalEntry[i].peBlue = colours->colours[i].blue;
  lp->palPalEntry[i].peFlags = 0;
 }

 return CreatePalette(lp);
}

void
ui_destroy_colourmap(HCOLOURMAP hPal)
{
 DeleteObject(hPal);
}

void
ui_set_colourmap(HCOLOURMAP hPal)
{
 pal = hPal;
 GetPaletteEntries(hPal, 0, 256, (PPALETTEENTRY)pal_entries);
 mswin_bgr2rgb(pal_entries);
 SetDIBColorTable(backstore_dc, 0, 256, pal_entries);
}

void
ui_set_clip(int x, int y, int cx, int cy)
{
 clip_rect.left = x;
 clip_rect.right = x + cx;
 clip_rect.top = y;
 clip_rect.bottom = y + cy;

 IntersectClipRect(backstore_dc, x, y, x + cx, y + cy);
}

void
ui_reset_clip(void)
{
 clip_rect.left = 0;
 clip_rect.right = width;
 clip_rect.top = 0;
 clip_rect.bottom = height;

 SelectClipRgn(backstore_dc, NULL);
}

void
ui_bell(void)
{
 MessageBeep(-1);
}

void
ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
PatBlt(backstore_dc, x, y, cx, cy, WIN32_ROP(opcode));
mswin_invalidate(x, y, cx, cy);
}

void
ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
    BRUSH *brush, int bgcolour, int fgcolour)
{
 HBRUSH hOldBrush, hBrush;
 POINT ptOldOrig;

 hBrush = mswin_create_brush(brush, bgcolour, fgcolour);
 if (!hBrush)
  return;

 hOldBrush = (HBRUSH)SelectObject(backstore_dc, hBrush);

 if (brush->xorigin || brush->yorigin)
  SetBrushOrgEx(backstore_dc, brush->xorigin, brush->yorigin, &ptOldOrig);

 PatBlt(backstore_dc, x, y, cx, cy, WIN32_ROP(opcode));

 SelectObject(backstore_dc, hOldBrush);
 SetBrushOrgEx(backstore_dc, ptOldOrig.x, ptOldOrig.y, NULL);
 DeleteObject(hBrush);

 mswin_invalidate(x, y, cx, cy);
}

void
ui_screenblt(uint8 opcode, int x, int y,
    int cx, int cy, int srcx, int srcy)
{
 GdiFlush();
 BitBlt(backstore_dc, x, y, cx, cy,
  backstore_dc, srcx, srcy, WIN32_ROP(opcode));

 mswin_invalidate(x, y, cx, cy);
}

void
ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
    HBITMAP src, HCOLOURMAP map, int srcx, int srcy)
{
 HBITMAP hOldBmp;
 HDC hMemDC;

 if (!map || !src)
  return;

 hMemDC = CreateCompatibleDC(backstore_dc);
 hOldBmp = SelectObject(hMemDC, src);

 if (map != pal)
 {
  /* switch palette */
  RGBQUAD palette[256];
  GetPaletteEntries(map, 0, 256, (PPALETTEENTRY)palette);
  mswin_bgr2rgb(palette);
  SetDIBColorTable(hMemDC, 0, 256, palette);
 }
 else
  SetDIBColorTable(hMemDC, 0, 256, pal_entries);

 BitBlt(backstore_dc, x, y, cx, cy, hMemDC, srcx, srcy, WIN32_ROP(opcode));

 SelectObject(hMemDC, hOldBmp);
 DeleteDC(hMemDC);

 mswin_invalidate(x, y, cx, cy);
}

void
ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
     HBITMAP src, HCOLOURMAP map, int srcx, int srcy,
     BRUSH *brush, int bgcolour, int fgcolour)
{
 HBRUSH hOldBrush, hBrush;
 HBITMAP hOldBmp;
 HDC hMemDC;
 POINT ptOldOrig;

 if (!map || !src)
  return;

 hMemDC = CreateCompatibleDC(backstore_dc);
 if (!hMemDC)
  return;

 hBrush = mswin_create_brush(brush, bgcolour, fgcolour);
 if (hBrush)
 {
  hOldBmp = SelectObject(hMemDC, src);

  if (map != pal)
  {
   /* switch palette */
   RGBQUAD palette[256];
   GetPaletteEntries(map, 0, 256, (PPALETTEENTRY)palette);
   mswin_bgr2rgb(palette);
   SetDIBColorTable(hMemDC, 0, 256, palette);
  }
  else
   SetDIBColorTable(hMemDC, 0, 256, pal_entries);

  hOldBrush = SelectObject(backstore_dc, hBrush);
  if (brush->xorigin || brush->yorigin)
   SetBrushOrgEx(backstore_dc, brush->xorigin, brush->yorigin, &ptOldOrig);

  BitBlt(backstore_dc, x, y, cx, cy, hMemDC, srcx, srcy, WIN32_ROP(opcode));

  SelectObject(backstore_dc, hOldBrush);
  SetBrushOrgEx(backstore_dc, ptOldOrig.x, ptOldOrig.y, NULL);
  SelectObject(hMemDC, hOldBmp);
  DeleteObject(hBrush);
 }

 DeleteDC(hMemDC);

 mswin_invalidate(x, y, cx, cy);
}

void
ui_line(uint8 opcode, int startx, int starty, int endx, int endy, PEN *pen)
{
 RECT rc = { startx, starty, endx, endy };
 HPEN hOldPen, hPen;
 int nOldRop2;

 hPen = mswin_create_pen(pen);
 if (!hPen)
  return;

 hOldPen = SelectObject(backstore_dc, hPen);
 nOldRop2 = SetROP2(backstore_dc, opcode);

 MoveToEx(backstore_dc, startx, starty, NULL);
 LineTo(backstore_dc, endx, endy);

 SetROP2(backstore_dc, nOldRop2);
 SelectObject(backstore_dc, hOldPen);
 DeleteObject(hPen);

 if (startx < endx)
  rc.left -= pen->width, rc.right += pen->width;
 else
  rc.left += pen->width, rc.right -= pen->width;

 if (starty < endy)
  rc.top -= pen->width, rc.bottom += pen->width;
 else
  rc.top += pen->width, rc.bottom -= pen->width;

 InvalidateRect(wnd, &rc, False);
}

void
ui_rect(int x, int y, int cx, int cy, int colour)
{
 if (cx * cy < 512)
 {
  int i;
  uint8 *data;

  if (x >= clip_rect.right || y >= clip_rect.bottom
   || x + cx < clip_rect.left || y + cy < clip_rect.top)
  {
   /* Should never be happen - nothing to draw*/
   return;
  }

  if (clip_rect.left > x)
   cx -= (clip_rect.left - x), x = clip_rect.left;

  if (clip_rect.top > y)
   cy -= (clip_rect.top - y), y = clip_rect.top;

  if (clip_rect.right < (x + cx))
   cx = clip_rect.right - x;

  if (clip_rect.bottom < (y + cy))
   cy = clip_rect.bottom - y;

  data = backstore_bits + y * width + x;
  for(i = 0; i < cy; i++)
  {
   FillMemory(data, cx, colour);
   data += width;
  }
 }
 else
 {
  HBRUSH hBrush = CreateSolidBrush(mswin_pal_colour(colour));
  RECT rc = {x, y, x + cx, y + cy};
  FillRect(backstore_dc, &rc, hBrush);
  DeleteObject(hBrush);
 }

 mswin_invalidate(x, y, cx, cy);
}

void
ui_draw_text(uint8 font, uint8 flags, int mixmode, int x, int y, int clipx,
int clipy, int clipcx, int clipcy, int boxx, int boxy, int boxcx, int boxcy,
int bgcolour, int fgcolour, uint8 *text, uint8 length)
{
 FONTGLYPH *glyph;
 int i, offset;

 if (boxcx > 0)
  ui_rect(boxx, boxy, boxcx, boxcy, bgcolour);
 else if (MIX_OPAQUE == mixmode)
  ui_rect(clipx, clipy, clipcx, clipcy, bgcolour);

 /* Paint text, character by character */
 for (i = 0; i < length; i++)
 {
  glyph = cache_get_font(font, text[i]);

  if (!(flags & TEXT2_IMPLICIT_X))
  {
   offset = text[++i];
   if (offset & 0x80)
    offset = ((offset & 0x7f) << 8) | text[++i];

   if (flags & TEXT2_VERTICAL)
    y += offset;
   else
    x += offset;
  }

  if (glyph != NULL)
  {
   int startx = x + (short) glyph->offset;
   int starty = y + (short) glyph->baseline;

   mswin_draw_glyph(startx, starty,
     glyph->width, glyph->height,
     glyph->pixmap, fgcolour);

   mswin_invalidate(startx, starty, glyph->width, glyph->height);

   if (flags & TEXT2_IMPLICIT_X)
    x += glyph->width;
  }
 }
}

void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
 cache_put_desktop(offset, cx, cy, width, 1, backstore_bits + y * width +
x);
}

void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
 uint8 *bits = backstore_bits + y * width + x;
 uint8 *data = cache_get_desktop(offset, cx, cy, 1);
 int i;

 if (data == NULL)
  return;

 for (i = 0; i < cy; i++)
 {
  CopyMemory(bits, data, cx);
  bits += width;
  data += cx;
 }
 mswin_invalidate(x, y, cx, cy);
}
