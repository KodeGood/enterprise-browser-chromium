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

#ifndef ENTERPRISE_BROWSER_BROWSER_UI_SIGNIN_ENTERPRISE_BROWSER_DICE_WEB_SIGNIN_INTERCEPTOR_DELEGATE_HELPER_H_
#define ENTERPRISE_BROWSER_BROWSER_UI_SIGNIN_ENTERPRISE_BROWSER_DICE_WEB_SIGNIN_INTERCEPTOR_DELEGATE_HELPER_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "chrome/browser/signin/web_signin_interceptor.h"

class Profile;

namespace enterprise_browser {

// Handles OIDC enterprise signin interception when no browser window is
// available. This is used in the profile picker context.
std::unique_ptr<ScopedWebSigninInterceptionBubbleHandle>
CreateOidcInterceptionHandleNoBrowser(
    Profile* profile,
    const WebSigninInterceptor::Delegate::BubbleParameters& bubble_parameters,
    signin::SigninChoiceWithConfirmAndRetryCallback callback,
    base::OnceClosure dialog_closed_closure);

}  // namespace enterprise_browser

#endif  // ENTERPRISE_BROWSER_BROWSER_UI_SIGNIN_ENTERPRISE_BROWSER_DICE_WEB_SIGNIN_INTERCEPTOR_DELEGATE_HELPER_H_
