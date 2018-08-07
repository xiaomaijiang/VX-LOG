# Remove "-P 8" to disable parallel execution
echo ../../../src/modules/*/*/*-api.xml \
    ../../../src/common/core-api.xml | \
    xargs -n 1 -P 8 ../apidoc.pl -lang en
