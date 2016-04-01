#pragma once
#include "entity_system/processing_system.h"
#include "../components/gui_element_component.h"
#include "../messages/camera_render_request_message.h"
#include "../messages/gui_intents.h"
#include "../messages/raw_window_input_message.h"

#include "augs/gui/gui_world.h"
#include "../detail/gui/game_gui_root.h"

class gui_system : public augs::processing_system_templated<components::gui_element> {
	friend class item_button;

	bool is_gui_look_enabled = false;
	bool preview_due_to_item_picking_request = false;

	game_gui_world gui;
	game_gui_root game_gui_root;

	vec2 initial_inventory_root_position();
	augs::entity_id get_game_world_crosshair();

	std::vector<messages::raw_window_input_message> buffered_inputs_during_freeze;
	bool freeze_gui_model();
public:
	gui_system(world& parent_world);

	void resize(vec2i size) { gui.resize(size); }

	void rebuild_gui_tree_based_on_game_state();
	void translate_raw_window_inputs_to_gui_events();
	void suppress_inputs_meant_for_gui();

	void switch_to_gui_mode_and_back();

	void draw_gui_overlays_for_camera_rendering_request(messages::camera_render_request_message);

	rects::xywh<float> get_rectangle_for_slot_function(slot_function);
	vec2i get_initial_position_for_special_control(special_control);
};