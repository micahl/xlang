#include "pch.h"

namespace winrt::impl
{
    template <typename T>
    struct boxed_weak_ref : inspectable_abi, IWeakReference
    {
        explicit boxed_weak_ref(com_ref<T> const& value)
        {
            check_hresult(value.try_as<IWeakReferenceSource>()->GetWeakReference(m_ref.put()));
        }

        int32_t __stdcall Resolve(guid const& id, void** value) noexcept final
        {
            return m_ref->Resolve(id, value);
        }

        int32_t __stdcall QueryInterface(guid const& id, void** result) noexcept final
        {
            if (is_guid_of<IWeakReference>(id))
            {
                *result = static_cast<IWeakReference*>(this);
                AddRef();
                return 0;
            }

            if (is_guid_of<Windows::Foundation::IInspectable>(id) || is_guid_of<Windows::Foundation::IUnknown>(id))
            {
                *result = static_cast<inspectable_abi*>(this);
                AddRef();
                return 0;
            }

            *result = nullptr;
            return error_no_interface;
        }

        uint32_t __stdcall AddRef() noexcept final
        {
            return 1 + m_references.fetch_add(1, std::memory_order_relaxed);
        }

        uint32_t __stdcall Release() noexcept final
        {
            uint32_t const target = m_references.fetch_sub(1, std::memory_order_release) - 1;

            if (target == 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete this;
            }

            return target;
        }

        int32_t __stdcall GetIids(uint32_t* count, guid** array) noexcept final
        {
            *count = 0;
            *array = nullptr;
            return 0;
        }

        int32_t __stdcall GetRuntimeClassName(void** name) noexcept final
        {
            *name = nullptr;
            return 0;
        }

        int32_t __stdcall GetTrustLevel(Windows::Foundation::TrustLevel* level) noexcept final
        {
            *level = Windows::Foundation::TrustLevel::BaseTrust;
            return 0;
        }

    private:
        std::atomic<uint32_t> m_references{ 1 };
        com_ptr<IWeakReference> m_ref;
    };
}

namespace winrt
{
    template <typename T>
    Windows::Foundation::IInspectable box_weak_ref(T const& object)
    {
        return { new impl::boxed_weak_ref<impl::wrapped_type_t<T>>(object), take_ownership_from_abi };
    }

    template <typename T>
    impl::com_ref<T> unbox_weak_ref(Windows::Foundation::IInspectable const& object) noexcept
    {
        if (!object)
        {
            return nullptr;
        }

        void* result{};
        object.try_as<impl::IWeakReference>()->Resolve(guid_of<T>(), &result);
        return { result, take_ownership_from_abi };
    }
}

using namespace winrt;
using namespace winrt::Windows::Foundation;

struct not_winrt : implements<not_winrt, IPersist>
{
    HRESULT __stdcall GetClassID(GUID* id) noexcept final
    {
        *id = {};
        return S_OK;
    }
};

int main()
{
    {
        Uri original(L"http://kennykerr.ca/");

        auto boxed = box_weak_ref(original);

        {
            Uri copy = unbox_weak_ref<Uri>(boxed);

            WINRT_ASSERT(copy.Domain() == original.Domain());
        }

        original = nullptr;

        {
            Uri copy = unbox_weak_ref<Uri>(boxed);

            WINRT_ASSERT(copy == nullptr);
        }
    }
    {
        com_ptr<IPersist> original = make<not_winrt>();

        auto boxed = box_weak_ref(original);

        {
            com_ptr<IPersist> copy = unbox_weak_ref<IPersist>(boxed);

            WINRT_ASSERT(copy == original);
        }

        original = nullptr;

        {
            com_ptr<IPersist> copy = unbox_weak_ref<IPersist>(boxed);

            WINRT_ASSERT(copy == nullptr);
        }
    }
}
