# kata-accounting-phase1 build, verify and test procedure

Prerequisites:
  1. can build eosio, eosio.cdt and eosio.contracts from their respective source code
  2. nodeos is actively running on the background
  3. wallets for default and alice, accounts for eosio.token and alice have been created
  4. valid environment variables EOSIO_ROOT, EOSIO_CDT_ROOT and EOSIO_CONTRACTS_ROOT have been created in the ~/.bash_profile
  
Steps:
  1. Deploy the system contracts first, for example:
    
    cleos set contract eosio.token $HOME/eosio.contracts/build/contracts/eosio.token --abi eosio.token.abi -p eosio.token@active

  2. Create, issue and transfer eosio token SYS
    
    cleos push action eosio.token create '["eosio", "1000000.0000 SYS"]' -p eosio.token
    cleos push action eosio.token issue '[eosio, "10000.0000 SYS", "issuing gifts"]' -p eosio
    cleos push action eosio.token transfer '[ "eosio", "alice", "1000.0000 SYS", "Gift for birthday" ]' -p eosio

  3. Build and deploy this accounting contract. Make sure wallets are in the unlocked state. Then run the following commands
    
    mkdir build && cd build
    cmake ..
    make
    cd kata_accounting
    cleos set contract alice $(pwd) --abi accounting.abi -p alice

  4. Test actions created in the source file  
    
    # add categories
    cleos push action alice addcategory '["checking"]' -p alice@active
    cleos push action alice addcategory '["saving"]' -p alice@active
    cleos push action alice addcategory '["retirement"]' -p alice@active 
    # list categories
    cleos -v push action alice listcategory '[""]' -p alice@active 
    # internal transfers
    cleos -v push action alice internaltran '["default", "saving", "1000.0000 SYS", "for saving"]' -p alice@active
    cleos -v push action alice internaltran '["default", "checking", "100.0000 SYS", "for checking"]' -p alice@active
    cleos -v push action alice internaltran '["default", "retirement", "500.0000 SYS", "for retirement"]' -p alice@active
    cleos -v push action alice internaltran '["default", "saving", "10.0009 SYS", "for saving"]' -p alice@active
    # external transfers
    cleos push action eosio.token transfer '["eosio", "alice", "1000.0000 SYS", "second gift"]' -p eosio@active alice@active
    cleos push action eosio.token transfer '["alice", "eosio", "500.0000 SYS", "returning back half"]' -p alice@active
    # reset categories
    cleos -v push action alice reset '[""]' -p alice@active
    # remove a contract
    cleos set contract -c alice . accounting.abi

  5. Run automated tests.
    
    cd ../tests/
    ./accounting_tests
    verify that "*** No errors detected" is displayed at the end
    
  
