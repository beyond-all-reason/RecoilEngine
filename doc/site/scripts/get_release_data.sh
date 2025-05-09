#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
DATA_FILE="$SCRIPT_DIR/../data/latest_release.json"

echo "> writing $DATA_FILE"

rm $DATA_FILE
curl -s https://api.github.com/repos/beyond-all-reason/RecoilEngine/releases/latest  \
  > $DATA_FILE