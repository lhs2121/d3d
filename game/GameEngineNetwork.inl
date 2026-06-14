// Included by GameEngine.cpp. Shares its private class declarations and file-local helpers.

void GameEngine::InitializeNetwork(const NetworkConfig& config)
{
	m_networkConfig = config;
	m_networkMode = config.mode;
	m_networkConnected = false;
	m_networkSeedReady = false;
	m_networkTime = 0.0f;
	m_networkSendTimer = 0.0f;
	m_networkHelloTimer = 0.0f;
	m_networkItemSendTimer = 0.0f;
	m_networkStatusTimer = 0.0f;
	m_networkPeers.clear();
	m_remotePlayers.clear();
	m_remoteBlockBreaks.clear();
	m_networkTileHistory.clear();
	m_pendingNetworkTileEdits.clear();
	m_pendingNetworkDroppedItems.clear();
	m_nextNetworkPlayerId = 2;
	m_localPlayerId = 1;
	m_networkPlayerSequence = 0;
	m_networkTileSequence = 0;
	m_networkItemSequence = 0;
	m_networkBreakingBlockIndex = -1;

	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
	{
		m_worldSeed = GetTickCount();
		m_networkSeedReady = true;
		return;
	}

	if (m_networkTransport != nullptr)
	{
		DeleteNetworkTransport(m_networkTransport);
		m_networkTransport = nullptr;
	}
	CreateNetworkTransport(&m_networkTransport);
	if (m_networkTransport == nullptr)
	{
		m_networkMode = NetworkConfig::Mode::SinglePlayer;
		m_worldSeed = GetTickCount();
		m_networkSeedReady = true;
		return;
	}

	m_networkClientToken = HashUInt(GetTickCount() ^ static_cast<unsigned int>(reinterpret_cast<uintptr_t>(this)));
	if (m_networkClientToken == 0)
		m_networkClientToken = 1;

	bool started = false;
	if (m_networkMode == NetworkConfig::Mode::Host)
		started = StartNetworkHost(config.port);
	else if (m_networkMode == NetworkConfig::Mode::Client)
		started = StartNetworkClient(config.host.data(), config.port);

	if (!started)
	{
		ShutdownNetwork();
		m_networkMode = NetworkConfig::Mode::SinglePlayer;
		m_worldSeed = GetTickCount();
		m_networkSeedReady = true;
	}
}

bool GameEngine::StartNetworkHost(unsigned short port)
{
	if (m_networkTransport == nullptr || !m_networkTransport->StartHost(port))
		return false;

	if (m_worldSeed == 0)
		m_worldSeed = GetTickCount();
	m_networkSeedReady = true;
	m_networkConnected = true;
	m_localPlayerId = 1;
	RefreshLocalNetworkAddress();
	return true;
}

bool GameEngine::StartNetworkClient(const char* host, unsigned short port)
{
	if (m_networkTransport == nullptr || !m_networkTransport->StartClient(host, port))
		return false;

	m_localPlayerId = 0;
	SendNetworkHello();
	return true;
}

bool GameEngine::WaitForNetworkSeed(float timeoutSeconds)
{
	const DWORD startTick = GetTickCount();
	DWORD lastHelloTick = 0;
	const DWORD timeoutMs = static_cast<DWORD>((std::max)(0.1f, timeoutSeconds) * 1000.0f);
	while (!m_networkSeedReady && GetTickCount() - startTick < timeoutMs)
	{
		PollNetwork();
		const DWORD now = GetTickCount();
		if (now - lastHelloTick >= static_cast<DWORD>(NetworkHelloInterval * 1000.0f))
		{
			SendNetworkHello();
			lastHelloTick = now;
		}
		Sleep(10);
	}

	return m_networkSeedReady;
}

void GameEngine::ShutdownNetwork()
{
	if (m_networkTransport != nullptr)
	{
		m_networkTransport->Shutdown();
		DeleteNetworkTransport(m_networkTransport);
		m_networkTransport = nullptr;
	}

	m_networkConnected = false;
	m_networkSeedReady = false;
	m_networkPeers.clear();
	m_remotePlayers.clear();
	m_remoteBlockBreaks.clear();
	m_pendingNetworkTileEdits.clear();
	m_pendingNetworkDroppedItems.clear();
	m_networkBreakingBlockIndex = -1;
}

void GameEngine::UpdateStartMenu(float deltaTime)
{
	(void)deltaTime;
	if (m_input == nullptr)
		return;

	if (IsKeyDown(VK_LBUTTON))
	{
		float cursorX = 0.0f;
		float cursorY = 0.0f;
		if (GetCursorViewPosition(cursorX, cursorY))
		{
			const int action = GetStartMenuActionAt(cursorX, cursorY);
			m_startJoinHostEditing = action == StartMenuActionInput;
			if (action == StartMenuActionSingle)
			{
				m_networkConfig.mode = NetworkConfig::Mode::SinglePlayer;
				m_startJoinHostEditing = false;
				SetMultiplayerMenuStatus("싱글플레이 선택됨");
			}
			else if (action == StartMenuActionHost)
			{
				m_networkConfig.mode = NetworkConfig::Mode::Host;
				m_startJoinHostEditing = false;
				SetMultiplayerMenuStatus("호스트 선택됨");
			}
			else if (action == StartMenuActionJoin)
			{
				m_networkConfig.mode = NetworkConfig::Mode::Client;
				m_startJoinHostEditing = true;
				SetMultiplayerMenuStatus("참가 선택됨");
			}
			else if (action == StartMenuActionStart)
			{
				BeginSelectedGameFromMenu();
			}
		}
	}

	if (IsKeyDown(VK_RETURN))
	{
		if (!m_startJoinHostEditing)
			BeginSelectedGameFromMenu();
		else if (m_networkConfig.mode == NetworkConfig::Mode::Client && m_multiplayerJoinHost[0] != '\0')
			BeginSelectedGameFromMenu();
		return;
	}

	if (!m_startJoinHostEditing)
		return;

	if ((IsKeyHeld(VK_CONTROL) || IsKeyHeld(VK_LCONTROL) || IsKeyHeld(VK_RCONTROL)) &&
		IsKeyDown(0x56))
	{
		PasteMultiplayerJoinHostFromClipboard();
		return;
	}

	if (IsKeyDown(VK_BACK))
	{
		const size_t length = std::strlen(m_multiplayerJoinHost.data());
		if (length > 0)
			m_multiplayerJoinHost[length - 1] = '\0';
		return;
	}

	for (int digit = 0; digit <= 9; ++digit)
	{
		if (IsKeyDown(0x30 + digit) || IsKeyDown(VK_NUMPAD0 + digit))
		{
			AppendMultiplayerJoinHostChar(static_cast<char>('0' + digit));
			return;
		}
	}

	if (IsKeyDown(VK_OEM_PERIOD) || IsKeyDown(VK_DECIMAL))
	{
		AppendMultiplayerJoinHostChar('.');
		return;
	}

	if (IsKeyDown(VK_OEM_MINUS) || IsKeyDown(VK_SUBTRACT))
	{
		AppendMultiplayerJoinHostChar('-');
		return;
	}

	for (int letter = 0; letter < 26; ++letter)
	{
		const int keyCode = 0x41 + letter;
		if (IsKeyDown(keyCode))
		{
			AppendMultiplayerJoinHostChar(static_cast<char>('A' + letter));
			return;
		}
	}
}

