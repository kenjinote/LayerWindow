#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"gdiplus.lib")

#include <windows.h>
#include <gdiplus.h>
#include "resource.h"

using namespace Gdiplus;

TCHAR szClassName[] = TEXT("Window");

Gdiplus::Bitmap* LoadPngFromResource(int nID)
{
	Gdiplus::Bitmap* pImage = 0;
	const HINSTANCE hInst = GetModuleHandle(0);
	const HRSRC hResource = FindResource(hInst, MAKEINTRESOURCE(nID), TEXT("PNG"));
	if (!hResource)
		return 0;
	const DWORD dwImageSize = SizeofResource(hInst, hResource);
	if (!dwImageSize)
		return 0;
	const void* pResourceData = LockResource(LoadResource(hInst, hResource));
	if (!pResourceData)
		return 0;
	HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, dwImageSize);
	if (hBuffer)
	{
		void* pBuffer = GlobalLock(hBuffer);
		if (pBuffer)
		{
			CopyMemory(pBuffer, pResourceData, dwImageSize);
			IStream* pStream = NULL;
			if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) == S_OK)
			{
				pImage = new Gdiplus::Bitmap(pStream);
				pStream->Release();
				if (pImage)
				{
					if (pImage->GetLastStatus() != Gdiplus::Ok)
					{
						delete pImage;
						pImage = NULL;
					}
				}
			}
			::GlobalUnlock(hBuffer);
		}
	}
	return pImage;
}

BOOL UpdateLayeredWindow(HWND hWnd, Bitmap *pBitmap)
{
	if (pBitmap == 0 || pBitmap->GetPixelFormat() != PixelFormat32bppARGB)
		return FALSE;
	HDC hdc = GetDC(0);
	HDC hMemDC = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = 0;
	pBitmap->GetHBITMAP(0, &hBitmap);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	BLENDFUNCTION bf = { 0 };
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 32; // 透明度
	POINT point = { 0 }, surfacePos = { 0 };
	SIZE  surfaceSize = { (LONG)pBitmap->GetWidth() ,(LONG)pBitmap->GetHeight() };
	BOOL bReturn = UpdateLayeredWindow(
		hWnd, hdc,
		&point, &surfaceSize,
		hMemDC, &surfacePos,
		0, &bf, ULW_ALPHA);
	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	DeleteObject(hBitmap);
	ReleaseDC(0, hdc);
	return bReturn;
}

VOID CenterWindow(HWND hWnd)
{
	HMONITOR hPrimaryMonitor = MonitorFromPoint(POINT{ 0,0 }, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFOEX MonitorInfoEx;
	MonitorInfoEx.cbSize = sizeof(MonitorInfoEx);
	if (GetMonitorInfo(hPrimaryMonitor, &MonitorInfoEx) != 0)
	{
		DEVMODE dm = { 0 };
		dm.dmSize = sizeof(DEVMODE);
		if (EnumDisplaySettings(MonitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &dm) != 0)
		{
			int nMonitorWidth = dm.dmPelsWidth;
			int nMonitorHeight = dm.dmPelsHeight;
			RECT rect;
			GetWindowRect(hWnd, &rect);
			SetWindowPos(hWnd, 0,
				(nMonitorWidth - (rect.right - rect.left)) / 2,
				(nMonitorHeight - (rect.bottom - rect.top)) / 2,
				0,
				0,
				SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER | SWP_NOSENDCHANGING | SWP_NOCOPYBITS | SWP_NOOWNERZORDER);
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Bitmap *pBitmap;
	switch (msg)
	{
	case WM_CREATE:
		pBitmap = LoadPngFromResource(IDB_PNG1);
		UpdateLayeredWindow(hWnd, pBitmap);
		CenterWindow(hWnd);
		break;
	case WM_DESTROY:
		delete pBitmap;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	GdiplusStartupInput gdiSI;
	GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		0,
		hInstance,
		0,
		0,
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		szClassName,
		0,
		WS_POPUP,
		0,
		0,
		0,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}