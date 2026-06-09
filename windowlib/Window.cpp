#include "pch.h"
#include "Window.h"

Window* g_window = nullptr;

namespace
{
	HCURSOR CreatePixelGameCursor()
	{
		constexpr int SourceSize = 16;
		constexpr int CursorScale = 2;
		constexpr int CursorSize = SourceSize * CursorScale;
		const char* shape[SourceSize] =
		{
			"X...............",
			"XX..............",
			"XWX.............",
			"XWWX............",
			"XWCWX...........",
			"XWWCWX..........",
			"XWWWCWX.........",
			"XWWWWCWX........",
			"XWWWWWCWX.......",
			"XWWWWXXXX.......",
			"XWWXWXX.........",
			"XWX.XWX.........",
			"XX..XWX.........",
			"X....XWX........",
			".....XWX........",
			"......X........."
		};

		BITMAPINFO bitmapInfo = {};
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapInfo.bmiHeader.biWidth = CursorSize;
		bitmapInfo.bmiHeader.biHeight = -CursorSize;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;

		void* bits = nullptr;
		HDC screenDc = GetDC(nullptr);
		HBITMAP colorBitmap = CreateDIBSection(screenDc, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0);
		ReleaseDC(nullptr, screenDc);
		if (colorBitmap == nullptr || bits == nullptr)
			return nullptr;

		DWORD* pixels = static_cast<DWORD*>(bits);
		for (int sourceY = 0; sourceY < SourceSize; ++sourceY)
		{
			for (int sourceX = 0; sourceX < SourceSize; ++sourceX)
			{
				DWORD color = 0x00000000;
				switch (shape[sourceY][sourceX])
				{
				case 'X':
					color = 0xFF111111;
					break;
				case 'W':
					color = 0xFFFFFFFF;
					break;
				case 'C':
					color = 0xFF49D8FF;
					break;
				default:
					break;
				}

				for (int y = 0; y < CursorScale; ++y)
				{
					for (int x = 0; x < CursorScale; ++x)
					{
						const int pixelX = sourceX * CursorScale + x;
						const int pixelY = sourceY * CursorScale + y;
						pixels[pixelY * CursorSize + pixelX] = color;
					}
				}
			}
		}

		BYTE maskBits[((CursorSize + 15) / 16) * 2 * CursorSize] = {};
		HBITMAP maskBitmap = CreateBitmap(CursorSize, CursorSize, 1, 1, maskBits);
		if (maskBitmap == nullptr)
		{
			DeleteObject(colorBitmap);
			return nullptr;
		}

		ICONINFO iconInfo = {};
		iconInfo.fIcon = FALSE;
		iconInfo.xHotspot = 1;
		iconInfo.yHotspot = 1;
		iconInfo.hbmMask = maskBitmap;
		iconInfo.hbmColor = colorBitmap;

		HCURSOR cursor = CreateIconIndirect(&iconInfo);
		DeleteObject(maskBitmap);
		DeleteObject(colorBitmap);

		return cursor;
	}
}

Window::Window()
{
	g_window = this;
}

Window::~Window()
{
	if (m_hCursor)
		DestroyCursor(m_hCursor);
}

void Window::Initialize(const char* szTitle, float posX, float posY, float width, float height, const HINSTANCE hInstance, IGameLoop* gameLoop)
{
	m_szTitle = szTitle;
	m_posX = posX;
	m_posY = posY;
	m_width = width;
	m_height = height;
	m_hInstance = hInstance;
	m_gameLoop = gameLoop;
	m_hCursor = CreatePixelGameCursor();

	WNDCLASSEXA wcex = { 0 };

	wcex.cbSize = sizeof(WNDCLASSEXA);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = m_hCursor ? m_hCursor : LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = "MainWindow";
	wcex.hIconSm = nullptr;

	ATOM A = RegisterClassExA(&wcex);

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	m_hWnd = CreateWindowA("MainWindow", szTitle, WS_OVERLAPPEDWINDOW,
		(int)m_posX, (int)m_posY, (int)m_width, (int)m_height, nullptr, nullptr, m_hInstance, nullptr);

	if (!m_hWnd)
		__debugbreak();

	if (m_hCursor)
		SetClassLongPtrA(m_hWnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(m_hCursor));

	m_dpi = GetDpiForWindow(m_hWnd);
	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SETCURSOR:
	{
		if (g_window && g_window->m_hCursor && LOWORD(lParam) == HTCLIENT)
		{
			SetCursor(g_window->m_hCursor);
			return TRUE;
		}
		break;
	}
	case WM_DPICHANGED:
	{
		g_window->m_dpi = GetDpiForWindow(g_window->m_hWnd);
		break;
	}
	case WM_SIZE:
	{
		g_window->m_width = LOWORD(lParam);
		g_window->m_height = HIWORD(lParam);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void Window::RunMessageLoop()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			m_gameLoop->Update();
		}
	}

	m_gameLoop->Release();
}
