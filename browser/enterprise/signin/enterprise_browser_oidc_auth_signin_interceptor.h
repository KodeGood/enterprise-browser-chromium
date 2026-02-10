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

#ifndef ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_H_
#define ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_H_

#include <memory>
#include <string>
#include <variant>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/enterprise/signin/oidc_metrics_utils.h"
#include "chrome/browser/enterprise/signin/token_managed_profile_creation_delegate.h"
#include "chrome/browser/signin/web_signin_interceptor.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/policy/core/common/cloud/cloud_policy_client.h"

namespace content {
class WebContents;
}

namespace policy {
class CloudPolicyClientRegistrationHelper;
}  // namespace policy

class OidcAuthenticationSigninInterceptorTest;

class Profile;
class ProfileAttributesEntry;

using OidcInterceptionCallback = base::OnceCallback<void()>;
using policy::CloudPolicyClient;

// Called after a valid OIDC authentication redirection is captured. The
// interceptor is responsible for starting registration process, collecting user
// consent, and creating/switching to a new managed profile if agreed.

// The main steps of the interception are:
// - Check if the user is elligible to interception in
// MaybeInterceptOidcAuthentication()
// - Show the dialog and wait for the user choice in
// ShowOIDCInterceptionDialog()
// - User choice is received in OnProfileCreationChoice()
// - Go through registration process with StartOidcRegistration() and
// OnClientRegistered()
// - Create a new profile with ManagedProfileCreator and then
// OnNewSignedInProfileCreated()
// - Fetch policies, received in OnPolicyFetchCompleteInNewProfile()
// - Notify the dialog that the profile creation is complete with
// user_choice_handling_done_callback_
// - Wait for the dialog to be closed by the user and open a browser registered
// for policies via oidc
class EnterpriseBrowserOidcAuthSigninInterceptor
    : public WebSigninInterceptor,

      // TODO(350960816): Restructure `EnterpriseBrowserOidcAuthSigninInterceptor` to
      // be a state machine instead of a keyed service.
      public KeyedService {
 public:
  enum class SigninInterceptionType {
    kProfileSwitch,
    kEnterprise,
  };

  EnterpriseBrowserOidcAuthSigninInterceptor(
      Profile* profile,
      std::unique_ptr<WebSigninInterceptor::Delegate> delegate);
  ~EnterpriseBrowserOidcAuthSigninInterceptor() override;

  EnterpriseBrowserOidcAuthSigninInterceptor(
      const EnterpriseBrowserOidcAuthSigninInterceptor&) = delete;
  EnterpriseBrowserOidcAuthSigninInterceptor& operator=(
      const EnterpriseBrowserOidcAuthSigninInterceptor&) = delete;

  // Intercept and kick off OIDC registration process if the tokens we received
  // are valid.
  virtual void MaybeInterceptOidcAuthentication(
      content::WebContents* intercepted_contents,
      const ProfileManagementOidcTokens& oidc_tokens,
      const std::string& issuer_id,
      const std::string& subject_id,
      const std::string& email,
      OidcInterceptionCallback oidc_callback);

  // KeyedService:
  void Shutdown() override;

  void SetCloudPolicyClientForTesting(
      std::unique_ptr<CloudPolicyClient> client) {
    client_for_testing_ = std::move(client);
  }

 protected:
  virtual void OnPolicyFetchCompleteInNewProfile(bool success);
  virtual void FinalizeSigninInterception();
  virtual void CreateBrowserAfterSigninInterception();

 private:
  friend class MockOidcAuthenticationSigninInterceptor;

  // Cancels any current signin interception and resets the interceptor to its
  // initial state.
  void Reset();

  // `is_dasher_based` should be nullopt when `result` has type
  // `OidcInterceptionResult` since its histogram does not have
  // Dasher-based/Dasherless variants; it should be either True or False when
  // `result` has type `OidcProfileCreationResult` and be used for histogram
  // recording.
  void HandleError(
      std::variant<OidcInterceptionResult, OidcProfileCreationResult> result,
      std::optional<bool> is_dasher_based = std::nullopt);

  // Try to send OIDC tokens to DM server for registration.
  void StartOidcRegistration();
  // Called when OIDC registration finishes, the client should be registered
  // (aka has a dm token) and various information should be included, most
  // importantly, if the 3P user identity is sync-ed to Google or not.
  void OnClientRegistered(std::unique_ptr<CloudPolicyClient> client,
                          std::string preset_profile_guid,
                          base::TimeTicks registration_start_time,
                          CloudPolicyClient::Result result);

  // Called when user makes a decision on the profile creation dialog.
  void OnProfileCreationChoice(
      signin::SigninChoice choice,
      signin::SigninChoiceOperationDoneCallback confirm_callback,
      signin::SigninChoiceOperationRetryCallback retry_callback);
  void OnProfileSwitchChoice(SigninInterceptionResult result);
  // Called when the new profile has been created.
  void OnNewSignedInProfileCreated(base::WeakPtr<Profile> new_profile);

  const raw_ptr<Profile, DanglingUntriaged> profile_;
  base::WeakPtr<Profile> new_profile_;
  std::unique_ptr<WebSigninInterceptor::Delegate> delegate_;
  std::unique_ptr<ManagedProfileCreator> profile_creator_;

  // Members below are related to the interception in progress.
  base::WeakPtr<content::WebContents> web_contents_;
  ProfileManagementOidcTokens oidc_tokens_;
  std::string dm_token_;
  std::string client_id_;
  std::string user_display_name_;
  std::string user_email_;
  // Unique id for the OIDC user, format:
  // "iss:<value of 'iss' field>,sub:<value of 'sub'field>"
  // For context, 'iss' is the ID of the OIDC issuer and 'sub' is the
  // unique-per-user subject ID within the issuer.
  std::string unique_user_identifier_;
  bool dasher_based_ = true;
  std::string preset_profile_id_;
  raw_ptr<const ProfileAttributesEntry> switch_to_entry_ = nullptr;
  SkColor profile_color_;
  bool interception_in_progress_ = false;

  std::unique_ptr<policy::CloudPolicyClientRegistrationHelper>
      registration_helper_for_temporary_client_;

  // Used to retain the interception UI bubble until profile creation completes.
  std::unique_ptr<ScopedWebSigninInterceptionBubbleHandle>
      interception_bubble_handle_;

  std::unique_ptr<CloudPolicyClient> client_for_testing_ = nullptr;

  OidcInterceptionCallback oidc_callback_;

  signin::SigninChoiceOperationDoneCallback user_choice_handling_done_callback_;
  signin::SigninChoiceOperationRetryCallback
      user_choice_handling_retry_callback_;

  base::WeakPtrFactory<EnterpriseBrowserOidcAuthSigninInterceptor> weak_factory_{this};

  friend class OidcAuthenticationSigninInterceptorTest;
};

#endif  // ENTERPRISE_BROWSER_BROWSER_ENTERPRISE_SIGNIN_ENTERPRISE_BROWSER_OIDC_AUTH_SIGNIN_INTERCEPTOR_H_
