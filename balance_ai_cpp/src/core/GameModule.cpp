#include "gba/core/GameModule.h"
#include "gba/genres/TowerDefenseModule.h"
#include "gba/genres/ArenaSurvivalModule.h"
#include "gba/genres/DeckbuilderModule.h"
#include "gba/genres/RoguelikeModule.h"
#include "gba/genres/TurnStrategyModule.h"
#include "gba/genres/AutoBattlerModule.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace gba {

EvaluationSummary IGameModule::evaluateBalance(const GameDefinition&, const Genome&, const std::vector<SimulationResult>& runs) const {
    EvaluationSummary e;
    if (runs.empty()) return e;
    std::vector<double> progress; progress.reserve(runs.size());
    double totalContribution = 0.0;
    for (const auto& r : runs) {
        e.avgProgress += r.progress;
        e.successRate += r.success ? 1.0 : 0.0;
        e.earlySafety += r.earlySafety;
        e.wasteRatio += r.resourceIncome > 0 ? std::max(0.0, r.resourceResidual) / r.resourceIncome : 0.0;
        progress.push_back(r.progress);
        for (const auto& [k,v] : r.metrics) e.meanMetrics[k] += v;
        for (const auto& [k,v] : r.usage) e.meanUsage[k] += v;
        for (const auto& [k,v] : r.contribution) { e.meanContribution[k] += v; totalContribution += std::max(0.0,v); }
    }
    const double n = static_cast<double>(runs.size());
    e.avgProgress/=n; e.successRate/=n; e.earlySafety/=n; e.wasteRatio/=n;
    for (auto& [k,v] : e.meanMetrics) v/=n;
    for (auto& [k,v] : e.meanUsage) v/=n;
    for (auto& [k,v] : e.meanContribution) v/=n;
    std::sort(progress.begin(),progress.end());
    e.p10Progress=progress[static_cast<std::size_t>(std::floor((progress.size()-1)*0.10))];
    std::vector<double> shares;
    double contribMeanTotal=0; for (const auto& [k,v]:e.meanContribution) contribMeanTotal += std::max(0.0,v);
    for (const auto& [k,v]:e.meanContribution) shares.push_back(contribMeanTotal>0?std::max(0.0,v)/contribMeanTotal:0.0);
    std::sort(shares.rbegin(),shares.rend());
    e.topShare=shares.empty()?0:shares[0];
    e.topPairShare=shares.size()<2?e.topShare:shares[0]+shares[1];
    int used=0; for(const auto& [k,v]:e.meanUsage) if(v>0.05) used++;
    e.diversity=e.meanUsage.empty()?0:used/static_cast<double>(e.meanUsage.size());
    e.fitness = e.avgProgress*40 + e.p10Progress*15 + e.successRate*1200 + e.earlySafety*1800 + e.diversity*600;
    e.fitness -= std::max(0.0,e.topShare-0.55)*4500;
    e.fitness -= std::max(0.0,e.topPairShare-0.80)*5500;
    e.fitness -= std::max(0.0,e.wasteRatio-0.22)*1200;
    std::ostringstream s; s<<"avg="<<e.avgProgress<<" p10="<<e.p10Progress<<" success="<<e.successRate<<" early="<<e.earlySafety<<" top="<<e.topShare<<" duo="<<e.topPairShare<<" div="<<e.diversity<<" waste="<<e.wasteRatio;
    e.explanation=s.str();
    return e;
}

double IGameModule::strategyFitness(const GameDefinition&, const SimulationResult& r) const {
    return r.progress*100 + r.score + (r.success?1000:0) + r.earlySafety*250 - std::max(0.0,r.resourceResidual)*0.002;
}

std::vector<Genome> IGameModule::targetedBalanceExperiments(const GameDefinition&, const Genome&, const EvaluationSummary&, std::uint64_t&, int, std::mt19937_64&, double) const { return {}; }

std::unique_ptr<IGameModule> createGameModule(const std::string& genre) {
    if(genre=="tower_defense") return std::make_unique<TowerDefenseModule>();
    if(genre=="arena_survival") return std::make_unique<ArenaSurvivalModule>();
    if(genre=="deckbuilder") return std::make_unique<DeckbuilderModule>();
    if(genre=="roguelike") return std::make_unique<RoguelikeModule>();
    if(genre=="turn_strategy") return std::make_unique<TurnStrategyModule>();
    if(genre=="auto_battler") return std::make_unique<AutoBattlerModule>();
    throw std::runtime_error("Unbekanntes Genre: "+genre);
}
std::vector<std::string> supportedGenres(){ return {"tower_defense","arena_survival","deckbuilder","roguelike","turn_strategy","auto_battler"}; }

} // namespace gba
