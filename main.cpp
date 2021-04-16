#include "property.hpp"

#include <array>
#include <iostream>
#include <string>
#include <vector>

#define PROPERTY(base, member) object_property{ &base::member, #member }

struct my_object
{
	char a = 'c';
	int b = 1;
	float c = 3.f;

	struct nested_object
	{
		std::array<int, 3> r = { 0, 1, 2 };
		std::string s = "string";
		std::vector<int> v = { 3, 4 };

		bool operator==(const nested_object& rhs) const
		{
			return std::tie(r, s, v) == std::tie(rhs.r, rhs.s, rhs.v);
		}
	} d;

	bool operator==(const my_object& rhs) const
	{
		return std::tie(a, b, c, d) == std::tie(rhs.a, rhs.b, rhs.c, rhs.d);
	}
};

template<>
struct object_properties<my_object>
{
	using base = my_object;

	static constexpr auto value = std::make_tuple(
		PROPERTY(base, a),
		PROPERTY(base, b),
		PROPERTY(base, c),
		PROPERTY(base, d)
	);
};

template<>
struct object_properties<my_object::nested_object>
{
	using base = my_object::nested_object;

	static constexpr auto value = std::make_tuple(
		PROPERTY(base, r),
		PROPERTY(base, s),
		PROPERTY(base, v)
	);
};

int main()
{
	my_object obj{};

	const auto j = to_json(obj);
	std::cout << j.dump(4) << '\n';

	const auto obj2 = from_json<my_object>(j);
	std::cout << '\n';
	std::cout << "Objects are equal: " << std::boolalpha << (obj == obj2) << '\n';

	return 0;
}
