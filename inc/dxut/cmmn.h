
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>

#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "d3dx12.h"
#include <DirectXMath.h>

#include <wrl.h>
#include <vector>
#include <memory>
#include <map>

#include <ppl.h>
#include <ppltasks.h>

using namespace DirectX;
using namespace std;
using namespace Microsoft::WRL;
using namespace concurrency;

inline size_t aligned_size256(size_t s) {
	return (s + 255) & ~255;
}

inline float randf() {
	return ((float)rand()) / (float)RAND_MAX;
}

inline void chk(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

#include <codecvt>
inline wstring s2ws(const std::string& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_typeX;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

inline string ws2s(const std::wstring& wstr)
{
	typedef std::codecvt_utf8<wchar_t> convert_typeX;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}