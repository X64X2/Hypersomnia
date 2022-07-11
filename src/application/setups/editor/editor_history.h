#pragma once
#include <vector>
#include <variant>

#include "augs/templates/history.h"

#include "application/setups/editor/commands/editor_command_meta.h"

#include "application/setups/editor/commands/edit_resource_command.h"
#include "application/setups/editor/commands/edit_node_command.h"
#include "application/setups/editor/commands/create_layer_command.h"
#include "application/setups/editor/commands/create_node_command.h"

#include "application/setups/editor/editor_history_declaration.h"

struct editor_history : public editor_history_base {
	using base = editor_history_base;
	using base::base;
};
