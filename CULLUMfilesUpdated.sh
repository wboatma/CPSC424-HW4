#!/bin/bash

# Cullum Smith (gcsmith@clemson.edu)
# CPSC 624 - Spring 2015 - Dr. Martin
# Homework 2 - filesUpdated.sh

usage () {
   echo "usage: filesUpdated.sh [directory] [daysAgo] [mode]"
}

INTEGER_REGEXP='^[1-9][0-9]*$'
MODE_REGEXP='^(1|2)$'


# ensure 3 args were passed
if (( $# != 3 )) ; then
   echo "[ERROR] missing operand"
   usage
   exit -1
fi

directory="$1"
daysAgo="$2"
mode="$3"

# ensure daysAgo is a positive integer
if ! [[ "$daysAgo" =~ $INTEGER_REGEXP ]] ; then
   echo "[ERROR] daysAgo must be a positive integer"
   usage
   exit -1
fi

# ensure mode is either 1 or 2
if ! [[ "$mode" =~ $MODE_REGEXP ]] ; then
   echo "[ERROR] mode must be either 1 or 2"
   usage
   exit -1
fi


# associative array mapping usernames to file counts
declare -A fileCount

# get file count in $directory for each system user
while read -r user ; do
   count=$(find "$directory" -type f -mtime "-$daysAgo" -user "$user" -print0 | tr -d -c '\0' | wc -c)
   if (( "$count" > 0 )) ; then
      fileCount["$user"]="$count"
   fi
done <<< "$(awk -F':' '{ print $1 }' /etc/passwd)"


case $mode in
   1) # summary mode
      printf "Owner      Files changed in past %d days\n" "$daysAgo"
      for user in "${!fileCount[@]}" ; do
         printf "%-10s %d\n" "$user" "${fileCount["$user"]}"
      done | sort -rn -k2
      ;;

   2) # verbose mode
      while read -r user ; do
         echo $user
         find "$directory" -type f -mtime "-$daysAgo" -user "$user" -exec ls -lt {} \;
         echo
      done <<< "$(for user in "${!fileCount[@]}" ; do echo "$user" "${fileCount["$user"]}" ; done | sort -rn -k2 | awk '{ print $1 }')"
      ;;
esac
