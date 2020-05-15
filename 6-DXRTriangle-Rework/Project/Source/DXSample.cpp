//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "DXSample.h"

#include <codecvt>


#include "Win32Application.h"

using namespace Microsoft::WRL;

DXSample::DXSample(const UINT width, const UINT height, const std::wstring name) :
	m_width(width),
	m_height(height),
	m_useWarpDevice(false),
	m_title(name)
{
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

// Helper function for setting the window's title text.
void DXSample::SetCustomWindowText(const LPCWSTR text) const
{
	const std::wstring windowText = m_title + L": " + text;
	SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}

void DXSample::msgBox(const std::string& msg)
{
	MessageBoxA(Win32Application::GetHwnd(), msg.c_str(), "Error", MB_OK);
}

void DXSample::d3dTraceHR(const std::string& msg, const HRESULT hr)
{
	char hr_msg[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

	const std::string error_msg = msg + ".\nError! " + hr_msg;
	msgBox(error_msg);
}

std::wstring DXSample::string_2_wstring(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
	std::wstring ws = cvt.from_bytes(s);
	return ws;
}

std::string DXSample::wstring_2_string(const std::wstring& ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	std::string s = cvt.to_bytes(ws);
	return s;
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_

void DXSample::ParseCommandLineArgs(WCHAR* argv[], const int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			m_useWarpDevice = true;
			m_title = m_title + L" (WARP)";
		}
	}
}
