#pragma once
#include "Interface.h"

struct KeyState
{
	bool isDown = false;
	bool isPress = false;
	bool isUp = false;
	bool isFree = false;
};

class CInputObject : public IInputObject
{
public:
	CInputObject()
	{
		m_userList.reserve(100);
	}
	~CInputObject();

	void Initailize() override;

	void UpdateKeyStates() override;

	bool IsDown(int _keyCode, void* _userPtr) override;
	bool IsPress(int _keyCode, void* _userPtr) override;
	bool IsUp(int _keyCode, void* _userPtr) override;
	bool IsFree(int _keyCode, void* _userPtr) override;
	void AddUser(void* _UserPtr) override;

	void CreateKey(int _keyCode);

private:
	std::unordered_map<int, KeyState*> m_keyStateMap;
	std::vector<void*> m_userList;
};

