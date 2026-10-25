#pragma once
#include "gba/core/GameModule.h"
namespace gba {
class AutoBattlerModule final : public IGameModule {
public:
    std::string genre() const override;
    std::vector<ParameterSpec> balanceParameters(const GameDefinition& game) const override;
    std::vector<ParameterSpec> strategyParameters(const GameDefinition& game) const override;
    SimulationResult simulate(const GameDefinition& game,const Genome& balance,const Genome& strategy,std::uint64_t seed,int maxProgress) const override;
    EvaluationSummary evaluateBalance(const GameDefinition& game,const Genome& balance,const std::vector<SimulationResult>& runs) const override;
    std::vector<Genome> targetedBalanceExperiments(const GameDefinition& game,const Genome& base,const EvaluationSummary& current,std::uint64_t& nextId,int generation,std::mt19937_64& rng,double strength) const override;
};
}
