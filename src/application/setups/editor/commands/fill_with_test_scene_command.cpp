#include "game/cosmos/entity_handle.h"

#include "application/intercosm.h"
#include "application/setups/editor/editor_command_input.h"
#include "application/setups/editor/editor_folder.h"

#include "application/setups/editor/commands/fill_with_test_scene_command.h"

#include "test_scenes/test_scene_settings.h"

#include "augs/readwrite/byte_readwrite.h"
#include "augs/readwrite/memory_stream.h"
#include "application/setups/editor/editor_settings.h"

std::string fill_with_test_scene_command::describe() const {
	return typesafe_sprintf("Filled with %x", minimal ? "minimal test scene" : "test scene");
}

void fill_with_test_scene_command::redo(const editor_command_input in) {
	in.purge_selections();

	auto& work = in.folder.work;
	auto& view = in.folder.view;

	auto& mode_vars = in.folder.mode_vars;
	auto& player = in.folder.player;

	work->to_bytes(intercosm_before_fill);
	view_before_fill = augs::to_bytes(view);
	modes_before_fill = augs::to_bytes(mode_vars);
	player_before_fill = augs::to_bytes(player);

	test_scene_mode_vars test_vars;
	bomb_mode_vars bomb_vars;
	bomb_vars.warmup_secs = 7;

	const auto& settings = in.settings.test_scene;

#if BUILD_TEST_SCENES
	work->make_test_scene(in.lua, { minimal, settings.scene_tickrate }, test_vars, std::addressof(bomb_vars));
#endif
	view = {};

	auto& cosm = work->world;

	mode_vars.clear();

	{
		const auto test_vars_id = mode_vars_id(0);
		mode_vars.get_for<test_scene_mode_vars>().try_emplace(test_vars_id, std::move(test_vars));

		if (!settings.start_bomb_mode) {
			test_scene_mode mode;
			view.local_player_id = mode.add_player({ test_vars, cosm }, faction_type::RESISTANCE);

			player.current_mode_vars_id = test_vars_id;
			player.current_mode.emplace<test_scene_mode>(std::move(mode));
		}
	}

	{
		const auto bomb_vars_id = mode_vars_id(0);
		mode_vars.get_for<bomb_mode_vars>().try_emplace(bomb_vars_id, std::move(bomb_vars));

		if (settings.start_bomb_mode) {
			bomb_mode mode;

			auto& player_id = view.local_player_id;

			{
				const auto in = bomb_mode::input { bomb_vars, cosm.get_solvable().significant, cosm };
				player_id = mode.add_player(in, "Editor-player");
			}

			//mode.auto_assign_faction(cosm, player_id);
			mode.choose_faction(player_id, faction_type::RESISTANCE);

			player.current_mode_vars_id = bomb_vars_id;
			player.current_mode.emplace<bomb_mode>(std::move(mode));
		}
	}

	player.mode_initial_signi = std::make_unique<cosmos_solvable_significant>(work->world.get_solvable().significant);
}

void fill_with_test_scene_command::undo(const editor_command_input in) {
	in.purge_selections();

	auto& work = in.folder.work;
	work->from_bytes(intercosm_before_fill);
	augs::from_bytes(view_before_fill, in.folder.view);
	augs::from_bytes(modes_before_fill, in.folder.mode_vars);
	augs::from_bytes(player_before_fill, in.folder.player);

	intercosm_before_fill.clear();
	view_before_fill.clear();
}
