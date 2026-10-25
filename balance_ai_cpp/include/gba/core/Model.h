#pragma once
#include "gba/core/Json.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gba {

struct ParameterSpec {
    std::string key;
    double defaultValue = 1.0;
    double minValue = 0.1;
    double maxValue = 10.0;
    double mutationScale = 0.15;
    bool integer = false;
    std::string category;
};

struct GameDefinition {
    std::string name;
    std::string genre;
    Json raw;
};

struct Genome {
    std::uint64_t id = 0;
    int generation = 0;
    std::map<std::string, double> genes;
    double fitness = -1e100;
    double validationFitness = -1e100;
    std::string note;

    double get(const std::string& key, double fallback = 1.0) const;
    void set(const std::string& key, double value);
    Json toJson() const;
    static Genome fromJson(const Json& j);
};

struct SimulationResult {
    std::uint64_t strategyId = 0;
    std::uint64_t balanceId = 0;
    std::uint64_t seed = 0;
    double progress = 0.0;
    double score = 0.0;
    double earlySafety = 1.0;
    double resourceIncome = 0.0;
    double resourceResidual = 0.0;
    bool success = false;
    std::map<std::string, double> metrics;
    std::map<std::string, double> usage;
    std::map<std::string, double> contribution;
};

struct EvaluationSummary {
    double fitness = -1e100;
    double avgProgress = 0.0;
    double p10Progress = 0.0;
    double successRate = 0.0;
    double earlySafety = 0.0;
    double topShare = 0.0;
    double topPairShare = 0.0;
    double diversity = 0.0;
    double wasteRatio = 0.0;
    std::map<std::string, double> meanMetrics;
    std::map<std::string, double> meanUsage;
    std::map<std::string, double> meanContribution;
    std::string explanation;
};

struct TrainerConfig {
    int generations = 120;
    int population = 80;
    int elite = 8;
    double mutationRate = 0.20;
    double mutationStrength = 0.20;
    double minMutationStrength = 0.08;
    double maxMutationStrength = 0.38;
    double randomShare = 0.12;
    double diversityPressure = 0.12;
    int trainRuns = 5;
    int validationRuns = 24;
    int balanceEvery = 5;
    int balancePopulation = 14;
    int balanceFinalists = 3;
    int adaptationCandidates = 8;
    int adaptationRuns = 3;
    double balanceMutationStrength = 0.28;
    int balanceChangesPerGenome = 4;
    double minBalanceImprovement = 0.002;
    int threads = 4;
    std::uint64_t trainingSeed = 1337;
    std::uint64_t validationSeed = 9000001;
    bool resume = false;
    bool sensitivity = true;
    int sensitivityEvery = 25;
    int hallOfFameSize = 12;
    bool adversarialValidation = true;
    int adversarialEvery = 5;
    int adversarialCandidateSeeds = 20;
    int adversarialWorstSeeds = 5;
    int checkpointEvery = 5;
    int maxProgress = 220;
    int timeLimitMinutes = 0; // 0 = kein Zeitlimit

    static TrainerConfig fromJson(const Json& j);
    Json toJson() const;
};

GameDefinition loadGameDefinition(const std::string& path);

} // namespace gba
