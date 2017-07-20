#pragma once

template <class Enum>
class menu_ui_root;

template <class E>
class menu_ui_root_in_context {
public:
	using dereferenced_type = menu_ui_root<E>;

	bool operator==(menu_ui_root_in_context b) const {
		return true;
	}

	template <class C>
	bool alive(const C context) const {
		return true;
	}

	template <class C>
	decltype(auto) dereference(const C context) const {
		return &context.get_root();
	}
};

namespace std {
	template <class E>
	struct hash<menu_ui_root_in_context<E>> {
		size_t operator()(const menu_ui_root_in_context<E>& k) const {
			return hash<size_t>()(typeid(menu_ui_root_in_context<E>).hash_code());
		}
	};
}