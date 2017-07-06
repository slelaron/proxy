#pragma once

#include <functional>
#include <vector>

template <typename... Args>
struct file_descriptor;
struct simple_file_descriptor;
struct acceptable_tag;
struct executable_tag;
struct writable;
struct readable;
struct everything_executor;

template <typename T, unsigned item>
struct from_container_erasable;

template <typename T, typename... Args>
struct contains_in_args
{
	static const bool value = false;
};

template <typename T, typename F, typename... Args>
struct contains_in_args <T, F, Args...>
{
	static const bool value = contains_in_args <T, Args...>::value;
};

template <typename T, typename... Args>
struct contains_in_args <T, T, Args...>
{
	static const bool value = true;
};

template <typename T, typename... Args>
struct contains_convertible_in_args
{
	static const unsigned value = 0;
};

template <typename T, typename F, typename... Args>
struct contains_convertible_in_args <T, F, Args...>
{
	static const unsigned value = contains_convertible_in_args <T, Args...>::value + std::is_convertible <F, T>::value;
};

template <bool truth, typename Including, typename... Args>
struct include_if_absent
{
	typedef file_descriptor <Args...> type;
};

template <typename Including, typename... Args>
struct include_if_absent <true, Including, Args...>
{
	typedef file_descriptor <Including, Args...> type;
};

template <typename T, typename... Args>
struct get_first_convertible
{
	typedef void value;
};

template <typename T, typename F, typename... Args>
struct get_first_convertible <T, F, Args...>
{
	typedef typename std::conditional <std::is_convertible <F, T>::value, F, typename get_first_convertible <T, Args...>::value>::type value;
};

template <typename... Args>
struct another_list
{
	typedef void next;
	static const bool value = true;
};

template <typename Q, typename... Args>
struct another_list <Q, Args...>
{
	static const bool value = std::is_convertible <Q, executable_tag>::value;
	typedef typename std::conditional <another_list <Args...>::value, another_list <Args...>, typename another_list <Args...>::next>::type next;
	typedef Q type;
};

template <typename... Args>
struct another_list <std::tuple <Args...> >
{
	typedef another_list <Args...> next;
};

template <typename... Args>
struct unpack_from_file_descriptor_to_tuple
{};

template <typename... Args>
struct unpack_from_file_descriptor_to_tuple <file_descriptor <Args...> >
{
	typedef std::tuple <void, Args...> type;
};

template <typename... Args>
struct template_list
{};

template <typename... Args>
struct template_list <another_list <Args...>>
{
	static const unsigned count = 0;
	
	template <typename T>
	static void set(const T& descriptor)
	{}
};

template <typename Q, typename... Args>
struct template_list <another_list <Q, Args...>>
{
	typedef template_list <typename another_list <Q, Args...>::next> next;
	typedef typename another_list <Q, Args...>::type type;
	static const unsigned count = next::count + 1;

	template <size_t item, typename T, typename... Args1>
	static typename std::enable_if <item + 1 < std::tuple_size <std::tuple <Args1...>>::value, void>::type set(T& descriptor, std::tuple <Args1...> arg)
	{
		static_cast <type&> (descriptor).set_func(std::get<item>(arg));
		next::template set <item + 1>(descriptor, arg);
	}

	template <size_t item, typename T, typename... Args1>
	static typename std::enable_if <item + 1 == std::tuple_size <std::tuple <Args1...>>::value, void>::type set(T& descriptor, std::tuple <Args1...> arg)
	{
		static_cast <type&> (descriptor).set_func(std::get<item>(arg));
	}
};
