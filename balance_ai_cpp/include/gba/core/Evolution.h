#pragma once
#include "gba/core/GameModule.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <random>

namespace gba {

class EvolutionEngine {
public:
    EvolutionEngine(GameDefinition game, std::unique_ptr<IGameModule> module, TrainerConfig config, std::filesystem::path outDir);
    void run();
    const Genome& bestStrategy() const { return bestStrategy_; }
    const Genome& currentBalance() const { return currentBalance_; }

private:
    GameDefinition game_;
    std::unique_ptr<IGameModule> module_;
    TrainerConfig config_;
    std::filesystem::path outDir_;
    std::vector<ParameterSpec> strategySpecs_;
    std::vector<ParameterSpec> balanceSpecs_;
    std::vector<Genome> population_;
    std::vector<Genome> hallOfFame_;
    Genome currentBalance_;
    Genome bestStrategy_;
    std::uint64_t nextGenomeId_ = 1;
    int startGeneration_ = 1;
    double mutationStrength_ = 0.2;
    int staleGenerations_ = 0;
    std::mt19937_64 rng_;
    std::map<std::uint64_t,double> lastRawFitness_;
    std::map<std::uint64_t,double> lastNovelty_;

    Genome makeBase(const std::vector<ParameterSpec>& specs, int generation, const std::string& note);
    Genome randomGenome(const std::vector<ParameterSpec>& specs, int generation, const std::string& note);
    Genome mutateGenome(const Genome& parent, const std::vector<ParameterSpec>& specs, int generation, double strength, int changes, const std::string& note);
    void clampGenome(Genome& g, const std::vector<ParameterSpec>& specs) const;

    EvaluationSummary evaluateStrategy(Genome& strategy, const Genome& balance, int runs, std::uint64_t seedBase, bool validation=false) const;
    EvaluationSummary evaluateBalance(const Genome& balance, const Genome& strategy, int runs, std::uint64_t seedBase) const;
    std::vector<SimulationResult> runBatch(const Genome& balance, const Genome& strategy, int runs, std::uint64_t seedBase) const;
    void evaluatePopulation(int generation);
    void reproduce(int generation);
    void updateHallOfFame(const Genome& champion);
    void balanceCycle(int generation);
    Genome adaptStrategyForBalance(const Genome& balance, int generation, double& outFitness);
    void sensitivityAnalysis(int generation);
    void writeGeneration(int generation, const EvaluationSummary& championSummary);
    void writePareto(int generation);
    void writeExploit(int generation, const EvaluationSummary& summary);
    void adversarialValidation(Genome& strategy, int generation);
    void writeBalanceCandidate(int generation, const Genome& g, const EvaluationSummary& s, bool accepted);
    void writeSummary(int generation, const EvaluationSummary& finalEval);
    void saveCheckpoint(int generation) const;
    bool loadCheckpoint();
};

} // namespace gba
