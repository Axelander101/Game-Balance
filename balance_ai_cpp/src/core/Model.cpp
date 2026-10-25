#include "gba/core/Model.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gba {

double Genome::get(const std::string& key, double fallback) const {
    auto it = genes.find(key);
    return it == genes.end() ? fallback : it->second;
}
void Genome::set(const std::string& key, double value) { genes[key] = value; }
Json Genome::toJson() const {
    Json::Object g;
    for (const auto& [k, v] : genes) g[k] = v;
    return Json::Object{{"id", static_cast<double>(id)}, {"generation", generation}, {"fitness", fitness}, {"validationFitness", validationFitness}, {"note", note}, {"genes", Json(std::move(g))}};
}
Genome Genome::fromJson(const Json& j) {
    Genome g;
    g.id = static_cast<std::uint64_t>(j["id"].asLong());
    g.generation = j["generation"].asInt();
    g.fitness = j["fitness"].asNumber(-1e100);
    g.validationFitness = j["validationFitness"].asNumber(-1e100);
    g.note = j["note"].asString();
    if (j["genes"].isObject()) for (const auto& [k, v] : j["genes"].asObject()) g.genes[k] = v.asNumber();
    return g;
}

static int jint(const Json& j, const char* k, int d) { return j[k].isNumber() ? j[k].asInt() : d; }
static double jdbl(const Json& j, const char* k, double d) { return j[k].isNumber() ? j[k].asNumber() : d; }
static bool jbool(const Json& j, const char* k, bool d) { return j[k].isBool() ? j[k].asBool() : d; }
static std::uint64_t ju64(const Json& j, const char* k, std::uint64_t d) { return j[k].isNumber() ? static_cast<std::uint64_t>(j[k].asLong()) : d; }

TrainerConfig TrainerConfig::fromJson(const Json& j) {
    TrainerConfig c;
    c.generations = std::max(1, jint(j,"generations",c.generations));
    c.population = std::max(4, jint(j,"population",c.population));
    c.elite = std::clamp(jint(j,"elite",c.elite), 1, c.population - 1);
    c.mutationRate = std::clamp(jdbl(j,"mutationRate",c.mutationRate),0.01,1.0);
    c.mutationStrength = std::clamp(jdbl(j,"mutationStrength",c.mutationStrength),0.001,2.0);
    c.minMutationStrength = std::clamp(jdbl(j,"minMutationStrength",c.minMutationStrength),0.001,2.0);
    c.maxMutationStrength = std::max(c.minMutationStrength,jdbl(j,"maxMutationStrength",c.maxMutationStrength));
    c.randomShare = std::clamp(jdbl(j,"randomShare",c.randomShare),0.0,0.8);
    c.diversityPressure = std::clamp(jdbl(j,"diversityPressure",c.diversityPressure),0.0,2.0);
    c.trainRuns = std::max(1,jint(j,"trainRuns",c.trainRuns));
    c.validationRuns = std::max(c.trainRuns,jint(j,"validationRuns",c.validationRuns));
    c.balanceEvery = std::max(1,jint(j,"balanceEvery",c.balanceEvery));
    c.balancePopulation = std::max(3,jint(j,"balancePopulation",c.balancePopulation));
    c.balanceFinalists = std::clamp(jint(j,"balanceFinalists",c.balanceFinalists),1,c.balancePopulation);
    c.adaptationCandidates = std::max(2,jint(j,"adaptationCandidates",c.adaptationCandidates));
    c.adaptationRuns = std::max(1,jint(j,"adaptationRuns",c.adaptationRuns));
    c.balanceMutationStrength = std::clamp(jdbl(j,"balanceMutationStrength",c.balanceMutationStrength),0.01,2.0);
    c.balanceChangesPerGenome = std::max(1,jint(j,"balanceChangesPerGenome",c.balanceChangesPerGenome));
    c.minBalanceImprovement = std::max(0.0,jdbl(j,"minBalanceImprovement",c.minBalanceImprovement));
    c.threads = std::max(1,jint(j,"threads",c.threads));
    c.trainingSeed = ju64(j,"trainingSeed",c.trainingSeed);
    c.validationSeed = ju64(j,"validationSeed",c.validationSeed);
    c.resume = jbool(j,"resume",c.resume);
    c.sensitivity = jbool(j,"sensitivity",c.sensitivity);
    c.sensitivityEvery = std::max(1,jint(j,"sensitivityEvery",c.sensitivityEvery));
    c.hallOfFameSize = std::max(1,jint(j,"hallOfFameSize",c.hallOfFameSize));
    c.adversarialValidation = jbool(j,"adversarialValidation",c.adversarialValidation);
    c.adversarialEvery = std::max(1,jint(j,"adversarialEvery",c.adversarialEvery));
    c.adversarialCandidateSeeds = std::max(1,jint(j,"adversarialCandidateSeeds",c.adversarialCandidateSeeds));
    c.adversarialWorstSeeds = std::clamp(jint(j,"adversarialWorstSeeds",c.adversarialWorstSeeds),1,c.adversarialCandidateSeeds);
    c.checkpointEvery = std::max(1,jint(j,"checkpointEvery",c.checkpointEvery));
    c.maxProgress = std::max(1,jint(j,"maxProgress",c.maxProgress));
    c.timeLimitMinutes = std::max(0,jint(j,"timeLimitMinutes",c.timeLimitMinutes));
    return c;
}
Json TrainerConfig::toJson() const {
    return Json::Object{
        {"generations",generations},{"population",population},{"elite",elite},{"mutationRate",mutationRate},{"mutationStrength",mutationStrength},{"minMutationStrength",minMutationStrength},{"maxMutationStrength",maxMutationStrength},{"randomShare",randomShare},{"diversityPressure",diversityPressure},
        {"trainRuns",trainRuns},{"validationRuns",validationRuns},{"balanceEvery",balanceEvery},{"balancePopulation",balancePopulation},{"balanceFinalists",balanceFinalists},{"adaptationCandidates",adaptationCandidates},{"adaptationRuns",adaptationRuns},{"balanceMutationStrength",balanceMutationStrength},{"balanceChangesPerGenome",balanceChangesPerGenome},{"minBalanceImprovement",minBalanceImprovement},
        {"threads",threads},{"trainingSeed",static_cast<double>(trainingSeed)},{"validationSeed",static_cast<double>(validationSeed)},{"resume",resume},{"sensitivity",sensitivity},{"sensitivityEvery",sensitivityEvery},{"hallOfFameSize",hallOfFameSize},{"adversarialValidation",adversarialValidation},{"adversarialEvery",adversarialEvery},{"adversarialCandidateSeeds",adversarialCandidateSeeds},{"adversarialWorstSeeds",adversarialWorstSeeds},{"checkpointEvery",checkpointEvery},{"maxProgress",maxProgress},{"timeLimitMinutes",timeLimitMinutes}
    };
}

GameDefinition loadGameDefinition(const std::string& path) {
    Json j = Json::parseFile(path);
    GameDefinition d;
    d.name = j["name"].asString("Unnamed Game");
    d.genre = j["genre"].asString();
    if (d.genre.empty()) throw std::runtime_error("game.json braucht ein Feld 'genre'");
    d.raw = std::move(j);
    return d;
}

} // namespace gba
