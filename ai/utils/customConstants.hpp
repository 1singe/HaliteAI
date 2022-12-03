#pragma once

namespace ai {
	namespace constants {
		static const int MAP_SIZE = 64;
		static const int TURN_BONUS_TO_SOLVE_CONFLITS = 10;
		static const int MAX_ITERATIONS_WITHOUT_GETTING_CLOSER_ALLOWED = 100;
		static const float COLLECT_PERCENTAGE_BEFORE_BACK = 80;
		static const float COLLECT_PERCENTAGE_CANCEL_BACK = 60;
		static const float MINIMUM_SHIP_FOR_BONUS = 80.0f;
		static float AVERAGE_HALITE_PER_CELL = 223.0f;
		static const int INSPIRATION_STARTING_TURN = 310;
		static const int HALITE_FLATTENING_RADIUS = 0;
		static const int SCORE_FLATTENING_RADIUS = 3;
		static const float HALITE_DISTRIBUTION_CURVE_SIGMA = 0.4f;
		static const float HALITE_DISTRIBUTION_CURVE_MULTIPLIER = 2.0f;
		static const int MAX_DISTANCE_FOR_ADVANCED_AVERAGE = 3;
		static const int UNSAFE_RADIUS_AROUND_DROPOFF = 2;
		static const float MAX_FILLED_CELL_AMOUNT = 1000.f;
		static const float END_DISTANCE_TO_DROPOFF_MULTIPLIER = 0.1f;
		static const float EXPANSIONISM = 0.5f;

		// --- POIDS ---

		static const float DROPOFF_DISTANCE_WEIGHT = 0.1f;
		static const float SHIP_DISTANCE_WEIGHT = 0.8f;
		static const float TARGET_WEIGHT = 1.0f;
		static const float HALITE_FLATTENER_WEIGHT = 0.05f;
	}
}