#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

// Static RRIP
class SRRIPReplPolicy : public ReplPolicy {
    protected: 
        uint32_t* array;
        uint32_t numLines;
	uint32_t maxRpv;
	bool* recentlyAdded; 

    public:
        SRRIPReplPolicy(uint32_t _numLines, uint32_t _rpvMax) {
		maxRpv = _rpvMax;
		numLines = _numLines;
		array = gm_calloc<uint32_t>(numLines);
		for (uint32_t i = 0; i < numLines; i++) {array[i] = maxRpv;}
		recentlyAdded = gm_calloc<bool>(numLines);
		for (uint32_t j = 0; j < numLines; j++) {recentlyAdded[j] = false;}
	}

        ~SRRIPReplPolicy() {
            gm_free(array);
        }

        void update(uint32_t id, const MemReq* req) {
		if (recentlyAdded[id] == true) {
			recentlyAdded[id] = false;
            		array[id] = (maxRpv - 1);
		}
		else {
			array[id] = 0;
		}
        }

        void replaced(uint32_t id) {
		recentlyAdded[id] = true;
        }

        template <typename C> inline uint32_t rank(const MemReq* req, C cands) {
		for (;;) {
			for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
				if (array[*ci] == maxRpv) {return *ci;}
            		}
			for (auto cj = cands.begin(); cj != cands.end(); cj.inc()) {
				if (array[*cj] != maxRpv) {array[*cj]++;}
            		}
		}
        }

        DECL_RANK_BINDINGS;
};
#endif // RRIP_REPL_H_
