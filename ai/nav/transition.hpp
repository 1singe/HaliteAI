#pragma once

#include "node.hpp"

#include <memory>

namespace ai {
	namespace nav {

		struct Transition {
			const Node from;
			const Node to;
			float cost = 0;
			bool enabled = true;

			bool operator==(const Transition& other) const;
			bool operator<(const Transition& a) const;
		};
	}
}