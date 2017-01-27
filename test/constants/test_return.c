#include "greatest.h"
#include "moonchild.h"
#include "monitor.h"

moon_closure * create_main_closure();

TEST should_return_42(void) {
    moon_reference buf_ref;
    moon_closure * closure = create_main_closure();

    moon_run_closure(closure, NULL);
    moon_create_value_copy(&buf_ref, &(closure->result));

    ASSERT_EQ(TRUE, MOON_IS_INT(&buf_ref));
    ASSERT_EQ(42, MOON_AS_INT(&buf_ref)->val);

    moon_delete_value((moon_value *) closure);
    GREATEST_PASS();

    if (buf_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf_ref.value_addr);
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

SUITE(ReturnConst) {
    RUN_TEST(should_return_42);
    RUN_TEST(should_not_leak_at_reload);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    moon_init();
    RUN_SUITE(ReturnConst);
    GREATEST_MAIN_END();
}
