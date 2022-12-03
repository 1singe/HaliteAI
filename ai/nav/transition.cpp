#include "transition.hpp"

bool ai::nav::Transition::operator==(const Transition& kr_other) const {
	return (from == kr_other.from && to == kr_other.to);
}

bool ai::nav::Transition::operator<(const Transition& kr_other) const {
	return (std::tie(from, to) < std::tie(kr_other.from, kr_other.to));
}