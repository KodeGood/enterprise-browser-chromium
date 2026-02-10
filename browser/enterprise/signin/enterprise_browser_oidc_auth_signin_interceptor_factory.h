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

#ifndef ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_FACTORY_H_
#define ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class EnterpriseBrowserOidcAuthSigninInterceptor;
class Profile;

class EnterpriseBrowserOidcAuthSigninInterceptorFactory
    : public ProfileKeyedServiceFactory {
 public:
  static EnterpriseBrowserOidcAuthSigninInterceptor* GetForProfile(Profile* profile);
  static EnterpriseBrowserOidcAuthSigninInterceptorFactory* GetInstance();

  EnterpriseBrowserOidcAuthSigninInterceptorFactory(
      const EnterpriseBrowserOidcAuthSigninInterceptorFactory&) = delete;
  EnterpriseBrowserOidcAuthSigninInterceptorFactory& operator=(
      const EnterpriseBrowserOidcAuthSigninInterceptorFactory&) = delete;

 private:
  friend class base::NoDestructor<EnterpriseBrowserOidcAuthSigninInterceptorFactory>;
  EnterpriseBrowserOidcAuthSigninInterceptorFactory();
  ~EnterpriseBrowserOidcAuthSigninInterceptorFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* profile) const override;
};

#endif  // ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_FACTORY_H_
