#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "ai/nav/astar.hpp"
#include "ai/halite_to_astar.hpp"
#include "ai/managers/spawnmanager.hpp"
#include "ai/managers/dropoffManager.hpp"
#include "ai/halite_to_btree.hpp"

#include <random>
#include <ctime>
#include <Windows.h>
#include <chrono>

using namespace std;
using namespace hlt;
using namespace ai;
using namespace ai::nav;
using namespace ai::btree;

//retourne la distance au dropoff le plus proche
int getDistanceToClosestDropoff(hlt::Ship* p_ship) {
    int minDistance = BB->mkr_game.game_map->calculate_distance(p_ship->position, BB->mkr_game.me->shipyard->position);

    for (const std::pair<hlt::EntityId, std::shared_ptr<hlt::Dropoff>>& dropoff : BB->mkr_game.me->dropoffs) {
        int newDistance = BB->mkr_game.game_map->calculate_distance(p_ship->position, dropoff.second->position);
        if (newDistance <= minDistance) {
            minDistance = newDistance;
        }
    }

    return minDistance;
}

//retourne si p_shipA est plus proche d'un dropoff que p_shipB
bool isShipCloserToDropoff(hlt::Ship* p_shipA, hlt::Ship* p_shipB)
{
    return getDistanceToClosestDropoff(p_shipA) < getDistanceToClosestDropoff(p_shipB);
}

int main(int argc, char* argv[]) {


#if defined _DEBUG
    Sleep(6000); //laisse le temps de s'attacher
#endif

    srand(time(NULL));

    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    } else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    DropoffManager dropoffManager = DropoffManager(game);
    SpawnManager spawner = SpawnManager(game, dropoffManager);
    HaliteToBTreeLinker hltToBTree;

    const size_t k_size = ai::constants::MAP_SIZE;

    //cree le graphe de transitions pour astar
    int haliteSum = 0;
    set<Transition> transitions;
    for (int x = 0; x < k_size; x++) {
        for (int y = 0; y < k_size; y++) {
            float hlt = game.game_map->at({ x,y })->halite / 10.0f;
            transitions.insert({ Node(x,y), Node((x + 1) & (k_size - 1), y), hlt/10.0f });
            transitions.insert({ Node(x,y), Node((x - 1) & (k_size - 1), y), hlt/10.0f });
            transitions.insert({ Node(x,y), Node(x, y + 1 & (k_size - 1)), hlt/10.0f });
            transitions.insert({ Node(x,y), Node(x, y - 1 & (k_size - 1)), hlt/10.0f });
            haliteSum += hlt;
        }
    }
    haliteSum /= (k_size * k_size);
    ai::constants::AVERAGE_HALITE_PER_CELL = haliteSum;
    shared_ptr<Astar> astar = make_shared<Astar>(transitions, AstarHeuristic::MANHATTAN_DISTANCE, ToroidalData({ k_size, k_size }));

    HaliteToAstarLinker hltToAstar(astar, *(game.game_map));
    BlackBoard bB = BlackBoard(game, hltToAstar);

    game.ready("Nino et Joachim");
    
    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");
    log::log("Average halite on this map is : " + std::to_string(ai::constants::AVERAGE_HALITE_PER_CELL));

    for (;;) {
        game.update_frame();

        auto t1 = std::chrono::high_resolution_clock::now(); 
        
        shared_ptr<Player> me = game.me;
        hltToAstar.updateAstar(game.players, me->id);

        //evalue si il faut construire un dropoff ce tour ou non, si oui recupere le plan de construction
        if (dropoffManager.shouldBuildADropoff())
        {
            bB.mp_plan = dropoffManager.getDropoffPlan();
        }
        else {
            dropoffManager.deletePlan();
            bB.mp_plan = nullptr;
        }

        //met a jour la partie commune de la carte de score de navigation
        bB.updateStaticMap(game, 3);

        //trie les bateaux en fonction de la distance au dropoff le plus proche
        std::vector<hlt::Ship*> sortedShips(me->ships.size());
        int i = 0;
        for (const auto& ship_iterator : me->ships) {
            sortedShips[i] = ship_iterator.second.get();
            i++;
        }
        std::sort(sortedShips.begin(), sortedShips.end(), isShipCloserToDropoff);

        //donne une commande a chaque vaisseau en evaluant le beahviour tree ou, s'il ne reste plus de temps, les laissant sur place
        vector<Command> command_queue;
        for (const auto& p_ship : sortedShips) {
            auto t2 = std::chrono::high_resolution_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() < 1900)
            {
                hltToBTree.evaluate(p_ship, &command_queue);
            }
            else {
                command_queue.push_back(p_ship->stay_still());
            }
        }

        //construit un vaisseau si c'est juge necessaire
        if (spawner.canSpawn()) {
            command_queue.push_back(me->shipyard->spawn());
            bB.IncShip();
        }

        //efface l'ancien plan de constrcution si un dropoff a ete construit ce tour
        if (bB.m_hasBuiltADropoff) {
            bB.m_hasBuiltADropoff = false;
            dropoffManager.deletePlan();
        }

        if (!game.end_turn(command_queue)) {
            break;
        }

        auto t3 = std::chrono::high_resolution_clock::now();
        log::log("turn duration : " + std::to_string(chrono::duration_cast<chrono::milliseconds>(t3 - t1).count()) + "ms");
    }

    return 0;
}
