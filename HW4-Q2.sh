#! /bin/bash

## One way add a new cron job is to create a temporary file to backup
##  your current cron, append the new cron job, and set the crontab to 
##  the temp file. Optionally remove the temp file as well.
## > crontab -l > /tmp/cron.tmp
## > echo "0 * * * bash ~/HW4-Q2.sh" >> /tmp/cron.tmp   (only if script in ~/)
## > crontab /tmp/cron.tmp
## > rm -f /tmp/cron.tmp             (Optional)

file="/var/log/auth.log";
cur_date=$(date '+%s');  ## date in EPOCH form
log_date=0; 
date_expr="[JFMASOND]{1}[a-z]{2}[ ]+[1-9]{1}[0-9]*[ ]+[0-9]{2}:[0-9]{2}:[0-9]{2}";

su_succ=0; 
su_fail=0;
auth_succ=0;
auth_fail=0;

su_expr="";
auth_expr="";
assess="OK";

while read line; 
do
  date_string=$(echo $line |  grep -a -o -E "$date_expr");
  log_date=$(date -d "$date_string" '+%s');
  if [ $(( (log_date - cur_date)/(60*60*24) )) -ge 0 ]; then
    su_expr=$(echo $line | grep -a -E "(sudo: pam_unix\(sudo:|su\[[0-9]+\]: (Succ|FAIL))");

    auth_expr=$(echo $line | grep -a -E "sshd\[[0-9]+\]:[ ]+(Accepted|Failed)");
    if [ -n "$su_expr" ]; then
      if [ -n "$(echo $su_expr | grep -E "(authentication failure|FAILED)")" ]; then
       su_fail=$((su_fail + 1));
      elif [ -n "$(echo $su_expr | grep -E "(session opened|Successful)")" ]; then
        su_succ=$((su_succ + 1));
      fi
    elif [ -n "$auth_expr" ]; then
      if [ -n "$(echo $auth_expr | grep "Failed password")" ]; then
        auth_fail=$((auth_fail + 1));
      elif [ -n "$(echo $auth_expr | grep "Accepted password")" ]; then
        auth_succ=$((auth_succ + 1));
      fi 
    fi 
  fi
done < $file

## If there are more than 10 sudo or auth failures, set STRANGE
## If there are more than 20 sudo or auth failures, set UNDER_ATTACK
##  These settings are more designed for a home network that does not
##  recieve many outbound connections. For a network like Clemson, 
##  these values would be much higher
cur_date=$(date);
if [ $su_fail -ge 10 ] || [ $auth_fail -ge 10 ]; then
  assess="STRANGE";
elif [ $su_fail -ge 20 ] || [ $auth_fail -ge 20 ]; then
  assess="UNDER_ATTACK";
fi

echo "$cur_date: $assess, su:$su_succ,$su_fail, auth:$auth_succ,$auth_fail";
