#pragma once
#include "../common.h"
#include <retesteth/Options.h>
#include <retesteth/TestHelper.h>

using namespace test;
namespace testprivate
{
class scheme_RPCTestBase : public object
{
public:
    scheme_RPCTestBase(DataObject const& _test) :
        object(_test)
    {
        requireJsonFields(_test, "rpcTest " + _test.getKey(),
            {{"request_method", {{DataType::String}, jsonField::Required}},
             {"request_params", {{DataType::Array}, jsonField::Required}},
             {"expect_return", {{DataType::Object}, jsonField::Required}},
             {"genesis", {{DataType::Object}, jsonField::Optional}},
             {"sealEngine", {{DataType::String}, jsonField::Optional}},
             {"pre", {{DataType::Object}, jsonField::Optional}}
                          });
        if (_test.count("pre") || _test.count("sealEngine") || _test.count("genesis"))
        {
            if (!_test.count("pre") && !_test.count("sealEngine") && !_test.count("genesis"))
                ETH_STDERROR_MESSAGE(TestInfo::caseName() +
                                     " A genesis information require 'pre', 'sealEngine' and "
                                     "'genesis' sections to be defined!");

            string sealEngine = _test.atKey("sealEngine").asString();
            scheme_env env(_test.atKey("genesis"));
            scheme_state pre(_test.atKey("pre"));
            string network = "Frontier";

            m_genesis.clear();
            m_genesis["sealEngine"] = sealEngine;
            m_genesis["genesis"] = env.getDataForRPC();
            for (auto const& acc : pre.getData().getSubObjects())
                m_genesis["alloc"].addSubObject(acc);
            m_genesis["Config"] = prepareGenesisParams(network);
        }
    }

    bool hasGenesis() const {
        return m_genesis.getSubObjects().size() > 0;
    }

    DataObject const& getGenesisForRPC() const {
        return m_genesis;
    }

    std::string get_method() { return m_data.atKey("request_method").asString(); }
    std::vector<std::string> get_params()
    {
        // no static here because of the threads
        std::vector<std::string> params;
        for (auto const& d : m_data.atKey("request_params").getSubObjects())
        {
            if (d.type() == DataType::String)
                params.push_back("\"" + d.asString() + "\"");
            else if (d.type() == DataType::Bool)
            {
                if (d == true)
                    params.push_back("true");
                else
                    params.push_back("false");
            }
        }
        return params;
    }
private:
    string m_network;
    DataObject m_genesis;
};
}

using namespace testprivate;
namespace test
{
class scheme_RPCTest : public scheme_RPCTestBase
{
public:
    scheme_RPCTest(DataObject const& _test) : scheme_RPCTestBase(_test){}
};

class scheme_RPCTestFiller : public scheme_RPCTestBase
{
public:
    scheme_RPCTestFiller(DataObject const& _test) : scheme_RPCTestBase(_test){}
    DataObject const& get_expectReturn() { return m_data.atKey("expect_return"); }
};
}
