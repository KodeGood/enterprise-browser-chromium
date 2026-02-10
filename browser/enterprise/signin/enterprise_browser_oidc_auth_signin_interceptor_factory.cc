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

#include "enterprise_browser/browser/enterprise/signin/enterprise_browser_oidc_auth_signin_interceptor_factory.h"

#include "chrome/browser/enterprise/identifiers/profile_id_service_factory.h"
#include "chrome/browser/enterprise/profile_management/profile_management_features.h"
#include "chrome/browser/enterprise/signin/user_policy_oidc_signin_service_factory.h"
#include "chrome/browser/policy/cloud/user_policy_signin_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/signin/dice_web_signin_interceptor_delegate.h"

#include "enterprise_browser/browser/enterprise/signin/enterprise_browser_oidc_auth_signin_interceptor.h"

// static
EnterpriseBrowserOidcAuthSigninInterceptor*
EnterpriseBrowserOidcAuthSigninInterceptorFactory::GetForProfile(Profile* profile) {
  return static_cast<EnterpriseBrowserOidcAuthSigninInterceptor*>(
                   GetInstance()->GetServiceForBrowserContext(profile, true));
}

//  static
EnterpriseBrowserOidcAuthSigninInterceptorFactory*
EnterpriseBrowserOidcAuthSigninInterceptorFactory::GetInstance() {
  static base::NoDestructor<EnterpriseBrowserOidcAuthSigninInterceptorFactory>
      instance;
  return instance.get();
}

EnterpriseBrowserOidcAuthSigninInterceptorFactory::
    EnterpriseBrowserOidcAuthSigninInterceptorFactory()
    : ProfileKeyedServiceFactory("EnterpriseBrowserOidcAuthSigninInterceptor") {
  DependsOn(enterprise::ProfileIdServiceFactory::GetInstance());
  DependsOn(policy::UserPolicySigninServiceFactory::GetInstance());
  DependsOn(policy::UserPolicyOidcSigninServiceFactory::GetInstance());
}

EnterpriseBrowserOidcAuthSigninInterceptorFactory::
    ~EnterpriseBrowserOidcAuthSigninInterceptorFactory() = default;

std::unique_ptr<KeyedService> EnterpriseBrowserOidcAuthSigninInterceptorFactory::
    BuildServiceInstanceForBrowserContext(
        content::BrowserContext* context) const {
  return std::make_unique<EnterpriseBrowserOidcAuthSigninInterceptor>(
      Profile::FromBrowserContext(context),
      std::make_unique<DiceWebSigninInterceptorDelegate>());
}
