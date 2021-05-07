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
struct is_specialized<Ty, std::void_t<decltype(Ty{})>> : std::true_type {};

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

template<typename Ty>
inline void serialize_to_json(const Ty& obj_data, nlohmann::json& json_data)
{
	constexpr auto prop_size = std::tuple_size_v<decltype(object_properties<Ty>::value)>;
	for_sequence(std::make_index_sequence<prop_size>{}, [&](auto i)
		{
			constexpr auto prop = std::get<i>(object_properties<Ty>::value);
			using prop_type = decltype(prop)::type;
			const auto& obj_member = obj_data.*(prop.member);
			auto& json_value = json_data[prop.name.data()];
			if constexpr (is_specialized_v<object_properties<prop_type>>)
				serialize_to_json(obj_member, json_value); // recursively parses sub-properties
			else
				json_value = obj_member;
		});
}

template<typename Ty>
inline nlohmann::json serialize_to_json(const Ty& obj_data)
{
	nlohmann::json json_data{};
	serialize_to_json(obj_data, json_data);
	return json_data;
}

template<typename Ty>
inline void deserialize_from_json(const nlohmann::json& json_data, Ty& obj_data)
{
	constexpr auto prop_size = std::tuple_size_v<decltype(object_properties<Ty>::value)>;
	for_sequence(std::make_index_sequence<prop_size>{}, [&](auto i)
		{
			constexpr auto prop = std::get<i>(object_properties<Ty>::value);
			using prop_type = decltype(prop)::type;
			auto& obj_member = obj_data.*(prop.member);
			const auto& json_value = json_data[prop.name.data()];
			if constexpr (is_specialized_v<object_properties<prop_type>>)
				deserialize_from_json<prop_type>(json_value, obj_member); // recursively parses sub-properties
			else
				obj_member = json_value.get<prop_type>();
		});
}

template<typename Ty, typename std::enable_if_t<std::is_default_constructible_v<Ty>, bool> = true>
inline Ty deserialize_from_json(const nlohmann::json& json_data)
{
	Ty obj_data{};
	deserialize_from_json(json_data, obj_data);
	return obj_data;
}
