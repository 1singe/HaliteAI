#include "astar.hpp"

float ai::nav::Astar::getHeuristicDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const {
	switch (m_heuristic)
	{
	case ai::nav::AstarHeuristic::EUCLIDEAN_DISTANCE:
		if (m_useToroidalMap) {
			return getEuclideanToroidalDistance(kr_nodeA, kr_nodeB);
		}
		else {
			return getEuclideanDistance(kr_nodeA, kr_nodeB);
		}
		break;
	case ai::nav::AstarHeuristic::MANHATTAN_DISTANCE:
		if (m_useToroidalMap) {
			return getManhattanToroidalDistance(kr_nodeA, kr_nodeB);
		}
		else {
			return getManhattanDistance(kr_nodeA, kr_nodeB);
		}
		break;
	default:
		throw ("Error with calculating the distance");
		break;
	}
}

float ai::nav::Astar::getEuclideanDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const {
	return sqrtf(powf(kr_nodeA.getX() - kr_nodeB.getX(), 2) + powf(kr_nodeA.getY() - kr_nodeB.getY(), 2));
}
 
float ai::nav::Astar::getManhattanDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const {
	return std::abs(kr_nodeA.getX() - kr_nodeB.getX()) + std::abs(kr_nodeA.getY() - kr_nodeB.getY());
}

float ai::nav::Astar::getEuclideanToroidalDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const  {
	const int dx = std::abs(kr_nodeA.getX() - kr_nodeB.getX());
	const int dy = std::abs(kr_nodeA.getY() - kr_nodeB.getY());

	const int toroidal_dx = (dx < m_toroidalData.width - dx) ? dx : (m_toroidalData.width - dx);   //min
	const int toroidal_dy = (dy < m_toroidalData.height - dy) ? dy : (m_toroidalData.height - dy); //min

	return std::sqrt(std::pow(toroidal_dx,2) + std::pow(toroidal_dy,2));
}

float ai::nav::Astar::getManhattanToroidalDistance(const ai::nav::Node& kr_nodeA, const ai::nav::Node& kr_nodeB) const {

	const int dx = std::abs(kr_nodeA.getX() - kr_nodeB.getX());
	const int dy = std::abs(kr_nodeA.getY() - kr_nodeB.getY());

	const int toroidal_dx = (dx < m_toroidalData.width - dx)? dx : (m_toroidalData.width - dx);   //min
	const int toroidal_dy = (dy < m_toroidalData.height - dy)? dy : (m_toroidalData.height - dy); //min

	return ( toroidal_dx + toroidal_dy);
}

ai::nav::Astar::Astar(const std::set<Transition>& kr_transitions, AstarHeuristic heuristic, ToroidalData toroidalData) {
	for (const Transition& transition : kr_transitions) {
		if (m_successors.find(transition.from) == m_successors.end()) {
			std::set<Transition> newSet;
			m_successors[transition.from] = newSet;
		}
		m_successors.at(transition.from).insert(transition);
		m_nodes.insert(transition.from);
	}
	if (toroidalData != ToroidalData({0, 0})) {
		m_useToroidalMap = true;
		m_toroidalData = toroidalData;
	}
	else {
		m_useToroidalMap = false;
	}

	m_heuristic = heuristic;
}

void ai::nav::Astar::setTransitionEnable(const Transition& kr_transition, bool enabled) {
	std::set<Transition>::iterator it_transition = m_successors[kr_transition.from].find(kr_transition);
	if (it_transition != m_successors[kr_transition.from].end()) {
		Transition newTransition = *it_transition;
		newTransition.enabled = enabled;
		m_successors[kr_transition.from].erase(it_transition);
		m_successors[kr_transition.from].insert(newTransition);
	} else {
		std::cerr << "No transition has been found" << std::endl;
	}
}

void ai::nav::Astar::setTransitionCost(const Transition& kr_transition, float newCost) {
	std::set<Transition>::iterator it_transition = m_successors[kr_transition.from].find(kr_transition);
	if (it_transition != m_successors[kr_transition.from].end()) {
		Transition newTransition = *it_transition;
		newTransition.cost = newCost;
		m_successors[kr_transition.from].erase(it_transition);
		m_successors[kr_transition.from].insert(newTransition);
	} else {
		std::cerr << "No transition has been found" << std::endl;
	}
}

void ai::nav::Astar::setTransitionEnableAndCost(const Transition& kr_transition, bool enabled, float newCost) {
	std::set<Transition>::iterator it_transition = m_successors[kr_transition.from].find(kr_transition);
	if (it_transition != m_successors[kr_transition.from].end()) {
		Transition newTransition = *it_transition;
		newTransition.cost = newCost;
		newTransition.enabled = enabled;
		m_successors[kr_transition.from].erase(it_transition);
		m_successors[kr_transition.from].insert(newTransition);
	}
	else {
		std::cerr << "No transition has been found" << std::endl;
	}
}

