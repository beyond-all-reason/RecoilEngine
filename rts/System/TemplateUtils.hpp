#pragma once

#include <memory>
#include <functional>
#include <tuple>
#include <variant>
#include <type_traits>

#include "Matrix44f.h"
#include "Transform.hpp"

// TODO Move to namespace Recoil
namespace spring {
	template<bool...> struct bool_pack;
	template<bool... bs>
	using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;

	// check if parameters pack is constructible
	template<class... Ts>
	using are_all_constructible = all_true<std::is_constructible_v<std::decay_t<Ts>>...>;

	// https://stackoverflow.com/questions/38067106/c-verify-callable-signature-of-template-type
	template<typename, typename, typename = void>
	struct is_signature : std::false_type {};

	// use static_assert(is_signature<SomeTemplateF, int(int)>::value);
	template<typename TFunc, typename Ret, typename... Args>
	struct is_signature<TFunc, Ret(Args...),
			typename std::enable_if_t<
				std::is_convertible<TFunc, std::function<Ret(Args...)>>::value
			>
		> : public std::true_type
	{};

	// use static_assert(is_signature<std::function<int(int)>, decltype(someFunction)>::value);
	template<typename TFunc1, typename TFunc2>
	struct is_signature<TFunc1, TFunc2,
			typename std::enable_if_t<
				std::is_convertible<TFunc1, TFunc2>::value
			>
		> : public std::true_type
	{};
	
	template<typename... Args>
	inline constexpr bool is_signature_v = is_signature<Args...>::value;

	// cpp20 compat
	template< class T >
	struct remove_cvref {
		typedef std::remove_cv_t<std::remove_reference_t<T>> type;
	};

	// https://blog.tartanllama.xyz/exploding-tuples-fold-expressions/
	template <std::size_t... Idx>
	auto make_index_dispatcher(std::index_sequence<Idx...>) {
		return [](auto&& f) { (f(std::integral_constant<std::size_t, Idx>{}), ...); };
	}

	template <std::size_t N>
	auto make_index_dispatcher() {
		return make_index_dispatcher(std::make_index_sequence<N>{});
	}

	template<typename... Type>
	struct type_list_t {
		using type = type_list_t;
		static constexpr auto size = sizeof...(Type);
	};

	template<typename... Type>
	static constexpr type_list_t<Type...> type_list{};

	template<std::size_t, typename>
	struct type_list_element;

	template<std::size_t Index, typename Type, typename... Other>
	struct type_list_element<Index, type_list_t<Type, Other...>>
		: type_list_element<Index - 1u, type_list_t<Other...>> {
	};

	template<typename Type, typename... Other>
	struct type_list_element<0u, type_list_t<Type, Other...>> {
		using type = Type;
	};

	template<std::size_t Index, typename List>
	using type_list_element_t = typename type_list_element<Index, List>::type;

	template<typename... Lists>
	struct type_list_concat;

	template<>
	struct type_list_concat<> {
		using type = type_list_t<>;
	};

	template<typename... Ts>
	struct type_list_concat<type_list_t<Ts...>> {
		using type = type_list_t<Ts...>;
	};

	template<typename... Ts, typename... Us, typename... Rest>
	struct type_list_concat<type_list_t<Ts...>, type_list_t<Us...>, Rest...> {
		using type = typename type_list_concat<type_list_t<Ts..., Us...>, Rest...>::type;
	};

	template<typename... Lists>
	using type_list_concat_t = typename type_list_concat<Lists...>::type;

	template<template<class> class Pred, typename List>
	struct type_list_filter;

	template<template<class> class Pred, typename... Ts>
	struct type_list_filter<Pred, type_list_t<Ts...>> {
		using type = type_list_concat_t<
			std::conditional_t<Pred<Ts>::value, type_list_t<Ts>, type_list_t<>>...
		>;
	};

	template<template<class> class Pred, typename List>
	using type_list_filter_t = typename type_list_filter<Pred, List>::type;

	template<typename T, typename TypeList>
	struct type_in_list;

	template<typename T, typename... Types>
	struct type_in_list<T, type_list_t<Types...>> {
		static constexpr bool value = (std::is_same_v<T, Types> || ...);
	};

