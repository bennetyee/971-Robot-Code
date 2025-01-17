#include "aos/events/logging/log_backend.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "aos/containers/resizeable_buffer.h"
#include "aos/testing/tmpdir.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace aos::logger::testing {
TEST(LogBackendTest, CreateSimpleFile) {
  const std::string logevent = aos::testing::TestTmpDir() + "/logevent/";
  FileBackend backend(logevent);
  auto file = backend.RequestFile("test.log");
  ASSERT_EQ(file->OpenForWrite(), WriteCode::kOk);
  auto result = write(file->fd(), "test", 4);
  EXPECT_GT(result, 0);
  EXPECT_EQ(file->Close(), WriteCode::kOk);
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log"));
}

TEST(LogBackendTest, CreateRenamableFile) {
  const std::string logevent = aos::testing::TestTmpDir() + "/logevent/";
  RenamableFileBackend backend(logevent);
  auto file = backend.RequestFile("test.log");
  ASSERT_EQ(file->OpenForWrite(), WriteCode::kOk);
  auto result = write(file->fd(), "testtest", 8);
  EXPECT_GT(result, 0);
  EXPECT_EQ(file->Close(), WriteCode::kOk);
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log"));
}

TEST(LogBackendTest, UseTempRenamableFile) {
  const std::string logevent = aos::testing::TestTmpDir() + "/logevent/";
  RenamableFileBackend backend(logevent);
  backend.EnableTempFiles();
  auto file = backend.RequestFile("test.log");
  ASSERT_EQ(file->OpenForWrite(), WriteCode::kOk);
  auto result = write(file->fd(), "testtest", 8);
  EXPECT_GT(result, 0);
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log.tmp"));

  EXPECT_EQ(file->Close(), WriteCode::kOk);
  // Check that file is renamed.
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log"));
}

TEST(LogBackendTest, RenameBaseAfterWrite) {
  const std::string logevent = aos::testing::TestTmpDir() + "/logevent/";
  RenamableFileBackend backend(logevent);
  auto file = backend.RequestFile("test.log");
  ASSERT_EQ(file->OpenForWrite(), WriteCode::kOk);
  auto result = write(file->fd(), "testtest", 8);
  EXPECT_GT(result, 0);
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log"));

  std::string renamed = aos::testing::TestTmpDir() + "/renamed/";
  backend.RenameLogBase(renamed);

  EXPECT_FALSE(std::filesystem::exists(logevent + "test.log"));
  EXPECT_TRUE(std::filesystem::exists(renamed + "test.log"));

  EXPECT_EQ(file->Close(), WriteCode::kOk);
  // Check that file is renamed.
  EXPECT_TRUE(std::filesystem::exists(renamed + "test.log"));
}

TEST(LogBackendTest, UseTestAndRenameBaseAfterWrite) {
  const std::string logevent = aos::testing::TestTmpDir() + "/logevent/";
  RenamableFileBackend backend(logevent);
  backend.EnableTempFiles();
  auto file = backend.RequestFile("test.log");
  ASSERT_EQ(file->OpenForWrite(), WriteCode::kOk);
  auto result = write(file->fd(), "testtest", 8);
  EXPECT_GT(result, 0);
  EXPECT_TRUE(std::filesystem::exists(logevent + "test.log.tmp"));

  std::string renamed = aos::testing::TestTmpDir() + "/renamed/";
  backend.RenameLogBase(renamed);

  EXPECT_FALSE(std::filesystem::exists(logevent + "test.log.tmp"));
  EXPECT_TRUE(std::filesystem::exists(renamed + "test.log.tmp"));

  EXPECT_EQ(file->Close(), WriteCode::kOk);
  // Check that file is renamed.
  EXPECT_TRUE(std::filesystem::exists(renamed + "test.log"));
}

// It represents calls to Write function (batching of calls and messages) where
// int values are sizes of each message in the queue.
using WriteRecipe = std::vector<std::vector<int>>;

struct FileWriteTestBase : public ::testing::Test {
  uint8_t NextRandom() { return distribution(engine); }

  class AlignedReallocator {
   public:
    static void *Realloc(void *old, size_t old_size, size_t new_capacity) {
      void *new_memory = std::aligned_alloc(512, new_capacity);
      if (old) {
        memcpy(new_memory, old, old_size);
        free(old);
      }
      return new_memory;
    }
  };

  AllocatorResizeableBuffer<AlignedReallocator> buffer;

