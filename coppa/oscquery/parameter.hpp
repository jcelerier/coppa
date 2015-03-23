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

        struct Values
        {
                coppa_name(Values)
                std::vector<Variant> values;
        };

        struct Ranges
        {
                coppa_name(Ranges)
                std::vector<oscquery::Range> ranges;
        };

        struct ClipModes
        {
                coppa_name(ClipModes)
                std::vector<ClipMode> clipmodes;
        };

        using Parameter = ParameterAdapter<
                            Values,
                            Ranges,
                            Access,
                            ClipModes,
                            Description,
                            Tags>;

        inline void addValue(Parameter& parameter,
                      const Variant& var,
                      const oscquery::Range& range = {{}, {}, {}},
                      const ClipMode& clipmode = ClipMode::None)
        {
            parameter.values.push_back(var);
            parameter.ranges.push_back(range);
            parameter.clipmodes.push_back(clipmode);
        }

        using ParameterMap = ParameterMapType<Parameter>;
    }
}
