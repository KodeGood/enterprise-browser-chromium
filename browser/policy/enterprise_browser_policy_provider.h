// Copyright (c) 2026 Jani Hautakangas <jani@kodegood.com>
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ENTERPRISE_BROWSER_BROWSER_POLICY_ENTERPRISE_BROWSER_POLICY_PROVIDER_H_
#define ENTERPRISE_BROWSER_BROWSER_POLICY_ENTERPRISE_BROWSER_POLICY_PROVIDER_H_

#include "components/policy/core/common/configuration_policy_provider.h"

namespace enterprise_browser {

class EnterpriseBrowserPolicyProvider : public policy::ConfigurationPolicyProvider {
 public:
  EnterpriseBrowserPolicyProvider();
  ~EnterpriseBrowserPolicyProvider() override;

  EnterpriseBrowserPolicyProvider(const EnterpriseBrowserPolicyProvider&) = delete;
  EnterpriseBrowserPolicyProvider& operator=(const EnterpriseBrowserPolicyProvider&) = delete;

  // ConfigurationPolicyProvider implementation.
  void RefreshPolicies(policy::PolicyFetchReason reason) override;
  bool IsFirstPolicyLoadComplete(policy::PolicyDomain domain) const override;

 private:
  bool first_policies_loaded_ = false;
};

}  // namespace enterprise_browser

#endif  // ENTERPRISE_BROWSER_BROWSER_POLICY_ENTERPRISE_BROWSER_POLICY_PROVIDER_H_
