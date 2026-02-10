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

#ifndef ENTERPRISE_BROWSER_EBCM_EBCM_URLS_H_
#define ENTERPRISE_BROWSER_EBCM_EBCM_URLS_H_

#include "base/memory/singleton.h"
#include "url/gurl.h"
#include "url/origin.h"

// A singleton that provides all the URLs that are used for connecting to EBCM.
class EbcmUrls {
 public:
  static EbcmUrls* GetInstance();

  EbcmUrls(const EbcmUrls&) = delete;
  EbcmUrls& operator=(const EbcmUrls&) = delete;

  // The base URL for the Enterprise Browser Cloud Management server.
  const GURL& ebcm_url() const;

  // The origin for the Enterprise Browser Cloud Management server.
  const url::Origin& ebcm_origin() const;

  // The URL for Chrome Sync sign-in via EBCM.
  const GURL& signin_chrome_sync_dice() const;

 private:
  friend struct base::DefaultSingletonTraits<EbcmUrls>;

  EbcmUrls();
  ~EbcmUrls();

  GURL ebcm_url_;
  url::Origin ebcm_origin_;
  GURL signin_chrome_sync_dice_;
};

#endif  // ENTERPRISE_BROWSER_EBCM_EBCM_URLS_H_
