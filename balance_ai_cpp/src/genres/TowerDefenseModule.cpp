#include "gba/genres/TowerDefenseModule.h"
#include "gba/genres/GenreUtil.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace gba {
using namespace genreutil;

std::string TowerDefenseModule::genre() const{return "tower_defense";}

std::vector<ParameterSpec> TowerDefenseModule::balanceParameters(const GameDefinition& game) const{
    std::vector<ParameterSpec> p;
    auto towers=arrayObjects(game.raw,"towers");
    for(const auto&t:towers){std::string id=str(t,"id");
        p.push_back(spec("tower."+id+".costMul",1,.45,2.2,.18,false,"tower"));
        p.push_back(spec("tower."+id+".powerMul",1,.35,2.8,.20,false,"tower"));
        p.push_back(spec("tower."+id+".rateMul",1,.55,1.8,.16,false,"tower"));
        p.push_back(spec("tower."+id+".rangeMul",1,.70,1.45,.12,false,"tower"));
        p.push_back(spec("tower."+id+".roleMul",1,.45,3.0,.22,false,"role"));
        p.push_back(spec("tower."+id+".upgradeCostMul",1,.45,2.4,.18,false,"upgrade"));
        p.push_back(spec("tower."+id+".upgradePowerMul",1,.45,2.5,.20,false,"upgrade"));
    }
    auto enemies=arrayObjects(game.raw,"enemies");
    for(const auto&e:enemies){std::string id=str(e,"id");
        p.push_back(spec("enemy."+id+".hpMul",1,.45,2.8,.18,false,"enemy"));
        p.push_back(spec("enemy."+id+".speedMul",1,.65,1.6,.12,false,"enemy"));
        p.push_back(spec("enemy."+id+".rewardMul",1,.45,1.8,.14,false,"economy"));
        p.push_back(spec("enemy."+id+".shareMul",1,.2,2.5,.20,false,"spawn"));
        int fw=static_cast<int>(attr(e,"firstWave",1));
        p.push_back(spec("enemy."+id+".firstWave",fw,std::max(1,fw-25),fw+35,.08,true,"timing"));
    }
    p.push_back(spec("phase.early.hpMul",1,.65,1.5,.14,false,"phase"));p.push_back(spec("phase.early.countMul",1,.7,1.5,.14,false,"phase"));
    p.push_back(spec("phase.mid.hpMul",1,.55,1.8,.16,false,"phase"));p.push_back(spec("phase.mid.countMul",1,.6,1.8,.16,false,"phase"));
    p.push_back(spec("phase.late.hpMul",1,.45,2.2,.18,false,"phase"));p.push_back(spec("phase.late.countMul",1,.55,2.1,.18,false,"phase"));
    p.push_back(spec("phase.end.hpMul",1,.45,2.5,.20,false,"phase"));p.push_back(spec("phase.end.countMul",1,.55,2.2,.18,false,"phase"));
    p.push_back(spec("bank.costMul",1,.5,2.0,.15,false,"economy"));p.push_back(spec("bank.incomeMul",1,.45,2.2,.18,false,"economy"));
    p.push_back(spec("global.rewardMul",1,.6,1.5,.12,false,"economy"));
    return p;
}

std::vector<ParameterSpec> TowerDefenseModule::strategyParameters(const GameDefinition& game) const{
    std::vector<ParameterSpec> p;auto towers=arrayObjects(game.raw,"towers");
    for(const auto&t:towers){std::string id=str(t,"id");p.push_back(spec("pref."+id,1,.02,4.0,.22,false,"preference"));p.push_back(spec("upgrade."+id,.7,0,2.5,.18,false,"preference"));}
    p.push_back(spec("bankBias",.45,0,1.5,.18));p.push_back(spec("reserveBias",.18,0,.75,.16));p.push_back(spec("expensiveBias",.9,.25,2.0,.18));p.push_back(spec("adaptRole",1,.1,2.5,.20));p.push_back(spec("sellBankWave",165,80,215,.08,true));
    return p;
}

