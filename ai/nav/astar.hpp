#pragma once

#include "transition.hpp"

#include <queue>
#include <map>
#include <set>
#include <functional>
#include <limits>
#include <iostream>

namespace ai {
	namespace nav {

		struct PathData {
			std::deque<Node> path;
			float finalCost;
			bool goesToPlannedDestination;
		};

		struct ToroidalData {
			float width;
			float height;

			bool operator!=(const ToroidalData& kr_other) {
				return width != kr_other.width || height != kr_other.height;
			}
		};

		enum class AstarHeuristic { EUCLIDEAN_DISTANCE, MANHATTAN_DISTANCE };

		class Astar {
		public:
			//build the astar graph from its transtions, an heuristic mode and with optionnal toroidal informations
			Astar(const std::set<Transition>& kr_transitions, AstarHeuristic heuristic = AstarHeuristic::EUCLIDEAN_DISTANCE, ToroidalData toroidalData = {0,0});

			/*give the path from kr_from to kr_to
			*params:
			*    -overridenTransitionCost: set this value to a number bigger than 0 to use this new value as the new cost of every transition
			*	 -useDisabledTransitions: set this value to true to get a path that also uses the disabled transitions
			*    -limitOfTurnsWithoutGettingCloser: if after limitOfTurnsWithoutGettingCloser iterations, the path doesn't seem to get closer of the kr_to node (calculated with the heuristic), it will stop and returns the path to the clothest node from kr_to that has been evaluated
			*	 -heuristicMultiplier : multiplies the value of the heuristicaly calculated distance by this constant
			*/
			PathData getPath(const Node& kr_from, Node kr_to, const float overridenTransitionCost = -1.0f, const bool useDisabledTransitions = false, const float limitOfTurnsWithoutGettingCloser = std::numeric_limits<float>::max(), const float heuristicMultiplier = 1.0f) const;
			
			//change the enabled state of a transition
			void setTransitionEnable(const Transition& kr_transition, bool enabled);

			//change the cost of a transition
			void setTransitionCost(const Transition& kr_transition, float newCost);

			//change the enabled state and the cost of a transition
			void setTransitionEnableAndCost(const Transition& kr_transition, bool enabled, float newCost);

			//change the used heuristic mode
			void setHeuristic(AstarHeuristic heuristic);
			
			//given a transition, return if either this transition is enabled or not
			bool getIsTransitionEnable(const Transition& kr_transition) const;

			//return the heuristic currently used
			AstarHeuristic getHeuristic() const;

			//return all the transitions (enabled and disabled) from a given node
			std::set<Transition> getTransitionsFromNode(const Node& kr_node) const;

		private:
			std::set<Node> m_nodes;
			std::map<Node, std::set<Transition>> m_successors;
			AstarHeuristic m_heuristic;

			ToroidalData m_toroidalData;
			bool m_useToroidalMap;

			float getHeuristicDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const;
			float getEuclideanDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const;
			float getManhattanDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const;
			float getEuclideanToroidalDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const;
			float getManhattanToroidalDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const;

			Astar(const Astar&) = delete;
			Astar& operator=(Astar const&) = delete;
		};
	}
}