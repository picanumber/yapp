#include "test_common.h"
#include "yap/pipeline.h"
#include "yap/runtime_utilities.h"

#include <gtest/gtest.h>

#include <iterator>
#include <memory>
#include <string>
#include <vector>

TEST(TestConsumeUtility, CheckVectorConsume)
{
    std::vector<std::string> words{"one", "two",   "three", "four", "five",
                                   "six", "seven", "eight", "nine", "ten"};
    auto wordsCopy{words};

    yap::Consume consumer(words.begin(), words.end());

    // Check the consumer produced correct results.
    for (auto &w : wordsCopy)
    {
        EXPECT_EQ(w, consumer());
    }
    EXPECT_THROW(consumer(), yap::GeneratorExit);

    // Check the source container is in the expected state.
    EXPECT_EQ(words.size(), wordsCopy.size());
    EXPECT_EQ(words, wordsCopy);
}

TEST(TestConsumeUtility, CheckVectorMoveConsume)
{
    std::vector<std::string> words{"one", "two",   "three", "four", "five",
                                   "six", "seven", "eight", "nine", "ten"};
    std::vector<std::unique_ptr<std::string>> uwords;
    std::vector<std::unique_ptr<std::string>> uwordsDest;

    for (auto &w : words)
    {
        uwords.emplace_back(std::make_unique<std::string>(w));
    }

    yap::Consume consumer(std::make_move_iterator(uwords.begin()),
                          std::make_move_iterator(uwords.end()));

    // Check the consumer produced correct results.
    for (auto &w : words)
    {
        uwordsDest.emplace_back(consumer());
        EXPECT_EQ(w, *uwordsDest.back());
    }
    EXPECT_THROW(consumer(), yap::GeneratorExit);

    // Check the source container is in the expected state.
    EXPECT_EQ(words.size(), uwords.size());
    EXPECT_EQ(uwords.size(), uwordsDest.size());

    for (auto &w : uwords)
    {
        EXPECT_FALSE(w); // A moved from unique pointer is nil.
    }

    for (std::size_t i(0); i < uwordsDest.size(); ++i)
    {
        EXPECT_EQ(words[i], *uwordsDest[i]);
    }
}

TEST(TestConsumeUtility, CheckContainerConsumingPipeline)
{
    std::vector<std::size_t> charCount;
    std::vector<std::string> words{"The", "wheels", "on",  "the",   "bus",
                                   "go",  "round",  "and", "round", "!"};

    auto cCounter =
        yap::Pipeline{} | yap::Consume(words.begin(), words.end()) |
        [](std::string s) { return s.length(); } |
        [&charCount](std::size_t nChars) { charCount.push_back(nChars); };

    cCounter.consume();

    EXPECT_EQ(charCount.size(), words.size());

    for (auto [it1, it2] = std::make_tuple(charCount.begin(), words.begin());
         it1 != charCount.end() && it2 != words.end(); ++it1, ++it2)
    {
        EXPECT_EQ(*it1, it2->length());
    }
}

TEST(TestConsumeUtility, CheckContainerMoveConsumingPipeline)
{
    std::vector<std::size_t> charCount;
    std::vector<std::string> words{"The", "wheels", "on",  "the",   "bus",
                                   "go",  "round",  "and", "round", "!"};
    std::vector<std::string> wordsCopy{words};

    auto cCounter =
        yap::Pipeline{} |
        yap::Consume(std::make_move_iterator(words.begin()),
                     std::make_move_iterator(words.end())) |
        [](std::string s) { return s.length(); } |
        [&charCount](std::size_t nChars) { charCount.push_back(nChars); };

    cCounter.consume();

    EXPECT_EQ(charCount.size(), words.size());

    for (auto [it1, it2] =
             std::make_tuple(charCount.begin(), wordsCopy.begin());
         it1 != charCount.end() && it2 != wordsCopy.end(); ++it1, ++it2)
    {
        EXPECT_EQ(*it1, it2->length());
    }
}
