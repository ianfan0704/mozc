// Copyright 2010-2016, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "rewriter/katakana_promotion_rewriter.h"

#include <memory>
#include <string>

#include "converter/segments.h"
#include "data_manager/testing/mock_data_manager.h"
#include "dictionary/pos_matcher.h"
#include "protocol/commands.pb.h"
#include "request/conversion_request.h"
#include "rewriter/transliteration_rewriter.h"
#include "testing/base/public/gunit.h"
#include "testing/base/public/mozctest.h"

namespace mozc {
namespace {

void AddCandidateWithValue(const string &value, Segment *segment) {
  Segment::Candidate *candidate = segment->add_candidate();
  candidate->Init();
  candidate->key = segment->key();
  candidate->content_key = segment->key();
  candidate->value = value;
  candidate->content_value = value;
}

// Returns -1 if not found.
int GetCandidateIndexByValue(const string &value, const Segment &segment) {
  for (size_t i = 0; i < segment.candidates_size(); ++i) {
    if (segment.candidate(i).value == value) {
      return i;
    }
  }
  return -1;
}

class KatakanaPromotionRewriterTest : public ::testing::Test {
 protected:
  KatakanaPromotionRewriterTest() {
    t13n_rewriter_.reset(new TransliterationRewriter(
        dictionary::POSMatcher(mock_data_manager_.GetPOSMatcherData())));

    desktop_request_.set_mixed_conversion(false);
    desktop_conv_request_.set_request(&desktop_request_);

    mobile_request_.set_mixed_conversion(true);
    mobile_conv_request_.set_request(&mobile_request_);
  }

  ~KatakanaPromotionRewriterTest() override {}

  std::unique_ptr<TransliterationRewriter> t13n_rewriter_;
  ConversionRequest desktop_conv_request_;
  ConversionRequest mobile_conv_request_;

 private:
  const testing::ScopedTmpUserProfileDirectory scoped_profile_dir_;
  const testing::MockDataManager mock_data_manager_;
  commands::Request desktop_request_;
  commands::Request mobile_request_;
};

TEST_F(KatakanaPromotionRewriterTest, Capability) {
  KatakanaPromotionRewriter rewriter;

  EXPECT_EQ(RewriterInterface::ALL,
            rewriter.capability(mobile_conv_request_));

  EXPECT_EQ(RewriterInterface::NOT_AVAILABLE,
            rewriter.capability(desktop_conv_request_));
}

TEST_F(KatakanaPromotionRewriterTest, PromoteKatakanaFromT13N) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  // "きょう"
  segment->set_key("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86");
  // "今日"
  AddCandidateWithValue("\xe4\xbb\x8a\xe6\x97\xa5", segment);
  // "きょう"
  AddCandidateWithValue("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86", segment);
  // "強"
  AddCandidateWithValue("\xe5\xbc\xb7", segment);
  // "教"
  AddCandidateWithValue("\xe6\x95\x99", segment);
  // "凶"
  AddCandidateWithValue("\xe5\x87\xb6", segment);
  // "卿"
  AddCandidateWithValue("\xe5\x8d\xbf", segment);

  // "キョウ"
  EXPECT_EQ(-1, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));
  // "キョウ"
  EXPECT_EQ(-1, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));

  EXPECT_TRUE(rewriter.Rewrite(mobile_conv_request_, &segments));
  // "キョウ"
  EXPECT_EQ(5, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));
}

TEST_F(KatakanaPromotionRewriterTest, PromoteKatakanaFromT13NForFewCandidates) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  // "きょう"
  segment->set_key("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86");
  // "今日"
  AddCandidateWithValue("\xe4\xbb\x8a\xe6\x97\xa5", segment);
  // "きょう"
  AddCandidateWithValue("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86", segment);
  // "強"
  AddCandidateWithValue("\xe5\xbc\xb7", segment);

  // "キョウ"
  EXPECT_EQ(-1, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));
  // "キョウ"
  EXPECT_EQ(-1, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));

  EXPECT_TRUE(rewriter.Rewrite(mobile_conv_request_, &segments));
  // "キョウ"
  EXPECT_EQ(3, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));
}

TEST_F(KatakanaPromotionRewriterTest, PromoteKatakana) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  // "きょう"
  segment->set_key("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86");
  // "今日"
  AddCandidateWithValue("\xe4\xbb\x8a\xe6\x97\xa5", segment);
  // "きょう"
  AddCandidateWithValue("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86", segment);
  // "強"
  AddCandidateWithValue("\xe5\xbc\xb7", segment);
  // "教"
  AddCandidateWithValue("\xe6\x95\x99", segment);
  // "凶"
  AddCandidateWithValue("\xe5\x87\xb6", segment);
  // "卿"
  AddCandidateWithValue("\xe5\x8d\xbf", segment);
  // "京"
  AddCandidateWithValue("\xe4\xba\xac", segment);
  // "キョウ"
  AddCandidateWithValue("\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", segment);

  const int katakana_index = GetCandidateIndexByValue(
      // "キョウ"
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment);
  EXPECT_EQ(katakana_index, 7);

  Segment::Candidate *katakana_candidate =
      segment->mutable_candidate(katakana_index);
  katakana_candidate->lid = 1;
  katakana_candidate->rid = 1;

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));

  EXPECT_TRUE(rewriter.Rewrite(mobile_conv_request_, &segments));

  const int promoted_index = GetCandidateIndexByValue(
      // "キョウ"
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment);
  // Make sure that the existing candidate was promoted.
  EXPECT_EQ(5, promoted_index);
  EXPECT_EQ(1, segment->candidate(promoted_index).lid);
  EXPECT_EQ(1, segment->candidate(promoted_index).rid);
}

