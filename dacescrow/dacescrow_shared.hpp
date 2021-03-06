#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/time.hpp>
#include <optional>

using namespace eosio;
using namespace std;

struct [[eosio::table("escrows"), eosio::contract("dacescrow")]] escrow_info {
    uint64_t        key;
    name            sender;
    name            receiver;
    name            arb;
    extended_asset  ext_asset;
    string          memo;
    time_point_sec  expires;
    uint16_t        arb_payment;

    uint64_t        primary_key() const { return key; }

    uint64_t        by_sender() const { return sender.value; }
};

typedef multi_index<"escrows"_n, escrow_info,
        indexed_by<"bysender"_n, const_mem_fun<escrow_info, uint64_t, &escrow_info::by_sender> >
> escrows_table;
