#!/bin/sh
./tester.pl conftest.txt
(cd ../doc/reference-manual/config-examples && ./example-tester.sh) || (echo "testing failed in doc/reference-manual/config-examples" && exit 1)
