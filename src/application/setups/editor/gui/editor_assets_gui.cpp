#include "augs/string/string_templates.h"

#include "augs/templates/introspection_utils/field_name_tracker.h"
#include "augs/templates/introspection_utils/introspect_with_containers.h"

#include "augs/misc/imgui/imgui_scope_wrappers.h"
#include "augs/misc/imgui/imgui_control_wrappers.h"
#include "augs/misc/imgui/path_tree_structs.h"

#include "game/organization/for_each_entity_type.h"

#include "application/setups/editor/gui/editor_assets_gui.h"
#include "application/setups/editor/editor_folder.h"
#include "application/intercosm.h"

#include "application/setups/editor/detail/format_struct_name.h"
#include "augs/templates/introspection_utils/validate_fields_in.h"

template <class id_type>
struct asset_gui_path_entry : public browsed_path_entry_base {
	using base = browsed_path_entry_base;

	id_type id;
	std::vector<std::string> locations;

	bool used() const {
		return locations.size() > 0;
	}

	asset_gui_path_entry() = default;
	asset_gui_path_entry(
		augs::path_type from,
	   	const id_type id
	) :
		id(id),
		base(from)
	{}
};

template <class T>
constexpr bool is_whitelisted_container = 
	is_one_of_v<T, wandering_pixels_frames>
;

template <class Se, class O, class F>
void find_object_in_object(
	const Se& searched_object,
	const O& in_object,
	F location_callback
) {
	using S = std::decay_t<Se>;

	static_assert(std::is_same_v<S, decltype(invariants::sprite::tex)>);

	thread_local augs::field_name_tracker fields;
	fields.clear();

	auto callback = augs::recursive(
		[&searched_object, &location_callback](auto&& self, const auto& label, auto& field) {
			using T = std::decay_t<decltype(field)>;

			if constexpr(
				is_one_of_v<T, all_logical_assets, all_entity_flavours>
			) {
				/* This has a special logic */
			}
			else if constexpr(std::is_same_v<T, S>) {
				if (searched_object == field) {
					location_callback(fields.get_full_name(label));
				}
			}
			else if constexpr(is_introspective_leaf_v<T>) {
				return;
			}
			else if constexpr(augs::has_dynamic_content_v<T>) {
				if constexpr(!(is_container_v<T> && !is_whitelisted_container<T>)) {	
					/* Is not a blacklisted container */

					augs::on_dynamic_content(
						[&](auto& dyn, auto... args) {
							auto scope = fields.track(typesafe_sprintf("%x", args...));
							self(self, "", dyn);
						},
						field
					);
				}
				else {
					validate_fields_in(field, [](const auto& m){
						using M = std::decay_t<decltype(m)>;
						static_assert(!std::is_same_v<M, S>, "A blacklisted container may contain the searched object.");
					});
				}
			}
			else {
				auto scope = fields.track(label);
				augs::introspect(augs::recursive(self), field);
			}
		}
	);

	augs::introspect(callback, in_object);
}

template <class F>
void find_locations_that_use(
	const assets::image_id id,
	const intercosm& inter,
	F location_callback
) {
	auto traverse = [&](const std::string& preffix, const auto& object) {
		find_object_in_object(id, object, [&](const auto& location) {
			location_callback(preffix + location);
		});
	};

	const auto& common = inter.world.get_common_significant();

	traverse("Common: ", common);

	for_each_entity_type([&](auto e){ 
		using E = decltype(e);

		common.get_flavours<E>().for_each([&](const auto, const auto& flavour) {
			const auto& name = flavour.template get<invariants::name>().name;

			auto for_each_through = [&](const auto& where) {
				for_each_through_std_get(
					where,
					[&](const auto& c) {
						find_object_in_object(id, c, [&](const auto& location) {
							/* location_callback(format_struct_name(c) + "of " + name + ": " + location); */
							location_callback("Flavour: " + name + " (" + format_struct_name(c) + "." + location + ")");
						});
					}
				);
			};

			for_each_through(flavour.initial_components);
			for_each_through(flavour.invariants);
		});
	});

	//traverse("Particle effects: ", inter.viewables.particle_effects);
}

void editor_images_gui::perform(editor_command_input in) {
	using namespace augs::imgui;

	auto window = make_scoped_window();

	if (!window) {
		return;
	}

	auto& work = *in.folder.work;
	auto& cosm = work.world;

	const auto& viewables = work.viewables;

	using asset_id_type = assets::image_id;
	using path_entry_type = asset_gui_path_entry<asset_id_type>;
	thread_local std::vector<path_entry_type> all_paths;

	all_paths.clear();

	viewables.image_loadables.for_each_object_and_id(
		[&](const auto& object, const auto id) mutable {
			all_paths.emplace_back(object.source_image_path, id);
		}
	);

	sort_range(all_paths);
	
	for (auto& p : all_paths) {
		find_locations_that_use(p.id, work, [&](const auto& location) {
			p.locations.push_back(location);
		});
	}

	browser_settings.do_tweakers();

	thread_local ImGuiTextFilter filter;
	filter.Draw();

	acquire_keyboard_once();

	auto files_view = scoped_child("Files view");

	auto& tree_settings = browser_settings.tree_settings;

	if (tree_settings.linear_view) {
		ImGui::Columns(3);

		int i = 0;

		auto do_path = [&](const auto& path_entry) {
			auto scope = scoped_id(i++);

			const auto displayed_name = tree_settings.get_prettified(path_entry.get_filename());
			const auto displayed_dir = path_entry.get_displayed_directory();

			const auto id = path_entry.id;

			if (!filter.PassFilter(displayed_name.c_str()) && !filter.PassFilter(displayed_dir.c_str())) {
				return;
			}

			{
				if (auto node = scoped_tree_node(displayed_name.c_str())) {

				}
			}

			ImGui::NextColumn();

			text_disabled(displayed_dir);

			ImGui::NextColumn();

			if (path_entry.used()) {
				const auto& locations = path_entry.locations;

				if (auto node = scoped_tree_node(typesafe_sprintf("%x locations###locations", locations.size()).c_str())) {
					for (const auto& l : locations) {
						text(l);
					}
				}
			}
			else {
				const auto scoped_style = scoped_style_var(ImGuiStyleVar_FramePadding, ImVec2(3, 0));

				if (ImGui::Button("Forget")) {
					forget_asset_id_command<asset_id_type> cmd;
					cmd.forgotten_id = path_entry.id;
					cmd.built_description = 
						typesafe_sprintf("Stopped tracking %x", augs::to_display_path(path_entry.get_full_path()))
					;

					in.folder.history.execute_new(std::move(cmd), in);
				}
			}

			ImGui::NextColumn();
		};

		bool any_unused = false;

		if (browser_settings.show_unused) {
			for (const auto& u : all_paths) {
				if (!any_unused) {
					text_disabled("Unused images");
					ImGui::NextColumn();
					text_disabled("Location");
					ImGui::NextColumn();
					text_disabled("Operations");
					ImGui::NextColumn();
					ImGui::Separator();

					any_unused = true;
				}
				if (!u.used()) {
					do_path(u);
				}
			}
		}

		if (any_unused) {
			ImGui::Separator();
		}

		tree_settings.do_name_location_columns();

		text_disabled("Used at");
		ImGui::NextColumn();

		ImGui::Separator();

		for (const auto& p : all_paths) {
			if (p.used()) {
				do_path(p);
			}
		}
	}
	else {

	}
}