SimulationResult TowerDefenseModule::simulate(const GameDefinition& game,const Genome& balance,const Genome& strategy,std::uint64_t seed,int maxProgress) const{
    std::mt19937_64 rng(seed);SimulationResult r;r.strategyId=strategy.id;r.balanceId=balance.id;r.seed=seed;
    auto towers=arrayObjects(game.raw,"towers");auto enemies=arrayObjects(game.raw,"enemies");const Json&rules=game.raw["rules"];
    double money=attr(rules,"startMoney",130),lives=attr(rules,"startLives",30),startLives=lives;double income=money;double bankCount=0;double bankCost=attr(rules,"bankCost",250)*gene(balance,"bank.costMul");double bankIncome=attr(rules,"bankIncome",35)*gene(balance,"bank.incomeMul");double countBase=attr(rules,"enemyCountBase",8),countGrowth=attr(rules,"enemyCountGrowth",.62),countQuadratic=attr(rules,"enemyCountQuadratic",0.0);double hpGrowth=attr(rules,"enemyHpGrowth",.034);double pathPressure=attr(rules,"pathPressure",1.0);int slots=static_cast<int>(attr(rules,"maxTowerSlots",42));
    std::map<std::string,double> counts,levels;double totalTowerCount=0;double earlyLife=startLives;bool died=false;
    for(int wave=1;wave<=maxProgress;++wave){
        double phaseHp=1,phaseCount=1;if(wave<=5){phaseHp=gene(balance,"phase.early.hpMul");phaseCount=gene(balance,"phase.early.countMul");}else if(wave<=60){phaseHp=gene(balance,"phase.mid.hpMul");phaseCount=gene(balance,"phase.mid.countMul");}else if(wave<=180){phaseHp=gene(balance,"phase.late.hpMul");phaseCount=gene(balance,"phase.late.countMul");}else{phaseHp=gene(balance,"phase.end.hpMul");phaseCount=gene(balance,"phase.end.countMul");}
        double enemyCount=(countBase+countGrowth*wave+countQuadratic*wave*wave)*phaseCount*(.94+.12*rand01(rng));
        double armor=0,crowd=clamp01((enemyCount-25)/45.0),boss=0,hidden=0,fast=0;double enemyEhp=0,rewardPool=0,shareSum=0;
        struct ET{Json j;double share;};std::vector<ET> active;
        for(const auto&e:enemies){std::string id=str(e,"id");int fw=static_cast<int>(std::round(gene(balance,"enemy."+id+".firstWave",attr(e,"firstWave",1))));if(wave<fw)continue;double sh=std::max(.001,attr(e,"share",1)*gene(balance,"enemy."+id+".shareMul"));active.push_back({e,sh});shareSum+=sh;}
        if(active.empty()&&!enemies.empty()){active.push_back({enemies.front(),1});shareSum=1;}
        for(const auto&x:active){const auto&e=x.j;std::string id=str(e,"id"),role=str(e,"role","normal");double frac=x.share/shareSum;double n=enemyCount*frac;double hp=attr(e,"hp",10)*gene(balance,"enemy."+id+".hpMul")*phaseHp*std::pow(1+hpGrowth,std::max(0,wave-1));double speed=attr(e,"speed",1)*gene(balance,"enemy."+id+".speedMul");double threat=1+std::max(0.0,speed-1)*.35;enemyEhp+=n*hp*threat;rewardPool+=n*attr(e,"reward",1)*gene(balance,"enemy."+id+".rewardMul")*gene(balance,"global.rewardMul");if(role=="armor")armor+=frac;if(role=="boss")boss+=frac;if(role=="hidden")hidden+=frac;if(role=="fast")fast+=frac;}
        if(wave%static_cast<int>(attr(rules,"bossInterval",20))==0)boss=std::max(boss,.4);

        // Economy decision before combat. Banks are attractive while the horizon is long.
        int sellWave=static_cast<int>(strategy.get("sellBankWave",165));if(wave>=sellWave&&bankCount>0){money+=bankCount*bankCost*.55;bankCount=0;}
        double bankDesire=strategy.get("bankBias",.45)*(1-wave/static_cast<double>(std::max(1,maxProgress)))*1.15;
        while(wave>7&&wave<sellWave&&money>bankCost*1.15&&bankCount<8&&bankDesire>(.55+.12*bankCount)){money-=bankCost;bankCount++;r.usage["bank"]+=1;}
        double reserve=money*std::clamp(strategy.get("reserveBias",.18),0.0,.75);int purchases=0;
        while(totalTowerCount<slots&&money>20&&purchases<12){
            const Json*best=nullptr;double bestUtility=-1;double bestCost=0;std::string bestId;
            for(const auto&t:towers){std::string id=str(t,"id"),role=str(t,"role","allround");double cost=attr(t,"cost",100)*gene(balance,"tower."+id+".costMul");if(cost>money-reserve)continue;double rm=roleMatch(role,armor,crowd,boss,hidden,fast);double pref=strategy.get("pref."+id,1);double expensive=std::pow(std::max(.25,cost/100.0),.10*(strategy.get("expensiveBias",.9)-1));double util=pref*std::pow(rm,strategy.get("adaptRole",1))*expensive*attr(t,"power",10)*gene(balance,"tower."+id+".powerMul")/std::max(1.0,cost);util*=.92+rand01(rng)*.16;if(util>bestUtility){bestUtility=util;best=&t;bestCost=cost;bestId=id;}}
            if(!best) break;
            money-=bestCost; counts[bestId]+=1; totalTowerCount++; r.usage[bestId]+=1; purchases++;
            if(purchases>2&&rand01(rng)<.25)break;
        }
        // Upgrades: abstract three levels per tower type.
        for(const auto&t:towers){std::string id=str(t,"id");double c=counts[id];if(c<=0)continue;double lv=levels[id];if(lv>=3)continue;double baseCost=attr(t,"upgradeCost",attr(t,"cost",100)*1.5);double uc=baseCost*std::pow(2.2,lv)*gene(balance,"tower."+id+".upgradeCostMul");double desire=strategy.get("upgrade."+id,.7)*(1+.25*lv);if(money-reserve>uc&&desire>.45+.13*lv){money-=uc;levels[id]++;r.usage[id+".upgrade"]+=1;}}

        double capacity=0;double supportBonus=1;for(const auto&t:towers){std::string id=str(t,"id"),role=str(t,"role","allround");double c=counts[id];if(c<=0)continue;double lv=levels[id];double rm=roleMatch(role,armor,crowd,boss,hidden,fast);double up=1+lv*.42*gene(balance,"tower."+id+".upgradePowerMul");double power=attr(t,"power",10)*gene(balance,"tower."+id+".powerMul")*gene(balance,"tower."+id+".rateMul")*up;double rolePower=std::pow(rm,gene(balance,"tower."+id+".roleMul"));double range=std::pow(gene(balance,"tower."+id+".rangeMul"),.35);double contrib=c*power*rolePower*range;if(role=="support"){supportBonus+=std::min(.55,contrib/(1200+contrib)*.18);contrib*=.35;}capacity+=contrib;r.contribution[id]+=contrib;}
        capacity*=supportBonus*(8.0+wave*.065); // converts abstract tower DPS to per-wave capacity
        double ratio=capacity/std::max(1.0,enemyEhp*pathPressure);double killed=clamp01(ratio);double leak=1-killed;if(leak>0){double damage=std::ceil(leak*enemyCount*(.12+.012*wave));lives-=damage;}
        double earned=rewardPool*killed;money+=earned;income+=earned;
        if(bankCount>0){double bi=bankCount*bankIncome*(1+std::min(2.5,wave/80.0));money+=bi;income+=bi;r.metrics["bankIncome"]+=bi;}
        r.metrics["enemyEhp"]+=enemyEhp;r.metrics["capacity"]+=capacity;r.metrics["armorPressure"]+=armor;r.metrics["crowdPressure"]+=crowd;
        if(wave==5)earlyLife=std::max(0.0,lives);
        if(lives<=0){r.progress=wave;r.success=false;died=true;break;}r.progress=wave;
    }
    if(!died){r.success=r.progress>=maxProgress;}r.earlySafety=clamp01(earlyLife/std::max(1.0,startLives));r.resourceIncome=income;r.resourceResidual=money;r.score=r.progress*25+r.earlySafety*600+r.success*1200;
    return r;
}

