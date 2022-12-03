#pragma once

#include <tuple>

namespace ai {
	namespace nav {

		class Node {
		public:
			Node(float x, float y);
			Node(const Node& kr_other);

			bool operator==(const Node& kr_other) const;
			bool operator!=(const Node& kr_other) const;
			bool operator<(const Node& kr_other) const;

			float getX() const;
			float getY() const;

		private:
			float m_x;
			float m_y;
		};
	}
}