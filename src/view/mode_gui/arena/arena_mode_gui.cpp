#include "view/mode_gui/arena/arena_mode_gui.h"
#include "view/viewables/images_in_atlas_map.h"
#include "augs/gui/text/printer.h"
#include "augs/templates/chrono_templates.h"
#include "application/config_lua_table.h"
#include "game/modes/mode_player_id.h"
#include "augs/window_framework/event.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "game/cosmos/entity_handle.h"
#include "game/modes/bomb_mode.hpp"

#include "game/cosmos/cosmos.h"
#include "game/modes/test_scene_mode.h"
#include "game/modes/bomb_mode.h"
#include "augs/string/format_enum.h"

bool arena_scoreboard_gui::control(
	const augs::event::state& common_input_state,
	const augs::event::change e
) {
	using namespace augs::event;
	using namespace augs::event::keys;

	(void)common_input_state;
	/* const bool has_ctrl{ common_input_state[key::LCTRL] }; */
	/* const bool has_shift{ common_input_state[key::LSHIFT] }; */

	if (e.was_pressed(key::TAB)) {
		show = true;
		return true;
	}

	if (e.was_released(key::TAB)) {
		show = false;
		return true;
	}

	return false;
}

template <class M>
void arena_scoreboard_gui::perform_imgui(
	const draw_mode_gui_input draw_in, 
	const M& typed_mode, 
	const typename M::input& mode_input
) {
	if (!show) {
		return;
	}

	using namespace augs::imgui;

	ImGui::SetNextWindowPosCenter();

	ImGui::SetNextWindowSize((vec2(ImGui::GetIO().DisplaySize) * 0.6f).operator ImVec2(), ImGuiCond_FirstUseEver);

	const auto window_name = "Scoreboard";
	auto window = scoped_window(window_name, nullptr, ImGuiWindowFlags_NoTitleBar);

	{
		const auto s = ImGui::CalcTextSize(window_name, nullptr, true);
		const auto w = ImGui::GetWindowWidth();
		ImGui::SetCursorPosX(w / 2 - s.x / 2);
		text(window_name);
	}


	auto spacing = ImGui::GetStyle().ItemSpacing;
	spacing.y *= 1.9;
	auto spacing_scope = scoped_style_var(ImGuiStyleVar_ItemSpacing, spacing);

	const auto p = typed_mode.calc_participating_factions(mode_input);

	const auto num_columns = 8;

	const auto ping_col_w = ImGui::CalcTextSize("9999", nullptr, true).x;
	const auto point_col_w = ImGui::CalcTextSize("999", nullptr, true).x;
	const auto score_col_w = ImGui::CalcTextSize("999999", nullptr, true).x;
	const auto money_col_w = ImGui::CalcTextSize("999999$", nullptr, true).x;

	ImGui::Columns(num_columns, nullptr, false);

	const auto w_off = 12;

	ImGui::SetColumnWidth(0, ping_col_w + w_off);
	ImGui::SetColumnWidth(2, money_col_w + w_off);
	ImGui::SetColumnWidth(3, point_col_w + w_off);
	ImGui::SetColumnWidth(4, point_col_w + w_off);
	ImGui::SetColumnWidth(5, point_col_w + w_off);
	ImGui::SetColumnWidth(6, score_col_w + w_off);

	text_disabled("Ping");
	ImGui::NextColumn();
	text_disabled("Player");
	ImGui::NextColumn();
	text_disabled("Money");
	ImGui::NextColumn();
	text_disabled("K");
	ImGui::NextColumn();
	text_disabled("A");
	ImGui::NextColumn();
	text_disabled("D");
	ImGui::NextColumn();
	text_disabled("Score");
	ImGui::NextColumn();
	ImGui::NextColumn();

	ImGui::Columns(1);

	auto print_faction = [&](const faction_type faction, const bool on_top) {
		const auto orig_faction_col = get_faction_color(faction);
		const auto faction_color = rgba(orig_faction_col).multiply_rgb(1.2f);
		const auto disabled_faction_color = rgba(orig_faction_col).multiply_rgb(.6f);

		const auto faction_bg = rgba(orig_faction_col).multiply_rgb(.25f);
		const auto disabled_faction_bg = rgba(orig_faction_col).multiply_rgb(.085f);

		auto scope_separator = scoped_style_color(ImGuiCol_Separator, disabled_faction_bg);

		ImGui::Columns(num_columns, nullptr, false);

		const auto& cosm = mode_input.cosm;

		std::vector<std::pair<bomb_mode_player, mode_player_id>> sorted_players;

		typed_mode.for_each_player_in(faction, [&](
			const auto& id, 
			const auto& player
		) {
			sorted_players.emplace_back(player, id);
		});

		sort_range(sorted_players);

		auto draw_headline = [&]() {
			ImGui::Columns(1);

			const auto& faction_state = typed_mode.factions[faction];
			const auto faction_name = format_enum(faction);
			const auto headline = [&]() {
				const auto& score = faction_state.score;

				auto score_number = typesafe_sprintf("%x", score);

				if (score < 10) {
					score_number += " ";
				}

				const auto n_players = sorted_players.size();
				const auto n_players_conscious = typed_mode.num_conscious_players_in(cosm, faction);

				const auto l1 = typesafe_sprintf("%x for %x", score_number, faction_name);
				const auto l2 = typesafe_sprintf("Players conscious: %x/%x", n_players_conscious, n_players); 

				return std::make_pair(l1, l2);
			}();

			auto scope_header = scoped_style_color(ImGuiCol_Header, disabled_faction_bg);
			auto scope_header_hovered = scoped_style_color(ImGuiCol_HeaderHovered, disabled_faction_bg);
			auto scope_header_active = scoped_style_color(ImGuiCol_HeaderActive, disabled_faction_bg);

			auto scope_text = scoped_text_color(faction_color);

			if (on_top) {
				ImGui::Selectable(" ", true);
			}

			ImGui::Selectable(headline.first.c_str(), true);

			{
				const auto w = ImGui::CalcTextSize(headline.second.c_str(), nullptr, true);
				ImGui::SameLine(ImGui::GetWindowWidth() - w.x - 10);
			}

			text_color(headline.second, faction_color);

			if (!on_top) {
				ImGui::Selectable(" ", true);
			}
		};

		if (!on_top) {
			draw_headline();
			ImGui::Columns(num_columns, nullptr, false);
		}

		for (const auto& s : sorted_players) {
			const auto& id = s.second;

			const auto& player = s.first;

			const auto player_handle = cosm[player.guid];
			const auto is_conscious = player_handle.alive() && player_handle.template get<components::sentience>().is_conscious();

			auto bg_color = is_conscious ? faction_bg : disabled_faction_bg;
			auto color = is_conscious ? faction_color : disabled_faction_color;

			if (id == draw_in.local_player) {
				bg_color.multiply_rgb(1.6f);
				color.multiply_rgb(1.9f);
			}

			auto scope = scoped_style_color(ImGuiCol_Button, bg_color);
			auto scope_text = scoped_text_color(color);
 
			const auto ping = 0;
			const auto ping_str = typesafe_sprintf("%x", ping);

			auto scope_header = scoped_style_color(ImGuiCol_Header, bg_color);
			auto scope_header_hovered = scoped_style_color(ImGuiCol_HeaderHovered, rgba(bg_color).multiply_rgb(1.4f));
			auto scope_header_active = scoped_style_color(ImGuiCol_HeaderActive, rgba(bg_color).multiply_rgb(1.6f));

			ImGui::Selectable(ping_str.c_str(), true, ImGuiSelectableFlags_SpanAllColumns);

			ImGui::NextColumn();
			text(player.chosen_name);
			ImGui::NextColumn();
			text(typesafe_sprintf("%x$", player.money));
			ImGui::NextColumn();
			text(typesafe_sprintf("%x", player.knockouts));
			ImGui::NextColumn();
			text(typesafe_sprintf("%x", player.assists));
			ImGui::NextColumn();
			text(typesafe_sprintf("%x", player.deaths));
			ImGui::NextColumn();

			text(typesafe_sprintf("%x", player.calc_score()));
			ImGui::NextColumn();
			ImGui::NextColumn();

			/* auto& sepcol = ImGui::GetStyle().Colors[ImGuiCol_Separator]; */
			/* auto prev = sepcol; */
			/* sepcol = white; */

			/* { */
			{
				auto spacing_sep_scope = scoped_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 0));
				auto b_scope_header = scoped_style_color(ImGuiCol_Header, disabled_faction_bg);
				auto b_scope_header_hovered = scoped_style_color(ImGuiCol_HeaderHovered, disabled_faction_bg);
				auto b_scope_header_active = scoped_style_color(ImGuiCol_HeaderActive, disabled_faction_bg);
				auto b_sep = scoped_style_color(ImGuiCol_Separator, disabled_faction_bg);

			/* 	auto idd = scoped_id(id.value); */
			//ImGui::Selectable("##dummy", true, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0));
				ImGui::Separator();
			}

				next_columns(num_columns);
			/* } */
			/* auto bss = ImVec2(ImGui::GetWindowContentRegionMax().x, 2); */
			/* ImGui::Button("", bss); */

			/* auto spacing_sep_scope = scoped_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(spacing.x, 1)); */
			/* ImGui::Separator(); */
			/* ImGui::Separator(); */
			/* sepcol = prev; */
		}

		if (on_top) {
			draw_headline();
		}

		ImGui::Columns(1);
	};

	print_faction(p.defusing, true);
	print_faction(p.bombing, false);
}

