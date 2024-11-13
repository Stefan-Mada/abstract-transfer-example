#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/ValueTracking.h"
#include <stdexcept>
#include <vector>

using namespace std;
using namespace llvm;

vector<KnownBits> enumerateAllValues(size_t bitwidth) {
  vector<KnownBits> retVec;
  if(bitwidth == 0)
    return {};

  retVec.emplace_back(bitwidth);

  vector<KnownBits> tempVec;
  for(size_t i = 0; i < bitwidth; ++i) {
    for(auto knownBit : retVec) {
      // first get the case where it's unknown
      tempVec.push_back(knownBit);
      // then get the case where it's a known 1
      knownBit.One.setBit(i);
      tempVec.push_back(knownBit);
      // then get the case where it's a known 0
      knownBit.One.setBitVal(i, 0);
      knownBit.Zero.setBit(i);
      tempVec.push_back(knownBit);
    }
    retVec = tempVec;
    tempVec.clear();
  }

  return retVec;
}

string getStr(const KnownBits& dt)
{
    string retStr;

    for(size_t i = 0; i < dt.getBitWidth(); ++i) {
      if(dt.One[i] && dt.Zero[i])
        throw invalid_argument("How did we get to this impossibility?");
      else if(dt.One[i])
        retStr += "1";
      else if(dt.Zero[i])
        retStr += "0";
      else
        retStr += "T";
    }
    
    return retStr;
}

vector<APInt> concretization(const KnownBits& knownBits) {
  vector<KnownBits> retVec;

  const size_t bitwidth = knownBits.getBitWidth();
  retVec.push_back(knownBits);

  vector<KnownBits> tempVec;
  for(size_t i = 0; i < bitwidth; ++i) {
    for(auto knownBit : retVec) {
      if(knownBit.One[i] && knownBit.Zero[i])
        throw invalid_argument("How did we get to this impossibility?");
      else if(knownBit.One[i] || knownBit.Zero[i])
        tempVec.push_back(knownBit);
      else {
        knownBit.One.setBit(i);
        tempVec.push_back(knownBit);
        // then get the case where it's a known 0
        knownBit.One.setBitVal(i, 0);
        knownBit.Zero.setBit(i);
        tempVec.push_back(knownBit);
      }
    }
    retVec = tempVec;
    tempVec.clear();
  }

  vector<APInt> realRetVec;
  for(const auto& bit : retVec) {
    realRetVec.push_back(bit.getConstant());
  }

  return realRetVec;
}

KnownBits abstraction(const vector<APInt>& vals) {
  KnownBits bestGuess;
  if(vals.empty())
    return bestGuess;

  bestGuess.One = vals[0];
  bestGuess.Zero = ~vals[0];

  for(size_t i = 1; i < vals.size(); ++i) {
    bestGuess.One &= vals[i];
    bestGuess.Zero &= ~vals[i];
  }

  return bestGuess;
}

vector<pair<KnownBits&, KnownBits&>> getAllPairs(vector<KnownBits>& fullSet) {
  vector<pair<KnownBits&, KnownBits&>> retVec;
  for(size_t i = 0; i < fullSet.size(); ++i) {
    for(size_t j = i; j < fullSet.size(); ++j) {
      pair<KnownBits&, KnownBits&> currPair = {fullSet[i], fullSet[j]};
      retVec.push_back(currPair);
    }
  }
  return retVec;
}

vector<APInt> computeAllConcretePairs(const vector<APInt>& vals1, const vector<APInt>& vals2) {
  vector<APInt> retVec;
  for(size_t i = 0; i < vals1.size(); ++i) {
    for(size_t j = 0; j < vals2.size(); ++j) {
      retVec.push_back(vals1[i].sadd_sat(vals2[j]));
    }
  }
  return retVec;
}

// is bit1 more precise than bit2 (less unknowns)
bool morePrecise(const KnownBits& bit1, const KnownBits& bit2) {
  size_t unknown1 = 0;
  size_t unknown2 = 0;
  
  for(size_t i = 0; i < bit1.getBitWidth(); ++i)
    if(bit1.One == 0 && bit1.Zero == 0)
      ++unknown1;

  for(size_t i = 0; i < bit2.getBitWidth(); ++i)
    if(bit2.One == 0 && bit2.Zero == 0)
      ++unknown2;

  return unknown1 < unknown2;
}

bool isIncomparable(const KnownBits& bit1, const KnownBits& bit2) {
  for(size_t i = 0; i < bit1.getBitWidth(); ++i) {
    if(bit1.One[i] && bit2.Zero[i])
      return true;
    if(bit2.One[i] && bit1.Zero[i])
      return true;
  }

  return false;
}

void compareTransferFunctions(size_t bitwidth) {
  size_t naiveMorePreciseCount = 0;
  size_t naiveLessPreciseCount = 0;
  size_t naiveSamePrecisionCount = 0;
  size_t incomparableCount = 0;
  size_t totalPairsConcreteValCount = 0;

  auto fullKnownBits = enumerateAllValues(bitwidth);
  const auto allAbstractPairs = getAllPairs(fullKnownBits);

  for(const auto& [bits1, bits2] : allAbstractPairs) {
    auto llvmKnownResults = KnownBits::sadd_sat(bits1, bits2);

    const auto concreteVals1 = concretization(bits1);
    const auto concreteVals2 = concretization(bits2);
    const auto concreteResults = computeAllConcretePairs(concreteVals1, concreteVals2);
    const auto naiveKnownResults = abstraction(concreteResults);

    bool isNaiveMorePrecise = morePrecise(naiveKnownResults, llvmKnownResults);
    bool isNaiveLessPrecise = morePrecise(llvmKnownResults, naiveKnownResults);
    if(isNaiveLessPrecise)
      ++naiveLessPreciseCount;
    if(isNaiveMorePrecise)
      ++naiveMorePreciseCount;
    if(!isNaiveLessPrecise && !isNaiveMorePrecise)
      ++naiveSamePrecisionCount;

    if(isIncomparable(naiveKnownResults, llvmKnownResults))
      ++incomparableCount;
    totalPairsConcreteValCount += concreteResults.size();
  }

  outs() << "Total abstract values in this domain: " << fullKnownBits.size() << "\n";
  outs() << "Total pairs of abstract values in this domain: " << allAbstractPairs.size() << "\n";
  outs() << "Total pairs of concrete values evaluated in this domain: " << totalPairsConcreteValCount << "\n\n";
  outs() << "Naive approach was more precise: " << naiveMorePreciseCount << " times\n";
  outs() << "Naive approach was less precise: " << naiveLessPreciseCount << " times\n";
  outs() << "Naive approach was equally precise: " << naiveSamePrecisionCount << " times\n";
  outs() << "Naive approach was incomparable: " << incomparableCount << " times\n";
}

int main(int argc, char** argv) {
  if(argc != 2) {
    errs() << "Must input one value representing bit width to evaluate, exiting.\n";
    exit(1);
  }
  compareTransferFunctions(stoul(argv[1]));
}