	template<typename T, typename TypeList>
	constexpr bool type_in_list_v = type_in_list<T, TypeList>::value;

	template<typename T>
	struct function_traits {
		using return_type = T;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename... Ts>
	struct function_traits<std::function<R(Ts...)>> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename... Ts>
	struct function_traits<std::function<R(Ts...)> const> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename T, typename... Ts>
	struct function_traits<std::function<R(Ts...)> T::*> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename T, typename... Ts>
	struct function_traits<std::function<R(Ts...)> const T::*> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename T, typename... Ts>
	struct function_traits<std::function<R(Ts...)> T::* const&> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename T, typename... Ts>
	struct function_traits<std::function<R(Ts...)> const T::* const> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename... Ts>
	struct function_traits<R(*)(Ts...)> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename... Ts>
	struct function_traits<R& (*)(Ts...)> {
		using return_type = R;
		using arg_types = type_list_t<Ts...>;
	};

	template<typename R, typename T>
	struct function_traits<R(T::*)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<R& (T::*)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>(T::*)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>& (T::*)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<R(T::* const)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<R& (T::* const)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>(T::* const)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>& (T::* const)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<R(T::* const&)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<R& (T::* const&)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>(T::* const&)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename R, typename T>
	struct function_traits<std::shared_ptr<R>& (T::* const&)() const> {
		using return_type = R;
		using arg_types = type_list_t<>;
	};

	template<typename T>
	using return_type_t = typename function_traits<T>::return_type;

	template<typename T>
	using arg_types_t = typename function_traits<T>::arg_types;

	template<typename TypeList, typename Func, std::size_t... Is>
	inline void type_list_exec_at_impl(size_t index, TypeList&& t, Func&& f, std::index_sequence<Is...>)
	{
		using FnType = void(*)(TypeList&&, Func&&);
		static constexpr FnType table[] = {
			[](TypeList&&, Func&& func) { func(type_list_element_t<Is, std::decay_t<TypeList>> {}); }...
		};
		if (index < sizeof...(Is))
			table[index](std::forward<TypeList>(t), std::forward<Func>(f));
		else
			throw std::out_of_range("type list index out of range");
	}

	template<typename TypeList, typename Func>
	inline void type_list_exec_at(size_t index, TypeList&& typeList, Func&& f)
	{
		using TypeListDecayed = std::decay_t<TypeList>;
		type_list_exec_at_impl(index, std::forward<TypeList>(typeList), std::forward<Func>(f), std::make_index_sequence<TypeListDecayed::size>{});
	}

	// Executes f with each type in the type list, returns tuple of results (void return returns nothing)
	template<typename... T, typename F>
	constexpr decltype(auto) type_list_exec_all(type_list_t<T...>, F&& f) {
		if constexpr (std::is_void_v<return_type_t<F>>) {
			(f(T{}), ...);
		}
		else {
			return std::forward_as_tuple(f(T{})...);
		}
	}

	// same as type_list_exec_at...
	template<typename Tuple, typename Func, std::size_t... Is>
	inline void tuple_exec_at_impl(size_t index, Tuple& t, Func&& f, std::index_sequence<Is...>)
	{
		using FnType = void(*)(Tuple&, Func&&);
		static constexpr FnType table[] = {
			[](Tuple& t, Func&& func) { func(std::get<Is>(t)); }...
		};
		if (index < sizeof...(Is))
			table[index](t, std::forward<Func>(f));
		else
			throw std::out_of_range("tuple index out of range");
	}

