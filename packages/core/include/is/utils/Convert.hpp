/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 * Copyright (C) 2020 - present Proyectos y Sistemas de Mantenimiento SL (eProsima).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _IS_UTILS_CONVERT_HPP_
#define _IS_UTILS_CONVERT_HPP_

#include <is/Message.hpp>

#include <algorithm>
#include <mutex>
#include <type_traits>
#include <vector>

namespace eprosima {
namespace is {
namespace utils {

// TODO(@jamoralp) doxygen this

//==============================================================================
/// \brief A utility to help with converting data between generic DynamicData
/// field objects and middleware-specific data structures.
///
/// This struct will work as-is on primitive types (a.k.a. arithmetic types or
/// strings), but you should create a template specialization for converting to
/// or from any complex class types.
template<typename Type>
struct Convert
{
    using native_type = Type;

    static constexpr bool type_is_primitive =
            std::is_arithmetic<Type>::value
            || std::is_same<std::string, Type>::value
            || std::is_same<std::basic_string<char16_t>, Type>::value;

    /// \brief Move data from a xtype field to a native middleware
    /// data structure
    static void from_xtype_field(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            native_type& to)
    {
        to = from.value<native_type>();
    }

    /// \brief Move data from a native middleware data structure to a xtype
    /// message field.
    static void to_xtype_field(
            const native_type& from,
            eprosima::xtypes::WritableDynamicDataRef to)
    {
        static_assert(type_is_primitive,
                "The is::utils::Convert struct should be specialized for non-primitive types");
        to.value<native_type>(from);
    }

};

//==============================================================================
/// \brief A class that helps create a Convert<> specialization for managing some
/// char issues.
///
/// 'rosidl' parse 'char' types as 'signed' values from 'msg' files and
/// parse as 'unsigned' from idl files. This create a mismatched between types.
/// This patch solves this issue when the native type differs from the
/// DynamicData type for this specific case.
// NOTE: This specialization can be removed safety if rosidl modifies its behaviour.
struct CharConvert
{
    using native_type = char;

    static constexpr bool type_is_primitive = true;

    // Documentation inherited from Convert
    static void from_xtype_field(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            native_type& to)
    {
        if (from.type().kind() == eprosima::xtypes::TypeKind::UINT_8_TYPE)
        {
            to = static_cast<native_type>(from.value<uint8_t>());
        }
        else
        {
            to = from.value<native_type>();
        }
    }

    // Documentation inherited from Convert
    static void to_xtype_field(
            const native_type& from,
            eprosima::xtypes::WritableDynamicDataRef to)
    {
        if (to.type().kind() == eprosima::xtypes::TypeKind::UINT_8_TYPE)
        {
            to.value<uint8_t>(static_cast<uint8_t>(from));
        }
        else
        {
            to.value<native_type>(from);
        }
    }

};

template<>
struct Convert<char> : CharConvert { };

//==============================================================================
/// \brief A class that helps create a Convert<> specialization for compound
/// message types.
///
/// To create a specialization for a native middleware message type, do the
/// following:
///
/// \code
/// namespace eprosima {
/// namespace is {
/// namespace utils {
///
/// template<>
/// struct Convert<native::middleware::type>
///   : eprosima::is::utils::MessageConvert<
///        native::middleware::type,
///       &native::middleware::convert_from_xtype_fnc,
///       &native::middleware::convert_to_xtype_fnc
///   > { };
///
/// } // namespace utils
/// } // namespace is
/// } // namespace eprosima
/// \endcode
///
/// Make sure that this template specialization is put into the eprosima::is::utils
/// namespace.
template<
    typename Type,
    void (* _from_xtype)(const eprosima::xtypes::ReadableDynamicDataRef& from, Type& to),
    void (* _to_xtype)(const Type& from, eprosima::xtypes::WritableDynamicDataRef to)>
struct MessageConvert
{
    using native_type = Type;

    static constexpr bool type_is_primitive = false;

    // Documentation inherited from Convert
    static void from_xtype_field(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            native_type& to)
    {
        (*_from_xtype)(from, to);
    }

    // Documentation inherited from Convert
    static void to_xtype_field(
            const native_type& from,
            eprosima::xtypes::WritableDynamicDataRef to)
    {
        (*_to_xtype)(from, to);
    }

};

//==============================================================================
/// \brief Convenience function for converting a bounded eprosima::xtypes::CollectionType
/// into a bounded vector of middleware-specific messages.
///
/// To make the limit unbounded, set the UpperBound argument to
/// std::numeric_limits<VectorType::size_type>::max()
template<
    typename ElementType,
    typename NativeType,
    std::size_t UpperBound>
struct ContainerConvert
{
    using native_type = NativeType;

