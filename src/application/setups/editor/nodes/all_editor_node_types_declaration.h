#pragma once
#include "augs/templates/type_list.h"
#include "augs/templates/type_mod_templates.h"

struct editor_light_node;
struct editor_sprite_node;
struct editor_sound_node;
struct editor_prefab_node;
struct editor_particles_node;
struct editor_wandering_pixels_node;

struct editor_firearm_node;
struct editor_ammunition_node;
struct editor_melee_node;
struct editor_explosive_node;

using all_editor_node_types = type_list<
	editor_sprite_node,
	editor_sound_node,
	editor_light_node,
	editor_particles_node,
	editor_wandering_pixels_node,

	editor_firearm_node,
	editor_ammunition_node,
	editor_melee_node,
	editor_explosive_node

	//editor_prefab_node,
>;
