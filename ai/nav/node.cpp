#include "node.hpp"

ai::nav::Node::Node(float x, float y) : m_x(x), m_y(y) {
}

ai::nav::Node::Node(const Node& kr_other) : m_x(kr_other.m_x), m_y(kr_other.m_y) {
}

bool ai::nav::Node::operator==(const Node& kr_other) const {
	return (m_x == kr_other.m_x && m_y == kr_other.m_y);
}

bool ai::nav::Node::operator!=(const Node& kr_other) const {
	return (m_x != kr_other.m_x || m_y != kr_other.m_y);
}

bool ai::nav::Node::operator<(const Node& kr_other) const {
	return (std::tie(m_x, m_y) < std::tie(kr_other.m_x, kr_other.m_y));
}

float ai::nav::Node::getX() const {
	return m_x;
};

float ai::nav::Node::getY() const {
	return m_y;
};