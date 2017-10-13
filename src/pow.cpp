// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{

    
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;
    
    

    const CBlockIndex* pindexFirst = pindexLast;
    arith_uint256 bnTot {0};
    for (int i = 0; pindexFirst && i < params.nPowAveragingWindow; i++) {
        arith_uint256 bnTmp;
        bnTmp.SetCompact(pindexFirst->nBits);
        bnTot += bnTmp;
        pindexFirst = pindexFirst->pprev;
    }
    
    if (pindexFirst == NULL)
        return nProofOfWorkLimit;
    
    arith_uint256 bnAvg {bnTot / params.nPowAveragingWindow};
    
    
//    int nHeightNext = pindexLast->nHeight + 1;
//    if (nHeightNext >= params.BTGHeight  && nHeightNext < params.BTGHeight + params.BTGPremineWindow)
//    {
//        // Lowest difficulty for Bitcoin GPU premining period.
//        return nProofOfWorkLimit;
//    }
//    else if (nHeightNext % params.DifficultyAdjustmentInterval() != 0)
//    {
//        // Difficulty adjustment interval is not finished. Keep the last value.
//        if (params.fPowAllowMinDifficultyBlocks)
//        {
//            // Special difficulty rule for testnet:
//            // If the new block's timestamp is more than 2* 10 minutes
//            // then allow mining of a min-difficulty block.
//            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
//                return nProofOfWorkLimit;
//            else
//            {
//                // Return the last non-special-min-difficulty-rules-block
//                const CBlockIndex* pindex = pindexLast;
//                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
//                    pindex = pindex->pprev;
//                return pindex->nBits;
//            }
//        }
//        return pindexLast->nBits;
//    }
    

    return CalculateNextWorkRequired(bnAvg, pindexLast->GetMedianTimePast(), pindexFirst->GetMedianTimePast(), params);
}

unsigned int CalculateNextWorkRequired(arith_uint256 bnAvg, int64_t nLastBlockTime, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    
    // Limit adjustment
    int64_t nActualTimespan = nLastBlockTime - nFirstBlockTime;
    nActualTimespan = params.AveragingWindowTimespan() + (nActualTimespan - params.AveragingWindowTimespan())/4;
    
    if (nActualTimespan < params.MinActualTimespan())
        nActualTimespan = params.MinActualTimespan();
    if (nActualTimespan > params.MaxActualTimespan())
        nActualTimespan = params.MaxActualTimespan();

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew {bnAvg};
    bnNew /= params.AveragingWindowTimespan();
    bnNew *= nActualTimespan;
    
    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

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
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
