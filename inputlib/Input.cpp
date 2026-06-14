#include "pch.h"
#include "Input.h"

Input::~Input()
{
}

void Input::Initialize()
{
	RegisterKey(VK_LBUTTON);
	RegisterKey(VK_RBUTTON);
	RegisterKey(VK_CANCEL);
	RegisterKey(VK_MBUTTON);
	RegisterKey(VK_XBUTTON1);
	RegisterKey(VK_XBUTTON2);

	RegisterKey(VK_BACK);
	RegisterKey(VK_TAB);

	RegisterKey(VK_RETURN);
	RegisterKey(VK_SHIFT);
	RegisterKey(VK_CONTROL);
	RegisterKey(VK_MENU);
	RegisterKey(VK_PAUSE);
	RegisterKey(VK_CAPITAL);
	RegisterKey(VK_HANGEUL);

	RegisterKey(VK_ESCAPE);
	RegisterKey(VK_SPACE);
	RegisterKey(VK_PRIOR);
	RegisterKey(VK_NEXT);
	RegisterKey(VK_END);
	RegisterKey(VK_HOME);
	RegisterKey(VK_LEFT);
	RegisterKey(VK_UP);
	RegisterKey(VK_RIGHT);
	RegisterKey(VK_DOWN);
	RegisterKey(VK_SNAPSHOT);
	RegisterKey(VK_INSERT);
	RegisterKey(VK_DELETE);

	RegisterKey(0x30); // 0 
	RegisterKey(0x31); // 1
	RegisterKey(0x32); // 2
	RegisterKey(0x33); // 3 
	RegisterKey(0x34); // 4
	RegisterKey(0x35); // 5
	RegisterKey(0x36); // 6
	RegisterKey(0x37); // 7
	RegisterKey(0x38); // 8
	RegisterKey(0x39); // 9

	RegisterKey(0x41); // A
	RegisterKey(0x42); // B
	RegisterKey(0x43); // C
	RegisterKey(0x44); // D
	RegisterKey(0x45); // E 
	RegisterKey(0x46); // F
	RegisterKey(0x47); // G
	RegisterKey(0x48); // H
	RegisterKey(0x49); // I
	RegisterKey(0x4A); // J
	RegisterKey(0x4B); // K
	RegisterKey(0x4C); // L
	RegisterKey(0x4D); // M
	RegisterKey(0x4E); // N
	RegisterKey(0x4F); // O
	RegisterKey(0x50); // P
	RegisterKey(0x51); // Q
	RegisterKey(0x52); // R 
	RegisterKey(0x53); // S
	RegisterKey(0x54); // T
	RegisterKey(0x55); // U
	RegisterKey(0x56); // V
	RegisterKey(0x57); // W
	RegisterKey(0x58); // X
	RegisterKey(0x59); // Y
	RegisterKey(0x5A); // Z

	RegisterKey(VK_LWIN);
	RegisterKey(VK_RWIN);
	RegisterKey(VK_NUMPAD0);
	RegisterKey(VK_NUMPAD1);
	RegisterKey(VK_NUMPAD2);
	RegisterKey(VK_NUMPAD3);
	RegisterKey(VK_NUMPAD4);
	RegisterKey(VK_NUMPAD5);
	RegisterKey(VK_NUMPAD6);
	RegisterKey(VK_NUMPAD7);
	RegisterKey(VK_NUMPAD8);
	RegisterKey(VK_NUMPAD9);

	RegisterKey(VK_MULTIPLY);
	RegisterKey(VK_ADD);
	RegisterKey(VK_SEPARATOR);
	RegisterKey(VK_SUBTRACT);
	RegisterKey(VK_DECIMAL);
	RegisterKey(VK_DIVIDE);

	RegisterKey(VK_F1);
	RegisterKey(VK_F2);
	RegisterKey(VK_F3);
	RegisterKey(VK_F4);
	RegisterKey(VK_F5);
	RegisterKey(VK_F6);
	RegisterKey(VK_F7);
	RegisterKey(VK_F8);
	RegisterKey(VK_F9);
	RegisterKey(VK_F10);
	RegisterKey(VK_F11);
	RegisterKey(VK_F12);

	RegisterKey(VK_NUMLOCK);
	RegisterKey(VK_SCROLL);
	RegisterKey(VK_LSHIFT);
	RegisterKey(VK_RSHIFT);
	RegisterKey(VK_LCONTROL);
	RegisterKey(VK_RCONTROL);
	RegisterKey(VK_LMENU);
	RegisterKey(VK_RMENU);
	RegisterKey(VK_VOLUME_MUTE);
	RegisterKey(VK_VOLUME_DOWN);
	RegisterKey(VK_VOLUME_UP);

	RegisterKey(VK_OEM_1);
	RegisterKey(VK_OEM_PLUS);
	RegisterKey(VK_OEM_COMMA);
	RegisterKey(VK_OEM_MINUS);
	RegisterKey(VK_OEM_PERIOD);
	RegisterKey(VK_OEM_2);
	RegisterKey(VK_OEM_3);
	RegisterKey(VK_OEM_4);
	RegisterKey(VK_OEM_5);
	RegisterKey(VK_OEM_6);
}

