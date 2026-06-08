#pragma once
#include "interface.h"

class CWindowObject : public IWindowObject
{
public:
	CWindowObject();
	~CWindowObject();

	void Initialize(const char* szTitle, float posX, float posY, float width, float height, const HINSTANCE hInstance, IEngine* pEngine) override;

	void MessageLoop() override;

	const char* GetTitle() override { return m_szTitle; }

	UINT GetDpi() override { return m_dpi; }

	float GetWidth() override { return m_width; }

	float GetHeight() override { return m_height; }

	float GetPosX() override { return m_posX; }

	float GetPosY() override { return m_posY; }
	
	HINSTANCE GetHINSTANCE() override { return m_hInstance; }

	HWND* GetHWND() override { return &m_hWnd; }

	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

private:
	const char* m_szTitle;
	UINT m_dpi;
	float m_posX;
	float m_posY;
	float m_width;
	float m_height;
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	HCURSOR m_hCursor = nullptr;
	IEngine* m_pEngine;
};
