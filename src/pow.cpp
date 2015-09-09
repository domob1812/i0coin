// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

// Return the target (difficulty) to the new block based on the pindexLast block
unsigned int static GetNextWorkRequired_OLD(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const arith_uint256 powLimit = UintToArith256(params.powLimit);

    /* these values shadow the global ones with the same name
     * this function is only used for blocks before 14640... */
    const int64_t nTargetTimespan = 7 * 24 * 60 * 60; // one week
    const int64_t nTargetSpacing = 5 * 60;
    const int64_t nInterval = nTargetTimespan / nTargetSpacing;

    // Genesis block
    if (pindexLast == NULL)
        return powLimit.GetCompact();

    int64_t nRemaining = (pindexLast->nHeight+1) % nInterval;

    // Only change once per interval
    if ( nRemaining != 0)
	{
/*
		const CBlockIndex* pindexFirst = pindexLast;
		for (int i = 0; pindexFirst && i < nRemaining-1; i++)
			pindexFirst = pindexFirst->pprev;
		assert(pindexFirst);

		int64 rema = GetAdjustedTime() - pindexFirst->GetBlockTime();

		if(rema < nTargetTimespan)
*/
			return pindexLast->nBits;
	}

    // Go back by what we want to be 7 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval-1; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %ld  before bounds\n", nActualTimespan);
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > powLimit)
        bnNew = powLimit;

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("nTargetTimespan = %ld    nActualTimespan = %ld\n", nTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x\n", pindexLast->nBits);
    LogPrintf("After:  %08x\n", bnNew.GetCompact());

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    unsigned int nProofOfWorkLimit = powLimit.GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    const int height = pindexLast->nHeight + 1;

    //okay, maybe not this line
    if (height < 14640)
       return GetNextWorkRequired_OLD(pindexLast, params);
    //hardcoded switch to 256.0 difficulty at block 14639
    if (height == 14640)
       return 0x1C00FFFF;

    // Only change once per interval
    const int64_t nInterval = params.DifficultyAdjustmentInterval();
    const int64_t nTargetSpacing = params.nPowTargetSpacing;
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        // Special difficulty rule for testnet:
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }

        return pindexLast->nBits;
   }

   // This fixes an issue where a 51% attack can change difficulty at will.
   // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
   // Patch modified from Litecoin.
   int blockstogoback = nInterval-1;
   if (height >= 150000 && height != nInterval)
        blockstogoback = nInterval;

   // Go back by what we want to be 14 days worth of blocks
   const CBlockIndex* pindexFirst = pindexLast;
   for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
   assert(pindexFirst);

   // Limit adjustment step
   const int64_t nTargetTimespan = params.nPowTargetTimespan;
   int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
   int64_t nTwoPercent = nTargetTimespan/50;
   //printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);

   if (nActualTimespan < nTargetTimespan)  //is time taken for a block less than 3minutes?
   {
        //limit increase to a much lower amount than dictates to get past the pump-n-dump mining phase
        //due to retargets being done more often it also needs to be lowered significantly from the 4x increase
        if(nActualTimespan<(nTwoPercent*16)) //less than a minute?
            nActualTimespan=(nTwoPercent*45); //pretend it was only 10% faster than desired
        else if(nActualTimespan<(nTwoPercent*32)) //less than 2 minutes?
            nActualTimespan=(nTwoPercent*47); //pretend it was only 6% faster than desired
        else
            nActualTimespan=(nTwoPercent*49); //pretend it was only 2% faster than desired

        //int64 nTime=nTargetTimespan-nActualTimespan;
        //nActualTimespan = nTargetTimespan/2;
    }
    else if (nActualTimespan > nTargetTimespan*4)   nActualTimespan = nTargetTimespan*4;

    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > powLimit)
        bnNew = powLimit;

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("nTargetTimespan = %ld    nActualTimespan = %ld\n", nTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x\n", pindexLast->nBits);
    LogPrintf("After:  %08x\n", bnNew.GetCompact());

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return error("CheckProofOfWork(): hash doesn't match nBits");

    return true;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