bool arena_gui_state::control(
	const augs::event::state& common_input_state,
	const augs::event::change e
) {
	if (scoreboard.control(common_input_state, e)) {
		return true;
	}

	return false;
}

template <class M>
void arena_gui_state::perform_imgui(
	draw_mode_gui_input in, 
	const M& typed_mode, 
	const typename M::input& mode_input
) {
	if constexpr(M::round_based) {
		scoreboard.perform_imgui(in, typed_mode, mode_input);
	}
}

template <class M>
void arena_gui_state::draw_mode_gui(
	const draw_setup_gui_input& in,
	const draw_mode_gui_input& mode_in,

	const M& typed_mode, 
	const typename M::input& mode_input
) const {
	const auto& cfg = in.config.arena_mode_gui;

	if constexpr(M::round_based) {
		using namespace augs::gui::text;

		const auto local_player = mode_in.local_player;
		auto game_screen_top = mode_in.game_screen_top;
		game_screen_top += 2;

		{
			const auto& cosm = mode_input.cosm;
			const auto& kos = typed_mode.knockouts;
			const auto& clk = cosm.get_clock();

			const auto knockouts_to_show = std::min(
				kos.size(), 
				static_cast<std::size_t>(cfg.show_recent_knockouts_num)
			);

			const auto starting_i = [&]() {
				auto i = kos.size() - knockouts_to_show;

				while (i < kos.size() && clk.diff_seconds(kos[i].when) >= cfg.keep_knockout_boxes_for_seconds) {
					++i;
				}

				return i;
			}();

			auto pen = vec2i(10, game_screen_top);

			for (std::size_t i = starting_i; i < kos.size(); ++i) {
				pen.x = 10;
				pen.y += cfg.between_knockout_boxes_pad;

				const auto& ko = kos[i];

				auto get_col = [&](const mode_player_id id) {
					if (const auto p = mapped_or_nullptr(typed_mode.players, id)) {
						return ::get_faction_color(p->faction);
					}

					return gray;
				};

				auto get_name = [&](const auto id) -> entity_name_str {
					if (!id.is_set()) {
						return "";
					}

					if (const auto p = mapped_or_nullptr(typed_mode.players, id)) {
						return p->chosen_name;
					}

					return "Disconnected";
				};

				auto colored = [&](const auto& text, const auto& c) {
					const auto text_style = style(
						in.gui_font,
						c
					);

					return formatted_string(text, text_style);
				};

				const auto knockouter = get_name(ko.knockouter);
				const auto assist = get_name(ko.assist);
				const auto victim = get_name(ko.victim);

				const bool is_suicide = ko.knockouter == ko.victim;
				(void)is_suicide;

				const auto lhs_text = 
					colored(is_suicide ? "" : knockouter, get_col(ko.knockouter))
					+ colored(assist.size() > 0 ? " (+ " + assist + ")" : "", get_col(ko.assist))
				;

				const auto rhs_text = colored(victim, get_col(ko.victim));

				const auto lhs_bbox = get_text_bbox(lhs_text);
				const auto rhs_bbox = get_text_bbox(rhs_text);

				const auto bg_alpha = 0.9f;

				struct colors {
					rgba background;
					rgba border;
				} cols = [&]() -> colors {
					if (local_player == ko.victim) {
						return { rgba(150, 0, 0, 170), rgba(0, 0, 0, 0) };
					}

					if (local_player == ko.knockouter || local_player == ko.assist) {
						return { black, rgba(180, 0, 0, 255) };
					}
					
					return { { 0, 0, 0, 80 }, rgba(0, 0, 0, 0) };
				}();

				cols.background.multiply_alpha(bg_alpha);
				cols.border.multiply_alpha(bg_alpha);

				const auto tool_image_id = [&]() {
					auto from_flavour = [&](const auto flavour_id) -> auto {
						if (!flavour_id.is_set()) {
							return assets::image_id();
						}

						return flavour_id.dispatch([&](const auto& typed_flavour_id) {
							if (const auto flavour = cosm.find_flavour(typed_flavour_id)) {
								if (const auto sentience = flavour->template find<invariants::sentience>()) {
									return assets::image_id();
								}

								if (const auto sprite = flavour->template find<invariants::sprite>()) {
									return sprite->image_id;
								}
							}

							return assets::image_id();
						});
					};

					if (ko.origin.cause.spell.is_set()) {
						return ko.origin.cause.spell.dispatch([&](auto dummy){
							const auto& meta = get_meta_of(dummy, cosm.get_common_significant().spells);
							return meta.appearance.icon;
						});
					}

					if (const auto img = from_flavour(ko.origin.sender.direct_sender_flavour); img.is_set()) {
						return img;
					}

					if (const auto img = from_flavour(ko.origin.cause.flavour); img.is_set()) {
						return img;
					}

					return assets::image_id();
				}();

				const auto& entry = tool_image_id.is_set() ? in.images_in_atlas.at(tool_image_id) : image_in_atlas();

				const auto tool_size = [&]() {
					const auto original_tool_size = static_cast<vec2i>(entry.get_original_size());
					const auto max_tool_height = cfg.max_weapon_icon_height;

					if (max_tool_height == 0) {
						/* No limit */
						return original_tool_size;
					}

					const auto m = vec2(0, max_tool_height);
					auto s = vec2(original_tool_size);

					if (s.y > m.y) {
						s.x *= m.y / s.y;
						s.y = m.y;
					}

					return static_cast<vec2i>(s);
				}();

				const auto total_bbox = xywhi(
					pen.x,
					pen.y,
					cfg.inside_knockout_box_pad * 2 + cfg.weapon_icon_horizontal_pad * 2 + tool_size.x + lhs_bbox.x + rhs_bbox.x, 
					cfg.inside_knockout_box_pad * 2 + std::max(tool_size.y, std::max(lhs_bbox.y, rhs_bbox.y))
				);

				in.drawer.aabb_with_border(
					total_bbox,
					cols.background,
					cols.border,
					border_input { 1, 0 }
				);

				pen.x += cfg.inside_knockout_box_pad;

				const auto icon_drawn_aabb = [&]() {
					auto r = ltrbi(vec2i(pen.x + lhs_bbox.x + cfg.weapon_icon_horizontal_pad, 0), tool_size);

					auto temp = r;
					temp.place_in_center_of(ltrb(total_bbox));

					r.t = temp.t;
					r.b = temp.b;

					return r;
				}();

				pen.y = icon_drawn_aabb.get_center().y;

				print_stroked(
					in.drawer,
					pen,
					lhs_text,
					{ augs::center::Y }
				);

				pen.x += lhs_bbox.x;
				pen.x += cfg.weapon_icon_horizontal_pad;

				{
					in.drawer.base::aabb(
						entry.diffuse,
						icon_drawn_aabb,
						white
					);
				}

				pen.x += tool_size.x;
				pen.x += cfg.weapon_icon_horizontal_pad;

				print_stroked(
					in.drawer,
					pen,
					rhs_text,
					{ augs::center::Y }
				);

				pen.y = total_bbox.b();
			}
		}

		auto draw_indicator_at = [&](const std::string& val, const rgba& col, const auto t) {
			const auto text_style = style(
				in.gui_font,
				col
			);

			const auto s = in.screen_size;

			print_stroked(
				in.drawer,
				{ s.x / 2, static_cast<int>(t) },
				formatted_string(val, text_style),
				{ augs::center::X }
			);
		};

		auto play_tick_if_soon = [&](const auto& val, const auto& when_ticking_starts, const bool alarm) {
			auto play_tick = [&]() {
				auto& vol = in.config.audio_volume;

				tick_sound.just_play(
					alarm ? in.sounds.alarm_tick : in.sounds.round_clock_tick, 
					vol.sound_effects
				);
			};

			auto& last = last_seconds_value;

			if (val > 0.f && val <= when_ticking_starts) {
				if (last == std::nullopt) {
					play_tick();
				}
				else {
					if (val < *last) {
						play_tick();
					}
				}

				last = val;
			}
			else {
				last = std::nullopt;
			}
		};

		auto draw_time_at_top = [&](const std::string& val, const rgba& col) {
			draw_indicator_at(val, col, game_screen_top);
		};

		auto draw_info_indicator = [&](const std::string& val, const rgba& col) {
			const auto one_fourth_t = in.screen_size.y / 6;
			draw_indicator_at(val, col, one_fourth_t);
		};

		auto draw_warmup_indicator = draw_info_indicator;

		if (const auto warmup_left = typed_mode.get_warmup_seconds_left(mode_input); warmup_left > 0.f) {
			const auto c = std::ceil(warmup_left);

			play_tick_if_soon(c, 5.f, true);
			draw_warmup_indicator("WARMUP\n" + format_mins_secs(c), white);
			return;
		}

		{
			if (const auto secs = typed_mode.get_seconds_since_planting(mode_input); secs >= 0.f && secs <= 3.f) {
				draw_info_indicator("The bomb has been planted!", yellow);
				return;
			}

			auto& win = typed_mode.last_win;

			if (win.was_set()) {
				draw_info_indicator(format_enum(win.winner) + " wins!", yellow);
				return;
			}
		}

		if (const auto match_begins_in_seconds = typed_mode.get_match_begins_in_seconds(mode_input); match_begins_in_seconds >= 0.f) {
			const auto c = std::ceil(match_begins_in_seconds - 1.f);

			if (c > 0.f) {
				draw_warmup_indicator("Match begins in\n" + format_mins_secs(match_begins_in_seconds), yellow);
			}
			else {
				draw_warmup_indicator("The match has begun", yellow);
			}

			return;
		}

		if (const auto freeze_left = typed_mode.get_freeze_seconds_left(mode_input); freeze_left > 0.f) {
			const auto c = std::ceil(freeze_left);
			const auto col = c <= 10.f ? red : white;

			play_tick_if_soon(c, 3.f, false);
			draw_time_at_top(format_mins_secs(c), col);

			return;
		}

		if (const auto critical_left = typed_mode.get_critical_seconds_left(mode_input); critical_left > 0.f) {
			const auto c = std::ceil(critical_left);
			const auto col = red;

			draw_time_at_top(format_mins_secs(c), col);

			return;
		}

		if (const auto time_left = std::ceil(typed_mode.get_round_seconds_left(mode_input)); time_left > 0.f) {
			const auto c = std::ceil(time_left);
			const auto col = time_left <= 10.f ? red : white;

			draw_time_at_top(format_mins_secs(c), col);

			return;
		}

		/* If the code reaches here, it's probably the round end delay, so draw no timer. */
	}
	else {
		(void)typed_mode;
		(void)in;
	}
}

template void arena_gui_state::draw_mode_gui(
	const draw_setup_gui_input&,
	const draw_mode_gui_input&,
	const test_scene_mode&, 
	const test_scene_mode::input&
) const;

template void arena_gui_state::draw_mode_gui(
	const draw_setup_gui_input&,
	const draw_mode_gui_input&,
	const bomb_mode&, 
	const bomb_mode::input&
) const;

template void arena_gui_state::perform_imgui(
	draw_mode_gui_input, 
	const bomb_mode&, 
	const typename bomb_mode::input&
);

template void arena_gui_state::perform_imgui(
	draw_mode_gui_input, 
	const test_scene_mode&, 
	const typename test_scene_mode::input&
);