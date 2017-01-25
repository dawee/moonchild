#include "greatest.h"
#include "moonchild.h"
#include "monitor.h"

moon_closure * create_main_closure();

TEST should_not_crash(void) {
    moon_closure * closure = create_main_closure();
    moon_run_closure(closure, NULL);
    moon_delete_value((moon_value *) closure);
    GREATEST_PASS();
}

TEST should_not_leak_at_reload(void) {
    int memo;

    moon_closure * closure = create_main_closure();
    moon_run_closure(closure, NULL);

    memo = moon_monitor_get_total();
    moon_run_closure(closure, NULL);
    ASSERT_EQ(memo, moon_monitor_get_total());

    moon_delete_value((moon_value *) closure);
    GREATEST_PASS();
}

SUITE(Empty) {
    RUN_TEST(should_not_crash);
    RUN_TEST(should_not_leak_at_reload);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    moon_init();
    RUN_SUITE(Empty);
    GREATEST_MAIN_END();
}