TEST_F(KatakanaPromotionRewriterTest, KatakanaIsAlreadyRankedHigh) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  // "きょう"
  segment->set_key("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86");
  // "今日"
  AddCandidateWithValue("\xe4\xbb\x8a\xe6\x97\xa5", segment);
  // "きょう"
  AddCandidateWithValue("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86", segment);
  // "キョウ"
  AddCandidateWithValue("\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", segment);
  // "強"
  AddCandidateWithValue("\xe5\xbc\xb7", segment);
  // "教"
  AddCandidateWithValue("\xe6\x95\x99", segment);
  // "凶"
  AddCandidateWithValue("\xe5\x87\xb6", segment);
  // "卿"
  AddCandidateWithValue("\xe5\x8d\xbf", segment);

  // "キョウ"
  EXPECT_EQ(2, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));

  EXPECT_FALSE(rewriter.Rewrite(mobile_conv_request_, &segments));
  // "キョウ"
  EXPECT_EQ(2, GetCandidateIndexByValue(
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6", *segment));
}

TEST_F(KatakanaPromotionRewriterTest, PromoteKatakanaForMultiSegments) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  // "きょうは"
  segment->set_key("\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86\xe3\x81\xaf");
  // "今日は"
  AddCandidateWithValue("\xe4\xbb\x8a\xe6\x97\xa5\xe3\x81\xaf", segment);
  // "きょうは"
  AddCandidateWithValue(
      "\xe3\x81\x8d\xe3\x82\x87\xe3\x81\x86\xe3\x81\xaf", segment);
  // "強は"
  AddCandidateWithValue("\xe5\xbc\xb7\xe3\x81\xaf", segment);
  // "教は"
  AddCandidateWithValue("\xe6\x95\x99\xe3\x81\xaf", segment);
  // "凶は"
  AddCandidateWithValue("\xe5\x87\xb6\xe3\x81\xaf", segment);
  // "卿は"
  AddCandidateWithValue("\xe5\x8d\xbf\xe3\x81\xaf", segment);

  segment = segments.push_back_segment();
  // "はれ"
  segment->set_key("\xe3\x81\xaf\xe3\x82\x8c");
  // "晴れ"
  AddCandidateWithValue("\xe6\x99\xb4\xe3\x82\x8c", segment);
  // "腫れ"
  AddCandidateWithValue("\xe8\x85\xab\xe3\x82\x8c", segment);
  // "晴"
  AddCandidateWithValue("\xe6\x99\xb4", segment);
  // "貼れ"
  AddCandidateWithValue("\xe8\xb2\xbc\xe3\x82\x8c", segment);
  // "張れ"
  AddCandidateWithValue("\xe5\xbc\xb5\xe3\x82\x8c", segment);
  // "脹れ"
  AddCandidateWithValue("\xe8\x84\xb9\xe3\x82\x8c", segment);

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));

  EXPECT_EQ(-1, GetCandidateIndexByValue(
      // "キョウハ"
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6\xe3\x83\x8f",
      segments.conversion_segment(0)));
  EXPECT_EQ(-1, GetCandidateIndexByValue(
      // "ハレ"
      "\xe3\x83\x8f\xe3\x83\xac", segments.conversion_segment(1)));

  EXPECT_TRUE(rewriter.Rewrite(mobile_conv_request_, &segments));
  EXPECT_EQ(5, GetCandidateIndexByValue(
      // "キョウハ"
      "\xe3\x82\xad\xe3\x83\xa7\xe3\x82\xa6\xe3\x83\x8f",
      segments.conversion_segment(0)));
  EXPECT_EQ(5, GetCandidateIndexByValue(
      // "ハレ"
      "\xe3\x83\x8f\xe3\x83\xac", segments.conversion_segment(1)));
}

TEST_F(KatakanaPromotionRewriterTest, NoNeedToPromote) {
  KatakanaPromotionRewriter rewriter;

  Segments segments;
  Segment *segment = segments.push_back_segment();
  segment->set_key("go");
  AddCandidateWithValue("google", segment);
  AddCandidateWithValue("golden", segment);
  AddCandidateWithValue("goodness", segment);
  AddCandidateWithValue("governor", segment);
  AddCandidateWithValue("goalkeeper", segment);
  AddCandidateWithValue("gorgeous", segment);

  EXPECT_TRUE(t13n_rewriter_->Rewrite(mobile_conv_request_, &segments));

  EXPECT_EQ(-1, GetCandidateIndexByValue(
      "go", segments.conversion_segment(0)));

  EXPECT_FALSE(rewriter.Rewrite(mobile_conv_request_, &segments));
}

}  // namespace
}  // namespace mozc
