// code adapted from https://stackoverflow.com/a/34165367
#pragma once

#include "json.hpp"

#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

template<typename, typename = void>
struct is_specialized : std::false_type {};

template<typename Ty>
struct is_specialized < Ty, std::void_t<decltype(Ty{}) >> : std::true_type {};

template<typename Ty>
inline constexpr bool is_specialized_v = is_specialized<Ty>::value;

template<typename Base, typename Ty>
struct object_property
{
	using type = Ty;

	constexpr object_property(Ty Base::* member, std::string_view name)
		: member{ member }, name{ name } {}

	Ty Base::* member;
	std::string_view name;
};

template<typename T> // primary template
struct object_properties;

// sequence for
template<typename Ty, typename Func, Ty... Vals>
inline constexpr void for_sequence(std::integer_sequence<Ty, Vals...>, Func&& fn)
{
	(static_cast<void>(fn(std::integral_constant<Ty, Vals>{})), ...);
}

// serialize function
template<typename Ty>
inline nlohmann::json to_json(const Ty& obj_data)
{
	nlohmann::json json_data{};

	constexpr auto prop_size = std::tuple_size_v<decltype(object_properties<Ty>::value)>;
	for_sequence(std::make_index_sequence<prop_size>{}, [&](auto i)
		{
			constexpr auto prop = std::get<i>(object_properties<Ty>::value);
			using prop_type = decltype(prop)::type;
			auto& value = json_data[prop.name.data()];
			if constexpr (is_specialized_v<object_properties<prop_type>>)
				value = to_json(obj_data.*(prop.member)); // parses sub-properties
			else
				value = obj_data.*(prop.member);
		});

	return json_data;
}

// deserialize function
template<typename Ty, typename std::enable_if_t<std::is_default_constructible_v<Ty>, bool> = true>
inline Ty from_json(const nlohmann::json& json_data)
{
	Ty obj_data{};

	constexpr auto prop_size = std::tuple_size_v<decltype(object_properties<Ty>::value)>;
	for_sequence(std::make_index_sequence<prop_size>{}, [&](auto i)
		{
			constexpr auto prop = std::get<i>(object_properties<Ty>::value);
			using prop_type = decltype(prop)::type;
			const auto& value = json_data[prop.name.data()];
			if constexpr (is_specialized_v<object_properties<prop_type>>)
				obj_data.*(prop.member) = from_json<prop_type>(value); // parses sub-properties
			else
				obj_data.*(prop.member) = value.get<prop_type>();
		});

	return obj_data;
}
