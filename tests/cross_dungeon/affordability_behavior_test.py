#!/usr/bin/env python3
"""Exercise the actual headless affordability canary with real shared-item ownership views."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
text = (root / 'combo/ComboRandoHeadless.cpp').read_text()
begin = text.index('auto affordabilityViolations =')
end = text.index('\n        };', begin) + len('\n        };')
body = text[begin:end]
fixture = r'''
#include "combo/rando/ComboPlaythrough.h"
#include <cassert>
#include <iostream>
using namespace ComboRando;
using Json = nlohmann::json;
int main() {
    Json ootPrices = Json::object(), mmPrices = {{"Curiosity Shop Special Item",166}};
    int ootBaseWalletTier=1;
    uint32_t sharedMask=0;
    std::vector<CwSharedPair> pairs{{"Progressive Wallet","Progressive Wallet"}};
    auto affordabilitySharedGroups=BuildSharedItemGroups(pairs,sharedMask);
    CANARY
    Json wallet={{"game","oot"},{"check","Free wallet"},{"item","Progressive Wallet"},
                 {"foreign",false},{"advancement",true}};
    Json purchase={{"game","mm"},{"check","Curiosity Shop Special Item"},
                   {"item","Stone Tower Stray Fairy"},{"foreign",false},{"advancement",true}};
    Json walk=Json::array({{{"sphere",0},{"steps",Json::array({wallet})}},
                          {{"sphere",1},{"steps",Json::array({purchase})}}});
    // Native NEI sharing exists even with the optional Shared Items mask off.
    assert(affordabilityViolations(walk)==0);
    affordabilitySharedGroups.clear();
    assert(affordabilityViolations(walk)==1);
    affordabilitySharedGroups=BuildSharedItemGroups(pairs,0);
    Json sameSphere=Json::array({{{"sphere",0},{"steps",Json::array({wallet,purchase})}}});
    assert(affordabilityViolations(sameSphere)==1);
    // A foreign wallet is credited to its real home, then mirrored by the same groups.
    wallet["game"]="mm";wallet["foreign"]=true;
    walk[0]["steps"]=Json::array({wallet});
    assert(affordabilityViolations(walk)==0);
    std::cout<<"production affordability: native shared wallets, foreign ownership and sphere ordering PASS\n";
}
'''.replace('CANARY', body)
with tempfile.TemporaryDirectory(prefix='affordability-') as tmp:
    tmp=Path(tmp)
    source=tmp/'canary.cpp';source.write_text(fixture)
    binary=tmp/'canary'
    subprocess.run(['g++','-std=c++20','-I',str(root),'-I',str(root/'combo'),'-I',str(root.parent/'build-deps/usr/include'),
                    str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
