/*
 * Copyright (C) 2019 Open Source Robotics Foundation
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

#include <is/core/runtime/StringTemplate.hpp>

#include <vector>

namespace eprosima {
namespace is {
namespace core {

class StringTemplate::Implementation
{
public:

    Implementation(
            const std::string& template_string,
            const std::string& usage_details)
        : _converter(usage_details)
    {
        std::size_t last_end = 0;
        std::size_t start = template_string.find('{', last_end);

        while (start < template_string.size())
        {
            _components.emplace_back(template_string.substr(last_end, start - last_end));

            last_end = template_string.find('}', start) + 1;
            if (last_end == std::string::npos)
            {
                throw InvalidTemplateFormat(template_string, usage_details);
            }

            const std::string substitution_string =
                    template_string.substr(start + 1, last_end - start - 2);
            if (substitution_string.substr(0, 8) != "message.")
            {
                throw InvalidTemplateFormat(template_string, usage_details);
            }
            _substitutions[_components.size()] = substitution_string.substr(8);

            // We use an empty string to represent components that will get substituted later.
            _components.emplace_back("");

            start = template_string.find('{', last_end);
        }

        if (last_end < template_string.size())
        {
            _components.emplace_back(template_string.substr(last_end));
        }
    }

    Implementation(
            const Implementation& /*other*/) = default;

    Implementation(
            Implementation&& /*other*/) = default;

    ~Implementation() = default;

    const std::string compute_string(
            const eprosima::xtypes::DynamicData& message) const
    {
        std::string result;
        if (!_components.empty())
        {
            result = _components[0];
        }

        SubstitutionMap::const_iterator substitute_it = _substitutions.begin();
        for (std::size_t i = 1; i < _components.size(); ++i)
        {
            if (substitute_it != _substitutions.end() && substitute_it->first == i)
            {
                const std::string& field_name = substitute_it->second;
                const xtypes::AggregationType& type =
                        static_cast<const xtypes::AggregationType&>(message.type());

                if (!type.has_member(field_name))
                {
                    throw UnavailableMessageField(field_name, _converter.details());
                }

                xtypes::ReadableDynamicDataRef data = message[field_name];
                result += _converter.to_string(data, field_name);
                ++substitute_it;
                continue;
            }

            result += _components[i];
        }

        return result;
    }

    const FieldToString& converter() const
    {
        return _converter;
    }

    FieldToString& converter()
    {
        return _converter;
    }

private:

    /**
     * Class members.
     */

    FieldToString _converter;

    /**
     * This contains a vector of string literal components.
     */
    std::vector<std::string> _components;

    /**
     * This uses an ordered map so that we can iterate
     * through it linearly, as we perform substitutions.
     */
    using SubstitutionMap = std::map<std::size_t, std::string>;

    /**
     * Right now this simply maps a component index to a xtypes::DynamicData field name.
     * In the future, we can replace std::string with an abstract Substitution
     * class that gets factory generated based on what type of substitution is
     * requested. For right now we've only implemented "message.field" substitutions.
     */
    SubstitutionMap _substitutions;
};

//==============================================================================
StringTemplate::StringTemplate(
        const std::string& template_string,
        const std::string& usage_details)
    : _pimpl(new Implementation(template_string, usage_details))
{
}

//==============================================================================
StringTemplate::StringTemplate(
        const StringTemplate& other)
    : _pimpl(new Implementation(*other._pimpl))
{
}

//==============================================================================
StringTemplate::StringTemplate(
        StringTemplate&& other)
    : _pimpl(new Implementation(std::move(*other._pimpl)))
{
}

//==============================================================================
StringTemplate::~StringTemplate()
{
    _pimpl.reset();
}

//==============================================================================
const std::string StringTemplate::compute_string(
        const eprosima::xtypes::DynamicData& message) const
{
    return _pimpl->compute_string(message);
}

//==============================================================================
std::string& StringTemplate::usage_details()
{
    return _pimpl->converter().details();
}

//==============================================================================
const std::string& StringTemplate::usage_details() const
{
    return _pimpl->converter().details();
}

//==============================================================================
InvalidTemplateFormat::InvalidTemplateFormat(
        const std::string& template_string,
        const std::string& details)
    : std::runtime_error(
        std::string()
        + "ERROR : Template string '" + template_string + "' was incorrectly "
        + "formatted. Details: " + details)
    , _template_string(template_string)
{
}

//==============================================================================
const std::string& InvalidTemplateFormat::template_string() const
{
    return _template_string;
}

//==============================================================================
UnavailableMessageField::UnavailableMessageField(
        const std::string& field_name,
        const std::string& details)
    : std::runtime_error(
        std::string()
        + "ERROR : Unable to find a required field '" + field_name
        + "'. Details: " + details)
    , _field_name(field_name)
{
}

//==============================================================================
const std::string& UnavailableMessageField::field_name() const
{
    return _field_name;
}

} //  namespace core
} //  namespace is
} //  namespace eprosima