std::set<ai::nav::Transition> ai::nav::Astar::getTransitionsFromNode(const Node& kr_node) const {
	std::map<Node, std::set<Transition>>::const_iterator it_transitions = m_successors.find(kr_node);
	if (it_transitions != m_successors.end()) {
		return it_transitions->second;
	}
	else {
		return std::set<Transition>({});
	}
}

void ai::nav::Astar::setHeuristic(AstarHeuristic heuristic) {
	m_heuristic = heuristic;
}

ai::nav::AstarHeuristic ai::nav::Astar::getHeuristic() const {
	return m_heuristic;
}

bool ai::nav::Astar::getIsTransitionEnable(const Transition& kr_transition) const {
	std::map<Node, std::set<Transition>>::const_iterator it_transitions = m_successors.find(kr_transition.from);
	if (it_transitions != m_successors.end()) {
		std::set<Transition>::iterator it_node = it_transitions->second.find(kr_transition);
		if (it_node != it_transitions->second.end()) {
			return it_node->enabled;
		}
	}
	return false;
}


ai::nav::PathData ai::nav::Astar::getPath(const Node& kr_from, Node kr_to, const float overridenTransitionCost, const bool useDisableTransitions, const float limitOfTurnsWithoutGettingCloser, const float heuristicMultiplier) const {

	if (kr_from == kr_to) {
		return { std::deque<Node>({kr_from}), 0, false };
	}

	//init local variables
	std::map<Node, float> distances;
	std::map<Node, float> distancesWithHeuristic;
	std::map<Node, Node*> predecessors;
	std::set<Node> openSet;
	std::set<Node> closedSet;

	//init the initial values
	openSet.insert(kr_from);
	for (const Node& kr_node : m_nodes) {
		distances[kr_node] = std::numeric_limits<float>::max();
		distancesWithHeuristic[kr_node] = std::numeric_limits<float>::max();
	}
	distances[kr_from] = 0;
	distancesWithHeuristic[kr_from] = getHeuristicDistance(kr_from, kr_to) * heuristicMultiplier;
	predecessors[kr_from] = new Node(kr_from);

	//prevent from looking for a way too long path or impossible path
	int numberOfTurnWithoutGettingCloser = 0;
	Node closestNodeFoundAroundDestination = kr_from;
	float closestHeuristicalDistance = std::numeric_limits<float>::max();

	while (!openSet.empty()) {

		//find the node with the lowest distance with heuristic
		float minimalDistance = std::numeric_limits<float>::max();;
		const Node* kr_closestNode = nullptr;
		for (const Node& kr_node: openSet) {
			float distance = distancesWithHeuristic[kr_node];
			if (distance < minimalDistance) {
				minimalDistance = distance;
				kr_closestNode = &kr_node;
			}
		}
		Node closestNode(*kr_closestNode);

		//check if has approched from the destination
		float estimatedHeuristicalDistanceLeft = getHeuristicDistance(closestNode, kr_to) * heuristicMultiplier;
		if (estimatedHeuristicalDistanceLeft < closestHeuristicalDistance) {
			numberOfTurnWithoutGettingCloser = 0;
			closestNodeFoundAroundDestination = *kr_closestNode;
			closestHeuristicalDistance = estimatedHeuristicalDistanceLeft;
		}
		else {
			numberOfTurnWithoutGettingCloser++;
			if (numberOfTurnWithoutGettingCloser > limitOfTurnsWithoutGettingCloser) {
				kr_to = closestNodeFoundAroundDestination;
				closestNode = kr_to;
			}
		}

		//check if we have finished
		if (closestNode == kr_to) {
			PathData pathData;
			pathData.finalCost = distances[kr_to];
			Node currentNode = kr_to;
			do {
				pathData.path.push_front(currentNode);
				currentNode = *predecessors[currentNode];
				delete predecessors[pathData.path.front()];
			} while (!currentNode.operator==(kr_from));
			pathData.goesToPlannedDestination = true;
			return pathData;
		}
	
		//if not finished, look at its neighbors
		openSet.erase(closestNode);
		closedSet.insert(closestNode);

		for (const Transition& kr_transitions : m_successors.at(closestNode)) {
			const Node& kr_neighbor = kr_transitions.to;
			if ( (!kr_transitions.enabled && useDisableTransitions == false) || closedSet.find(kr_neighbor) != closedSet.end()) {
				continue;
			}	

			float distance = distances[closestNode];
			if (overridenTransitionCost <= 0) {
				distances[closestNode] += kr_transitions.cost;
			}
			else {
				distances[closestNode] += overridenTransitionCost;
			}

			if (openSet.find(closestNode) == openSet.end() || distance < distances[kr_neighbor]) {
				distances[kr_neighbor] = distance;
				distancesWithHeuristic[kr_neighbor] = distances[kr_neighbor] + getHeuristicDistance(kr_neighbor, kr_to) * heuristicMultiplier;
				predecessors[kr_neighbor] = new Node(closestNode);

				openSet.insert(kr_neighbor);
			}
		}
	}

	for (std::pair<Node, Node*> pair : predecessors) {
		delete pair.second;
	}
	return { std::deque<Node>({kr_from}), 0, false };
}