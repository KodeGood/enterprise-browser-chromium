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

#include "enterprise_browser/ebcm/ebcm_switches.h"

namespace ebcm {

// Chrome is being opened to show the credential provider logon page. This
// page is always shown in incognito mode.
const char kEbcmSigninSwitch[] = "ebcm-signin";

} // namespace ebcm

