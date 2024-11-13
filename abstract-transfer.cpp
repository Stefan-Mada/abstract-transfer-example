#include <iostream>
#include "llvm/ADT/APInt.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/KnownBits.h"
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

ostream& operator<<(ostream& os, const KnownBits& dt)
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
    
    os << retStr;
    return os;
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



int main() {
  auto allEnums = enumerateAllValues(2);
  for(const auto& knownBit : allEnums)
    outs() << knownBit << " ";
  outs() << "\n";

  for(const auto& knownBit : allEnums) {
    outs() << knownBit << ": ";
    for(auto bitSet : concretization(knownBit)) {
      bitSet.print(outs(), false);
      outs() << ", ";
    }
    outs() << "\n";
  }
      
  outs() << "\n";
}