  void TestRecipe(const WriteRecipe &recipe) {
    VLOG(1) << "Starting";
    for (const std::vector<int> &r : recipe) {
      VLOG(1) << "  chunk " << absl::StrJoin(r, ", ");
    }
    size_t requested_size = 0;
    for (const auto &call : recipe) {
      for (const auto &message_size : call) {
        requested_size += message_size;
      }
    }

    // Grow the cached buffer if it needs to grow.  Building a random buffer is
    // the most expensive part of the test.
    if (buffer.size() < requested_size) {
      // Make sure it is 512 aligned...  That makes sure we have the best chance
      // of transitioning to and from being aligned.
      buffer.resize((requested_size + FileHandler::kSector - 1) &
                    (~(FileHandler::kSector - 1)));
      std::generate(std::begin(buffer), std::end(buffer),
                    [this]() { return NextRandom(); });
    }

    // Back align the data to the buffer so we make sure the contents of the
    // file keep changing in case a file somehow doesn't get deleted, or
    // collides with something else.
    const uint8_t *adjusted_start =
        buffer.data() + buffer.size() - requested_size;

    // logevent has to end with '/' to be recognized as a folder.
    const std::string logevent = aos::testing::TestTmpDir() + "/";
    const auto file = std::filesystem::path(logevent) / "test.log";
    std::filesystem::remove_all(file);
    VLOG(1) << "Writing to " << file.c_str();

    FileBackend backend(logevent);
    auto handler = backend.RequestFile("test.log");
    ASSERT_EQ(handler->OpenForWrite(), WriteCode::kOk);

    // Build arguments for Write.
    size_t position = 0;
    for (const auto &call : recipe) {
      std::vector<absl::Span<const uint8_t>> queue;
      for (const auto &message_size : call) {
        const uint8_t *current = adjusted_start + position;
        queue.emplace_back(current, message_size);
        position += message_size;
      }
      auto result = handler->Write(queue);
      EXPECT_EQ(result.code, WriteCode::kOk);
      EXPECT_EQ(result.messages_written, call.size());
    }

    ASSERT_EQ(handler->Close(), WriteCode::kOk);
    EXPECT_TRUE(std::filesystem::exists(file));
    EXPECT_EQ(std::filesystem::file_size(file), requested_size);

    // Confirm that the contents then match the original buffer.
    std::ifstream file_stream(file, std::ios::in | std::ios::binary);
    std::vector<uint8_t> content((std::istreambuf_iterator<char>(file_stream)),
                                 std::istreambuf_iterator<char>());
    ASSERT_EQ(content.size(), requested_size);
    bool matches = true;
    for (size_t i = 0; i < content.size(); ++i) {
      if (content[i] != adjusted_start[i]) {
        matches = false;
        break;
      }
    }
    if (!matches) {
      ASSERT_TRUE(false);
    }
  }

  std::random_device random;
  std::mt19937 engine{random()};
  std::uniform_int_distribution<uint8_t> distribution{0, 0xFF};
};

// Tests that random sets of reads and writes always result in all the data
// being written.
TEST_F(FileWriteTestBase, RandomTest) {
  std::mt19937 engine2{random()};
  std::uniform_int_distribution<int> count_distribution{1, 5};

  // Pick a bunch of lengths that will result in things that add up to multiples
  // of 512 and end up transitioning across the aligned and unaligned boundary.
  const std::vector<int> lengths = {
      0x100b5,  0xff4b,   0x10000,  1024 - 7, 1024 - 6, 1024 - 5, 1024 - 4,
      1024 - 3, 1024 - 2, 1024 - 1, 1024,     1024 + 1, 1024 + 2, 1024 + 3,
      1024 + 4, 1024 + 5, 1024 + 6, 1024 + 7, 512 - 7,  512 - 6,  512 - 5,
      512 - 4,  512 - 3,  512 - 2,  512 - 1,  512,      512 + 1,  512 + 2,
      512 + 3,  512 + 4,  512 + 5,  512 + 6,  512 + 7};
  std::uniform_int_distribution<int> lengths_distribution{
      0, static_cast<int>(lengths.size() - 1)};

  for (int i = 0; i < 100000; ++i) {
    WriteRecipe recipe;
    int number_of_writes = count_distribution(engine2);
    for (int j = 0; j < number_of_writes; ++j) {
      int number_of_chunks = count_distribution(engine2);
      std::vector<int> r;
      for (int k = 0; k < number_of_chunks; ++k) {
        r.emplace_back(lengths[lengths_distribution(engine2)]);
      }
      recipe.emplace_back(std::move(r));
    }

    TestRecipe(recipe);
  }
}

