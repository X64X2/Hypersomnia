#pragma once
#include "game/inferred_caches/physics_world_cache.h"
#include "augs/misc/convex_partitioned_shape.hpp"
#include "game/enums/filters.h"

template <class E>
auto calc_filters(const E& handle) {
	const auto& colliders_data = handle.template get<invariants::fixtures>();

	if (handle.is_like_planted_bomb()) {
		return filters[predefined_filter_type::PLANTED_EXPLOSIVE];
	}

	if (handle.is_like_thrown_explosive()) {
		return filters[predefined_filter_type::FLYING_ITEM];
	}

	return colliders_data.filter;
}

template <class E>
void physics_world_cache::infer_rigid_body_existence(const E& typed_handle) {
	if (const auto cache = find_rigid_body_cache(typed_handle)) {
		return;
	}

	infer_rigid_body(typed_handle);
}

template <class E>
void physics_world_cache::specific_infer_cache_for(const E& typed_handle) {
	if constexpr(E::template has<invariants::rigid_body>()) {
		specific_infer_rigid_body(typed_handle);
	}

	if constexpr(E::template has<invariants::fixtures>()) {
		specific_infer_colliders(typed_handle);
	}

#if TODO_JOINTS
	if constexpr() {
		specific_infer_joint(typed_handle);
	}
#endif
}

template <class E>
void physics_world_cache::specific_infer_rigid_body(const E& handle) {
	const auto it = rigid_body_caches.try_emplace(unversioned_entity_id(handle));
	auto& cache = (*it.first).second;

	const auto& physics_def = handle.template get<invariants::rigid_body>();

	auto to_b2Body_type = [](const rigid_body_type t) {
		switch (t) {
			case rigid_body_type::DYNAMIC: return b2BodyType::b2_dynamicBody;
			case rigid_body_type::STATIC: return b2BodyType::b2_staticBody;
			case rigid_body_type::KINEMATIC: return b2BodyType::b2_kinematicBody;
			default: return b2BodyType::b2_staticBody;
		}
	};

	const auto body_type = [&]() {
		return handle.is_like_planted_bomb() ? rigid_body_type::STATIC : physics_def.body_type;
	}();

	if (!it.second) {
		/* The cache already existed. */
		auto& body = *cache.body;

		bool only_update_properties = true;

		if (to_b2Body_type(body_type) != body.GetType()) {
			only_update_properties = false;
		}

		if (only_update_properties) {
			/* 
				Invariant/component guaranteed to exist because it must have once been created from an existing def,
				and changing type content implies reinference of the entire cosm.
			*/
	
			const auto& def = handle.template get<invariants::rigid_body>();
			const auto rigid_body = handle.template get<components::rigid_body>();
			const auto damping = rigid_body.calc_damping_mults(def);
			const auto& data = rigid_body.get_raw_component();
	
			/* 
				Currently, nothing that can change inside the component could possibly trigger the need to rebuild the body.
				This may change once we want to delete bodies without fixtures.
			*/
	
			/* These have no side-effects */
			body.SetLinearDamping(damping.linear);
			body.SetAngularDamping(damping.angular);
			body.SetLinearDampingVec(b2Vec2(damping.linear_axis_aligned));
			body.SetAngledDampingEnabled(def.angled_damping);
	
			if (handle.template has<components::missile>()) {
				body.SetFixedRotation(true);
			}

			/* These have side-effects, thus we guard */
			if (body.IsSleepingAllowed() != def.allow_sleep) {
				body.SetSleepingAllowed(def.allow_sleep);
			}
	
			if (body.GetLinearVelocity() != b2Vec2(data.velocity)) {
				body.SetLinearVelocity(b2Vec2(data.velocity));
			}
	
			if (body.GetAngularVelocity() != data.angular_velocity) {
				body.SetAngularVelocity(data.angular_velocity);
			}
	
			if (!(body.m_xf == data.physics_transforms.m_xf)) {
				body.m_xf = data.physics_transforms.m_xf;
				body.m_sweep = data.physics_transforms.m_sweep;
	
				b2BroadPhase* broadPhase = &body.m_world->m_contactManager.m_broadPhase;
	
				for (b2Fixture* f = body.m_fixtureList; f; f = f->m_next)
				{
					f->Synchronize(broadPhase, body.m_xf, body.m_xf);
				}
			}
	
			return;
		}
	}

	/*
		Here the cache is not constructed so we rebuild from scratch.
	*/
	cache.clear(*this);

	const auto rigid_body = handle.template get<components::rigid_body>();
	const auto& physics_data = rigid_body.get_raw_component();

	b2BodyDef def;
	def.type = to_b2Body_type(body_type);

	def.userData = unversioned_entity_id(handle);

	def.bullet = physics_def.bullet;
	def.allowSleep = physics_def.allow_sleep;

	const auto damping = rigid_body.calc_damping_mults(physics_def);

	def.angularDamping = damping.angular;
	def.linearDamping = damping.linear;

	def.transform = physics_data.physics_transforms.m_xf;
	def.sweep = physics_data.physics_transforms.m_sweep;

	def.linearVelocity = b2Vec2(physics_data.velocity);
	def.angularVelocity = physics_data.angular_velocity;

	if (handle.template has<components::missile>()) {
		def.fixedRotation = true;
	}

	def.active = true;

	cache.body = b2world->CreateBody(&def);

	cache.body->SetAngledDampingEnabled(physics_def.angled_damping);
	cache.body->SetLinearDampingVec(b2Vec2(damping.linear_axis_aligned));

	/*
		All colliders caches, before their own inference,
		manually infer the existence of the rigid body.

		Thus the rigid body, on its own inference, does not have to inform all fixtures
		about that it has just come into existence.
	*/
}

