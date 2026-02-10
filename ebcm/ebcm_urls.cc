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

#include "enterprise_browser/ebcm/ebcm_urls.h"

#include "base/command_line.h"
#include "net/base/url_util.h"

namespace {

const char kDefaultEBCMUrl[] = "http://localhost:3001";
const char kSigninChromeSyncDice[] = "signin?ssp=1";

}  // namespace

// static
EbcmUrls* EbcmUrls::GetInstance() {
  return base::Singleton<EbcmUrls>::get();
}

EbcmUrls::EbcmUrls() {
  ebcm_url_ = GURL(kDefaultEBCMUrl);
  ebcm_origin_ = url::Origin::Create(ebcm_url_);
  signin_chrome_sync_dice_ = ebcm_url_.Resolve(kSigninChromeSyncDice);
}

EbcmUrls::~EbcmUrls() = default;

const GURL& EbcmUrls::ebcm_url() const {
  return ebcm_url_;
}

const url::Origin& EbcmUrls::ebcm_origin() const {
  return ebcm_origin_;
}

const GURL& EbcmUrls::signin_chrome_sync_dice() const {
  return signin_chrome_sync_dice_;
}
