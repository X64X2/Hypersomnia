#include <thread>
#include "game/bindings/bind_game_and_augs.h"
#include "augs/global_libraries.h"
#include "application/game_window.h"

#include "game/resources/manager.h"

#include "game/scene_managers/testbed.h"
#include "game/scene_managers/resource_setups/all.h"

#include "game/transcendental/types_specification/all_component_includes.h"
#include "game/transcendental/types_specification/all_messages_includes.h"
#include "game/transcendental/viewing_session.h"
#include "game/transcendental/step_packaged_for_network.h"
#include "game/transcendental/cosmos.h"
#include "game/transcendental/data_living_one_step.h"

#include "augs/misc/debug_entropy_player.h"
#include "game/transcendental/step.h"

#include "augs/filesystem/file.h"
#include "determinism_test_setup.h"

#include "augs/misc/templated_readwrite.h"

void determinism_test_setup::process(
	const config_lua_table& cfg, 
	game_window& window
) {
	const vec2i screen_size = vec2i(window.get_screen_size());

	const unsigned cosmoi_count = 1 + cfg.determinism_test_cloned_cosmoi_count;
	std::vector<cosmos> hypersomnias(cosmoi_count, cosmos(3000));

	cosmic_entropy total_collected_entropy;
	augs::debug_entropy_player<cosmic_entropy> player;
	augs::fixed_delta_timer timer = augs::fixed_delta_timer(5);
	std::vector<scene_managers::testbed> testbeds(cosmoi_count);

	if (augs::file_exists("save.state")) {
		for (auto& h : hypersomnias) {
			ensure(h.load_from_file("save.state"));
		}
	}
	else {
		for (size_t i = 0; i < cosmoi_count; ++i) {
			hypersomnias[i].set_fixed_delta(cfg.tickrate);
			testbeds[i].populate_world_with_entities(hypersomnias[i], vec2i(1920, 1080));
		}
	}

	for (auto& h : hypersomnias) {
		ensure(h == hypersomnias[0]);
	}

	if (cfg.get_input_recording_mode() != input_recording_type::DISABLED) {
		if (player.try_to_load_or_save_new_session("sessions/", "recorded.inputs")) {
			timer.set_stepping_speed_multiplier(cfg.recording_replay_speed);
		}
	}

	viewing_session session;
	session.reserve_caches_for_entities(3000);
	session.camera.configure_size(screen_size);
	session.set_interpolation_enabled(false);
	session.set_master_gain(cfg.sound_effects_volume);
	session.configure_input();

	unsigned currently_viewn_cosmos = 0;
	bool divergence_detected = false;
	unsigned which_divergent = 0;

	timer.reset_timer();

	while (!should_quit) {
		augs::machine_entropy new_machine_entropy;

		new_machine_entropy.local = window.collect_entropy(!cfg.debug_disable_cursor_clipping);
		
		if (process_exit_key(new_machine_entropy.local)) {
			break;
		}

		for (auto& n : new_machine_entropy.local) {
			if (n.was_any_key_pressed()) {
				if (n.key == augs::window::event::keys::key::F3) {
					++currently_viewn_cosmos;
					currently_viewn_cosmos %= cosmoi_count;
				}
			}
		}

		session.switch_between_gui_and_back(new_machine_entropy.local);

		session.control_gui_and_remove_fetched_events(
			hypersomnias[0][testbeds[0].get_selected_character()],
			new_machine_entropy.local
		);

		auto new_intents = session.context.to_key_and_mouse_intents(new_machine_entropy.local);

		session.control_and_remove_fetched_intents(new_intents);

		auto new_cosmic_entropy = cosmic_entropy(
			hypersomnias[0][testbeds[0].get_selected_character()],
			new_intents
		);

		new_cosmic_entropy += session.systems_audiovisual.get<gui_element_system>().get_and_clear_pending_events();

		total_collected_entropy += new_cosmic_entropy;

		auto steps = timer.count_logic_steps_to_perform(hypersomnias[0].get_fixed_delta());

		while (steps--) {
			if (divergence_detected) {
				break;
			}

			for (size_t i = 0; i < cosmoi_count; ++i) {
				auto& h = hypersomnias[i];
				
				if (i + 1 < cosmoi_count) {
					hypersomnias[i] = hypersomnias[i + 1];
				}

				testbeds[i].control_character_selection(new_intents);

				player.advance_player_and_biserialize(total_collected_entropy);

				augs::renderer::get_current().clear_logic_lines();

				h.advance_deterministic_schemata(total_collected_entropy, [](auto) {},
					[this, &session](const const_logic_step step) {
						session.spread_past_infection(step);
					}
				);
			}

			session.resample_state_for_audiovisuals(hypersomnias[0]);

			auto& first_cosm = hypersomnias[0].reserved_memory_for_serialization;

			augs::output_stream_reserver first_cosm_reserver;
			augs::write_object(first_cosm_reserver, hypersomnias[0].significant);
			first_cosm.reserve(first_cosm_reserver.get_write_pos());
			first_cosm.reset_write_pos();
			augs::write_object(first_cosm, hypersomnias[0].significant);

			for (unsigned i = 1; i < cosmoi_count; ++i) {
				auto& second_cosm = hypersomnias[i].reserved_memory_for_serialization;

				augs::output_stream_reserver second_cosm_reserver;
				augs::write_object(second_cosm_reserver, hypersomnias[i].significant);
				second_cosm.reserve(second_cosm_reserver.get_write_pos());
				second_cosm.reset_write_pos();
				augs::write_object(second_cosm, hypersomnias[i].significant);

				if (!(first_cosm == second_cosm)) {
					divergence_detected = true;
					which_divergent = i;
					break;
				}
			}

			total_collected_entropy = cosmic_entropy();
		}

		std::string logged;

		if (divergence_detected) {
			logged += typesafe_sprintf("Divergence detected in cosmos: %x (step: %x)\n", which_divergent, hypersomnias[0].get_total_steps_passed());
		}

		logged += typesafe_sprintf("Currently viewn cosmos: %x (F3 to switch)\n", currently_viewn_cosmos);

		auto& renderer = augs::renderer::get_current();
		renderer.clear_current_fbo();

		session.view(
			cfg,
			renderer, 
			hypersomnias[currently_viewn_cosmos], 
			testbeds[currently_viewn_cosmos].get_selected_character(), 
			session.frame_timer.extract_variable_delta(hypersomnias[currently_viewn_cosmos].get_fixed_delta(), timer)
		);
	
		window.swap_buffers();
	}
}