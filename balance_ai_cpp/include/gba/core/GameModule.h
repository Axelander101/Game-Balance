#pragma once
#include "gba/core/Model.h"
#include <memory>
#include <random>

namespace gba {

class IGameModule {
public:
    virtual ~IGameModule() = default;
    virtual std::string genre() const = 0;
    virtual std::vector<ParameterSpec> balanceParameters(const GameDefinition& game) const = 0;
    virtual std::vector<ParameterSpec> strategyParameters(const GameDefinition& game) const = 0;
    virtual SimulationResult simulate(const GameDefinition& game, const Genome& balance, const Genome& strategy, std::uint64_t seed, int maxProgress) const = 0;
    virtual EvaluationSummary evaluateBalance(const GameDefinition& game, const Genome& balance, const std::vector<SimulationResult>& runs) const;
    virtual double strategyFitness(const GameDefinition& game, const SimulationResult& run) const;
    virtual std::vector<Genome> targetedBalanceExperiments(const GameDefinition& game, const Genome& base, const EvaluationSummary& current, std::uint64_t& nextId, int generation, std::mt19937_64& rng, double strength) const;
};

std::unique_ptr<IGameModule> createGameModule(const std::string& genre);
std::vector<std::string> supportedGenres();

} // namespace gba