void Input::RegisterKey(int _keyCode)
{
	m_keyStateMap.emplace(_keyCode, KeyState());
}

void Input::Update()
{
	for (auto& pair : m_keyStateMap)
	{
		int keyCode = pair.first;
		KeyState* Key = &pair.second;
		const bool isHeld = (GetAsyncKeyState(keyCode) & 0x8000) != 0;
		if (!isHeld) // �ȴ�������
		{
			if (Key->isDown || Key->isPressed) // ������ �������־��ٸ� Up
			{
				Key->isDown = false;
				Key->isPressed = false;
				Key->isReleased = true;
				Key->isFree = false;
				continue;
			}

			// �ƴ϶�� Free
			Key->isDown = false;
			Key->isPressed = false;
			Key->isReleased = false;
			Key->isFree = true;
			continue;
		}
		else // ��������
		{
			if (Key->isDown || Key->isPressed) // ������ �������ִٸ� Press
			{
				Key->isDown = false;
				Key->isPressed = true;
				Key->isReleased = false;
				Key->isFree = false;
				continue;
			}

			// �ƴ϶�� Down
			Key->isDown = true;
			Key->isPressed = false;
			Key->isReleased = false;
			Key->isFree = false;
			continue;
		}
	}
}

bool Input::IsRegisteredUser(void* _userPtr) const
{
	for (void* UserPtr : m_userList)
	{
		if (UserPtr == _userPtr)
			return true;
	}

	return false;
}

KeyState* Input::FindKeyState(int _keyCode)
{
	auto iter = m_keyStateMap.find(_keyCode);
	if (iter == m_keyStateMap.end())
		return nullptr;

	return &iter->second;
}

bool Input::IsDown(int _keyCode, void* _userPtr)
{
	if (!IsRegisteredUser(_userPtr))
		return false;

	const KeyState* key = FindKeyState(_keyCode);
	if (key == nullptr)
		return false;

	return key->isDown;
}

bool Input::IsPressed(int _keyCode, void* _userPtr)
{
	if (!IsRegisteredUser(_userPtr))
		return false;

	const KeyState* key = FindKeyState(_keyCode);
	if (key == nullptr)
		return false;

	return key->isPressed;
}

bool Input::IsReleased(int _keyCode, void* _userPtr)
{
	if (!IsRegisteredUser(_userPtr))
		return false;

	const KeyState* key = FindKeyState(_keyCode);
	if (key == nullptr)
		return false;

	return key->isReleased;
}

bool Input::IsFree(int _keyCode, void* _userPtr)
{
	if (!IsRegisteredUser(_userPtr))
		return false;

	const KeyState* key = FindKeyState(_keyCode);
	if (key == nullptr)
		return false;

	return key->isFree;
}

void Input::AddUser(void* _userPtr)
{
	if (IsRegisteredUser(_userPtr))
		return;

	m_userList.push_back(_userPtr);
}