	// Executes function f with the tuple element at the given index.
	// throws std::out_of_range if index >= tuple size
	template<typename Tuple, typename Func>
	inline void tuple_exec_at(size_t index, Tuple& t, Func&& f)
	{
		tuple_exec_at_impl(index, t, std::forward<Func>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
	}

	template<typename TupleType, typename Type>
	struct tuple_contains_type;

	template<typename Type, typename... TupleElementTypes>
	struct tuple_contains_type<std::tuple<TupleElementTypes...>, Type> : std::disjunction<std::is_same<Type, TupleElementTypes>...> {};

	template<typename TupleType, typename Type>
	constexpr inline bool tuple_contains_type_v = tuple_contains_type<TupleType, Type>::value;


	template <typename T, typename Tuple>
	struct tuple_type_index;

	template <typename T, typename... Types>
	struct tuple_type_index<T, std::tuple<T, Types...>> {
		static const std::size_t value = 0;
	};

	template <typename T, typename U, typename... Types>
	struct tuple_type_index<T, std::tuple<U, Types...>> {
		static const std::size_t value = 1 + tuple_type_index<T, std::tuple<Types...>>::value;
	};
	template <typename T, typename Tuple>
	constexpr size_t tuple_type_index_v = tuple_type_index<T, Tuple>::value;


	template<typename FuncType>
	struct func_signature;

	template<typename ReturnType, typename... ArgTypes>
	struct func_signature<ReturnType(ArgTypes...)> {
		using type = std::tuple<ArgTypes...>;
		using ret = ReturnType;
	};

	template<typename FuncType>
	using func_signature_t = typename func_signature<FuncType>::type;

	template<
		typename F,
		std::enable_if_t<std::is_function<F>::value, bool> = true
	>
	auto arg_types_tuple_t(const F&) -> typename func_signature<F>::type;
	template<
		typename F,
		std::enable_if_t<std::is_function<F>::value, bool> = true
	>
	auto arg_types_tuple_t(const F*) -> typename func_signature<F>::type;


	// This particular helper accepts a nullptr, in which case it falls back to a specified default signature, or just an empty tuple
	template<auto FuncPtr, typename... FallbackSignature>
	struct func_ptr_signature {
		using type = func_signature_t<std::remove_pointer_t<std::remove_pointer_t<decltype(FuncPtr)>>>;
	};
	template<typename... FallbackSignature>
	struct func_ptr_signature<nullptr, FallbackSignature...> {
		using type = std::tuple<FallbackSignature...>;
	};

	template<auto FuncPtr, typename... FallbackSignature>
	using func_ptr_signature_t = typename func_ptr_signature<FuncPtr, FallbackSignature...>::type;
};

namespace Recoil {
	// taken from https://github.com/spnda/fastgltf/blob/main/include/fastgltf/util.hpp
	// The MIT License
	// Copyright (C) 2022 - 2025 Sean Apeler
	template <typename Visitor, typename Variant, std::size_t... i>
	constexpr bool is_exhaustive_visitor(std::integer_sequence<std::size_t, i...>) noexcept {
		return std::conjunction_v<std::is_invocable<Visitor, std::variant_alternative_t<i, std::remove_cvref_t<Variant>>>...>;
	}

	/**
	 * Simple wrapper around std::visit for a single variant that checks at compile-time if the given visitor contains
	 * overloads for *all* required alternatives.
	 */
	template<typename Visitor, typename Variant>
		constexpr decltype(auto) visit_exhaustive(Visitor&& visitor, Variant&& variant) {
		static_assert(is_exhaustive_visitor<Visitor, Variant>(std::make_index_sequence<std::variant_size_v<std::remove_cvref_t<Variant>>>()),
			"The visitor does not include all necessary overloads for the given variant");
		return std::visit(std::forward<Visitor>(visitor), std::forward<Variant>(variant));
	}

	// useful in the context of static_assert(always_false_v<T>, ...)
	// https://stackoverflow.com/a/76675119
	template <typename T>
	constexpr bool always_false_v = false;
}

namespace Concepts {
	template <typename T>
	concept HasSizeAndData = requires(T t) {
		{ t.size() } -> std::same_as<std::size_t>;
		{ t.data() } -> std::convertible_to<const typename T::value_type*>;
	};
	template<typename T>
	concept HasMemberBeginEnd = requires(T t) {
		{ t.begin() } -> std::input_or_output_iterator;
		{ t.end() } -> std::sentinel_for<decltype(t.begin())>;
	};
	template <typename T>
	concept CanTransform = std::same_as<T, CMatrix44f> || std::same_as<T, Transform>;
}