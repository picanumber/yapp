#include "yap/pipeline.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

class FileReader
{
    std::shared_ptr<std::ifstream> _input;

  public:
    FileReader(const char *fname)
        : _input(std::make_shared<std::ifstream>(fname))
    {
        if (!_input->is_open())
        {
            throw std::logic_error("Cannot open input file");
        }
    }

    std::string operator()()
    {
        std::string line;
        if (!std::getline(*_input, line))
        {
            throw yap::GeneratorExit{};
        }

        return line;
    }
};

struct LineSplitter
{
    std::vector<std::string> operator()(std::string line)
    {
        std::string word;
        std::vector<std::string> words;

        std::istringstream iss(line);
        while (std::getline(iss, word, ' '))
        {
            words.emplace_back(std::move(word));
        }

        return words;
    }
};

class FrequencyCounter
{
    std::unordered_map<std::string, std::size_t> _wordFreq;

  public:
    std::vector<std::pair<std::size_t, std::string>> operator()(
        std::vector<std::string> newWords)
    {
        std::vector<std::pair<std::size_t, std::string>> newFreqs;

        for (std::string const &word : newWords)
        {
            auto [it, isNew] = _wordFreq.try_emplace(word, 1);
            if (!isNew)
            {
                it->second += 1;
            }

            newFreqs.emplace_back(it->second, word);
        }

        return newFreqs;
    }
};

class KTopWords
{
    using list_t = std::list<std::pair<std::size_t, std::string>>;

    list_t _kMostFrequent;
    std::unordered_map<std::string, list_t::iterator> _index;
    std::size_t _k;

  public:
    explicit KTopWords(std::size_t K) : _k(K)
    {
        if (0 == _k)
        {
            throw std::logic_error("What do you mean top 0?");
        }
    }

    void operator()(std::vector<std::pair<std::size_t, std::string>> newFreqs)
    {
        for (auto &[freq, word] : newFreqs)
        {
            if (_kMostFrequent.empty() || _kMostFrequent.back().first < freq)
            {
                auto it = _index.find(word);
                if (it != _index.end())
                {
                    auto listIt = it->second;

                    // Place word where its new frequency would mandate.
                    _kMostFrequent.splice(
                        std::lower_bound(
                            _kMostFrequent.begin(), _kMostFrequent.end(),
                            std::make_pair(freq, word), std::greater<>{}),
                        _kMostFrequent, listIt);

                    listIt->first = freq; // Update the frequency value.
                }
                else
                {
                    auto newItemIt = _kMostFrequent.insert(
                        std::lower_bound(
                            _kMostFrequent.begin(), _kMostFrequent.end(),
                            std::make_pair(freq, word), std::greater<>{}),
                        std::make_pair(freq, word));
                    _index.emplace(word, newItemIt);

                    if (_kMostFrequent.size() > _k)
                    {
                        auto w = _kMostFrequent.back().second;
                        _kMostFrequent.pop_back();
                        _index.erase(w);
                    }
                }
            }
        }
    }

    auto get() const
    {
        return _kMostFrequent;
    }
};

} // namespace

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please provide an input file\n";
        return 1;
    }

    int k = 10;
    if (argc > 2)
    {
        k = std::stoi(argv[2]);
    }

    std::cout << "Counting top " << k << " words of " << argv[1] << std::endl;

    auto topW = std::make_shared<KTopWords>(k);

    auto pl = yap::Pipeline{} | FileReader(argv[1]) | LineSplitter{} |
              FrequencyCounter{} | [topW](auto newFreq) { (*topW)(newFreq); };

    auto start = std::chrono::steady_clock::now();
    pl.consume();
    auto dur = std::chrono::steady_clock::now() - start;

    std::cout
        << "\nTop " << k << " words discovered in "
        << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
        << " ms\n\n";

    for (auto [freq, word] : topW->get())
    {
        std::cout << freq << " : " << word << std::endl;
    }

    return 0;
}
