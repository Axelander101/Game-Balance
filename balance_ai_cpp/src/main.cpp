#include "gba/core/Evolution.h"
#include "gba/core/GameModule.h"
#include "gba/core/Json.h"
#include "gba/core/Model.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gba;

static void help(){
    std::cout<<"General Game Balance AI (C++20)\n\n"
             <<"Usage:\n"
             <<"  game_balance_ai --game <game.json> --config <config.json> --out <folder> [--resume]\n"
             <<"  game_balance_ai --list-genres\n\n"
             <<"Example:\n"
             <<"  game_balance_ai --game samples/tower_defense.json --config config/quick_test.json --out runs/td_test\n";
}

int main(int argc,char**argv){
    try{
        if(argc==1){help();return 0;}
        std::string gamePath,configPath,outPath="general_balance_runs/run";bool resume=false;
        for(int i=1;i<argc;++i){std::string a=argv[i];
            if(a=="--help"||a=="-h"){help();return 0;}
            if(a=="--list-genres"){for(const auto&g:supportedGenres())std::cout<<g<<'\n';return 0;}
            if(a=="--resume"){resume=true;continue;}
            if((a=="--game"||a=="--config"||a=="--out")&&i+1>=argc)throw std::runtime_error("Fehlender Wert nach "+a);
            if(a=="--game")gamePath=argv[++i];
            else if(a=="--config")configPath=argv[++i];
            else if(a=="--out")outPath=argv[++i];
            else throw std::runtime_error("Unbekanntes Argument: "+a);
        }
        if(gamePath.empty())throw std::runtime_error("--game fehlt");
        GameDefinition game=loadGameDefinition(gamePath);
        TrainerConfig cfg;
        if(!configPath.empty())cfg=TrainerConfig::fromJson(Json::parseFile(configPath));
        cfg.resume=cfg.resume||resume;
        auto module=createGameModule(game.genre);
        std::cout<<"General Game Balance AI\nGame: "<<game.name<<"\nGenre: "<<game.genre<<"\nOutput: "<<outPath<<"\nThreads: "<<cfg.threads<<"\n\n";
        EvolutionEngine engine(std::move(game),std::move(module),cfg,std::filesystem::path(outPath));engine.run();
        return 0;
    }catch(const std::exception&e){std::cerr<<"FEHLER: "<<e.what()<<'\n';return 1;}
}
