#pragma once
#include "../../../configs/FORK.h"
#include "../../../types/Ethereum/StateIncomplete.h"
#include <retesteth/dataObject/DataObject.h>
#include <retesteth/dataObject/SPointer.h>
#include <testStructures/types/StateTests/Filler/StateTestFillerTransaction.h>
using namespace dataobject;
using namespace test::teststruct;

namespace test
{
namespace teststruct
{
struct StateTestFillerExpectSection
{
    StateTestFillerExpectSection(spDataObjectMove, spStateTestFillerTransaction const&);
    StateIncomplete const& result() const { return m_result; }
    DataObject const& initialData() const { return m_initialData; }
    std::vector<FORK> const& forks() const { return m_forks; }
    bool hasFork(FORK const& _fork) const
    {
        for (auto const& el : m_forks)
            if (el == _fork)
                return true;
        return false;
    }

    // Check that this indexes are present in this expect section
    bool checkIndexes(size_t _dInd, size_t _gInd, size_t _vInd) const;
    void correctMiningReward(FH20 const& _coinbase, VALUE const& _reward);

    // Get expect exception for transaction
    string const& getExpectException(FORK const& _net) const
    {
        static string emptyString = string();  // mutex ??
        if (m_expectExceptions.count(_net))
            return m_expectExceptions.at(_net);
        return emptyString;
    }

private:
    std::set<int> m_dataInd;
    std::set<int> m_gasInd;
    std::set<int> m_valInd;
    std::vector<FORK> m_forks;
    GCP_SPointer<StateIncomplete> m_result;
    spDataObject m_initialData;

    std::map<FORK, string> m_expectExceptions;
};


}  // namespace teststruct
}  // namespace test
