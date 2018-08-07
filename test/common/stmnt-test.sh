FAILED=0
for i in statements/good/*; do
    echo $i |grep ".conf" >/dev/null;
    if test $? = 1; then
        if ! ./stmnt-test $i 2>/dev/null >/dev/null; then
            echo "$i failed";
            let FAILED=$FAILED+1;
        fi
    fi
done

if test $FAILED = 0; then
    echo "$0 OK"
else
    echo "Test failed"
fi

VALGRIND=/usr/bin/valgrind
INVALID_RUNS=0

if ! test -x $VALGRIND; then
    echo "Skipping valgrind tests, $VALGRIND is not available"
    exit $FAILED
fi

echo "=== Executing stmnt-test under Valgrind ==="

for i in statements/good/*; do
    echo $i |grep ".conf" >/dev/null;
    if test $? = 1; then
        FILENAME=$(basename $i)
        VG_OUT="${FILENAME}.vgout"
        $VALGRIND --error-exitcode=2 \
                  --tool=memcheck \
                  --leak-check=full \
                  --suppressions=stmnt-test.supp ./stmnt-test $i \
                  2> $VG_OUT > /dev/null
        VG_EXIT_STATUS=$?
        if test $VG_EXIT_STATUS = 2; then
            # If a leak is detected print the error message and exit script
            # immediately
            echo "$i";
            echo "Valgrind detected a memory error(s) in $i, see ${VG_OUT} for details"
            exit 1;
        elif test $VG_EXIT_STATUS != 0; then
            # If there is some other error print which test failed and
            # continue, output is in the file VG_OUT
            echo "$i failed";
            let INVALID_RUNS=$INVALID_RUNS+1
        elif [ -f $VG_OUT ]; then
            # If everything is ok delete the file VG_OUT
            rm $VG_OUT
        fi
    fi
done

if test $INVALID_RUNS = 0; then
    echo "$0 Valgrind check OK"
else
    echo "Valgrind check failed"
fi

let FAILED=$FAILED+$INVALID_RUNS

exit $FAILED
