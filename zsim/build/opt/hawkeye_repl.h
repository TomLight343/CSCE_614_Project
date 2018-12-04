#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_

#include "repl_policies.h"
#define LOOK_BACK_RANGE 8
#define MAX_RPV 7
#define MAX_HAWK_VAL 7
#define CACHE_FRIENDLY_MIN 4

// Hawkeye
class HawkeyeReplPolicy : public ReplPolicy {
    protected:
        // add class member variables here
        typedef struct SetOccupancyVector{
            Address address;
            uint8_t entry;
        }SetOccupancyVector;
        typedef struct OccupancyVector {
            SetOccupancyVector *_setOccupancyVector;
            uint32_t end = 0;
        }OccupancyVector;

        OccupancyVector *_occupancyVector;

	uint32_t* rpvArray;
	uint32_t* hawkeyePredictor;
	bool* recentlyAdded;

        const uint32_t numLines;
        const uint32_t numWays;
        const uint32_t numOffsetBits;
        const uint32_t numIndexBits;
        const uint32_t numOfNonTagBits;
        const uint32_t sizeOfSetOccupancyVector;

    public:
        // add member methods here, refer to repl_policies.h
        HawkeyeReplPolicy(uint32_t _numLines, uint32_t _numWays, uint32_t lineSize):
            numLines(_numLines),
            numWays(_numWays),
            numOffsetBits(ceil(log2(lineSize/8))),
            numIndexBits(ceil(log2(numLines))),
            numOfNonTagBits(numOffsetBits + numIndexBits),
            sizeOfSetOccupancyVector(numWays*LOOK_BACK_RANGE)
        {
            _occupancyVector = gm_calloc<OccupancyVector>(numWays);
            for(uint32_t i = 0; i < numWays; i++){
                _occupancyVector[i]._setOccupancyVector = gm_calloc<SetOccupancyVector>(sizeOfSetOccupancyVector);
            }
		// Initialize RRIP array for cache replacement
		rpvArray = gm_calloc<uint32_t>(numLines);
		for (uint32_t i = 0; i < numLines; i++) {rpvArray[i] = MAX_RPV;}

		// Array that tracks whether or not a given block was only just recently put in cache
		recentlyAdded = gm_calloc<bool>(numLines);
		for (uint32_t j = 0; j < numLines; j++) {recentlyAdded[j] = false;}

		// Initialize Hawkeye Predictor array
		hawkeyePredictor = gm_calloc<uint32_t>(numLines);
		for (uint32_t i = 0; i < numLines; i++) {hawkeyePredictor[i] = 0;}
        }

        ~HawkeyeReplPolicy(){
            for(uint32_t i = 0; i < numWays; i++){
                gm_free(_occupancyVector[i]._setOccupancyVector);
            }
            gm_free(_occupancyVector);
        }

        void update(uint32_t id, const MemReq* req) {
		if (updateOptGen(req)) {
			if (hawkeyePredictor[id] != MAX_HAWK_VAL) {hawkeyePredictor[id]++;}
		}
		else {
			if (hawkeyePredictor[id] != 0) {hawkeyePredictor[id]--;}
		}

		if (hawkeyePredictor[id] >= CACHE_FRIENDLY_MIN) {
			rpvArray[id] = 0;
			if (recentlyAdded[id]) {
				recentlyAdded[id] = false;
				for (uint32_t i = 0; i < numLines; i++) {if (i != id) {rpvArray[i]++;}}
			}
		}
		else {
			rpvArray[id] = 7;
		}
        }

        void replaced(uint32_t id) {
            recentlyAdded[id] = true;
        }

        template <typename C> uint32_t rank(const MemReq* req, C cands) {
		uint32_t oldestRpvIndex = *(cands.begin()); // Needed if no block has the maximum RRIP value
		uint32_t oldestRpvEncountered = rpvArray[oldestRpvIndex];
		for (auto ci = cands.begin(); ci != cands.end(); ci.inc()) {
			if (rpvArray[*ci] == MAX_RPV) {return *ci;}
			else if (rpvArray[*ci] > oldestRpvEncountered) {
				oldestRpvIndex = *ci;
				oldestRpvEncountered = rpvArray[*ci];
			}
            	}

		return oldestRpvIndex;
        }
        //DECL_RANK_BINDINGS;

    private:
        inline uint32_t getCacheSet(const MemReq* req){
            return (req->lineAddr >> numOffsetBits) & ((1<<numIndexBits)-1);
        }
        inline Address getSearchAddress(const MemReq* req){
            return req->lineAddr >> numOfNonTagBits;
        }
        inline int64_t getLastIndexOf(
            SetOccupancyVector *_vector,
            Address address,
            uint32_t end,
            uint32_t maxSize
        ){
            for(uint32_t i = end; i != (end+1); i--){
                // underflow condition
                if(i == 0){
                    i = maxSize-1;
                }
                if(_vector[i].address == address){
                    return i;
                }
            }
            return -1;
        }
        inline bool isOptMiss(
            SetOccupancyVector *_vector,
            uint32_t start,
            uint32_t end,
            uint32_t numWays,
            uint32_t maxSize
        ){
            for(uint32_t i = start; i != end; i++){
                // overflow condition
                if (i == maxSize){
                    i = 0;
                }
                if(_vector[i].entry >= numWays){
                    return true;
                }
            }
            return false;
        }
        inline void updateStateOfOccupancyVector(
            SetOccupancyVector *_vector,
            uint32_t start,
            uint32_t end,
            uint32_t maxSize
        ){
            for(uint32_t i = start; i != end; i++){
                // overflow condition
                if(i == maxSize){
                    i = 0;
                }
                _vector[i].entry++;
            }
        }
        bool updateOptGen(const MemReq* req){
            bool returnState = false;
            uint32_t set = getCacheSet(req);
            Address searchAddress = getSearchAddress(req);
            int64_t lastIndexOfSearchAddress = getLastIndexOf(
                _occupancyVector[set]._setOccupancyVector,
                searchAddress,
                _occupancyVector[set].end,
                sizeOfSetOccupancyVector
            );
            // check if address in present in occupancy vector
            if(lastIndexOfSearchAddress > 0){
                // check if address causes a miss condition
                if(isOptMiss(
                    _occupancyVector[set]._setOccupancyVector,
                    (uint32_t)lastIndexOfSearchAddress,
                    _occupancyVector[set].end,
                    numWays,
                    sizeOfSetOccupancyVector
                    )
                ){
                    returnState = false;
                }
                // if its a hit condition update each entry in occupancy vector
                else{
                    updateStateOfOccupancyVector(
                        _occupancyVector[set]._setOccupancyVector,
                        (uint32_t)lastIndexOfSearchAddress,
                        _occupancyVector[set].end,
                        sizeOfSetOccupancyVector
                    );
                    returnState = true;
                }
            }
            // add the new entry in occupancy vector
            _occupancyVector[set]._setOccupancyVector[_occupancyVector[set].end].entry = 0;
            _occupancyVector[set]._setOccupancyVector[_occupancyVector[set].end].address = searchAddress;

            // move the end index of occupancy vector
            _occupancyVector[set].end++;
            // roll to start incase of overflow
            if(_occupancyVector[set].end > sizeOfSetOccupancyVector){
                _occupancyVector[set].end = 0;
            }

            return returnState;
        }

        DECL_RANK_BINDINGS;
};
#endif // HAWKEYE_REPL_H_
