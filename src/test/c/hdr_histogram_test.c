/**
 * hdrh_test.c
 * Written by Michael Barker and released to the public domain,
 * as explained at http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <stdio.h>
#include <hdr_histogram.h>
#include "minunit.h"

bool compare_percentile(int64_t a, double b, double variation)
{
    return fabs(a - b) <= b * variation;
}

int tests_run = 0;

static struct hdr_histogram* raw_histogram = NULL;
static struct hdr_histogram* cor_histogram = NULL;

static void load_histograms()
{
    int i;
    if (raw_histogram)
    {
        free(raw_histogram);
    }

    hdrh_alloc(100000000, 3, &raw_histogram);

    if (cor_histogram)
    {
        free(cor_histogram);
    }

    hdrh_alloc(100000000, 3, &cor_histogram);

    for (i = 0; i < 10000; i++)
    {
        hdrh_record_value(raw_histogram, 1000L);
        hdrh_record_corrected_value(cor_histogram, 1000L, 10000L);
    }

    hdrh_record_value(raw_histogram, 100000000L);
    hdrh_record_corrected_value(cor_histogram, 100000000L, 10000L);
}

static char* test_create()
{
    struct hdr_histogram* h = NULL;
    int r = hdrh_alloc(36000000, 4, &h);

    mu_assert("Failed to allocate hdr_histogram", r == 0);
    mu_assert("Failed to allocate hdr_histogram", h != 0);

    free(h);

    return 0;
}

static char* test_invalid_significant_figures()
{
    struct hdr_histogram* h = NULL;

    int r = hdrh_alloc(36000000, 2, &h);
    mu_assert("Result was not -1",      r == -1);
    mu_assert("Histogram was not null", h == 0);

    return 0;
}

static char* test_total_count()
{
    load_histograms();

    mu_assert("Total raw count != 10001",       raw_histogram->total_count == 10001);
    mu_assert("Total corrected count != 20000", cor_histogram->total_count == 20000);

    return 0;
}

static char* test_get_max_value()
{
    load_histograms();


    int64_t actual_raw_max = hdrh_max(raw_histogram);
    mu_assert("hdrh_max(raw_histogram) != 100000000L",
              hdrh_values_are_equivalent(raw_histogram, actual_raw_max, 100000000L));
    int64_t actual_cor_max = hdrh_max(cor_histogram);
    mu_assert("hdrh_max(cor_histogram) != 100000000L",
              hdrh_values_are_equivalent(cor_histogram, actual_cor_max, 100000000L));

    return 0;
}

static char* test_get_min_value()
{
    load_histograms();

    mu_assert("hdrh_min(raw_histogram) != 1000", hdrh_min(raw_histogram) == 1000L);
    mu_assert("hdrh_min(cor_histogram) != 1000", hdrh_min(cor_histogram) == 1000L);

    return 0;
}

static char* test_percentiles()
{
    load_histograms();

    mu_assert("Value at 30% not 1000.0",
              compare_percentile(hdrh_value_at_percentile(raw_histogram, 30.0), 1000.0, 0.001));
    mu_assert("Value at 99% not 1000.0",
              compare_percentile(hdrh_value_at_percentile(raw_histogram, 99.0), 1000.0, 0.001));
    mu_assert("Value at 99.99% not 1000.0",
              compare_percentile(hdrh_value_at_percentile(raw_histogram, 99.99), 1000.0, 0.001));
    mu_assert("Value at 99.999% not 100000000.0",
              compare_percentile(hdrh_value_at_percentile(raw_histogram, 99.999), 100000000.0, 0.001));
    mu_assert("Value at 100% not 100000000.0",
              compare_percentile(hdrh_value_at_percentile(raw_histogram, 100.0), 100000000.0, 0.001));

    mu_assert("Value at 30% not 1000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 30.0), 1000.0, 0.001));
    mu_assert("Value at 50% not 1000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 50.0), 1000.0, 0.001));
    mu_assert("Value at 75% not 50000000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 75.0), 50000000.0, 0.001));
    mu_assert("Value at 90% not 80000000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 90.0), 80000000.0, 0.001));
    mu_assert("Value at 99% not 98000000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 99.0), 98000000.0, 0.001));
    mu_assert("Value at 99.999% not 100000000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 99.999), 100000000.0, 0.001));
    mu_assert("Value at 100% not 100000000.0",
              compare_percentile(hdrh_value_at_percentile(cor_histogram, 100.0), 100000000.0, 0.001));

    return 0;
}

static char* test_recorded_values()
{
    load_histograms();

    /*
    int index = 0;
    // Iterate raw data by stepping through every value that has a count recorded:
    for (HistogramIterationValue v : rawHistogram.getHistogramData().recordedValues()) {
        long countAddedInThisBucket = v.getCountAddedInThisIterationStep();
        if (index == 0) {
            Assert.assertEquals("Raw recorded value bucket # 0 added a count of 10000",
                    10000, countAddedInThisBucket);
        } else {
            Assert.assertEquals("Raw recorded value bucket # " + index + " added a count of 1",
                    1, countAddedInThisBucket);
        }
        index++;
    }
    Assert.assertEquals(2, index);
    */

    struct hdrh_recorded_iter iter;
    hdrh_recorded_iter_init(&iter, raw_histogram);

    int index = 0;
    while (hdrh_recorded_iter_next(&iter))
    {
        int64_t count_added_in_this_bucket = iter.count_added_in_this_iteration_step;
        if (index == 0)
        {
            mu_assert("Value at 0 is not 10000", count_added_in_this_bucket == 10000);
        }
        else
        {
            mu_assert("Value at 1 is not 1", count_added_in_this_bucket == 1);
        }

        index++;
    }

    mu_assert("Should have encountered 2 values", index == 2);

    /*
    index = 0;
    long totalAddedCounts = 0;
    // Iterate data using linear buckets of 1 sec each.
    for (HistogramIterationValue v : histogram.getHistogramData().recordedValues()) {
        long countAddedInThisBucket = v.getCountAddedInThisIterationStep();
        if (index == 0) {
            Assert.assertEquals("Recorded bucket # 0 [" +
                    v.getValueIteratedFrom() + ".." + v.getValueIteratedTo() +
                    "] added a count of 10000",
                    10000, countAddedInThisBucket);
        }
        Assert.assertTrue("The count in recorded bucket #" + index + " is not 0",
                v.getCountAtValueIteratedTo() != 0);
        Assert.assertEquals("The count in recorded bucket #" + index +
                " is exactly the amount added since the last iteration ",
                v.getCountAtValueIteratedTo(), v.getCountAddedInThisIterationStep());
        totalAddedCounts += v.getCountAddedInThisIterationStep();
        index++;
    }
    Assert.assertEquals("Total added counts should be 20000", 20000, totalAddedCounts);
    */

    return 0;
}


static struct mu_result all_tests()
{
    mu_run_test(test_create);
    mu_run_test(test_invalid_significant_figures);
    mu_run_test(test_total_count);
    mu_run_test(test_get_min_value);
    mu_run_test(test_get_max_value);
    mu_run_test(test_percentiles);
    mu_run_test(test_recorded_values);

    mu_ok;
}

int main(int argc, char **argv)
{
    struct mu_result result = all_tests();

    if (result.message != 0)
    {
        printf("%s(): %s\n", result.test, result.message);
    }
    else
    {
        printf("ALL TESTS PASSED\n");
    }

    printf("Tests run: %d\n", tests_run);

    return result.message != 0;
}
