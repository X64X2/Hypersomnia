#pragma once
#include "simulation_exchange.h"

namespace augs {
	namespace network {
		class client;
	}
}

class simulation_receiver : public simulation_exchange {
public:
	augs::jitter_buffer<packaged_step> jitter_buffer;
	std::vector<guid_mapped_entropy> predicted_steps;

	void read_entropy_for_next_step(augs::stream&, bool skip_command);
	void read_entropy_with_heartbeat_for_next_step(augs::stream&, bool skip_command);

	struct unpacking_result {
		bool use_extrapolated_cosmos = true;
	};

	unsigned current_steps_to_the_future = 0;

	void delay_sent_commands(unsigned steps_to_the_future) {

		current_steps_to_the_future += steps_to_the_future;
	}

	void catch_up_commands(unsigned steps_to_return) {

		current_steps_to_the_future -= steps_to_return;
	}

	template<class Step>
	void send_commands_and_predict(augs::network::client& client, cosmic_entropy new_entropy, cosmos& predicted_cosmos, Step advance) {
		augs::stream client_commands;
		augs::write_object(client_commands, network_command::CLIENT_REQUESTED_ENTROPY);

		guid_mapped_entropy guid_mapped(new_entropy, predicted_cosmos);
		augs::write_object(client_commands, guid_mapped);

		client.post_redundant(client_commands);
		client.send_pending_redundant();

		predicted_steps.push_back(guid_mapped);
		advance(new_entropy, predicted_cosmos);
	}

	template<class Step>
	unpacking_result unpack_deterministic_steps(cosmos& referential_cosmos, cosmos& last_delta_unpacked, cosmos& predicted_cosmos, Step advance) {
		unpacking_result result;

		auto new_commands = jitter_buffer.buffer;
		jitter_buffer.buffer.clear();

		result.use_extrapolated_cosmos = true;

		struct step_to_simulate {
			bool resubstantiate = false;
			guid_mapped_entropy entropy;
		};

		std::vector<step_to_simulate> entropies_to_simulate;

		bool reconciliate_predicted = false;

		for (size_t i = 0; i < new_commands.size(); ++i) {
			auto& new_command = new_commands[i];

			if (new_command.step_type == packaged_step::type::NEW_ENTROPY_WITH_HEARTBEAT) {
				cosmic_delta::decode(last_delta_unpacked, new_command.delta);
				referential_cosmos = last_delta_unpacked;

				entropies_to_simulate.clear();
				reconciliate_predicted = true;
			}
			else
				ensure(new_command.step_type == packaged_step::type::NEW_ENTROPY);

			step_to_simulate sim;
			sim.resubstantiate = new_command.shall_resubstantiate;
			sim.entropy = new_command.entropy;

			entropies_to_simulate.emplace_back(sim);

			if (new_command.next_client_commands_accepted) {
				ensure(predicted_steps.size() > 0);
				
				if (!current_steps_to_the_future) {
					
				}

				if (sim.resubstantiate || predicted_steps.front() != sim.entropy) {
					reconciliate_predicted = true;
				}

				predicted_steps.erase(predicted_steps.begin());
			}
			else {
				reconciliate_predicted = true;
			}
		}

		const unsigned previous_step = referential_cosmos.get_total_steps_passed();

		for (const auto& e : entropies_to_simulate) {
			const cosmic_entropy cosmic_entropy_for_this_step(e.entropy, referential_cosmos);
			
			if (e.resubstantiate)
				referential_cosmos.complete_resubstantiation();

			advance(cosmic_entropy_for_this_step, referential_cosmos);
		}
		
		// LOG("Unpacking from %x to %x", previous_step, referential_cosmos.get_total_steps_passed());

		if (reconciliate_predicted) {
			unsigned predicted_was_at_step = predicted_cosmos.get_total_steps_passed();

			predicted_cosmos = referential_cosmos;

			for (const auto& s : predicted_steps) {
				advance(cosmic_entropy(s, predicted_cosmos), predicted_cosmos);
			}

			//ensure(predicted_cosmos.get_total_steps_passed() == predicted_was_at_step);

			//while (predicted_cosmos.get_total_steps_passed() < predicted_was_at_step) {
			//
			//}
		}

		return std::move(result);
	}
};