void GameEngine::DrawStartMenu()
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	DrawSolidRect(0.0f, 0.0f, viewHalfWidth * 2.0f, viewHalfHeight * 2.0f,
		0.0f, 0.0f, 0.0f, 1.0f, -3.0f);

	UiRect panel;
	panel.width = std::clamp(viewHalfWidth * 1.18f, 500.0f, 650.0f);
	panel.height = 382.0f;
	panel.left = -panel.width * 0.5f;
	panel.top = panel.height * 0.5f + 12.0f;
	DrawUiPanel(panel, nullptr);

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const bool hasCursor = GetCursorViewPosition(cursorX, cursorY);
	const int hoverAction = hasCursor ? GetStartMenuActionAt(cursorX, cursorY) : MultiplayerActionNone;
	const NetworkConfig::Mode selectedMode = m_networkConfig.mode;

	auto drawChoice = [&](float centerX, float centerY, float width, float height, const char* text, int action, bool selected)
	{
		const bool hovered = hoverAction == action;
		DrawSolidRect(centerX, centerY, width, height,
			selected ? 0.78f : (hovered ? 0.105f : 0.060f),
			selected ? 0.86f : (hovered ? 0.130f : 0.070f),
			selected ? 0.74f : (hovered ? 0.095f : 0.075f),
			0.98f, -6.2f);

		RectOutlineDesc outline;
		outline.positionX = centerX;
		outline.positionY = centerY;
		outline.width = width;
		outline.height = height;
		outline.thickness = selected ? 2.3f : (hovered ? 1.8f : 1.2f);
		outline.colorR = hovered || selected ? 0.82f : 0.42f;
		outline.colorG = hovered || selected ? 0.88f : 0.48f;
		outline.colorB = hovered || selected ? 0.78f : 0.42f;
		outline.colorA = 0.96f;
		outline.depth = -7.4f;
		m_renderer->DrawRectOutline(outline);

		DrawCenteredUiText(centerX, centerY + 6.0f, text, 1.55f,
			selected ? 0.055f : 0.84f,
			selected ? 0.050f : 0.90f,
			selected ? 0.070f : 0.80f,
			1.0f, -8.0f);
	};

	const float choiceWidth = (panel.width - 68.0f) / 3.0f;
	const float choiceY = panel.top - 54.0f;
	drawChoice(panel.left + 24.0f + choiceWidth * 0.5f, choiceY, choiceWidth, 42.0f,
		"싱글플레이", StartMenuActionSingle, selectedMode == NetworkConfig::Mode::SinglePlayer);
	drawChoice(panel.left + 34.0f + choiceWidth * 1.5f, choiceY, choiceWidth, 42.0f,
		"호스트", StartMenuActionHost, selectedMode == NetworkConfig::Mode::Host);
	drawChoice(panel.left + 44.0f + choiceWidth * 2.5f, choiceY, choiceWidth, 42.0f,
		"참가", StartMenuActionJoin, selectedMode == NetworkConfig::Mode::Client);

	const float inputWidth = panel.width - 56.0f;
	const float inputCenterX = panel.left + panel.width * 0.5f;
	const float inputCenterY = panel.top - 123.0f;
	const bool inputHovered = hoverAction == StartMenuActionInput;
	const bool inputActive = selectedMode == NetworkConfig::Mode::Client;
	DrawSolidRect(inputCenterX, inputCenterY, inputWidth, 34.0f,
		m_startJoinHostEditing ? 0.105f : (inputHovered && inputActive ? 0.090f : 0.060f),
		m_startJoinHostEditing ? 0.130f : (inputHovered && inputActive ? 0.110f : 0.070f),
		m_startJoinHostEditing ? 0.095f : (inputHovered && inputActive ? 0.090f : 0.075f),
		inputActive ? 0.98f : 0.46f, -6.2f);

	RectOutlineDesc inputOutline;
	inputOutline.positionX = inputCenterX;
	inputOutline.positionY = inputCenterY;
	inputOutline.width = inputWidth;
	inputOutline.height = 34.0f;
	inputOutline.thickness = m_startJoinHostEditing ? 2.0f : 1.2f;
	inputOutline.colorR = inputActive ? 0.82f : 0.42f;
	inputOutline.colorG = inputActive ? 0.88f : 0.48f;
	inputOutline.colorB = inputActive ? 0.78f : 0.42f;
	inputOutline.colorA = inputActive ? 0.96f : 0.46f;
	inputOutline.depth = -7.5f;
	m_renderer->DrawRectOutline(inputOutline);

	char inputText[72] = {};
	std::snprintf(inputText, sizeof(inputText), "%s%s", m_multiplayerJoinHost.data(), m_startJoinHostEditing ? "-" : "");
	DrawUiText(inputCenterX - inputWidth * 0.5f + 12.0f, inputCenterY + 6.0f, inputText, 1.55f,
		inputActive ? 0.86f : 0.48f, inputActive ? 0.92f : 0.52f, inputActive ? 0.80f : 0.48f,
		1.0f, -8.0f);

	const float startY = panel.top - 186.0f;
	drawChoice(inputCenterX, startY, 220.0f, 48.0f, "게임 시작", StartMenuActionStart, false);

	if (m_multiplayerMenuStatus[0] != '\0')
		DrawUiText(panel.left + 28.0f, panel.top - 268.0f, m_multiplayerMenuStatus.data(), 1.35f,
			0.86f, 0.92f, 0.80f, 1.0f, -8.0f);
}

void GameEngine::BeginSelectedGameFromMenu()
{
	if (m_gameStarted)
		return;

	NetworkConfig config = m_networkConfig;
	if (config.mode == NetworkConfig::Mode::Client)
	{
		if (m_multiplayerJoinHost[0] == '\0')
		{
			SetMultiplayerMenuStatus("호스트 주소를 입력하세요");
			m_startJoinHostEditing = true;
			return;
		}

		std::snprintf(config.host.data(), config.host.size(), "%s", m_multiplayerJoinHost.data());
	}

	if (m_networkMode != NetworkConfig::Mode::SinglePlayer || m_networkTransport != nullptr)
		ShutdownNetwork();

	m_worldSeed = 0;
	InitializeNetwork(config);
	m_networkConfig = config;

	if (config.mode == NetworkConfig::Mode::SinglePlayer)
	{
		InitializeWorld();
		m_gameStarted = true;
		m_multiplayerMenuOpen = false;
		m_startJoinHostEditing = false;
		SetStatusText("싱글플레이 시작", 1.8f);
		return;
	}

	if (config.mode == NetworkConfig::Mode::Host)
	{
		if (m_networkMode == NetworkConfig::Mode::Host && m_networkConnected)
		{
			InitializeWorld();
			m_gameStarted = true;
			m_multiplayerMenuOpen = false;
			m_startJoinHostEditing = false;
			RefreshLocalNetworkAddress();
			CopyTextToClipboard(m_localNetworkAddress.data());
			char status[64] = {};
			std::snprintf(status, sizeof(status), "호스트 중 %s", m_localNetworkAddress.data());
			SetMultiplayerMenuStatus(status);
			SetStatusText("호스트 준비됨", 2.0f);
			return;
		}

		SetMultiplayerMenuStatus("호스트 실패");
		return;
	}

	if (config.mode == NetworkConfig::Mode::Client)
	{
		if (m_networkMode == NetworkConfig::Mode::Client)
		{
			SetMultiplayerMenuStatus("호스트 참가 중");
			SetStatusText("호스트 참가 중", 1.4f);
			return;
		}

		SetMultiplayerMenuStatus("참가 실패");
	}
}

