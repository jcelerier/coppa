#pragma once

#include <coppa/coppa.hpp>
namespace coppa
{
    namespace minuit
    {
        struct RepetitionFilter
        {
                coppa_name(RepetitionFilter)
                bool repetitionFilter;
        };

        template<typename ValueType>
        struct Interval
        {
                enum class Type { OpenOpen, OpenClosed, ClosedOpen, ClosedClosed } type;
                std::pair<ValueType, ValueType> range;
        };

        template<typename ValueType> using Enum = std::vector<ValueType>;
        enum class BoundingMode { Free, Clip, Wrap, Fold };

        template<typename ValueType>
        using DomainType = std::vector<eggs::variant<Interval<ValueType>, Enum<ValueType>>>;

        template<typename DomainValueType,
                 typename InfComparator,
                 typename EqComparator>
        class Bounds
        {
            public:
                coppa_name(Bounds)

                DomainType<DomainValueType> domain;
                BoundingMode lower_bound = BoundingMode::Free;
                BoundingMode upper_bound = BoundingMode::Free;

                template<typename ValueType>
                bool valueIsInBounds(ValueType val)
                {
                    using namespace eggs::variants;
                    for(auto&& subdomain : domain)
                    {
                        if(const auto& interval = get<Interval<DomainValueType>>(subdomain))
                        {
                            if(InfComparator::operatorInf(interval.range.first, val))
                            {
                            }
                        }
                        else
                        {
                            const auto& enumeration = get<Enum<DomainValueType>>(subdomain);
                            for(auto enum_val : enumeration)
                            {
                                if(EqComparator::operatorEq(val, enum_val))
                                {
                                }
                            }
                        }
                    }
                }
        };

        template<class ValueType>
        class StandardComparator
        {
                static bool operatorInf(const ValueType& lhs, const ValueType& rhs)
                { return lhs < rhs; }
                static bool operatorEq(const ValueType& lhs, const ValueType& rhs)
                { return lhs == rhs; }
        };


        using Parameter = ParameterAdapter<SimpleValue<Variant>,
        Tags,
        Alias,
        RepetitionFilter,
        Bounds<Variant,
        StandardComparator<Variant>,
        StandardComparator<Variant>>>;
    }
}
