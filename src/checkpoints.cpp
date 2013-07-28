// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
	// before block 14640, the retarget period in I0coin was 1 week
	// the function ComputeMinWork uses the current retarget period
	// of 3 hours, this is no problem UNLESS someone adds a checkpoint before
	// block 14640, should be no problem....
	(  36180, uint256("0x0000000000635e5e1a8027383f028f4c666f9e20f4f90968ba8bf7ba8431c71f"))
	( 127360, uint256("0x000000000330be69aa359cb69896554c0dfcd9d76b5415d526708ed737bfe0b6"))
	( 131130, uint256("0x0000000000853272e70ba9aafe9f685c186a7ba3aa57d2ddba7c44c6a25efe09"))
	( 136800, uint256("0x0000000000c8c592fce349ed8cf7eba3113f3c243c9e1cbe27fb6166cc4ffa00"))
	( 142900, uint256("0x00000000005eb49db6f29a6aae382b7a8e9a109aba42e536e6d74b95aba4dffd"))
	( 155000, uint256("0x0000000000041a6bc4cd419ed90a6bb1dbf8df8a587d162504dba9ae84a4418c"))
	( 161000, uint256("0x601581f84984f86f5c4d080b2e32bd1c4da4061730fd9bc6b4ce08c65b30c4bd"))
	( 367000, uint256("0xb619876887c0baac0aca8cef5eea23869bce693b8629fb6b62d8b529cd216586"))
	( 837000, uint256("0x421c7a8246ed2759191beff61c46897c0787779cfd174ba53b01f6e5b5ab6ff1"))
	;

    /* i0coin had not testnet checkblocks, if they even start to exist, they can be added here */
    static MapCheckpoints mapCheckpointsTestnet; // = 
        //boost::assign::map_list_of
	// ( insert testnet checkpoints here, just like above )
        //;

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
