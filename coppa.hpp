#pragma once
#include <string>
#include <vector>
#include <utility>
#include <eggs/variant.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/tokenizer.hpp>
#include <jeayeson/jeayeson.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#define coppa_name(theName) public: static constexpr const char * name{ #theName }; private:

namespace coppa
{

template<typename ValueType>
class Value
{
	coppa_name(Value);

	public:
		ValueType value;
};

class Tags
{
	coppa_name(Tags);

	public:
		using Tag = std::string;
		std::vector<Tag> tags;
};

class Alias
{
	coppa_name(Alias);

	public:
		std::string alias;
};

class Description
{
	coppa_name(Description);

	public:
		std::string description;
};


class RepetitionFilter
{
	coppa_name(RepetitionFilter);

	public:
		bool repetitionFilter;
};


template<typename ValueType>
struct Interval
{
	enum class Type { OpenOpen, OpenClosed, ClosedOpen, ClosedClosed } type;
	std::pair<ValueType, ValueType> range;
};

template<typename ValueType>
using Enum = std::vector<ValueType>;
enum class BoundingMode
{
	Free, Clip, Wrap, Fold
};

template<typename ValueType>
using DomainType = std::vector<eggs::variant<Interval<ValueType>, Enum<ValueType>>>;

template<typename DomainValueType,
		 typename InfComparator,
		 typename EqComparator>
class Bounds
{
	coppa_name(Bounds);

	public:
		DomainType<DomainValueType> domain;
		BoundingMode lower_bound = BoundingMode::Free;
		BoundingMode upper_bound = BoundingMode::Free;

