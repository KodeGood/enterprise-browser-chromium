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

#ifndef ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_PROFILE_MANAGEMENT_ENTERPRISE_BROWSER_OIDC_AUTH_NAVIGATION_THROTTLE_H_
#define ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_PROFILE_MANAGEMENT_ENTERPRISE_BROWSER_OIDC_AUTH_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "components/url_matcher/url_matcher.h"
#include "content/public/browser/navigation_throttle.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace profile_management {

// This throttle looks for redirection from Oidc authentications to the hard
// coded host `chromeprofiletoken`. It will capture the redirection and try to
// create or switch to a managed profile using the tokens from the auth
// response. The workflow is currently experimental and not productionized.
class EnterpriseBrowserOidcAuthNavigationThrottle
    : public content::NavigationThrottle {
 public:
  // Create a navigation throttle for the given navigation if Oidc
  // authentication based enrollment is enabled. Returns nullptr if no
  // throttling should be done.
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  explicit EnterpriseBrowserOidcAuthNavigationThrottle(
      content::NavigationThrottleRegistry& registry);

  EnterpriseBrowserOidcAuthNavigationThrottle(
      const EnterpriseBrowserOidcAuthNavigationThrottle&) = delete;
  EnterpriseBrowserOidcAuthNavigationThrottle& operator=(
      const EnterpriseBrowserOidcAuthNavigationThrottle&) = delete;
  ~EnterpriseBrowserOidcAuthNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillProcessResponse() override;

  const char* GetNameForLogging() override;

  // Method to get a new URL matcher instead of the usual static one for
  // testing, due the feature flag value may have changed in different cases.
  static std::unique_ptr<url_matcher::URLMatcher>
  GetOidcEnrollmentUrlMatcherForTesting();

 private:
  ThrottleCheckResult AttemptToTriggerUrlInterception();
  ThrottleCheckResult AttemptToTriggerHeaderInterception();

  // Starts OIDC registration and profile creation process if the response is
  // valid.
  void RegisterWithOidcTokens(ProfileManagementOidcTokens tokens,
                              data_decoder::DataDecoder::ValueOrError result);

  bool interception_triggered_ = false;
  base::WeakPtrFactory<EnterpriseBrowserOidcAuthNavigationThrottle>
      weak_ptr_factory_{this};
};

}  // namespace profile_management

#endif  // ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_PROFILE_MANAGEMENT_ENTERPRISE_BROWSER_OIDC_AUTH_NAVIGATION_THROTTLE_H_
