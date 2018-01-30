#pragma once
#include "augs/templates/maybe_const.h"

template <class entity_handle_type, class component_type>
class synchronizer_base {
protected:
	/*
		A value of nullptr means that the entity has no such component.
	*/

	static constexpr bool is_const = entity_handle_type::is_const_value;
	using component_pointer = maybe_const_ptr_t<is_const, component_type>;

	component_pointer component;
	entity_handle_type handle;

public:
	auto get_handle() const {
		return handle;
	}

	auto& get_raw_component() const {
		return *component;
	}

	bool operator==(const std::nullptr_t) const {
		return component == nullptr;
	}

	bool operator!=(const std::nullptr_t) const {
		return component != nullptr;
	}

	operator bool() const {
		return component != nullptr;
	}

	synchronizer_base(
		const component_pointer c, 
		const entity_handle_type h
	) : 
		component(c), 
		handle(h)
	{}
};

template <class, class>
class component_synchronizer;