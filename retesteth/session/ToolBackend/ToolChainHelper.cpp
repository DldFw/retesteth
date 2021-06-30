#include "ToolChainHelper.h"
#include "ToolChainManager.h"
#include <retesteth/EthChecks.h>
#include <retesteth/Options.h>
#include <retesteth/testStructures/Common.h>
using namespace dev;
using namespace test;
using namespace teststruct;
using namespace dataobject;

namespace toolimpl
{
ToolParams::ToolParams(DataObject const& _data)
{
    const bigint unreachable = 10000000000;
    if (_data.count("homesteadForkBlock"))
        m_homesteadForkBlock = spVALUE(new VALUE(_data.atKey("homesteadForkBlock")));
    else
        m_homesteadForkBlock = spVALUE(new VALUE(unreachable));

    if (_data.count("byzantiumForkBlock"))
        m_byzantiumForkBlock = spVALUE(new VALUE(_data.atKey("byzantiumForkBlock")));
    else
        m_byzantiumForkBlock = spVALUE(new VALUE(unreachable));

    if (_data.count("constantinopleForkBlock"))
        m_constantinopleForkBlock = spVALUE(new VALUE(_data.atKey("constantinopleForkBlock")));
    else
        m_constantinopleForkBlock = spVALUE(new VALUE(unreachable));

    if (_data.count("muirGlacierForkBlock"))
        m_muirGlacierForkBlock = spVALUE(new VALUE(_data.atKey("muirGlacierForkBlock")));
    else
        m_muirGlacierForkBlock = spVALUE(new VALUE(unreachable));

    if (_data.count("londonForkBlock"))
        m_londonForkBlock = spVALUE(new VALUE(_data.atKey("londonForkBlock")));
    else
        m_londonForkBlock = spVALUE(new VALUE(unreachable));

    requireJsonFields(_data, "ToolParams " + _data.getKey(),
        {{"fork", {{DataType::String}, jsonField::Required}},
            {"muirGlacierForkBlock", {{DataType::String}, jsonField::Optional}},
            {"constantinopleForkBlock", {{DataType::String}, jsonField::Optional}},
            {"byzantiumForkBlock", {{DataType::String}, jsonField::Optional}},
            {"londonForkBlock", {{DataType::String}, jsonField::Optional}},
            {"homesteadForkBlock", {{DataType::String}, jsonField::Optional}}});
}

// We simulate the client backend side here, so thats why number5 is hardcoded
// Map rewards from non standard forks into standard
static std::map<FORK, FORK> RewardMapForToolBefore5 = {
    {"FrontierToHomesteadAt5", "Frontier"},
    {"HomesteadToEIP150At5", "Homestead"},
    {"EIP158ToByzantiumAt5", "EIP158"},
    {"HomesteadToDaoAt5", "Homestead"},
    {"ByzantiumToConstantinopleFixAt5", "Byzantium"},
    {"BerlinToLondonAt5", "Berlin"}
};
static std::map<FORK, FORK> RewardMapForToolAfter5 = {
    {"FrontierToHomesteadAt5", "Homestead"},
    {"HomesteadToEIP150At5", "EIP150"},
    {"EIP158ToByzantiumAt5", "Byzantium"},
    {"HomesteadToDaoAt5", "Homestead"},
    {"ByzantiumToConstantinopleFixAt5", "ConstantinopleFix"},
    {"BerlinToLondonAt5", "London"}
};

std::tuple<VALUE, FORK> prepareReward(SealEngine _engine, FORK const& _fork, VALUE const& _blockNumber)
{
    if (_engine == SealEngine::Ethash)
        ETH_WARNING_TEST("t8ntool backend treat Ethash as NoProof!", 6);

    // Setup mining rewards
    std::map<FORK, spVALUE> const& rewards = Options::get().getDynamicOptions().getCurrentConfig().getRewardMap();
    if (rewards.count(_fork))
        return {rewards.at(_fork).getCContent(), _fork};
    else
    {
        if (_blockNumber < 5)
        {
            if (!RewardMapForToolBefore5.count(_fork))
                ETH_ERROR_MESSAGE("ToolBackend error getting reward for fork: " + _fork.asString());
            auto const& trFork = RewardMapForToolBefore5.at(_fork);
            assert(rewards.count(trFork));
            return {rewards.at(trFork).getCContent(), trFork};
        }
        else
        {
            if (!RewardMapForToolAfter5.count(_fork))
                ETH_ERROR_MESSAGE("ToolBackend error getting reward for fork: " + _fork.asString());
            auto const& trFork = RewardMapForToolAfter5.at(_fork);
            assert(rewards.count(trFork));
            return {rewards.at(trFork).getCContent(), _fork == "HomesteadToDaoAt5" ? "HomesteadToDaoAt5" : trFork};
        }
    }
}

VALUE calculateGasLimit(VALUE const& _parentGasLimit, VALUE const& _parentGasUsed)
{
    static bigint gasFloorTarget = 3141562;  //_gasFloorTarget == Invalid256 ? 3141562 : _gasFloorTarget;
    bigint gasLimit = _parentGasLimit.asBigInt();
    static bigint boundDivisor = bigint("0x0400");
    if (gasLimit < gasFloorTarget)
        return min<bigint>(gasFloorTarget, gasLimit + gasLimit / boundDivisor - 1);
    else
        return max<bigint>(
            gasFloorTarget, gasLimit - gasLimit / boundDivisor + 1 + (_parentGasUsed.asBigInt() * 6 / 5) / boundDivisor);
}

// Because tool report incomplete state. restore missing fields with zeros
// Also remove leading zeros in storage
State restoreFullState(DataObject const& _toolState)
{
    DataObject fullState;
    for (auto const& accTool : _toolState.getSubObjects())
    {
        DataObject acc;
        acc["balance"] = accTool.count("balance") ? accTool.atKey("balance").asString() : "0x00";
        acc["nonce"] = accTool.count("nonce") ? accTool.atKey("nonce").asString() : "0x00";
        acc["code"] = accTool.count("code") ? accTool.atKey("code").asString() : "0x";
        acc["storage"] = accTool.count("storage") ? accTool.atKey("storage") : DataObject(DataType::Object);
        for (auto& storageRecord : acc.atKeyUnsafe("storage").getSubObjectsUnsafe())
        {
            storageRecord.performModifier(mod_removeLeadingZerosFromHexValuesEVEN);
            storageRecord.performModifier(mod_removeLeadingZerosFromHexKeysEVEN);
        }
        fullState[accTool.getKey()] = acc;
    }
    return State(fullState);
}

ChainOperationParams ChainOperationParams::defaultParams(ToolParams const& _params)
{
    ChainOperationParams aleth;
    aleth.durationLimit = u256("0x0d");
    aleth.minimumDifficulty = u256("0x20000");
    aleth.difficultyBoundDivisor = u256("0x0800");
    aleth.homesteadForkBlock = _params.homesteadForkBlock().asBigInt();
    aleth.byzantiumForkBlock = _params.byzantiumForkBlock().asBigInt();
    aleth.constantinopleForkBlock = _params.constantinopleForkBlock().asBigInt();
    aleth.muirGlacierForkBlock = _params.muirGlacierForkBlock().asBigInt();
    aleth.londonForkBlock = _params.londonForkBlock().asBigInt();
    return aleth;
}

// Aleth calculate difficulty formula
VALUE calculateEthashDifficulty(
    ChainOperationParams const& _chainParams, spBlockHeader const& _bi, spBlockHeader const& _parent)
{
    const unsigned c_expDiffPeriod = 100000;

    if (_bi->number() == 0)
        throw test::UpwardsException("calculateEthashDifficulty was called for block with number == 0");

    auto const& minimumDifficulty = _chainParams.minimumDifficulty;
    auto const& difficultyBoundDivisor = _chainParams.difficultyBoundDivisor;
    auto const& durationLimit = _chainParams.durationLimit;

    VALUE target(0);  // stick to a bigint for the target. Don't want to risk going negative.
    if (_bi->number() < _chainParams.homesteadForkBlock)
    {
        // Frontier-era difficulty adjustment
        target = _bi->timestamp() >= _parent->timestamp() + durationLimit ?
                     _parent->difficulty() - (_parent->difficulty() / difficultyBoundDivisor) :
                     (_parent->difficulty() + (_parent->difficulty() / difficultyBoundDivisor));
    }
    else
    {
        VALUE const timestampDiff = _bi->timestamp() - _parent->timestamp();
        VALUE const adjFactor =
            _bi->number() < _chainParams.byzantiumForkBlock ?
                max<bigint>(1 - timestampDiff.asBigInt() / 10, -99) :  // Homestead-era difficulty adjustment
                max<bigint>((_parent->hasUncles() ? 2 : 1) - timestampDiff.asBigInt() / 9,
                    -99);  // Byzantium-era difficulty adjustment

        target = _parent->difficulty() + _parent->difficulty() / 2048 * adjFactor;
    }

    VALUE o = target;
    unsigned exponentialIceAgeBlockNumber = (unsigned)_parent->number().asBigInt() + 1;

    // EIP-2384 Istanbul/Berlin Difficulty Bomb Delay
    if (_bi->number().asBigInt() >= _chainParams.muirGlacierForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 9000000)
            exponentialIceAgeBlockNumber -= 9000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }
    // EIP-1234 Constantinople Ice Age delay
    else if (_bi->number().asBigInt() >= _chainParams.constantinopleForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 5000000)
            exponentialIceAgeBlockNumber -= 5000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }
    // EIP-649 Byzantium Ice Age delay
    else if (_bi->number().asBigInt() >= _chainParams.byzantiumForkBlock)
    {
        if (exponentialIceAgeBlockNumber >= 3000000)
            exponentialIceAgeBlockNumber -= 3000000;
        else
            exponentialIceAgeBlockNumber = 0;
    }

    unsigned periodCount = exponentialIceAgeBlockNumber / c_expDiffPeriod;
    // latter will eventually become huge, so ensure it's a bigint.
    if (periodCount > 1)
        o += VALUE((bigint(1) << (periodCount - 2)));

    o = max<bigint>(minimumDifficulty, o.asBigInt());
    return o;  // bigint(min<bigint>(o, std::numeric_limits<bigint>::max()));
}


