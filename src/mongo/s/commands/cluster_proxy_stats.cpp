// cluster_proxy_stats.cpp

/**
 *    Tencent is pleased to support the open source community by making CMONGO available.
 *
 *    Copyright (C) 2018 THL A29 Limited, a Tencent company. All rights reserved.
 *
 *    Licensed under the GNU Affero General Public License Version 3 (the "License");
 *    you may not use this file except in compliance with the License. You may obtain a 
 *    copy of the License at https://www.gnu.org/licenses/agpl-3.0.en.html
 *
 *    Unless required by applicable law or agreed to in writing, software distributed under
 *    the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 *    either express or implied. See the License for the specific language governing permissions
 *    and limitations under the License.
*/
#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kCommand

#include "mongo/db/commands.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/s/grid.h"
#include "mongo/util/flow_manager.h"
#include "mongo/util/log.h"


namespace mongo {
class ProxyStatsCmd: public BasicCommand {
public:
	ProxyStatsCmd(): BasicCommand("proxyStatus") {}

    virtual bool slaveOk() const {
        return true;
    }

    virtual bool adminOnly() const {
        return true;
    }

    virtual bool supportsWriteConcern(const BSONObj& cmd) const override {
        return false;
    }

    AllowedOnSecondary secondaryAllowed(ServiceContext*) const final {
        return AllowedOnSecondary::kAlways;
    }

    virtual bool isWriteCommandForConfigServer() const {
        return false;
    }

    std::string help() const final {
        return "get proxyStats";
    }

    Status checkAuthForCommand(Client* client,
                               const std::string& dbname,
                               const BSONObj& cmdObj) const final {
        // NOTE(deyukong): no privileges needed for proxyStats,
        // if new action named action::proxyStatus is created, mongod has to be upgraded
        return Status::OK();
    }

    bool run(OperationContext* opCtx,
                     const std::string& dbname,
                     const BSONObj& cmdObj,
                     BSONObjBuilder& result) final {
        bool internal = false;
        auto client = opCtx->getClient();
        if (client &&
            AuthorizationSession::get(client)->isAuthenticatedAsUserWithRole(RoleName("root", "admin"))) {
            internal = true;
        }
        DumpConnType dumpConnType = dumpConnNone;
        if (cmdObj["dumpConnType"].type() == String) {
            std::string dumpConnTypeString = cmdObj["dumpConnType"].String();
            if (0 == dumpConnTypeString.compare("aggregated")) {
                dumpConnType = dumpConnAggregated;
            } else if (0 == dumpConnTypeString.compare("all")) {
                dumpConnType = dumpConnAll;
            }
        }

        grid.getFlowManager()->dump(&result, opCtx, dumpConnType, internal);
        return true;
    }
} proxyStats;
}