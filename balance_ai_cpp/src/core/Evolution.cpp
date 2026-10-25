#include "gba/core/Evolution.h"
#include "gba/core/Report.h"
#include <algorithm>
#include <chrono>
#include <atomic>
#include <cmath>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>

namespace gba {
namespace {
double clampTo(const ParameterSpec& s,double v){v=std::clamp(v,s.minValue,s.maxValue);if(s.integer)v=std::round(v);return v;}
std::string genesCompact(const Genome& g){std::ostringstream s;bool first=true;for(const auto&[k,v]:g.genes){if(!first)s<<';';first=false;s<<k<<'='<<std::setprecision(6)<<v;}return s.str();}
}

EvolutionEngine::EvolutionEngine(GameDefinition game,std::unique_ptr<IGameModule> module,TrainerConfig config,std::filesystem::path outDir)
:game_(std::move(game)),module_(std::move(module)),config_(config),outDir_(std::move(outDir)),rng_(config.trainingSeed){
    strategySpecs_=module_->strategyParameters(game_);balanceSpecs_=module_->balanceParameters(game_);mutationStrength_=config_.mutationStrength;
    ensureRunFiles(outDir_);
}

Genome EvolutionEngine::makeBase(const std::vector<ParameterSpec>& specs,int generation,const std::string& note){Genome g;g.id=nextGenomeId_++;g.generation=generation;g.note=note;for(const auto&s:specs)g.genes[s.key]=s.defaultValue;return g;}
Genome EvolutionEngine::randomGenome(const std::vector<ParameterSpec>& specs,int generation,const std::string& note){Genome g;g.id=nextGenomeId_++;g.generation=generation;g.note=note;for(const auto&s:specs){std::uniform_real_distribution<double>d(s.minValue,s.maxValue);g.genes[s.key]=clampTo(s,d(rng_));}return g;}
Genome EvolutionEngine::mutateGenome(const Genome& parent,const std::vector<ParameterSpec>& specs,int generation,double strength,int changes,const std::string& note){Genome g=parent;g.id=nextGenomeId_++;g.generation=generation;g.fitness=-1e100;g.validationFitness=-1e100;g.note=note;if(specs.empty())return g;std::uniform_int_distribution<std::size_t>pick(0,specs.size()-1);std::normal_distribution<double>z(0,1);for(int i=0;i<std::max(1,changes);++i){const auto&s=specs[pick(rng_)];double cur=g.get(s.key,s.defaultValue);double span=s.maxValue-s.minValue;double step=std::max(1e-9,span*s.mutationScale*strength);double nv=cur+z(rng_)*step;if(cur>0&&s.minValue>=0&&s.maxValue/s.minValue>5.0){nv=cur*std::exp(z(rng_)*s.mutationScale*strength);}g.set(s.key,clampTo(s,nv));}return g;}
void EvolutionEngine::clampGenome(Genome&g,const std::vector<ParameterSpec>&specs)const{for(const auto&s:specs)g.genes[s.key]=clampTo(s,g.get(s.key,s.defaultValue));}

std::vector<SimulationResult> EvolutionEngine::runBatch(const Genome& balance,const Genome& strategy,int runs,std::uint64_t seedBase)const{
    std::vector<SimulationResult> out(static_cast<std::size_t>(runs));std::atomic<int>next{0};int tc=std::min(config_.threads,runs);std::vector<std::thread>workers;workers.reserve(tc);
    for(int t=0;t<tc;++t)workers.emplace_back([&,t]{for(;;){int i=next.fetch_add(1);if(i>=runs)break;std::uint64_t seed=seedBase+static_cast<std::uint64_t>(i)*0x9E3779B97F4A7C15ULL;out[static_cast<std::size_t>(i)]=module_->simulate(game_,balance,strategy,seed,config_.maxProgress);}});
    for(auto&th:workers) th.join();
    return out;
}

EvaluationSummary EvolutionEngine::evaluateStrategy(Genome& strategy,const Genome& balance,int runs,std::uint64_t seedBase,bool validation)const{
    auto rs=runBatch(balance,strategy,runs,seedBase);EvaluationSummary s=module_->evaluateBalance(game_,balance,rs);double fit=0;for(const auto&r:rs)fit+=module_->strategyFitness(game_,r);fit/=std::max<std::size_t>(1,rs.size());s.fitness=fit;if(validation)strategy.validationFitness=fit;else strategy.fitness=fit;return s;
}
EvaluationSummary EvolutionEngine::evaluateBalance(const Genome& balance,const Genome& strategy,int runs,std::uint64_t seedBase)const{return module_->evaluateBalance(game_,balance,runBatch(balance,strategy,runs,seedBase));}

void EvolutionEngine::evaluatePopulation(int generation){
    std::cout<<"[Gen "<<generation<<"] Strategien evaluieren ("<<population_.size()<<")...\n";
    std::atomic<int> next{0};
    std::vector<double> raw(population_.size());
    int tc=std::min<int>(config_.threads,static_cast<int>(population_.size()));
    std::vector<std::thread> workers;
    for(int t=0;t<tc;++t) workers.emplace_back([&,t]{
        for(;;){
            int i=next.fetch_add(1); if(i>=static_cast<int>(population_.size())) break;
            Genome local=population_[i]; double f=0;
            for(int rr=0;rr<config_.trainRuns;++rr){
                auto sr=module_->simulate(game_,currentBalance_,local,config_.trainingSeed+static_cast<std::uint64_t>(generation)*1000003ULL+static_cast<std::uint64_t>(rr)*101ULL,config_.maxProgress);
                f+=module_->strategyFitness(game_,sr);
            }
            raw[static_cast<std::size_t>(i)]=f/config_.trainRuns;
        }
    });
    for(auto& th:workers) th.join();

    lastRawFitness_.clear(); lastNovelty_.clear();
    for(std::size_t i=0;i<population_.size();++i){
        std::vector<double> distances;
        distances.reserve(population_.size()>0?population_.size()-1:0);
        for(std::size_t j=0;j<population_.size();++j){
            if(i==j) continue;
            double sum=0; int n=0;
            for(const auto& spec:strategySpecs_){
                double span=std::max(1e-9,spec.maxValue-spec.minValue);
                double a=population_[i].get(spec.key,spec.defaultValue);
                double b=population_[j].get(spec.key,spec.defaultValue);
                double d=(a-b)/span; sum+=d*d; n++;
            }
            distances.push_back(n?std::sqrt(sum/n):0.0);
        }
        std::sort(distances.begin(),distances.end());
        int k=std::min<int>(5,distances.size()); double novelty=0;
        for(int q=0;q<k;++q) novelty+=distances[static_cast<std::size_t>(q)];
        if(k>0) novelty/=k;
        population_[i].fitness=raw[i]+config_.diversityPressure*novelty*1000.0;
        lastRawFitness_[population_[i].id]=raw[i];
        lastNovelty_[population_[i].id]=novelty;
    }
    std::sort(population_.begin(),population_.end(),[](const Genome&a,const Genome&b){return a.fitness>b.fitness;});
}

void EvolutionEngine::reproduce(int generation){
    std::vector<Genome> next;next.reserve(config_.population);int eliteCount=std::min<int>(config_.elite,population_.size());for(int i=0;i<eliteCount;++i)next.push_back(population_[i]);
    int randomCount=static_cast<int>(std::round(config_.population*config_.randomShare));std::uniform_int_distribution<int>parentPick(0,std::max(0,eliteCount-1));std::bernoulli_distribution mutate(config_.mutationRate);
    while(static_cast<int>(next.size())<config_.population-randomCount){const Genome&p=population_[parentPick(rng_)];Genome c=p;c.id=nextGenomeId_++;c.generation=generation+1;c.note="offspring";c.fitness=-1e100;c.validationFitness=-1e100;if(mutate(rng_))c=mutateGenome(p,strategySpecs_,generation+1,mutationStrength_,1+static_cast<int>(rng_()%3),"mutated");next.push_back(std::move(c));}
    while(static_cast<int>(next.size())<config_.population) next.push_back(randomGenome(strategySpecs_,generation+1,"explorer"));
    population_=std::move(next);
}

void EvolutionEngine::updateHallOfFame(const Genome& champion){hallOfFame_.push_back(champion);std::sort(hallOfFame_.begin(),hallOfFame_.end(),[](const Genome&a,const Genome&b){return std::max(a.validationFitness,a.fitness)>std::max(b.validationFitness,b.fitness);});if(static_cast<int>(hallOfFame_.size())>config_.hallOfFameSize)hallOfFame_.resize(config_.hallOfFameSize);}

Genome EvolutionEngine::adaptStrategyForBalance(const Genome& balance,int generation,double&outFitness){
    std::vector<Genome> candidates; candidates.reserve(config_.adaptationCandidates);
    candidates.push_back(bestStrategy_);
    for(const auto&h:hallOfFame_){if(static_cast<int>(candidates.size())>=config_.adaptationCandidates)break;candidates.push_back(h);}
    while(static_cast<int>(candidates.size())<config_.adaptationCandidates){if(!bestStrategy_.genes.empty())candidates.push_back(mutateGenome(bestStrategy_,strategySpecs_,generation,mutationStrength_*1.35,2+static_cast<int>(rng_()%4),"balance-adapt"));else candidates.push_back(randomGenome(strategySpecs_,generation,"balance-adapt-random"));}
    Genome best=candidates.front();outFitness=-1e100;
    for(auto&c:candidates){auto s=evaluateStrategy(c,balance,config_.adaptationRuns,config_.trainingSeed+700000000ULL+static_cast<std::uint64_t>(generation)*10007ULL);if(s.fitness>outFitness){outFitness=s.fitness;best=c;}}
    return best;
}

void EvolutionEngine::writeGeneration(int generation,const EvaluationSummary&s){CsvAppender csv(outDir_/"generations.csv","time,generation,strategy_id,strategy_fitness,validation_fitness,balance_id,avg_progress,p10,success,early_safety,top_share,top_pair,diversity,waste,mutation_strength");std::ostringstream r;r<<csvEscape(nowIso())<<','<<generation<<','<<bestStrategy_.id<<','<<bestStrategy_.fitness<<','<<bestStrategy_.validationFitness<<','<<currentBalance_.id<<','<<s.avgProgress<<','<<s.p10Progress<<','<<s.successRate<<','<<s.earlySafety<<','<<s.topShare<<','<<s.topPairShare<<','<<s.diversity<<','<<s.wasteRatio<<','<<mutationStrength_;csv.append(r.str());}
void EvolutionEngine::writePareto(int generation){
    CsvAppender csv(outDir_/"pareto_front.csv","generation,strategy_id,raw_fitness,novelty,combined_fitness,note");
    std::vector<const Genome*> front;
    for(const auto& a:population_){
        double ar=lastRawFitness_.count(a.id)?lastRawFitness_[a.id]:a.fitness;
        double an=lastNovelty_.count(a.id)?lastNovelty_[a.id]:0;
        bool dominated=false;
        for(const auto& b:population_){
            if(a.id==b.id) continue;
            double br=lastRawFitness_.count(b.id)?lastRawFitness_[b.id]:b.fitness;
            double bn=lastNovelty_.count(b.id)?lastNovelty_[b.id]:0;
            if(br>=ar && bn>=an && (br>ar || bn>an)){dominated=true;break;}
        }
        if(!dominated) front.push_back(&a);
    }
    for(const Genome* g:front){
        std::ostringstream row; row<<generation<<','<<g->id<<','<<lastRawFitness_[g->id]<<','<<lastNovelty_[g->id]<<','<<g->fitness<<','<<csvEscape(g->note); csv.append(row.str());
    }
}

void EvolutionEngine::writeExploit(int generation,const EvaluationSummary& s){
    if(s.topShare<=.55 && s.topPairShare<=.80) return;
    CsvAppender csv(outDir_/"exploit_candidates.csv","generation,type,severity,top_share,top_pair,diversity,explanation");
    std::string type=s.topShare>.65?"single_entity_dominance":"pair_dominance";
    double sev=std::max(std::max(0.0,s.topShare-.55),std::max(0.0,s.topPairShare-.80));
    std::ostringstream row; row<<generation<<','<<type<<','<<sev<<','<<s.topShare<<','<<s.topPairShare<<','<<s.diversity<<','<<csvEscape(s.explanation); csv.append(row.str());
}

void EvolutionEngine::adversarialValidation(Genome& strategy,int generation){
    if(!config_.adversarialValidation || generation%config_.adversarialEvery!=0) return;
    struct SeedFit{std::uint64_t seed;double fit;double progress;};
    std::vector<SeedFit> values; values.reserve(config_.adversarialCandidateSeeds);
    for(int i=0;i<config_.adversarialCandidateSeeds;++i){
        std::uint64_t seed=config_.validationSeed+7000000000ULL+static_cast<std::uint64_t>(generation)*10007ULL+static_cast<std::uint64_t>(i)*7919ULL;
        auto r=module_->simulate(game_,currentBalance_,strategy,seed,config_.maxProgress);
        values.push_back({seed,module_->strategyFitness(game_,r),r.progress});
    }
    std::sort(values.begin(),values.end(),[](const SeedFit&a,const SeedFit&b){return a.fit<b.fit;});
    int n=std::min<int>(config_.adversarialWorstSeeds,values.size()); double worst=0;
    CsvAppender csv(outDir_/"adversarial_seeds.csv","generation,strategy_id,seed,fitness,progress,is_worst");
    for(std::size_t i=0;i<values.size();++i){if(static_cast<int>(i)<n)worst+=values[i].fit;std::ostringstream row;row<<generation<<','<<strategy.id<<','<<values[i].seed<<','<<values[i].fit<<','<<values[i].progress<<','<<(static_cast<int>(i)<n?1:0);csv.append(row.str());}
    if(n>0){worst/=n;strategy.validationFitness=.80*strategy.validationFitness+.20*worst;}
}

void EvolutionEngine::writeBalanceCandidate(int generation,const Genome&g,const EvaluationSummary&s,bool accepted){CsvAppender csv(outDir_/"balance_genomes.csv","time,generation,balance_id,fitness,avg_progress,p10,success,early_safety,top_share,top_pair,diversity,waste,accepted,note,genes");std::ostringstream r;r<<csvEscape(nowIso())<<','<<generation<<','<<g.id<<','<<s.fitness<<','<<s.avgProgress<<','<<s.p10Progress<<','<<s.successRate<<','<<s.earlySafety<<','<<s.topShare<<','<<s.topPairShare<<','<<s.diversity<<','<<s.wasteRatio<<','<<(accepted?1:0)<<','<<csvEscape(g.note)<<','<<csvEscape(genesCompact(g));csv.append(r.str());}

void EvolutionEngine::balanceCycle(int generation){
    std::cout<<"[Gen "<<generation<<"] Balance-Evolution...\n";
    double baseAdaptFit=0;Genome baseStrategy=adaptStrategyForBalance(currentBalance_,generation,baseAdaptFit);EvaluationSummary base=evaluateBalance(currentBalance_,baseStrategy,config_.validationRuns,config_.validationSeed+static_cast<std::uint64_t>(generation)*100003ULL);
    struct Candidate{Genome balance;Genome strategy;EvaluationSummary quick;double adaptFit=-1e100;};std::vector<Candidate>cands;cands.reserve(config_.balancePopulation+8);
    for(int i=0;i<config_.balancePopulation;++i){Genome g=mutateGenome(currentBalance_,balanceSpecs_,generation,config_.balanceMutationStrength,config_.balanceChangesPerGenome,"free-balance");double af;Genome s=adaptStrategyForBalance(g,generation,af);auto q=evaluateBalance(g,s,std::max(3,config_.trainRuns),config_.trainingSeed+810000000ULL+static_cast<std::uint64_t>(generation)*1009ULL+i*131ULL);cands.push_back({std::move(g),std::move(s),q,af});}
    auto targeted=module_->targetedBalanceExperiments(game_,currentBalance_,base,nextGenomeId_,generation,rng_,config_.balanceMutationStrength);
    for(auto&g:targeted){clampGenome(g,balanceSpecs_);double af;Genome s=adaptStrategyForBalance(g,generation,af);auto q=evaluateBalance(g,s,std::max(3,config_.trainRuns),config_.trainingSeed+820000000ULL+static_cast<std::uint64_t>(generation)*1013ULL+g.id%997ULL);cands.push_back({std::move(g),std::move(s),q,af});}
    std::sort(cands.begin(),cands.end(),[](const Candidate&a,const Candidate&b){return a.quick.fitness>b.quick.fitness;});
    int finalists=std::min<int>(config_.balanceFinalists,cands.size());double bestFit=base.fitness;int bestIdx=-1;EvaluationSummary bestSummary=base;Genome bestAdapt=baseStrategy;
    for(int i=0;i<finalists;++i){double af;Genome adapted=adaptStrategyForBalance(cands[i].balance,generation,af);auto val=evaluateBalance(cands[i].balance,adapted,config_.validationRuns,config_.validationSeed+static_cast<std::uint64_t>(generation)*100003ULL);cands[i].balance.validationFitness=val.fitness;writeBalanceCandidate(generation,cands[i].balance,val,false);if(val.fitness>bestFit){bestFit=val.fitness;bestIdx=i;bestSummary=val;bestAdapt=adapted;}}
    double threshold=base.fitness+std::max(1.0,std::abs(base.fitness)*config_.minBalanceImprovement);
    if(bestIdx>=0&&bestFit>threshold){Genome accepted=cands[bestIdx].balance;accepted.note="ACCEPTED "+accepted.note;currentBalance_=accepted;bestStrategy_=bestAdapt;bestStrategy_.fitness=bestAdapt.fitness;population_.insert(population_.begin(),bestStrategy_);if(static_cast<int>(population_.size())>config_.population)population_.resize(config_.population);writeBalanceCandidate(generation,currentBalance_,bestSummary,true);currentBalance_.toJson().writeFile((outDir_/"current_balance.json").string(),2);bestStrategy_.toJson().writeFile((outDir_/"best_strategy.json").string(),2);std::cout<<"  Balance akzeptiert: "<<base.fitness<<" -> "<<bestFit<<"\n";}else std::cout<<"  Keine robuste Verbesserung (Basis "<<base.fitness<<", beste "<<bestFit<<")\n";
}

void EvolutionEngine::sensitivityAnalysis(int generation){
    if(!config_.sensitivity) return;
    std::cout<<"[Gen "<<generation<<"] Sensitivity...\n";
    CsvAppender csv(outDir_/"sensitivity.csv","generation,parameter,direction,base_value,test_value,base_fitness,test_fitness,delta");
    auto base=evaluateBalance(currentBalance_,bestStrategy_,std::min(8,config_.validationRuns),config_.validationSeed+500000000ULL+generation*43ULL);
    int count=0;for(const auto&s:balanceSpecs_){if(count++>=40)break;for(int dir:{-1,1}){Genome g=currentBalance_;double v=g.get(s.key,s.defaultValue);double delta=(s.maxValue-s.minValue)*0.04*dir;g.set(s.key,clampTo(s,v+delta));auto x=evaluateBalance(g,bestStrategy_,std::min(8,config_.validationRuns),config_.validationSeed+500000000ULL+generation*43ULL);std::ostringstream row;row<<generation<<','<<csvEscape(s.key)<<','<<dir<<','<<v<<','<<g.get(s.key)<<','<<base.fitness<<','<<x.fitness<<','<<(x.fitness-base.fitness);csv.append(row.str());}}
}

void EvolutionEngine::saveCheckpoint(int generation)const{Json::Array pop;for(const auto&g:population_)pop.push_back(g.toJson());Json::Array hof;for(const auto&g:hallOfFame_)hof.push_back(g.toJson());Json root=Json::Object{{"format","GBA_CHECKPOINT_V1"},{"generation",generation},{"nextGenomeId",static_cast<double>(nextGenomeId_)},{"mutationStrength",mutationStrength_},{"currentBalance",currentBalance_.toJson()},{"bestStrategy",bestStrategy_.toJson()},{"population",Json(std::move(pop))},{"hallOfFame",Json(std::move(hof))},{"config",config_.toJson()}};root.writeFile((outDir_/"checkpoint.json").string(),2);}
bool EvolutionEngine::loadCheckpoint(){auto p=outDir_/"checkpoint.json";if(!std::filesystem::exists(p))return false;Json j=Json::parseFile(p.string());if(j["format"].asString()!="GBA_CHECKPOINT_V1")return false;startGeneration_=j["generation"].asInt()+1;nextGenomeId_=static_cast<std::uint64_t>(j["nextGenomeId"].asLong(nextGenomeId_));mutationStrength_=j["mutationStrength"].asNumber(config_.mutationStrength);currentBalance_=Genome::fromJson(j["currentBalance"]);bestStrategy_=Genome::fromJson(j["bestStrategy"]);population_.clear();if(j["population"].isArray())for(const auto&x:j["population"].asArray())population_.push_back(Genome::fromJson(x));hallOfFame_.clear();if(j["hallOfFame"].isArray())for(const auto&x:j["hallOfFame"].asArray())hallOfFame_.push_back(Genome::fromJson(x));std::cout<<"Checkpoint geladen: Generation "<<startGeneration_-1<<"\n";return !population_.empty();}

void EvolutionEngine::writeSummary(int generation,const EvaluationSummary&e){std::ofstream f(outDir_/"summary.txt");f<<"GENERAL GAME BALANCE AI - SUMMARY\n";f<<"=================================\n";f<<"Game: "<<game_.name<<"\nGenre: "<<game_.genre<<"\nGeneration: "<<generation<<"\n";f<<"Best strategy ID: "<<bestStrategy_.id<<"\nBalance ID: "<<currentBalance_.id<<"\n";f<<"Strategy fitness: "<<bestStrategy_.validationFitness<<"\nBalance target fitness: "<<e.fitness<<"\n";f<<"Avg progress: "<<e.avgProgress<<"\nP10 progress: "<<e.p10Progress<<"\nSuccess: "<<e.successRate*100<<"%\nEarly safety: "<<e.earlySafety*100<<"%\nTop contribution: "<<e.topShare*100<<"%\nTop pair: "<<e.topPairShare*100<<"%\nDiversity: "<<e.diversity*100<<"%\nWaste: "<<e.wasteRatio*100<<"%\n";f<<"\nBalance genes:\n";for(const auto&[k,v]:currentBalance_.genes)f<<"  "<<k<<" = "<<v<<"\n";f<<"\nStrategy genes:\n";for(const auto&[k,v]:bestStrategy_.genes)f<<"  "<<k<<" = "<<v<<"\n";f<<"\nTarget explanation: "<<e.explanation<<"\n";
    std::ofstream patch(outDir_/"balance_patch.txt");
    patch<<"GENERAL GAME BALANCE AI - BALANCE PATCH\n========================================\n";
    for(const auto& spec:balanceSpecs_){double v=currentBalance_.get(spec.key,spec.defaultValue);double d=spec.defaultValue;double rel=(std::abs(d)>1e-12)?(v/d-1.0):0.0;if((spec.integer&&std::abs(v-d)>=0.5)||(!spec.integer&&std::abs(rel)>=0.02)){patch<<spec.key<<": "<<d<<" -> "<<v;if(!spec.integer&&std::abs(d)>1e-12)patch<<" ("<<(rel>=0?"+":"")<<rel*100<<"%)";patch<<"\n";}}
}

void EvolutionEngine::run(){
    if(config_.resume&&loadCheckpoint()){}else{currentBalance_=makeBase(balanceSpecs_,0,"base-balance");bestStrategy_=makeBase(strategySpecs_,0,"base-strategy");population_.clear();population_.push_back(bestStrategy_);while(static_cast<int>(population_.size())<config_.population)population_.push_back(randomGenome(strategySpecs_,0,"initial-random"));}
    int lastGen=startGeneration_-1; auto started=std::chrono::steady_clock::now();
    for(int gen=startGeneration_;gen<=config_.generations;++gen){
        if(config_.timeLimitMinutes>0){auto mins=std::chrono::duration_cast<std::chrono::minutes>(std::chrono::steady_clock::now()-started).count();if(mins>=config_.timeLimitMinutes){std::cout<<"Zeitlimit erreicht ("<<config_.timeLimitMinutes<<" min).\n";break;}}
        lastGen=gen;evaluatePopulation(gen);writePareto(gen);Genome champion=population_.front();auto val=evaluateStrategy(champion,currentBalance_,config_.validationRuns,config_.validationSeed+static_cast<std::uint64_t>(gen)*100003ULL,true);adversarialValidation(champion,gen);if(champion.validationFitness>bestStrategy_.validationFitness||bestStrategy_.genes.empty()){bestStrategy_=champion;staleGenerations_=0;mutationStrength_=std::max(config_.minMutationStrength,mutationStrength_*0.96);}else{staleGenerations_++;if(staleGenerations_%15==0)mutationStrength_=std::min(config_.maxMutationStrength,mutationStrength_*1.15);}updateHallOfFame(champion);auto balanceView=evaluateBalance(currentBalance_,bestStrategy_,std::min(config_.validationRuns,12),config_.validationSeed+static_cast<std::uint64_t>(gen)*9973ULL);writeGeneration(gen,balanceView);writeExploit(gen,balanceView);std::cout<<"  Champion "<<bestStrategy_.id<<" fit="<<bestStrategy_.validationFitness<<" | "<<balanceView.explanation<<"\n";if(gen%config_.balanceEvery==0)balanceCycle(gen);if(config_.sensitivity&&gen%config_.sensitivityEvery==0)sensitivityAnalysis(gen);if(gen%config_.checkpointEvery==0)saveCheckpoint(gen);reproduce(gen);}
    auto finalEval=evaluateBalance(currentBalance_,bestStrategy_,config_.validationRuns,config_.validationSeed+999999999ULL);bestStrategy_.toJson().writeFile((outDir_/"best_strategy.json").string(),2);currentBalance_.toJson().writeFile((outDir_/"current_balance.json").string(),2);writeSummary(lastGen,finalEval);saveCheckpoint(lastGen);std::cout<<"Fertig. Reports: "<<outDir_<<"\n";
}

} // namespace gba
