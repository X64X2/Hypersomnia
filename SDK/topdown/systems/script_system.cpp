#include "stdafx.h"
#include "entity_system/world.h"
#include "entity_system/entity.h"

#include "script_system.h"

#include "../components/scriptable_component.h"
#include "../bindings/bindings.h"

int bitor(lua_State* L) {
	int arg_count = lua_gettop(L);
	int result = 0;

	for (int i = 1; i <= arg_count; i++) {
		luabind::object obj(luabind::from_stack(L, i));

		if(luabind::type(obj) == LUA_TNUMBER) 
			result |= luabind::object_cast<int>(obj);
	}

	lua_pushinteger(L, result);
	return 1;
}

int bitflag(lua_State* L) {
	int result = 1 << luabind::object_cast<int>(luabind::object(luabind::from_stack(L, 1)));
	lua_pushinteger(L, result);
	return 1;
}


namespace bindings {
	extern luabind::scope
		_minmax(),
		_vec2(),
		_b2Filter(),
		_rgba(),
		_rect_ltrb(),
		_rect_xywh(),
		_glwindow(),
		_script(),
		_texture(),
		_animation(),
		_world(),
		_entity_ptr(),
		_sprite(),

		_particle(),
		_emission(),
		_particle_effect(),

		_intent_message(),
		_animate_message(),
		_particle_burst_message(),

		_render_component(),
		_transform_component(),
		_animate_component(),
		_camera_component(),
		_chase_component(),
		_children_component(),
		_crosshair_component(),
		_damage_component(),
		_gun_component(),
		_health_component(),
		_input_component(),
		_lookat_component(),
		_movement_component(),
		_particle_emitter_component(),
		_physics_component(),
		_scriptable_component(),

		_entity(),
		_body_helper();
}

script_system::script_system() : lua_state(luaL_newstate()) {
	using namespace resources;
	using namespace topdown;

	luabind::open(lua_state);

	luaL_openlibs(lua_state);

	lua_register(lua_state, "bitor", bitor);
	lua_register(lua_state, "bitflag", bitflag);
	luabind::module(lua_state)[
			bindings::_minmax(),
			bindings::_vec2(),
			bindings::_b2Filter(),
			bindings::_rgba(),
			bindings::_rect_ltrb(),
			bindings::_rect_xywh(),
			bindings::_glwindow(),
			bindings::_script(),
			bindings::_texture(),
			bindings::_animation(),
			bindings::_world(),
			bindings::_entity_ptr(),
			bindings::_sprite(),

			bindings::_particle(),
			bindings::_emission(),
			bindings::_particle_effect(),
					  
			bindings::_intent_message(),
			bindings::_animate_message(),
			bindings::_particle_burst_message(),

			bindings::_render_component(),
			bindings::_transform_component(),
			bindings::_animate_component(),
			bindings::_camera_component(),
			bindings::_chase_component(),
			bindings::_children_component(),
			bindings::_crosshair_component(),
			bindings::_damage_component(),
			bindings::_gun_component(),
			bindings::_health_component(),
			bindings::_input_component(),
			bindings::_lookat_component(),
			bindings::_movement_component(),
			bindings::_particle_emitter_component(),
			bindings::_physics_component(),
			bindings::_scriptable_component(),

			bindings::_entity(),
			bindings::_body_helper()
	];
}

script_system::~script_system() {
	lua_close(lua_state);
}

void script_system::process_entities(world& owner) {
}