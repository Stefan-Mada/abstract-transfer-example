#include <iostream>
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/raw_ostream.h"
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

  const size_t bitwidth = vals[0].getBitWidth();

  for(size_t i = 1; i < vals.size(); ++i) {
    bestGuess.One &= vals[i];
    bestGuess.Zero &= ~vals[i];
  }

  return bestGuess;
}



int main() {
  auto allEnums = enumerateAllValues(2);
  for(const auto& knownBit : allEnums)
    outs() << getStr(knownBit) << " ";
  outs() << "\n";

  for(const auto& knownBit : allEnums) {
    outs() << getStr(knownBit) << ": ";
    auto concreteVals = concretization(knownBit);
    for(auto bitSet : concreteVals) {
      bitSet.print(outs(), false);
      outs() << ", ";
    }
    outs() << " : ABSTRACT: " << getStr(abstraction(concreteVals)) << "\n";

    outs() << "\n";
  }

  outs() << "FINAL: " << getStr(abstraction(vector{APInt(4, 10), APInt(4,1)})) << "\n";
      
  outs() << "\n";
}