int GameEngine::GetStartMenuActionAt(float viewX, float viewY) const
{
	const float viewHalfWidth = GetViewHalfWidth();
	UiRect panel;
	panel.width = std::clamp(viewHalfWidth * 1.18f, 500.0f, 650.0f);
	panel.height = 382.0f;
	panel.left = -panel.width * 0.5f;
	panel.top = panel.height * 0.5f + 12.0f;
	const float panelCenterX = panel.left + panel.width * 0.5f;
	const float panelCenterY = panel.top - panel.height * 0.5f;
	if (!IsPointInsideRect(viewX, viewY, panelCenterX, panelCenterY, panel.width, panel.height))
		return MultiplayerActionNone;

	const float choiceWidth = (panel.width - 68.0f) / 3.0f;
	const float choiceY = panel.top - 54.0f;
	if (IsPointInsideRect(viewX, viewY, panel.left + 24.0f + choiceWidth * 0.5f, choiceY, choiceWidth, 42.0f))
		return StartMenuActionSingle;
	if (IsPointInsideRect(viewX, viewY, panel.left + 34.0f + choiceWidth * 1.5f, choiceY, choiceWidth, 42.0f))
		return StartMenuActionHost;
	if (IsPointInsideRect(viewX, viewY, panel.left + 44.0f + choiceWidth * 2.5f, choiceY, choiceWidth, 42.0f))
		return StartMenuActionJoin;

	if (m_networkConfig.mode == NetworkConfig::Mode::Client &&
		IsPointInsideRect(viewX, viewY, panel.left + panel.width * 0.5f, panel.top - 123.0f, panel.width - 56.0f, 34.0f))
	{
		return StartMenuActionInput;
	}

	if (IsPointInsideRect(viewX, viewY, panel.left + panel.width * 0.5f, panel.top - 186.0f, 220.0f, 48.0f))
		return StartMenuActionStart;

	return MultiplayerActionNone;
}

void GameEngine::UpdateMultiplayerMenu(float deltaTime)
{
	(void)deltaTime;
	if (m_input == nullptr)
		return;

	if (IsKeyDown(VK_ESCAPE))
	{
		m_multiplayerMenuOpen = false;
		m_multiplayerJoinHostEditing = false;
		return;
	}

	if (IsKeyDown(VK_LBUTTON))
	{
		float cursorX = 0.0f;
		float cursorY = 0.0f;
		if (GetCursorViewPosition(cursorX, cursorY))
		{
			const int action = GetMultiplayerMenuActionAt(cursorX, cursorY);
			m_multiplayerJoinHostEditing = action == MultiplayerActionInput;
			if (action == MultiplayerActionHost)
			{
				TryBeginMenuHost();
			}
			else if (action == MultiplayerActionJoin)
			{
				TryBeginMenuJoin();
			}
			else if (action == MultiplayerActionClose)
			{
				m_multiplayerMenuOpen = false;
				m_multiplayerJoinHostEditing = false;
			}
			else if (action == MultiplayerActionCopyIp)
			{
				RefreshLocalNetworkAddress();
				if (CopyTextToClipboard(m_localNetworkAddress.data()))
					SetMultiplayerMenuStatus("주소 복사됨");
				else
					SetMultiplayerMenuStatus("복사 실패");
			}
		}
	}

	if (!m_multiplayerJoinHostEditing)
		return;

	if ((IsKeyHeld(VK_CONTROL) || IsKeyHeld(VK_LCONTROL) || IsKeyHeld(VK_RCONTROL)) &&
		IsKeyDown(0x56))
	{
		PasteMultiplayerJoinHostFromClipboard();
		return;
	}

	if (IsKeyDown(VK_BACK))
	{
		const size_t length = std::strlen(m_multiplayerJoinHost.data());
		if (length > 0)
			m_multiplayerJoinHost[length - 1] = '\0';
		return;
	}

	if (IsKeyDown(VK_RETURN))
	{
		TryBeginMenuJoin();
		return;
	}

	for (int digit = 0; digit <= 9; ++digit)
	{
		if (IsKeyDown(0x30 + digit) || IsKeyDown(VK_NUMPAD0 + digit))
		{
			AppendMultiplayerJoinHostChar(static_cast<char>('0' + digit));
			return;
		}
	}

	if (IsKeyDown(VK_OEM_PERIOD) || IsKeyDown(VK_DECIMAL))
	{
		AppendMultiplayerJoinHostChar('.');
		return;
	}

	if (IsKeyDown(VK_OEM_MINUS) || IsKeyDown(VK_SUBTRACT))
	{
		AppendMultiplayerJoinHostChar('-');
		return;
	}

	for (int letter = 0; letter < 26; ++letter)
	{
		const int keyCode = 0x41 + letter;
		if (IsKeyDown(keyCode))
		{
			AppendMultiplayerJoinHostChar(static_cast<char>('A' + letter));
			return;
		}
	}
}

void GameEngine::DrawMultiplayerMenu()
{
	if (!m_multiplayerMenuOpen)
		return;

	const UiRect panel = GetRightPanelRect(2);
	DrawUiPanel(panel, nullptr);
	const float panelLeft = panel.left;
	const float panelTop = panel.top;

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const bool hasCursor = GetCursorViewPosition(cursorX, cursorY);
	const int hoverAction = hasCursor ? GetMultiplayerMenuActionAt(cursorX, cursorY) : MultiplayerActionNone;

	auto drawButton = [&](float centerX, float centerY, float width, float height, const char* text, int action)
	{
		const bool hovered = hoverAction == action;
		DrawSolidRect(centerX, centerY, width, height,
			hovered ? 0.78f : 0.060f,
			hovered ? 0.86f : 0.070f,
			hovered ? 0.74f : 0.075f,
			0.96f, -9.5f);

		RectOutlineDesc buttonOutline;
		buttonOutline.positionX = centerX;
		buttonOutline.positionY = centerY;
		buttonOutline.width = width;
		buttonOutline.height = height;
		buttonOutline.thickness = hovered ? 2.2f : 1.4f;
		buttonOutline.colorR = hovered ? 0.82f : 0.42f;
		buttonOutline.colorG = hovered ? 0.88f : 0.48f;
		buttonOutline.colorB = hovered ? 0.78f : 0.42f;
		buttonOutline.colorA = 0.95f;
		buttonOutline.depth = -10.0f;
		m_renderer->DrawRectOutline(buttonOutline);

		DrawCenteredUiText(centerX, centerY + 5.0f, text, 1.30f,
			hovered ? 0.055f : 0.84f,
			hovered ? 0.050f : 0.90f,
			hovered ? 0.070f : 0.80f,
			1.0f, -10.2f);
	};

	const float buttonWidth = (panel.width - 31.0f) * 0.5f;
	drawButton(panelLeft + 12.0f + buttonWidth * 0.5f, panelTop - 42.0f, buttonWidth, 24.0f, "호스트", MultiplayerActionHost);
	drawButton(panelLeft + 19.0f + buttonWidth * 1.5f, panelTop - 42.0f, buttonWidth, 24.0f, "참가", MultiplayerActionJoin);

	const float inputCenterX = panelLeft + panel.width * 0.5f;
	const float inputCenterY = panelTop - 68.0f;
	const float inputWidth = panel.width - 24.0f;
	const float inputHeight = 24.0f;
	const bool inputHovered = hoverAction == MultiplayerActionInput;
	DrawSolidRect(inputCenterX, inputCenterY, inputWidth, inputHeight,
		m_multiplayerJoinHostEditing ? 0.105f : (inputHovered ? 0.090f : 0.060f),
		m_multiplayerJoinHostEditing ? 0.130f : (inputHovered ? 0.110f : 0.070f),
		m_multiplayerJoinHostEditing ? 0.095f : (inputHovered ? 0.090f : 0.075f),
		0.98f, -9.5f);

	RectOutlineDesc inputOutline;
	inputOutline.positionX = inputCenterX;
	inputOutline.positionY = inputCenterY;
	inputOutline.width = inputWidth;
	inputOutline.height = inputHeight;
	inputOutline.thickness = m_multiplayerJoinHostEditing ? 2.0f : 1.3f;
	inputOutline.colorR = 0.82f;
	inputOutline.colorG = 0.88f;
	inputOutline.colorB = 0.78f;
	inputOutline.colorA = 0.96f;
	inputOutline.depth = -10.0f;
	m_renderer->DrawRectOutline(inputOutline);

	char inputText[72] = {};
	std::snprintf(inputText, sizeof(inputText), "%s%s", m_multiplayerJoinHost.data(), m_multiplayerJoinHostEditing ? "-" : "");
	DrawUiText(inputCenterX - inputWidth * 0.5f + 9.0f, inputCenterY + 5.0f, inputText, 1.24f,
		0.86f, 0.92f, 0.80f, 1.0f, -10.2f);

	char localIpText[80] = {};
	std::snprintf(localIpText, sizeof(localIpText), "내 주소 %s", m_localNetworkAddress[0] != '\0' ? m_localNetworkAddress.data() : "호스트 먼저");
	DrawUiText(panelLeft + 12.0f, panelTop - 93.0f, localIpText, 1.12f,
		0.72f, 0.82f, 0.68f, 1.0f, -10.0f);
	drawButton(panelLeft + 12.0f + buttonWidth * 0.5f, panelTop - 119.0f, buttonWidth, 22.0f, "주소 복사", MultiplayerActionCopyIp);
	drawButton(panelLeft + 19.0f + buttonWidth * 1.5f, panelTop - 119.0f, buttonWidth, 22.0f, "닫기", MultiplayerActionClose);

	if (m_multiplayerMenuStatus[0] != '\0')
	{
		DrawUiText(panelLeft + 12.0f, panelTop - panel.height + 18.0f, m_multiplayerMenuStatus.data(), 1.12f,
			0.86f, 0.92f, 0.80f, 1.0f, -10.0f);
	}
}

