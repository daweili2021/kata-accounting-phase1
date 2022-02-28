#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio.token/eosio.token.hpp>
#include <algorithm>
#include <eosio/system.hpp>

class [[eosio::contract("accounting")]] accounting : eosio::contract {
public:
    using eosio::contract::contract;

    accounting(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds) : 
        contract(receiver, code, ds)
    {
        zero = eosio::asset(0, eosio::symbol("SYS", 4));
        require_auth(_self);
        category_table ct{get_self(), 0};
        // add in the default category if not exist
        if ( ct.find( eosio::name("default").value ) == ct.end() ) {
            ct.emplace(get_self(), [&](auto & item) {
                item.category_name = eosio::name("default");
                item.id = "default"_n.value;
                item.balance = eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() );
            });
        }
    }


    // add a new category, exit if already exit
    [[eosio::action]]
    void addcategory(eosio::name new_category) {
        require_auth(get_self());
        category_table ct{get_self(), 0};
        auto itr = ct.find( new_category.value );
        eosio::check( itr == ct.end(), "Category has already been created." );
        ct.emplace(get_self(), [&](auto & item) {
            item.category_name = new_category;
            item.id = new_category.value;
            item.balance = zero;
        });
    }

    // list existed categories and print out the total number of tokens
    [[eosio::action]]
    void listcategory() {
        require_auth( get_self() );
        category_table ct{get_self(), 0};
        eosio::print( "ID   ", "  Category  ", "  Balance(SYS)  ", "\n" );
        for (auto& item : ct) {
            eosio::print_f( "%      %    %\n", item.id, item.category_name, item.balance );
        }
        eosio::print( "Totol token should be ", eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() ), ".\n", "If they don't add up, you might have a scheduled transfer.\n" );
    }
    
    // internal transfer from one category to another
    [[eosio::action]]
    void internaltran(eosio::name acct_from, eosio::name acct_to, eosio::asset amount, std::string memo) {
        require_auth( get_self() );
        eosio::check( amount > zero, "Transfer amount should not be a negative number or zero." );
        category_table ct{get_self(), 0};
        auto itr_from = ct.find( acct_from.value );
        eosio::check( itr_from != ct.end(), std::string( "Category " ) + acct_from.to_string() + std::string( " does not exist. Execute listcategory action to get a complete list of categories." ) );
        eosio::check( itr_from->balance >= amount, std::string( "This action will cause overdraft in the category of " ) + acct_from.to_string() + std::string( " , which has asset of " ) + itr_from->balance.to_string() );

        auto itr_to = ct.find(acct_to.value);
        eosio::check( itr_to != ct.end(), std::string( "Category " ) + acct_to.to_string() + std::string( " does not exist. Execute listcategory action to get a complete list of categories.") );
        
        ct.modify( itr_from, get_self(), [&]( auto& ct ) {
            ct.balance -= amount;
        });
        
        ct.modify( itr_to, get_self(), [&]( auto& ct ) {
            ct.balance += amount;
        });
    }

    // external transfers, eosio <-> account and between different accounts 
    [[eosio::on_notify("eosio.token::transfer")]]
    void transfer(eosio::name from, eosio::name to, eosio::asset balance, std::string memo) {
        
        require_auth( _self );
        category_table ct{get_self(), 0};
        auto itr = ct.find( eosio::name("default").value );
        
        if (from == get_self()) {
            eosio::check( itr->balance >= balance, std::string( "Overdraft, your default category only has " ) + itr->balance.to_string() );
        }

        ct.modify(itr, get_self(), [&]( auto& item ) {
            item.balance = ( from == get_self() ? item.balance - balance : item.balance + balance );
        });
        
    }

    // remove all catetories to make retest easier
    [[eosio::action]]
    void reset() {
        require_auth( get_self() );
        category_table ct{get_self(), 0};
        auto itr = ct.begin();
        while ( itr != ct.end() )
            itr = ct.erase(itr);
        uint64_t cnt = 0;
        for (auto category: ct)
            cnt++;
        //eosio::print("current size of the category table is ", cnt, "\n");
        eosio::print_f("Resetting categories is finished. The number of token in the default category is % \n", eosio::token::get_balance("eosio.token"_n, get_self(), zero.symbol.code()));
    }

    // work around of the get_row_by_account error
    [[eosio::action]]
    void checkbalance(eosio::name category, eosio::asset amount) {    
        require_auth( _self );
        category_table ct{get_self(), 0};
        auto itr = ct.find( category.value );
        eosio::check( itr != ct.end(), std::string("category ") + category.to_string() + std::string(" does not exist \n") );
        eosio::check( itr->balance == amount, std::string("The balance of category ") + category.to_string() + std::string(" is ") + itr->balance.to_string() + std::string(", which does not match the given amount - ") + amount.to_string() + std::string("\n") );
    }

    // make a single transfer from a category to eosio: move fund to the default, then use action send to execute the transfer on behalf of eosio.token
    [[eosio::action]]
    void singlepay(eosio::name from, eosio::name to, eosio::asset balance, std::string memo) {     
        require_auth( _self );
        category_table ct{get_self(), 0};
        auto itr = ct.find( from.value );
        eosio::check( itr != ct.end(), std::string("category ") + from.to_string() + std::string(" does not exist \n") );
        eosio::check( itr->balance >= balance, std::string("Overdraft, your ") + from.to_string() + std::string(" category only has " ) + itr->balance.to_string() );
        internaltran(from, "default"_n, balance, memo);
        eosio::action{
            eosio::permission_level{get_self(), "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(get_self(), "eosio"_n, balance, memo)
        }.send();
    }

    // set up future transfer: reserve fund by transferring to the default category, then add an entry in the hold table
    [[eosio::action]]
    void setfuturetra(eosio::name category, eosio::asset amount, uint32_t time) {
        require_auth( get_self() );
        category_table ct{ get_self(), 0 };
        auto itr = ct.find( category.value );
        eosio::check( itr != ct.end(), std::string("Category ") + category.to_string() + std::string(" does not exist.\n") );
        eosio::check( itr->balance >= amount, "This future transfer amount exceeds the category's balance.\n" );
        eosio::check( time > current_time(), "The given time is not a future time\n" );
        internaltran(category, "default"_n, amount, std::string("reserved for a future transfer"));
        hold_table ht{ get_self(), 0 };
        auto it_ht = ht.find( category.value );
        eosio::check( it_ht == ht.end(), "A future transfer of this category has already been set up.\n" );
        ht.emplace( get_self(), [&](auto& item) {
            item.category_name = category;
            item.id = category.value;
            item.amount = amount;
            item.due_time = time;
            item.recurring = 0;
            item.time_interval = 0;
        });
    }

    // execute a future transfer: use action send to execute the transfer on behalf of eosio.token
    [[eosio::action]]
    void dofuturetran(eosio::name category) {
        require_auth( get_self() );
        hold_table ht{ get_self(), 0 };
        auto itr = ht.find( category.value );
        eosio::check( itr != ht.end(), "Provided category does not exist or does not have a scheduled future transfer.\n" );
        eosio::print("current time is ");
        eosio::print( current_time() );
        eosio::print("\nscheduled time is ");
        eosio::print(itr->due_time);
        eosio::check( current_time() >= itr->due_time, "Still early to make the scheduled transfer happen.\n" );
        eosio::action{
            eosio::permission_level{get_self(), "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(get_self(), "eosio"_n, itr->amount, std::string("future transfer complete, check your balance for confirmation."))
        }.send();
        ht.erase(itr);
    }

    // cancel a future transfer: move reserved fund back from the default category, then clean up
    [[eosio::action]]
    void cancelfuture(eosio::name category) {
        require_auth( get_self() );
        hold_table ht{ get_self(), 0 };
        auto itr = ht.find( category.value );
        eosio::check( itr != ht.end(), "Provided category does not exist or does not have a scheduled future transfer.\n" );
        eosio::check( current_time() < itr->due_time, "Current time is newer than the due time of the future transfer. Unable to cancel it, you can only execute it.\n" );
        internaltran("default"_n, category, itr->amount, std::string("release from a cancelled future transfer"));
        ht.erase(itr);
    }

    // set up recurring transfer action: checks before adding it
    [[eosio::action]]
    void setrecurring(eosio::name category, eosio::asset amount, uint32_t time, uint32_t interval) {
        require_auth( get_self() );
        category_table ct{ get_self(), 0 };
        auto it_ct = ct.find( category.value );
        eosio::check( it_ct != ct.end(), std::string("Category ") + category.to_string() + std::string(" does not exist.\n") );
        eosio::check( it_ct->balance >= amount, "This future transfer amount exceeds the category's balance.\n" );
        eosio::check( time > current_time(), "The given time is not a future time\n" );

        hold_table ht{ get_self(), 0 };
        auto it_ht = ht.find( category.value );
        if ( it_ht == ht.end() ) {
            ht.emplace( get_self(), [&](auto& item) {
                item.category_name = category;
                item.id = category.value;
                item.amount = amount;
                item.due_time = time;
                item.recurring = 1;
                item.time_interval = interval;
            });
        } else {
            eosio::check( 1 != 1, std::string("Category ") + category.to_string() + std::string(" already has a recurring transfer set up.\n") );
        }
    }
    // recurring transfer action: execute the transfer as the sleep is handled in the helper.sh script
    [[eosio::action]]
    void recurringtra(eosio::name category) {
        require_auth( get_self() );
        hold_table ht{ get_self(), 0 };
        auto it_ht = ht.find( category.value );
        eosio::check( it_ht != ht.end(), std::string("Category ") + category.to_string() + std::string(" does not exist in the recurring list table. Or the recurring transfer has been cancelled.\n") );

        internaltran(category, "default"_n, it_ht->amount, "For recurring transfer\n");
        eosio::action{
            eosio::permission_level{get_self(), "active"_n},
            "eosio.token"_n,
            "transfer"_n,
            std::make_tuple(get_self(), "eosio"_n, it_ht->amount, std::string("One recurring transfer complete, check your balance for confirmation."))
        }.send();
    }

    // generic transfer action
    // 1. execute all matured future transfers, then clean up
    // 2. exeucute only one recurring transfer, then modify the due_time to the next scheduled time based on recurring transfer
    [[eosio::action]]
    void generictran() {
        require_auth( get_self() );
        hold_table ht{ get_self(), 0 };
        auto it_ht = ht.begin();
        while ( it_ht != ht.end() ) {
            if ( it_ht->recurring ) {
                recurringtra( it_ht->category_name );
                ht.modify(it_ht, get_self(), [&]( auto& item ) {
                    item.due_time += item.time_interval;
                });
                it_ht++; // advance the iterator to the next entry
            } else {
                eosio::action{
                    eosio::permission_level{get_self(), "active"_n},
                    "eosio.token"_n,
                    "transfer"_n,
                    std::make_tuple(get_self(), "eosio"_n, it_ht->amount, std::string("future transfer complete, check your balance for confirmation."))
                }.send();
                it_ht = ht.erase(it_ht);
            }    
        }
    }

    // cancel a recurring transfer
    [[eosio::action]]
    void cancelrecur(eosio::name category) {
        require_auth( get_self() );
        hold_table ht{ get_self(), 0 };
        auto itr = ht.find( category.value );
        eosio::check( itr != ht.end(), std::string("Category ") + category.to_string() + std::string(" does not exist.\n") );
        ht.erase( itr );
    }

    // list future transfers
    [[eosio::action]]
    void listfuture() {
        require_auth( get_self() );
        hold_table ht{get_self(), 0};
        eosio::print( "       ID       ", "     Category   ", " Amount  ", "    Time    ", " Recurring", " Interval(s)\n" );
        for (auto it : ht)
                eosio::print_f( "%, %, %, %, %, %\n", it.id, it.category_name, it.amount, it.due_time, it.recurring, it.time_interval );
    }

private:

    struct [[eosio::table]] category {
        eosio::name category_name;
        uint64_t id;
        eosio::asset balance;
        
        uint64_t primary_key() const { return category_name.value; }
    };

    // note that only one future transfer is allowed due to uniqueness of the primary_key.
    // if multiple future transfers are needed, use map<eosio::name, transfer_info> instead.
    struct [[eosio::table]] hold {
        eosio::name category_name;
        uint64_t id;
        uint32_t due_time;
        uint32_t recurring;
        uint32_t time_interval;
        eosio::asset amount;
        uint64_t primary_key() const { return category_name.value; }
    };

    uint32_t current_time() {
        return eosio::current_time_point().sec_since_epoch();
    }

    using category_table = eosio::multi_index<"categories"_n, category>;
    using hold_table = eosio::multi_index<"hold"_n, hold>;
    eosio::asset zero;
};
