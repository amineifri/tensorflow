/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/xla/layout.h"

#include <memory>
#include <sstream>
#include <vector>

#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status.h"
#include "tensorflow/compiler/xla/test.h"
#include "tensorflow/compiler/xla/xla_data.pb.h"

namespace xla {
namespace {

class LayoutTest : public ::testing::Test {};

TEST_F(LayoutTest, ToString) {
  EXPECT_EQ(Layout().ToString(), "{}");
  EXPECT_EQ(Layout({4, 5, 6}).ToString(), "{4,5,6}");
  EXPECT_EQ(Layout({4, 5, 6}).ToString(), "{4,5,6}");
  EXPECT_EQ(
      Layout({3, 2, 1, 0}, {}, {Tile({42, 123}), Tile({4, 5})}).ToString(),
      "{3,2,1,0:T(42,123)(4,5)}");
  EXPECT_EQ(Layout({3, 2, 1, 0}, {}, {Tile({42, 123}), Tile({4, 5})})
                .set_memory_space(3)
                .ToString(),
            "{3,2,1,0:T(42,123)(4,5)S(3)}");
}

TEST_F(LayoutTest, StreamOut) {
  {
    std::ostringstream oss;
    oss << Tile({7, 8});
    EXPECT_EQ(oss.str(), "(7,8)");
  }

  {
    std::ostringstream oss;
    oss << Layout({0, 1, 2});
    EXPECT_EQ(oss.str(), "{0,1,2}");
  }
}

TEST_F(LayoutTest, Equality) {
  EXPECT_EQ(Layout(), Layout());
  const std::vector<int64_t> empty_dims;
  EXPECT_EQ(Layout(empty_dims), Layout(empty_dims));
  EXPECT_EQ(Layout(), Layout(empty_dims));
  EXPECT_EQ(Layout({0, 1, 2, 3}), Layout({0, 1, 2, 3}));
  EXPECT_NE(Layout({0, 1, 2, 3}), Layout({0, 1, 2}));
  EXPECT_EQ(Layout({0, 1, 2}, {}, {Tile({42, 44})}),
            Layout({0, 1, 2}, {}, {Tile({42, 44})}));
  EXPECT_NE(Layout({0, 1, 2}, {}, {Tile({42, 44})}),
            Layout({0, 1, 2}, {}, {Tile({42, 45})}));
  EXPECT_NE(Layout({0, 1, 2}, {}, {Tile({42, 44})}), Layout({0, 1, 2, 3}));
  EXPECT_EQ(Layout({0, 1, 2}).set_memory_space(3),
            Layout({0, 1, 2}).set_memory_space(3));
  EXPECT_NE(Layout({0, 1, 2}).set_memory_space(1),
            Layout({0, 1, 2}).set_memory_space(3));
  EXPECT_FALSE(Layout::Equal()(Layout({0, 1, 2}, {}, {Tile({42, 44})}),
                               Layout({0, 1, 2})));
  EXPECT_TRUE(Layout::Equal().IgnoreTiles()(
      Layout({0, 1, 2}, {}, {Tile({42, 44})}), Layout({0, 1, 2})));
  EXPECT_FALSE(Layout::Equal()(Layout({0, 1, 2}, {}, {}, 32),
                               Layout({0, 1, 2}, {}, {}, 1)));
  EXPECT_TRUE(Layout::Equal().IgnoreMemorySpace()(
      Layout({0, 1, 2}).set_memory_space(1),
      Layout({0, 1, 2}).set_memory_space(3)));
}

TEST_F(LayoutTest, LayoutToFromProto) {
  // Round-trips a Layout through proto de/serialization.
  auto expect_unchanged = [](const Layout& layout) {
    EXPECT_EQ(layout, Layout::CreateFromProto(layout.ToProto()));
  };

  expect_unchanged(Layout());
  expect_unchanged(Layout({1, 3, 2, 0}));
  expect_unchanged(Layout({3, 2, 1, 0}, {}, {Tile({42, 123}), Tile({4, 5})}));
  expect_unchanged(Layout({1, 0}, {DIM_DENSE, DIM_COMPRESSED}, {}));
  expect_unchanged(
      Layout({1, 0}, {DIM_DENSE, DIM_COMPRESSED}, {}, 0,
             std::make_unique<Shape>(ShapeUtil::MakeShape(S32, {10, 10}))));
}

}  // namespace
}  // namespace xla