void GameEngine::SetMultiplayerMenuStatus(const char* text)
{
	if (text == nullptr)
	{
		m_multiplayerMenuStatus[0] = '\0';
		return;
	}

	std::snprintf(m_multiplayerMenuStatus.data(), m_multiplayerMenuStatus.size(), "%s", text);
}

void GameEngine::TryBeginMenuHost()
{
	if (m_networkMode != NetworkConfig::Mode::SinglePlayer || m_networkTransport != nullptr)
		ShutdownNetwork();

	NetworkConfig config;
	config.mode = NetworkConfig::Mode::Host;
	config.port = 27015;
	InitializeNetwork(config);
	if (m_networkMode == NetworkConfig::Mode::Host && m_networkConnected)
	{
		RefreshLocalNetworkAddress();
		CopyTextToClipboard(m_localNetworkAddress.data());
		char status[64] = {};
		std::snprintf(status, sizeof(status), "호스트 중 %s", m_localNetworkAddress.data());
		SetMultiplayerMenuStatus(status);
		SetStatusText("호스트 준비됨", 1.8f);
		return;
	}

	SetMultiplayerMenuStatus("호스트 실패");
}

void GameEngine::TryBeginMenuJoin()
{
	const char* host = m_multiplayerJoinHost.data();
	if (host == nullptr || host[0] == '\0')
	{
		SetMultiplayerMenuStatus("호스트 주소를 입력하세요");
		return;
	}

	if (m_networkMode != NetworkConfig::Mode::SinglePlayer || m_networkTransport != nullptr)
		ShutdownNetwork();

	NetworkConfig config;
	config.mode = NetworkConfig::Mode::Client;
	config.port = 27015;
	std::snprintf(config.host.data(), config.host.size(), "%s", host);
	InitializeNetwork(config);
	if (m_networkMode == NetworkConfig::Mode::Client)
	{
		SetMultiplayerMenuStatus("호스트 참가 중");
		SetStatusText("호스트 참가 중", 1.4f);
		return;
	}

	SetMultiplayerMenuStatus("참가 실패");
}

void GameEngine::AppendMultiplayerJoinHostChar(char value)
{
	if (!IsHostInputCharacter(value))
		return;

	const size_t length = std::strlen(m_multiplayerJoinHost.data());
	if (length + 1 >= m_multiplayerJoinHost.size())
		return;

	m_multiplayerJoinHost[length] = value;
	m_multiplayerJoinHost[length + 1] = '\0';
}

void GameEngine::PasteMultiplayerJoinHostFromClipboard()
{
	if (!OpenClipboard(nullptr))
		return;

	HANDLE clipboardData = GetClipboardData(CF_TEXT);
	if (clipboardData == nullptr)
	{
		CloseClipboard();
		return;
	}

	const char* text = static_cast<const char*>(GlobalLock(clipboardData));
	if (text == nullptr)
	{
		CloseClipboard();
		return;
	}

	m_multiplayerJoinHost[0] = '\0';
	for (const char* cursor = text; *cursor != '\0'; ++cursor)
	{
		if (!IsHostInputCharacter(*cursor))
			continue;

		AppendMultiplayerJoinHostChar(*cursor);
		if (std::strlen(m_multiplayerJoinHost.data()) + 1 >= m_multiplayerJoinHost.size())
			break;
	}

	GlobalUnlock(clipboardData);
	CloseClipboard();
	SetMultiplayerMenuStatus("붙여넣음");
}

int GameEngine::GetMultiplayerMenuActionAt(float viewX, float viewY) const
{
	const UiRect panel = GetRightPanelRect(2);
	const float panelCenterX = panel.left + panel.width * 0.5f;
	const float panelCenterY = panel.top - panel.height * 0.5f;
	const float panelLeft = panel.left;
	const float panelTop = panel.top;
	const float buttonWidth = (panel.width - 31.0f) * 0.5f;

	if (!IsPointInsideRect(viewX, viewY, panelCenterX, panelCenterY, panel.width, panel.height))
		return MultiplayerActionNone;

	if (IsPointInsideRect(viewX, viewY, panelLeft + 12.0f + buttonWidth * 0.5f, panelTop - 42.0f, buttonWidth, 24.0f))
		return MultiplayerActionHost;
	if (IsPointInsideRect(viewX, viewY, panelLeft + 19.0f + buttonWidth * 1.5f, panelTop - 42.0f, buttonWidth, 24.0f))
		return MultiplayerActionJoin;
	if (IsPointInsideRect(viewX, viewY, panelLeft + panel.width * 0.5f, panelTop - 68.0f, panel.width - 24.0f, 24.0f))
		return MultiplayerActionInput;
	if (IsPointInsideRect(viewX, viewY, panelLeft + 12.0f + buttonWidth * 0.5f, panelTop - 119.0f, buttonWidth, 22.0f))
		return MultiplayerActionCopyIp;
	if (IsPointInsideRect(viewX, viewY, panelLeft + 19.0f + buttonWidth * 1.5f, panelTop - 119.0f, buttonWidth, 22.0f))
		return MultiplayerActionClose;

	return MultiplayerActionNone;
}

bool GameEngine::IsPointInsideRect(float pointX, float pointY, float centerX, float centerY, float width, float height) const
{
	return pointX >= centerX - width * 0.5f &&
		pointX <= centerX + width * 0.5f &&
		pointY >= centerY - height * 0.5f &&
		pointY <= centerY + height * 0.5f;
}

