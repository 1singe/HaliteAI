#pragma once

#include "../../hlt/position.hpp"
#include "../../hlt/direction.hpp"
#include "customConstants.hpp"
#include <map>


#if defined _DEBUG
#define LOG(MSG) hlt::log::log(MSG)
#else
#define LOG(MSG)
#endif

#define INSPI(N) -1.0/(12.0 * N + 1.0) + 2.0							// Fonction tendant vers 2, trouvée sur géogébra :)
#define A_MOD(A, N) ((A%N)+N)%N											// Modulo Arithmétique
#define E 2.71828														//Nombre de Neper
#define SQRT_2PI 2.5													//Simplified square root of 2*PI
#define HALIT(DIST) 1.0 / (12.0 * DIST + 1.0)
#define DROPOFF_DIST_MULT(TURN, END_MULTIPLIER) (1.0 - END_MULTIPLIER) / (0.2 * TURN + 1.0) + END_MULTIPLIER

float calculatePseudoNormalLaw(int dist, float flattener, float sigma);
hlt::Direction* directionTowards(const hlt::Position& kr_source, const hlt::Position& kr_destination);		//the direction to reach a point
int getSign(int n);																							//sign of a number