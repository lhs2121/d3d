#include "pch.h"
#include <Windows.h>
#include <inputlib/Interface.h>
#include <commonlib/Interface.h>
#include <rendererlib/Interface.h>
#include <windowlib/Interface.h>
#include "GameEngine.h"
#include <cwchar>
#include <string>
#include <vector>

namespace
{
	std::vector<std::wstring> TokenizeCommandLine(LPWSTR commandLine)
	{
		std::vector<std::wstring> tokens;
		if (commandLine == nullptr)
			return tokens;

		const wchar_t* cursor = commandLine;
		while (*cursor != L'\0')
		{
			while (*cursor == L' ' || *cursor == L'\t')
				++cursor;
			if (*cursor == L'\0')
				break;

			std::wstring token;
			bool quoted = false;
			while (*cursor != L'\0')
			{
				if (*cursor == L'"')
				{
					quoted = !quoted;
					++cursor;
					continue;
				}
				if (!quoted && (*cursor == L' ' || *cursor == L'\t'))
					break;

				token.push_back(*cursor);
				++cursor;
			}

			if (!token.empty())
				tokens.push_back(token);
		}

		return tokens;
	}

	bool IsArg(const std::wstring& value, const wchar_t* expected)
	{
		return _wcsicmp(value.c_str(), expected) == 0;
	}

	bool TryParsePort(const std::wstring& value, unsigned short& port)
	{
		if (value.empty())
			return false;

		wchar_t* end = nullptr;
		const long parsed = std::wcstol(value.c_str(), &end, 10);
		if (end == value.c_str() || *end != L'\0' || parsed <= 0 || parsed > 65535)
			return false;

		port = static_cast<unsigned short>(parsed);
		return true;
	}

	void CopyHost(std::array<char, 64>& destination, const std::wstring& host)
	{
		destination.fill('\0');
		WideCharToMultiByte(
			CP_UTF8,
			0,
			host.c_str(),
			-1,
			destination.data(),
			static_cast<int>(destination.size()),
			nullptr,
			nullptr);
		destination.back() = '\0';
	}

	GameEngine::NetworkConfig ParseNetworkConfig(LPWSTR commandLine)
	{
		GameEngine::NetworkConfig config;
		const std::vector<std::wstring> tokens = TokenizeCommandLine(commandLine);
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (IsArg(tokens[i], L"-host"))
			{
				config.mode = GameEngine::NetworkConfig::Mode::Host;
				if (i + 1 < tokens.size())
				{
					unsigned short parsedPort = config.port;
					if (TryParsePort(tokens[i + 1], parsedPort))
					{
						config.port = parsedPort;
						++i;
					}
				}
				continue;
			}

			if (IsArg(tokens[i], L"-join") && i + 1 < tokens.size())
			{
				config.mode = GameEngine::NetworkConfig::Mode::Client;
				CopyHost(config.host, tokens[i + 1]);
				++i;
				if (i + 1 < tokens.size())
				{
					unsigned short parsedPort = config.port;
					if (TryParsePort(tokens[i + 1], parsedPort))
					{
						config.port = parsedPort;
						++i;
					}
				}
			}
		}

		return config;
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//Debug::CrtSetBreakAlloc(406);
	Debug::CrtSetDbgFlag();

	GameEngine engine;
	const GameEngine::NetworkConfig networkConfig = ParseNetworkConfig(lpCmdLine);
	const char* title = "LegoEngine <DX11>";
	if (networkConfig.mode == GameEngine::NetworkConfig::Mode::Host)
		title = "LegoEngine <DX11> HOST";
	else if (networkConfig.mode == GameEngine::NetworkConfig::Mode::Client)
		title = "LegoEngine <DX11> JOIN";

	engine.Start(title, 50, 50, 1366, 789, hInstance, networkConfig);
}
