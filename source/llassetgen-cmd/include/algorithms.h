#pragma once

#include <map>
#include <set>
#include <string>

#include <llassetgen/DistanceTransform.h>
#include <llassetgen/Image.h>
#include <llassetgen/Packing.h>

using namespace llassetgen;

using VecIter = std::vector<Vec2<size_t>>::const_iterator;
using ImageTransform = void (*)(Image&, Image&);

std::map<std::string, ImageTransform> dtAlgos{
    {"deadrec", [](Image& input, Image& output) { DeadReckoning(input, output).transform(); }},
    {"parabola", [](Image& input, Image& output) { ParabolaEnvelope(input, output).transform(); }},
};

std::map<std::string, Packing (*)(VecIter, VecIter, bool)> packingAlgos{
    {"shelf", shelfPackAtlas},
    {"maxrects", maxRectsPackAtlas}
};

std::map<std::string, ImageTransform> downsamplingAlgos{
    {"center", [](Image& input, Image& output) { input.centerDownsampling<DistanceTransform::OutputType>(output); }},
    {"average", [](Image& input, Image& output) { input.averageDownsampling<DistanceTransform::OutputType>(output); }},
    {"min", [](Image& input, Image& output) { input.minDownsampling<DistanceTransform::OutputType>(output); }}
};

template <class Func>
std::set<std::string> algoNames(const std::map<std::string, Func> & map) {
    std::set<std::string> names;
    for (const auto& mapEntry : map) {
        names.insert(mapEntry.first);
    }
    return names;
}