template <class E>
void physics_world_cache::specific_infer_colliders_from_scratch(const E& handle, const colliders_connection& connection) {
	const auto& cosm = handle.get_cosmos();

	const auto it = colliders_caches.try_emplace(handle.get_id().to_unversioned());

	auto& cache = (*it.first).second;
	cache.clear(*this);

	const auto new_owner = cosm[connection.owner];

	if (new_owner.dead()) {
		colliders_caches.erase(it.first);
		return;
	}

	infer_rigid_body_existence(new_owner);

	const auto body_cache = find_rigid_body_cache(new_owner);

	if (!body_cache) {
		/* 
			No body to attach to. 
			Might happen if we once implement it that the logic deactivates bodies for some reason. 
			Or, if collider owner calculation returns incorrectly an entity without rigid body component.
		*/

		return;
	}

	const auto si = handle.get_cosmos().get_si();
	auto& owner_b2Body = *body_cache->body.get();

	const auto& colliders_data = handle.template get<invariants::fixtures>(); 

	b2FixtureDef fixdef;

	fixdef.userData = handle;

	fixdef.density = handle.calc_density(connection, colliders_data);

	fixdef.friction = colliders_data.friction;
	fixdef.restitution = colliders_data.restitution;
	fixdef.isSensor = colliders_data.sensor;
	fixdef.filter = calc_filters(handle);

	cache.connection = connection;

	auto& constructed_fixtures = cache.constructed_fixtures;
	ensure(constructed_fixtures.empty());

	const auto flips = handle.calc_flip_flags();
	const bool flip_order = flips && flips->vertically != flips->horizontally;

	auto from_convex_partition = [&](auto shape) {
		shape.offset_vertices(connection.shape_offset);

		if (flips) {
			if (flips->horizontally) {
				for (auto& v : shape.original_poly) {
					v.neg_x();
				}
			}

			if (flips->vertically) {
				for (auto& v : shape.original_poly) {
					v.neg_y();
				}
			}
		}

		unsigned ci = 0;

		auto add_convex = [&](const auto& convex) {
			augs::constant_size_vector<b2Vec2, decltype(logic_convex_poly::original_poly)::max_size()> b2verts(
				convex.begin(), 
				convex.end()
			);

			if (flip_order) {
				reverse_range(b2verts);
			}

			for (auto& v : b2verts) {
				v = si.get_meters(v);
			}

			b2PolygonShape ps;
			ps.Set(b2verts.data(), static_cast<int32>(b2verts.size()));

			fixdef.shape = &ps;
			b2Fixture* const new_fix = owner_b2Body.CreateFixture(&fixdef);

			ensure(static_cast<short>(ci) < std::numeric_limits<short>::max());
			new_fix->index_in_component = static_cast<short>(ci++);

			constructed_fixtures.emplace_back(new_fix);
		};

		shape.for_each_convex(add_convex);
	};

	auto from_circle_shape = [&](const real32 radius) {
		b2CircleShape shape;
		shape.m_radius = si.get_meters(radius);
		shape.m_p += b2Vec2(connection.shape_offset.pos);

		fixdef.shape = &shape;
		b2Fixture* const new_fix = owner_b2Body.CreateFixture(&fixdef);

		new_fix->index_in_component = 0u;
		constructed_fixtures.emplace_back(new_fix);
	};

	if (const auto* const shape_polygon = handle.template find<invariants::shape_polygon>()) {
		from_convex_partition(shape_polygon->shape);
		return;
	}

	if (const auto shape_circle = handle.template find<invariants::shape_circle>()) {
		from_circle_shape(shape_circle->get_radius());
		return;
	}

	if (!handle.is_like_planted_bomb() && handle.is_like_thrown_explosive()) {
		if (const auto fuse = handle.template find<invariants::hand_fuse>()) {
			from_circle_shape(fuse->circle_shape_radius_when_released);
		}

		return;
	}

	if (const auto expl_body = handle.template find<invariants::cascade_explosion>()) {
		from_circle_shape(expl_body->circle_collider_radius);
		return;
	}

	if (const auto* const sprite = handle.template find<invariants::sprite>()) {
		const auto& offsets = cosm.get_logical_assets().get_offsets(sprite->image_id);

		if (const auto& shape = offsets.non_standard_shape; !shape.empty()) {
			from_convex_partition(shape);
		}
		else {
			auto size = handle.get_logical_size();

			size.x = std::max(1.f, size.x);
			size.y = std::max(1.f, size.y);

			size = si.get_meters(size);

			b2PolygonShape ps;
			ps.SetAsBox(size.x * 0.5f, size.y * 0.5f);

			const auto off_meters = si.get_meters(connection.shape_offset.pos);

			for (int i = 0; i < 4; ++i) {
				vec2 out = ps.m_vertices[i];

				out.rotate(connection.shape_offset.rotation);
				out += off_meters;

				ps.m_vertices[i] = b2Vec2(out);
			}

			fixdef.shape = &ps;
			b2Fixture* const new_fix = owner_b2Body.CreateFixture(&fixdef);

			new_fix->index_in_component = 0;

			constructed_fixtures.emplace_back(new_fix);
		}

		return;
	}

	ensure(false && "fixtures requested with no shape attached!");
}


