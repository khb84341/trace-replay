#rm ../simpleReplay
echo 67584 > /proc/sys/vm/min_free_kbytes
rm ../result/readtime 
rm ../traceReplay
cp run_replay.sh ../run_replay.sh
#g++ simpleReplay.cpp -g -o ../simpleReplay --static -lm
g++ -std=c++11 traceReplay.cpp cJSON.cpp traceConfig.cpp simpleReplay.cpp cacheReplay.cpp dbReplay.cpp -g -o ../traceReplay --static -lm
echo 7650000 > /proc/sys/vm/min_free_kbytes
