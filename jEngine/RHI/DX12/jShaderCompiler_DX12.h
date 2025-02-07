#pragma once
#include "dxcapi.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

//////////////////////////////////////////////////////////////////////////
// jDXC
class jDXC
{
public:
	jDXC() {}
	jDXC(jDXC&& InOther)
	{
		m_dll = InOther.m_dll;
		InOther.m_dll = nullptr;

		m_createFn = InOther.m_createFn;
		InOther.m_createFn = nullptr;

		m_createFn2 = InOther.m_createFn2;
		InOther.m_createFn2 = nullptr;
	}
	~jDXC();

	HRESULT Initialize() { return Initialize(TEXT("dxcompiler.dll"), "DxcCreateInstance"); }
	HRESULT Initialize(const wchar_t* InDllName, const char* InFnName);
	void Release();

	template <typename T>
	HRESULT CreateInstance(REFCLSID InCLSID, T** pResult) const
	{
		return CreateInstance(InCLSID, __uuidof(T), (IUnknown**)pResult);
	}

	template <typename T>
	HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID InCLSID, T** pResult) const
	{
		return CreateInstance2(pMalloc, InCLSID, __uuidof(T), (IUnknown**)pResult);
	}

	HRESULT CreateInstance(REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const;
	HRESULT CreateInstance2(IMalloc* pMalloc, REFCLSID InCLSID, REFIID InIID, IUnknown** pResult) const;

	bool HasCreateWithMalloc() const { return m_createFn2; }
	bool IsEnable() const { return m_dll; }
	HMODULE Detach();

private:

private:
	HMODULE m_dll = nullptr;
	DxcCreateInstanceProc m_createFn = nullptr;
	DxcCreateInstance2Proc m_createFn2 = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// jShaderCompiler_DX12
class jShaderCompiler_DX12
{
public:
	static jShaderCompiler_DX12& Get()
	{
		if (!_instance)
		{
			_instance = new jShaderCompiler_DX12();
			_instance->Initialize();
		}
		return *_instance;
	}
	static void Destroy()
	{
		delete _instance;
	}

	HRESULT Initialize();
	ComPtr<IDxcBlob> CompileFromFile(const wchar_t* InFilename, const wchar_t* InShadingModel
		, const wchar_t* InEntryPoint = nullptr, bool InRowMajorMatrix = false) const;
	ComPtr<IDxcBlob> Compile(const char* InShaderCode, uint32 InShaderCodeLength, const wchar_t* InShadingModel
		, const wchar_t* InEntryPoint = nullptr, bool InRowMajorMatrix = false, std::vector<const wchar_t*> InCompileOptions = {}) const;

public:
	jShaderCompiler_DX12(jShaderCompiler_DX12 const&) = delete;
	jShaderCompiler_DX12& operator=(jShaderCompiler_DX12 const&) = delete;
	jShaderCompiler_DX12(jShaderCompiler_DX12&&) = delete;
	jShaderCompiler_DX12& operator=(jShaderCompiler_DX12&&) = delete;

private:
	jDXC m_dxc;

private:
	jShaderCompiler_DX12() {}
	static jShaderCompiler_DX12* _instance;
};