		template<typename ValueType>
		bool valueIsInBounds(ValueType val)
		{
			for(auto&& subdomain : domain)
			{
				if(const auto& interval = eggs::variants::get<Interval<DomainValueType>>(subdomain))
				{
					if(InfComparator::operatorInf(interval.range.first, val))
					{
					}
				}
				else
				{
					const auto& enumeration = eggs::variants::get<Enum<DomainValueType>>(subdomain);
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
	{
		return lhs < rhs;
	}
	static bool operatorEq(const ValueType& lhs, const ValueType& rhs)
	{
		return lhs == rhs;
	}
};

enum class AccessMode { None = 0,
						Get = 1,
						Set = 2,
						Both = 3 };
class ParameterBase
{
	public:
		ParameterBase() = default;
		virtual std::vector<std::string> attributes() const noexcept = 0;
		virtual bool hasAttribute(const std::string& attr) const noexcept = 0;


		std::string destination;
		const std::string& getDestination() const
		{ return destination; }


		AccessMode accessMode{AccessMode::None};

		// Need to ask before to know the available attributes
		template<typename Attribute>
		Attribute* get_dyn()
		{
			return dynamic_cast<Attribute*>(this);
		}
};

template<typename... Args>
class Attributes : public Args...
{
	public:
		std::array<const char*, sizeof...(Args)> m_attributes{Args::name...};
};

template<typename... Args>
class ParameterAdapter : public ParameterBase, public Attributes<Args...>
{
	public:
		ParameterAdapter() = default;
		virtual std::vector<std::string> attributes() const noexcept override
		{
			using namespace std;
			vector<string> attr;
			copy(begin(Attributes<Args...>::m_attributes),
				 end(Attributes<Args...>::m_attributes),
				 back_inserter(attr));
			return attr;
		}

		virtual bool hasAttribute(const std::string& attr) const noexcept override
		{
			return find(begin(Attributes<Args...>::m_attributes),
						end(Attributes<Args...>::m_attributes), attr)
					!= end(Attributes<Args...>::m_attributes);
		}

		template<typename Attribute>
		static constexpr bool has() noexcept
		{
			return std::is_base_of<Attribute, ParameterAdapter>::value;
		}

		// No need to ask before
		template<typename Attribute>
		Attribute& get()
		{
			return static_cast<Attribute&>(*this);
		}
};

using Generic = const char*;
using Variant = eggs::variant<int, float, bool, std::string, Generic>;


/// OSCQuery stuff
struct Range
{
	boost::optional<Variant> min;
	boost::optional<Variant> max;
	std::vector<Variant> values;
};
enum class ClipMode
{
	None, Low, High, Both
};
struct OSCQueryValue
{
	Variant value;
	Range range;
	ClipMode clipMode{ClipMode::None};
};
class Values
{
	coppa_name(Values)
	public:
		std::vector<OSCQueryValue> values;
};



using OSCQueryParameter = ParameterAdapter<Values,
										   Description,
										   Tags,
										   Alias,
										   RepetitionFilter,
										   Bounds<Variant,
											  StandardComparator<Variant>,
											  StandardComparator<Variant>>>;



using MinuitParameter = ParameterAdapter<Value<Variant>,
										 Tags,
										 Alias,
										 RepetitionFilter,
										 Bounds<Variant,
											StandardComparator<Variant>,
											StandardComparator<Variant>>>;

namespace bmi = boost::multi_index;
template<typename ParameterType>
using ParameterMapType = bmi::multi_index_container<
    ParameterType,
    bmi::indexed_by<
        bmi::hashed_unique<
            bmi::const_mem_fun<ParameterBase,
							   std::string const&,
							   &ParameterBase::getDestination>
        >
    >
>;

using OSCQueryParameterMap = ParameterMapType<OSCQueryParameter>;

// A ParameterMap can be JSON'd
json_map toJson(const OSCQueryParameterMap& map)
{
	using namespace std;
	using namespace boost;
	using namespace eggs::variants;
	// Root node
	json_map root;
	root["description"] = string("root node");
	root["full_path"] = string("/");
	root["access"] = 0;

	// Create a tree with the parameters
	for(const auto& parameter : map)
	{
		char_separator<char> sep("/");
		tokenizer<char_separator<char>> tokens(parameter.destination, sep);

		// Create the required parts of the tree and navigate to the corresponding node
		auto* current_map = &root;
		for(const auto& token : tokens)
		{
			if(!current_map->has("contents"))
			{ current_map->set("contents", json_map{}); }

			current_map = &current_map->get_for_path<json_map>("contents");

			if(!current_map->has(token))
			{ current_map->set(token, json_map{}); }

			current_map = &current_map->get_for_path<json_map>(token);
		}

		// These attributes are always here
		current_map->set("full_path", parameter.destination);
		current_map->set("access", static_cast<int>(parameter.accessMode));

		// Potentially empty attributes :
		if(!parameter.description.empty())
		{ current_map->set("description", parameter.description); }

		if(!parameter.tags.empty())
		{
			json_array arr;
			for(const auto& tag : parameter.tags)
				arr.add(tag);

			current_map->set("tags", arr);
		}

		// Handling of the types / values
		if(!parameter.values.empty())
		{
			std::string str_type;
			json_array value_arr;
			json_array clip_arr;
			json_array range_arr;
			for(const auto& oscqvalue : parameter.values)
			{
				const auto& value = oscqvalue.value;
				switch(value.which())
				{
					case 0: // int
						str_type += "i";
						value_arr.add(get<int>(value));
						break;
					case 1: // float
						str_type += "f";
						value_arr.add(get<float>(value));
						break;
					case 2: // bool : no bool in OSCQuery ?
						//str_type += "b";
						//value_arr.add(get<bool>(value));
						break;
					case 3: // string
						str_type += "s";
						value_arr.add(get<std::string>(value));
						break;
					case 4: // generic
						str_type += "b";
						value_arr.add(get<const char*>(value));
						break;
				}

				switch(oscqvalue.clipMode)
				{
					case ClipMode::None: clip_arr.add("None"); break;
					case ClipMode::Low:  clip_arr.add("Low");  break;
					case ClipMode::High: clip_arr.add("High"); break;
					case ClipMode::Both: clip_arr.add("Both"); break;
				}

				json_array range_subarray;
				if(!oscqvalue.range.min)
				{
					range_subarray.add("null");
				}
				else
				{
					const auto& minValue = *oscqvalue.range.min;
					switch(minValue.which())
					{
						case 0: range_subarray.add(get<int>(minValue)); break;
						case 1: range_subarray.add(get<float>(minValue)); break;
						case 2: // bool : no bool in OSCQuery ?
							//value_arr.add(get<bool>(minValue));
							break;
						case 3: range_subarray.add(get<string>(minValue)); break;
						case 4: range_subarray.add(get<const char*>(minValue)); break;
					}
				}

				if(!oscqvalue.range.max)
				{
					range_subarray.add("null");
				}
				else
				{
					const auto& maxValue = *oscqvalue.range.max;
					switch(maxValue.which())
					{
						case 0: range_subarray.add(get<int>(maxValue)); break;
						case 1: range_subarray.add(get<float>(maxValue)); break;
						case 2: // bool : no bool in OSCQuery ?
							//value_arr.add(get<bool>(maxValue));
							break;
						case 3: range_subarray.add(get<string>(maxValue)); break;
						case 4: range_subarray.add(get<const char*>(maxValue)); break;
					}
				}

				if(oscqvalue.range.values.empty())
				{
					range_subarray.add("null");
				}
				else
				{
					json_array range_values_array;
					for(auto& elt : oscqvalue.range.values)
					{
						switch(elt.which())
						{
							case 0: range_values_array.add(get<int>(elt)); break;
							case 1: range_values_array.add(get<float>(elt)); break;
							case 2: // bool : no bool in OSCQuery ?
								//value_arr.add(get<bool>(elt));
								break;
							case 3: range_values_array.add(get<string>(elt)); break;
							case 4: range_values_array.add(get<const char*>(elt)); break;
						}
					}
					range_subarray.add(range_values_array);
				}

				range_arr.add(range_subarray);
			}

			current_map->set("type", str_type);
			current_map->set("value", value_arr);
			current_map->set("clip_mode", clip_arr);
			current_map->set("range_mode", range_arr);
		}
	}

	// 2. For each parameter set its attributes
	std::cout << root;
	return root;

}
// A device is an interface (parameters) bound to an object (an OSC / MIDI / Minuit socket)
// The image of a remote device.
// For OSC it only has the local & remote port
template<typename DeviceImpl>
class RemoteDevice
{
	DeviceImpl m_impl;
	//ParameterMap m_map;

	public:
		template<typename... Args>
		RemoteDevice(Args&&... args):
			m_impl{std::forward<Args>(args)...}
		{

		}

		// Add a new parameter
		/*
		void addParameter(Parameter p)
		{
			//m_map.insert(p);
			// Send update (all / only the one)
		}
		*/

		void removeParameter(std::string destination)
		{
			//m_map.erase(destination);
			// Send update (all / only the one)
		}

/*
		ParameterMap map() const
		{
			return map;
		}

		// Updates the remote namespace
		void update(ParameterMap newMap)
		{
			m_map = newMap;
		}
		*/
};

}

// Clients should be able to tell if they want to get updates pushed to them.
// They would either choose :
//    - the whole tree each time
//    - only what changed
//
// They could also want to pull updates.



