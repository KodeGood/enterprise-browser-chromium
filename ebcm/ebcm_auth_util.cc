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

#include "enterprise_browser/ebcm/ebcm_auth_util.h"

#include "enterprise_browser/ebcm/ebcm_urls.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/scheme_host_port.h"

namespace ebcm {

bool HasEbcmSchemeHostPort(const GURL& url) {
  const url::Origin& ebcm_origin = EbcmUrls::GetInstance()->ebcm_origin();
  if (ebcm_origin.opaque()) {
    return false;
  }

  return url::SchemeHostPort(url) ==
         ebcm_origin.GetTupleOrPrecursorTupleIfOpaque();
}

bool IsThirdPartyIdpUrl(const GURL& url) {
  std::string redirect_uri;
  if (net::GetValueForKeyInQuery(url, "redirect_uri", &redirect_uri) ||
      net::GetValueForKeyInQuery(url, "redirect_url", &redirect_uri)) {
    GURL redirect_gurl(redirect_uri);
    return redirect_gurl.is_valid() && HasEbcmSchemeHostPort(redirect_gurl);
  }
  return false;
}

}  // namespace ebcm
