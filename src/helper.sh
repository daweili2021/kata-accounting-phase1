#!/usr/bin/env bash

if [ $# -ne 4 ]
then
    echo "usage: ./helper.sh category_name amount first_transfer_time interval"
    exit 1
fi
category=$1
amount=$2
amountarray=($amount)
#echo ${amountarray[0]}
first_transfer_time=$3
#echo $first_transfer_time
interval=$4
current_time=`date +%s`
wait_time=$((first_transfer_time-current_time))

if [ $wait_time < 0 -o $interval -le 0 ]
then
    echo "Check your input arguments, first_transfer_time should be a future time and interval should be a positive number."
    exit 1
fi
sleep $wait_time
counter=0

while [ $counter -le 9 -a $? -eq 0 ]
do
    cleos wallet unlock -n default --password `cat $HOME/Work/passwd/default.wallet.password`
    cleos wallet unlock -n alice --password `cat $HOME/Work/passwd/alice.wallet.password`
    counter=$((counter+1))
    cleos -v push action alice listcategory '[""]' -p alice@active
    cleos -v push action alice recurringtra '["'"$category"'"]' -p alice@active
    first_transfer_time=$(($first_transfer_time+$interval))
    sleep $interval
done
exit 0