#include "gba/core/Evolution.h"
#include "gba/core/GameModule.h"
#include "gba/core/Model.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace gba;

static Genome baseGenome(const std::vector<ParameterSpec>& specs,std::uint64_t id){Genome g;g.id=id;for(const auto&s:specs)g.genes[s.key]=s.defaultValue;return g;}

int main(){
    try{
        std::filesystem::path root=GBA_SOURCE_DIR;
        std::vector<std::string> files={"tower_defense.json","arena_survival.json","deckbuilder.json","roguelike.json","turn_strategy.json","auto_battler.json"};
        int ok=0;
        for(const auto&f:files){auto game=loadGameDefinition((root/"samples"/f).string());auto module=createGameModule(game.genre);auto b=baseGenome(module->balanceParameters(game),1);auto s=baseGenome(module->strategyParameters(game),2);std::vector<SimulationResult> runs;for(int i=0;i<3;++i){auto r=module->simulate(game,b,s,1234+i,40);if(!std::isfinite(r.progress)||!std::isfinite(r.score))throw std::runtime_error("ungueltiger Simulationswert in "+f);runs.push_back(r);}auto e=module->evaluateBalance(game,b,runs);if(!std::isfinite(e.fitness))throw std::runtime_error("ungueltiger Fitnesswert in "+f);std::cout<<"OK "<<game.genre<<" progress="<<e.avgProgress<<" fit="<<e.fitness<<"\n";ok++;}
        // End-to-End engine smoke test.
        auto game=loadGameDefinition((root/"samples"/"tower_defense.json").string());TrainerConfig c;c.generations=2;c.population=8;c.elite=2;c.trainRuns=2;c.validationRuns=3;c.balanceEvery=1;c.balancePopulation=3;c.balanceFinalists=1;c.adaptationCandidates=3;c.adaptationRuns=1;c.sensitivity=false;c.threads=2;c.checkpointEvery=1;c.maxProgress=60;auto out=root/"build"/"selftest_run";std::filesystem::remove_all(out);EvolutionEngine engine(game,createGameModule(game.genre),c,out);engine.run();if(!std::filesystem::exists(out/"summary.txt")||!std::filesystem::exists(out/"checkpoint.json"))throw std::runtime_error("End-to-End Reports fehlen");
        std::cout<<"SELFTEST "<<ok<<"/6 + End-to-End OK\n";return 0;
    }catch(const std::exception&e){std::cerr<<"SELFTEST FEHLER: "<<e.what()<<"\n";return 1;}
}
