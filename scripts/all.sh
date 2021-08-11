#!/bin/bash

cd `dirname $0`
cd ..
mkdir -p tmp

CC_BATCH=CC-MAIN-2021-21
if [ $# -eq 1 ]; then
	CC_BATCH=$1
fi

echo "Running $CC_BATCH";

cd tmp
rm warc.paths.gz
wget https://commoncrawl.s3.amazonaws.com/crawl-data/$CC_BATCH/warc.paths.gz
gunzip -f warc.paths.gz
cd ..

num_proc=100
offset_start=1
offset_end=100000
procs=( )

id=1

for key in `awk "NR >= $offset_start && NR <= $offset_end" tmp/warc.paths`; do

	echo "Starting $key"

	aws lambda invoke --function-name cc-parser --invocation-type Event --payload '{"s3bucket":"commoncrawl", "s3key": "'"$key"'"}' response.txt &
	#echo '{"s3bucket":"alexandria-cc-output", "s3key": "'"$key"'", "id": "'"$id"'"}'
	#aws lambda invoke --function-name cc-indexer --invocation-type Event --payload '{"s3bucket":"alexandria-cc-output", "s3key": "'"$key"'", "id": '"$id"'}' response.txt &

	procs+=( $! )
	id=$((id+1))

	if [ ${#procs[@]} -eq $num_proc ] ; then
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