void GameEngine::RefreshLocalNetworkAddress()
{
	if (m_networkTransport != nullptr)
		m_networkTransport->GetLocalAddressText(m_localNetworkAddress.data(), static_cast<int>(m_localNetworkAddress.size()));
	else
		GetPrimaryLocalNetworkAddress(m_localNetworkAddress.data(), static_cast<int>(m_localNetworkAddress.size()));
}

void GameEngine::PollNetwork()
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;

	for (int packetCount = 0; packetCount < 96; ++packetCount)
	{
		char buffer[1200] = {};
		NetworkAddress from = {};
		int received = 0;
		if (!m_networkTransport->Receive(buffer, static_cast<int>(sizeof(buffer)), &received, &from))
			break;

		if (received >= static_cast<int>(sizeof(NetworkPacketHeader)))
			HandleNetworkPacket(buffer, received, from);
	}
}

void GameEngine::UpdateNetwork(float deltaTime)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;

	m_networkTime += deltaTime;
	PollNetwork();

	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
	{
		m_networkHelloTimer += deltaTime;
		if (m_networkHelloTimer >= NetworkHelloInterval)
		{
			m_networkHelloTimer = 0.0f;
			SendNetworkHello();
		}
		return;
	}

	for (RemotePlayerState& remotePlayer : m_remotePlayers)
	{
		if (remotePlayer.active && m_networkTime - remotePlayer.lastHeardTime > NetworkRemoteTimeout)
			remotePlayer.active = false;
	}

	for (RemoteBlockBreakState& remoteBreak : m_remoteBlockBreaks)
	{
		if (remoteBreak.active && m_networkTime - remoteBreak.lastHeardTime > NetworkBlockBreakRemoteTimeout)
			remoteBreak.active = false;
	}

	m_networkSendTimer += deltaTime;
	if (m_networkSendTimer >= NetworkPlayerSendInterval)
	{
		m_networkSendTimer = 0.0f;
		SendLocalPlayerState();
	}

	m_networkItemSendTimer += deltaTime;
	if (m_networkItemSendTimer >= NetworkItemSendInterval)
	{
		m_networkItemSendTimer = 0.0f;
		SendOwnedDroppedItemStates();
	}
}

void GameEngine::SendNetworkHello()
{
	if (m_networkMode != NetworkConfig::Mode::Client || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;

	NetworkHelloPacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::Hello, sizeof(packet));
	packet.token = m_networkClientToken;
	m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::SendLocalPlayerState()
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;

	NetworkPlayerStatePacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::PlayerState, sizeof(packet));
	packet.sequence = ++m_networkPlayerSequence;
	packet.playerId = m_localPlayerId;
	packet.x = m_player.x;
	packet.y = m_player.y;
	packet.velocityX = m_player.velocityX;
	packet.velocityY = m_player.velocityY;
	packet.animationTime = m_player.animationTime;
	packet.attackTimer = m_attackTimer;
	packet.facing = m_player.facing;
	packet.health = m_playerHealth;
	packet.selectedInventorySlot = m_selectedInventorySlot;
	packet.onGround = m_player.onGround ? 1 : 0;

	if (m_networkMode == NetworkConfig::Mode::Host)
		BroadcastNetworkPacket(&packet, sizeof(packet));
	else
		m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::SendWelcomePacket(const NetworkAddress& address, int playerId, unsigned int token)
{
	NetworkWelcomePacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::Welcome, sizeof(packet));
	packet.token = token;
	packet.playerId = playerId;
	packet.worldSeed = m_worldSeed;
	packet.blockWidth = m_blockWidth;
	packet.blockHeight = m_blockHeight;
	packet.spawnX = m_playerSpawnX;
	packet.spawnY = m_playerSpawnY;
	SendNetworkPacket(address, &packet, sizeof(packet));

	for (const NetworkTileEditState& edit : m_networkTileHistory)
		SendTileEditPacket(edit.tileX, edit.tileY, edit.tileIndex, edit.visible, edit.sequence, &address);

	for (DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		EnsureDroppedItemNetworkId(item);
		const int itemOwnerId = static_cast<int>((item.networkId >> 24) & 0xFFu);
		SendDroppedItemPacket(item, itemOwnerId > 0 ? itemOwnerId : m_localPlayerId, &address);
	}
}

void GameEngine::SendTileEditPacket(int tileX, int tileY, unsigned short tileIndex, unsigned char visible, unsigned int sequence, const NetworkAddress* targetAddress)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;

	NetworkTileEditPacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::TileEdit, sizeof(packet));
	packet.sequence = sequence;
	packet.playerId = m_localPlayerId;
	packet.tileX = tileX;
	packet.tileY = tileY;
	packet.tileIndex = tileIndex;
	packet.visible = visible;

	if (targetAddress != nullptr)
	{
		SendNetworkPacket(*targetAddress, &packet, sizeof(packet));
		return;
	}

	if (m_networkMode == NetworkConfig::Mode::Host)
		BroadcastNetworkPacket(&packet, sizeof(packet));
	else if (m_networkConnected)
		m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::BroadcastTileEdit(int tileX, int tileY, unsigned short tileIndex, unsigned char visible)
{
	if (m_networkMode != NetworkConfig::Mode::Host)
		return;

	const unsigned int sequence = ++m_networkTileSequence;
	NetworkTileEditState edit;
	edit.tileX = tileX;
	edit.tileY = tileY;
	edit.tileIndex = tileIndex;
	edit.visible = visible;
	edit.sequence = sequence;
	m_networkTileHistory.push_back(edit);
	SendTileEditPacket(tileX, tileY, tileIndex, visible, sequence, nullptr);
}

void GameEngine::PublishLocalTileEdit(int tileX, int tileY)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || !IsTileInBounds(tileX, tileY))
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;

	const BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
	if (m_networkMode == NetworkConfig::Mode::Host)
	{
		BroadcastTileEdit(tileX, tileY, tile.tileIndex, tile.visible);
		return;
	}

	SendTileEditPacket(tileX, tileY, tile.tileIndex, tile.visible, ++m_networkTileSequence, nullptr);
}

void GameEngine::ApplyNetworkTileEdit(int tileX, int tileY, unsigned short tileIndex, unsigned char visible)
{
	if (!IsTileInBounds(tileX, tileY))
		return;

	if (m_blocks.size() != static_cast<size_t>(m_blockWidth * m_blockHeight))
	{
		NetworkTileEditState edit;
		edit.tileX = tileX;
		edit.tileY = tileY;
		edit.tileIndex = tileIndex;
		edit.visible = visible;
		m_pendingNetworkTileEdits.push_back(edit);
		return;
	}

	const int blockIndex = tileY * m_blockWidth + tileX;
	m_blocks[blockIndex].tileIndex = tileIndex;
	m_blocks[blockIndex].visible = visible != 0 ? 1 : 0;
	if (blockIndex >= 0 && blockIndex < static_cast<int>(m_blockBreaks.size()))
		m_blockBreaks[blockIndex] = BlockBreakState();
	for (RemoteBlockBreakState& remoteBreak : m_remoteBlockBreaks)
	{
		if (remoteBreak.tileX == tileX && remoteBreak.tileY == tileY)
			remoteBreak.active = false;
	}
	MarkBlockChunkDirty(tileX, tileY);
}

