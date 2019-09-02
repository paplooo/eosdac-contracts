#include <distribution.hpp>

void distribution::regdistri(name distri_id, name dac_id, name owner, name approver_account, extended_asset total_amount, uint8_t distri_type, string memo){
  
    require_auth(owner);
    check(distri_type < INVALID, "Distribution type out of bound. Use 0 = claim; 1 = send tokens");
    check(memo.length() <= 256, "Memo can't be longer then 256 chars.");

    districonf_table districonf_t(get_self(), get_self().value);
    auto existing_distri = districonf_t.find(distri_id.value);

    check(existing_distri == districonf_t.end(), "Distribution config with this id already exists.");
    check(total_amount.quantity.amount > 0, "Total distribution amount must be greater then zero.");

    //rampayer is owner
    districonf_t.emplace(owner, [&](auto& n) {
        n.distri_id = distri_id;
        n.dac_id = dac_id;
        n.owner = owner;
        n.total_amount = total_amount;
        n.approved = 0;
        n.approver_account = approver_account;
        n.distri_type = distri_type;
        n.total_sent = asset( 0, total_amount.quantity.symbol );
        n.memo = memo;
    });
}

void distribution::deldistrconf(name distri_id){

    districonf_table districonf_t(get_self(), get_self().value);
    auto districonf = districonf_t.find(distri_id.value);
    check(districonf != districonf_t.end(), "Distribution config with this id doesn't exists.");
    require_auth(districonf->owner);

    distri_table distri_t(get_self(), distri_id.value);
    check(distri_t.begin() == distri_t.end(), "Can't delete config while the distribution list isn't empty. Empty the distribution list before calling this action.");

    districonf_t.erase(districonf);
}

void distribution::approve(name distri_id){

    districonf_table districonf_t(get_self(), get_self().value);
    auto districonf = districonf_t.find(distri_id.value);
    check(districonf != districonf_t.end(), "Distribution config with this id doesn't exists.");
    require_auth(districonf->approver_account);

    distri_table distri_t(get_self(), distri_id.value);
    check(distri_t.begin() != distri_t.end(), "Can't approve an empty distribution list.");

    check(districonf->approved == 0, "Distribution is already approved.");

    districonf_t.modify(districonf, same_payer, [&](auto& n) {
        n.approved = 1;
    });
  
}

void distribution::populate(name distri_id, vector<dropdata> data, bool allow_modify){

    districonf_table districonf_t(get_self(), get_self().value);
    auto existing_distri = districonf_t.find(distri_id.value);

    check(existing_distri != districonf_t.end(), "Distribution config with this id doesn't exist.");
    check(existing_distri->approved == 0, "Can't populate an already approved distribution list.");

    require_auth(existing_distri->owner);

    distri_table distri_t(get_self(), distri_id.value);


    name rampayer = existing_distri->owner;
  
    for (dropdata dropitem: data) {
        check(dropitem.amount.amount > 0, "Amount must be greater then zero.");
        check(dropitem.amount.symbol == existing_distri->total_amount.quantity.symbol, "Symbol doesn't match the distribution config.");

        auto existing_entry = distri_t.find(dropitem.receiver.value);
        if (existing_entry == distri_t.end()){
            //new entry - always allowed
            distri_t.emplace(rampayer, [&](auto& n) {
                n.receiver = dropitem.receiver;
                n.amount = dropitem.amount;
            });
        }
        else if (allow_modify){
            //existing entry and allowed to modify
            distri_t.modify(existing_entry, same_payer, [&](auto& n) {
                n.amount = dropitem.amount;
            });
        }

    }
}

void distribution::empty(name distri_id, uint8_t batch_size){

    districonf_table districonf_t(get_self(), get_self().value);
    auto existing_distri = districonf_t.find(distri_id.value);
    check(existing_distri != districonf_t.end(), "Distribution config with this id doesn't exists.");
    require_auth(existing_distri->owner);
    check(existing_distri->approved == 0, "Can't clear an already approved distribution.");

    distri_table distri_t(get_self(), distri_id.value);
    check(distri_t.begin() != distri_t.end(), "No more entries, table is already empty.");

    check(batch_size > 0, "Batch size must be greater then zero.");
    uint8_t count = 0;
    for(auto itr = distri_t.begin(); itr != distri_t.end() && count!=batch_size;) {
        itr = distri_t.erase(itr);
        count++;
    }

}

void distribution::sendtokens(name distri_id, uint8_t batch_size){

    districonf_table districonf_t(get_self(), get_self().value);
    auto existing_distri = districonf_t.find(distri_id.value);
    check(existing_distri != districonf_t.end(), "Distribution config with this id doesn't exists.");
    check(existing_distri->approved == 1, "Distribution must be approved first.");
    check(existing_distri->distri_type == SENDABLE, "distri_type must be of type SENDABLE.");

    distri_table distri_t(get_self(), distri_id.value);
    check(distri_t.begin() != distri_t.end(), "Sending tokens completed, no more entries.");

    check(batch_size > 0, "Batch size must be greater then zero.");

    string memo = existing_distri->memo;
    name tokencontract = existing_distri->total_amount.contract;

    uint8_t count = 0;
    for(auto itr = distri_t.begin(); itr != distri_t.end() && count!=batch_size;) {

        action(
            permission_level{get_self(), "active"_n},
            tokencontract, "transfer"_n,
            make_tuple(get_self(), itr->receiver, itr->amount, memo)
            )
            .send();

        districonf_t.modify(existing_distri, same_payer, [&](auto& n) {
            n.total_sent += itr->amount;
        });

        itr = distri_t.erase(itr);
        count++;
    }
}

void distribution::claim(name distri_id, name receiver){
    require_auth(receiver);
    districonf_table districonf_t(get_self(), get_self().value);
    auto existing_distri = districonf_t.find(distri_id.value);
    check(existing_distri != districonf_t.end(), "Distribution config with this id doesn't exists.");
    check(existing_distri->approved == 1, "Distribution must be approved first.");
    check(existing_distri->distri_type == CLAIMABLE, "distri_type must be of type CLAIMABLE.");

    distri_table distri_t(get_self(), distri_id.value);
    auto claim_entry = distri_t.find(receiver.value);
    check(claim_entry != distri_t.end(), "You don't have something to claim.");

    string memo = existing_distri->memo;

    name tokencontract = existing_distri->total_amount.contract;

    action(
            permission_level{get_self(), "active"_n},
            tokencontract, "transfer"_n,
            make_tuple(get_self(), receiver, claim_entry->amount, memo)
    )
            .send();

    districonf_t.modify(existing_distri, same_payer, [&](auto& n) {
        n.total_sent += claim_entry->amount;
    });

    distri_t.erase(claim_entry);
  
}
