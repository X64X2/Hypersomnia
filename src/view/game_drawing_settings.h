#pragma once
#include "augs/pad_bytes.h"
#include "augs/templates/maybe.h"
#include "augs/graphics/rgba.h"
#include "game/modes/detail/fog_of_war_settings.h"

struct fog_of_war_appearance_settings {
	// GEN INTROSPECTOR struct fog_of_war_appearance_settings
	bool overlay_color_on_visible = true;
	rgba overlay_color = rgba(255, 255, 255, 2);
	// END GEN INTROSPECTOR
};

struct game_drawing_settings {
	// GEN INTROSPECTOR struct game_drawing_settings
	bool draw_crosshairs = true;
	bool draw_weapon_laser = true;
	bool draw_aabb_highlighter = true;
	augs::maybe<float> draw_area_markers = augs::maybe<float>(0.5f, true);
	augs::maybe<float> draw_callout_indicators = augs::maybe<float>(0.5f, true);
	bool draw_enemy_hud = false;
	bool draw_cp_bar = true;
	bool draw_pe_bar = false;

	bool draw_offscreen_indicators = true;
	bool draw_offscreen_callouts = true;
	bool draw_nicknames = true;
	bool draw_health_numbers = true;
	augs::maybe<float> draw_teammate_indicators = augs::maybe<float>(1.f, true);
	augs::maybe<rgba> draw_danger_indicators = augs::maybe<rgba>(rgba(255, 50, 50, 255), true);
	augs::maybe<float> draw_tactical_indicators = augs::maybe<float>(1.f, true);

	float show_danger_indicator_for_seconds = 5.f;
	float fade_danger_indicator_for_seconds = 2.f;

	float show_death_indicator_for_seconds = 5.f;
	float fade_death_indicator_for_seconds = 2.f;

	vec2 radar_pos = vec2i(-40, 20 + 16 * 4);
	bool print_current_character_callout = false;
	int nickname_characters_for_offscreen_indicators = 4;

	fog_of_war_settings fog_of_war;
	fog_of_war_appearance_settings fog_of_war_appearance;
	// END GEN INTROSPECTOR
};