void GameEngine::SendBlockBreakPacket(int playerId, int tileX, int tileY, float progress, unsigned char active, const NetworkAddress* targetAddress)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;

	NetworkBlockBreakPacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::BlockBreak, sizeof(packet));
	packet.playerId = playerId;
	packet.tileX = tileX;
	packet.tileY = tileY;
	packet.progress = Clamp01(progress);
	packet.active = active;

	if (targetAddress != nullptr)
	{
		SendNetworkPacket(*targetAddress, &packet, sizeof(packet));
		return;
	}

	if (m_networkMode == NetworkConfig::Mode::Host)
		BroadcastNetworkPacket(&packet, sizeof(packet));
	else if (m_networkConnected)
		m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::PublishBlockBreakState(int tileX, int tileY, float progress, bool active)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;
	if (!IsTileInBounds(tileX, tileY))
		return;

	SendBlockBreakPacket(m_localPlayerId, tileX, tileY, progress, active ? 1 : 0, nullptr);
}

void GameEngine::ApplyNetworkBlockBreakState(int playerId, int tileX, int tileY, float progress, bool active)
{
	if (playerId == m_localPlayerId || playerId <= 0 || !IsTileInBounds(tileX, tileY))
		return;

	RemoteBlockBreakState* state = nullptr;
	for (RemoteBlockBreakState& remoteBreak : m_remoteBlockBreaks)
	{
		if (remoteBreak.playerId == playerId)
		{
			state = &remoteBreak;
			break;
		}
	}

	if (!active)
	{
		if (state != nullptr && state->tileX == tileX && state->tileY == tileY)
			state->active = false;
		return;
	}

	if (m_blocks.size() != static_cast<size_t>(m_blockWidth * m_blockHeight))
		return;

	const int blockIndex = tileY * m_blockWidth + tileX;
	if (m_blocks[blockIndex].visible == 0)
		return;

	if (state == nullptr)
	{
		for (RemoteBlockBreakState& remoteBreak : m_remoteBlockBreaks)
		{
			if (!remoteBreak.active)
			{
				state = &remoteBreak;
				break;
			}
		}
	}

	if (state == nullptr)
	{
		if (static_cast<int>(m_remoteBlockBreaks.size()) >= NetworkMaxPeers)
			return;

		m_remoteBlockBreaks.push_back(RemoteBlockBreakState());
		state = &m_remoteBlockBreaks.back();
	}

	state->playerId = playerId;
	state->tileX = tileX;
	state->tileY = tileY;
	state->progress = Clamp01(progress);
	state->lastHeardTime = m_networkTime;
	state->active = true;
}

void GameEngine::SendLeafEffectPacket(int playerId, int tileX, int tileY, unsigned int seed, const NetworkAddress* targetAddress)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;

	NetworkLeafEffectPacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::LeafEffect, sizeof(packet));
	packet.playerId = playerId;
	packet.tileX = tileX;
	packet.tileY = tileY;
	packet.seed = seed;

	if (targetAddress != nullptr)
	{
		SendNetworkPacket(*targetAddress, &packet, sizeof(packet));
		return;
	}

	if (m_networkMode == NetworkConfig::Mode::Host)
		BroadcastNetworkPacket(&packet, sizeof(packet));
	else if (m_networkConnected)
		m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::PublishLeafBreakEffect(int tileX, int tileY, unsigned int seed)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;
	if (!IsTileInBounds(tileX, tileY))
		return;

	SendLeafEffectPacket(m_localPlayerId, tileX, tileY, seed, nullptr);
}

void GameEngine::ApplyNetworkLeafBreakEffect(int tileX, int tileY, unsigned int seed)
{
	SpawnLeafBreakEffectWithSeed(tileX, tileY, seed);
}

void GameEngine::SendDroppedItemPacket(const DroppedItemState& item, int playerId, const NetworkAddress* targetAddress)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen() || item.networkId == 0)
		return;

	NetworkDroppedItemPacket packet;
	packet.header = MakeNetworkHeader(NetworkPacketType::DroppedItem, sizeof(packet));
	packet.playerId = playerId;
	packet.networkId = item.networkId;
	packet.x = item.x;
	packet.y = item.y;
	packet.velocityX = item.velocityX;
	packet.velocityY = item.velocityY;
	packet.pickupDelay = item.pickupDelay;
	packet.tileIndex = item.tileIndex;
	packet.amount = item.amount;
	packet.pickupPlayerId = item.pickupPlayerId != 0 ? item.pickupPlayerId : m_localPlayerId;
	packet.alive = item.alive ? 1 : 0;

	if (targetAddress != nullptr)
	{
		SendNetworkPacket(*targetAddress, &packet, sizeof(packet));
		return;
	}

	if (m_networkMode == NetworkConfig::Mode::Host)
		BroadcastNetworkPacket(&packet, sizeof(packet));
	else if (m_networkConnected)
		m_networkTransport->SendToServer(&packet, sizeof(packet));
}

void GameEngine::PublishDroppedItemState(const DroppedItemState& item)
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;

	SendDroppedItemPacket(item, m_localPlayerId, nullptr);
}

void GameEngine::ApplyNetworkDroppedItemState(unsigned int networkId, int playerId, int pickupPlayerId, float x, float y, float velocityX, float velocityY, float pickupDelay, unsigned short tileIndex, int amount, unsigned char alive)
{
	if (networkId == 0 || amount <= 0)
		return;

	const int resolvedPickupPlayerId = pickupPlayerId > 0 ? pickupPlayerId : playerId;
	if (m_blocks.size() != static_cast<size_t>(m_blockWidth * m_blockHeight))
	{
		DroppedItemState pendingItem;
		pendingItem.x = x;
		pendingItem.y = y;
		pendingItem.velocityX = velocityX;
		pendingItem.velocityY = velocityY;
		pendingItem.pickupDelay = (std::max)(0.0f, pickupDelay);
		pendingItem.tileIndex = tileIndex;
		pendingItem.networkId = networkId;
		pendingItem.amount = amount;
		pendingItem.pickupPlayerId = resolvedPickupPlayerId;
		pendingItem.alive = alive != 0;
		m_pendingNetworkDroppedItems.push_back(pendingItem);
		return;
	}

	DroppedItemState* item = FindDroppedItemByNetworkId(networkId);
	if (alive == 0)
	{
		if (item != nullptr)
			item->alive = false;
		return;
	}

	if (item != nullptr && !item->alive)
		return;

	DroppedItemState state;
	state.x = x;
	state.y = y;
	state.velocityX = velocityX;
	state.velocityY = velocityY;
	state.pickupDelay = (std::max)(0.0f, pickupDelay);
	state.tileIndex = tileIndex;
	state.networkId = networkId;
	state.amount = amount;
	state.pickupPlayerId = resolvedPickupPlayerId;
	state.alive = true;

	if (item != nullptr)
	{
		*item = state;
		return;
	}

	for (DroppedItemState& existing : m_droppedItems)
	{
		if (existing.alive)
			continue;

		existing = state;
		return;
	}

	m_droppedItems.push_back(state);
}

void GameEngine::SendOwnedDroppedItemStates()
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || m_networkTransport == nullptr || !m_networkTransport->IsOpen())
		return;
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		return;

	for (DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		EnsureDroppedItemNetworkId(item);
		if (IsNetworkItemOwnedByLocal(item))
			SendDroppedItemPacket(item, m_localPlayerId, nullptr);
	}
}

unsigned int GameEngine::GenerateNetworkItemId()
{
	const int safePlayerId = std::clamp(m_localPlayerId <= 0 ? 1 : m_localPlayerId, 1, 255);
	const unsigned int playerBits = static_cast<unsigned int>(safePlayerId) << 24;
	for (int attempt = 0; attempt < 0x00FFFFFF; ++attempt)
	{
		m_networkItemSequence = (m_networkItemSequence + 1u) & 0x00FFFFFFu;
		if (m_networkItemSequence == 0)
			m_networkItemSequence = 1;

		const unsigned int networkId = playerBits | m_networkItemSequence;
		if (FindDroppedItemByNetworkId(networkId) == nullptr)
			return networkId;
	}

	return playerBits | (HashUInt(GetTickCount()) & 0x00FFFFFFu);
}

