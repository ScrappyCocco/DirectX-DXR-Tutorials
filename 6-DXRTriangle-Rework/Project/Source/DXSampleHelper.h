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

inline void ThrowIfFailed(const HRESULT hr, const std::string& error = "Generic Error")
{
	if (FAILED(hr))
	{
		throw std::runtime_error(error);
	}
}

inline void ThrowError(const std::string& error)
{
	throw std::runtime_error(error);
}
