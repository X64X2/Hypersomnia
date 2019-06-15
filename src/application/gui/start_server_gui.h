#pragma once
#include "application/setups/server/server_start_input.h"
#include "augs/math/vec2.h"
#include "augs/misc/imgui/standard_window_mixin.h"
#include "application/setups/server/server_instance_type.h"
#include "augs/misc/timing/timer.h"

class start_server_gui_state : public standard_window_mixin<start_server_gui_state> {
public:
	using base = standard_window_mixin<start_server_gui_state>;
	using base::base;

	server_instance_type instance_type = server_instance_type::INTEGRATED;

	bool show_help = false;
	augs::timer lockfile_query_timer;

	bool perform(
		server_start_input& into
	);
};
