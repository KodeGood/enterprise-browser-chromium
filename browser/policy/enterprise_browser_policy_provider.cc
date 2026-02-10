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

#include "enterprise_browser/browser/policy/enterprise_browser_policy_provider.h"

#include "base/logging.h"

#include "components/policy/core/common/policy_bundle.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"

namespace enterprise_browser {

EnterpriseBrowserPolicyProvider::EnterpriseBrowserPolicyProvider() {
  RefreshPolicies(policy::PolicyFetchReason::kBrowserStart);
}

EnterpriseBrowserPolicyProvider::~EnterpriseBrowserPolicyProvider() = default;

void EnterpriseBrowserPolicyProvider::RefreshPolicies(policy::PolicyFetchReason reason) {
  policy::PolicyBundle bundle;
  policy::PolicyMap& map = bundle.Get(policy::PolicyNamespace(
      policy::POLICY_DOMAIN_CHROME, std::string()));

  // ProfilePickerOnStartupAvailability = 1 (kDisabled)
  map.Set(policy::key::kProfilePickerOnStartupAvailability,
          policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_MACHINE,
          policy::POLICY_SOURCE_PLATFORM, base::Value(1), nullptr);

  // BrowserSignin = 2 (kForced)
  map.Set(policy::key::kBrowserSignin, policy::POLICY_LEVEL_MANDATORY,
          policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
          base::Value(2), nullptr);

  // ForceBrowserSignin = true (kForced)
  map.Set(policy::key::kForceBrowserSignin, policy::POLICY_LEVEL_MANDATORY,
          policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
          base::Value(true), nullptr);

  // BrowserAddPersonEnabled = false
  map.Set(policy::key::kBrowserAddPersonEnabled, policy::POLICY_LEVEL_MANDATORY,
          policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
          base::Value(false), nullptr);

  // CloudReportingEnabled = true
  map.Set(policy::key::kCloudReportingEnabled, policy::POLICY_LEVEL_MANDATORY,
          policy::POLICY_SCOPE_MACHINE, policy::POLICY_SOURCE_PLATFORM,
          base::Value(true), nullptr);

  first_policies_loaded_ = true;
  UpdatePolicy(std::move(bundle));
}

bool EnterpriseBrowserPolicyProvider::IsFirstPolicyLoadComplete(
    policy::PolicyDomain domain) const {
  return first_policies_loaded_;
}

}  // namespace enterprise_browser
