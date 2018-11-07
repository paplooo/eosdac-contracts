#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

using namespace eosio;
using namespace std;


class [[eosio::contract("dacmultisigs")]] dacmultisigs : public contract {

    private:

        struct [[eosio::table]] storedproposal {
            name proposalname;
            capi_checksum256 transactionid;
            uint32_t modifieddate;

            uint64_t primary_key() const { return proposalname.value; }

            EOSLIB_SERIALIZE(
                storedproposal,
                    (proposalname)
                    (transactionid)
                    (modifieddate)
            )
        };
        typedef multi_index<"proposals"_n, storedproposal> proposals_table;

    public:

        using contract::contract;

        [[eosio::action]]
        void stproposal(name proposer, name proposalname, string metadata);

        [[eosio::action]]
        void approved( name proposer, name proposal_name, permission_level level );

        [[eosio::action]]
        void unapproved( name proposer, name proposal_name, permission_level level );

        [[eosio::action]]
        void cancelled( name proposer, name proposal_name, name canceler );

        [[eosio::action]]
        void executed( name proposer, name proposal_name, name executer );

        [[eosio::action]]
        void clean( name proposer, name proposal_name );
};