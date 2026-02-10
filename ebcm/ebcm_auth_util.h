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

#ifndef ENTERPRISE_BROWSER_EBCM_EBCM_AUTH_UTIL_H_
#define ENTERPRISE_BROWSER_EBCM_EBCM_AUTH_UTIL_H_

class GURL;

namespace ebcm {

// Returns true if the URL's scheme, host, and port match the EBCM origin.
bool HasEbcmSchemeHostPort(const GURL& url);

// Returns true if the URL contains a redirect_uri parameter that matches
// the EBCM origin.
bool IsThirdPartyIdpUrl(const GURL& url);

}  // namespace ebcm

#endif  // ENTERPRISE_BROWSER_EBCM_EBCM_AUTH_UTIL_H_
