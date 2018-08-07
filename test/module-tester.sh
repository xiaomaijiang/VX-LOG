#!/bin/sh
ulimit -c unlimited
if test -f tmp/nxlog.pid; then
    echo "stopping nxlog with pid `cat tmp/nxlog.pid`"
    kill `cat tmp/nxlog.pid`
    sleep 1
    if test -f tmp/nxlog.pid; then
	kill -9 `cat tmp/nxlog.pid`;
    fi
fi

./tester.pl modules/input/udp/im_udp.txt || FAILED="im_udp (ignored)"
./tester.pl modules/input/ssl/im_ssl.txt || FAILED="$FAILED im_ssl" 
./tester.pl modules/input/tcp/im_tcp.txt || FAILED="$FAILED im_tcp"
./tester.pl modules/input/uds/im_uds.txt || FAILED="$FAILED im_uds"
./tester.pl modules/input/file/im_file.txt || FAILED="$FAILED im_file"
./tester.pl modules/output/file/om_file.txt || FAILED="$FAILED om_file"
./tester.pl modules/output/ssl/om_ssl.txt || FAILED="$FAILED om_ssl"
./tester.pl modules/output/tcp/om_tcp.txt || FAILED="$FAILED om_tcp"
./tester.pl modules/output/udp/om_udp.txt || FAILED="$FAILED om_udp (ignored)"
./tester.pl modules/processor/filter/pm_filter.txt || FAILED="$FAILED pm_filter"
./tester.pl modules/processor/transformer/pm_transformer.txt || FAILED="$FAILED pm_transformer"
./tester.pl modules/processor/norepeat/pm_norepeat.txt || FAILED="$FAILED pm_norepeat"
./tester.pl modules/processor/pattern/pattern.txt || FAILED="$FAILED pm_pattern"
./tester.pl modules/processor/evcorr/pm_evcorr.txt || FAILED="$FAILED pm_evcorr"
./tester.pl modules/extension/multiline/xm_multiline.txt || FAILED="$FAILED xm_multiline"
./tester.pl modules/extension/perl/xm_perl.txt || FAILED="$FAILED xm_perl"
./tester.pl modules/extension/kvp/xm_kvp.txt || FAILED="$FAILED xm_kvp"
./tester.pl modules/extension/wtmp/xm_wtmp.txt || FAILED="$FAILED xm_wtmp"
if test "x$FAILED" != "x"; then
    echo "Failed tests: $FAILED";
    exit 1;
fi
