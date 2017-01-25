#include "greatest.h"
#include "moonchild.h"

moon_closure * create_main_closure();

TEST should_not_crash(void) {
    moon_closure * closure = create_main_closure();
    moon_run_closure(closure, NULL);
    moon_delete_value((moon_value *) closure);
    GREATEST_PASS();
}

SUITE(Empty) {
    RUN_TEST(should_not_crash);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    moon_init();
    RUN_SUITE(Empty);
    GREATEST_MAIN_END();
}
