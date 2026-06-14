#pragma once
#include "Interface.h"

struct KeyState
{
	bool isDown = false;
	bool isPressed = false;
	bool isReleased = false;
	bool isFree = false;
};

class Input : public IInput
{
public:
	Input()
	{
		m_userList.reserve(100);
	}
	~Input();

	void Initialize() override;

	void Update() override;

	bool IsDown(int _keyCode, void* _userPtr) override;
	bool IsPressed(int _keyCode, void* _userPtr) override;
	bool IsReleased(int _keyCode, void* _userPtr) override;
	bool IsFree(int _keyCode, void* _userPtr) override;
	void AddUser(void* _UserPtr) override;

	void RegisterKey(int _keyCode);

private:
	bool IsRegisteredUser(void* _userPtr) const;
	KeyState* FindKeyState(int _keyCode);

	std::unordered_map<int, KeyState> m_keyStateMap;
	std::vector<void*> m_userList;
};
