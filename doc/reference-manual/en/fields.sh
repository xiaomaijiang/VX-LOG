# Remove "-P 8" to disable parallel execution
echo ../../../src/modules/*/*/*-fields.xml \
    ../../../src/common/core-fields.xml | \
    xargs -n 1 -P 8 ../fields.pl -lang en
