#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 

#include <Windows.h>
#include <roapi.h>
#include <winstring.h>
#include <rometadataresolution.h>
#include <combaseapi.h>
#include <wrl.h>
#include <xmllite.h>
#include <Shlwapi.h>
#include <comutil.h>
#include <fstream>
#include <unordered_map>
#include <codecvt>
#include <locale>
#include <RoMetadataApi.h>
#include "catalog.h"
#include "TypeResolution.h"

using namespace std;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

// TODO: Components won't respect COM lifetime. workaround to get them in the COM list?

extern "C"
{
    typedef HRESULT(__stdcall* activation_factory_type)(HSTRING, IActivationFactory**);
}

// Intentionally no class factory cache here. That would be excessive since 
// other layers already cache.
struct component
{
    wstring module_name;
    wstring xmlns;
    HMODULE handle = nullptr;
    activation_factory_type get_activation_factory;
    ABI::Windows::Foundation::ThreadingType threading_model;

    ~component()
    {
        if (handle)
        {
            FreeLibrary(handle);
        }
    }

    HRESULT LoadModule()
    {
        if (handle == nullptr)
        {
            handle = LoadLibraryExW(module_name.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (handle == nullptr)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
            this->get_activation_factory = (activation_factory_type)GetProcAddress(handle, "DllGetActivationFactory");
            if (this->get_activation_factory == nullptr)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }
        return (handle != nullptr && this->get_activation_factory != nullptr) ? S_OK : E_FAIL;
    }

    HRESULT GetActivationFactory(HSTRING className, REFIID  iid, void** factory)
    {
        RETURN_IF_FAILED(LoadModule());

        IActivationFactory* ifactory = nullptr;
        HRESULT hr = this->get_activation_factory(className, &ifactory);
        // optimize for IActivationFactory?
        if (SUCCEEDED(hr))
        {
            hr = ifactory->QueryInterface(iid, factory);
            ifactory->Release();
        }
        return hr;
    }
};

static unordered_map<wstring, shared_ptr<component>> g_types;

HRESULT WinRTLoadComponent(PCWSTR manifest_path)
{
    ComPtr<IStream> fileStream;
    ComPtr<IXmlReader> xmlReader;
    XmlNodeType nodeType;
    LPCWSTR localName = nullptr;
    LPCWSTR fileName = nullptr;
    auto locale = _create_locale(LC_ALL, "C");

    RETURN_IF_FAILED(SHCreateStreamOnFileEx(manifest_path, STGM_READ, FILE_ATTRIBUTE_NORMAL, false, nullptr, &fileStream));
    RETURN_IF_FAILED(CreateXmlReader(__uuidof(IXmlReader), (void**)&xmlReader, nullptr));
    RETURN_IF_FAILED(xmlReader->SetInput(fileStream.Get()));
    while (S_OK == xmlReader->Read(&nodeType))
    {
        if (nodeType == XmlNodeType_Element)
        {
            RETURN_IF_FAILED((xmlReader->GetLocalName(&localName, nullptr)));

            if (_wcsicmp_l(localName, L"file", locale) == 0)
            {
                RETURN_IF_FAILED(xmlReader->MoveToAttributeByName(L"name", nullptr));
                RETURN_IF_FAILED(xmlReader->GetValue(&fileName, nullptr));
            }
            if (_wcsicmp_l(localName, L"activatableClass", locale) == 0)
            {
                auto this_component = make_shared<component>();
                this_component->module_name = fileName;

                const WCHAR* threadingModel;
                RETURN_IF_FAILED(xmlReader->MoveToAttributeByName(L"threadingModel", nullptr));
                RETURN_IF_FAILED(xmlReader->GetValue(&threadingModel, nullptr));

                if (_wcsicmp_l(L"sta", threadingModel, locale) == 0)
                {
                    this_component->threading_model = ABI::Windows::Foundation::ThreadingType::ThreadingType_STA;
                }
                else if (_wcsicmp_l(L"mta", threadingModel, locale) == 0)
                {
                    this_component->threading_model = ABI::Windows::Foundation::ThreadingType::ThreadingType_MTA;
                }
                else if (_wcsicmp_l(L"both", threadingModel, locale) == 0)
                {
                    this_component->threading_model = ABI::Windows::Foundation::ThreadingType::ThreadingType_BOTH;
                }
                else
                {
                    return HRESULT_FROM_WIN32(ERROR_SXS_MANIFEST_PARSE_ERROR);
                }

                const WCHAR* xmlns;
                RETURN_IF_FAILED(xmlReader->MoveToAttributeByName(L"xmlns", nullptr));
                RETURN_IF_FAILED(xmlReader->GetValue(&xmlns, nullptr));
                this_component->xmlns = xmlns;

                const WCHAR* activatableClass;
                RETURN_IF_FAILED(xmlReader->MoveToAttributeByName(L"name", nullptr));
                RETURN_IF_FAILED(xmlReader->GetValue(&activatableClass, nullptr));
                g_types[activatableClass] = this_component;
            }
        }
    }

    return S_OK;
}

HRESULT WinRTGetThreadingModel(HSTRING activatableClassId, ABI::Windows::Foundation::ThreadingType* threading_model)
{
    auto raw_class_name = WindowsGetStringRawBuffer(activatableClassId, nullptr);
    auto component_iter = g_types.find(raw_class_name);
    if (component_iter != g_types.end())
    {
        *threading_model = component_iter->second->threading_model;
        return S_OK;
    }
    return REGDB_E_CLASSNOTREG;
}

HRESULT WinRTGetActivationFactory(
    HSTRING activatableClassId,
    REFIID  iid,
    void** factory)
{
    auto raw_class_name = WindowsGetStringRawBuffer(activatableClassId, nullptr);
    auto component_iter = g_types.find(raw_class_name);
    if (component_iter != g_types.end())
    {
        return component_iter->second->GetActivationFactory(activatableClassId, iid, factory);
    }
    return REGDB_E_CLASSNOTREG;    
}

HRESULT WinRTGetMetadataFile(
    const HSTRING        name,
    IMetaDataDispenserEx* metaDataDispenser,
    HSTRING* metaDataFilePath,
    IMetaDataImport2** metaDataImport,
    mdTypeDef* typeDefToken)
{
    wchar_t folderPrefix[9];
    PCWSTR pszFullName = WindowsGetStringRawBuffer(name, nullptr);
    HRESULT hr = StringCchCopyW(folderPrefix, _countof(folderPrefix), pszFullName);
    if (hr != S_OK && hr != STRSAFE_E_INSUFFICIENT_BUFFER)
    {
        return hr;
    }
    if (CompareStringOrdinal(folderPrefix, -1, L"Windows.", -1, false) == CSTR_EQUAL)
    {
        return RO_E_METADATA_NAME_NOT_FOUND;
    }

    if (metaDataFilePath != nullptr)
    {
        *metaDataFilePath = nullptr;
    }
    if (metaDataImport != nullptr)
    {
        *metaDataImport = nullptr;
    }
    if (typeDefToken != nullptr)
    {
        *typeDefToken = mdTypeDefNil;
    }

    if (((metaDataImport == nullptr) && (typeDefToken != nullptr)) ||
        ((metaDataImport != nullptr) && (typeDefToken == nullptr)))
    {
        return E_INVALIDARG;
    }

    ComPtr<IMetaDataDispenserEx> spMetaDataDispenser;
    // The API uses the caller's passed-in metadata dispenser. If null, it
    // will create an instance of the metadata reader to dispense metadata files.
    if (metaDataDispenser == nullptr)
    {
        RETURN_IF_FAILED(CoCreateInstance(CLSID_CorMetaDataDispenser,
            nullptr,
            CLSCTX_INPROC,
            IID_IMetaDataDispenser,
            &spMetaDataDispenser));
        {
            variant_t version{ L"WindowsRuntime 1.4" };
            RETURN_IF_FAILED(spMetaDataDispenser->SetOption(MetaDataRuntimeVersion, &version.GetVARIANT()));
        }
    }
    return UndockedRegFreeWinRT::ResolveThirdPartyType(
        spMetaDataDispenser.Get(),
        pszFullName,
        metaDataFilePath,
        metaDataImport,
        typeDefToken);
}