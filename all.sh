#!/bin/bash

rm warc.paths.gz
wget https://commoncrawl.s3.amazonaws.com/crawl-data/CC-MAIN-2021-10/warc.paths.gz
gunzip -f warc.paths.gz

numProc=100
procs=( )

id=1

for key in `awk 'NR >= 1001 && NR <= 100000' warc.paths`; do

	echo "Starting $key"

	#aws lambda invoke --function-name cc-parser --invocation-type Event --payload '{"s3bucket":"commoncrawl", "s3key": "'"$key"'"}' response.txt &
	echo '{"s3bucket":"alexandria-cc-output", "s3key": "'"$key"'", "id": "'"$id"'"}'
	aws lambda invoke --function-name cc-indexer --invocation-type Event --payload '{"s3bucket":"alexandria-cc-output", "s3key": "'"$key"'", "id": '"$id"'}' response.txt &

	procs+=( $! )
	id=$((id+1))

	if [ ${#procs[@]} -eq $numProc ] ; then
		for proc in "${procs[@]}"; do
			echo "Waiting for"
			echo $proc
			wait $proc
		done
		procs=( )
	fi


done

for proc in "${procs[@]}"; do
	echo "Waiting for"
	echo $proc
	wait $proc
done
