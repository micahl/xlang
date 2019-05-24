#pragma once

#include "model_types.h"
#include "xmeta_models.h"

namespace xlang::xmeta
{
    void type_ref::set_semantic(type_semantics const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(class_type_semantics const& sem) noexcept
    {
        if (std::holds_alternative<std::shared_ptr<delegate_model>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<delegate_model>>(sem));
        }
        if (std::holds_alternative<std::shared_ptr<class_model>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<class_model>>(sem));
        }
        if (std::holds_alternative<std::shared_ptr<interface_model>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<interface_model>>(sem));
        }
        if (std::holds_alternative<std::shared_ptr<enum_model>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<enum_model>>(sem));
        }
        if (std::holds_alternative<std::shared_ptr<struct_model>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<struct_model>>(sem));
        }
        if (std::holds_alternative<std::shared_ptr<xlang::meta::reader::TypeDef>>(sem))
        {
            m_semantic.resolve(std::get<std::shared_ptr<xlang::meta::reader::TypeDef>>(sem));
        }
    }

    void type_ref::set_semantic(std::shared_ptr<class_model> const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(std::shared_ptr<enum_model> const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(std::shared_ptr<interface_model> const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(std::shared_ptr<struct_model> const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(std::shared_ptr<delegate_model> const& sem) noexcept
    {
        m_semantic.resolve(sem);
    }

    void type_ref::set_semantic(simple_type st)
    {
        m_semantic.resolve(st);
    }

    void type_ref::set_semantic(object_type o)
    {
        m_semantic.resolve(o);
    }

    // To be use for testing purposes only
    bool type_ref::operator==(type_ref const& right_ref) const
    {
        if (!m_semantic.is_resolved() && right_ref.get_semantic().is_resolved())
        {
            std::string left_type = get_semantic().get_ref_name();
            type_semantics right_type = right_ref.get_semantic().get_resolved_target();

            if (std::holds_alternative<std::shared_ptr<class_model>>(right_type))
            {
                return left_type == std::get<std::shared_ptr<class_model>>(right_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<interface_model>>(right_type))
            {
                return left_type == std::get<std::shared_ptr<interface_model>>(right_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<enum_model>>(right_type))
            {
                return left_type == std::get<std::shared_ptr<enum_model>>(right_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<struct_model>>(right_type))
            {
                return left_type == std::get<std::shared_ptr<struct_model>>(right_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<delegate_model>>(right_type))
            {
                return left_type == std::get<std::shared_ptr<delegate_model>>(right_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<xlang::meta::reader::TypeDef>>(right_type))
            {
                // TODO: maybe also check the type?
                return left_type == std::get<std::shared_ptr<xlang::meta::reader::TypeDef>>(right_type)->FullyQualifiedName();
            }
        }
        if (m_semantic.is_resolved() && !right_ref.get_semantic().is_resolved())
        {
            std::string right_type = right_ref.get_semantic().get_ref_name();
            type_semantics left_type = m_semantic.get_resolved_target();

            if (std::holds_alternative<std::shared_ptr<class_model>>(left_type))
            {
                return right_type == std::get<std::shared_ptr<class_model>>(left_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<interface_model>>(left_type))
            {
                return right_type == std::get<std::shared_ptr<interface_model>>(left_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<enum_model>>(left_type))
            {
                return right_type == std::get<std::shared_ptr<enum_model>>(left_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<struct_model>>(left_type))
            {
                return right_type == std::get<std::shared_ptr<struct_model>>(left_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<delegate_model>>(left_type))
            {
                return right_type == std::get<std::shared_ptr<delegate_model>>(left_type)->get_fully_qualified_id();
            }
            if (std::holds_alternative<std::shared_ptr<xlang::meta::reader::TypeDef>>(left_type))
            {
                // TODO: maybe also check the type?
                return right_type == std::get<std::shared_ptr<xlang::meta::reader::TypeDef>>(left_type)->FullyQualifiedName();
            }
        }
        if (m_semantic.is_resolved() && right_ref.get_semantic().is_resolved())
        {
            type_semantics left_type = m_semantic.get_resolved_target();
            type_semantics right_type = right_ref.get_semantic().get_resolved_target();
            if (std::holds_alternative<simple_type>(left_type) && std::holds_alternative<simple_type>(right_type))
            {
                return std::get<simple_type>(left_type) == std::get<simple_type>(right_type);
            }
            if (std::holds_alternative<object_type>(left_type) && std::holds_alternative<object_type>(right_type))
            {
                return true;
            }
            if (std::holds_alternative<std::shared_ptr<class_model>>(left_type) && std::holds_alternative<std::shared_ptr<class_model>>(right_type))
            {
                return std::get<std::shared_ptr<class_model>>(left_type) == std::get<std::shared_ptr<class_model>>(right_type);
            }
            if (std::holds_alternative<std::shared_ptr<interface_model>>(left_type) && std::holds_alternative<std::shared_ptr<interface_model>>(right_type))
            {
                return std::get<std::shared_ptr<interface_model>>(left_type) == std::get<std::shared_ptr<interface_model>>(right_type);
            }
            if (std::holds_alternative<std::shared_ptr<enum_model>>(left_type) && std::holds_alternative<std::shared_ptr<enum_model>>(right_type))
            {
                return std::get<std::shared_ptr<enum_model>>(left_type) == std::get<std::shared_ptr<enum_model>>(right_type);
            }
            if (std::holds_alternative<std::shared_ptr<struct_model>>(left_type) && std::holds_alternative<std::shared_ptr<struct_model>>(right_type))
            {
                return std::get<std::shared_ptr<struct_model>>(left_type) == std::get<std::shared_ptr<struct_model>>(right_type);
            }
            if (std::holds_alternative<std::shared_ptr<delegate_model>>(left_type) && std::holds_alternative<std::shared_ptr<delegate_model>>(right_type))
            {
                return std::get<std::shared_ptr<delegate_model>>(left_type) == std::get<std::shared_ptr<delegate_model>>(right_type);
            }
            if (std::holds_alternative<std::shared_ptr<xlang::meta::reader::TypeDef>>(left_type) && std::holds_alternative<std::shared_ptr<xlang::meta::reader::TypeDef>>(right_type))
            {
                // TODO: maybe also check the type?
                return std::get<std::shared_ptr<xlang::meta::reader::TypeDef>>(left_type) == std::get<std::shared_ptr<xlang::meta::reader::TypeDef>>(right_type);
            }
        }
        return false;
    }
}