// Test an aligned to unaligned transition to make sure everything works.
TEST_F(FileWriteTestBase, AlignedToUnaligned) {
  AllocatorResizeableBuffer<AlignedReallocator> aligned_buffer;
  AllocatorResizeableBuffer<AlignedReallocator> unaligned_buffer;

  aligned_buffer.resize(FileHandler::kSector * 4);
  std::generate(std::begin(aligned_buffer), std::end(aligned_buffer),
                [this]() { return NextRandom(); });

  unaligned_buffer.resize(FileHandler::kSector * 4);
  std::generate(std::begin(unaligned_buffer), std::end(unaligned_buffer),
                [this]() { return NextRandom(); });

  const size_t kOffset = 53;
  absl::Span<const uint8_t> unaligned_span(unaligned_buffer.data() + kOffset,
                                           aligned_buffer.size() - kOffset);

  std::vector<absl::Span<const uint8_t>> queue;

  queue.emplace_back(aligned_buffer.data(), aligned_buffer.size());
  queue.emplace_back(unaligned_span);
  LOG(INFO) << "Queue 0 " << queue[0].size();
  LOG(INFO) << "Queue 1 " << queue[1].size();

  const std::string logevent = aos::testing::TestTmpDir() + "/";
  const auto file = std::filesystem::path(logevent) / "test.log";
  std::filesystem::remove_all(file);
  VLOG(1) << "Writing to " << file.c_str();

  FileBackend backend(logevent);
  auto handler = backend.RequestFile("test.log");
  ASSERT_EQ(handler->OpenForWrite(), WriteCode::kOk);

  auto result = handler->Write(queue);
  EXPECT_EQ(result.code, WriteCode::kOk);
  EXPECT_EQ(result.messages_written, queue.size());

  ASSERT_EQ(handler->Close(), WriteCode::kOk);
  EXPECT_TRUE(std::filesystem::exists(file));
  const size_t requested_size = queue[0].size() + queue[1].size();
  EXPECT_EQ(std::filesystem::file_size(file), requested_size);

  // Confirm that the contents then match the original buffer.
  std::ifstream file_stream(file, std::ios::in | std::ios::binary);
  std::vector<uint8_t> content((std::istreambuf_iterator<char>(file_stream)),
                               std::istreambuf_iterator<char>());
  ASSERT_EQ(content.size(), requested_size);
  bool matches = true;
  for (size_t i = 0; i < queue[0].size(); ++i) {
    if (content[i] != aligned_buffer.data()[i]) {
      matches = false;
      break;
    }
  }
  for (size_t i = 0; i < queue[1].size(); ++i) {
    if (content[i + queue[0].size()] != unaligned_span.data()[i]) {
      matches = false;
      break;
    }
  }
  if (!matches) {
    ASSERT_TRUE(false);
  }
}

struct FileWriteTestFixture : public ::testing::WithParamInterface<WriteRecipe>,
                              public FileWriteTestBase {};

TEST_P(FileWriteTestFixture, CheckSizeOfWrittenFile) {
  auto recipe = GetParam();
  TestRecipe(recipe);
}

// Try out some well known failure cases transitioning across the alignment
// boundary.
INSTANTIATE_TEST_SUITE_P(
    FileWriteTest, FileWriteTestFixture,
    ::testing::Values(WriteRecipe{{0x10000}}, WriteRecipe{{0x10000, 0x1000b5}},
                      WriteRecipe{{0x10000, 0x1000b5}, {0xfff4b, 0x10000}},
                      WriteRecipe{{0x1000b5, 0xfff4b}, {0x10000}},
                      WriteRecipe{{65536, 517, 65717}},
                      WriteRecipe{{65536, 517, 518, 511},
                                  {514},
                                  {505, 514},
                                  {505, 514, 65355, 519}},
                      WriteRecipe{{65536, 518, 511, 511},
                                  {65717},
                                  {65717, 65717, 518},
                                  {65536, 65536, 508, 65355},
                                  {515, 519}},
                      WriteRecipe{{0x1000b5, 0xfff4b, 0x100000}, {0x10000}}));

}  // namespace aos::logger::testing