EvaluationSummary TowerDefenseModule::evaluateBalance(const GameDefinition& game,const Genome& balance,const std::vector<SimulationResult>& runs) const{
    EvaluationSummary e=IGameModule::evaluateBalance(game,balance,runs);double target=game.raw["targets"]["standardProgress"].asNumber(180);double softMax=game.raw["targets"]["standardSoftMax"].asNumber(202);
    e.fitness += e.earlySafety*3200;if(e.earlySafety<.94)e.fitness-=(.94-e.earlySafety)*9500;
    if(e.avgProgress<target*.68)e.fitness-=(target*.68-e.avgProgress)*70;else e.fitness+=std::min(e.avgProgress,target+10)*18;
    if(e.avgProgress>softMax)e.fitness-=(e.avgProgress-softMax)*55;
    e.fitness-=std::max(0.0,e.topShare-.50)*5000;e.fitness-=std::max(0.0,e.topPairShare-.78)*7000;
    double bank=e.meanUsage.count("bank")?e.meanUsage.at("bank"):0;if(bank>0)e.fitness+=180;
    std::ostringstream s;s<<"TD target="<<e.fitness<<" avgW="<<e.avgProgress<<" p10="<<e.p10Progress<<" early="<<e.earlySafety<<" top="<<e.topShare<<" duo="<<e.topPairShare<<" div="<<e.diversity; e.explanation=s.str();return e;
}

