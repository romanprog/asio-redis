#include "../include/cmd_traits.hpp"


namespace redis {
namespace cmd {
namespace cluster {

const std::vector<std::string> cl_addslot::name {"CLUSTER", "ADDSLOTS"};

const std::vector<std::string> cl_cf_reports::name {"CLUSTER", "COUNT-FAILURE-REPORTS"};

const std::vector<std::string> cl_countkeysinslot::name {"CLUSTER", "COUNTKEYSINSLOT"};

const std::vector<std::string> cl_delslots::name {"CLUSTER", "DELSLOTS"};

const std::vector<std::string> cl_failover::name {"CLUSTER", "FAILOVER"};

const std::vector<std::string> cl_forget::name {"CLUSTER", "FORGET"};

const std::vector<std::string> cl_getkeysinslot::name {"CLUSTER", "GETKEYSINSLOT"};

const std::vector<std::string> cl_info::name {"CLUSTER", "INFO"};

const std::vector<std::string> cl_keyslot::name {"CLUSTER", "KEYSLOT"};

const std::vector<std::string> cl_meet::name {"CLUSTER", "MEET"};

const std::vector<std::string> cl_nodes::name {"CLUSTER", "NODES"};

const std::vector<std::string> cl_replicate::name {"CLUSTER", "REPLICATE"};

const std::vector<std::string> cl_reset::name {"CLUSTER", "RESET"};

const std::vector<std::string> cl_saveconfig::name {"CLUSTER", "SAVECONFIG"};

const std::vector<std::string> cl_set_config_epoch::name {"CLUSTER", "SET-CONFIG-EPOCH"};

const std::vector<std::string> cl_set_slot::name {"CLUSTER", "SETSLOT"};

const std::vector<std::string> cl_slaves::name {"CLUSTER", "SLAVES"};

} // namespace claster
////////////////////////////////////////////////////////////////////////
} // namespace cmd

} // namespace redis