void GameEngine::EnsureDroppedItemNetworkId(DroppedItemState& item)
{
	if (item.networkId == 0 && m_networkMode != NetworkConfig::Mode::SinglePlayer)
		item.networkId = GenerateNetworkItemId();
}

GameEngine::DroppedItemState* GameEngine::FindDroppedItemByNetworkId(unsigned int networkId)
{
	if (networkId == 0)
		return nullptr;

	for (DroppedItemState& item : m_droppedItems)
	{
		if (item.networkId == networkId)
			return &item;
	}

	return nullptr;
}

bool GameEngine::IsNetworkItemOwnedByLocal(const DroppedItemState& item) const
{
	if (item.networkId == 0)
		return true;

	const int itemOwnerId = static_cast<int>((item.networkId >> 24) & 0xFFu);
	return itemOwnerId == m_localPlayerId;
}

int GameEngine::ChooseDroppedItemPickupPlayer(float worldX, float worldY) const
{
	const int localPlayerId = m_localPlayerId > 0 ? m_localPlayerId : 1;
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return localPlayerId;

	int bestPlayerId = localPlayerId;
	const float localDeltaX = m_player.x - worldX;
	const float localDeltaY = m_player.y - worldY;
	float bestDistanceSq = localDeltaX * localDeltaX + localDeltaY * localDeltaY;
	const float claimRadius = m_tileSize * 7.5f;
	const float claimRadiusSq = claimRadius * claimRadius;

	for (const RemotePlayerState& remotePlayer : m_remotePlayers)
	{
		if (!remotePlayer.active || remotePlayer.id <= 0)
			continue;

		const float deltaX = remotePlayer.player.x - worldX;
		const float deltaY = remotePlayer.player.y - worldY;
		const float distanceSq = deltaX * deltaX + deltaY * deltaY;
		if (distanceSq <= claimRadiusSq && distanceSq < bestDistanceSq)
		{
			bestDistanceSq = distanceSq;
			bestPlayerId = remotePlayer.id;
		}
	}

	return bestPlayerId;
}

bool GameEngine::CanLocalPlayerPickupItem(const DroppedItemState& item) const
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return true;

	return item.pickupPlayerId == 0 || item.pickupPlayerId == m_localPlayerId;
}

bool GameEngine::TryGetDroppedItemPickupPosition(const DroppedItemState& item, float& targetX, float& targetY) const
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer || item.pickupPlayerId == 0 || item.pickupPlayerId == m_localPlayerId)
	{
		targetX = m_player.x;
		targetY = m_player.y;
		return true;
	}

	const int remotePlayerIndex = FindRemotePlayer(item.pickupPlayerId);
	if (remotePlayerIndex < 0)
		return false;

	const RemotePlayerState& remotePlayer = m_remotePlayers[remotePlayerIndex];
	if (!remotePlayer.active)
		return false;

	targetX = remotePlayer.player.x;
	targetY = remotePlayer.player.y;
	return true;
}

bool GameEngine::SendNetworkPacket(const NetworkAddress& address, const void* packet, int packetSize)
{
	if (m_networkTransport == nullptr || !m_networkTransport->IsOpen() || packet == nullptr || packetSize <= 0)
		return false;

	return m_networkTransport->Send(address, packet, packetSize);
}

void GameEngine::BroadcastNetworkPacket(const void* packet, int packetSize, const NetworkAddress* exceptAddress)
{
	if (m_networkMode != NetworkConfig::Mode::Host)
		return;

	for (const NetworkPeerState& peer : m_networkPeers)
	{
		if (!peer.active)
			continue;
		if (exceptAddress != nullptr && NetworkAddressesEqual(peer.address, *exceptAddress))
			continue;

		SendNetworkPacket(peer.address, packet, packetSize);
	}
}