std::vector<Genome> TowerDefenseModule::targetedBalanceExperiments(const GameDefinition& game,const Genome& base,const EvaluationSummary& current,std::uint64_t& nextId,int generation,std::mt19937_64& rng,double strength) const{
    std::vector<Genome> out;auto towers=arrayObjects(game.raw,"towers");if(towers.empty())return out;
    std::string dominant,alternative;double hi=-1,lo=1e100;for(const auto&t:towers){std::string id=str(t,"id");double c=current.meanContribution.count(id)?current.meanContribution.at(id):0;if(c>hi){hi=c;dominant=id;}double u=current.meanUsage.count(id)?current.meanUsage.at(id):0;if(u<lo){lo=u;alternative=id;}}
    if(!dominant.empty()&&!alternative.empty()&&dominant!=alternative){Genome g=base;g.id=nextId++;g.generation=generation;g.note="anti-meta:"+dominant+"->"+alternative;g.genes["tower."+dominant+".powerMul"]*=std::max(.55,1-strength*.45);g.genes["tower."+dominant+".costMul"]*=1+strength*.12;g.genes["tower."+alternative+".powerMul"]*=1+strength*.35;g.genes["tower."+alternative+".roleMul"]*=1+strength*.40;out.push_back(std::move(g));}
    // Role-specialization experiments: generic power traded for role power.
    std::uniform_int_distribution<std::size_t> pick(0,towers.size()-1);for(int k=0;k<2;++k){auto t=towers[pick(rng)];std::string id=str(t,"id");Genome g=base;g.id=nextId++;g.generation=generation;g.note="role-specialize:"+id;g.genes["tower."+id+".powerMul"]*=1-strength*.18;g.genes["tower."+id+".roleMul"]*=1+strength*.70;out.push_back(std::move(g));}
    return out;
}

} // namespace gba