VALUE calculateEIP1559BaseFee(ChainOperationParams const& _chainParams, spBlockHeader const& _bi, spBlockHeader const& _parent)
{
    (void)_chainParams;
    (void)_bi;

    VALUE expectedBaseFee(0);
    BlockHeader1559 const& parent = BlockHeader1559::castFrom(_parent);

    VALUE const parentGasTarget = parent.gasLimit() / ELASTICITY_MULTIPLIER;

    if (_bi->number().asBigInt() == _chainParams.londonForkBlock)
        expectedBaseFee = INITIAL_BASE_FEE;
    else if (parent.gasUsed() == parentGasTarget)
        expectedBaseFee = parent.baseFee().asBigInt();
    else if (parent.gasUsed() > parentGasTarget)
    {
        VALUE gasUsedDelta = parent.gasUsed() - parentGasTarget;
        VALUE formula = parent.baseFee() * gasUsedDelta / parentGasTarget / BASE_FEE_MAX_CHANGE_DENOMINATOR;
        VALUE baseFeePerGasDelta = max<dev::bigint>(formula.asBigInt(), dev::bigint(1));
        expectedBaseFee = parent.baseFee() + baseFeePerGasDelta;
    }
    else
    {
        VALUE gasUsedDelta = parentGasTarget - parent.gasUsed();
        VALUE baseFeePerGasDelta = parent.baseFee() * gasUsedDelta / parentGasTarget / BASE_FEE_MAX_CHANGE_DENOMINATOR;
        VALUE formula = parent.baseFee() - baseFeePerGasDelta;
        expectedBaseFee = VALUE(max<dev::bigint>(formula.asBigInt(), dev::bigint(0))).asBigInt();
    }
    return expectedBaseFee;
}

}  // namespace toolimpl
