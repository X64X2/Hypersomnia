#pragma once
#include "augs/math/vec2.h"
#include "augs/misc/minmax.h"
#include "augs/pad_bytes.h"

#include "game/transcendental/entity_flavour_id.h"

struct fast_randomization;

namespace invariants {
	struct trace;
}

namespace components {
	struct trace {
		// GEN INTROSPECTOR struct components::trace
		vec2 chosen_multiplier = vec2(-1.f, -1.f);
		float chosen_lengthening_duration_ms = -1.f;
		float lengthening_time_passed_ms = 0.f;

		vec2 last_size_mult;
		vec2 last_center_offset_mult;

		bool is_it_a_finishing_trace = false;
		pad_bytes<3> pad;
		// END GEN INTROSPECTOR

		void reset(
			const invariants::trace&,
			fast_randomization& p
		);
	};
}

namespace invariants {
	struct trace {
		using implied_component = components::trace;
		using minmax = augs::minmax<float>;

		// GEN INTROSPECTOR struct invariants::trace
		minmax max_multiplier_x = minmax(1.f, 1.f);
		minmax max_multiplier_y = minmax(1.f, 1.f);

		vec2 additional_multiplier;

		minmax lengthening_duration_ms = minmax(200.f, 400.f);

		entity_flavour_id finishing_trace_flavour = 0;

		// END GEN INTROSPECTOR
	};
}