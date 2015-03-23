#pragma once
#include <coppa/coppa.hpp>

namespace coppa
{
    namespace oscquery
    {
        struct Range
        {
                boost::optional<Variant> min;
                boost::optional<Variant> max;
                std::vector<Variant> values;
        };

        struct Value
        {
                Variant value;
                Range range;
                ClipMode clipMode{ClipMode::None};
        };

        struct Values
        {
                coppa_name(Values)
                std::vector<oscquery::Value> values;
        };

        using Parameter = ParameterAdapter<
                            Values,
                            Description,
                            Tags>;

        using ParameterMap = ParameterMapType<Parameter>;
    }
}
