#include <retesteth/configs/Genesis.h>
#include <string>
using namespace std;

string default_EIP158ToByzantiumAt5_config = R"({
    "params" : {
        "homesteadForkBlock" : "0x00",
        "EIP150ForkBlock" : "0x00",
        "EIP158ForkBlock" : "0x00",
        "byzantiumForkBlock" : "0x05"
    }
})";
