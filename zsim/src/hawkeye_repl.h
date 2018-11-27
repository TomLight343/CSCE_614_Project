#ifndef HAWKEYE_REPL_H_
#define HAWKEYE_REPL_H_

#include "repl_policies.h"
#define LOOK_BACK_RANGE 8

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
            for(int i = 0; i < numWays; i++){
                _occupancyVector[i]._setOccupancyVector = gm_calloc<SetOccupancyVector>(sizeOfSetOccupancyVector);
            }
        }

        ~HawkeyeReplPolicy(){
            for(int i = 0; i < numWays; i++){
                gm_free(_occupancyVector[i]._setOccupancyVector);
            }
            gm_free(_occupancyVector);
        }

        void update(uint32_t id, const MemReq* req) {
            // update method
        }

        void replaced(uint32_t id) {
            // replaced method
        }

        template <typename C> uint32_t rank(const MemReq* req, C cands) {
            // rank method
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
            for(int i = end; i != (end+1); i--){
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
            for(int i = start; i != end; i++){
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
            for(int i = start; i != end; i++){
                // overflow condition
                if(i == maxSize){
                    i = 0;
                }
                _vector[i].entry++;
            }
        }
        bool updateOptGen(const MemReq* req){
            bool returnState;
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
};
#endif // HAWKEYE_REPL_H_
