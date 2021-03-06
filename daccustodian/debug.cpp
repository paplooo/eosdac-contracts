
void daccustodian::resetvotes(name voter, name dac_id){
    require_auth(get_self());

    votes_table votes_cast_by_members(_self, dac_id.value);
    auto existingVote = votes_cast_by_members.find(voter.value);

    check(existingVote != votes_cast_by_members.end(), "No votes");

    votes_cast_by_members.erase(existingVote);
}

void daccustodian::resetcands(name dac_id){
    require_auth(get_self());

    candidates_table candidates(_self, dac_id.value);
    auto cand = candidates.begin();

    while (cand != candidates.end()){
        candidates.modify(cand, same_payer, [&](candidate& c){
            c.total_votes = 0;
        });

        cand++;
    }
}
