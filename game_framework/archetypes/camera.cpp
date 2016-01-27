#include "archetypes.h"
#include "entity_system/entity.h"
#include "entity_system/world.h"

#include "game_framework/components/chase_component.h"
#include "game_framework/components/camera_component.h"
#include "game_framework/components/input_component.h"

namespace archetypes {
	void camera(augs::entity_id e, int w, int h) {
		components::transform transform;
		components::input input;
		components::camera camera;
		components::chase chase;

		input.add(messages::intent_message::SWITCH_LOOK);
		input.add(messages::intent_message::ZOOM_CAMERA);

		camera.enable_smoothing = true;
		camera.mask = components::render::WORLD;
		camera.smoothing_average_factor = 0.5;
		camera.averages_per_sec = 25;

		camera.orbit_mode = camera.LOOK;
		camera.max_look_expand.set(w / 2, h / 2);
		camera.angled_look_length = 10;

		camera.viewport.set(0, 0, w, h);
		camera.visible_world_area.set(w, h);

		chase.relative = false;

		e->add(transform);
		e->add(input);
		e->add(camera);
		e->add(chase);
	}
}