void GameEngine::HandleNetworkPacket(const char* data, int dataSize, const NetworkAddress& from)
{
	if (data == nullptr || dataSize < static_cast<int>(sizeof(NetworkPacketHeader)))
		return;

	const NetworkPacketHeader* header = reinterpret_cast<const NetworkPacketHeader*>(data);
	if (header->magic != NetworkMagic || header->version != NetworkVersion || header->size != dataSize)
		return;

	const NetworkPacketType packetType = static_cast<NetworkPacketType>(header->type);
	switch (packetType)
	{
	case NetworkPacketType::Hello:
	{
		if (m_networkMode != NetworkConfig::Mode::Host || dataSize != static_cast<int>(sizeof(NetworkHelloPacket)))
			return;

		const NetworkHelloPacket* packet = reinterpret_cast<const NetworkHelloPacket*>(data);
		int peerIndex = FindNetworkPeer(from);
		if (peerIndex < 0)
		{
			if (static_cast<int>(m_networkPeers.size()) >= NetworkMaxPeers)
				return;

			NetworkPeerState peer;
			peer.address = from;
			peer.lastHeardTime = m_networkTime;
			peer.token = packet->token;
			peer.playerId = m_nextNetworkPlayerId++;
			peer.active = true;
			m_networkPeers.push_back(peer);
			peerIndex = static_cast<int>(m_networkPeers.size()) - 1;
		}

		NetworkPeerState& peer = m_networkPeers[peerIndex];
		peer.lastHeardTime = m_networkTime;
		peer.token = packet->token;
		peer.active = true;
		SendWelcomePacket(from, peer.playerId, peer.token);
		break;
	}
	case NetworkPacketType::Welcome:
	{
		if (m_networkMode != NetworkConfig::Mode::Client || dataSize != static_cast<int>(sizeof(NetworkWelcomePacket)))
			return;

		const NetworkWelcomePacket* packet = reinterpret_cast<const NetworkWelcomePacket*>(data);
		if (packet->token != m_networkClientToken)
			return;
		if (packet->blockWidth != m_blockWidth || packet->blockHeight != m_blockHeight)
			return;

		const bool shouldRebuildWorld = !m_gameStarted || m_blocks.empty() || m_worldSeed != packet->worldSeed;
		m_localPlayerId = packet->playerId;
		m_worldSeed = packet->worldSeed;
		m_networkSeedReady = true;
		m_networkConnected = true;
		if (shouldRebuildWorld)
			InitializeWorld();
		m_gameStarted = true;
		m_multiplayerMenuOpen = false;
		m_startJoinHostEditing = false;
		m_multiplayerJoinHostEditing = false;
		SetMultiplayerMenuStatus("호스트 참가 완료");
		SetStatusText("호스트 참가 완료", 2.0f);
		break;
	}
	case NetworkPacketType::PlayerState:
	{
		if (dataSize != static_cast<int>(sizeof(NetworkPlayerStatePacket)))
			return;

		const NetworkPlayerStatePacket* packet = reinterpret_cast<const NetworkPlayerStatePacket*>(data);
		if (packet->playerId == m_localPlayerId || packet->playerId <= 0)
			return;

		if (m_networkMode == NetworkConfig::Mode::Host)
		{
			const int peerIndex = FindNetworkPeer(from);
			if (peerIndex < 0 || m_networkPeers[peerIndex].playerId != packet->playerId)
				return;

			m_networkPeers[peerIndex].lastHeardTime = m_networkTime;
			BroadcastNetworkPacket(packet, sizeof(*packet), &from);
		}

		RemotePlayerState* remotePlayer = GetOrCreateRemotePlayer(packet->playerId);
		if (remotePlayer == nullptr)
			return;

		remotePlayer->id = packet->playerId;
		remotePlayer->player.x = packet->x;
		remotePlayer->player.y = packet->y;
		remotePlayer->player.velocityX = packet->velocityX;
		remotePlayer->player.velocityY = packet->velocityY;
		remotePlayer->player.animationTime = packet->animationTime;
		remotePlayer->player.facing = packet->facing == 0 ? 1 : packet->facing;
		remotePlayer->player.onGround = packet->onGround != 0;
		remotePlayer->attackTimer = packet->attackTimer;
		remotePlayer->health = packet->health;
		remotePlayer->selectedInventorySlot = packet->selectedInventorySlot;
		remotePlayer->lastHeardTime = m_networkTime;
		remotePlayer->active = true;
		break;
	}
	case NetworkPacketType::TileEdit:
	{
		if (dataSize != static_cast<int>(sizeof(NetworkTileEditPacket)))
			return;

		const NetworkTileEditPacket* packet = reinterpret_cast<const NetworkTileEditPacket*>(data);
		if (m_networkMode == NetworkConfig::Mode::Host)
		{
			const int peerIndex = FindNetworkPeer(from);
			if (peerIndex < 0 || m_networkPeers[peerIndex].playerId != packet->playerId)
				return;

			m_networkPeers[peerIndex].lastHeardTime = m_networkTime;
			ApplyNetworkTileEdit(packet->tileX, packet->tileY, packet->tileIndex, packet->visible);
			BroadcastTileEdit(packet->tileX, packet->tileY, packet->tileIndex, packet->visible);
			return;
		}

		if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		{
			NetworkTileEditState edit;
			edit.tileX = packet->tileX;
			edit.tileY = packet->tileY;
			edit.tileIndex = packet->tileIndex;
			edit.visible = packet->visible;
			edit.sequence = packet->sequence;
			m_pendingNetworkTileEdits.push_back(edit);
			return;
		}

		ApplyNetworkTileEdit(packet->tileX, packet->tileY, packet->tileIndex, packet->visible);
		break;
	}
	case NetworkPacketType::BlockBreak:
	{
		if (dataSize != static_cast<int>(sizeof(NetworkBlockBreakPacket)))
			return;

		const NetworkBlockBreakPacket* packet = reinterpret_cast<const NetworkBlockBreakPacket*>(data);
		if (packet->playerId == m_localPlayerId || packet->playerId <= 0)
			return;

		if (m_networkMode == NetworkConfig::Mode::Host)
		{
			const int peerIndex = FindNetworkPeer(from);
			if (peerIndex < 0 || m_networkPeers[peerIndex].playerId != packet->playerId)
				return;

			m_networkPeers[peerIndex].lastHeardTime = m_networkTime;
			ApplyNetworkBlockBreakState(packet->playerId, packet->tileX, packet->tileY, packet->progress, packet->active != 0);
			BroadcastNetworkPacket(packet, sizeof(*packet), &from);
			return;
		}

		if (m_networkMode == NetworkConfig::Mode::Client && m_networkConnected)
			ApplyNetworkBlockBreakState(packet->playerId, packet->tileX, packet->tileY, packet->progress, packet->active != 0);
		break;
	}
	case NetworkPacketType::LeafEffect:
	{
		if (dataSize != static_cast<int>(sizeof(NetworkLeafEffectPacket)))
			return;

		const NetworkLeafEffectPacket* packet = reinterpret_cast<const NetworkLeafEffectPacket*>(data);
		if (packet->playerId == m_localPlayerId || packet->playerId <= 0)
			return;

		if (m_networkMode == NetworkConfig::Mode::Host)
		{
			const int peerIndex = FindNetworkPeer(from);
			if (peerIndex < 0 || m_networkPeers[peerIndex].playerId != packet->playerId)
				return;

			m_networkPeers[peerIndex].lastHeardTime = m_networkTime;
			ApplyNetworkLeafBreakEffect(packet->tileX, packet->tileY, packet->seed);
			BroadcastNetworkPacket(packet, sizeof(*packet), &from);
			return;
		}

		if (m_networkMode == NetworkConfig::Mode::Client && m_networkConnected)
			ApplyNetworkLeafBreakEffect(packet->tileX, packet->tileY, packet->seed);
		break;
	}
	case NetworkPacketType::DroppedItem:
	{
		if (dataSize != static_cast<int>(sizeof(NetworkDroppedItemPacket)))
			return;

		const NetworkDroppedItemPacket* packet = reinterpret_cast<const NetworkDroppedItemPacket*>(data);
		if (packet->playerId <= 0 || packet->networkId == 0)
			return;

		const int itemOwnerId = static_cast<int>((packet->networkId >> 24) & 0xFFu);
		if (packet->alive != 0 && itemOwnerId != packet->playerId)
			return;

		if (m_networkMode == NetworkConfig::Mode::Host)
		{
			const int peerIndex = FindNetworkPeer(from);
			if (peerIndex < 0 || m_networkPeers[peerIndex].playerId != packet->playerId)
				return;

			m_networkPeers[peerIndex].lastHeardTime = m_networkTime;
			ApplyNetworkDroppedItemState(packet->networkId, packet->playerId, packet->pickupPlayerId, packet->x, packet->y, packet->velocityX, packet->velocityY, packet->pickupDelay, packet->tileIndex, packet->amount, packet->alive);
			BroadcastNetworkPacket(packet, sizeof(*packet), &from);
			return;
		}

		if (m_networkMode == NetworkConfig::Mode::Client && m_networkConnected)
			ApplyNetworkDroppedItemState(packet->networkId, packet->playerId, packet->pickupPlayerId, packet->x, packet->y, packet->velocityX, packet->velocityY, packet->pickupDelay, packet->tileIndex, packet->amount, packet->alive);
		break;
	}
	default:
		break;
	}
}

int GameEngine::FindNetworkPeer(const NetworkAddress& address) const
{
	for (size_t i = 0; i < m_networkPeers.size(); ++i)
	{
		const NetworkPeerState& peer = m_networkPeers[i];
		if (NetworkAddressesEqual(peer.address, address))
			return static_cast<int>(i);
	}

	return -1;
}

int GameEngine::FindRemotePlayer(int playerId) const
{
	for (size_t i = 0; i < m_remotePlayers.size(); ++i)
	{
		if (m_remotePlayers[i].id == playerId)
			return static_cast<int>(i);
	}

	return -1;
}

GameEngine::RemotePlayerState* GameEngine::GetOrCreateRemotePlayer(int playerId)
{
	if (playerId == m_localPlayerId || playerId <= 0)
		return nullptr;

	const int existingIndex = FindRemotePlayer(playerId);
	if (existingIndex >= 0)
		return &m_remotePlayers[existingIndex];

	if (static_cast<int>(m_remotePlayers.size()) >= NetworkMaxPeers)
		return nullptr;

	RemotePlayerState remotePlayer;
	remotePlayer.id = playerId;
	remotePlayer.health = GetPlayerMaxHealth();
	remotePlayer.active = true;
	m_remotePlayers.push_back(remotePlayer);
	return &m_remotePlayers.back();
}

const char* GameEngine::GetNetworkModeText() const
{
	if (m_networkMode == NetworkConfig::Mode::Host)
		return "호스트";
	if (m_networkMode == NetworkConfig::Mode::Client)
		return "참가";
	return "혼자";
}
