#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"

#include <random>
#include <ctime>

using namespace std;
using namespace hlt;

bool CompareCell(const MapCell& a, const MapCell& b) {
    return a.halite > b.halite;
}

void GetInfluenceMap(vector<vector<int>>& influence_map, const std::vector<std::shared_ptr<Player>>& players, int my_id, const unique_ptr<GameMap>& game_map) {
    for (const shared_ptr<Player>& player_iterator : players) {
        if (player_iterator->id == my_id)
            continue;

        for (const pair<EntityId, shared_ptr<Ship>>& ship_iterator : player_iterator->ships) {
            for (int x = ship_iterator.second->position.x - 4; x < ship_iterator.second->position.x + 4; x++) {
                for (int y = ship_iterator.second->position.y - 4; y < ship_iterator.second->position.y + 4; y++) {
                    if (game_map->calculate_distance(ship_iterator.second->position, {x,y}) <= 4)
                        influence_map[x & 64-1][y & 64-1] += 1;
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    } else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    // At this point "game" variable is populated with initial map data.
    // This is a good place to do computationally expensive start-up pre-processing.
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");

    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");

    for (;;) {
        game.update_frame();
        shared_ptr<Player> me = game.me;
        unique_ptr<GameMap>& game_map = game.game_map;

        vector<Command> command_queue;

        vector<vector<int>> influence_map(game_map->width, vector<int>(game_map->height, 0));
        GetInfluenceMap(influence_map, game.players, game.my_id, game_map);
        
        vector<MapCell> possibilities;

        //les meilleurs endroits où se rendre dans l'ordre
        for (vector<MapCell>& vector_iterator : game_map->cells) {
            for (MapCell cell_iterator : vector_iterator) {
                cell_iterator.halite *= (influence_map[cell_iterator.position.x][cell_iterator.position.y] >=2)? 2 : 1;//2:1
                possibilities.push_back(cell_iterator);
            }
        }
        sort(possibilities.begin(), possibilities.end(), CompareCell);
        log::log("nombre de possibilitees : " + to_string(possibilities.size()));

        for (const auto& ship_iterator : me->ships) {
            shared_ptr<Ship> ship = ship_iterator.second;
            //vérifie qu'il a assez de halite pour bouger
            if (game_map->at(ship)->halite!=0 && game_map->at(ship)->halite*0.1f > ship->halite) {
                command_queue.push_back(ship->stay_still());
                game_map->at(ship)->mark_unsafe(ship);
                continue;
            }

            Position bestPosition;
            int bestCellIndex = -1;
            //vérifie si il est plein
            if (ship->halite >= constants::MAX_HALITE*0.9f) {
                int minDistance = game_map->calculate_distance(ship->position, me->shipyard->position);
                bestPosition = me->shipyard->position;
                for (auto& dropoff : me->dropoffs) {
                    int distance = game_map->calculate_distance(ship->position, dropoff.second->position);
                    if (distance < minDistance) {
                        bestPosition = dropoff.second->position;
                        minDistance = distance;
                    }
                }
                //si il peut, construit un dropoff
                if (minDistance >= 32 && (me->halite + ship->halite) >= constants::DROPOFF_COST) {
                    log::log("try to build a dropoff");
                    command_queue.push_back(ship->make_dropoff());
                    continue;
                }
            }
            else {
                //cherche le meilleur endroit où aller
                Halite bestHalite = -INFINITY;
                bestPosition = ship->position;
                for (int index = 0; index < possibilities.size(); index++) {
                    Halite calculatedHalite = possibilities[index].halite - (game_map->calculate_distance(ship->position, possibilities[index].position))* game_map->at(ship)->halite*2; //cette valeur a tw
                    if (calculatedHalite > bestHalite) {
                        bestHalite = calculatedHalite;
                        bestPosition = possibilities[index].position;
                        bestCellIndex = index;
                    }
                }
            }

            //anticipe les collisions
            for (int x = ship->position.x - 2; x <= ship->position.x + 2; x++) {
                for (int y = ship->position.y - 2; y <= ship->position.y + 2; y++) {
                    if (game_map->calculate_distance(ship->position, { x,y }) <= 2) {
                        if (game_map->at({ x, y })->is_occupied()) {
                            if (game_map->at({ x, y })->ship->owner != game.me->id) {
                                for (int xp = x - 1; xp <= x + 1; xp++) {
                                    for (int yp = y - 1; yp <= y + 1; yp++) {
                                        if (game_map->calculate_distance({ x,y }, { xp,yp }) <= 1) {
                                            game_map->at({ xp, yp })->mark_unsafe(game_map->at({ x, y })->ship);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }



            //avance dans cette direction
            Direction bestDirection = game_map->naive_navigate(ship, bestPosition);
            log::log("from " + to_string(ship->position.x) +":"+ to_string(ship->position.y) + " / to " + to_string(bestPosition.x)+":"+to_string(bestPosition.y));
            command_queue.push_back(ship->move(bestDirection));
            if (bestCellIndex != -1 && ship->position.directional_offset(bestDirection) == bestPosition) {
                possibilities.erase(possibilities.begin() + bestCellIndex);
            }
        }

        if (
            game.turn_number <= 300 &&
            me->halite >= constants::SHIP_COST &&
            !game_map->at(me->shipyard)->is_occupied())
        {
            command_queue.push_back(me->shipyard->spawn());
        }

        if (!game.end_turn(command_queue)) {
            break;
        }
    }

    return 0;
}

//qvoir les influences
//recalculer les halites avec
//retouner un vector avec les cases dans l'ordre
//mettre en unsafe celles autour apres déplacement
