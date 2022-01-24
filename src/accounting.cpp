#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio.token/eosio.token.hpp>


class [[eosio::contract("accounting")]] accounting : eosio::contract {
public:
    using eosio::contract::contract;

    accounting(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds) : 
		contract(receiver, code, ds)
    {
        zero = eosio::asset(0, eosio::symbol("SYS", 4));
    }

    // add a new category, exit if already exit
    [[eosio::action]]
    void addcategory(eosio::name new_category) {
        require_auth(get_self());
        category_table ct{get_self(), 0};
        auto ct_names = ct.get_index<"getname"_n>();

        // before the first new category gets created, add in the default one
        auto itr = ct_names.find(eosio::name( "default" ).value);       
        if (itr == ct_names.end()) {
            ct.emplace(get_self(), [&](auto & item) {
                item.id = ct.available_primary_key();
                item.category_name = eosio::name( "default" );
                item.balance = eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() );
            });  
        }

        itr = ct_names.find( new_category.value );
        eosio::check( itr == ct_names.end(), "Category has already been created." );
        
        ct.emplace(get_self(), [&](auto & item) {
            item.id = ct.available_primary_key();
            item.category_name = new_category;
            item.balance = zero;
        });
    }

    // list existed categories and print out the total number of tokens
    [[eosio::action]]
    void listcategory() {
        require_auth( get_self() );

        category_table ct{get_self(), 0};
        auto ct_names = ct.get_index<"getname"_n>();
        auto itr = ct_names.find(eosio::name( "default" ).value);
        // restore if the default category doesn't exist
        if (itr == ct_names.end()) {
            ct.emplace(get_self(), [&](auto & item) {
                item.id = ct.available_primary_key();
                item.category_name = eosio::name( "default" );
                item.balance = eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() );
            });
        }
        eosio::print( "ID   ", "  Category  ", "  Balance(SYS)  ", "\n" );
        for (auto item : ct) {
            eosio::print_f( "%      %    %\n", item.id, item.category_name, item.balance );
        }
        eosio::print( "Totol token should be ", eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() ) );
    }
    
    // internal transfer from one category to another
    [[eosio::action]]
    void internaltran(eosio::name acct_from, eosio::name acct_to, eosio::asset amount, std::string memo) {
        require_auth( get_self() );
        eosio::check( amount > zero, "Transfer amount should not be a negative number or zero." );

        category_table ct{get_self(), 0};
        auto ct_names = ct.get_index<"getname"_n>();
        auto itr_from = ct_names.find( acct_from.value );
        eosio::check( itr_from != ct_names.end(), std::string( "Category " ) + acct_from.to_string() + std::string( " does not exist. Execute listcategory action to get a complete list of categories." ) );
        eosio::check( itr_from->balance >= amount, std::string( "This action will cause overdraft in the category of " ) + acct_from.to_string() + std::string( " , which has asset of " ) + itr_from->balance.to_string() );

        auto itr_to = ct_names.find(acct_to.value);
        eosio::check( itr_to != ct_names.end(), std::string( "Category " ) + acct_to.to_string() + std::string( " does not exist. Execute listcategory action to get a complete list of categories.") );
        
        ct_names.modify( itr_from, get_self(), [&]( auto& ct ) {
            ct.balance -= amount;
        });
        
        ct_names.modify( itr_to, get_self(), [&]( auto& ct ) {
            ct.balance += amount;
        });
    }
    // external transfers, eosio <-> account and between different accounts
    [[eosio::on_notify("eosio.token::transfer")]]
    void transfer(eosio::name from, eosio::name to, eosio::asset balance, std::string memo) {
        
        require_auth( _self );
        category_table ct{get_self(), 0};
        auto ct_table = ct.get_index<"getname"_n>();
        auto itr = ct_table.find( eosio::name("default").value );
        
        if (itr == ct_table.end()) {
            ct.emplace(get_self(), [&](auto & item) {
                item.id = ct.available_primary_key();
                item.category_name = eosio::name( "default" );
                item.balance = eosio::token::get_balance( "eosio.token"_n, get_self(), zero.symbol.code() );
            });
            return;
        }
        
        if (from == get_self()) {
            eosio::check( itr->balance >= balance, std::string( "Overdraft, your default category only has " ) + itr->balance.to_string() );
        }

        ct_table.modify(itr, get_self(), [&]( auto& item ) {
            item.balance = ( from == get_self() ? item.balance - balance : item.balance + balance );
        });
        
    }

    // remove all catetories to make retest easier
    [[eosio::action]]
    void reset() {
        require_auth(_self);
        category_table ct{get_self(), 0};
        auto itr = ct.begin();
        while ( itr != ct.end() )
            itr = ct.erase(itr);        
        eosio::print_f("Resetting categories is finished. The number of token in the default category is % \n", eosio::token::get_balance("eosio.token"_n, get_self(), zero.symbol.code()));
    }

private:

    struct [[eosio::table]] category {
        uint64_t id;
        eosio::name category_name;
        eosio::asset balance;
        
        uint64_t primary_key() const { return id; }
        uint64_t getname() const { return category_name.value; }
    };

    using category_table = eosio::multi_index<"category"_n, category, eosio::indexed_by<"getname"_n, eosio::const_mem_fun<category, uint64_t, &category::getname>>>;

    eosio::asset zero;
};