template <class E>
void physics_world_cache::specific_infer_colliders(const E& handle) {
	std::optional<colliders_connection> calculated_connection;

	auto get_calculated_connection = [&](){
		if (!calculated_connection) {
			if (const auto connection = handle.calc_colliders_connection()) {
				calculated_connection = connection; 
			}
			else {
				/* A default with unset owner body will prevent cache from building */
				calculated_connection = colliders_connection();
			}
		}

		return *calculated_connection;
	};

	const auto it = colliders_caches.try_emplace(handle.get_id().to_unversioned());
	auto& cache = (*it.first).second;

	if (!it.second) {
		/* Cache already existed. */
		bool only_update_properties = true;
		
		if (get_calculated_connection() != cache.connection) {
			only_update_properties = false;
		}

		if (cache.constructed_fixtures.empty()) {
			only_update_properties = false;
		}

		if (only_update_properties) {
			auto& compared = *cache.constructed_fixtures[0].get();
			const auto& colliders_data = handle.template get<invariants::fixtures>();

			if (const auto new_density = handle.calc_density(
					get_calculated_connection(), 
					colliders_data
				);

				compared.GetDensity() != new_density
			) {
				for (auto& f : cache.constructed_fixtures) {
					f.get()->SetDensity(new_density);
				}

				compared.GetBody()->ResetMassData();
			}

			const auto chosen_filters = calc_filters(handle);
			const bool rebuild_filters = compared.GetFilterData() != chosen_filters;

			for (auto& f : cache.constructed_fixtures) {
				f.get()->SetRestitution(colliders_data.restitution);
				f.get()->SetFriction(colliders_data.friction);
				f.get()->SetSensor(colliders_data.sensor);

				if (rebuild_filters) {
					f.get()->SetFilterData(chosen_filters);
				}
			}
			
			return;
		}
	}

	specific_infer_colliders_from_scratch(handle, get_calculated_connection());
}

template <class E>
void physics_world_cache::specific_infer_joint(const E& /* handle */) {
#if TODO_JOINTS
	const auto& cosm = handle.get_cosmos();

	if (const auto motor_joint = handle.find<components::motor_joint>();

		motor_joint != nullptr
		&& rigid_body_cache_exists_for(cosm[motor_joint.get_target_bodies()[0]])
		&& rigid_body_cache_exists_for(cosm[motor_joint.get_target_bodies()[1]])
	) {
		const components::motor_joint joint_data = motor_joint.get_raw_component();

		const auto si = handle.get_cosmos().get_si();
		auto cache = find_joint_cache(handle);
		
		ensure_eq(nullptr, cache->joint);

		b2MotorJointDef def;
		def.userData = handle.get_id();
		def.bodyA = find_rigid_body_cache(cosm[joint_data.target_bodies[0]]).body;
		def.bodyB = find_rigid_body_cache(cosm[joint_data.target_bodies[1]]).body;
		def.collideConnected = joint_data.collide_connected;
		def.linearOffset = b2Vec2(si.get_meters(joint_data.linear_offset));
		def.angularOffset = DEG_TO_RAD<float> * joint_data.angular_offset;
		def.maxForce = si.get_meters(joint_data.max_force);
		def.maxTorque = joint_data.max_torque;
		def.correctionFactor = joint_data.correction_factor;

		cache->joint = b2world->CreateJoint(&def);
	}
#endif
}