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

#pragma once

#include <dxgi1_4.h>

class DXSample
{
public:
	DXSample(UINT width, UINT height, std::wstring name);
	virtual ~DXSample() = default;

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	// Samples override the event handlers to handle specific messages.
	virtual void OnKeyDown(UINT8 /*key*/)
	{
	}

	virtual void OnKeyUp(UINT8 /*key*/)
	{
	}

	// Accessors.
	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	const WCHAR* GetTitle() const { return m_title.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	// Convert a string to a wide-string
	static std::wstring string_2_wstring(const std::string& s);

	// Convert a wide-string to a string
	static std::string wstring_2_string(const std::wstring& ws);

	// Convert a blob to at string
	template<class BlotType>
	static std::string convertBlobToString(BlotType* pBlob)
	{
		std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
		memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
		infoLog[pBlob->GetBufferSize()] = 0;
		return std::string(infoLog.data());
	}
protected:
	void SetCustomWindowText(LPCWSTR text) const;

	// Viewport dimensions.
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;

	// Adapter info.
	bool m_useWarpDevice;

	//Error functions
	static void msgBox(const std::string& msg);

	static void d3dTraceHR(const std::string& msg, HRESULT hr);

	//Define NVIDIA utils
#define d3d_call(a) {HRESULT hr_ = a; if(FAILED(hr_)) { d3dTraceHR( #a, hr_); }}
#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

private:
	// Window title.
	std::wstring m_title;
};