    static constexpr bool type_is_primitive =
            Convert<ElementType>::type_is_primitive;

    static void from_xtype(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            ElementType& to)
    {
        Convert<ElementType>::from_xtype_field(from, to);
    }

    /// \brief This template specialization is needed to deal with the edge case
    /// produced by vectors of bools. std::vector<bool> is specialized to be
    /// implemented as a bitmap, and as a result its operator[] cannot return its
    /// bool elements by reference. Instead it returns a "reference" proxy object.
    static void from_xtype(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            std::vector<bool>::reference to)
    {
        bool temp = from;
        to = temp;
    }

    template<typename Container>
    static void container_resize(
            Container& vector,
            std::size_t size)
    {
        vector.resize(size);
    }

    template<template <typename, std::size_t> class Array, typename T, std::size_t N>
    static void container_resize(
            Array<T, N>& /*array*/,
            std::size_t /*size*/)
    {
        // Do nothing. Arrays don't need to be resized.
    }

    // Documentation inherited from Convert
    static void from_xtype_field(
            const eprosima::xtypes::ReadableDynamicDataRef& from,
            native_type& to)
    {
        const std::size_t N = std::min(from.size(), UpperBound);
        container_resize(to, N);
        for (std::size_t i = 0; i < N; ++i)
        {
            from_xtype(from[i], to[i]);
        }
    }

    // Documentation inherited from Convert
    static void to_xtype_field(
            const native_type& from,
            eprosima::xtypes::WritableDynamicDataRef to)
    {
        const std::size_t N = std::min(from.size(), UpperBound);
        to.resize(N);
        for (std::size_t i = 0; i < N; ++i)
        {
            Convert<ElementType>::to_xtype_field(from[i], to[i]);
        }
    }

};

//==============================================================================
template<typename ElementType, typename Allocator>
struct Convert<std::vector<ElementType, Allocator> >
    : ContainerConvert<
        ElementType,
        std::vector<typename Convert<ElementType>::native_type, Allocator>,
        std::numeric_limits<typename std::vector<ElementType, Allocator>::size_type>::max()> { };

//==============================================================================
template<template <typename, std::size_t> class Array, typename ElementType, std::size_t N>
struct Convert<Array<ElementType, N> >
    : ContainerConvert<
        ElementType,
        Array<typename Convert<ElementType>::native_type, N>,
        N> { };

//==============================================================================
template<typename ElementType, std::size_t N, typename Allocator,
        template<typename, std::size_t, typename> class VectorImpl>
struct Convert<VectorImpl<ElementType, N, Allocator> >
    : ContainerConvert<
        ElementType,
        VectorImpl<typename Convert<ElementType>::native_type, N, Allocator>,
        N> { };

//==============================================================================
/// \brief A thread-safe repository for resources to avoid unnecessary
/// allocations
template<typename Resource, Resource(* initializerT)()>
class ResourcePool
{
public:

    ResourcePool(
            const std::size_t initial_depth = 1)
    {
        _queue.reserve(initial_depth);
        for (std::size_t i = 0; i < initial_depth; ++i)
        {
            _queue.emplace_back((_initializer)());
        }
    }

    void setInitializer(
            std::function<Resource()> initializer)
    {
        _initializer = std::move(initializer);
    }

    Resource pop()
    {
        if (_queue.empty())
        {
            return (_initializer)();
        }

        std::unique_lock<std::mutex> lock(_mutex);
        Resource r = std::move(_queue.back());
        _queue.pop_back();
        return r;
    }

    void recycle(
            Resource&& r)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.emplace_back(std::move(r));
    }

private:

    std::vector<Resource> _queue;
    std::mutex _mutex;
    std::function<Resource()> _initializer = initializerT;

};

//==============================================================================
template<typename Resource>
using UniqueResourcePool =
        ResourcePool<std::unique_ptr<Resource>, &std::make_unique<Resource> >;

template<typename Resource>
std::unique_ptr<Resource> initialize_unique_null()
{
    return nullptr;
}

template<typename Resource>
using SharedResourcePool =
        ResourcePool<std::shared_ptr<Resource>, &std::make_shared<Resource> >;

template<typename Resource>
std::shared_ptr<Resource> initialize_shared_null()
{
    return nullptr;
}

} //  namespace utils
} //  namespace is
} //  namespace eprosima

#endif //  _IS_UTILS_CONVERT_HPP_