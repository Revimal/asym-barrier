if [ -d ./autotest ]
then
	rm -rf ./autotest
fi

mkdir autotest && cd autotest

echo "[INFO] Preparing test cases."
gcc -o test_barrier ../test_barrier.c -lpthread
if [ $? -ne 0 ]
then
	echo "[ERROR] Test case preparation failed." >&2
	exit 1
fi

ASYMB_CPU_COUNT=$(grep -c ^processor /proc/cpuinfo)
echo "[INFO] THREAD = $ASYMB_CPU_COUNT"
echo "[INFO] BINARY = $(ls -al | grep test_barrier | awk -F ' ' '{print $1}')"

if [ $ASYMB_CPU_COUNT -lt 4 ]
then
	echo "[INFO] Escaped."
	exit 0
fi

echo "[INFO] Running test cases."
ASYMB_TC_RESULTS=($(./test_barrier | grep -v "RESULT" | grep -v "UNSAFE" | awk -F ' ' '{print $2}' | sed 's/\///'))
if [ $? -ne 0 ]
then
	echo "[ERROR] Test case execution failed." >&2
	exit 1
fi

echo "[INFO] RESULTS = ${ASYMB_TC_RESULTS[@]}"

for ASYMB_TC_RESULT in ${ASYMB_TC_RESULTS[@]}
do
	echo "[INFO] VALUE = ${ASYMB_TC_RESULT}"

	if [ ${ASYMB_TC_RESULT} -ne 0 ]
	then
		echo "[ERROR] Test case validation failed." >&2
		exit 1
	fi
done

echo "[INFO] Done."
