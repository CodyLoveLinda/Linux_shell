//sysfs functions for qdma_cash unit tests
//only include this file in qdma_mod.c
extern ssize_t show_libqdma_cash_test_cases(struct device *, struct device_attribute *, char *);
extern ssize_t run_libqdma_cash_test_case(struct device *, struct device_attribute *, const char *, size_t );
