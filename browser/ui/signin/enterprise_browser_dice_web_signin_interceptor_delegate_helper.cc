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

#include "enterprise_browser/browser/ui/signin/enterprise_browser_dice_web_signin_interceptor_delegate_helper.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/single_thread_task_runner.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/signin/dice_web_signin_interceptor_delegate.h"

namespace enterprise_browser {

namespace {

class OidcEnterpriseSigninInterceptionHandleNoBrowser
    : public ScopedWebSigninInterceptionBubbleHandle {
 public:
  OidcEnterpriseSigninInterceptionHandleNoBrowser(
      Profile* profile,
      const WebSigninInterceptor::Delegate::BubbleParameters& bubble_parameters,
      signin::SigninChoiceWithConfirmAndRetryCallback callback,
      base::OnceClosure dialog_closed_closure) {
    DiceWebSigninInterceptorDelegate::RecordInterceptionResult(
        bubble_parameters, profile, SigninInterceptionResult::kAccepted);

    // When the interceptor finishes (profile creation, policy fetch), it calls
    // the 'done' callback. For the 'NoBrowser' case, we immediately trigger
    // the finalize closure (dialog_closed_closure) to transition to the browser.
    auto done_callback = base::BindOnce(
        [](base::OnceClosure finalize_closure,
           signin::SigninChoiceOperationResult result,
           signin::SigninChoiceErrorType error) {
          std::move(finalize_closure).Run();
        },
        std::move(dialog_closed_closure));

    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), signin::SIGNIN_CHOICE_NEW_PROFILE,
                       std::move(done_callback), base::DoNothing()));
  }
  ~OidcEnterpriseSigninInterceptionHandleNoBrowser() override = default;
};

}  // namespace

std::unique_ptr<ScopedWebSigninInterceptionBubbleHandle>
CreateOidcInterceptionHandleNoBrowser(
    Profile* profile,
    const WebSigninInterceptor::Delegate::BubbleParameters& bubble_parameters,
    signin::SigninChoiceWithConfirmAndRetryCallback callback,
    base::OnceClosure dialog_closed_closure) {
  return std::make_unique<OidcEnterpriseSigninInterceptionHandleNoBrowser>(
      profile, bubble_parameters, std::move(callback),
      std::move(dialog_closed_closure));
}

}  // namespace enterprise_browser
